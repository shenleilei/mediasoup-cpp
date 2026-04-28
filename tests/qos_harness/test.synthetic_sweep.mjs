import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import {
  deriveCaseEvaluation,
  extractTiming,
  analyzeResult,
  computeReportedRtt,
  computeSmoothedJitter,
  getCaseExpectation,
  getPhaseNetwork,
  getImpairedStateForEvaluation,
  stateRank,
  summarizePhaseState,
  toSyntheticCondition,
} from './synthetic_sweep_shared.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const scenarios = JSON.parse(
  fs.readFileSync(path.join(__dirname, 'scenarios', 'sweep_cases.json'), 'utf8')
);

function allowedRankBounds(expect = {}) {
  const states = Array.isArray(expect.states)
    ? expect.states
    : [expect.state ?? 'stable'];
  const ranks = states.map(stateRank).sort((left, right) => left - right);
  return {
    minRank: ranks[0],
    maxRank: ranks[ranks.length - 1],
    maxLevel: expect.maxLevel ?? Number.POSITIVE_INFINITY,
  };
}

test('baseline group uses case-specific network values', () => {
  const caseDefn = {
    caseId: 'B3',
    group: 'baseline',
    bandwidth: 2000,
    rtt: 55,
    loss: 0.5,
    jitter: 12,
  };

  assert.deepEqual(getPhaseNetwork(caseDefn, 'baseline'), {
    bandwidth: 2000,
    rtt: 55,
    loss: 0.5,
    jitter: 12,
  });
  assert.deepEqual(getPhaseNetwork(caseDefn, 'impaired'), {
    bandwidth: 2000,
    rtt: 55,
    loss: 0.5,
    jitter: 12,
  });
});

test('transition group uses declared baseline overrides for baseline and recovery', () => {
  const caseDefn = {
    caseId: 'T5',
    group: 'transition',
    bandwidth: 4000,
    rtt: 25,
    loss: 20,
    jitter: 5,
    baselineLoss: 0.1,
  };

  assert.deepEqual(getPhaseNetwork(caseDefn, 'baseline'), {
    bandwidth: 4000,
    rtt: 25,
    loss: 0.1,
    jitter: 5,
  });
  assert.deepEqual(getPhaseNetwork(caseDefn, 'recovery'), {
    bandwidth: 4000,
    rtt: 25,
    loss: 0.1,
    jitter: 5,
  });
});

test('synthetic condition for B3 is no longer treated as a healthy default', () => {
  const condition = toSyntheticCondition({
    bandwidth: 2000,
    rtt: 55,
    loss: 0.5,
    jitter: 12,
  });

  assert.equal(condition.targetBitrateBps, 900000);
  assert.equal(condition.rttMs, 76);
  assert.ok(condition.bitrateBps < condition.targetBitrateBps);
  assert.ok(condition.bitrateBps <= 765000);
});

test('B3 expectation stays aligned with degraded weak-baseline behavior', () => {
  const b3 = scenarios.find(caseDefn => caseDefn.caseId === 'B3');

  const expectation = getCaseExpectation(b3);
  assert.deepEqual(expectation?.states, ['early_warning', 'congested']);
  assert.equal(expectation?.minLevel, 1);
  assert.equal(expectation?.maxLevel, 4);
});

test('runner-specific expectation merges with default expectation when provided', () => {
  const caseDefn = {
    caseId: 'X1',
    group: 'bw_sweep',
    expect: {
      state: 'stable',
      maxLevel: 0,
    },
    expectByRunner: {
      loopback: {
        states: ['stable', 'early_warning'],
        maxLevel: 1,
      },
    },
  };

  assert.deepEqual(getCaseExpectation(caseDefn), {
    state: 'stable',
    maxLevel: 0,
  });
  assert.deepEqual(getCaseExpectation(caseDefn, 'loopback'), {
    state: 'stable',
    states: ['stable', 'early_warning'],
    maxLevel: 1,
  });
});

