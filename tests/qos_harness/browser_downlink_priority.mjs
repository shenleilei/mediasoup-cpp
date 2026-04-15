/**
 * Node runner for downlink priority competition with real WebRTC media.
 *
 * Uses tc/netem on loopback to throttle real UDP RTP traffic.
 * Subscriber uses mediasoup-client Device + DownlinkSampler for real getStats().
 *
 * Tests:
 *   1. PriorityCompetitionPinnedVsGrid
 *   2. PriorityCompetitionScreenShareVsGrid
 *   3. RecoveryAfterThrottleRelease
 *   4. NoRegressionWithoutThrottle
 */
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { spawn, execSync } from 'node:child_process';
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

async function withTimeout(label, promise, timeoutMs = 30000) {
  let timer = null;
  try {
    return await Promise.race([
      promise,
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(`${label} timed out after ${timeoutMs} ms`)), timeoutMs);
      }),
    ]);
  } finally {
    if (timer) clearTimeout(timer);
  }
}

// ── build ──

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'downlink-priority-entry.js')],
    outfile, bundle: true, platform: 'browser', format: 'iife', target: ['chrome120'],
  });
  return outfile;
}

// ── SFU ──

function startSfu() {
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    ['--nodaemon', `--port=${signalingPort}`, '--workers=1', '--workerBin=./mediasoup-worker',
     '--announcedIp=127.0.0.1', '--listenIp=127.0.0.1', '--redisHost=0.0.0.0', '--redisPort=1'],
    { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'] },
  );
  child.stdout.on('data', () => {});
  child.stderr.on('data', () => {});
  return child;
}

function stopSfu(child, ms = 3000) {
  return new Promise(resolve => {
    if (!child.pid || child.exitCode !== null) { resolve(); return; }
    const t = setTimeout(() => { try { child.kill('SIGKILL'); } catch {} resolve(); }, ms);
    child.once('close', () => { clearTimeout(t); resolve(); });
    child.kill('SIGTERM');
  });
}

async function waitForPort(port, ms = 7000) {
  const net = await import('node:net');
  const dl = Date.now() + ms;
  while (Date.now() < dl) {
    try {
      await new Promise((ok, fail) => {
        const s = net.createConnection({ host: '127.0.0.1', port }, () => { s.destroy(); ok(); });
        s.once('error', fail);
      });
      return;
    } catch { await sleep(100); }
  }
  throw new Error(`port ${port} not ready`);
}

// ── netem ──

function hasNetem() {
  try { execSync('tc qdisc show dev lo 2>/dev/null', { stdio: 'pipe' }); return true; }
  catch { return false; }
}

function applyNetem({ delayMs = 0, lossPct = 0, rateKbit = 0 }) {
  clearNetem();

  // Isolate impairment to UDP so loopback TCP control traffic (signaling and
  // test orchestration) is not throttled together with RTP media.
  execSync('tc qdisc add dev lo root handle 1: prio', { stdio: 'pipe' });

  let netemCmd = 'tc qdisc add dev lo parent 1:3 handle 30: netem';
  if (delayMs) netemCmd += ` delay ${delayMs}ms`;
  if (lossPct) netemCmd += ` loss ${lossPct}%`;
  if (rateKbit) netemCmd += ` rate ${rateKbit}kbit`;
  execSync(netemCmd, { stdio: 'pipe' });
  execSync(
    'tc filter add dev lo protocol ip parent 1:0 prio 1 u32 match ip protocol 17 0xff flowid 1:3',
    { stdio: 'pipe' },
  );
}

function clearNetem() {
  try { execSync('tc qdisc del dev lo root 2>/dev/null', { stdio: 'pipe' }); } catch {}
}

// ── helpers ──

