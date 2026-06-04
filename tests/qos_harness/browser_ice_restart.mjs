/**
 * Browser runner for ICE restart recovery.
 *
 * Scenarios:
 *   1. Start a local SFU and a real Chromium mediasoup-client page.
 *   2. Publish canvas video and subscribe to it over WebRTC/UDP.
 *   3. Drop UDP packets for the SFU WebRTC port.
 *   4. Wait for browser transport state to leave connected, then hold a 10s
 *      client grace window.
 *   5. Restore UDP, call signaling restartIce for send/recv transports, apply
 *      transport.restartIce({ iceParameters }) in the browser.
 *   6. Assert transports reconnect and RTP counters keep increasing.
 *   7. Separately verify signaling TCP/WSS cut preserves media, reconnecting
 *      with the same peer returns replaced-session, and a later media failure
 *      can recover by restartIce on the preserved transports.
 */
import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import net from 'node:net';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { createRequire } from 'node:module';
import { ensureSignalingTlsFiles } from './prepare_signaling_tls.mjs';
import { resolveChromiumExecutable } from './browser_runtime_helpers.mjs';
import {
  acquireNetemGuard,
  preflightNetemGuard,
  releaseNetemGuard,
  clearRootQdisc,
} from './netem_guard.mjs';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const GRACE_WINDOW_MS = Number(process.env.ICE_RESTART_GRACE_MS || 10000);
const REPEAT_COUNT = Number(process.env.ICE_RESTART_REPEAT || 1);

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function allocatePort() {
  return await new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(error => error ? reject(error) : resolve(port));
    });
  });
}

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'ice-restart-entry.js')],
    outfile,
    bundle: true,
    nodePaths: [path.join(__dirname, 'node_modules')],
    platform: 'browser',
    format: 'iife',
    target: ['chrome120'],
  });
  return outfile;
}

function startStaticServer(bundlePath) {
  const html = '<!doctype html><html><body><script src="/bundle.js"></script></body></html>';
  const server = http.createServer((req, res) => {
    if (req.url === '/bundle.js') {
      res.writeHead(200, { 'content-type': 'application/javascript' });
      res.end(fs.readFileSync(bundlePath));
      return;
    }

    res.writeHead(200, { 'content-type': 'text/html' });
    res.end(html);
  });

  return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

function stopStaticServer(server) {
  return new Promise(resolve => server.close(resolve));
}

async function waitForPort(port, timeoutMs = 10000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: '127.0.0.1', port }, () => {
          socket.destroy();
          resolve();
        });
        socket.once('error', reject);
        socket.setTimeout(500, () => {
          socket.destroy();
          reject(new Error('timeout'));
        });
      });
      return;
    } catch {
      await sleep(100);
    }
  }

  throw new Error(`port ${port} did not become ready`);
}

function startSfu(signalingPort, webRtcServerPort) {
  ensureSignalingTlsFiles();

  const env = {
    ...process.env,
    MEDIASOUP_WORKER_LOG_LEVEL: 'warn',
    MEDIASOUP_WORKER_LOG_TAGS: 'ice',
  };
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${signalingPort}`,
      '--localOnly',
      `--webRtcServerPort=${webRtcServerPort}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
    ],
    { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'], env }
  );

  let log = '';
  const collect = chunk => {
    log += chunk.toString();
    const lines = log.split('\n');
    if (lines.length > 400) {
      log = lines.slice(-400).join('\n');
    }
  };
  child.stdout.on('data', collect);
  child.stderr.on('data', collect);

  return { child, getLog: () => log };
}

function stopSfu(sfu, timeoutMs = 3000) {
  return new Promise(resolve => {
    const child = sfu?.child;
    if (!child || !child.pid || child.exitCode !== null) {
      resolve();
      return;
    }

    const timer = setTimeout(() => {
      try { child.kill('SIGKILL'); } catch {}
      resolve();
    }, timeoutMs);

    child.once('close', () => {
      clearTimeout(timer);
      resolve();
    });
    child.kill('SIGTERM');
  });
}

