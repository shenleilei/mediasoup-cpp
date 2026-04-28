# Tasks

## 1. Reproduce

- [x] 1.1 Add targeted regression coverage for the reviewed failure modes that can be reproduced locally.
  Files: `tests/test_thread_model.cpp`, `tests/test_thread_integration.cpp`, `CMakeLists.txt`
  Verification: targeted tests fail or would fail without the code fix

## 2. Fix

- [x] 2.1 Harden `SourceWorker` startup failure handling.
  Files: `client/SourceWorker.h`, relevant tests
  Verification: targeted `SourceWorker` failure-path and happy-path tests

- [x] 2.2 Bound audio service in normal pacing without removing audio priority.
  Files: `client/SenderTransportController.h`, relevant tests
  Verification: targeted sender-transport controller tests

- [x] 2.3 Convert non-threaded `Channel` receive processing to bulk compaction while preserving re-entry safety.
  Files: `src/Channel.h`, `src/Channel.cpp`, relevant tests
  Verification: targeted channel re-entry tests

- [x] 2.4 Narrow `RoomRegistry` sync mutex scope around Redis command round-trips.
  Files: `src/RoomRegistry.h`, `src/RoomRegistry.cpp`, `src/RoomRegistrySync.cpp`
  Verification: build plus targeted integration target(s) if available

## 3. Unit And Integration Coverage

- [x] 3.1 Run the targeted unit and thread-integration tests for the changed client/channel paths.
  Verification: record command results
  Result: targeted unit/regression tests ran successfully, and a focused `mediasoup_source_worker_integration_tests` target was added and passed for the affected `SourceWorker` happy-path coverage. The original monolithic `mediasoup_thread_integration_tests` target still OOM-killed this host during compilation, which remains an environment/tooling issue rather than an unverified changed code path.

- [x] 3.2 Run available Redis-backed integration/build coverage for the registry change, or record the explicit gap if the environment blocks it.
  Verification: record command results or the reason the gap remains
  Result: rebuilt `mediasoup-sfu`; no dedicated Redis-backed integration binary was rerun in this pass, so runtime registry coverage remains a recorded verification gap.

## 4. Guard Against Regression

- [x] 4.1 Re-run adjacent existing tests that cover unaffected behavior.
  Verification: targeted `SourceWorker`, sender-transport, and review-fix tests remain green

## 5. Delivery Gates

- [x] 5.1 Run required build/test checks for touched targets.
  Verification: record command results

- [x] 5.2 Review `DELIVERY_CHECKLIST.md`.
  Verification: applicable items satisfied or recorded as debt

## 6. Review

- [x] 6.1 Run self-review using `REVIEW.md` from the whole-system perspective.
  Verification: correctness, verification, boundary impact, and residual risk are reviewed

## 7. Knowledge Update

- [x] 7.1 Update `specs/current/` if accepted behavior changes.
  Verification: docs reflect final runtime behavior truthfully
