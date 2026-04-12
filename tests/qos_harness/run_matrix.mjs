import fs from 'node:fs';
import path from 'node:path';
import {
  createLoopbackHarness,
  applyNetemConfig,
  clearNetem,
  sleep,
} from './loopback_runner.mjs';
import {
  deriveCaseEvaluation,
  extractTiming,
  getPhaseNetwork,
  getImpairedStateForEvaluation,
  summarizePhaseState,
} from './synthetic_sweep_shared.mjs';
import {
  archiveCurrentReportSet,
  backupLatestReportSet,
  getReportSetPaths,
  getRunTypeForSelectedCases,
} from './report_artifacts.mjs';

const __dirname = path.dirname(new URL(import.meta.url).pathname);
const repoRoot = path.resolve(__dirname, '..', '..');
const scenariosPath = path.join(__dirname, 'scenarios', 'sweep_cases.json');

const args = process.argv.slice(2);
const caseArg = args.find(arg => arg.startsWith('--cases='));
const selectedCases = caseArg
  ? new Set(caseArg.replace('--cases=', '').split(',').map(id => id.trim()).filter(Boolean))
  : null;
const runType = getRunTypeForSelectedCases(selectedCases);
const reportPaths = getReportSetPaths(repoRoot, runType);
const outputPath = reportPaths.matrixJsonPath;

const durationScale = Number(process.env.QOS_MATRIX_SPEED) || 1;

function scaleDuration(ms, fallbackMs) {
  const raw = typeof ms === 'number' ? ms : fallbackMs;
  return Math.max(1000, Math.round(raw * durationScale));
}

function toNetemConfig(network = {}) {
  const config = {};
  if (typeof network.bandwidth === 'number') config.bandwidth = network.bandwidth;
  if (typeof network.rtt === 'number') config.rtt = network.rtt;
  if (typeof network.loss === 'number') config.loss = network.loss;
  if (typeof network.jitter === 'number') config.jitter = network.jitter;
  return config;
}

async function runPhase(harness, config, durationMs) {
  if (Object.keys(config).length > 0) {
    applyNetemConfig(config);
  } else {
    clearNetem();
  }

  const startMs = await harness.nowMs();
  await sleep(durationMs);
  const state = await harness.getState();
  return {
    startMs,
    durationMs,
    netem: config,
    state,
  };
}

async function runCase(caseDef) {
  let harness;
  const result = {
    caseId: caseDef.caseId,
    group: caseDef.group,
    priority: caseDef.priority,
    startTime: new Date().toISOString(),
  };

  try {
    harness = await createLoopbackHarness();
    const baselineNetwork = getPhaseNetwork(caseDef, 'baseline');
    const impairedNetwork = getPhaseNetwork(caseDef, 'impaired');
    const recoveryNetwork = getPhaseNetwork(caseDef, 'recovery');

    const baseline = await runPhase(
      harness,
      toNetemConfig(baselineNetwork),
      scaleDuration(caseDef.baselineMs, 15000)
    );
    const impairment = await runPhase(
      harness,
      toNetemConfig(impairedNetwork),
      scaleDuration(caseDef.impairmentMs, 20000)
    );

    let recovery;
    if (caseDef.expect?.recovery === false) {
      recovery = {
        startMs: await harness.nowMs(),
        durationMs: 0,
        netem: toNetemConfig(impairedNetwork),
        state: impairment.state,
      };
    } else {
      recovery = await runPhase(
        harness,
        toNetemConfig(recoveryNetwork),
        scaleDuration(caseDef.recoveryMs, 30000)
      );
    }

    clearNetem();

    const fullTrace =
      recovery.state?.trace ||
      impairment.state?.trace ||
      baseline.state?.trace ||
      [];
    const baselineSummary = summarizePhaseState(
      fullTrace,
      baseline.startMs,
      baseline.state,
      impairment.startMs
    );
    const impairmentSummary = summarizePhaseState(
      fullTrace,
      impairment.startMs,
      impairment.state,
      recovery.startMs
    );
    const recoverySummary = summarizePhaseState(
      fullTrace,
      recovery.startMs,
      recovery.state
    );
    const impairedStateForEvaluation = getImpairedStateForEvaluation(
      caseDef,
      impairmentSummary
    );
    const actionTypes = fullTrace
      .map(entry => entry?.plannedAction?.type)
      .filter(type => type && type !== 'noop');
    const evaluation = deriveCaseEvaluation(
      caseDef,
      baselineSummary.current,
      impairedStateForEvaluation,
      recoverySummary.best
    );

    result.baseline = baseline;
    result.impairment = impairment;
    result.recovery = recovery;
    result.phaseSummary = {
      baseline: baselineSummary,
      impairment: impairmentSummary,
      recovery: recoverySummary,
    };
    result.timing = {
      impairment: extractTiming(fullTrace, impairment.startMs, recovery.startMs),
      recovery:
        caseDef.expect?.recovery === false
          ? {}
          : extractTiming(fullTrace, recovery.startMs),
    };
    result.analysis = evaluation.analysis;
    result.actionCount = actionTypes.length;
    result.actionTypes = actionTypes;
    result.verdict = {
      passed: evaluation.passed,
      reason: evaluation.reason,
      expectation: caseDef.expect ?? {},
    };
    result.endTime = new Date().toISOString();
    return result;
  } catch (error) {
    if (harness?.getDiagnostics) {
      try {
        error.qosDiagnostics = await harness.getDiagnostics();
      } catch {}
    }
    throw error;
  } finally {
    clearNetem();
    await harness?.stop?.();
  }
}

