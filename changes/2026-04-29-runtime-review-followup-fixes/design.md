# Design

## Context

The confirmed defects span signaling finalization, Redis-backed cache publication, transport protocol validation, and low-level channel I/O. The changes should stay narrow, preserve existing successful flows, and add explicit failure handling where the current code silently commits invalid state.

## Approach

### 1. Join completion commits only after worker availability check

- Keep worker-side request dispatch unchanged.
- In the deferred main-loop completion callback, resolve the worker first.
- If the join result was logically successful but the worker is no longer available:
  - clear pending join state for that session
  - unassign the room if needed
  - send an explicit join failure response to an alive socket
  - skip socket registration entirely
- Reuse a small socket-state helper so the pending-state cleanup is explicit and unit-testable.

This keeps the main-thread `wsMap` and `PerSocketData` in sync with actual worker availability.

### 2. Publish full room-registry snapshots only when complete

- Teach `scanKeys()` to report whether the scan completed successfully instead of returning only a partial key list.
- Make full-snapshot assembly require:
  - complete node scan
  - valid node `MGET` reply
  - complete room scan
  - valid room `MGET` reply
- If any of those steps fail, log the incomplete snapshot and keep the old cache unchanged.
- Keep `syncNodesSnapshot()` conservative as well so malformed node snapshot fetches do not partially mutate the cache unexpectedly.

The key invariant is: `cache_.replaceAll(...)` only runs for a complete snapshot assembled from consistent Redis replies.

### 3. Validate transport connect worker responses centrally

- Add a small internal helper header for connect-response parsing/validation shared by WebRTC, plain, and pipe transports.
- WebRTC connect:
  - require `WebRtcTransport_ConnectResponse`
  - require a concrete local DTLS role
  - return and persist the parsed role instead of hardcoding `"server"`
- Plain connect:
  - require `PlainTransport_ConnectResponse`
  - require a valid tuple before returning success
- Pipe connect:
  - require `PipeTransport_ConnectResponse`
  - require a valid tuple before returning success

This keeps validation logic consistent without adding a new public abstraction layer.

### 4. Retry recoverable channel write errors

- Add a tiny helper that classifies recoverable channel write errors.
- In `sendBytes()`:
  - retry on recoverable errors such as `EINTR`
  - keep hard failures on unrecoverable write errors or zero-byte writes
- Preserve the existing close-on-hard-failure behavior.

This fix is intentionally minimal and does not redesign the broader channel send path.

## Non-Goals

- Redesigning worker ownership or uWS shutdown sequencing outside this join-finalization guard
- Changing transport connect JSON shapes beyond making them truthful and validated
- Reworking Redis command-connection ownership or heartbeat locking in the same change

## Risks

- Join finalization changes touch an already subtle race path; failure responses must not regress the existing early-close rollback behavior.
- Snapshot completeness checks can change cache-refresh frequency under Redis instability; logs need to make the reason clear.
- Connect-response validation could surface previously hidden worker/test harness issues by turning silent success into explicit failure.
- Channel write retry must not hide real pipe failure indefinitely; only clearly recoverable errors should loop.

## Verification Strategy

- unit tests for transport connect response parsing/validation using real `Channel` request/response pipes with a fake worker responder
- unit test for recoverable channel write error classification
- registry sync test that simulates a connection that drops during `syncAllSnapshot()` after the connection is established
- socket-state unit coverage for unavailable-worker join commit preparation
- targeted reruns:
  - `mediasoup_tests`
  - `mediasoup_review_fix_tests`

## Rollout Notes

- No migration or config change is required.
- Runtime behavior becomes stricter on malformed worker responses; rollback is straightforward because the change is local to validation and state-finalization guards.
