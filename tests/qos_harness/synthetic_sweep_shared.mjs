const DEFAULT_CONFIGURED_BITRATE_BPS = 900000;

export const DEFAULT_SYNTHETIC_BASELINE = {
  bandwidth: 4000,
  rtt: 25,
  loss: 0.1,
  jitter: 5,
};

function clamp(value, min, max) {
  return Math.min(max, Math.max(min, value));
}

function asFiniteNumber(value, fallback) {
  return Number.isFinite(value) ? value : fallback;
}

function normalizeBandwidthStress(bandwidthKbps) {
  return clamp((2500 - bandwidthKbps) / 2000, 0, 1);
}

function normalizeLossStress(lossPct) {
  return clamp(lossPct / 10, 0, 1);
}

function normalizeRttStress(reportedRttMs) {
  return clamp((reportedRttMs - 100) / 250, 0, 1);
}

function normalizeJitterStress(jitterMs) {
  return clamp((jitterMs - 10) / 60, 0, 1);
}

export function stateRank(state) {
  return {
    stable: 0,
    early_warning: 1,
    recovering: 2,
    congested: 3,
  }[state] ?? 99;
}

export function getBaselineNetwork(caseDefn) {
  if (caseDefn.group === 'baseline') {
    return {
      bandwidth: asFiniteNumber(caseDefn.bandwidth, DEFAULT_SYNTHETIC_BASELINE.bandwidth),
      rtt: asFiniteNumber(caseDefn.rtt, DEFAULT_SYNTHETIC_BASELINE.rtt),
      loss: asFiniteNumber(caseDefn.loss, DEFAULT_SYNTHETIC_BASELINE.loss),
      jitter: asFiniteNumber(caseDefn.jitter, DEFAULT_SYNTHETIC_BASELINE.jitter),
    };
  }

  return {
    bandwidth: asFiniteNumber(caseDefn.baselineBw, DEFAULT_SYNTHETIC_BASELINE.bandwidth),
    rtt: asFiniteNumber(caseDefn.baselineRtt, DEFAULT_SYNTHETIC_BASELINE.rtt),
    loss: asFiniteNumber(caseDefn.baselineLoss, DEFAULT_SYNTHETIC_BASELINE.loss),
    jitter: asFiniteNumber(caseDefn.baselineJitter, DEFAULT_SYNTHETIC_BASELINE.jitter),
  };
}

export function getPhaseNetwork(caseDefn, phase) {
  const baseline = getBaselineNetwork(caseDefn);

  if (phase === 'baseline' || phase === 'recovery' || caseDefn.group === 'baseline') {
    return baseline;
  }

  return {
    bandwidth: asFiniteNumber(caseDefn.bandwidth, baseline.bandwidth),
    rtt: asFiniteNumber(caseDefn.rtt, baseline.rtt),
    loss: asFiniteNumber(caseDefn.loss, baseline.loss),
    jitter: asFiniteNumber(caseDefn.jitter, baseline.jitter),
  };
}

export function toSyntheticCondition(network) {
  const bandwidthKbps = asFiniteNumber(network.bandwidth, DEFAULT_SYNTHETIC_BASELINE.bandwidth);
  const lossPct = asFiniteNumber(network.loss, DEFAULT_SYNTHETIC_BASELINE.loss);
  const jitterMs = asFiniteNumber(network.jitter, DEFAULT_SYNTHETIC_BASELINE.jitter);
  const baseRttMs = asFiniteNumber(network.rtt, DEFAULT_SYNTHETIC_BASELINE.rtt);
  const reportedRttMs = Math.round(baseRttMs + Math.min(baseRttMs, 100));

  const bandwidthStress = normalizeBandwidthStress(bandwidthKbps);
  const lossStress = normalizeLossStress(lossPct);
  const rttStress = normalizeRttStress(reportedRttMs);
  const jitterStress = normalizeJitterStress(jitterMs);

  let utilization =
    0.98 -
    0.45 * bandwidthStress -
    0.15 * lossStress -
    0.2 * rttStress -
    0.35 * jitterStress;
  utilization = clamp(utilization, 0.05, 1.05);

  if (bandwidthKbps <= 300 || lossPct >= 40) {
    utilization = Math.min(utilization, 0.18);
  } else if (bandwidthKbps <= 500 || lossPct >= 20) {
    utilization = Math.min(utilization, 0.42);
  }

  const targetBitrateBps = DEFAULT_CONFIGURED_BITRATE_BPS;
  const bitrateBps = Math.max(0, Math.round(targetBitrateBps * utilization));
  const severity = Math.max(bandwidthStress, lossStress, rttStress, jitterStress);

  return {
    bitrateBps,
    targetBitrateBps,
    lossRate: clamp(lossPct / 100, 0, 1),
    rttMs: reportedRttMs,
    jitterMs,
    qualityLimitationReason:
      severity >= 0.85 || utilization < 0.55 ? 'bandwidth' : 'none',
  };
}