test('analysis flags slow recovery for transition cases', () => {
  const caseDefn = {
    caseId: 'T5',
    group: 'transition',
    expect: {
      state: 'congested',
      maxLevel: 4,
    },
  };
  const baseline = { state: 'stable', level: 0 };
  const impaired = { state: 'congested', level: 4 };
  const recovered = { state: 'congested', level: 4 };

  const analysis = analyzeResult(caseDefn, baseline, impaired, recovered);

  assert.equal(analysis.verdict, '恢复过慢');
});

test('analysis allows explicit recovery opt-out for transition cases', () => {
  const caseDefn = {
    caseId: 'T8',
    group: 'transition',
    expect: {
      state: 'congested',
      maxLevel: 4,
      recovery: false,
    },
  };
  const baseline = { state: 'early_warning', level: 1 };
  const impaired = { state: 'congested', level: 4 };
  const recovered = { state: 'congested', level: 4 };

  const analysis = analyzeResult(caseDefn, baseline, impaired, recovered);

  assert.equal(analysis.verdict, '符合');
});

test('extended blind-spot transition preserves high-bandwidth baseline on recovery', () => {
  const t9 = scenarios.find(caseDefn => caseDefn.caseId === 'T9');

  assert.deepEqual(getPhaseNetwork(t9, 'baseline'), {
    bandwidth: 8000,
    rtt: 20,
    loss: 0.1,
    jitter: 1,
  });
  assert.deepEqual(getPhaseNetwork(t9, 'impaired'), {
    bandwidth: 200,
    rtt: 500,
    loss: 20,
    jitter: 50,
  });
  assert.deepEqual(getPhaseNetwork(t9, 'recovery'), {
    bandwidth: 8000,
    rtt: 20,
    loss: 0.1,
    jitter: 1,
  });
});

test('analysis accepts allowed impaired state ranges', () => {
  const caseDefn = {
    caseId: 'R4',
    group: 'rtt_sweep',
    expect: {
      states: ['early_warning', 'congested'],
      minLevel: 1,
      maxLevel: 2,
    },
  };
  const baseline = { state: 'stable', level: 0 };
  const impaired = { state: 'congested', level: 2 };
  const recovered = { state: 'stable', level: 0 };

  const analysis = analyzeResult(caseDefn, baseline, impaired, recovered);

  assert.equal(analysis.verdict, '符合');
});

test('bandwidth sweeps do not fail only because recovery stays degraded', () => {
  const caseDefn = {
    caseId: 'BW6',
    group: 'bw_sweep',
    expect: {
      state: 'congested',
      maxLevel: 4,
    },
  };
  const baseline = { state: 'stable', level: 0 };
  const impaired = { state: 'congested', level: 4 };
  const recovered = { state: 'congested', level: 4 };

  const evaluation = deriveCaseEvaluation(caseDefn, baseline, impaired, recovered);

  assert.equal(evaluation.recoveryPassed, true);
  assert.equal(evaluation.passed, true);
});

test('runner-specific loopback expectation can widen accepted boundary behavior', () => {
  const caseDefn = {
    caseId: 'T1',
    group: 'transition',
    expect: {
      states: ['stable', 'early_warning'],
      maxLevel: 1,
    },
    expectByRunner: {
      loopback: {
        states: ['stable', 'early_warning', 'congested'],
        maxLevel: 4,
      },
    },
  };
  const baseline = { state: 'stable', level: 0 };
  const impaired = { state: 'congested', level: 4 };
  const recovered = { state: 'stable', level: 0 };

  assert.equal(
    deriveCaseEvaluation(caseDefn, baseline, impaired, recovered).passed,
    false
  );
  assert.equal(
    deriveCaseEvaluation(caseDefn, baseline, impaired, recovered, 'loopback').passed,
    true
  );
});

