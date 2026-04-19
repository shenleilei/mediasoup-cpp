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
  getCaseExpectation,
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
import {
  filterScenarioCatalog,
  loadScenarioCatalog,
} from './scenario_catalog.mjs';

const __dirname = path.dirname(new URL(import.meta.url).pathname);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);
const caseArg = args.find(arg => arg.startsWith('--cases='));
const selectedCases = caseArg
  ? new Set(caseArg.replace('--cases=', '').split(',').map(id => id.trim()).filter(Boolean))
  : null;
const includeExtended = args.includes('--include-extended');
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

function fmtState(state) {
  if (!state) {
    return '-';
  }
  return `${state.state}/L${state.level}`;
}

function compactJson(value) {
  try {
    return JSON.stringify(value);
  } catch {
    return String(value);
  }
}

function emitDiagnostics(caseId, diagnostics) {
  if (!diagnostics) {
    return;
  }

  const snapshot = diagnostics.currentSnapshot;
  const host = snapshot?.host;
  const node = snapshot?.node;
  const browser = snapshot?.browser;
  const browserState = [
    `browserPid=${browser?.pid ?? '-'}`,
    `connected=${browser?.connected ?? '-'}`,
    `alive=${browser?.alive ?? '-'}`,
    `browserDisconnected=${diagnostics.browserDisconnected ?? '-'}`,
    `pageClosed=${diagnostics.pageClosed ?? '-'}`,
  ].join(' ');
  console.error(`[${caseId}] diagnostics summary ${browserState}`);

  if (host || node || browser?.procStatus) {
    console.error(
      `[${caseId}] diagnostics resources ` +
      `freeMemMb=${host?.freeMemMb ?? '-'} totalMemMb=${host?.totalMemMb ?? '-'} ` +
      `loadAvg=${(host?.loadAvg ?? []).join('/') || '-'} ` +
      `nodeRssMb=${node?.rssMb ?? '-'} browserVmRssKb=${browser?.procStatus?.vmRssKb ?? '-'} ` +
      `browserOomScore=${browser?.oomScore ?? '-'} browserOomScoreAdj=${browser?.oomScoreAdj ?? '-'}`
    );
  }

  if (diagnostics.failureContext) {
    console.error(
      `[${caseId}] diagnostics failureContext ${compactJson(diagnostics.failureContext)}`
    );
  }

  const recentEvents = (diagnostics.events ?? []).slice(-12);
  if (recentEvents.length > 0) {
    console.error(`[${caseId}] diagnostics recent events:`);
    for (const event of recentEvents) {
      console.error(
        `[${caseId}]   ${event.ts} ${event.type} ${compactJson(event.details ?? {})}`
      );
    }
  }

  const recentPageErrors = (diagnostics.pageErrors ?? []).slice(-5);
  if (recentPageErrors.length > 0) {
    console.error(`[${caseId}] diagnostics page errors:`);
    for (const entry of recentPageErrors) {
      console.error(`[${caseId}]   ${entry.ts ?? '-'} ${entry.message ?? compactJson(entry)}`);
    }
  }

  const recentCrashes = (diagnostics.pageCrashes ?? []).slice(-5);
  if (recentCrashes.length > 0) {
    console.error(`[${caseId}] diagnostics page crash events:`);
    for (const entry of recentCrashes) {
      console.error(`[${caseId}]   ${entry.ts ?? '-'} ${entry.message ?? compactJson(entry)}`);
    }
  }

  const recentConsole = (diagnostics.console ?? [])
    .filter(entry => entry.type === 'error' || entry.type === 'warning')
    .slice(-8);
  if (recentConsole.length > 0) {
    console.error(`[${caseId}] diagnostics console warnings/errors:`);
    for (const entry of recentConsole) {
      console.error(
        `[${caseId}]   ${entry.ts ?? '-'} ${entry.type}: ${entry.text}` +
        `${entry.location?.url ? ` @ ${entry.location.url}:${entry.location.lineNumber ?? 0}` : ''}`
      );
    }
  }

  const recentRequests = (diagnostics.requestFailures ?? []).slice(-5);
  if (recentRequests.length > 0) {
    console.error(`[${caseId}] diagnostics request failures:`);
    for (const entry of recentRequests) {
      console.error(
        `[${caseId}]   ${entry.ts ?? '-'} ${entry.failure}: ${entry.url ?? '-'}`
      );
    }
  }

  const browserProcessExits = (diagnostics.browserProcessExits ?? []).slice(-5);
  if (browserProcessExits.length > 0) {
    console.error(`[${caseId}] diagnostics browser process exits:`);
    for (const entry of browserProcessExits) {
      console.error(
        `[${caseId}]   ${entry.ts ?? '-'} code=${entry.code ?? '-'} signal=${entry.signal ?? '-'}`
      );
    }
  }

  const recentSnapshots = (diagnostics.runtimeSnapshots ?? []).slice(-6);
  if (recentSnapshots.length > 0) {
    console.error(`[${caseId}] diagnostics runtime snapshots:`);
    for (const entry of recentSnapshots) {
      console.error(
        `[${caseId}]   ${entry.reason ?? '-'} @ ${entry.capturedAt ?? '-'} ` +
        `freeMemMb=${entry.host?.freeMemMb ?? '-'} nodeRssMb=${entry.node?.rssMb ?? '-'} ` +
        `browserVmRssKb=${entry.browser?.procStatus?.vmRssKb ?? '-'} browserAlive=${entry.browser?.alive ?? '-'}`
      );
    }
  }

  const relevantProcesses = diagnostics.relevantProcesses ?? [];
  if (relevantProcesses.length > 0) {
    console.error(`[${caseId}] diagnostics relevant processes:`);
    for (const line of relevantProcesses) {
      console.error(`[${caseId}]   ${line}`);
    }
  }

  const kernelTail = diagnostics.kernelTail ?? [];
  if (kernelTail.length > 0) {
    console.error(`[${caseId}] diagnostics kernel tail:`);
    for (const line of kernelTail) {
      console.error(`[${caseId}]   ${line}`);
    }
  }
}

