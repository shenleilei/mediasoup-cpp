# Bugfix Analysis

## Summary

Review follow-up found three remaining correctness/verification gaps in the current worktree:

- `SourceWorker` now relies on exception-throwing FFmpeg RAII helpers but its worker thread has no top-level exception boundary, so runtime FFmpeg failures can terminate the whole process.
- `render_cpp_client_case_report.mjs` derives fallback verdicts for per-case rows but not for summary/failure-list generation, so reports can contradict themselves when stored verdicts are absent.
- `RoomRegistry` sync lock-scope changes were implemented, but the repo still lacks focused Redis-backed verification that concurrent registry operations stay responsive during sync-driven refresh.

## Reproduction

1. Inspect `client/SourceWorker.h` and note that the worker thread directly calls `loopFile()` / `loopCamera()` while `common/ffmpeg::{InputFormat,Decoder,Encoder}` now throw on runtime FFmpeg errors.
2. Render a plain-client case report from a matrix JSON where a case has no stored `verdict` field but still derives to pass.
3. Compare the renderer’s per-case result row with the generated “失败 / 错误 Case” section.
4. Inspect `changes/2026-04-28-runtime-review-priority-fixes/tasks.md` and note that no dedicated Redis-backed integration test was run for the `RoomRegistry` lock-scope refactor.

## Observed Behavior

- A runtime `av_read_frame` / decode / encode failure in `SourceWorker` can escape the worker thread and trigger `std::terminate`.
- Plain-client case reports can show a passing case row while still listing that case under failures.
- The Redis lock-scope change is not covered by a focused integration test that exercises concurrent registry work during sync-triggered refresh.

## Expected Behavior

- `SourceWorker` SHALL catch runtime FFmpeg exceptions at the thread boundary, log them, and stop the worker safely.
- Plain-client case-report summary, failure list, and per-case rows SHALL use one shared resolved-verdict semantic.
- The repository SHALL include Redis-backed regression coverage proving that concurrent registry operations are not blocked for the whole sync refresh window.

## Known Scope

- `client/SourceWorker.h`
- `common/ffmpeg/InputFormat.h`
- `common/ffmpeg/InputFormat.cpp`
- `tests/test_source_worker_failure.cpp`
- `tests/qos_harness/render_cpp_client_case_report.mjs`
- `tests/qos_harness/test.case_report_renderers.mjs`
- `tests/test_room_registry_sync.cpp`
- `CMakeLists.txt`

## Must Not Regress

- Existing `SourceWorker` startup failure handling and happy-path encode behavior.
- Existing plain-client full/targeted report artifact locations and stored-verdict behavior.
- Existing Redis-backed room resolution and cache propagation behavior.

## Acceptance Criteria

### Requirement 1

`SourceWorker` SHALL fail safely on runtime FFmpeg exceptions after startup.

#### Scenario: Read failure during active file worker loop

- WHEN a test-only FFmpeg hook injects a runtime read failure after the worker thread has started
- THEN the worker thread exits without crashing the test process
- AND `stop()` returns normally

### Requirement 2

Plain-client report generation SHALL resolve verdicts consistently across all sections.

#### Scenario: Derived-pass case without stored verdict

- WHEN a case result omits `verdict`
- THEN the per-case result row, summary counters, and failed-case list all use the same derived verdict
- AND a derived passing case is not listed as failed

### Requirement 3

Redis-backed registry sync changes SHALL have focused concurrency regression coverage.

#### Scenario: Concurrent registry update during sync-triggered node refresh

- WHEN one thread triggers `resolveRoom()` on a registry whose cache must refresh many Redis node keys
- AND another thread performs `updateLoad()` concurrently
- THEN `updateLoad()` completes without waiting for the entire sync-triggered refresh sequence to finish

## Regression Expectations

- Required automated coverage: targeted `SourceWorker` failure-path tests, case-report renderer tests, and Redis-backed registry integration coverage.
- Required build coverage: the review-fix integration target continues to compile with the new direct `RoomRegistry` test.
