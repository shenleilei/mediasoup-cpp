import assert from 'node:assert/strict';
import test from 'node:test';

import {
  analyzeResult,
  getPhaseNetwork,
  toSyntheticCondition,
} from './synthetic_sweep_shared.mjs';

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