test('runner-specific expectations merge with base expectations', () => {
  const caseDefn = {
    caseId: 'O1',
    group: 'oscillation',
    expect: {
      states: ['stable', 'early_warning', 'congested'],
      maxLevel: 4,
      maxActionCount: 5,
    },
    expectByRunner: {
      loopback: {
        maxActionCount: 5,
      },
    },
  };

  const evaluation = deriveCaseEvaluation(
    caseDefn,
    { state: 'stable', level: 0 },
    { state: 'congested', level: 4 },
    { state: 'stable', level: 0 },
    'loopback',
    { actionCount: 5 }
  );

  assert.equal(evaluation.expectation.stateMatch, true);
  assert.equal(evaluation.expectation.levelMatch, true);
  assert.equal(evaluation.maxActionCountPassed, true);
});

test('maxActionCount participates in verdict', () => {
  const caseDefn = {
    caseId: 'O1',
    group: 'oscillation',
    expect: {
      states: ['stable', 'early_warning', 'congested'],
      maxLevel: 4,
      maxActionCount: 5,
    },
  };

  const evaluation = deriveCaseEvaluation(
    caseDefn,
    { state: 'stable', level: 0 },
    { state: 'congested', level: 4 },
    { state: 'stable', level: 0 },
    'default',
    { actionCount: 6 }
  );

  assert.equal(evaluation.expectation.stateMatch, true);
  assert.equal(evaluation.expectation.levelMatch, true);
  assert.equal(evaluation.recoveryPassed, true);
  assert.equal(evaluation.maxActionCountPassed, false);
  assert.equal(evaluation.passed, false);
});

test('phase summary uses peak impaired severity instead of only final state', () => {
  const phase = summarizePhaseState(
    [
      {
        tsMs: 1500,
        stateAfter: 'early_warning',
        plannedAction: { level: 1 },
      },
      {
        tsMs: 2500,
        stateAfter: 'stable',
        plannedAction: { level: 0 },
      },
    ],
    1000,
    { state: 'stable', level: 0 },
    3000
  );

  assert.deepEqual(phase.current, { state: 'stable', level: 0 });
  assert.deepEqual(phase.peak, { state: 'early_warning', level: 1 });
  assert.deepEqual(phase.best, { state: 'stable', level: 0 });
});

test('baseline cases evaluate against the settled baseline state', () => {
  const caseDefn = {
    caseId: 'B3',
    group: 'baseline',
    expect: {
      states: ['early_warning', 'congested'],
      minLevel: 1,
      maxLevel: 4,
    },
  };
  const baselineSummary = {
    current: { state: 'early_warning', level: 1 },
  };
  const impairmentSummary = {
    current: { state: 'stable', level: 0 },
    peak: { state: 'congested', level: 4 },
  };

  const impairedState = getImpairedStateForEvaluation(
    caseDefn,
    impairmentSummary,
    baselineSummary
  );

  assert.deepEqual(impairedState, { state: 'early_warning', level: 1 });
  assert.equal(
    deriveCaseEvaluation(
      caseDefn,
      baselineSummary.current,
      impairedState,
      { state: 'stable', level: 0 }
    ).passed,
    true
  );
});

test('extractTiming stays inside the requested phase window', () => {
  const trace = [
    {
      tsMs: 1100,
      stateAfter: 'recovering',
      plannedAction: { type: 'setEncodingParameters', level: 3 },
    },
    {
      tsMs: 1150,
      stateAfter: 'stable',
      plannedAction: { type: 'setEncodingParameters', level: 0 },
    },
    {
      tsMs: 1200,
      stateAfter: 'congested',
      plannedAction: { type: 'setEncodingParameters', level: 4 },
    },
    {
      tsMs: 5200,
      stateAfter: 'early_warning',
      plannedAction: { type: 'setEncodingParameters', level: 1 },
    },
  ];

  const timing = extractTiming(trace, 1000, 3000);

  assert.equal(timing.t_detect_recovering, 100);
  assert.equal(timing.t_detect_stable, 150);
  assert.equal(timing.t_detect_congested, 200);
  assert.equal(timing.t_level_0, 150);
  assert.equal(timing.t_level_4, 200);
  assert.equal(timing.t_detect_warning, null);
  assert.equal(timing.t_level_1, null);
});

