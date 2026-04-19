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
let signalingPort = 0;

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

function buildPhaseStats(phaseNet, overrides = {}) {
  const visible = overrides.visible ?? true;
  const targetWidth = visible ? (overrides.targetWidth ?? 1280) : 0;
  const targetHeight = visible ? (overrides.targetHeight ?? 720) : 0;
  return {
    visible,
    pinned: overrides.pinned ?? true,
    targetWidth,
    targetHeight,
    availableIncomingBitrate: (phaseNet.bandwidth ?? 5000) * 1000,
    currentRoundTripTime: (phaseNet.rtt ?? 30) / 1000,
    packetsLost: phaseNet.loss ?? 0,
    jitter: (phaseNet.jitter ?? 2) / 1000,
  };
}

/** Run a single phase, collecting trace entries with server-side consumer state. */
async function runPhase(page, phaseName, caseDef, durationMs, statsOverrides = {}) {
  const phaseNet = (caseDef.phases || {})[phaseName] || (caseDef.phases || {}).baseline || {};
  const stats = buildPhaseStats(phaseNet, statsOverrides);
  const iterations = Math.max(1, Math.floor(durationMs / 500));
  const startMs = Date.now();
  for (let i = 0; i < iterations; i++) {
    await page.evaluate((opts) => window.__downlinkV3Harness.sendDownlinkStats(opts), stats);
    await sleep(200);
    // collect consumer state + overrides as trace
    await page.evaluate((ph) => {
      const overrides = window.__downlinkV3Harness.drainAndLogOverrides();
      return window.__downlinkV3Harness.queryConsumerState().then(cs => {
        window.__downlinkV3Harness.recordTraceEntry(ph, cs, overrides);
      });
    }, phaseName);
    await sleep(300);
  }
  const endMs = Date.now();
  // snapshot consumer state at phase end
  const endState = await page.evaluate(() => window.__downlinkV3Harness.queryConsumerState());
  return { network: phaseNet, durationMs, startMs, endMs, endState };
}

async function runCompetitionPhase(page, phaseName, caseDef, durationMs) {
  const phaseNet = (caseDef.phases || {})[phaseName] || (caseDef.phases || {}).baseline || {};
  const lowStats = buildPhaseStats(phaseNet, {
    visible: true,
    pinned: false,
    targetWidth: 320,
    targetHeight: 180,
  });
  const highStats = buildPhaseStats(phaseNet, {
    visible: true,
    pinned: true,
    targetWidth: 1280,
    targetHeight: 720,
  });
  const iterations = Math.max(1, Math.floor(durationMs / 500));
  const startMs = Date.now();
  for (let i = 0; i < iterations; i++) {
    await page.evaluate((low, high) => {
      const h = window.__downlinkV3Harness;
      return h.sendDownlinkStatsBatch({
        availableIncomingBitrate: high.availableIncomingBitrate,
        currentRoundTripTime: high.currentRoundTripTime,
        subscriptions: [
          {
            consumerId: h.consumerId,
            producerId: h.producerId,
            visible: low.visible,
            pinned: low.pinned,
            activeSpeaker: false,
            isScreenShare: false,
            targetWidth: low.targetWidth,
            targetHeight: low.targetHeight,
            packetsLost: low.packetsLost,
            jitter: low.jitter,
            framesPerSecond: low.visible ? 30 : 0,
            frameWidth: low.visible ? low.targetWidth : 0,
            frameHeight: low.visible ? low.targetHeight : 0,
            freezeRate: 0,
          },
          {
            consumerId: h.consumerId2,
            producerId: h.producerId2,
            visible: high.visible,
            pinned: high.pinned,
            activeSpeaker: false,
            isScreenShare: false,
            targetWidth: high.targetWidth,
            targetHeight: high.targetHeight,
            packetsLost: high.packetsLost,
            jitter: high.jitter,
            framesPerSecond: high.visible ? 30 : 0,
            frameWidth: high.visible ? high.targetWidth : 0,
            frameHeight: high.visible ? high.targetHeight : 0,
            freezeRate: 0,
          },
        ],
      }, h.sub);
    }, lowStats, highStats);
    await sleep(200);
    await page.evaluate((ph) => {
      const h = window.__downlinkV3Harness;
      const overrides = h.drainAndLogOverrides();
      return Promise.all([
        h.queryConsumerState(h.sub, h.consumerId),
        h.queryConsumerState(h.sub, h.consumerId2),
      ]).then(([sub1State, sub2State]) => {
        h.recordTraceEntry(ph, { sub1State, sub2State }, overrides);
      });
    }, phaseName);
    await sleep(300);
  }

  const endMs = Date.now();
  const [state1, state2] = await page.evaluate(() => {
    const h = window.__downlinkV3Harness;
    return Promise.all([
      h.queryConsumerState(h.sub, h.consumerId),
      h.queryConsumerState(h.sub, h.consumerId2),
    ]);
  });
  return {
    network: phaseNet,
    durationMs,
    startMs,
    endMs,
    endState: { sub1State: state1, sub2State: state2 },
  };
}

