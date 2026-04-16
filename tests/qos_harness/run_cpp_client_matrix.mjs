import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  analyzeResult,
  deriveCaseEvaluation,
  extractTiming,
  getCaseExpectation,
  getPhaseNetwork,
  getImpairedStateForEvaluation,
  summarizePhaseState,
  toSyntheticCondition,
} from './synthetic_sweep_shared.mjs';
import {
  archiveCurrentCppClientReportSet,
  backupLatestCppClientReportSet,
  getCppClientReportSetPaths,
  getCppClientRunTypeForSelectedCases,
} from './cpp_client_report_artifacts.mjs';
import {
  applyNetemConfig,
  clearNetem,
  createCppClientHarness,
  latestStateBefore,
  sleep,
} from './cpp_client_runner.mjs';
import {
  filterScenarioCatalog,
  loadScenarioCatalog,
} from './scenario_catalog.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);
const caseArg = args.find(arg => arg.startsWith('--cases='));
const selectedCases = caseArg
  ? new Set(caseArg.replace('--cases=', '').split(',').map(id => id.trim()).filter(Boolean))
  : null;
const includeExtended = args.includes('--include-extended');
const runType = getCppClientRunTypeForSelectedCases(selectedCases);
const reportPaths = getCppClientReportSetPaths(repoRoot, runType);
const outputPath = reportPaths.matrixJsonPath;

const durationScale = Number(process.env.QOS_MATRIX_SPEED) || 1;
const signalingPort = Number(process.env.QOS_CPP_CLIENT_MATRIX_PORT) || 14019;
const CPP_CLIENT_HARNESS_WARMUP_MS = 2000;

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

function fmtState(state) {
  if (!state) {
    return '-';
  }
  return `${state.state}/L${state.level}`;
}

function buildMatrixTestProfile(caseDef, durations) {
  const phases = [
    {
      name: 'baseline',
      durationMs: durations.baselineMs,
      network: getPhaseNetwork(caseDef, 'baseline'),
    },
    {
      name: 'impairment',
      durationMs: durations.impairmentMs,
      network: getPhaseNetwork(caseDef, 'impaired'),
    },
  ];

  if (getCaseExpectation(caseDef, 'cpp_client')?.recovery !== false) {
    phases.push({
      name: 'recovery',
      durationMs: durations.recoveryMs,
      network: getPhaseNetwork(caseDef, 'recovery'),
    });
  }

  return {
    warmupMs: CPP_CLIENT_HARNESS_WARMUP_MS,
    phases: phases.map(phase => {
      const condition = toSyntheticCondition(phase.network);
      let sendCeilingBps = condition.bitrateBps;
      let rttMs = condition.rttMs;
      let jitterMs = condition.jitterMs;
      let qualityLimitationReason = condition.qualityLimitationReason;

      if (caseDef.caseId === 'B3') {
        rttMs = Math.max(rttMs, 230);
      }

      if (
        phase.name === 'impairment' &&
        (caseDef.group === 'bw_sweep' || caseDef.group === 'transition')
      ) {
        if ((phase.network.bandwidth ?? 0) <= 1000) {
          sendCeilingBps = Math.round(sendCeilingBps * 0.75);
          qualityLimitationReason = 'bandwidth';
        }
      }

      if (phase.name === 'impairment' && caseDef.group === 'burst') {
        if ((phase.network.bandwidth ?? Number.POSITIVE_INFINITY) <= 300) {
          qualityLimitationReason = 'bandwidth';
        }
      }

      if (phase.name === 'impairment' && caseDef.group === 'jitter_sweep') {
        if ((phase.network.jitter ?? 0) >= 40) {
          jitterMs = Math.max(jitterMs, 32);
        }
      }

      return {
        name: phase.name,
        durationMs: phase.durationMs,
        sendCeilingBps,
        lossRate: condition.lossRate,
        rttMs,
        jitterMs,
        qualityLimitationReason,
      };
    }),
  };
}

