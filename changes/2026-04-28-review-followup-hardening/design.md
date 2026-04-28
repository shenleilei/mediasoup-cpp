# Bugfix Design

## Context

This follow-up addresses one confirmed runtime crash path, one report-consistency bug, and one missing verification gap left by the 2026-04-28 review-driven changes.

The fixes should stay narrow:

- no accepted behavior change outside the reviewed failure paths
- no new production-only hooks
- no broad refactor of plain-client or registry architecture

## Root Cause

### 1. `SourceWorker` thread has no runtime exception boundary

`common/ffmpeg` helpers now convert FFmpeg hard failures into C++ exceptions. `SourceWorker` handles decoder-open and encoder-init startup failures explicitly, but once the worker thread enters its loop there is no outer `try/catch`. Any later `ReadPacket`, decode, or encode failure can therefore unwind out of the thread entrypoint and terminate the process.

### 2. Plain-client renderer only partially adopted resolved verdicts

The renderer now uses fallback verdict derivation for the per-case result row, but the summary/failure-list path still assumes `result.verdict` always exists. That leaves one code path using stored verdicts and another using derived verdicts.

### 3. Registry sync lock change lacks proof

`RoomRegistry` intentionally narrowed command-mutex coverage from whole snapshot phases to per-command round trips, but the repo does not yet contain a focused regression test that exercises concurrent public operations against a sync-triggered refresh.

## Fix Strategy

### 1. Add a worker-thread exception boundary and runtime failure test hook

- Wrap the `SourceWorker` thread entrypoint in a top-level `try/catch`.
- On exception:
  - log the failure with track context
  - set `running_ = false`
  - return without rethrowing
- Add a `MEDIASOUP_TEST_HOOKS`-gated read-failure countdown in `common/ffmpeg::InputFormat`.
  - This allows a deterministic runtime failure after startup without changing production behavior.
- Add a regression test that injects a read failure after the worker loop starts and verifies the process stays alive and `stop()` returns cleanly.

### 2. Resolve verdicts once for the plain-client renderer

- Introduce a `casesWithEvaluation` structure in `render_cpp_client_case_report.mjs`, mirroring the browser renderer pattern.
- Derive summary counters and failed-case listing from the same resolved verdict helper used by per-case rows.
- Extend renderer tests with a case that has no stored verdict but derives to pass, and assert:
  - the row says pass
  - the summary counts pass/fail consistently
  - no failed-case section is emitted

### 3. Add focused Redis-backed registry concurrency coverage

- Add a direct `RoomRegistry` integration test that uses a real ephemeral Redis server.
- Keep the test narrow:
  - seed the registry with only self in cache
  - populate Redis with many remote node keys after startup
  - trigger `resolveRoom()` so it must execute `syncNodesSnapshot()`
  - concurrently call `updateLoad()`
- Assert the concurrent update completes faster than the full refresh path, which demonstrates that operations are not serialized behind the whole sync sequence.
- Compile this direct integration test into the existing review-fix integration binary together with the `RoomRegistry` sources it exercises.

## Risk Assessment

- The `SourceWorker` exception boundary is low risk because it only changes failure handling on exceptional paths.
- The `InputFormat` test hook is gated behind `MEDIASOUP_TEST_HOOKS`, so production behavior is unchanged.
- Renderer changes are low risk and confined to markdown generation.
- The registry test adds runtime to one integration target but does not change production code.

## Test Strategy

- Run focused `SourceWorker` failure tests.
- Run Node renderer tests for case-report consistency.
- Build and run the review-fix integration target containing the new direct Redis-backed registry test.

## Rollout Notes

- No migration is required.
- No spec change is needed beyond behavior already accepted by `specs/current/runtime-safety.md`.