function assertStatsRecovered(result) {
  const { before, after } = result.growth;
  if (after.packetsReceived <= before.packetsReceived) {
    throw new Error(`packetsReceived did not grow after restartIce: ${JSON.stringify(result)}`);
  }
  if (after.bytesReceived <= before.bytesReceived) {
    throw new Error(`bytesReceived did not grow after restartIce: ${JSON.stringify(result)}`);
  }
  if (result.snapshot.sendState !== 'connected' || result.snapshot.recvState !== 'connected') {
    throw new Error(`transports did not reconnect: ${JSON.stringify(result.snapshot)}`);
  }
}

function assertRestartIceApplied(initial, restartPhases, restarted, options = {}) {
  const requiredRecoveryState = options.requiredRecoveryState || 'failed';

  if (restartPhases.sendIce.usernameFragment === initial.sendIceParameters?.usernameFragment) {
    throw new Error('send restartIce returned unchanged usernameFragment');
  }
  if (restartPhases.recvIce.usernameFragment === initial.recvIceParameters?.usernameFragment) {
    throw new Error('recv restartIce returned unchanged usernameFragment');
  }
  if (restartPhases.sendIce.password === initial.sendIceParameters?.password) {
    throw new Error('send restartIce returned unchanged password');
  }
  if (restartPhases.recvIce.password === initial.recvIceParameters?.password) {
    throw new Error('recv restartIce returned unchanged password');
  }

  const sendStates = restarted.snapshot.sendStates || [];
  const recvStates = restarted.snapshot.recvStates || [];

  if (!sendStates.includes(requiredRecoveryState) || !sendStates.includes('connected')) {
    throw new Error(`send transport state sequence does not show ${requiredRecoveryState}->connected: ${JSON.stringify(sendStates)}`);
  }
  if (!recvStates.includes(requiredRecoveryState) || !recvStates.includes('connected')) {
    throw new Error(`recv transport state sequence does not show ${requiredRecoveryState}->connected: ${JSON.stringify(recvStates)}`);
  }

  if (
    restartPhases.afterSendClientRestart.sendState === restartPhases.beforeClientRestart.sendState &&
    restartPhases.afterRecvClientRestart.recvState === restartPhases.beforeClientRestart.recvState
  ) {
    throw new Error(`client restartIce did not change transport states: ${JSON.stringify(restartPhases)}`);
  }
}

function applyUdpDropWithTc(port) {
  clearRootQdisc('lo');
  execFileSync('tc', ['qdisc', 'add', 'dev', 'lo', 'root', 'handle', '1:', 'prio'], { stdio: 'pipe' });
  execFileSync('tc', ['qdisc', 'add', 'dev', 'lo', 'parent', '1:3', 'handle', '30:', 'netem', 'loss', '100%'], { stdio: 'pipe' });
  execFileSync('tc', [
    'filter', 'add', 'dev', 'lo', 'protocol', 'ip', 'parent', '1:0', 'prio', '1',
    'u32', 'match', 'ip', 'protocol', '17', '0xff',
    'match', 'ip', 'sport', String(port), '0xffff',
    'flowid', '1:3',
  ], { stdio: 'pipe' });
  execFileSync('tc', [
    'filter', 'add', 'dev', 'lo', 'protocol', 'ip', 'parent', '1:0', 'prio', '1',
    'u32', 'match', 'ip', 'protocol', '17', '0xff',
    'match', 'ip', 'dport', String(port), '0xffff',
    'flowid', '1:3',
  ], { stdio: 'pipe' });

  return () => {
    clearRootQdisc('lo');
  };
}

function applyTcpDropWithTc(port) {
  clearRootQdisc('lo');
  execFileSync('tc', ['qdisc', 'add', 'dev', 'lo', 'root', 'handle', '1:', 'prio'], { stdio: 'pipe' });
  execFileSync('tc', ['qdisc', 'add', 'dev', 'lo', 'parent', '1:3', 'handle', '30:', 'netem', 'loss', '100%'], { stdio: 'pipe' });
  execFileSync('tc', [
    'filter', 'add', 'dev', 'lo', 'protocol', 'ip', 'parent', '1:0', 'prio', '1',
    'u32', 'match', 'ip', 'protocol', '6', '0xff',
    'match', 'ip', 'sport', String(port), '0xffff',
    'flowid', '1:3',
  ], { stdio: 'pipe' });
  execFileSync('tc', [
    'filter', 'add', 'dev', 'lo', 'protocol', 'ip', 'parent', '1:0', 'prio', '1',
    'u32', 'match', 'ip', 'protocol', '6', '0xff',
    'match', 'ip', 'dport', String(port), '0xffff',
    'flowid', '1:3',
  ], { stdio: 'pipe' });

  return () => {
    clearRootQdisc('lo');
  };
}