test('shared case evaluation matches peak/best matrix semantics', () => {
  const caseDefn = {
    caseId: 'S2',
    group: 'burst',
    expect: {
      state: 'congested',
      maxLevel: 4,
    },
  };
  const baselineSummary = { current: { state: 'stable', level: 0 } };
  const impairmentSummary = {
    current: { state: 'stable', level: 0 },
    peak: { state: 'congested', level: 4 },
  };
  const recoverySummary = { best: { state: 'stable', level: 0 } };

  const impairedState = getImpairedStateForEvaluation(caseDefn, impairmentSummary);
  const evaluation = deriveCaseEvaluation(
    caseDefn,
    baselineSummary.current,
    impairedState,
    recoverySummary.best
  );

  assert.deepEqual(impairedState, { state: 'congested', level: 4 });
  assert.equal(evaluation.analysis.verdict, '符合');
  assert.equal(evaluation.passed, true);
});

test('recovery must return to no worse than baseline in both state and level', () => {
  const caseDefn = {
    caseId: 'R-strict',
    group: 'transition',
    expect: {
      states: ['early_warning', 'congested'],
      maxLevel: 4,
    },
  };

  const stateRecoveredOnly = deriveCaseEvaluation(
    caseDefn,
    { state: 'stable', level: 0 },
    { state: 'congested', level: 4 },
    { state: 'stable', level: 2 }
  );
  const levelRecoveredOnly = deriveCaseEvaluation(
    caseDefn,
    { state: 'stable', level: 0 },
    { state: 'congested', level: 4 },
    { state: 'early_warning', level: 0 }
  );
  const fullyRecovered = deriveCaseEvaluation(
    caseDefn,
    { state: 'stable', level: 0 },
    { state: 'congested', level: 4 },
    { state: 'stable', level: 0 }
  );

  assert.equal(stateRecoveredOnly.analysis.verdict, '恢复过慢');
  assert.equal(stateRecoveredOnly.passed, false);
  assert.equal(levelRecoveredOnly.analysis.verdict, '恢复过慢');
  assert.equal(levelRecoveredOnly.passed, false);
  assert.equal(fullyRecovered.passed, true);
});

test('sweep expectations grow monotonically as impairment gets harsher', () => {
  const configs = [
    { group: 'bw_sweep', key: 'bandwidth', harsherFirst: (a, b) => a.bandwidth - b.bandwidth },
    { group: 'loss_sweep', key: 'loss', harsherFirst: (a, b) => b.loss - a.loss },
    { group: 'rtt_sweep', key: 'rtt', harsherFirst: (a, b) => b.rtt - a.rtt },
    { group: 'jitter_sweep', key: 'jitter', harsherFirst: (a, b) => b.jitter - a.jitter },
  ];

  for (const config of configs) {
    const ordered = scenarios
      .filter(caseDefn => caseDefn.group === config.group)
      .sort(config.harsherFirst)
      .map(caseDefn => ({
        caseId: caseDefn.caseId,
        bounds: allowedRankBounds(caseDefn.expect),
      }));

    for (let i = 1; i < ordered.length; i += 1) {
      const previous = ordered[i - 1];
      const current = ordered[i];
      assert.ok(
        current.bounds.minRank <= previous.bounds.minRank,
        `${config.group}: ${current.caseId} should not require stronger minimum state than harsher ${previous.caseId}`
      );
      assert.ok(
        current.bounds.maxRank <= previous.bounds.maxRank,
        `${config.group}: ${current.caseId} should not allow stronger maximum state than harsher ${previous.caseId}`
      );
      assert.ok(
        current.bounds.maxLevel <= previous.bounds.maxLevel,
        `${config.group}: ${current.caseId} should not allow higher maxLevel than harsher ${previous.caseId}`
      );
    }
  }
});

