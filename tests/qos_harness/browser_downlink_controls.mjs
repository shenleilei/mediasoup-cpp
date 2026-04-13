/**
 * Node runner for downlink consumer control browser harness.
 * Starts SFU, launches headless browser, runs 3 verification cases:
 *   1. pauseConsumer stops subscriber
 *   2. resumeConsumer restores subscriber
 *   3. requestConsumerKeyFrame succeeds after resume
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
const signalingPort = 14013;

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'downlink-controls-entry.js')],
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
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-downlink-'));
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
  const roomId = 'downlink_ctrl_' + Date.now();

  try {
    await waitForPort(signalingPort);

    await page.goto(getServerUrl(staticServer), { waitUntil: 'load' });

    // Init harness: pub joins, sub joins, pub produces, sub gets consumer
    const initResult = await page.evaluate(
      (port, room) => window.__downlinkControlsHarness.init(`ws://127.0.0.1:${port}/ws`, room),
      signalingPort, roomId,
    );
    console.log(`[init] consumerId=${initResult.consumerId}`);

    // Case 1: pauseConsumer
    const pauseResult = await page.evaluate(() => window.__downlinkControlsHarness.pauseConsumer());
    if (!pauseResult.paused) throw new Error('pauseConsumer did not set paused=true');
    console.log('[PASS] case 1: pauseConsumer sets paused=true');

    // Case 2: resumeConsumer
    const resumeResult = await page.evaluate(() => window.__downlinkControlsHarness.resumeConsumer());
    if (resumeResult.paused) throw new Error('resumeConsumer did not set paused=false');
    console.log('[PASS] case 2: resumeConsumer sets paused=false');

    // Case 3: requestConsumerKeyFrame after resume
    const kfResult = await page.evaluate(() => window.__downlinkControlsHarness.requestConsumerKeyFrame());
    // requestKeyFrame returns consumer json — just verify it succeeded
    if (!kfResult.id) throw new Error('requestConsumerKeyFrame did not return consumer data');
    console.log('[PASS] case 3: requestConsumerKeyFrame succeeds after resume');

    console.log('\nbrowser_downlink_controls: all 3 cases passed');
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
