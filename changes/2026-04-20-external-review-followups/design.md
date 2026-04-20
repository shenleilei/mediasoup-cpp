# Design

## Context

The external review ledger in `review.md` is useful, but it is too mixed to implement as one literal checklist.

For this wave, the change should stay reviewable and focus only on confirmed, still-live defects with clear code evidence:

- shared media/FFmpeg helper correctness
- room/QoS lifecycle cleanup
- room-registry stale redirect hardening
- small safe cleanup in the touched boundary

This wave intentionally does not absorb the broader pre-existing `WsClient` review debt, header layout cleanups, or other low-signal style issues.

## Root Cause

The remaining defects have different immediate causes but a common pattern: the refactor preserved the main control flow while missing some edge-case semantics.

- The new media helpers preserved the happy path but not all wire-level invariants.
- The FFmpeg adapter wraps receive-side `EAGAIN` correctly but does not treat send-side `EAGAIN` as the same normal flow-control condition.
- The RoomService cleanup code removes the original peer-keyed override record but does not also remove the later room-pressure key variant.
- Room-registry caching optimizes remote redirects but does not revalidate cached remote ownership before returning it.
- Small helper duplication and an unused registry wrapper were left behind while the code was being split.

## Fix Strategy

### 1. Correct shared media helper semantics

Apply narrow fixes in the shared RTP helpers:

- preserve the original H264 NAL F and NRI bits when building FU-A indicators
- make `RtpHeader::Parse()` reject packets whose version is not RTP v2
- make `RtpHeader::Parse()` subtract declared padding bytes from the payload span and reject invalid padding lengths

These are deterministic protocol fixes with low blast radius and direct unit-test coverage.

### 2. Harden the FFmpeg adapter layer

Keep the wrapper API small but make it behave like FFmpeg:

- `OutputFormat::Close()` should perform a best-effort trailer write when `headerWritten_` is still true
- explicit `WriteTrailer()` should remain the primary throwing API
- `Close()` must not leak resources or allow destructor-time exceptions to escape
- send-side wrappers for decoder, encoder, and bitstream filter should expose `EAGAIN` as a normal false/try-again result, matching the receive-side wrappers
- `MakeBitstreamFilterContext()` should propagate `av_bsf_alloc()` failures through `CheckError()`

If the send-side wrapper signatures need to change from `void` to `bool`, that API change should be kept local to the adapter layer and its direct callers.

### 3. Fix peer lifecycle cleanup symmetry

Treat automatic QoS override state as two parallel key spaces:

- peer key: `roomId/peerId`
- room-pressure key: `roomId/peerId#room`

`join(reconnect)` and `leave()` should clear both keys. This is a narrow state-cleanup fix and does not require changing the planner or the override protocol itself.

While touching this boundary, extract the duplicated `BuildStatsStoreResponseData(...)` helper into `RoomStatsQosHelpers.h` so the client-stats and downlink-stats paths cannot drift.

### 4. Harden cached room redirects without redesigning the cache model

Do not redesign `RoomRegistry` caching in this wave.

Use a narrow validation step for cached remote redirects:

- only when `claimRoom()` hits a cached remote owner address
- re-check the room ownership and/or remote node liveness against Redis under the existing command lock
- if the cached remote node is gone or the ownership no longer matches, refresh or reclaim instead of returning the stale redirect

This keeps the change small while addressing the concrete stale-cache failure mode.

### 5. Normalize touched RoomRegistry/RoomService boundary hygiene

Two low-risk cleanups belong in this wave because they reduce confusion in the exact code being changed:

- make `RoomRegistry::start()` follow the same command mutex discipline as other public command paths
- remove the unused `RoomService::heartbeatRegistry()` wrapper instead of leaving a dead worker-thread Redis path in the interface

These should be separate small edits, not broad refactors.

## Risk Assessment

- The FFmpeg wrapper changes may require small call-site updates if the send APIs return `bool`.
- A best-effort trailer write in `Close()` must not introduce destructor exceptions or double-trailer writes.
- The room-registry stale-redirect hardening must preserve the current fail-open local behavior when Redis is unavailable.
- Removing the dead heartbeat wrapper must not remove a call site that exists only in a less-common build configuration; verify with global search and final build/test coverage.

## Test Strategy

- add or update unit tests in `tests/test_common_media.cpp` for FU-A F-bit preservation and RTP version/padding parsing
- add targeted regression coverage for FFmpeg wrapper cleanup/backpressure behavior in an existing review-fix or recorder-adjacent test target
- add or update room/QoS lifecycle coverage in `tests/test_room.cpp` or `tests/test_review_fixes.cpp`
- add or update registry redirect regression coverage in `tests/test_multinode.cpp` or `tests/test_review_fixes_integration.cpp`
- run the affected gtest targets and the final `ctest` subset for touched modules

## Observability

- no new user-facing metrics are required
- if the room-registry stale-cache path is invalidated, keep enough debug/warn logging to explain why a cached redirect was rejected or refreshed
- FFmpeg close-path failures should remain diagnosable without crashing destructors

## Rollout Notes

- no schema, API, or migration work is expected
- no accepted behavior change is planned beyond bringing the implementation back into line with existing runtime-safety and protocol expectations
- if a fix reveals that an accepted behavior doc is incomplete, update `specs/current/` as part of the implementation wave rather than silently drifting