/** Run competition case for D7: one subscriber, two competing consumers. */
async function runCompetitionCase(page, caseDef) {
  const result = { caseId: caseDef.caseId, group: caseDef.group, priority: caseDef.priority, startTime: new Date().toISOString() };

  const initResult = await page.evaluate(
    (port, room) => window.__downlinkV3Harness.init(`ws://127.0.0.1:${port}/ws`, room),
    signalingPort, `dl_matrix_${caseDef.caseId}_${Date.now()}`,
  );
  result.producerId = initResult.producerId;
  result.consumerId = initResult.consumerId;

  const secondResult = await page.evaluate(
    (port) => window.__downlinkV3Harness.initSecondPublisher(`ws://127.0.0.1:${port}/ws`),
    signalingPort,
  );
  result.producerId2 = secondResult.producerId2;
  result.consumerId2 = secondResult.consumerId2;

  await page.evaluate(() => {
    const h = window.__downlinkV3Harness;
    return Promise.all([
      h.publishPublisherStats(),
      h.publishPublisherStats(h.pub2, h.producerId2, h.trackId2),
    ]);
  });
  await sleep(200);
  await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());

  result.baseline = await runCompetitionPhase(page, 'baseline', caseDef, caseDef.baselineMs);
  result.impairment = await runCompetitionPhase(page, 'impaired', caseDef, caseDef.impairmentMs);
  result.recovery = await runCompetitionPhase(page, 'recovery', caseDef, caseDef.recoveryMs);
  result.sub1ConsumerState = result.impairment.endState?.sub1State ?? null;
  result.sub2ConsumerState = result.impairment.endState?.sub2State ?? null;
  result.trace = await page.evaluate(() => window.__downlinkV3Harness.getTrace());
  result.overrideLog = await page.evaluate(() => window.__downlinkV3Harness.getOverrideLog());
  result.endTime = new Date().toISOString();
  result.verdict = evaluateCompetitionCase(caseDef, result);
  return result;
}