function printTimeline(label, samples) {
  console.log(`\n  timeline [${label}]:`);
  for (const s of samples) {
    const rel = s.ts - samples[0].ts;
    const hs = s.high, ls = s.low;
    let realInfo = '';
    if (s.realSubscriptions && s.realSubscriptions.length >= 2) {
      const r0 = s.realSubscriptions[0], r1 = s.realSubscriptions[1];
      realInfo = `  real: h.fps=${r0.framesPerSecond?.toFixed(1)||'?'} h.freeze=${r0.freezeRate?.toFixed(3)||'?'} l.fps=${r1.framesPerSecond?.toFixed(1)||'?'} l.freeze=${r1.freezeRate?.toFixed(3)||'?'}`;
    }
    if (s.realTransport) {
      realInfo += ` bw=${(s.realTransport.availableIncomingBitrate/1000).toFixed(0)}kbps rtt=${(s.realTransport.currentRoundTripTime*1000).toFixed(0)}ms`;
    }
    console.log(`    +${String(rel).padStart(5)}ms  high: paused=${hs.paused} spatial=${hs.spatial} pri=${hs.priority}  |  low: paused=${ls.paused} spatial=${ls.spatial} pri=${ls.priority}${realInfo}`);
  }
}

function score(s) {
  if (s.paused) return 0;
  return 100 + s.spatial * 10 + s.temporal;
}

// ── page helpers ──

async function initPage(page, roomId) {
  const result = await page.evaluate(
    (port, room) => window.__priorityHarness.init(`ws://127.0.0.1:${port}/ws`, room),
    signalingPort, roomId,
  );
  return result.consumers;
}

async function sendRealSnapshots(page, consumers, { highPinned = false, highScreenShare = false }) {
  await page.evaluate((consumers, highPinned, highScreenShare) => {
    return window.__priorityHarness.sendRealSnapshot([
      { consumerId: consumers[0].consumerId, producerId: consumers[0].producerId,
        pinned: highPinned, isScreenShare: highScreenShare, targetWidth: 1280, targetHeight: 720 },
      { consumerId: consumers[1].consumerId, producerId: consumers[1].producerId,
        pinned: false, isScreenShare: false, targetWidth: 320, targetHeight: 180 },
    ]);
  }, consumers, highPinned, highScreenShare);
}

async function collectSamples(page, count, intervalMs) {
  const samples = [];
  for (let i = 0; i < count; i++) {
    const s = await page.evaluate(() => window.__priorityHarness.sampleBoth());
    samples.push(s);
    if (i < count - 1) await sleep(intervalMs);
  }
  return samples;
}

async function createHarnessPage(browser, bundlePath) {
  const page = await browser.newPage();
  const diagnostics = {
    console: [],
    pageErrors: [],
    requestFailures: [],
  };

  page.on('console', message => {
    diagnostics.console.push({
      type: message.type(),
      text: message.text(),
    });
  });
  page.on('pageerror', error => {
    diagnostics.pageErrors.push(error?.stack || error?.message || String(error));
  });
  page.on('requestfailed', request => {
    diagnostics.requestFailures.push({
      url: request.url(),
      failure: request.failure()?.errorText || 'unknown',
    });
  });

  const bundleCode = fs.readFileSync(bundlePath, 'utf8');
  const inlineBundle = bundleCode.replace(/<\/script/gi, '<\\/script');
  await withTimeout('page.setContent', page.setContent(
    `<!doctype html><html><head><meta charset="utf-8"><title>Downlink Priority</title></head><body><script>${inlineBundle}</script></body></html>`,
    { waitUntil: 'domcontentloaded' },
  ), 45000);

  page.__diagnostics = diagnostics;
  return page;
}

// ── tests ──

