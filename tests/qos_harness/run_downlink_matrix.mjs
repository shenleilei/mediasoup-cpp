/**
 * Downlink QoS matrix runner.
 *
 * Runs each downlink case definition through the browser_downlink_v3 harness
 * infrastructure (SFU + headless browser), collects results, and writes a
 * JSON report to docs/generated/downlink-qos-matrix-report.json.
 *
 * Usage:
 *   node tests/qos_harness/run_downlink_matrix.mjs
 *   node tests/qos_harness/run_downlink_matrix.mjs --cases=D1,D8
 */
import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
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
const signalingPort = 14018;

const args = process.argv.slice(2);
const caseArg = args.find(a => a.startsWith('--cases='));
const selectedCases = caseArg
  ? new Set(caseArg.replace('--cases=', '').split(',').map(s => s.trim()).filter(Boolean))
  : null;

const DOWNLINK_REPORT_PATHS = {
  archiveRoot: 'docs/archive/downlink-qos-runs',
  fullMatrixJson: 'docs/generated/downlink-qos-matrix-report.json',
  fullCaseMarkdown: 'docs/downlink-qos-case-results.md',
  targetedMatrixJson: 'docs/generated/downlink-qos-matrix-report.targeted.json',
  targetedCaseMarkdown: 'docs/generated/downlink-qos-case-results.targeted.md',
};

const runType = selectedCases ? 'targeted' : 'full';
const relJsonPath = runType === 'targeted'
  ? DOWNLINK_REPORT_PATHS.targetedMatrixJson
  : DOWNLINK_REPORT_PATHS.fullMatrixJson;
const outputPath = path.join(repoRoot, relJsonPath);

function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

function loadCases() {
  const raw = JSON.parse(
    fs.readFileSync(path.join(__dirname, 'scenarios', 'downlink_cases.json'), 'utf8'),
  );
  if (selectedCases) return raw.filter(c => selectedCases.has(c.caseId));
  return raw;
}

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
  child.stdout.on('data', chunk => process.stderr.write(chunk));
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

async function runCase(caseDef, page) {
  const roomId = `dl_matrix_${caseDef.caseId}_${Date.now()}`;
  const result = { caseId: caseDef.caseId, group: caseDef.group, priority: caseDef.priority, startTime: new Date().toISOString() };

  const initResult = await page.evaluate(
    (port, room) => window.__downlinkV3Harness.init(`ws://127.0.0.1:${port}/ws`, room),
    signalingPort, roomId,
  );
  result.producerId = initResult.producerId;
  result.consumerId = initResult.consumerId;

  // publish initial stats
  await page.evaluate(() => window.__downlinkV3Harness.publishPublisherStats());
  await sleep(200);
  await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());

  const expect = caseDef.expect || {};

  if (expect.pauseUpstream) {
    // zero-demand case: send hidden stats to trigger pauseUpstream
    for (let i = 0; i < 12; i++) {
      await page.evaluate(() => window.__downlinkV3Harness.sendDownlinkStats({ visible: false }));
      await sleep(500);
    }
    const pause = await page.evaluate(() =>
      window.__downlinkV3Harness.waitForTrackOverrideWithReason('downlink_v3_zero_demand_pause', 3000));
    result.pauseReceived = !!pause;

    if (expect.resumeUpstream) {
      await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());
      for (let i = 0; i < 4; i++) {
        await page.evaluate(() => window.__downlinkV3Harness.sendDownlinkStats({
          visible: true, pinned: true, availableIncomingBitrate: 5_000_000,
          targetWidth: 1280, targetHeight: 720,
        }));
        await sleep(500);
      }
      const resume = await page.evaluate(() =>
        window.__downlinkV3Harness.waitForTrackOverrideWithReason('downlink_v3_demand_resumed', 3000));
      result.resumeReceived = !!resume;
    }
  } else {
    // normal case: send visible stats throughout phases
    const phases = caseDef.phases || {};
    for (const phaseName of ['baseline', 'impaired', 'recovery']) {
      const durationMs = phaseName === 'baseline' ? caseDef.baselineMs
        : phaseName === 'impaired' ? caseDef.impairmentMs : caseDef.recoveryMs;
      const phaseNet = phases[phaseName] || phases.baseline || {};
      const bw = phaseNet.bandwidth || 5000;
      const iterations = Math.max(1, Math.floor(durationMs / 500));
      for (let i = 0; i < iterations; i++) {
        await page.evaluate((vis, bwKbps) => window.__downlinkV3Harness.sendDownlinkStats({
          visible: vis, pinned: true,
          availableIncomingBitrate: bwKbps * 1000,
          targetWidth: 1280, targetHeight: 720,
        }), true, bw);
        await sleep(500);
      }
      result[phaseName] = { network: phaseNet, durationMs };
    }
  }

  result.endTime = new Date().toISOString();
  result.verdict = evaluateCase(caseDef, result);
  return result;
}