export function getCaseExpectation(caseDefn, runner = 'default') {
  const runnerSpecific = caseDefn?.expectByRunner?.[runner];
  if (runnerSpecific && typeof runnerSpecific === 'object') {
    return runnerSpecific;
  }

  return caseDefn?.expect ?? {};
}

function expectsRecovery(caseDefn, runner = 'default') {
  const expectation = getCaseExpectation(caseDefn, runner);

  if (expectation?.recovery === false) {
    return false;
  }

  return (
    caseDefn.group === 'transition' ||
    caseDefn.group === 'burst' ||
    expectation?.recovery === true
  );
}

function getAllowedStates(expectation) {
  if (Array.isArray(expectation?.states) && expectation.states.length > 0) {
    return expectation.states;
  }

  if (typeof expectation?.state === 'string' && expectation.state.length > 0) {
    return [expectation.state];
  }

  return ['stable'];
}

export function evaluateExpectation(state, expect = {}) {
  const stateName = state?.state;
  const level = typeof state?.level === 'number' ? state.level : undefined;

  let stateMatch = true;
  if (Array.isArray(expect.states)) {
    stateMatch = expect.states.includes(stateName);
  } else if (typeof expect.state === 'string') {
    stateMatch = expect.state === stateName;
  }

  let levelMatch = true;
  if (typeof expect.maxLevel === 'number' && typeof level === 'number') {
    levelMatch = level <= expect.maxLevel && levelMatch;
  }
  if (typeof expect.minLevel === 'number' && typeof level === 'number') {
    levelMatch = level >= expect.minLevel && levelMatch;
  }

  return { stateMatch, levelMatch };
}

export function getImpairedStateForEvaluation(caseDefn, impairmentSummary, baselineSummary = null) {
  if (caseDefn.group === 'baseline') {
    return baselineSummary?.current ?? impairmentSummary?.current;
  }
  return impairmentSummary?.peak;
}

export function recoveryPassedForCase(caseDefn, baselineState, recoveredState, runner = 'default') {
  if (!expectsRecovery(caseDefn, runner)) {
    return true;
  }

  return (
    stateRank(recoveredState?.state) <= stateRank(baselineState?.state) &&
    recoveredState?.level <= baselineState?.level
  );
}

export function extractTiming(trace, phaseStartMs, phaseEndMs) {
  const timing = {};
  const find = predicate => {
    const entry = trace.find(entry => {
      if (entry.tsMs <= phaseStartMs) {
        return false;
      }
      if (typeof phaseEndMs === 'number' && entry.tsMs > phaseEndMs) {
        return false;
      }
      return predicate(entry);
    });
    return entry ? entry.tsMs - phaseStartMs : null;
  };

  timing.t_detect_warning = find(entry => entry.stateAfter === 'early_warning');
  timing.t_detect_recovering = find(entry => entry.stateAfter === 'recovering');
  timing.t_detect_stable = find(entry => entry.stateAfter === 'stable');
  timing.t_detect_congested = find(entry => entry.stateAfter === 'congested');
  timing.t_first_action = find(
    entry => entry.plannedAction?.type && entry.plannedAction.type !== 'noop'
  );

  for (let level = 0; level <= 4; level += 1) {
    timing[`t_level_${level}`] = find(
      entry =>
        entry.plannedAction?.type &&
        entry.plannedAction.type !== 'noop' &&
        entry.plannedAction.level === level
    );
  }

  timing.t_audio_only = find(entry => entry.plannedAction?.type === 'enterAudioOnly');

  return timing;
}

