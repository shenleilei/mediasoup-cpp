# Tasks

## 1. Helper Extraction

- [x] 1.1 Extract browser matrix baseline contamination helpers into a dedicated module.
  Files: `tests/qos_harness/matrix_runner_helpers.mjs`, `tests/qos_harness/run_matrix.mjs`
  Verification: targeted Node tests

## 2. Behavior Fix

- [x] 2.1 Make baseline-group cases bypass the contamination heuristic.
  Files: `tests/qos_harness/matrix_runner_helpers.mjs`
  Verification: targeted Node tests

## 3. Regression Coverage

- [x] 3.1 Add a Node regression test for `B3` and non-baseline contamination behavior.
  Files: `tests/qos_harness/test.matrix_runner_helpers.mjs`
  Verification: targeted Node tests
  Result: `node --test tests/qos_harness/test.matrix_runner_helpers.mjs` passed.