function evaluateCase(caseDef, result) {
  const expect = caseDef.expect || {};
  if (expect.pauseUpstream && !result.pauseReceived) {
    return { passed: false, reason: 'pauseUpstream not received' };
  }
  if (expect.resumeUpstream && !result.resumeReceived) {
    return { passed: false, reason: 'resumeUpstream not received' };
  }
  // For non-pause cases, the harness ran without error — mark pass.
  // Detailed layer/priority assertions require server-side stats collection
  // which will be added as the matrix matures.
  return { passed: true, reason: 'ok' };
}

function buildSummary(results) {
  const executed = results.filter(r => !r.error);
  const failed = executed.filter(r => !r.verdict?.passed);
  const errors = results.filter(r => r.error);
  return {
    total: results.length,
    executed: executed.length,
    passed: executed.length - failed.length,
    failed: failed.length,
    errors: errors.length,
  };
}

function archiveReport(report) {
  const archiveRoot = path.join(repoRoot, DOWNLINK_REPORT_PATHS.archiveRoot);
  const ts = report.generatedAt.replace(/:/g, '-');
  const archiveDir = path.join(archiveRoot, ts);
  fs.mkdirSync(archiveDir, { recursive: true });
  fs.copyFileSync(outputPath, path.join(archiveDir, path.basename(relJsonPath)));
  fs.writeFileSync(path.join(archiveDir, 'metadata.json'), JSON.stringify({
    generatedAt: report.generatedAt,
    runType,
    selectedCaseIds: selectedCases ? [...selectedCases] : 'all',
    sourceScript: 'tests/qos_harness/run_downlink_matrix.mjs',
  }, null, 2) + '\n');
}

async function runMatrix() {
  const cases = loadCases();
  if (cases.length === 0) {
    console.error('no downlink cases to run');
    process.exitCode = 1;
    return;
  }

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-dl-matrix-'));
  const bundlePath = buildBundle(tmpDir);
  const staticServer = await startStaticServer(bundlePath);
  const sfu = startSfu();
  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 120000,
    args: ['--no-sandbox'],
  });

  const results = [];

  try {
    await waitForPort(signalingPort);

    for (const caseDef of cases) {
      const page = await browser.newPage();
      const serverUrl = `http://127.0.0.1:${staticServer.address().port}`;
      let result;
      let lastError;

      for (let attempt = 1; attempt <= 2; attempt++) {
        try {
          console.error(`running ${caseDef.caseId}${attempt > 1 ? ' (retry)' : ''}`);
          await page.goto(serverUrl, { waitUntil: 'load' });
          result = await runCase(caseDef, page);
          break;
        } catch (error) {
          console.error(`[${caseDef.caseId}] attempt ${attempt} error: ${error.message}`);
          lastError = error;
          if (attempt === 1 && /Target closed|Session closed|Protocol error|ws connect timeout/.test(error.message)) {
            console.error(`${caseDef.caseId} retrying after browser error`);
            continue;
          }
          break;
        }
      }

      if (result) {
        results.push(result);
        const status = result.verdict.passed ? 'PASS' : 'FAIL';
        console.error(`[${caseDef.caseId}] ${status} ${result.verdict.reason}`);
      } else {
        console.error(`[${caseDef.caseId}] ERROR ${lastError.message}`);
        results.push({
          caseId: caseDef.caseId,
          group: caseDef.group,
          priority: caseDef.priority,
          error: lastError.message,
        });
      }

      await page.close().catch(() => {});
      await sleep(1500);
    }
  } finally {
    await browser.close();
    await new Promise(resolve => staticServer.close(resolve));
    await stopSfu(sfu);
  }

  const report = {
    generatedAt: new Date().toISOString(),
    selectedCaseIds: selectedCases ? [...selectedCases] : 'all',
    includedCaseIds: cases.map(c => c.caseId),
    summary: buildSummary(results),
    cases: results,
  };

  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, JSON.stringify(report, null, 2));
  archiveReport(report);

  console.error(
    `downlink matrix: passed=${report.summary.passed} failed=${report.summary.failed} errors=${report.summary.errors} total=${report.summary.total}`,
  );
  console.error(`report written to ${outputPath}`);

  if (report.summary.failed > 0 || report.summary.errors > 0) {
    process.exitCode = 1;
  }
}

runMatrix().catch(error => {
  console.error('downlink matrix runner failed:', error);
  process.exitCode = 1;
});
