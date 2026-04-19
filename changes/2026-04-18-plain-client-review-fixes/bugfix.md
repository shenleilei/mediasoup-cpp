# Plain Client Review Fixes

## Symptom

The plain-client and plain transport path contains several review-found correctness bugs:

- replacing a peer plain transport overwrites the old pointer without explicit transport close and local cleanup
- `plainPublish()` advertises H264 without pinning the Baseline profile that the linux client encoder actually emits
- RTCP PLI handling resends a cached keyframe using stale timing rather than requesting a fresh keyframe path
- RTCP RR cumulative loss is parsed as unsigned even though the RTCP field is signed 24-bit
- QoS loss deltas can spike when sampling falls back between local RTCP counters and server stats counters

## Reproduction

1. Join a room as a plain client and invoke `plainPublish()` or `plainSubscribe()` more than once for the same peer.
2. Observe that the previous plain transport is locally overwritten without explicit close/cleanup.
3. Run the linux client against the server and inspect H264 router capabilities versus the encoder profile.
4. Feed RTCP RR packets with signed negative cumulative loss or trigger a temporary gap in server stats responses.
5. Observe incorrect loss parsing or an artificial QoS loss spike after source switching.

## Observed Behavior

- old plain transports are not explicitly closed before replacement
- producer RTP parameters can describe Main-profile H264 while the client sends Baseline
- PLI recovery uses cached keyframe replay semantics rather than a fresh keyframe request path
- negative RR cumulative loss is decoded as a large positive integer
- QoS control derives deltas across incompatible counter baselines

## Expected Behavior

- replacing a plain transport MUST explicitly close and locally clean the previous transport-owned objects before reuse
- plain publish MUST register the Baseline H264 profile emitted by the client
- PLI handling MUST request a fresh keyframe path instead of replaying stale timing from cache
- RR cumulative loss MUST be decoded as signed 24-bit
- QoS deltas MUST reset when the loss counter source switches between local RTCP and server stats

## Suspected Scope

- `src/RoomServiceMedia.cpp`
- `client/RtcpHandler.h`
- `client/NetworkThread.h`
- `client/main.cpp`
- RTCP and plain-client unit/integration tests

## Known Non-Affected Behavior

- NACK retransmission packet-store behavior
- JS/C++ QoS override protocol validation
- core QoS state-machine transition rules

## Acceptance Criteria

- replacing plain send/recv transports explicitly closes the previous transport and removes closed local producer/consumer entries
- `plainPublish()` selects and registers the Baseline H264 codec/profile used by the client encoder
- PLI handling requests a fresh keyframe path in both threaded and non-threaded client paths
- RR cumulative loss parsing handles signed 24-bit values in both threaded and non-threaded RTCP paths
- QoS sampling resets its loss delta baseline when switching between local RTCP and server stats

## Regression Expectations

- existing plain-client integration behavior remains working
- RTCP sender-report, NACK, and threaded-network tests continue to pass
- new regression tests cover transport replacement cleanup, Baseline codec selection, signed RR parsing, PLI handling, and stats-source switching