function assertSignalCutBehavior(before, after, failureMessage) {
  if (after.sendState !== 'connected' || after.recvState !== 'connected') {
    throw new Error(`signal cut unexpectedly broke media transport: before=${JSON.stringify(before)} after=${JSON.stringify(after)}`);
  }
  if (after.packetsReceived <= before.packetsReceived || after.bytesReceived <= before.bytesReceived) {
    throw new Error(`signal cut did not preserve inbound media growth: before=${JSON.stringify(before)} after=${JSON.stringify(after)}`);
  }
  if (!/timeout|failed|websocket/i.test(failureMessage || '')) {
    throw new Error(`signal cut did not produce a signaling failure message: ${failureMessage}`);
  }
}

function assertSignalReconnectBehavior(reconnect, growth) {
  if (reconnect.joinMode !== 'replaced-session') {
    throw new Error(`signal reconnect did not replace existing session: ${JSON.stringify(reconnect)}`);
  }
  if (reconnect.snapshot.sendState !== 'connected' || reconnect.snapshot.recvState !== 'connected') {
    throw new Error(`signal reconnect unexpectedly changed media state: ${JSON.stringify(reconnect.snapshot)}`);
  }
  if (growth.after.packetsReceived <= growth.before.packetsReceived ||
      growth.after.bytesReceived <= growth.before.bytesReceived) {
    throw new Error(`media did not keep growing after signal reconnect: ${JSON.stringify(growth)}`);
  }
}

async function createPage(browser, staticServer) {
  const page = await browser.newPage();
  const consoleLines = [];
  page.on('console', message => consoleLines.push(`[${message.type()}] ${message.text()}`));
  page.on('pageerror', error => consoleLines.push(`[pageerror] ${error.message}`));
  await page.goto(`http://127.0.0.1:${staticServer.address().port}/`, { waitUntil: 'load' });
  return { page, consoleLines };
}

async function runUdpMediaCase(page, signalingPort, webRtcServerPort) {
  const roomId = `ice_restart_udp_${Date.now()}`;
  const initial = await page.evaluate(
    (port, room) => window.__iceRestartHarness.init(`wss://127.0.0.1:${port}/ws`, room),
    signalingPort,
    roomId
  );
  console.log(`[udp:init] room=${roomId} send=${initial.sendState} recv=${initial.recvState} recvPackets=${initial.packetsReceived}`);

  let clearDrop = applyUdpDropWithTc(webRtcServerPort);
  console.log(`[udp:drop] UDP blocked with tc on loopback port ${webRtcServerPort}`);

  try {
    const impaired = await page.evaluate(() => window.__iceRestartHarness.waitForFailed(45000));
    console.log(`[udp:impaired] send=${impaired.sendState} recv=${impaired.recvState}`);

    await sleep(GRACE_WINDOW_MS);
    const afterGrace = await page.evaluate(() => window.__iceRestartHarness.snapshot());
    console.log(`[udp:grace] waitedMs=${GRACE_WINDOW_MS} send=${afterGrace.sendState} recv=${afterGrace.recvState}`);

    clearDrop();
    clearDrop = null;
    console.log('[udp:drop] UDP restored');

    const restartPhases = await page.evaluate(() => window.__iceRestartHarness.restartIceStepwise());
    const reconnected = await page.evaluate(() => window.__iceRestartHarness.waitForReconnected(20000));
    const growth = await page.evaluate(() => window.__iceRestartHarness.waitForGrowth(15000));
    const restarted = {
      ...restartPhases,
      snapshot: reconnected,
      growth,
    };
    assertRestartIceApplied(initial, restartPhases, restarted);
    assertStatsRecovered(restarted);

    return { roomId, initial, impaired, afterGrace, restartPhases, restarted };
  } finally {
    if (clearDrop) {
      clearDrop();
    }
    await page.evaluate(() => window.__iceRestartHarness.close());
  }
}