async function runPhase(harness, phaseName, config, durationMs) {
  if (Object.keys(config).length > 0) {
    applyNetemConfig(config);
  } else {
    clearNetem();
  }

  const runtimeStart = await harness.captureRuntimeSnapshot?.(`${phaseName}:start`);
  const startMs = await harness.nowMs();
  await sleep(durationMs);
  const state = await harness.getState();
  const runtimeEnd = await harness.captureRuntimeSnapshot?.(`${phaseName}:end`);
  return {
    startMs,
    durationMs,
    netem: config,
    state,
    runtime: {
      start: runtimeStart ?? null,
      end: runtimeEnd ?? null,
    },
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
    harness = await createLoopbackHarness({ caseId: caseDef.caseId });
    const baselineNetwork = getPhaseNetwork(caseDef, 'baseline');
    const impairedNetwork = getPhaseNetwork(caseDef, 'impaired');
    const recoveryNetwork = getPhaseNetwork(caseDef, 'recovery');

    const baseline = await runPhase(
      harness,
      'baseline',
      toNetemConfig(baselineNetwork),
      scaleDuration(caseDef.baselineMs, 15000)
    );
    const impairment = await runPhase(
      harness,
      'impairment',
      toNetemConfig(impairedNetwork),
      scaleDuration(caseDef.impairmentMs, 20000)
    );

    let recovery;
    if (getCaseExpectation(caseDef, 'loopback')?.recovery === false) {
      recovery = {
        startMs: await harness.nowMs(),
        durationMs: 0,
        netem: toNetemConfig(impairedNetwork),
        state: impairment.state,
      };
    } else {
      recovery = await runPhase(
        harness,
        'recovery',
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
      impairmentSummary,
      baselineSummary
    );
    const actionTypes = fullTrace
      .map(entry => entry?.plannedAction?.type)
      .filter(type => type && type !== 'noop');
    const evaluation = deriveCaseEvaluation(
      caseDef,
      baselineSummary.current,
      impairedStateForEvaluation,
      recoverySummary.best,
      'loopback'
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
        getCaseExpectation(caseDef, 'loopback')?.recovery === false
          ? {}
          : extractTiming(fullTrace, recovery.startMs),
    };
    result.analysis = evaluation.analysis;
    result.actionCount = actionTypes.length;
    result.actionTypes = actionTypes;
    result.verdict = {
      passed: evaluation.passed,
      reason: evaluation.reason,
      expectation: getCaseExpectation(caseDef, 'loopback'),
    };
    result.endTime = new Date().toISOString();
    return result;
  } catch (error) {
    if (harness?.getDiagnostics) {
      try {
        error.qosDiagnostics = await harness.getDiagnostics({
          reason: 'run_case_error',
          caseId: caseDef.caseId,
          errorMessage: error?.message ?? String(error),
          errorStack: error?.stack ?? null,
        });
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

  const rawCases = loadScenarioCatalog();
  const cases = filterScenarioCatalog(rawCases, {
    selectedCaseIds: selectedCases,
    includeExtended,
  });
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
      if (!result.verdict.passed) {
        console.error(
          `[${caseDef.caseId}] detail baseline(current=${fmtState(result.phaseSummary?.baseline?.current)}) ` +
          `impairment(peak=${fmtState(result.phaseSummary?.impairment?.peak)}, current=${fmtState(result.phaseSummary?.impairment?.current)}) ` +
          `recovery(best=${fmtState(result.phaseSummary?.recovery?.best)}, current=${fmtState(result.phaseSummary?.recovery?.current)})`
        );
        console.error(
          `[${caseDef.caseId}] verdict reason=${result.verdict.reason} analysis=${result.analysis?.verdict ?? 'unknown'}`
        );
      }
    } else {
      console.error(`case ${caseDef.caseId} failed: ${lastError.message}`);
      emitDiagnostics(caseDef.caseId, lastError.qosDiagnostics);
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
    includeExtended,
    selectedCaseIds: selectedCases ? [...selectedCases] : 'all',
    includedCaseIds: cases.map(caseDef => caseDef.caseId),
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