async function runCase(caseDef, page) {
  const expect = caseDef.expect || {};

  // D7 competition: separate path
  if (expect.highPriorityBetterLayer) {
    return runCompetitionCase(page, caseDef);
  }

  const roomId = `dl_matrix_${caseDef.caseId}_${Date.now()}`;
  const result = { caseId: caseDef.caseId, group: caseDef.group, priority: caseDef.priority, startTime: new Date().toISOString() };

  const initResult = await page.evaluate(
    (port, room) => window.__downlinkV3Harness.init(`ws://127.0.0.1:${port}/ws`, room),
    signalingPort, roomId,
  );
  result.producerId = initResult.producerId;
  result.consumerId = initResult.consumerId;

  await page.evaluate(() => window.__downlinkV3Harness.publishPublisherStats());
  await sleep(200);
  await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());

  if (expect.pauseUpstream) {
    result.baseline = await runPhase(page, 'baseline', caseDef, caseDef.baselineMs);

    const pauseStartMs = Date.now();
    result.impairment = await runPhase(page, 'impaired', caseDef, caseDef.impairmentMs, {
      visible: false,
      pinned: false,
      targetWidth: 0,
      targetHeight: 0,
    });
    const pause = await page.evaluate((afterTsMs) =>
      window.__downlinkV3Harness.waitForOverride(
        { reasonPrefix: 'downlink_v3_zero_demand_pause', pauseUpstream: true },
        1000,
        afterTsMs,
      ), pauseStartMs);
    result.pauseReceived = !!pause;
    result.pauseReceivedAtMs = pause?.tsMs ?? null;
    result.pauseLatencyMs = pause?.tsMs ? pause.tsMs - pauseStartMs : null;

    if (expect.resumeUpstream) {
      await page.evaluate(() => window.__downlinkV3Harness.drainPublisherNotifications());
      const resumeStartMs = Date.now();
      result.recovery = await runPhase(page, 'recovery', caseDef, caseDef.recoveryMs);
      const resume = await page.evaluate((afterTsMs) =>
        window.__downlinkV3Harness.waitForOverride(
          { reasonPrefix: 'downlink_v3_demand_resumed', resumeUpstream: true },
          1000,
          afterTsMs,
        ), resumeStartMs);
      result.resumeReceived = !!resume;
      result.resumeLatencyMs = resume?.tsMs ? resume.tsMs - resumeStartMs : null;

      // Keep a short quiet window after recovery to catch stray re-emits.
      await sleep(5000);
      await page.evaluate(() => window.__downlinkV3Harness.drainAndLogOverrides());
      result.settleEndMs = Date.now();
    }
  } else {
    // normal phase-based case with server-side state assertions
    result.baseline = await runPhase(page, 'baseline', caseDef, caseDef.baselineMs);
    result.impairment = await runPhase(page, 'impaired', caseDef, caseDef.impairmentMs);
    result.recovery = await runPhase(page, 'recovery', caseDef, caseDef.recoveryMs);
  }

  result.trace = await page.evaluate(() => window.__downlinkV3Harness.getTrace());
  result.overrideLog = await page.evaluate(() => window.__downlinkV3Harness.getOverrideLog());
  result.endTime = new Date().toISOString();
  result.endTimeMs = Date.now();
  result.timing = extractDownlinkTiming(result);
  result.transitionAudit = extractPauseResumeAudit(result);
  if (result.transitionAudit?.overall) {
    const seq = result.transitionAudit.overall.sequence || [];
    result.oscillation = {
      ...result.transitionAudit.overall,
      oscillation:
        seq.length > 2 ||
        (seq.length === 2 && !(seq[0] === 'pause' && seq[1] === 'resume')) ||
        (seq.length === 1 && seq[0] === 'resume'),
    };
  }
  result.verdict = evaluateCase(caseDef, result);
  return result;
}

function extractDownlinkTiming(result) {
  const timing = {};
  const log = result.overrideLog || [];
  const trace = result.trace || [];
  const find = (arr, pred) => arr.find(pred) || null;

  timing.t_first_clamp = find(log, o => typeof o.maxLevelClamp === 'number')?.tsMs ?? null;
  timing.t_first_pause = find(log, o => o.pauseUpstream === true)?.tsMs ?? null;
  timing.t_first_resume = find(log, o => o.resumeUpstream === true)?.tsMs ?? null;

  // find first trace entry in recovery phase where consumer is not paused
  const recoveryEntries = trace.filter(e => e.phase === 'recovery');
  const firstUnpaused = find(recoveryEntries, e => e.consumerState && !e.consumerState.paused);
  timing.t_first_unpaused_consumer = firstUnpaused?.tsMs ?? null;

  // find first stable layer in recovery (preferredSpatialLayer matches baseline end)
  const baselineEntries = trace.filter(e => e.phase === 'baseline');
  const baselineLayer = baselineEntries.length > 0
    ? baselineEntries[baselineEntries.length - 1]?.consumerState?.preferredSpatialLayer
    : null;
  if (typeof baselineLayer === 'number') {
    const stableEntry = find(recoveryEntries,
      e => e.consumerState?.preferredSpatialLayer === baselineLayer);
    timing.t_layer_stable = stableEntry?.tsMs ?? null;
  }

  return timing;
}