function buildSummary(results) {
  const executed = results.filter(result => !result.error);
  const failed = executed.filter(result => result.verdict?.passed !== true);
  const errors = results.filter(result => result.error);
  const byGroup = {};

  for (const result of executed) {
    byGroup[result.group] ??= { total: 0, passed: 0, failed: 0 };
    byGroup[result.group].total += 1;
    if (result.verdict?.passed) {
      byGroup[result.group].passed += 1;
    } else {
      byGroup[result.group].failed += 1;
    }
  }

  return {
    total: results.length,
    executed: executed.length,
    passed: executed.length - failed.length,
    failed: failed.length,
    errors: errors.length,
    byGroup,
  };
}

async function runMatrix() {
  backupLatestReportSet(repoRoot, runType, {
    sourceScript: 'tests/qos_harness/run_matrix.mjs',
  });

  const rawCases = JSON.parse(fs.readFileSync(scenariosPath, 'utf8'));
  const cases = selectedCases
    ? rawCases.filter(caseDef => selectedCases.has(caseDef.caseId))
    : rawCases;
  const results = [];

  for (const caseDef of cases) {
    let result;
    let lastError;
    for (let attempt = 1; attempt <= 2; attempt += 1) {
      try {
        console.error(`running case ${caseDef.caseId}${attempt > 1 ? ` (retry ${attempt})` : ''}`);
        result = await runCase(caseDef);
        break;
      } catch (error) {
        lastError = error;
        const retryable = attempt === 1 &&
          /Target closed|Session closed|Protocol error \(Runtime\.callFunctionOn\)/.test(error.message);
        if (!retryable) {
          break;
        }
        console.error(`case ${caseDef.caseId} retrying after browser target closed`);
      }
    }
    if (result) {
      results.push(result);
      const status = result.verdict.passed ? 'PASS' : 'FAIL';
      console.error(
        `[${caseDef.caseId}] ${status} impaired=${result.impairment.state.state}/L${result.impairment.state.level} recovered=${result.recovery.state.state}/L${result.recovery.state.level}`
      );
    } else {
      console.error(`case ${caseDef.caseId} failed: ${lastError.message}`);
      results.push({
        caseId: caseDef.caseId,
        group: caseDef.group,
        priority: caseDef.priority,
        error: lastError.message,
        stack: lastError.stack,
        diagnostics: lastError.qosDiagnostics,
      });
    }
  }

  const report = {
    generatedAt: new Date().toISOString(),
    durationScale,
    selectedCaseIds: selectedCases ? [...selectedCases] : 'all',
    summary: buildSummary(results),
    cases: results,
  };

  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, JSON.stringify(report, null, 2));
  archiveCurrentReportSet(repoRoot, {
    generatedAt: report.generatedAt,
    runType,
    selectedCaseIds: report.selectedCaseIds,
    includeCaseMarkdown: false,
    sourceScript: 'tests/qos_harness/run_matrix.mjs',
  });
  console.error(
    `matrix summary: passed=${report.summary.passed} failed=${report.summary.failed} errors=${report.summary.errors} total=${report.summary.total}`
  );
  console.error(`matrix report written to ${outputPath}`);

  if (report.summary.failed > 0 || report.summary.errors > 0) {
    process.exitCode = 1;
  }
}

runMatrix().catch(error => {
  console.error('matrix runner failed:', error);
  process.exitCode = 1;
});