async function runSignalCutCase(page, signalingPort) {
  const roomId = `ice_restart_signal_${Date.now()}`;
  const initial = await page.evaluate(
    (port, room) => window.__iceRestartHarness.init(`wss://127.0.0.1:${port}/ws`, room),
    signalingPort,
    roomId
  );
  console.log(`[sig:init] room=${roomId} send=${initial.sendState} recv=${initial.recvState} recvPackets=${initial.packetsReceived}`);

  let clearDrop = applyTcpDropWithTc(signalingPort);
  console.log(`[sig:drop] TCP signaling blocked with tc on loopback port ${signalingPort}`);

  try {
    await sleep(GRACE_WINDOW_MS);
    const afterGrace = await page.evaluate(() => window.__iceRestartHarness.snapshot());
    let failureMessage = '';

    try {
      await page.evaluate(() => window.__iceRestartHarness.restartIceStepwise());
      throw new Error('restartIce unexpectedly succeeded during signaling cut');
    } catch (error) {
      failureMessage = String(error?.message || error);
    }

    assertSignalCutBehavior(initial, afterGrace, failureMessage);

    clearDrop();
    clearDrop = null;
    console.log('[sig:drop] TCP signaling restored');

    const reconnect = await page.evaluate(() => window.__iceRestartHarness.reconnectPublisherSignaling());
    const growthAfterReconnect = await page.evaluate(() => window.__iceRestartHarness.waitForGrowth(15000));
    assertSignalReconnectBehavior(reconnect, growthAfterReconnect);

    return { roomId, initial, afterGrace, failureMessage, reconnect, growthAfterReconnect };
  } finally {
    if (clearDrop) {
      clearDrop();
    }
    await page.evaluate(() => window.__iceRestartHarness.close());
  }
}

async function runSignalReconnectThenMediaRestartCase(page, signalingPort, webRtcServerPort) {
  const roomId = `ice_restart_replaced_media_${Date.now()}`;
  const initial = await page.evaluate(
    (port, room) => window.__iceRestartHarness.init(`wss://127.0.0.1:${port}/ws`, room),
    signalingPort,
    roomId
  );
  console.log(`[combo:init] room=${roomId} send=${initial.sendState} recv=${initial.recvState} recvPackets=${initial.packetsReceived}`);

  let clearSignalDrop = applyTcpDropWithTc(signalingPort);
  let clearMediaDrop = null;
  console.log(`[combo:sig-drop] TCP signaling blocked with tc on loopback port ${signalingPort}`);

  try {
    await sleep(GRACE_WINDOW_MS);
    const duringSignalCut = await page.evaluate(() => window.__iceRestartHarness.snapshot());

    clearSignalDrop();
    clearSignalDrop = null;
    console.log('[combo:sig-drop] TCP signaling restored');

    const reconnect = await page.evaluate(() => window.__iceRestartHarness.reconnectPublisherSignaling());
    const growthAfterReconnect = await page.evaluate(() => window.__iceRestartHarness.waitForGrowth(15000));
    assertSignalReconnectBehavior(reconnect, growthAfterReconnect);

    clearMediaDrop = applyUdpDropWithTc(webRtcServerPort);
    console.log(`[combo:udp-drop] UDP blocked with tc on loopback port ${webRtcServerPort}`);

    const impaired = await page.evaluate(() => window.__iceRestartHarness.waitForFailed(45000));
    console.log(`[combo:impaired] send=${impaired.sendState} recv=${impaired.recvState}`);

    await sleep(GRACE_WINDOW_MS);
    const afterMediaGrace = await page.evaluate(() => window.__iceRestartHarness.snapshot());
    console.log(`[combo:media-grace] waitedMs=${GRACE_WINDOW_MS} send=${afterMediaGrace.sendState} recv=${afterMediaGrace.recvState}`);

    clearMediaDrop();
    clearMediaDrop = null;
    console.log('[combo:udp-drop] UDP restored');

    const restartPhases = await page.evaluate(() => window.__iceRestartHarness.restartIceStepwise());
    const reconnected = await page.evaluate(() => window.__iceRestartHarness.waitForReconnected(20000));
    const growthAfterRestart = await page.evaluate(() => window.__iceRestartHarness.waitForGrowth(15000));
    const restarted = {
      ...restartPhases,
      snapshot: reconnected,
      growth: growthAfterRestart,
    };
    assertRestartIceApplied(initial, restartPhases, restarted, { requiredRecoveryState: 'disconnected' });
    assertStatsRecovered(restarted);

    return {
      roomId,
      initial,
      duringSignalCut,
      reconnect,
      growthAfterReconnect,
      impaired,
      afterMediaGrace,
      restartPhases,
      restarted,
    };
  } finally {
    if (clearMediaDrop) {
      clearMediaDrop();
    }
    if (clearSignalDrop) {
      clearSignalDrop();
    }
    await page.evaluate(() => window.__iceRestartHarness.close());
  }
}