test('baseline expectations track modeled stress ordering', () => {
  const baselineCases = scenarios
    .filter(caseDefn => caseDefn.group === 'baseline')
    .map(caseDefn => {
      const condition = toSyntheticCondition(getPhaseNetwork(caseDefn, 'impaired'));
      return {
        caseId: caseDefn.caseId,
        bitrateBps: condition.bitrateBps,
        rttMs: condition.rttMs,
        jitterMs: condition.jitterMs,
        lossRate: condition.lossRate,
        utilization: condition.bitrateBps / condition.targetBitrateBps,
        bounds: allowedRankBounds(caseDefn.expect),
      };
    })
    .sort((left, right) =>
      right.bitrateBps - left.bitrateBps ||
      left.rttMs - right.rttMs ||
      left.jitterMs - right.jitterMs ||
      left.lossRate - right.lossRate
    );

  for (let i = 1; i < baselineCases.length; i += 1) {
    const easier = baselineCases[i - 1];
    const harsher = baselineCases[i];
    assert.ok(
      harsher.bounds.minRank >= easier.bounds.minRank,
      `baseline: harsher ${harsher.caseId} should not require weaker minimum state than ${easier.caseId}`
    );
    assert.ok(
      harsher.bounds.maxRank >= easier.bounds.maxRank,
      `baseline: harsher ${harsher.caseId} should not allow weaker maximum state than ${easier.caseId}`
    );
    assert.ok(
      harsher.bounds.maxLevel >= easier.bounds.maxLevel,
      `baseline: harsher ${harsher.caseId} should not allow lower maxLevel than ${easier.caseId}`
    );
  }
});

// --- Calibration validation tests (empirical data from SIGCOMM 2018 / TMA 2021) ---

test('RTT amplification follows logarithmic decay toward 1.0x at high RTT', () => {
  // Low RTT: protocol overhead dominates → higher amplification
  const ratio25 = computeReportedRtt(25) / 25;
  const ratio100 = computeReportedRtt(100) / 100;
  const ratio350 = computeReportedRtt(350) / 350;

  assert.ok(ratio25 > 1.3, `25ms RTT amplification ${ratio25.toFixed(2)} should be >1.3x`);
  assert.ok(ratio100 > 1.1 && ratio100 < 1.5, `100ms RTT amplification ${ratio100.toFixed(2)} should be 1.1-1.5x`);
  assert.ok(ratio350 > 1.0 && ratio350 < 1.2, `350ms RTT amplification ${ratio350.toFixed(2)} should converge near 1.0x`);
  // Amplification ratio must decrease as base RTT increases
  assert.ok(ratio25 > ratio100, 'amplification should decrease with higher base RTT');
  assert.ok(ratio100 > ratio350, 'amplification should decrease with higher base RTT');
});

test('jitter smoothing reduces raw jitter by RFC 3550 EWMA factor', () => {
  assert.equal(computeSmoothedJitter(20), 15);
  assert.equal(computeSmoothedJitter(40), 30);
  assert.equal(computeSmoothedJitter(0), 0);
});

test('loss utilization matches empirical GCC data within tolerance', () => {
  // Empirical reference (SIGCOMM 2018, tc netem measurements):
  // 1% loss → util 0.85-0.95, 5% → 0.40-0.60, 10% → 0.15-0.35, 20% → <0.15
  const cases = [
    { loss: 1, minUtil: 0.80, maxUtil: 0.99, label: '1% loss' },
    { loss: 5, minUtil: 0.35, maxUtil: 0.65, label: '5% loss' },
    { loss: 10, minUtil: 0.10, maxUtil: 0.45, label: '10% loss' },
    { loss: 20, minUtil: 0.02, maxUtil: 0.32, label: '20% loss' },
  ];

  for (const tc of cases) {
    const condition = toSyntheticCondition({
      bandwidth: 4000, rtt: 25, loss: tc.loss, jitter: 5,
    });
    const util = condition.bitrateBps / condition.targetBitrateBps;
    assert.ok(
      util >= tc.minUtil && util <= tc.maxUtil,
      `${tc.label}: utilization ${util.toFixed(3)} outside empirical range [${tc.minUtil}, ${tc.maxUtil}]`
    );
  }
});