function summarizeActiveOverrideCounts(log, startMs, endMs) {
  const summary = { pauseCount: 0, resumeCount: 0, sequence: [] };
  let lastType = null;
  for (const entry of log || []) {
    if (entry.tsMs < startMs || entry.tsMs > endMs) continue;
    let type = null;
    if (entry.pauseUpstream === true) {
      summary.pauseCount++;
      type = 'pause';
    } else if (entry.resumeUpstream === true) {
      summary.resumeCount++;
      type = 'resume';
    }
    if (type && type !== lastType) {
      summary.sequence.push(type);
      lastType = type;
    }
  }
  return summary;
}

function extractPauseResumeAudit(result) {
  if (!result.impairment?.startMs) return null;
  const impairmentEnd = result.recovery?.startMs ?? result.impairment.endMs ?? result.impairment.startMs;
  const recoveryEnd = result.settleEndMs ?? result.recovery?.endMs ?? result.endTimeMs ?? impairmentEnd;
  return {
    impairment: summarizeActiveOverrideCounts(result.overrideLog, result.impairment.startMs, impairmentEnd),
    recovery: result.recovery?.startMs
      ? summarizeActiveOverrideCounts(result.overrideLog, result.recovery.startMs, recoveryEnd)
      : null,
    overall: summarizeActiveOverrideCounts(result.overrideLog, result.impairment.startMs, recoveryEnd),
  };
}

function evaluateCompetitionCase(caseDef, result) {
  const failures = [];
  const impaired = result.impairment?.endState || {};
  const baseline = result.baseline?.endState || {};
  const recovery = result.recovery?.endState || {};
  const s1 = impaired.sub1State;
  const s2 = impaired.sub2State;
  if (!s1 || !s2) return { passed: false, reason: 'could not query competition consumer states' };
  if (s2.priority <= s1.priority) {
    failures.push(`high-priority consumer priority ${s2.priority} <= low-priority ${s1.priority}`);
  }
  if (s2.preferredSpatialLayer <= s1.preferredSpatialLayer) {
    failures.push(`high-priority layer ${s2.preferredSpatialLayer} not strictly better than low-priority ${s1.preferredSpatialLayer}`);
  }
  if (caseDef.expect?.recovers) {
    const rb1 = baseline.sub1State;
    const rb2 = baseline.sub2State;
    const rr1 = recovery.sub1State;
    const rr2 = recovery.sub2State;
    if (!rr1 || !rr2) {
      failures.push('recovers: no recovery phase result');
    } else {
      if (rr1.paused || rr2.paused) failures.push('recovery left one or more competition consumers paused');
      if (rb1 && rr1.preferredSpatialLayer < rb1.preferredSpatialLayer) {
        failures.push(`low-priority recovery layer ${rr1.preferredSpatialLayer} < baseline ${rb1.preferredSpatialLayer}`);
      }
      if (rb2 && rr2.preferredSpatialLayer < rb2.preferredSpatialLayer) {
        failures.push(`high-priority recovery layer ${rr2.preferredSpatialLayer} < baseline ${rb2.preferredSpatialLayer}`);
      }
    }
  }
  if (failures.length > 0) return { passed: false, reason: failures.join('; ') };
  return { passed: true, reason: `high=${s2.preferredSpatialLayer} > low=${s1.preferredSpatialLayer}; priority=${s2.priority} > ${s1.priority}` };
}