async function run() {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'ice-restart-'));
  const bundlePath = buildBundle(tmpDir);
  const staticServer = await startStaticServer(bundlePath);
  const signalingPort = await allocatePort();
  const webRtcServerPort = await allocatePort();
  const sfu = startSfu(signalingPort, webRtcServerPort);
  let browser = null;
  let netemGuard = null;
  let currentPage = null;
  let currentConsoleLines = [];
  let currentScenario = null;

  try {
    preflightNetemGuard({ iface: 'lo' });
    netemGuard = await acquireNetemGuard({ label: 'browser-ice-restart', iface: 'lo' });
    await waitForPort(signalingPort);

    browser = await puppeteer.launch({
      executablePath: resolveChromiumExecutable(),
      headless: true,
      protocolTimeout: 120000,
      pipe: true,
      args: [
        '--no-sandbox',
        '--ignore-certificate-errors',
        '--allow-insecure-localhost',
        '--autoplay-policy=no-user-gesture-required',
      ],
    });

    const rounds = [];
    for (let round = 1; round <= REPEAT_COUNT; round += 1) {
      console.log(`[round:${round}/${REPEAT_COUNT}] start`);

      currentScenario = 'udp';
      ({ page: currentPage, consoleLines: currentConsoleLines } = await createPage(browser, staticServer));
      const udpCase = await runUdpMediaCase(currentPage, signalingPort, webRtcServerPort);
      await currentPage.close().catch(() => {});

      currentScenario = 'signal';
      ({ page: currentPage, consoleLines: currentConsoleLines } = await createPage(browser, staticServer));
      const signalCase = await runSignalCutCase(currentPage, signalingPort);
      await currentPage.close().catch(() => {});

      currentScenario = 'combo';
      ({ page: currentPage, consoleLines: currentConsoleLines } = await createPage(browser, staticServer));
      const signalReconnectThenMediaRestartCase =
        await runSignalReconnectThenMediaRestartCase(currentPage, signalingPort, webRtcServerPort);
      await currentPage.close().catch(() => {});

      rounds.push({
        round,
        udpCase,
        signalCase,
        signalReconnectThenMediaRestartCase,
        consoleTail: currentConsoleLines.slice(-20),
      });
      console.log(`[round:${round}/${REPEAT_COUNT}] done`);
      clearRootQdisc('lo');
    }

    console.log(JSON.stringify({
      ok: true,
      graceWindowMs: GRACE_WINDOW_MS,
      repeatCount: REPEAT_COUNT,
      webRtcServerPort,
      signalingPort,
      rounds,
      sfuLogTail: sfu.getLog().split('\n').slice(-80),
    }, null, 2));
  } catch (error) {
    let currentSnapshot = null;
    if (currentPage) {
      try { currentSnapshot = await currentPage.evaluate(() => window.__iceRestartHarness.snapshot()); } catch {}
    }
    console.log(JSON.stringify({
      ok: false,
      error: error.message,
      currentScenario,
      webRtcServerPort,
      signalingPort,
      graceWindowMs: GRACE_WINDOW_MS,
      repeatCount: REPEAT_COUNT,
      currentSnapshot,
      currentConsoleTail: currentConsoleLines.slice(-40),
      sfuLogTail: sfu.getLog().split('\n').slice(-120),
    }, null, 2));
    process.exitCode = 1;
  } finally {
    clearRootQdisc('lo');
    if (netemGuard) {
      await releaseNetemGuard(netemGuard);
    }
    if (browser) {
      await browser.close().catch(() => {});
    }
    await stopStaticServer(staticServer);
    await stopSfu(sfu);
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
