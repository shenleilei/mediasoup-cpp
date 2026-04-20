/**
 * Node runner for end-to-end downlink QoS browser harness.
 * Verifies the full loop:
 *   1. client sends downlinkClientStats(visible=false) → server pauses consumer
 *   2. client sends downlinkClientStats(visible=true)  → server resumes consumer
 *   3. priority field is set correctly for hidden vs visible
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
let signalingPort = 0;

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

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
    entryPoints: [path.join(__dirname, 'browser', 'downlink-e2e-entry.js')],
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
      '--redisHost=0.0.0.0', '--redisPort=1', '--noRedisRequired',
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
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-downlink-e2e-'));
  const bundlePath = buildBundle(tmpDir);
  signalingPort = await allocatePort();
  const staticServer = await startStaticServer(bundlePath);
  const sfu = startSfu();
  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 60000,
    args: ['--no-sandbox'],
  });
  const page = await browser.newPage();
  const roomId = 'downlink_e2e_' + Date.now();

  try {
    await waitForPort(signalingPort);
    await page.goto(getServerUrl(staticServer), { waitUntil: 'load' });

    // Init: pub joins, sub joins, pub produces, sub gets consumer
    const initResult = await page.evaluate(
      (port, room) => window.__downlinkE2eHarness.init(`ws://127.0.0.1:${port}/ws`, room),
      signalingPort, roomId,
    );
    console.log(`[init] consumerId=${initResult.consumerId} producerId=${initResult.producerId}`);

    // Baseline: consumer should not be paused
    const baseline = await page.evaluate(() => window.__downlinkE2eHarness.getConsumerState());
    if (baseline.paused) throw new Error('baseline: consumer should not be paused');
    console.log('[OK] baseline: consumer not paused');

    // Case 1: send downlinkClientStats with visible=false → server should pause
    await page.evaluate(() => window.__downlinkE2eHarness.sendDownlinkStats(false, 1));
    // downlinkClientStats is processed async on worker thread; give it a moment
    await new Promise(r => setTimeout(r, 200));
    const afterHidden = await page.evaluate(() => window.__downlinkE2eHarness.getConsumerState());
    if (!afterHidden.paused) throw new Error('case 1: consumer should be paused after visible=false');
    console.log('[PASS] case 1: visible=false → consumer paused');

    // Case 2: send downlinkClientStats with visible=true → server should resume
    await page.evaluate(() => window.__downlinkE2eHarness.sendDownlinkStats(true, 2));
    await new Promise(r => setTimeout(r, 200));
    const afterVisible = await page.evaluate(() => window.__downlinkE2eHarness.getConsumerState());
    if (afterVisible.paused) throw new Error('case 2: consumer should be resumed after visible=true');
    console.log('[PASS] case 2: visible=true → consumer resumed');

    // Case 3: verify priority is set correctly
    // Send hidden again and check priority=1
    await page.evaluate(() => window.__downlinkE2eHarness.sendDownlinkStats(false, 3));
    await new Promise(r => setTimeout(r, 200));
    const hiddenState = await page.evaluate(() => window.__downlinkE2eHarness.getConsumerState());
    if (hiddenState.priority !== 1) throw new Error(`case 3a: hidden priority should be 1, got ${hiddenState.priority}`);
    console.log('[PASS] case 3a: hidden → priority=1');

    // Send visible+pinned=false (grid) and check priority=120
    await page.evaluate(() => window.__downlinkE2eHarness.sendDownlinkStats(true, 4));
    await new Promise(r => setTimeout(r, 200));
    const gridState = await page.evaluate(() => window.__downlinkE2eHarness.getConsumerState());
    if (gridState.priority !== 120) throw new Error(`case 3b: visible grid priority should be 120, got ${gridState.priority}`);
    console.log('[PASS] case 3b: visible grid → priority=120');

    console.log('\nbrowser_downlink_e2e: all cases passed');
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