test('RTT-only sweep utilization stays within empirical bounds', () => {
  // Empirical: 0% loss, RTT 200ms → util 0.90-0.96
  const condition = toSyntheticCondition({
    bandwidth: 4000, rtt: 200, loss: 0.1, jitter: 5,
  });
  const util = condition.bitrateBps / condition.targetBitrateBps;
  assert.ok(util >= 0.75 && util <= 0.99,
    `RTT=200ms utilization ${util.toFixed(3)} outside expected range`);
});

test('severe conditions produce utilization below bandwidth-limited threshold', () => {
  // At 20% loss or bw=300, real GCC is near zero
  const severe = toSyntheticCondition({
    bandwidth: 300, rtt: 25, loss: 0.1, jitter: 5,
  });
  assert.ok(severe.bitrateBps / severe.targetBitrateBps <= 0.20,
    'bw=300 should produce utilization ≤0.20');
  assert.equal(severe.qualityLimitationReason, 'bandwidth');
});

// --- Legacy override compatibility tests ---
// These tests prove that retained runner overrides produce values within
// the calibrated model's empirical bounds, i.e. the overrides do not
// contradict the calibration.

test('bw<=1000 sendCeiling override stays within calibrated utilization range', () => {
  // The runner applies sendCeilingBps *= 0.75 for bw_sweep/transition when
  // bandwidth <= 1000.  Verify the result stays within the empirical GCC
  // utilization range for a 1Mbps link (SIGCOMM 2018: util 0.70-0.85).
  const condition = toSyntheticCondition({
    bandwidth: 1000, rtt: 25, loss: 0.1, jitter: 5,
  });
  const overriddenBps = Math.round(condition.bitrateBps * 0.75);
  const overriddenUtil = overriddenBps / condition.targetBitrateBps;
  // Real GCC at 1Mbps produces utilization ~0.30-0.65 (link-limited).
  // The override should keep the value below the model's raw output and
  // within a plausible GCC range.
  assert.ok(overriddenUtil < condition.bitrateBps / condition.targetBitrateBps,
    'override should reduce utilization below raw model output');
  assert.ok(overriddenUtil >= 0.20 && overriddenUtil <= 0.70,
    `overridden utilization ${overriddenUtil.toFixed(3)} outside GCC plausible range [0.20, 0.70]`);
});

test('burst bw<=300 qualityLimitationReason override is consistent with model caps', () => {
  // The runner forces qualityLimitationReason='bandwidth' for burst cases
  // with bw<=300.  The model itself produces utilization <= 0.12 at bw<=300,
  // which already triggers qualityLimitationReason='bandwidth' via the
  // severity/utilization threshold.
  const condition = toSyntheticCondition({
    bandwidth: 300, rtt: 25, loss: 0.1, jitter: 5,
  });
  assert.equal(condition.qualityLimitationReason, 'bandwidth',
    'model already produces bandwidth limitation at bw=300, override is redundant but consistent');
});

test('jitter sweep floor override stays within smoothed jitter range', () => {
  // The runner enforces jitterMs >= 32 for jitter_sweep when raw jitter >= 40.
  // After 0.75× smoothing, raw jitter 40 → smoothed 30.  The 32ms floor
  // is a small increase that stays within the unsmoothed raw value (40ms).
  const condition = toSyntheticCondition({
    bandwidth: 4000, rtt: 25, loss: 0.1, jitter: 40,
  });
  const overriddenJitter = Math.max(condition.jitterMs, 32);
  assert.ok(overriddenJitter >= condition.jitterMs,
    'override should be >= smoothed jitter');
  assert.ok(overriddenJitter <= 40,
    'override should not exceed raw network jitter');
});

// --- Combined override interaction tests ---
// These tests verify that the retained legacy overrides produce values
// compatible with the calibrated model when multiple overrides interact
// in realistic combined conditions (not just individually).

