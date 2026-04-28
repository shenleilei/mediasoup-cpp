# Bugfix Analysis

## Summary

Current review follow-up found three concrete defects in the active worktree:

- standalone `client/` configure is broken after linking `spdlog::spdlog`
- `run_all_tests.sh` does not build the newly added `SourceWorker` regression targets
- browser uplink case-report prose can still disagree with the stored final verdict

## Reproduction

1. Run `cmake -S client -B /tmp/plain-client-review`.
2. Observe `add_subdirectory(third_party/spdlog)` failing because the source dir is out of tree and no binary dir is provided.
3. Inspect `scripts/run_all_tests.sh` build target list and compare it with `CMakeLists.txt`.
4. Render a browser uplink case report from a JSON fixture whose stored verdict is forced to fail while local re-derivation still passes.

## Observed Behavior

- standalone plain-client configure fails before generation completes.
- `mediasoup_source_worker_failure_tests` and `mediasoup_source_worker_integration_tests` are not part of the default full-regression build entrypoint.
- `render_case_report.mjs` uses the stored verdict for summary/result lines but still derives `重点分析` from local re-evaluation, which can contradict the rendered verdict.

## Expected Behavior

- standalone `client/` configure SHALL succeed.
- the default full-regression build entrypoint SHALL compile the newly introduced `SourceWorker` regression targets.
- browser case-report narrative SHALL use the same resolved verdict semantics as the summary/result rows.

## Known Scope

- `client/CMakeLists.txt`
- `scripts/run_all_tests.sh`
- `tests/qos_harness/render_case_report.mjs`
- `tests/qos_harness/test.case_report_renderers.mjs`

## Must Not Regress

- root build path for `mediasoup-sfu` and test binaries
- plain-client linking against vendored spdlog
- existing renderer behavior when stored and derived verdict already agree

## Suspected Root Cause

- the client subproject now references an out-of-tree spdlog source directory without the required explicit binary dir.
- new test binaries were added in `CMakeLists.txt` but not wired into the repository-wide build script.
- renderer verdict resolution was updated only for summary/result fields, not for the prose helper.

## Acceptance Criteria

### Requirement 1

The standalone plain-client configure SHALL succeed again.

#### Scenario: Fresh client-only configure

- WHEN `cmake -S client -B <dir>` is executed in a clean temp directory
- THEN configuration completes without a `spdlog::spdlog` / `add_subdirectory` error

### Requirement 2

The repository-wide full-regression build entry SHALL include the new SourceWorker regression targets.

#### Scenario: Full regression build list

- WHEN `scripts/run_all_tests.sh` builds its target set
- THEN both `mediasoup_source_worker_failure_tests` and `mediasoup_source_worker_integration_tests` are included

### Requirement 3

Browser uplink report prose SHALL honor the stored resolved verdict.

#### Scenario: Forced-fail rendered case

- WHEN a case JSON stores `verdict.passed=false`
- THEN the report summary, result row, and `重点分析` text all reflect failure consistently

## Regression Expectations

- Existing unaffected behavior: root build path, plain-client runtime logic, report artifact path helpers.
- Required automated regression coverage: renderer unit tests plus clean configure/build smoke checks.
- Required manual smoke checks: none beyond targeted configure/build commands.
