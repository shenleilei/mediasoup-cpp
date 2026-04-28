# Bugfix Design

## Context

These are review-driven correctness and buildability follow-ups on top of the active client/runtime changes. They are small, localized fixes and should not alter accepted runtime behavior.

## Root Cause

### 1. Standalone `client/` configure

`client/CMakeLists.txt` now adds the vendored spdlog subdirectory from outside the client source tree, but plain `add_subdirectory(<external>)` requires an explicit binary directory for out-of-tree sources.

### 2. `run_all_tests.sh` coverage drift

New `SourceWorker` regression binaries were added to the root CMake build but not to the full-regression build target list, so default verification does not compile them.

### 3. Browser report prose drift

`render_case_report.mjs` resolves the final verdict one way for summary/result lines and another way for `重点分析`, leaving one path capable of contradicting the stored verdict.

## Fix Strategy

### 1. Fix client-only spdlog inclusion

- Update `client/CMakeLists.txt` to provide an explicit binary dir when adding the repository vendored spdlog directory.

### 2. Extend full-regression build coverage

- Add the two `SourceWorker` regression targets to the `run_all_tests.sh` build target list.
- Keep runtime group execution unchanged; this fix is about compile coverage only.

### 3. Reuse resolved verdict in browser narrative

- Add a small helper in `render_case_report.mjs` to resolve verdict semantics once.
- Reuse that helper in `实际结果`, summary, failed-case list, and `重点分析`.
- Extend the existing renderer test to assert the narrative reflects a forced stored failure.

## Risk Assessment

- The client configure fix is low risk and isolated to CMake generation.
- Adding targets to `run_all_tests.sh` increases build time slightly but improves coverage.
- Renderer fix is low risk and only affects generated markdown consistency.

## Test Strategy

- Run a fresh standalone `client/` configure in a temp directory.
- Run a fresh root configure/build of `mediasoup_source_worker_failure_tests` in a temp directory as a smoke check.
- Run `node --test tests/qos_harness/test.case_report_renderers.mjs`.

## Observability

- No runtime logging changes are needed.
- Successful configure/build commands and renderer test output are the evidence.

## Rollout Notes

- No migration is required.
- No generated reports need to be refreshed unless a user specifically wants regenerated markdown after the renderer fix.
