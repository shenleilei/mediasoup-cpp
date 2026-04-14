/**
 * Node runner for browser-side downlink QoS v3 pause/resume verification.
 *
 * Stage 4 of v3 implementation plan:
 *   - sustained zero-demand -> pauseUpstream override
 *   - demand restored -> resumeUpstream override
 *   - rapid hide/show flicker does NOT cause pause
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
const signalingPort = 14017;

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'downlink-v3-entry.js')],
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
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-downlink-v3-'));
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
  const roomId = 'downlink_v3_' + Date.now();

  try {
    await waitForPort(signalingPort);
    await page.goto(getServerUrl(staticServer), { waitUntil: 'load' });

    const initResult = await page.evaluate(
      (port, room) => window.__downlinkV3Harness.init(`ws://127.0.0.1:${port}/ws`, room),
      signalingPort, roomId,
    );
    console.log(`[init] producerId=${initResult.producerId} consumerId=${initResult.consumerId}`);

    // Publish initial stats so server knows the publisher track
    await page.evaluate(() => window.__downlinkV3Harness.publishPublisherStats());
    await sleep(200);
    await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());

    // ── Case 1: sustained zero-demand → pauseUpstream ──
    // Send hidden stats repeatedly to sustain zero-demand past kPauseConfirmMs (4s)
    console.log('[case 1] sending sustained hidden demand...');
    const pauseStartMs = Date.now();
    for (let i = 0; i < 12; i++) {
      await page.evaluate(() => window.__downlinkV3Harness.sendDownlinkStats({ visible: false }));
      await sleep(500);
    }
    // Should have received pauseUpstream by now (4s zero-demand + margin)
    const pause = await page.evaluate((afterTsMs) =>
      window.__downlinkV3Harness.waitForOverride(
        { reasonPrefix: 'downlink_v3_zero_demand_pause', pauseUpstream: true },
        3000,
        afterTsMs,
      ), pauseStartMs);
    if (!pause) {
      throw new Error('case 1 failed: no pauseUpstream received after sustained hidden');
    }
    if (!pause.pauseUpstream || pause.trackId !== 'camera-main') {
      throw new Error(`unexpected pause payload: ${JSON.stringify(pause)}`);
    }
    console.log('[PASS] case 1: sustained hidden -> pauseUpstream');

    // ── Case 2: demand restored → resumeUpstream ──
    console.log('[case 2] restoring visible demand...');
    await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());
    const resumeStartMs = Date.now();
    for (let i = 0; i < 4; i++) {
      await page.evaluate(() => window.__downlinkV3Harness.sendDownlinkStats({
        visible: true, pinned: true,
        availableIncomingBitrate: 5_000_000,
        targetWidth: 1280, targetHeight: 720,
      }));
      await sleep(500);
    }
    const resume = await page.evaluate((afterTsMs) =>
      window.__downlinkV3Harness.waitForOverride(
        { reasonPrefix: 'downlink_v3_demand_resumed', resumeUpstream: true },
        3000,
        afterTsMs,
      ), resumeStartMs);
    if (!resume) {
      throw new Error('case 2 failed: no resumeUpstream received after demand recovery');
    }
    if (!resume.resumeUpstream || resume.trackId !== 'camera-main') {
      throw new Error(`unexpected resume payload: ${JSON.stringify(resume)}`);
    }
    console.log('[PASS] case 2: demand restored -> resumeUpstream');

    // ── Case 3: rapid hide/show flicker does NOT cause pause ──
    console.log('[case 3] rapid hide/show flicker...');
    await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());
    const flickerStartMs = Date.now();
    for (let i = 0; i < 10; i++) {
      await page.evaluate((vis) => window.__downlinkV3Harness.sendDownlinkStats({
        visible: vis, pinned: vis,
        availableIncomingBitrate: 5_000_000,
        targetWidth: vis ? 1280 : 0, targetHeight: vis ? 720 : 0,
      }), i % 2 === 0);
      await sleep(200);
    }
    // End with visible
    await page.evaluate(() => window.__downlinkV3Harness.sendDownlinkStats({
      visible: true, pinned: true,
      availableIncomingBitrate: 5_000_000,
      targetWidth: 1280, targetHeight: 720,
    }));
    await sleep(500);
    const spuriousPause = await page.evaluate((afterTsMs) =>
      window.__downlinkV3Harness.waitForOverride(
        { reasonPrefix: 'downlink_v3_zero_demand_pause', pauseUpstream: true },
        1000,
        afterTsMs,
      ), flickerStartMs);
    if (spuriousPause) {
      throw new Error('case 3 failed: rapid flicker caused spurious pauseUpstream');
    }
    console.log('[PASS] case 3: rapid flicker did not cause pauseUpstream');

    console.log('\nbrowser_downlink_v3: all cases passed');
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