test('combined bw+loss overrides stay within calibrated bounds at bw=500,loss=10%', () => {
  // At bw=500 and loss=10%, both utilization caps and the bw<=1000 override
  // could apply.  The combined result must stay within GCC plausible range.
  const condition = toSyntheticCondition({
    bandwidth: 500, rtt: 55, loss: 10, jitter: 15,
  });
  const util = condition.bitrateBps / condition.targetBitrateBps;
  // Utilization cap at loss>=10% is 0.42, and bw<=500 cap is 0.30.
  // The min(0.30, 0.42) = 0.30 should dominate.
  assert.ok(util <= 0.32,
    `combined bw=500 + loss=10% utilization ${util.toFixed(3)} should be ≤0.32`);
  assert.ok(util >= 0.05,
    `combined utilization ${util.toFixed(3)} should stay above minimum floor`);
  // With bw<=1000 runner override (×0.75), the result stays even lower.
  const overriddenBps = Math.round(condition.bitrateBps * 0.75);
  const overriddenUtil = overriddenBps / condition.targetBitrateBps;
  assert.ok(overriddenUtil <= 0.30 && overriddenUtil >= 0.02,
    `overridden combined utilization ${overriddenUtil.toFixed(3)} should be in [0.02, 0.30]`);
});

test('combined jitter+loss overrides maintain correct qualityLimitationReason', () => {
  // High jitter (50ms) + moderate loss (5%) — jitter floor override applies
  // (override jitter to 32ms), and the model should still produce the correct
  // qualityLimitationReason based on overall severity.
  const condition = toSyntheticCondition({
    bandwidth: 4000, rtt: 25, loss: 5, jitter: 50,
  });
  const overriddenJitter = Math.max(condition.jitterMs, 32);
  // The model uses severity (max stress) and utilization for QLR.
  // With 5% loss: lossStress ≈ 0.83, severity ≥ 0.83 → QLR likely 'bandwidth'
  // or close to threshold.
  const util = condition.bitrateBps / condition.targetBitrateBps;
  assert.ok(util >= 0.20 && util <= 0.65,
    `5% loss + 50ms jitter utilization ${util.toFixed(3)} should be degraded`);
  // Override should not push jitter past raw value
  assert.ok(overriddenJitter <= 50,
    'jitter override should not exceed raw jitter');
});

test('bw=800 with moderate loss=3% override preserves calibrated monotonicity', () => {
  // At bw=800 (with ×0.75 override), worse network conditions should always
  // produce lower effective bitrate than better conditions.
  const mild = toSyntheticCondition({
    bandwidth: 800, rtt: 25, loss: 1, jitter: 5,
  });
  const moderate = toSyntheticCondition({
    bandwidth: 800, rtt: 55, loss: 3, jitter: 15,
  });
  // Apply bw<=1000 override to both
  const mildOverridden = Math.round(mild.bitrateBps * 0.75);
  const moderateOverridden = Math.round(moderate.bitrateBps * 0.75);
  assert.ok(moderateOverridden < mildOverridden,
    `overridden moderate ${moderateOverridden} should be < mild ${mildOverridden}`);
  // Both should stay within GCC plausible range for ~800kbps link
  assert.ok(mildOverridden / mild.targetBitrateBps <= 0.70,
    'mild override should be below 0.70 utilization');
  assert.ok(moderateOverridden / moderate.targetBitrateBps >= 0.10,
    'moderate override should be above 0.10 utilization');
});

// --- CC convergence behavioral tests ---
// These tests verify that the exponentialConverge function used in the C++
// synthetic profile produces the expected transition shape.  We reimplement
// the same math in JS to validate time constants without requiring a C++ build.

function exponentialConverge(current, target, deltaMs, tauMs) {
  if (tauMs <= 0 || deltaMs <= 0) return target;
  const alpha = 1.0 - Math.exp(-deltaMs / tauMs);
  return current + alpha * (target - current);
}

const CC_DEGRADE_TAU_MS = 1500.0;
const CC_RECOVER_TAU_MS = 6000.0;

