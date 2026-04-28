import assert from 'node:assert/strict';
import test from 'node:test';

import { detectBaselineContamination } from './matrix_runner_helpers.mjs';

test('baseline-group B3 is not treated as contamination', () => {
  const b3 = {
    caseId: 'B3',
    group: 'baseline',
    bandwidth: 2000,
    rtt: 55,
    loss: 0.5,
    jitter: 12,
  };

  const contamination = detectBaselineContamination(b3, {
    state: 'recovering',
    level: 3,
  });

  assert.equal(contamination, null);
});

test('non-baseline mild baseline still trips contamination protection', () => {
  const caseDef = {
    caseId: 'L1',
    group: 'loss_sweep',
    bandwidth: 4000,
    rtt: 25,
    loss: 0.5,
    jitter: 5,
  };

  const contamination = detectBaselineContamination(caseDef, {
    state: 'recovering',
    level: 3,
  });

  assert.equal(
    contamination,
    'baseline entered recovering before any impairment'
  );
});
