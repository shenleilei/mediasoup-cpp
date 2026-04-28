# Bugfix Analysis

## Summary

Repository review follow-up identified four high-priority runtime issues that are still present in current source:

- `SourceWorker` decoder startup ignores FFmpeg failure returns and can continue after a failed decoder open.
- `RoomRegistry` full sync holds the Redis command mutex across multi-round `SCAN` and `MGET` work.
- `SenderTransportController` can let continuous audio service starve fresh video in normal pacing.
- non-threaded `Channel::processAvailableData()` uses per-message head erases on the receive buffer hot path.

## Reproduction

1. Inspect `client/SourceWorker.h` file/camera decoder setup and observe ignored `avcodec_open2()` returns.
2. Inspect `src/RoomRegistrySync.cpp` and `src/RoomRegistry.cpp` sync callers and observe long-lived `command_.mutex` coverage.
3. Inspect `client/SenderTransportController.h` pacing order and observe unbounded `FlushAudioQueue()` ahead of fresh video.
4. Inspect `src/Channel.cpp` non-threaded receive loop and compare it with the threaded offset-based path.

## Observed Behavior

- Decoder open failures are not handled explicitly before decode/send loops begin.
- Redis-backed sync blocks unrelated registry operations for the whole scan/snapshot window.
- Fresh video can be denied a send opportunity while audio backlog keeps draining first.
- Non-threaded channel processing performs repeated O(N) buffer compaction on a hot path.

## Expected Behavior

- `SourceWorker` SHALL fail safely when decoder setup cannot be completed.
- Redis full-sync SHALL minimize mutex hold time to the individual command round-trips needed for correctness.
- Normal pacing SHALL preserve audio priority without allowing indefinite starvation of fresh video.
- Non-threaded channel message extraction SHALL retain re-entrant safety while avoiding per-message head erase churn.

## Known Scope

- `client/SourceWorker.h`
- `client/SenderTransportController.h`
- `src/Channel.h`
- `src/Channel.cpp`
- `src/RoomRegistry.h`
- `src/RoomRegistry.cpp`
- `src/RoomRegistrySync.cpp`
- `tests/test_thread_model.cpp`
- `tests/test_thread_integration.cpp`
- `CMakeLists.txt`
- relevant accepted specs under `specs/current/`

## Must Not Regress

- Existing `SourceWorker` happy-path file source behavior and command acknowledgements.
- `Channel` non-threaded re-entrant notification handling.
- Audio-before-video priority when both are queued on the same tick.
- Existing room registry correctness for resolve/claim/cache sync.

## Suspected Root Cause

- `SourceWorker` duplicates raw FFmpeg setup without enforcing the error handling used elsewhere in `common/ffmpeg`.
- `RoomRegistry` sync helpers depend on external lock discipline and currently keep the shared hiredis context locked for whole snapshot phases.
- `SenderTransportController` prioritizes audio by draining it fully before any fresh-video pass.
- The non-threaded `Channel` extractor preserved re-entrant safety by erasing the consumed prefix immediately, trading correctness for avoidable compaction cost.

## Acceptance Criteria

### Requirement 1

The system SHALL stop `SourceWorker` cleanly when decoder initialization fails.

#### Scenario: Decoder open failure

- WHEN decoder setup fails for a valid `SourceWorker` startup path
- THEN the worker exits without enqueuing encoded output or entering decode/send loops

### Requirement 2

The system SHALL reduce `RoomRegistry` sync mutex hold time to per-command critical sections.

#### Scenario: Full sync

- WHEN `syncAll()` or `syncNodes` refreshes Redis-backed snapshots
- THEN `command_.mutex` is not held across the whole scan/parse/cache-merge sequence

### Requirement 3

The system SHALL give fresh video a bounded opportunity to send even when audio backlog is continuous.

#### Scenario: Audio and fresh video queued together

- WHEN normal pacing runs with queued audio and queued fresh video
- THEN audio is still serviced first
- AND fresh video can still make progress without waiting for the entire audio backlog to drain

### Requirement 4

The system SHALL avoid per-message receive-buffer compaction in non-threaded channel processing without reintroducing replay on re-entry.

#### Scenario: Re-entrant notification callback

- WHEN a non-threaded notification callback re-enters `requestWait()` / `processAvailableData()`
- THEN already-consumed buffered messages are not replayed
- AND the processed prefix is compacted in bulk instead of one erase per message

## Regression Expectations

- Existing unaffected behavior: `SourceWorker` normal file-source encode path, `SenderTransportController` retransmission priority, `Channel` threaded mode, registry routing decisions.
- Required automated regression coverage: targeted sender-transport tests, targeted channel re-entry tests, targeted `SourceWorker` failure-path coverage, targeted thread-integration tests, build/test target that compiles the changed registry path.
- Required manual smoke checks: none beyond automated scope unless Redis-backed integration coverage is unavailable in the local environment.