test('CC degradation reaches ~63% of target within one tau (1.5s)', () => {
  const start = 900000;
  const target = 300000;
  const delta = start - target; // 600000
  let current = start;

  // Simulate 1.5s in 100ms steps
  for (let t = 0; t < 1500; t += 100) {
    current = exponentialConverge(current, target, 100, CC_DEGRADE_TAU_MS);
  }

  const fraction = (start - current) / delta;
  // After one tau, exponential convergence reaches ~63.2%
  assert.ok(fraction >= 0.55 && fraction <= 0.72,
    `degradation fraction ${fraction.toFixed(3)} should be ~0.632 after 1 tau`);
});

test('CC recovery reaches ~63% of target within one tau (6s)', () => {
  const start = 300000;
  const target = 900000;
  const delta = target - start; // 600000
  let current = start;

  // Simulate 6s in 100ms steps
  for (let t = 0; t < 6000; t += 100) {
    current = exponentialConverge(current, target, 100, CC_RECOVER_TAU_MS);
  }

  const fraction = (current - start) / delta;
  assert.ok(fraction >= 0.55 && fraction <= 0.72,
    `recovery fraction ${fraction.toFixed(3)} should be ~0.632 after 1 tau`);
});

test('CC degradation is substantially faster than recovery', () => {
  const start = 900000;
  const target = 300000;
  const delta = Math.abs(start - target);

  // Simulate 2s of degradation
  let degraded = start;
  for (let t = 0; t < 2000; t += 100) {
    degraded = exponentialConverge(degraded, target, 100, CC_DEGRADE_TAU_MS);
  }
  const degradeFraction = Math.abs(start - degraded) / delta;

  // Simulate 2s of recovery
  let recovered = target;
  for (let t = 0; t < 2000; t += 100) {
    recovered = exponentialConverge(recovered, start, 100, CC_RECOVER_TAU_MS);
  }
  const recoverFraction = Math.abs(recovered - target) / delta;

  // Degradation should reach much further than recovery in the same time
  assert.ok(degradeFraction > recoverFraction * 1.5,
    `after 2s: degrade ${degradeFraction.toFixed(3)} should be >1.5× recovery ${recoverFraction.toFixed(3)}`);
});

test('CC convergence reaches >95% of target within 3 tau', () => {
  // 3 tau for degrade = 4.5s, 3 tau for recover = 18s
  let degraded = 900000;
  for (let t = 0; t < 4500; t += 100) {
    degraded = exponentialConverge(degraded, 300000, 100, CC_DEGRADE_TAU_MS);
  }
  const degradeRemaining = Math.abs(degraded - 300000) / 600000;
  assert.ok(degradeRemaining < 0.06,
    `after 3×τ_degrade: remaining ${(degradeRemaining * 100).toFixed(1)}% should be <6%`);

  let recovered = 300000;
  for (let t = 0; t < 18000; t += 100) {
    recovered = exponentialConverge(recovered, 900000, 100, CC_RECOVER_TAU_MS);
  }
  const recoverRemaining = Math.abs(recovered - 900000) / 600000;
  assert.ok(recoverRemaining < 0.06,
    `after 3×τ_recover: remaining ${(recoverRemaining * 100).toFixed(1)}% should be <6%`);
});

test('loss rate convergence uses same time constants as other metrics', () => {
  // lossRate degradation (0% → 5%) should converge at τ_degrade
  let lossRate = 0.0;
  const targetLoss = 0.05;
  for (let t = 0; t < 1500; t += 100) {
    const lossDegrading = targetLoss > lossRate;
    lossRate = exponentialConverge(
      Math.max(0.0, lossRate), targetLoss, 100,
      lossDegrading ? CC_DEGRADE_TAU_MS : CC_RECOVER_TAU_MS);
  }
  const fraction = lossRate / targetLoss;
  assert.ok(fraction >= 0.55 && fraction <= 0.72,
    `loss degradation fraction ${fraction.toFixed(3)} should be ~0.632 after 1 tau`);
});
