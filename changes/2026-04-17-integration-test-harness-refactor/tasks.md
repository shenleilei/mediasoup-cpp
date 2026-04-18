# Tasks

## 1. Planning

- [x] 1.1 Define the integration-test harness refactor scope and non-goals
  Files: `changes/2026-04-17-integration-test-harness-refactor/*`
  Verification: requirements and design preserve scenario behavior

## 2. Shared Helpers

- [x] 2.1 Add shared helpers for SFU launch options, startup wait, shutdown wait, and unique test names
  Files: `tests/*`
  Verification: helpers compile and support both single-node and Redis-backed multi-node flows

- [x] 2.2 Add a shared default RTP capability helper for the standard Opus + VP8 fixture
  Files: `tests/*`
  Verification: migrated suites can use the helper without changing join behavior

- [x] 2.3 Add shared producer RTP parameter builders for standard Opus and VP8 fixtures
  Files: `tests/*`
  Verification: migrated suites can produce media through shared JSON helpers without changing behavior

- [x] 2.4 Add shared HTTP test helpers for raw-response and body extraction
  Files: `tests/*`
  Verification: migrated suites can issue HTTP GET requests through shared helpers without behavior changes

- [x] 2.5 Add shared WebSocket signaling helpers for standard join, transport creation, and produce requests
  Files: `tests/*`
  Verification: migrated suites can perform standard signaling flows through shared helpers without behavior changes

- [x] 2.6 Add shared connect-plus-join helpers for standard integration-test peer setup
  Files: `tests/*`
  Verification: migrated suites can create joined test peers through shared helpers without changing behavior

- [x] 2.7 Add shared publisher/subscriber subscription setup helpers for standard audio/video flows
  Files: `tests/*`
  Verification: migrated suites can establish auto-subscribed consumer state through shared helpers without changing behavior

- [x] 2.8 Add shared media test helpers for audio-opus RTP/PlainTransport fixtures
  Files: `tests/*`
  Verification: clean RTP accuracy suites can reuse shared codec, socket, and RTP parameter builders without changing behavior

## 3. Suite Migration

- [x] 3.1 Refactor representative single-node integration suites to use the shared harness
  Files: `tests/test_integration.cpp`, `tests/test_stability_integration.cpp`, `tests/test_qos_integration.cpp`, `tests/test_e2e_pubsub.cpp`, `tests/test_thread_integration.cpp`
  Verification: targets build successfully after migration

- [x] 3.2 Refactor representative multi-node startup paths to use the shared harness
  Files: `tests/test_integration.cpp`, `tests/test_multinode_integration.cpp`, `tests/test_multi_sfu_topology.cpp`
  Verification: the migrated multi-node fixtures build successfully with shared launch and HTTP helpers

## 4. Delivery Gates

- [x] 4.1 Build the migrated integration test targets
  Verification: `cmake --build ... --target mediasoup_integration_tests mediasoup_stability_integration_tests mediasoup_e2e_tests mediasoup_qos_integration_tests mediasoup_thread_integration_tests mediasoup_multinode_tests mediasoup_topology_tests mediasoup_qos_accuracy_tests mediasoup_qos_recording_accuracy_tests`

- [x] 4.2 Run representative integration binaries
  Verification: `mediasoup_integration_tests --gtest_filter=IntegrationTest.JoinAndLeave`, `mediasoup_stability_integration_tests --gtest_filter=StabilityIntegrationTest.CloseAfterJoinNoCrash`, `mediasoup_e2e_tests --gtest_filter=E2EPubSubTest.FullConferenceLifecycle`, `mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.GetStatsAfterProduce`, `mediasoup_thread_integration_tests --gtest_filter=ThreadedPlainPublishIntegrationTest.ExplicitThreadedSourcesDoNotRequireBootstrapMp4`

## 5. Review

- [x] 5.1 Self-review the change for behavior preservation and duplication removal
  Verification: migrated suites retain ports, worker counts, and Redis settings
