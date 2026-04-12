import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

import {
  deriveCaseEvaluation,
  extractTiming,
  analyzeResult,
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
  assert.equal(condition.rttMs, 110);
  assert.ok(condition.bitrateBps < condition.targetBitrateBps);
  assert.ok(condition.bitrateBps <= 765000);
});

test('B3 expectation stays aligned with degraded weak-baseline behavior', () => {
  const b3 = scenarios.find(caseDefn => caseDefn.caseId === 'B3');

  assert.deepEqual(b3?.expect?.states, ['early_warning', 'congested']);
  assert.equal(b3?.expect?.minLevel, 1);
  assert.equal(b3?.expect?.maxLevel, 4);
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

test('extractTiming stays inside the requested phase window', () => {
  const trace = [
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

  assert.equal(timing.t_detect_congested, 200);
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
