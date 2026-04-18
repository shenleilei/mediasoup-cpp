# Integration Test Harness Refactor Implementation

## Objective

This implementation document defines how the repository-wide integration-test refactor is executed without changing runtime behavior or test intent.

The focus of this change is to move repeated test infrastructure into shared helpers while keeping scenario assertions inside each suite.

## Refactor Scope

### In Scope

- shared SFU launch and teardown helpers
- shared unique test-name generation
- shared default RTP capability fixtures
- shared producer RTP parameter builders where the JSON shape is identical
- shared connect-plus-join helpers
- shared publisher/subscriber subscription setup helpers
- migration of clean or already-owned integration suites onto those helpers

### Out Of Scope

- runtime server behavior
- mediasoup worker behavior
- scenario-level test semantics
- changing dirty user-owned integration suites unless necessary and safe

## Execution Principles

1. Extract only repeated infrastructure first.
2. Keep suite-specific assertions local.
3. Prefer additive helper introduction before broad migration.
4. Build and run a representative test after each migration wave.
5. Avoid files with unrelated user changes unless the active refactor already owns them.

## Shared Helper Strategy

### Helper Layers

- `tests/TestProcessUtils.h`
  Purpose: low-level process liveness and termination primitives

- `tests/TestSfuHarness.h`
  Purpose: higher-level SFU launch options, startup wait, shutdown wait, unique IDs, default RTP fixtures, common producer RTP parameter builders, connect-plus-join flows, and subscription setup flows

- `tests/TestHttpUtils.h`
  Purpose: reusable HTTP request and response helpers for integration tests

- `tests/TestMediaUtils.h`
  Purpose: reusable media-test fixtures for audio-only loopback RTP and PlainTransport tests

### Why This Split

- low-level process control remains minimal and reusable
- higher-level test orchestration stays isolated from scenario files
- future migrations can adopt the harness incrementally

## Migration Waves

### Wave 1: Build-System Support

Status: completed

- ensure integration targets still build after helper introduction

### Wave 2: Single-Node WebSocket Suites

Status: completed

- `tests/test_integration.cpp`
- `tests/test_stability_integration.cpp`
- `tests/test_qos_integration.cpp`
- `tests/test_e2e_pubsub.cpp`

Changes:

- replaced inline SFU startup shell assembly with `SfuLaunchOptions`
- replaced inline port polling with shared wait helpers
- replaced inline unique room-name generation with `makeUniqueTestName()`
- replaced duplicated default RTP capabilities with a shared helper

### Wave 3: Additional Owned Integration Suites

Status: completed

- `tests/test_thread_integration.cpp`

Changes:

- migrated threaded integration startup and teardown to the shared harness
- reused shared default RTP capabilities

### Wave 4: Multi-Node / HTTP Utility Migration

Status: in_progress

Candidate files:

- `tests/test_multinode_integration.cpp`
- `tests/test_multi_sfu_topology.cpp`
- `tests/test_review_fixes_integration.cpp`

Current handling:

- `tests/test_multinode_integration.cpp` now reuses shared SFU launch, wait, and HTTP helpers in selected paths
- `tests/test_multi_sfu_topology.cpp` now reuses shared SFU launch and wait helpers in selected paths
- `tests/test_review_fixes_integration.cpp` now reuses shared launch, wait, RTP, and HTTP helpers in selected top-level and sub-fixture paths, but still carries broader local divergence

## File Ownership For This Change

### Owned By This Refactor

- `tests/TestSfuHarness.h`
- `tests/TestHttpUtils.h`
- `tests/TestMediaUtils.h`
- `tests/test_integration.cpp`
- `tests/test_stability_integration.cpp`
- `tests/test_qos_integration.cpp`
- `tests/test_e2e_pubsub.cpp`
- `tests/test_thread_integration.cpp`
- `tests/test_qos_accuracy.cpp`
- `tests/test_qos_recording_accuracy.cpp`
- change docs in this folder

Partially migrated under careful narrow edits:

- `tests/test_multinode_integration.cpp`
- `tests/test_multi_sfu_topology.cpp`
- `tests/test_review_fixes_integration.cpp`

### Not Owned In This Wave

- none; some files remain only partially migrated

## Verification Plan

### Build Verification

Run:

- `cmake --build /root/mediasoup-cpp/build-refactor-check --target mediasoup_integration_tests mediasoup_stability_integration_tests mediasoup_e2e_tests mediasoup_qos_integration_tests mediasoup_thread_integration_tests mediasoup_multinode_tests mediasoup_topology_tests mediasoup_qos_accuracy_tests mediasoup_qos_recording_accuracy_tests -j1`

### Runtime Verification

Run:

- `/root/mediasoup-cpp/build-refactor-check/mediasoup_integration_tests --gtest_filter=IntegrationTest.JoinAndLeave`
- `/root/mediasoup-cpp/build-refactor-check/mediasoup_stability_integration_tests --gtest_filter=StabilityIntegrationTest.CloseAfterJoinNoCrash`
- `/root/mediasoup-cpp/build-refactor-check/mediasoup_e2e_tests --gtest_filter=E2EPubSubTest.FullConferenceLifecycle`
- `/root/mediasoup-cpp/build-refactor-check/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.GetStatsAfterProduce`
- `/root/mediasoup-cpp/build-refactor-check/mediasoup_thread_integration_tests --gtest_filter=ThreadedPlainPublishIntegrationTest.ExplicitThreadedSourcesDoNotRequireBootstrapMp4`

Important execution rule:

- suites with fixed ports such as `14000`, `14011`, or `15050` should be verified sequentially, not in parallel, unless their port allocation is first refactored

### Acceptance Criteria For Each Wave

- target still builds
- migrated suite still starts and stops SFU cleanly
- migrated scenario still passes unchanged
- no user-visible target names change

## Rollback Plan

If a migration wave regresses:

1. revert the migrated suite to its previous inline setup
2. keep the shared helper additive until another suite consumes it safely
3. preserve successful migrations in unrelated suites

## Current Progress Summary

Completed:

- shared SFU harness introduced
- shared default RTP capabilities introduced
- shared standard audio/video producer RTP parameter builders introduced
- shared HTTP test helpers introduced
- shared WebSocket signaling helpers introduced
- shared connect-plus-join helpers introduced
- shared publisher/subscriber subscription setup helpers introduced
- shared media test fixtures introduced
- representative single-node and threaded integration suites migrated
- clean RTP accuracy suites migrated to shared media fixtures
- selected dirty multi-node suites migrated through narrow shared-helper substitutions
- selected dirty review-fix integration suites migrated through narrow shared-helper substitutions

Next safe work:

- continue replacing remaining file-local wrappers with direct shared-helper usage
- continue migrating remaining fixture-local helpers in `tests/test_review_fixes_integration.cpp`
