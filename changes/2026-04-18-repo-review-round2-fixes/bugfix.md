# Repository Review Round 2 Fixes

## Symptom

Whole-repository review found remaining correctness bugs outside the first plain-client fix set:

- repeated `createWebRtcTransport()` can overwrite the previous transport without explicit close and local cleanup
- consumers remain locally alive after upstream producer close notifications
- consumer RTP parameter generation can return codecs that the remote side did not negotiate

## Reproduction

1. Create a WebRTC send or recv transport twice for the same peer.
2. Produce media or auto-subscribe consumers across the old transport, then replace it.
3. Observe stale local producer/consumer state or leaked worker-side transports.
4. Produce media with a codec unsupported by the subscriber capabilities and call `consume()`.
5. Observe that consumer RTP parameters can still be generated instead of rejecting incompatibility.
6. Close a producer by leaving or replacing the publishing peer.
7. Observe that the subscriber-side consumer object can remain in local peer stats/state.

## Observed Behavior

- WebRTC transport replacement does not consistently close and unregister the previous runtime transport
- `CONSUMER_PRODUCER_CLOSE` does not fully close the local consumer object
- `getConsumerRtpParameters()` falls back to adding unmatched codecs instead of rejecting incompatible ones

## Expected Behavior

- replacing a WebRTC send/recv transport MUST explicitly close the previous transport and clean local producer/consumer indexes
- producer-close notifications MUST close and unregister the local consumer object
- consumer RTP parameters MUST contain only codecs negotiated by the remote endpoint, or fail with no compatible codecs

## Suspected Scope

- `src/RoomServiceMedia.cpp`
- `src/Consumer.cpp`
- `src/ortc.h`
- room media helper paths that register peer-owned producers/consumers
- integration and unit tests covering transport replacement, producer close, and ORTC compatibility

## Known Non-Affected Behavior

- plain transport lifecycle fixes from the earlier review pass
- RTCP signed loss and PLI fixes already applied in the plain client

## Acceptance Criteria

- repeated WebRTC transport creation for the same peer closes and unregisters old transport-owned objects
- consumer objects close and disappear from peer-owned state when the upstream producer closes
- incompatible consumer codec negotiation throws instead of falling back
- regression tests cover send/recv transport replacement, consumer removal after producer close, and ORTC incompatibility rejection

## Regression Expectations

- existing join/produce/consume flows still pass
- existing ORTC tests are updated to assert strict compatibility rather than permissive fallback
- stats and downlink logic continue to work with cleaned-up peer consumer maps
