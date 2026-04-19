# Review2 Runtime Safety Fixes

## Symptom

`revew2.md` surfaced several high-confidence runtime safety defects outside the existing media/client fix waves:

- malformed WebSocket request fields can throw JSON type exceptions from the message callback and terminate the server
- no-geo room-registry candidate sorting uses a comparator that violates strict weak ordering when the current node participates in the candidate set
- worker shutdown paths mutate `closed_` and `routers_` from multiple threads without synchronization
- channel shutdown from the read thread leaves a joinable `std::thread` behind
- recorder startup and H264 extradata paths assume FFmpeg allocation success and valid SPS length, and recorder shutdown races on socket/output-path state

## Reproduction

1. Send a WebSocket JSON message with `"request": 1` or another non-boolean type.
2. Run room resolution with no geo data and include the current node in the candidate set.
3. Trigger worker shutdown/death concurrently with router creation or close.
4. Close a `Channel` from its own read thread.
5. Start/stop a recorder while H264 SPS/PPS data is incomplete or FFmpeg stream allocation fails.

## Observed Behavior

- malformed request payloads can crash the server instead of being dropped
- room registry sorting has undefined behavior in the no-geo fallback
- worker/router shutdown contains data races and unordered teardown
- a joinable read thread can survive `Channel::close()` on the self-thread path
- recorder code can dereference null FFmpeg streams, read short SPS buffers, or race on socket/path state

## Expected Behavior

- malformed WebSocket messages MUST be rejected without crashing the process
- room-registry candidate ordering MUST satisfy strict weak ordering
- worker close/death paths MUST synchronize `closed_` transitions and router-set teardown
- `Channel::close()` MUST detach or join the read thread on every path
- recorder setup and shutdown MUST defend against allocation failure, short SPS input, and shutdown races

## Suspected Scope

- `src/SignalingServerWs.cpp`
- `src/RoomRegistry.cpp`
- `src/RoomRegistrySelection.h`
- `src/Worker.h`
- `src/Worker.cpp`
- `src/Channel.cpp`
- `src/Recorder.h`
- integration and unit tests covering malformed WebSocket input, room-registry comparator behavior, and recorder H264 extradata validation

## Known Non-Affected Behavior

- existing transport replacement and ORTC compatibility fixes tracked in `2026-04-18-repo-review-round2-fixes`
- existing plain-client RTCP/QoS fixes tracked in `2026-04-18-plain-client-review-fixes`

## Acceptance Criteria

- malformed WebSocket request fields no longer crash the server and a subsequent valid request still succeeds on the same process
- no-geo candidate ordering prefers the current node only as a proper tie-breaker and remains irreflexive
- worker close/death and router registration paths use synchronized closed-state and router-set access
- `Channel::close()` detaches the read thread on the self-thread path
- recorder stream allocation and H264 extradata logic reject invalid prerequisites safely

## Regression Expectations

- existing signaling integration tests remain green
- existing worker/channel tests continue to pass
- recorder QoS/unit tests still pass with the new H264 guardrails
