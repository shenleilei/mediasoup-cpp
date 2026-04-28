# Tasks

## 1. Runtime Hardening

- [x] 1.1 Add a thread-level exception boundary for `SourceWorker`.
  Files: `client/SourceWorker.h`
  Verification: targeted `SourceWorker` regression tests

- [x] 1.2 Add a test-only runtime read-failure hook for `InputFormat` and cover the post-start failure path.
  Files: `common/ffmpeg/InputFormat.h`, `common/ffmpeg/InputFormat.cpp`, `tests/test_source_worker_failure.cpp`
  Verification: targeted `SourceWorker` failure-path tests

## 2. Report Consistency

- [x] 2.1 Unify plain-client renderer summary/failure-list verdict resolution with per-case rows.
  Files: `tests/qos_harness/render_cpp_client_case_report.mjs`
  Verification: renderer tests

- [x] 2.2 Add regression coverage for derived-pass cases without stored verdicts.
  Files: `tests/qos_harness/test.case_report_renderers.mjs`
  Verification: renderer tests

## 3. Registry Verification

- [x] 3.1 Add focused Redis-backed concurrency coverage for sync-triggered registry refresh.
  Files: `tests/test_room_registry_sync.cpp`, `CMakeLists.txt`
  Verification: review-fix integration target build and run

## 4. Verification

- [x] 4.1 Run targeted `SourceWorker` failure tests.
  Result: `./build/mediasoup_source_worker_failure_tests` passed, including the injected runtime read-failure path.

- [x] 4.2 Run renderer tests.
  Result: `node --test tests/qos_harness/test.case_report_renderers.mjs` passed.

- [x] 4.3 Run the review-fix integration target with the new RoomRegistry coverage.
  Result: `./build/mediasoup_review_fix_tests --gtest_filter=RoomRegistrySyncIntegration.*` passed, and `CacheTest.ResolveUsesCache` also passed as an adjacent Redis-backed regression check.