export function summarizePhaseState(trace, phaseStartMs, fallbackState, phaseEndMs) {
  const current = {
    state: fallbackState?.state ?? 'stable',
    level: typeof fallbackState?.level === 'number' ? fallbackState.level : 0,
  };
  const peak = { ...current };
  const best = { ...current };

  const phaseEntries = (trace ?? []).filter(entry => {
    if (!entry || typeof entry.tsMs !== 'number') {
      return false;
    }
    if (entry.tsMs <= phaseStartMs) {
      return false;
    }
    if (typeof phaseEndMs === 'number' && entry.tsMs > phaseEndMs) {
      return false;
    }
    return true;
  });

  for (const entry of phaseEntries) {
    const state = typeof entry.stateAfter === 'string' ? entry.stateAfter : current.state;
    const level =
      typeof entry?.plannedAction?.level === 'number'
        ? entry.plannedAction.level
        : current.level;

    if (
      stateRank(state) > stateRank(peak.state) ||
      (stateRank(state) === stateRank(peak.state) && level > peak.level)
    ) {
      peak.state = state;
      peak.level = level;
    }
    if (
      stateRank(state) < stateRank(best.state) ||
      (stateRank(state) === stateRank(best.state) && level < best.level)
    ) {
      best.state = state;
      best.level = level;
    }
  }

  return {
    current,
    peak,
    best,
    entries: phaseEntries,
  };
}

export function analyzeResult(caseDefn, baseline, impaired, recovered, runner = 'default') {
  const expectation = getCaseExpectation(caseDefn, runner);
  const allowedStates = getAllowedStates(expectation);
  const allowedRanks = allowedStates.map(stateRank).sort((left, right) => left - right);
  const impairedRank = stateRank(impaired.state);
  const minLevel = expectation.minLevel ?? 0;
  const maxLevel = expectation.maxLevel ?? Infinity;

  let verdict = '符合';

  if (impairedRank < allowedRanks[0] || impaired.level < minLevel) {
    verdict = '过弱';
  } else if (
    impairedRank > allowedRanks[allowedRanks.length - 1] ||
    impaired.level > maxLevel
  ) {
    verdict = '过强';
  } else if (
    verdict === '符合' &&
    expectsRecovery(caseDefn, runner) &&
    (stateRank(recovered.state) > stateRank(baseline.state) || recovered.level > baseline.level)
  ) {
    verdict = '恢复过慢';
  }

  return {
    expectedStates: allowedStates,
    expectedMinLevel: minLevel,
    expectedMaxLevel: maxLevel,
    actualBaselineState: baseline.state,
    actualBaselineLevel: baseline.level,
    actualImpairedState: impaired.state,
    actualImpairedLevel: impaired.level,
    actualRecoveredState: recovered.state,
    actualRecoveredLevel: recovered.level,
    verdict,
  };
}

export function deriveCaseEvaluation(
  caseDefn,
  baselineState,
  impairedState,
  recoveredState,
  runner = 'default'
) {
  const expectation = getCaseExpectation(caseDefn, runner);
  const analysis = analyzeResult(caseDefn, baselineState, impairedState, recoveredState, runner);
  const expectationMatch = evaluateExpectation(impairedState, expectation);
  const recoveryPassed = recoveryPassedForCase(caseDefn, baselineState, recoveredState, runner);
  const passed =
    expectationMatch.stateMatch &&
    expectationMatch.levelMatch &&
    analysis.verdict === '符合' &&
    recoveryPassed;

  return {
    passed,
    analysis,
    expectation: expectationMatch,
    recoveryPassed,
    reason: passed
      ? 'expectation satisfied'
      : `stateMatch=${expectationMatch.stateMatch}, levelMatch=${expectationMatch.levelMatch}, recoveryPassed=${recoveryPassed}, analysis=${analysis.verdict}`,
  };
}
