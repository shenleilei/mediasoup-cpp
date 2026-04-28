# Bugfix Plan: QoS Regression Stabilization

## Symptom
The repository-level QoS regression entry still fails after the interop gates are green because two pre-existing QoS tests are unstable or incorrect.

## Reproduction
1. Run `scripts/run_all_tests.sh --skip-build qos`
2. Observe failures in:
   - `QosIntegrationTest.DownlinkClientStatsRateLimited`
   - `tests/test_qos_accuracy.cpp` (compile or assertion instability)

## Observed Behavior
- The downlink rate-limit integration test assumes a deterministic outcome that does not match the current worker-completed signaling semantics.
- The QoS accuracy test file contains a syntax defect and a localhost-sensitive assertion that treats RTCP fraction-lost formatting too strictly.

## Expected Behavior
- The QoS regression entry should fail only for real behavior regressions, not for stale or flaky test assumptions.
- The affected tests should reflect the documented semantics in `specs/current/`.

## Suspected Scope
- `tests/test_qos_integration.cpp`
- `tests/test_qos_accuracy.cpp`
- Possibly small supporting signaling behavior only if the documented semantics require it

## Known Non-affected Behavior
- The newly added video interop black-box gates already pass and are not the source of these failures.

## Acceptance Criteria
- `tests/test_qos_accuracy.cpp` compiles cleanly and uses stable assertions for localhost RTCP metrics.
- `QosIntegrationTest.DownlinkClientStatsRateLimited` reflects the documented rate-limit / stored-result semantics and passes reliably.
- `scripts/run_qos_tests.sh browser-harness:public-interop` remains green.
- The affected QoS suites pass under targeted reruns.

## Regression Expectations
- No weakening of real interoperability coverage.
- No contradiction with `specs/current/qos-signaling.md` or `specs/current/downlink-qos.md`.
