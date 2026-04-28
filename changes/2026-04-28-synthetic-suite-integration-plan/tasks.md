# Tasks

## 1. Scope And Boundary

- [x] 1.1 Add or update synthetic-only scope documentation.
  Outcome: synthetic suite is no longer confused with physical-E2E validation
  Files: synthetic analysis / methodology docs, runner/report labels as needed
  Result: `docs/uplink-qos-synthetic-value-analysis_cn.md` now documents the adopted synthetic-only scope, retained legacy forcing, and interpretation boundary.

## 2. Core Model Recalibration

- [x] 2.1 Port JS synthetic model recalibration onto current mainline.
  Outcome: calibrated loss/RTT/jitter/utilization model on current branch
  Files: `tests/qos_harness/synthetic_sweep_shared.mjs`
  Result: ported loss calibration, RTT logarithmic amplification, jitter smoothing, and updated utilization caps.

## 3. C++ Synthetic Convergence

- [x] 3.1 Port C++ convergence support onto current mainline without regressing current hook safety.
  Outcome: synthetic profile uses converged sendCeiling/RTT/jitter/lossRate
  Files: `client/PlainClientSupport.h`, `client/PlainClientSupport.cpp`
  Result: added `exponentialConverge` plus convergence state for ceiling/RTT/jitter/loss while preserving current `loadTestHookEnv()` safety model.

## 4. Validation

- [x] 4.1 Port JS synthetic validation tests.
  Files: `tests/qos_harness/test.synthetic_sweep.mjs`
  Result: added calibration, retained-override, and convergence tests.

- [x] 4.2 Port C++ convergence / pipeline / controller tests.
  Files: `tests/test_client_qos.cpp`
  Result: added `ExponentialConverge.*`, `SyntheticPipeline.*`, and `SyntheticRunnerPipeline.*`.

## 5. Guardrails

- [x] 5.1 Ensure `QOS_TEST_*` access remains gated by current mainline `MEDIASOUP_TEST_HOOKS` rules.
  Files: `client/PlainClientSupport.*`, `client/PlainClientApp.cpp`, related build files
  Result: retained current mainline hook behavior via `TestHooks.h` / `loadTestHookEnv()` instead of adopting the remote branch’s alternative build contract.

- [x] 5.2 Do not reintroduce old stdout-only trace parsing, old netem guard behavior, or old report-verdict behavior.
  Verification: compare against current mainline fixed files before landing
  Result: no changes were made to `cpp_client_runner`, `netem_guard`, or report renderer paths during this absorption pass.

## 6. Legacy Override Status

- [x] 6.1 Preserve retained legacy overrides only with explicit documentation and tests.
  Outcome: reviewers know these are temporary synthetic forcing values, not physical truth
  Result: retained overrides are documented in `docs/uplink-qos-synthetic-value-analysis_cn.md` and covered by JS/C++ synthetic tests.

## 7. Landing Plan

- [ ] 7.1 Split into small commits on top of current branch rather than merging the remote branch directly.
  Suggested commits:
  1. synthetic scope docs
  2. JS synthetic recalibration
  3. C++ convergence implementation
  4. JS/C++ synthetic tests

## Verification Notes

- [x] `node --test tests/qos_harness/test.synthetic_sweep.mjs`
- [x] `./build/mediasoup_qos_unit_tests --gtest_filter=ExponentialConverge.*:SyntheticPipeline.*:SyntheticRunnerPipeline.*`
