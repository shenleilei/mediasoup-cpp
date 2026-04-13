/**
 * Node runner for browser-side downlink QoS v2 verification.
 * Exercises publisher track clamp / clear generated from subscriber demand.
 */
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
const signalingPort = 14016;

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'downlink-v2-entry.js')],
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
  return new Promise(resolve => server.listen(0, '127.0.0.1', () => resolve(server)));
}

function getServerUrl(server) {
  return `http://127.0.0.1:${server.address().port}`;
}

function startSfu() {
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon', `--port=${signalingPort}`,
      '--workers=1', '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1', '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0', '--redisPort=1',
    ],
    { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'] },
  );
  child.stdout.on('data', chunk => process.stdout.write(chunk));
  child.stderr.on('data', chunk => process.stderr.write(chunk));
  return child;
}

function stopSfu(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child.pid || child.exitCode !== null) { resolve(); return; }
    const timer = setTimeout(() => { try { child.kill('SIGKILL'); } catch {} resolve(); }, timeoutMs);
    child.once('close', () => { clearTimeout(timer); resolve(); });
    child.kill('SIGTERM');
  });
}

async function waitForPort(port, timeoutMs = 7000) {
  const net = await import('node:net');
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const s = net.createConnection({ host: '127.0.0.1', port }, () => { s.destroy(); resolve(); });
        s.once('error', reject);
      });
      return;
    } catch { await sleep(100); }
  }
  throw new Error(`port ${port} not ready`);
}

async function runScenario() {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-downlink-v2-'));
  const bundlePath = buildBundle(tmpDir);
  const staticServer = await startStaticServer(bundlePath);
  const sfu = startSfu();
  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 60000,
    args: ['--no-sandbox'],
  });
  const page = await browser.newPage();
  const roomId = 'downlink_v2_' + Date.now();

  try {
    await waitForPort(signalingPort);
    await page.goto(getServerUrl(staticServer), { waitUntil: 'load' });

    const initResult = await page.evaluate(
      (port, room) => window.__downlinkV2Harness.init(`ws://127.0.0.1:${port}/ws`, room),
      signalingPort, roomId,
    );
    console.log(`[init] producerId=${initResult.producerId} consumerId=${initResult.consumerId} trackId=${initResult.trackId}`);

    await page.evaluate(() => window.__downlinkV2Harness.publishPublisherStats(1));
    await sleep(200);
    await page.evaluate(() => window.__downlinkV2Harness.drainPublisherNotifications());

    // Case 1: low demand should clamp publisher supply.
    await page.evaluate(() => window.__downlinkV2Harness.sendDownlinkStats({
      seq: 1,
      visible: true,
      pinned: false,
      availableIncomingBitrate: 50_000,
      targetWidth: 160,
      targetHeight: 90,
    }));
    const clamp = await page.evaluate(() => window.__downlinkV2Harness.waitForTrackOverride(3000));
    if (!clamp) throw new Error('did not receive downlink_v2 room-demand clamp');
    if (clamp.scope !== 'track' || clamp.trackId !== 'camera-main' || clamp.maxLevelClamp !== 0) {
      throw new Error(`unexpected clamp payload: ${JSON.stringify(clamp)}`);
    }
    console.log('[PASS] case 1: low demand -> track clamp maxLevelClamp=0');

    // Case 2: high demand should clear clamp.
    await page.evaluate(() => window.__downlinkV2Harness.sendDownlinkStats({
      seq: 2,
      visible: true,
      pinned: true,
      availableIncomingBitrate: 5_000_000,
      targetWidth: 1280,
      targetHeight: 720,
    }));
    const clear = await page.evaluate(() => window.__downlinkV2Harness.waitForTrackOverride(3000));
    if (!clear) throw new Error('did not receive downlink_v2 demand-restored clear');
    if (clear.reason !== 'downlink_v2_demand_restored' || clear.trackId !== 'camera-main' || clear.maxLevelClamp !== null) {
      throw new Error(`unexpected clear payload: ${JSON.stringify(clear)}`);
    }
    console.log('[PASS] case 2: high demand -> track clear');

    // Case 3: zero-demand hidden should enter hold clamp.
    await page.evaluate(() => window.__downlinkV2Harness.sendDownlinkStats({
      seq: 3,
      visible: false,
      pinned: false,
      availableIncomingBitrate: 5_000_000,
      targetWidth: 0,
      targetHeight: 0,
    }));
    const hold = await page.evaluate(() => window.__downlinkV2Harness.waitForTrackOverride(3000));
    if (!hold) throw new Error('did not receive downlink_v2 zero-demand hold clamp');
    if (hold.reason !== 'downlink_v2_zero_demand_hold' || hold.trackId !== 'camera-main' || hold.maxLevelClamp !== 0) {
      throw new Error(`unexpected zero-demand hold payload: ${JSON.stringify(hold)}`);
    }
    console.log('[PASS] case 3: hidden demand -> zero-demand hold clamp');

    console.log('\nbrowser_downlink_v2: all cases passed');
  } finally {
    await browser.close();
    await new Promise(resolve => staticServer.close(resolve));
    await stopSfu(sfu);
  }
}

runScenario().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