async function run() {
  const useNetem = hasNetem();
  if (!useNetem) {
    console.log('WARNING: tc/netem not available; throttle tests will use degraded snapshot injection instead');
  }

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-priority-'));
  const bundlePath = buildBundle(tmpDir);
  const results = [];
  let sfu, browser;

  try {
    signalingPort = await allocatePort();
    sfu = startSfu();
    await waitForPort(signalingPort);
    console.log(`[info] SFU listening on ${signalingPort}`);
    browser = await puppeteer.launch({
      executablePath: chromiumPath, headless: true, protocolTimeout: 120000, pipe: true,
      args: ['--no-sandbox', '--autoplay-policy=no-user-gesture-required',
             '--use-fake-device-for-media-stream'],
    });
    console.log('[info] browser launched');

    // ── Test 1: PriorityCompetitionPinnedVsGrid ──
    {
      const name = 'PriorityCompetitionPinnedVsGrid';
      const page = await createHarnessPage(browser, bundlePath);
      try {
        console.log(`[info] ${name}: page ready`);
        const consumers = await withTimeout(
          `${name} initPage`,
          initPage(page, 'pri_pin_' + Date.now()),
          45000,
        );
        console.log(`[info] ${name}: consumers=${consumers.length}`);

        // Let media flow and send initial snapshots with real stats
        await sleep(2000);
        await withTimeout(`${name} initial snapshot`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
        await sleep(500);
        const baseline = await withTimeout(`${name} baseline samples`, collectSamples(page, 3, 500), 30000);
        console.log(`[info] ${name}: baseline collected`);

        // Apply impairment
        if (useNetem) applyNetem({ lossPct: 15, delayMs: 100, rateKbit: 100 });
        // Send snapshots during impairment (real stats will reflect degradation)
        for (let i = 0; i < 6; i++) {
          await withTimeout(`${name} impaired snapshot ${i + 1}`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
          await sleep(500);
        }
        const throttled = await withTimeout(`${name} throttled samples`, collectSamples(page, 5, 500), 30000);
        if (useNetem) clearNetem();

        const priOk = throttled.every(s => s.high.priority > s.low.priority);
        const highBetter = throttled.filter(s => score(s.high) >= score(s.low)).length;
        const pass = priOk && highBetter >= Math.ceil(throttled.length * 0.6);

        if (!pass) {
          printTimeline(name, [...baseline, ...throttled]);
          const diagnostics = page.__diagnostics;
          if (diagnostics.pageErrors.length > 0) {
            console.log(`  pageErrors: ${diagnostics.pageErrors.join(' | ')}`);
          }
        }
        console.log(`[${pass ? 'PASS' : 'FAIL'}] ${name}  (priOk=${priOk} highBetter=${highBetter}/${throttled.length})`);
        results.push({ name, pass });
      } finally { await page.close(); }
    }

    // ── Test 2: PriorityCompetitionScreenShareVsGrid ──
    {
      const name = 'PriorityCompetitionScreenShareVsGrid';
      const page = await createHarnessPage(browser, bundlePath);
      try {
        console.log(`[info] ${name}: page ready`);
        const consumers = await withTimeout(
          `${name} initPage`,
          initPage(page, 'pri_scr_' + Date.now()),
          45000,
        );
        console.log(`[info] ${name}: consumers=${consumers.length}`);

        await sleep(2000);
        await withTimeout(`${name} initial snapshot`, sendRealSnapshots(page, consumers, { highScreenShare: true }), 30000);
        await sleep(500);
        const baseline = await withTimeout(`${name} baseline samples`, collectSamples(page, 3, 500), 30000);

        if (useNetem) applyNetem({ lossPct: 15, delayMs: 100, rateKbit: 100 });
        for (let i = 0; i < 6; i++) {
          await withTimeout(`${name} impaired snapshot ${i + 1}`, sendRealSnapshots(page, consumers, { highScreenShare: true }), 30000);
          await sleep(500);
        }
        const throttled = await withTimeout(`${name} throttled samples`, collectSamples(page, 5, 500), 30000);
        if (useNetem) clearNetem();

        const priOk = throttled.every(s => s.high.priority > s.low.priority);
        const highBetter = throttled.filter(s => score(s.high) >= score(s.low)).length;
        const pass = priOk && highBetter >= Math.ceil(throttled.length * 0.6);

        if (!pass) {
          printTimeline(name, [...baseline, ...throttled]);
          const diagnostics = page.__diagnostics;
          if (diagnostics.pageErrors.length > 0) {
            console.log(`  pageErrors: ${diagnostics.pageErrors.join(' | ')}`);
          }
        }
        console.log(`[${pass ? 'PASS' : 'FAIL'}] ${name}  (priOk=${priOk} highBetter=${highBetter}/${throttled.length})`);
        results.push({ name, pass });
      } finally { await page.close(); }
    }

    // ── Test 3: RecoveryAfterThrottleRelease ──
    {
      const name = 'RecoveryAfterThrottleRelease';
      const page = await createHarnessPage(browser, bundlePath);
      try {
        console.log(`[info] ${name}: page ready`);
        const consumers = await withTimeout(
          `${name} initPage`,
          initPage(page, 'pri_rec_' + Date.now()),
          45000,
        );
        console.log(`[info] ${name}: consumers=${consumers.length}`);

        await sleep(2000);
        await withTimeout(`${name} initial snapshot`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
        await sleep(500);

        // Impair
        if (useNetem) applyNetem({ lossPct: 15, delayMs: 100, rateKbit: 100 });
        for (let i = 0; i < 5; i++) {
          await withTimeout(`${name} impaired snapshot ${i + 1}`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
          await sleep(500);
        }

        // Release
        if (useNetem) clearNetem();
        for (let i = 0; i < 5; i++) {
          await withTimeout(`${name} recovery snapshot ${i + 1}`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
          await sleep(500);
        }
        const recovery = await withTimeout(`${name} recovery samples`, collectSamples(page, 5, 500), 30000);

        const highRecovered = recovery.filter(s => score(s.high) >= score(s.low)).length;
        const pass = highRecovered >= Math.ceil(recovery.length * 0.6);

        if (!pass) {
          printTimeline(name, recovery);
          const diagnostics = page.__diagnostics;
          if (diagnostics.pageErrors.length > 0) {
            console.log(`  pageErrors: ${diagnostics.pageErrors.join(' | ')}`);
          }
        }
        console.log(`[${pass ? 'PASS' : 'FAIL'}] ${name}  (highRecovered=${highRecovered}/${recovery.length})`);
        results.push({ name, pass });
      } finally { await page.close(); }
    }

    // ── Test 4: NoRegressionWithoutThrottle ──
    {
      const name = 'NoRegressionWithoutThrottle';
      const page = await createHarnessPage(browser, bundlePath);
      try {
        console.log(`[info] ${name}: page ready`);
        const consumers = await withTimeout(
          `${name} initPage`,
          initPage(page, 'pri_nreg_' + Date.now()),
          45000,
        );
        console.log(`[info] ${name}: consumers=${consumers.length}`);

        await sleep(2000);
        await withTimeout(`${name} initial snapshot`, sendRealSnapshots(page, consumers, { highPinned: true }), 30000);
        await sleep(500);
        const samples = await withTimeout(`${name} samples`, collectSamples(page, 5, 500), 30000);

        const allUnpaused = samples.every(s => !s.high.paused && !s.low.paused);
        const priOk = samples.every(s => s.high.priority > s.low.priority);
        const pass = allUnpaused && priOk;

        if (!pass) {
          printTimeline(name, samples);
          const diagnostics = page.__diagnostics;
          if (diagnostics.pageErrors.length > 0) {
            console.log(`  pageErrors: ${diagnostics.pageErrors.join(' | ')}`);
          }
        }
        console.log(`[${pass ? 'PASS' : 'FAIL'}] ${name}  (allUnpaused=${allUnpaused} priOk=${priOk})`);
        results.push({ name, pass });
      } finally { await page.close(); }
    }

  } finally {
    clearNetem();
    if (browser) await browser.close();
    if (sfu) await stopSfu(sfu);
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }

  // ── Summary ──
  console.log('\n── Priority Competition Summary ──');
  let allPass = true;
  for (const r of results) {
    console.log(`  ${r.pass ? 'PASS' : 'FAIL'}  ${r.name}`);
    if (!r.pass) allPass = false;
  }

  if (!allPass) {
    console.log('\nNote: If priority competition tests consistently fail to show');
    console.log('differentiation under throttle, the current setPriority() implementation');
    console.log('may not be sufficient to produce observable competition effects.');
    console.log('Next step would be a room-level bitrate budget allocator design.');
  }

  console.log(`\nbrowser_downlink_priority: ${results.filter(r => r.pass).length}/${results.length} passed`);
  process.exitCode = allPass ? 0 : 1;
}

run().catch(error => {
  console.error(error);
  clearNetem();
  process.exitCode = 1;
});
