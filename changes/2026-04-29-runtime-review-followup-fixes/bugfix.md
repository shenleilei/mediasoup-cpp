# Bugfix Analysis

## Summary

Current source still contains four confirmed runtime correctness defects from the latest repository review:

- WebSocket `join` completion can commit socket session state before verifying that the target worker still exists on the main loop.
- `RoomRegistry` full-sync can replace the in-memory cache with an incomplete snapshot after Redis scan/fetch failure.
- transport `connect()` helpers accept malformed or mismatched worker responses as success.
- `Channel::sendBytes()` treats recoverable pipe write interruption as fatal and closes the channel.

## Reproduction

1. Inspect `src/SignalingServerWs.cpp` deferred `join` completion and note that `RegisterJoinedSocket(...)` runs before the `deferredWt` existence check.
2. Inspect `src/RoomRegistrySync.cpp` and note that `scanKeys()` returns partial keys on failure while `syncAllSnapshot()` still calls `cache_.replaceAll(...)`.
3. Inspect `src/WebRtcTransport.cpp`, `src/PlainTransport.h`, and `src/PipeTransport.cpp` and note missing validation of the worker response body.
4. Inspect `src/Channel.cpp` and note that `sendBytes()` closes the channel on any `write()` result `<= 0` without retrying `EINTR`.

## Observed Behavior

- A successful worker-side join can still leave main-thread socket state committed even when the worker disappeared before finalization.
- A failed Redis full-sync can publish a partial or empty cache snapshot.
- Worker protocol drift or malformed connect responses are reported to callers as success.
- Signal interruption on channel pipe write can trigger channel close and wider worker-failure handling.

## Expected Behavior

- Join completion SHALL validate worker availability before committing joined socket state.
- Full snapshot publication SHALL happen only after a complete Redis scan/fetch result is assembled.
- Transport connect helpers SHALL validate the expected response body and fail explicitly on mismatch or missing fields.
- Channel pipe writes SHALL retry recoverable interruption instead of treating it as a hard failure.

## Known Scope

- `src/SignalingServerWs.cpp`
- `src/SignalingSocketState.h`
- `src/RoomRegistry.h`
- `src/RoomRegistrySync.cpp`
- `src/Channel.h`
- `src/Channel.cpp`
- `src/WebRtcTransport.cpp`
- `src/PlainTransport.h`
- `src/PipeTransport.cpp`
- `tests/test_review_fixes.cpp`
- `tests/test_room_registry_sync.cpp`
- `specs/current/runtime-safety.md`

## Must Not Regress

- repeated-join rejection and early-close join rollback semantics
- existing room-registry sync lock-scope behavior and cache lookup semantics outside incomplete-snapshot handling
- successful transport connect flows for WebRTC, plain, and pipe transports
- existing channel close behavior on real hard write failures

## Out Of Scope

- retransmission-vs-fresh-video pacing policy
- `RoomRegistry::heartbeat()` mutex hold-time redesign beyond the incomplete full-sync fix
- broader transport API redesign beyond response validation

## Acceptance Criteria

### Requirement 1

Join completion SHALL refuse to commit socket session state when the worker is unavailable during main-loop finalization.

#### Scenario: Worker disappears before deferred join completion

- WHEN worker-side join succeeded but the deferred completion callback cannot resolve the worker anymore
- THEN the socket is not registered as joined
- AND pending join state is cleared
- AND the client receives an explicit failure response if the socket is still alive

### Requirement 2

RoomRegistry full-sync SHALL preserve the previous cache when the snapshot assembly is incomplete.

#### Scenario: Redis command failure during full snapshot

- WHEN `syncAllSnapshot()` loses Redis connectivity or receives an invalid snapshot reply after the initial connectivity check
- THEN the existing cache remains unchanged
- AND the incomplete snapshot is not published via `replaceAll(...)`

### Requirement 3

Transport connect helpers SHALL validate worker response bodies before returning success.

#### Scenario: Malformed or mismatched connect response

- WHEN the worker response body type does not match the requested transport connect operation
- OR required tuple / DTLS role fields are missing or invalid
- THEN the helper throws an explicit error instead of returning success JSON

### Requirement 4

Channel pipe writes SHALL retry recoverable interruption.

#### Scenario: Recoverable write interruption

- WHEN `write()` fails with a recoverable interruption such as `EINTR`
- THEN `sendBytes()` retries the write
- AND the channel is not closed solely because of that recoverable error

## Regression Expectations

- add focused unit coverage for transport connect response validation and channel write retry classification
- add focused registry sync coverage for incomplete full-snapshot publication
- reuse existing join-state unit coverage and extend it for unavailable-worker completion semantics where feasible
- run the existing review-fix / room-registry test target plus the unit test target covering the changed helpers