function evaluateCase(caseDef, result) {
  const expect = caseDef.expect || {};
  const failures = [];

  // pause/resume assertions
  if (expect.pauseUpstream && !result.pauseReceived) {
    failures.push('pauseUpstream not received');
  }
  if (expect.resumeUpstream && !result.resumeReceived) {
    failures.push('resumeUpstream not received');
  }
  if (expect.pauseUpstream && result.transitionAudit?.impairment?.resumeCount > 0) {
    failures.push(`unexpected resumeUpstream during hidden phase: ${result.transitionAudit.impairment.resumeCount}`);
  }
  if (expect.pauseUpstream && result.transitionAudit?.impairment?.pauseCount < 1) {
    failures.push('pauseUpstream was not observed during hidden phase');
  }
  if (
    expect.pauseUpstream &&
    (result.transitionAudit?.impairment?.sequence || []).some(type => type !== 'pause')
  ) {
    failures.push(`hidden phase sequence is not monotonic pause: ${(result.transitionAudit?.impairment?.sequence || []).join('->')}`);
  }
  if (expect.resumeUpstream && (result.transitionAudit?.recovery?.resumeCount ?? 0) < 1) {
    failures.push('resumeUpstream was not observed during recovery phase');
  }
  if (expect.resumeUpstream && result.transitionAudit?.recovery?.pauseCount > 0) {
    failures.push(`unexpected pauseUpstream during recovery phase: ${result.transitionAudit.recovery.pauseCount}`);
  }
  if (
    expect.resumeUpstream &&
    (result.transitionAudit?.recovery?.sequence || []).some(type => type !== 'resume')
  ) {
    failures.push(`recovery phase sequence is not monotonic resume: ${(result.transitionAudit?.recovery?.sequence || []).join('->')}`);
  }
  if (expect.resumeUpstream && result.oscillation?.oscillation) {
    failures.push(`oscillation detected: ${(result.oscillation.sequence || []).join('->')}`);
  }

  // server-side consumer state assertions for phase-based cases
  if (!expect.pauseUpstream && result.impairment?.endState) {
    const cs = result.impairment.endState;
    if (typeof expect.consumerPaused === 'boolean' && cs.paused !== expect.consumerPaused) {
      failures.push(`consumerPaused: expected ${expect.consumerPaused}, got ${cs.paused}`);
    }
    if (typeof expect.preferredSpatialLayer === 'number' && cs.preferredSpatialLayer !== expect.preferredSpatialLayer) {
      failures.push(`preferredSpatialLayer: expected ${expect.preferredSpatialLayer}, got ${cs.preferredSpatialLayer}`);
    }
    if (typeof expect.maxSpatialLayer === 'number' && cs.preferredSpatialLayer > expect.maxSpatialLayer) {
      failures.push(`preferredSpatialLayer ${cs.preferredSpatialLayer} > maxSpatialLayer ${expect.maxSpatialLayer}`);
    }
  }

  // recovery assertion: consumer should not be paused and should return to baseline layer
  if (expect.recovers) {
    if (!result.recovery?.endState) {
      failures.push('recovers: no recovery phase result');
    } else {
      if (result.recovery.endState.paused) {
        failures.push('consumer still paused after recovery phase');
      }

      const baselineLayer = result.baseline?.endState?.preferredSpatialLayer;
      const recoveryLayer = result.recovery.endState.preferredSpatialLayer;
      const expectedRecoveryLayer =
        typeof expect.recoveryPreferredSpatialLayer === 'number'
          ? expect.recoveryPreferredSpatialLayer
          : baselineLayer;

      if (
        typeof expectedRecoveryLayer === 'number' &&
        typeof recoveryLayer === 'number' &&
        recoveryLayer < expectedRecoveryLayer
      ) {
        failures.push(`recovers: layer ${recoveryLayer} did not recover to expected ${expectedRecoveryLayer}`);
      }
    }
  }

  if (failures.length > 0) return { passed: false, reason: failures.join('; ') };
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
  signalingPort = await allocatePort();
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
    fs.rmSync(tmpDir, { recursive: true, force: true });
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
