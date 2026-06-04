import fs from 'node:fs';
import http from 'node:http';
import net from 'node:net';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync, spawn } from 'node:child_process';
import { createRequire } from 'node:module';
import { ensureSignalingTlsFiles } from './prepare_signaling_tls.mjs';
import { resolveChromiumExecutable } from './browser_runtime_helpers.mjs';
import {
  acquireNetemGuard,
  clearRootQdisc,
  preflightNetemGuard,
  releaseNetemGuard,
} from './netem_guard.mjs';

const require = createRequire(import.meta.url);
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

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

function startLocalSfu(signalingPort, webRtcServerPort) {
  ensureSignalingTlsFiles();
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
    { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'] },
  );

  let log = '';
  const collect = chunk => {
    log += chunk.toString();
    const lines = log.split('\n');
    if (lines.length > 300) {
      log = lines.slice(-300).join('\n');
    }
  };
  child.stdout.on('data', collect);
  child.stderr.on('data', collect);
  return { child, getLog: () => log };
}

function stopChild(child, timeoutMs = 3000) {
  return new Promise(resolve => {
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

function startStaticServer() {
  const html = `<!doctype html>
<html>
<body>
  <script src="/mediasoup-client.bundle.js"></script>
  <script src="/mediasoup-room-client.js"></script>
</body>
</html>`;

  const server = http.createServer((req, res) => {
    if (req.url === '/mediasoup-client.bundle.js') {
      res.writeHead(200, { 'content-type': 'application/javascript' });
      res.end(fs.readFileSync(path.join(repoRoot, 'public', 'mediasoup-client.bundle.js')));
      return;
    }
    if (req.url === '/mediasoup-room-client.js') {
      res.writeHead(200, { 'content-type': 'application/javascript' });
      res.end(fs.readFileSync(path.join(repoRoot, 'public', 'mediasoup-room-client.js')));
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
  return () => clearRootQdisc('lo');
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
  return () => clearRootQdisc('lo');
}

async function createPage(browser, baseUrl) {
  const page = await browser.newPage();
  const consoleLines = [];
  page.on('console', message => consoleLines.push(`[${message.type()}] ${message.text()}`));
  page.on('pageerror', error => consoleLines.push(`[pageerror] ${error.message}`));
  await page.goto(baseUrl, { waitUntil: 'load', timeout: 60000 });
  await page.waitForFunction(() => !!window.MediasoupRoomClient, { timeout: 10000 });
  return { page, consoleLines };
}

async function installHarness(page) {
  await page.evaluate(() => {
    const createAudioTrack = async () => {
      const oscillatorContext = new AudioContext();
      const oscillator = oscillatorContext.createOscillator();
      const gain = oscillatorContext.createGain();
      const destination = oscillatorContext.createMediaStreamDestination();
      oscillator.type = 'sine';
      oscillator.frequency.value = 660;
      gain.gain.value = 0.05;
      oscillator.connect(gain);
      gain.connect(destination);
      oscillator.start();
      const track = destination.stream.getAudioTracks()[0];
      track.__sdkCleanup = () => {
        try { oscillator.stop(); } catch {}
        oscillator.disconnect();
        gain.disconnect();
        destination.disconnect?.();
        oscillatorContext.close().catch(() => {});
      };
      return track;
    };

    const waitUntil = async (predicate, timeoutMs = 15000, intervalMs = 50) => {
      const deadline = Date.now() + timeoutMs;
      let last = null;
      while (Date.now() < deadline) {
        last = await predicate();
        if (last) return last;
        await new Promise(resolve => setTimeout(resolve, intervalMs));
      }
      throw new Error(`waitUntil timeout last=${JSON.stringify(last)}`);
    };

    window.__roomClientSdkHarness = {
      clients: new Map(),
      events: [],

      async create(name, options) {
        const device = new window.mediasoupClient.Device();
        const handler = device._handlerFactory();
        const nativeCaps = await handler.getNativeRtpCapabilities();
        handler.close();
        const client = new window.MediasoupRoomClient({
          ...options,
          rtpCapabilities: nativeCaps,
          onTrack: info => this.events.push({ type: 'track', name, info: this.serializeTrack(info) }),
          onTrackClosed: info => this.events.push({ type: 'trackClosed', name, info: this.serializeTrack(info) }),
          onStateChange: info => this.events.push({ type: 'state', name, info }),
        });
        this.clients.set(name, client);
        return true;
      },

      async createTalkback(name, roomClientName) {
        const roomClient = this.clients.get(roomClientName);
        const talkback = new window.TalkbackClient(roomClient, {
          onStateChange: info => this.events.push({ type: 'talkback-state', name, info }),
          onTargetsChanged: info => this.events.push({ type: 'talkback-targets', name, info }),
        });
        this.clients.set(name, talkback);
        return true;
      },

      serializeTrack(info) {
        return {
          peerId: info.peerId,
          producerId: info.producerId,
          consumerId: info.consumerId,
          kind: info.kind,
          source: info.source,
          appData: info.appData || {},
          hasStream: Boolean(info.stream),
          trackReadyState: info.track?.readyState || null,
        };
      },

      async join(name) {
        await this.clients.get(name).join();
        return true;
      },

      async publishAudio(name, appData) {
        const track = await createAudioTrack();
        const [producer] = await this.clients.get(name).publish({
          audio: {
            track,
            appData,
          },
        });
        return {
          id: producer.id,
          kind: producer.kind,
          appData: producer.appData || {},
        };
      },

      async waitForTrack(name, expectedSource) {
        return await waitUntil(() => {
          const found = this.events.find(event =>
            event.type === 'track' &&
            event.name === name &&
            event.info?.source === expectedSource
          );
          return found || null;
        });
      },

      async waitForTrackClosed(name, producerId) {
        return await waitUntil(() => {
          const found = this.events.find(event =>
            event.type === 'trackClosed' &&
            event.name === name &&
            event.info?.producerId === producerId
          );
          return found || null;
        });
      },

      async waitForTalkbackState(name, expectedState) {
        return await waitUntil(() => {
          const found = [...this.events].reverse().find(event =>
            event.type === 'talkback-state' &&
            event.name === name &&
            event.info?.state === expectedState
          );
          return found || null;
        }, 15000, 100);
      },

      async forceSocketClose(name) {
        const client = this.clients.get(name);
        client.ws?.close?.();
        return true;
      },

      async requestStats(name) {
        const client = this.clients.get(name);
        return await client.request('getStats', { peerId: client.peerId });
      },

      async openTalkback(name, targetPeerId) {
        await this.clients.get(name).openMic(targetPeerId);
        return true;
      },

      async closeTalkback(name) {
        await this.clients.get(name).closeMic();
        return true;
      },

      async waitForState(name, expectedState) {
        return await waitUntil(() => {
          const found = this.events.find(event =>
            event.type === 'state' &&
            event.name === name &&
            event.info?.state === expectedState
          );
          return found || null;
        }, 20000, 100);
      },

      async waitForJoined(name, expectedJoinMode) {
        return await waitUntil(() => {
          const found = [...this.events].reverse().find(event =>
            event.type === 'state' &&
            event.name === name &&
            event.info?.state === 'joined' &&
            (!expectedJoinMode || event.info?.data?.joinMode === expectedJoinMode)
          );
          return found || null;
        }, 20000, 100);
      },

      async waitForTransportState(name, expectedState) {
        return await waitUntil(() => {
          const found = [...this.events].reverse().find(event =>
            event.type === 'state' &&
            event.name === name &&
            event.info?.state === 'transport-state' &&
            event.info?.data?.connectionState === expectedState
          );
          return found || null;
        }, 20000, 100);
      },

      async unpublish(name, producerId) {
        await this.clients.get(name).unpublish({ producerId });
        return true;
      },

      async closeAll() {
        for (const client of this.clients.values()) {
          await client.leave();
        }
        this.clients.clear();
      },

      getEvents() {
        return this.events.slice();
      },
    };
  });
}

async function run() {
  const staticServer = await startStaticServer();
  const baseUrl = `http://127.0.0.1:${staticServer.address().port}/`;
  const signalingPort = await allocatePort();
  const webRtcServerPort = await allocatePort();
  const sfu = startLocalSfu(signalingPort, webRtcServerPort);
  let browser = null;
  let page = null;
  let consoleLines = [];
  let netemGuard = null;

  try {
    preflightNetemGuard({ iface: 'lo' });
    netemGuard = await acquireNetemGuard({ label: 'browser-room-client-sdk', iface: 'lo' });
    await waitForPort(signalingPort);
    browser = await puppeteer.launch({
      executablePath: resolveChromiumExecutable(),
      headless: true,
      protocolTimeout: 120000,
      args: [
        '--no-sandbox',
        '--ignore-certificate-errors',
        '--allow-insecure-localhost',
        '--use-fake-device-for-media-stream',
        '--use-fake-ui-for-media-stream',
        '--autoplay-policy=no-user-gesture-required',
      ],
    });

    ({ page, consoleLines } = await createPage(browser, baseUrl));
    await installHarness(page);

    const roomId = `sdk_room_${Date.now()}`;
    const wsUrl = `wss://127.0.0.1:${signalingPort}/ws`;

    await page.evaluate((url, room) => window.__roomClientSdkHarness.create('sub', {
      wssUrl: url,
      roomId: room,
      peerId: 'sdk-sub',
      displayName: 'sdk-sub',
    }), wsUrl, roomId);
    await page.evaluate(() => window.__roomClientSdkHarness.join('sub'));

    await page.evaluate((url, room) => window.__roomClientSdkHarness.create('restricted', {
      wssUrl: url,
      roomId: room,
      peerId: 'sdk-restricted',
      displayName: 'sdk-restricted',
      audioRole: 'audio-restricted',
    }), wsUrl, roomId);
    await page.evaluate(() => window.__roomClientSdkHarness.join('restricted'));

    await page.evaluate((url, room) => window.__roomClientSdkHarness.create('pub', {
      wssUrl: url,
      roomId: room,
      peerId: 'sdk-pub',
      displayName: 'sdk-pub',
    }), wsUrl, roomId);
    await page.evaluate(() => window.__roomClientSdkHarness.join('pub'));

    const published = await page.evaluate(() => window.__roomClientSdkHarness.publishAudio('pub', {
      source: 'audio',
      label: 'front-desk',
      lane: 2,
    }));

    const trackEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTrack('sub', 'audio'));
    if (!trackEvent?.info?.hasStream) {
      throw new Error(`missing remote stream in track event: ${JSON.stringify(trackEvent)}`);
    }
    if (trackEvent.info.kind !== 'audio') {
      throw new Error(`expected audio track event: ${JSON.stringify(trackEvent)}`);
    }
    if (trackEvent.info.appData?.label !== 'front-desk' || trackEvent.info.appData?.lane !== 2) {
      throw new Error(`appData was not preserved: ${JSON.stringify(trackEvent)}`);
    }

    await page.evaluate(() => window.__roomClientSdkHarness.createTalkback('talkback', 'pub'));
    await page.evaluate(() => window.__roomClientSdkHarness.openTalkback('talkback', 'sdk-restricted'));
    const talkbackOpenedEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTalkbackState('talkback', 'opened'));
    const restrictedTalkbackTrackEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTrack('restricted', 'talkback'));
    if (!talkbackOpenedEvent || !restrictedTalkbackTrackEvent) {
      throw new Error(`talkback open sequence missing: opened=${JSON.stringify(talkbackOpenedEvent)} track=${JSON.stringify(restrictedTalkbackTrackEvent)}`);
    }
    await page.evaluate(() => window.__roomClientSdkHarness.closeTalkback('talkback'));
    const talkbackClosedEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTalkbackState('talkback', 'idle'));

    await page.evaluate(producerId => window.__roomClientSdkHarness.unpublish('pub', producerId), published.id);
    const closedEvent = await page.evaluate(producerId => window.__roomClientSdkHarness.waitForTrackClosed('sub', producerId), published.id);
    if (!closedEvent?.info?.producerId || closedEvent.info.producerId !== published.id) {
      throw new Error(`trackClosed did not reference producerId=${published.id}: ${JSON.stringify(closedEvent)}`);
    }

    let clearTcpDrop = applyTcpDropWithTc(signalingPort);
    let replacedSessionEvent = null;
    let requestTimeoutReconnectEvent = null;
    try {
      await page.evaluate(() => window.__roomClientSdkHarness.requestStats('sub')).catch(() => {});
      await sleep(10_500);
      clearTcpDrop();
      clearTcpDrop = null;
      requestTimeoutReconnectEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForState('sub', 'reconnecting'));
      replacedSessionEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForJoined('sub', 'replaced-session'));
    } finally {
      if (clearTcpDrop) clearTcpDrop();
      clearTcpDrop = null;
      clearRootQdisc('lo');
    }
    if (!requestTimeoutReconnectEvent || !replacedSessionEvent) {
      throw new Error(`missing request-timeout recovery sequence: reconnecting=${JSON.stringify(requestTimeoutReconnectEvent)} replacedSession=${JSON.stringify(replacedSessionEvent)}`);
    }

    let clearDrop = applyUdpDropWithTc(webRtcServerPort);
    let disconnectedEvent = null;
    let recoveredTransportEvent = null;
    try {
      disconnectedEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTransportState('sub', 'disconnected'));
      clearDrop();
      clearDrop = null;
      recoveredTransportEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTransportState('sub', 'connected'));
    } finally {
      if (clearDrop) clearDrop();
      clearRootQdisc('lo');
    }
    if (!disconnectedEvent || !recoveredTransportEvent) {
      throw new Error(`transport reconnect sequence missing: disconnected=${JSON.stringify(disconnectedEvent)} connected=${JSON.stringify(recoveredTransportEvent)}`);
    }

    let clearSecondUdpDrop = applyUdpDropWithTc(webRtcServerPort);
    let replacedMediaBrokenEvent = null;
    let replacedMediaRecoveredEvent = null;
    try {
      replacedMediaBrokenEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTransportState('sub', 'disconnected'));
      clearSecondUdpDrop();
      clearSecondUdpDrop = null;
      replacedMediaRecoveredEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForTransportState('sub', 'connected'));
    } finally {
      if (clearSecondUdpDrop) clearSecondUdpDrop();
      clearRootQdisc('lo');
    }
    if (!replacedMediaBrokenEvent || !replacedMediaRecoveredEvent) {
      throw new Error(`replaced-session media restart sequence missing: disconnected=${JSON.stringify(replacedMediaBrokenEvent)} connected=${JSON.stringify(replacedMediaRecoveredEvent)}`);
    }

    await page.evaluate(() => window.__roomClientSdkHarness.forceSocketClose('pub'));
    const reconnectingEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForState('pub', 'reconnecting'));
    const rejoinedEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForJoined('pub', 'new-peer'));
    const reconnectedEvent = await page.evaluate(() => window.__roomClientSdkHarness.waitForState('pub', 'connected'));
    if (!reconnectingEvent || !rejoinedEvent || !reconnectedEvent) {
      throw new Error(`reconnect state sequence missing: reconnecting=${JSON.stringify(reconnectingEvent)} joined=${JSON.stringify(rejoinedEvent)} connected=${JSON.stringify(reconnectedEvent)}`);
    }

    const events = await page.evaluate(() => window.__roomClientSdkHarness.getEvents());
    console.log(JSON.stringify({
      ok: true,
      roomId,
      published,
      trackEvent,
      talkbackOpenedEvent,
      restrictedTalkbackTrackEvent,
      talkbackClosedEvent,
      closedEvent,
      requestTimeoutReconnectEvent,
      replacedSessionEvent,
      disconnectedEvent,
      recoveredTransportEvent,
      replacedMediaBrokenEvent,
      replacedMediaRecoveredEvent,
      reconnectingEvent,
      rejoinedEvent,
      reconnectedEvent,
      eventCount: events.length,
      consoleTail: consoleLines.slice(-20),
    }, null, 2));
  } catch (error) {
    const events = page
      ? await page.evaluate(() => window.__roomClientSdkHarness?.getEvents?.() || []).catch(() => [])
      : [];
    console.log(JSON.stringify({
      ok: false,
      error: error.message,
      consoleTail: consoleLines.slice(-30),
      sfuLogTail: sfu.getLog().split('\n').slice(-60),
      events,
    }, null, 2));
    process.exitCode = 1;
  } finally {
    if (page) {
      await page.evaluate(() => window.__roomClientSdkHarness?.closeAll?.()).catch(() => {});
      await page.close().catch(() => {});
    }
    if (browser) {
      await browser.close().catch(() => {});
    }
    if (netemGuard) {
      await releaseNetemGuard(netemGuard).catch(() => {});
    }
    clearRootQdisc('lo');
    await stopChild(sfu.child);
    await stopStaticServer(staticServer);
  }
}

await run();
