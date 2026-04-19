# Tasks

## 1. Define The Bugfix

- [x] 1.1 Record the flaky QoS integration failures and the lifecycle root cause
  Files: `changes/2026-04-19-qos-integration-port-isolation/bugfix.md`
  Verification: bugfix doc identifies the failing cases and the fixed-port/process-lifecycle issue

- [x] 1.2 Describe the isolation-based fix strategy
  Files: `changes/2026-04-19-qos-integration-port-isolation/design.md`
  Verification: design covers dynamic ports, direct child ownership, and release confirmation

## 2. Implement Shared Helpers

- [x] 2.1 Add reusable helpers for test-port allocation, SFU startup, readiness, and teardown
  Files: `tests/TestProcessUtils.h`
  Verification: helper-based targeted tests can spawn and clean up SFU deterministically

## 3. Migrate QoS Integration Fixtures

- [x] 3.1 Update `QosIntegrationTest` to use dynamic ports and shared lifecycle helpers
  Files: `tests/test_qos_integration.cpp`
  Verification: repeated targeted reruns pass

- [x] 3.2 Update `QosRecordingTest` to use the same lifecycle helpers
  Files: `tests/test_qos_integration.cpp`
  Verification: full QoS group passes

## 4. Verify

- [x] 4.1 Run repeated targeted regression for the previously flaky cases
  Verification:
  - `./build/mediasoup_qos_integration_tests --gtest_filter='QosIntegrationTest.GetStatsForOtherPeer:QosIntegrationTest.StatsReportIncludesDownlinkConsumerState:QosIntegrationTest.ClientStatsQosStoredAndAggregated' --gtest_repeat=20`

- [x] 4.2 Run the full QoS C++ regression group
  Verification:
  - `./scripts/run_all_tests.sh --skip-build qos`

## 5. Review

- [x] 5.1 Self-review against `REVIEW.md` and `DELIVERY_CHECKLIST.md`
  Verification: lifecycle correctness, cleanup guarantees, docs, and verification reviewed together
