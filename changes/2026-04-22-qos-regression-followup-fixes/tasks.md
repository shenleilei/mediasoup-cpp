# Tasks

## 1. Reproduce And Classify

- [x] 1.1 Reconfirm the downlink seq-reset failure against current binaries and source semantics
  Files: `tests/test_qos_integration.cpp`, `src/qos/DownlinkQosRegistry.cpp`
  Verification: targeted `mediasoup_qos_integration_tests` filter

- [x] 1.2 Reconfirm the cpp harness failures in isolated runs and record whether they are deterministic or timing-sensitive
  Files: `tests/qos_harness/run_cpp_client_harness.mjs`
  Verification: targeted harness runs for `multi_video_budget`, `threaded_multi_track_snapshot`, `threaded_quick`

- [ ] 1.3 Reconfirm `BW3`, `T9`, and `S4` in isolated matrix runs and compare with current accepted docs
  Files: `tests/qos_harness/run_matrix.mjs`, `docs/uplink-qos-case-results.md`, `docs/qos/uplink/blind-spot-scenario.md`
  Verification: `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`

## 2. Fix Downlink Integration Coverage

- [x] 2.1 Correct `DownlinkClientStatsAcceptsSeqResetFromHighValue` so it validates one coherent accepted contract
  Files: `tests/test_qos_integration.cpp`
  Verification: targeted downlink gtests

- [x] 2.2 Recheck neighboring `stale-seq` and `stale-ts` downlink cases after the correction
  Files: `tests/test_qos_integration.cpp`
  Verification: same targeted gtest filter

## 3. Fix Cpp Harness Timing

- [x] 3.1 Replace `multi_video_budget` single-tail-sample assertion with bounded window-based observation
  Files: `tests/qos_harness/run_cpp_client_harness.mjs`
  Verification: `node tests/qos_harness/run_cpp_client_harness.mjs multi_video_budget`

- [x] 3.2 Require full expected track-set convergence in `threaded_multi_track_snapshot`
  Files: `tests/qos_harness/run_cpp_client_harness.mjs`
  Verification: `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_track_snapshot`

- [ ] 3.3 Re-run the aggregate threaded smoke entrypoint
  Files: `tests/qos_harness/run_cpp_client_harness.mjs`
  Verification: `node tests/qos_harness/run_cpp_client_harness.mjs threaded_quick`

## 4. Repair Matrix Behavior

- [x] 4.1 Isolate whether `BW3` / `S4` / `T9` share a runner-level cause or require separate fixes
  Files: relevant runner/controller files discovered during repro
  Verification: targeted matrix reruns with diagnostics comparison

- [x] 4.2 Implement the smallest behavior fix that restores `BW3`, `T9`, and `S4` without broadening expectations first
  Files: to be narrowed after 4.1
  Verification: `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`

- [x] 4.3 Run adjacent targeted matrix coverage required by the chosen fix direction
  Files: affected runner/controller files and any updated docs
  Verification: targeted neighboring matrix cases

## 5. Review And Knowledge Update

- [ ] 5.1 Update affected docs/specs if accepted behavior or test contract changes
  Files: affected `docs/` and `specs/current/`
  Verification: docs match final behavior and test semantics

- [ ] 5.2 Run diff hygiene and delivery checklist review
  Files: whole change
  Verification: `git diff --check` and `docs/aicoding/DELIVERY_CHECKLIST.md`
