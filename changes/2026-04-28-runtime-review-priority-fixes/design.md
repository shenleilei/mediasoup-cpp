# Bugfix Design

## Context

This change bundles the currently confirmed high-priority runtime review follow-ups. The fixes cross the plain-client media path, the non-threaded worker channel path, and Redis-backed room registry sync. The implementation must stay narrow and preserve current external behavior except where the reviewed defect explicitly requires a runtime change.

## Root Cause

### SourceWorker startup safety

`SourceWorker` uses raw FFmpeg setup in both file and camera paths. Decoder discovery, parameter copy, and decoder open are not checked end to end, so a failed setup can still reach decode/send logic with invalid state.

### RoomRegistry sync lock scope

`RoomRegistry` sync helpers assume the caller already holds `command_.mutex`. That makes `syncAll()` and `syncNodes` hold the shared hiredis command connection lock across multi-round scan and reply parsing work instead of just the Redis calls.

### SenderTransportController starvation

The pacing tick drains audio ahead of retransmission and fresh video, and the audio pass has no per-tick service bound. Continuous audio backlog can therefore keep fresh video from ever getting a turn.

### Channel non-threaded buffer churn

The non-threaded extractor advances safety by erasing the consumed prefix before dispatching each message. That avoids replay on re-entry, but it does so with one front-erase per message.

## Fix Strategy

### 1. Harden `SourceWorker` decoder setup

- Add a small shared helper for decoder context creation/open.
- Check decoder discovery, context allocation, parameter copy, and decoder open.
- Fail the worker startup path cleanly when setup cannot complete.
- Keep the encoder path behavior unchanged except for explicit startup failure handling.

### 2. Narrow `RoomRegistry` sync critical sections

- Make Redis snapshot helpers self-synchronize around the individual command invocations they perform.
- Remove outer lock coverage around whole sync phases and keep cache merge/parsing outside the command mutex.
- Preserve the single shared hiredis context discipline by locking each command/ensure-connected section explicitly.

### 3. Bound audio service per normal pacing tick

- Preserve audio-first priority.
- Limit how much audio work can be consumed in one normal pacing tick when fresh video is waiting.
- Keep shutdown draining behavior unchanged so queued media can still be flushed aggressively during stop.
- Do not change retransmission priority in this change.

### 4. Bulk-compact non-threaded `Channel` receive buffers

- Track the processed prefix with member state so re-entrant calls can observe progress immediately.
- Advance the shared processed offset before dispatch.
- Compact the processed prefix once at the outermost processing frame.
- Preserve the existing replay-prevention semantics covered by the re-entrant notification test.

## Risk Assessment

- `SourceWorker` startup changes could accidentally suppress valid output if the new guard is overly strict.
- Registry lock-scope changes touch a shared hiredis context; incorrect locking would introduce races or disconnect handling regressions.
- Pacing changes can perturb existing test expectations and transport behavior at low bitrate.
- Channel receive-path changes are sensitive to re-entry ordering.

## Test Strategy

- Reproduction test: add targeted `SourceWorker` decoder-open failure coverage via a controlled FFmpeg hook in a dedicated test translation unit.
- Regression test: add sender-transport controller coverage that proves fresh video progresses while audio remains prioritized.
- Adjacent-path checks: rerun existing `Channel` re-entrant non-threaded tests and existing `SourceWorker` thread integration tests.
- Integration test if applicable: compile and run targeted review-fix / thread-integration targets; Redis-backed registry behavior will be covered by existing integration targets where feasible, otherwise record the gap explicitly.

## Observability

- Use existing log output for `SourceWorker`; add targeted warning logs on decoder setup failure so runtime diagnosis is explicit.
- No new metrics are required for `Channel`.
- `SenderTransportController` keeps existing counters; this change does not require a new public metric.

## Rollout Notes

- No migration is required.
- Rollback is straightforward because the change is limited to local runtime guards and scheduling logic.
- If Redis-backed integration coverage cannot be executed locally, record that verification gap in the handoff.
