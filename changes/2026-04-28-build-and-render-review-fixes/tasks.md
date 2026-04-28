# Tasks

## 1. Reproduce

- [x] 1.1 Confirm standalone `client/` configure failure and missing `run_all_tests.sh` target coverage.
  Verification: targeted commands demonstrate the current defects

## 2. Fix

- [x] 2.1 Fix standalone `client/` spdlog subdirectory inclusion.
  Files: `client/CMakeLists.txt`
  Verification: clean `cmake -S client -B <dir>` succeeds

- [x] 2.2 Add the new SourceWorker regression targets to `run_all_tests.sh`.
  Files: `scripts/run_all_tests.sh`
  Verification: target list includes both binaries

- [x] 2.3 Make browser report `重点分析` use the same resolved verdict semantics as summary/result fields.
  Files: `tests/qos_harness/render_case_report.mjs`, `tests/qos_harness/test.case_report_renderers.mjs`
  Verification: Node renderer tests pass

## 3. Delivery Gates

- [x] 3.1 Run targeted configure/build and renderer verification.
  Verification: record command results
  Result: clean `cmake -S client -B /tmp/plain-client-cmake-review3` succeeded; clean root configure/build of `mediasoup_source_worker_failure_tests` succeeded; `node --test tests/qos_harness/test.case_report_renderers.mjs` passed.

- [x] 3.2 Review `DELIVERY_CHECKLIST.md` and record residual gaps if any.
  Verification: applicable items satisfied or noted
  Result: no additional runtime-contract change was introduced; this bugfix is limited to buildability, default test coverage, and generated markdown consistency.