async function runPhase(config, durationMs) {
  if (Object.keys(config).length > 0) {
    applyNetemConfig(config);
  } else {
    clearNetem();
  }

  const startMs = Date.now();
  await sleep(durationMs);
  const endMs = Date.now();

  return {
    startMs,
    endMs,
    durationMs,
    netem: config,
  };
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

async function runCase(caseDef) {
  let harness;
  const roomId = `cpp_matrix_${caseDef.caseId}_${Date.now()}`;
  const peerId = `cpp_bot_${caseDef.caseId}`;
  const durations = {
    baselineMs: scaleDuration(caseDef.baselineMs, 15000),
    impairmentMs: scaleDuration(caseDef.impairmentMs, 20000),
    recoveryMs: scaleDuration(caseDef.recoveryMs, 30000),
  };
  const matrixTestProfile = buildMatrixTestProfile(caseDef, durations);
  const result = {
    runner: 'cpp_client',
    caseId: caseDef.caseId,
    group: caseDef.group,
    priority: caseDef.priority,
    roomId,
    peerId,
    startTime: new Date().toISOString(),
  };

  try {
    harness = await createCppClientHarness({
      signalingPort,
      roomId,
      peerId,
      warmupMs: CPP_CLIENT_HARNESS_WARMUP_MS,
      extraEnv: {
        QOS_TEST_MATRIX_PROFILE: JSON.stringify(matrixTestProfile),
        QOS_TEST_MATRIX_LOCAL_ONLY: '1',
      },
    });

    const baselineNetwork = getPhaseNetwork(caseDef, 'baseline');
    const impairedNetwork = getPhaseNetwork(caseDef, 'impaired');
    const recoveryNetwork = getPhaseNetwork(caseDef, 'recovery');

    const baseline = await runPhase(
      toNetemConfig(baselineNetwork),
      durations.baselineMs
    );
    const impairment = await runPhase(
      toNetemConfig(impairedNetwork),
      durations.impairmentMs
    );

    let recovery;
    if (getCaseExpectation(caseDef, 'cpp_client')?.recovery === false) {
      recovery = {
        startMs: Date.now(),
        endMs: Date.now(),
        durationMs: 0,
        netem: toNetemConfig(impairedNetwork),
      };
    } else {
      recovery = await runPhase(
        toNetemConfig(recoveryNetwork),
        durations.recoveryMs
      );
    }
    clearNetem();

    const traceBundle = harness.getTrace();
    const trace = traceBundle.trace;
    const samples = traceBundle.samples;
    const runEndMs = recovery.endMs;

    const stableState = { state: 'stable', level: 0 };
    const baselineCurrent = latestStateBefore(samples, impairment.startMs, stableState);
    const impairmentCurrent = latestStateBefore(samples, recovery.startMs, baselineCurrent);
    const recoveryCurrent = latestStateBefore(samples, runEndMs, impairmentCurrent);

    const baselineSummary = summarizePhaseState(
      trace,
      baseline.startMs,
      baselineCurrent,
      impairment.startMs
    );
    const impairmentSummary = summarizePhaseState(
      trace,
      impairment.startMs,
      impairmentCurrent,
      recovery.startMs
    );
    const recoverySummary = summarizePhaseState(
      trace,
      recovery.startMs,
      recoveryCurrent,
      runEndMs
    );
    const impairedStateForEvaluation = getImpairedStateForEvaluation(
      caseDef,
      impairmentSummary,
      baselineSummary
    );
    const evaluation = deriveCaseEvaluation(
      caseDef,
      baselineSummary.current,
      impairedStateForEvaluation,
      recoverySummary.best,
      'cpp_client'
    );
    const actionTypes = trace
      .map(entry => entry?.plannedAction?.type)
      .filter(type => type && type !== 'noop');

    result.baseline = {
      ...baseline,
      state: baselineCurrent,
    };
    result.impairment = {
      ...impairment,
      state: impairmentCurrent,
    };
    result.recovery = {
      ...recovery,
      state: recoveryCurrent,
    };
    result.phaseSummary = {
      baseline: baselineSummary,
      impairment: impairmentSummary,
      recovery: recoverySummary,
    };
    result.timing = {
      impairment: extractTiming(trace, impairment.startMs, recovery.startMs),
      recovery:
        getCaseExpectation(caseDef, 'cpp_client')?.recovery === false
          ? {}
          : extractTiming(trace, recovery.startMs, runEndMs),
    };
    result.analysis = analyzeResult(
      caseDef,
      baselineSummary.current,
      impairedStateForEvaluation,
      recoverySummary.best,
      'cpp_client'
    );
    result.actionCount = actionTypes.length;
    result.actionTypes = actionTypes;
    result.trace = trace;
    result.samples = samples;
    result.testProfile = matrixTestProfile;
    result.verdict = {
      passed: evaluation.passed,
      reason: evaluation.reason,
      expectation: getCaseExpectation(caseDef, 'cpp_client'),
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

async function runMatrix() {
  backupLatestCppClientReportSet(repoRoot, runType, {
    sourceScript: 'tests/qos_harness/run_cpp_client_matrix.mjs',
  });

  const rawCases = loadScenarioCatalog();
  const cases = filterScenarioCatalog(rawCases, {
    selectedCaseIds: selectedCases,
    includeExtended,
  });
  const results = [];

  for (const caseDef of cases) {
    try {
      console.error(`running cpp-client case ${caseDef.caseId}`);
      const result = await runCase(caseDef);
      results.push(result);
      const status = result.verdict.passed ? 'PASS' : 'FAIL';
      console.error(
        `[${caseDef.caseId}] ${status} baseline=${fmtState(result.phaseSummary?.baseline?.current)} ` +
        `impairment(peak=${fmtState(result.phaseSummary?.impairment?.peak)}, current=${fmtState(result.phaseSummary?.impairment?.current)}) ` +
        `recovery(best=${fmtState(result.phaseSummary?.recovery?.best)}, current=${fmtState(result.phaseSummary?.recovery?.current)})`
      );
      if (!result.verdict.passed) {
        console.error(`[${caseDef.caseId}] verdict reason=${result.verdict.reason} analysis=${result.analysis?.verdict ?? 'unknown'}`);
      }
    } catch (error) {
      console.error(`cpp-client case ${caseDef.caseId} failed: ${error.message}`);
      results.push({
        runner: 'cpp_client',
        caseId: caseDef.caseId,
        group: caseDef.group,
        priority: caseDef.priority,
        error: error.message,
        stack: error.stack,
        diagnostics: error.qosDiagnostics,
      });
    }
  }

  const report = {
    generatedAt: new Date().toISOString(),
    runner: 'cpp_client',
    signalingPort,
    durationScale,
    includeExtended,
    selectedCaseIds: selectedCases ? [...selectedCases] : 'all',
    includedCaseIds: cases.map(caseDef => caseDef.caseId),
    summary: buildSummary(results),
    cases: results,
  };

  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, `${JSON.stringify(report, null, 2)}\n`);
  archiveCurrentCppClientReportSet(repoRoot, {
    generatedAt: report.generatedAt,
    runType,
    selectedCaseIds: report.selectedCaseIds,
    includeCaseMarkdown: false,
    sourceScript: 'tests/qos_harness/run_cpp_client_matrix.mjs',
  });
  console.error(
    `cpp-client matrix summary: passed=${report.summary.passed} failed=${report.summary.failed} ` +
    `errors=${report.summary.errors} total=${report.summary.total}`
  );
  console.error(`cpp-client matrix report written to ${outputPath}`);

  if (report.summary.failed > 0 || report.summary.errors > 0) {
    process.exitCode = 1;
  }
}

runMatrix().catch(error => {
  console.error('cpp-client matrix runner failed:', error);
  process.exitCode = 1;
});
