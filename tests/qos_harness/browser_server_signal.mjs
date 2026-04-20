import fs from 'node:fs';
import http from 'node:http';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const chromiumPath = '/usr/lib64/chromium-browser/headless_shell';
let signalingPort = 0;
const PUPPETEER_PROTOCOL_TIMEOUT_MS = 10 * 60 * 1000;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function allocatePort() {
  const net = await import('node:net');
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
    entryPoints: [path.join(__dirname, 'browser', 'server-signaling-entry.js')],
    outfile,
    bundle: true,
    platform: 'browser',
    format: 'iife',
    target: ['chrome120'],
  });
  return outfile;
}

function startStaticServer(bundlePath) {
  const html = `<!doctype html><html><body><script src="/bundle.js"></script></body></html>`;
  const server = http.createServer((req, res) => {
    if (req.url === '/bundle.js') {
      res.writeHead(200, { 'content-type': 'application/javascript' });
      res.end(fs.readFileSync(bundlePath));
      return;
    }
    res.writeHead(200, { 'content-type': 'text/html' });
    res.end(html);
  });

  return new Promise(resolve => {
    server.listen(0, '127.0.0.1', () => resolve(server));
  });
}

function getServerUrl(server) {
  const address = server.address();
  return `http://127.0.0.1:${address.port}`;
}

function startSfu() {
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${signalingPort}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1',
      '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0',
      '--redisPort=1',
      '--noRedisRequired',
    ],
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
    }
  );
  child.stdout.on('data', chunk => process.stdout.write(chunk));
  child.stderr.on('data', chunk => process.stderr.write(chunk));
  return child;
}

function stopSfu(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child.pid || child.exitCode !== null) {
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

async function waitForPort(port, timeoutMs = 7000) {
  const net = await import('node:net');
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: '127.0.0.1', port }, () => {
          socket.destroy();
          resolve();
        });
        socket.once('error', reject);
      });
      return;
    } catch {
      await sleep(100);
    }
  }
  throw new Error(`port ${port} did not become ready`);
}

async function launchBrowser() {
  return puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: PUPPETEER_PROTOCOL_TIMEOUT_MS,
    args: ['--no-sandbox'],
  });
}

async function runScenario() {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-server-browser-'));
  const bundlePath = buildBundle(tmpDir);
  signalingPort = await allocatePort();
  const staticServer = await startStaticServer(bundlePath);
  const sfu = startSfu();
  const browser = await launchBrowser();
  const page = await browser.newPage();

  try {
    await waitForPort(signalingPort);
    await page.goto(getServerUrl(staticServer), { waitUntil: 'load' });
    await page.evaluate(
      (port) =>
        window.__qosServerHarness.init(
          `ws://127.0.0.1:${port}/ws`,
          'browser_server_room',
          'alice'
        ),
      signalingPort
    );

    const runtime = await page.evaluate(() =>
      window.__qosServerHarness.setPolicy({
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1500,
        snapshotIntervalMs: 4000,
        allowAudioOnly: false,
        allowVideoPause: false,
        profiles: {
          camera: 'conservative',
          screenShare: 'clarity-first',
          audio: 'speech-first',
        },
      })
    );
    if (
      runtime.sampleIntervalMs !== 1500 ||
      runtime.snapshotIntervalMs !== 4000 ||
      runtime.allowAudioOnly !== false ||
      runtime.allowVideoPause !== false
    ) {
      throw new Error(`runtime policy not applied: ${JSON.stringify(runtime)}`);
    }

    await page.evaluate(() =>
      window.__qosServerHarness.publishSnapshot({
        timestampMs: 1000,
        qualityLimitationReason: 'bandwidth',
        bytesSent: 0,
        packetsSent: 100,
        packetsLost: 50,
        targetBitrateBps: 900000,
        roundTripTimeMs: 400,
        jitterMs: 80,
      }, 5)
    );
    let override;
    try {
      override = await page.evaluate(() =>
        window.__qosServerHarness.waitOverride([
          'server_auto_poor',
          'server_auto_lost',
          'server_room_pressure',
        ])
      );
    } catch (error) {
      const controllerState = await page.evaluate(() => window.__qosServerHarness.getControllerState());
      const serverStats = await page.evaluate(() => window.__qosServerHarness.getServerStats());
      throw new Error(
        `qosOverride notification not received; controller=${JSON.stringify(controllerState)} serverStats=${JSON.stringify(serverStats)} original=${error}`
      );
    }
    if (
      override.notif.data.reason !== 'server_auto_poor' &&
      override.notif.data.reason !== 'server_auto_lost'
    ) {
      throw new Error(`unexpected override: ${JSON.stringify(override)}`);
    }

    await page.evaluate(() =>
      window.__qosServerHarness.sendRawClientSnapshot({
        schema: 'mediasoup.qos.client.v1',
        seq: 999,
        tsMs: 1712736020000,
        peerState: {
          mode: 'audio-video',
          quality: 'good',
          stale: false,
        },
        tracks: [
          {
            localTrackId: 'camera-main',
            producerId: 'browser-signal-producer',
            kind: 'video',
            source: 'camera',
            state: 'stable',
            reason: 'network',
            quality: 'good',
            ladderLevel: 0,
            signals: {
              sendBitrateBps: 800000,
              targetBitrateBps: 900000,
              lossRate: 0,
              rttMs: 120,
              jitterMs: 8,
              frameWidth: 640,
              frameHeight: 360,
              framesPerSecond: 24,
              qualityLimitationReason: 'none',
            },
          },
        ],
      })
    );
    let clearOverride;
    try {
      clearOverride = await page.evaluate(() =>
        window.__qosServerHarness.waitOverride([
          'server_auto_clear',
          'server_room_pressure_clear',
        ])
      );
    } catch (error) {
      const controllerState = await page.evaluate(() => window.__qosServerHarness.getControllerState());
      const serverStats = await page.evaluate(() => window.__qosServerHarness.getServerStats());
      throw new Error(
        `qosOverride clear not received; controller=${JSON.stringify(controllerState)} serverStats=${JSON.stringify(serverStats)} original=${error}`
      );
    }
    if (
      clearOverride.notif.data.reason !== 'server_auto_clear' &&
      clearOverride.notif.data.reason !== 'server_room_pressure_clear'
    ) {
      throw new Error(`unexpected clear override: ${JSON.stringify(clearOverride)}`);
    }

    console.log('browser_server_signal passed');
  } finally {
    await browser.close();
    await new Promise(resolve => staticServer.close(resolve));
    await stopSfu(sfu);
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

runScenario().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
