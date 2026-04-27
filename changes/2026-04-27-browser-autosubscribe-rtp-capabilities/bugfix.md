# Bugfix Plan: Browser Auto-subscribe RTP Capabilities

## Symptom
When two browser pages join the same room and one page publishes audio/video, the subscriber page stays on the remote placeholder and never renders video.

## Reproduction
1. Open `public/index.html` in two browser sessions.
2. Join the same room from both sessions.
3. Publish media from one session.
4. Observe that the other session does not receive a remote video card or `newConsumer`.

## Observed Behavior
- Publish succeeds on the sender.
- The subscriber creates a recv transport successfully.
- Server logs show `auto-subscribe FAILED ... no compatible codecs`.

## Expected Behavior
- A browser subscriber that has already loaded mediasoup `Device` capabilities and created a recv transport should receive `newConsumer` notifications for later producers.
- The demo page should render the remote video after the sender publishes.

## Suspected Scope
- `public/qos-demo.js`
- `src/SignalingRequestDispatcher.h`
- `src/RoomService.h`
- `src/RoomServiceMedia.cpp`
- `tests/test_integration.cpp`

## Known Non-affected Behavior
- Test clients that already send `rtpCapabilities` during `join` continue to auto-subscribe correctly.
- Explicit `consume` requests that pass `rtpCapabilities` directly still work.

## Acceptance Criteria
- `createWebRtcTransport` accepts subscriber RTP capabilities for consuming transports and persists them on the peer.
- The browser demo sends `state.device.rtpCapabilities` when creating a recv transport.
- Browser consumer setup preserves the consumable RTP encoding fields needed for decode.
- The demo page renders remote video reliably after subscription.
- A regression test covers `join` without RTP capabilities followed by recv transport creation with RTP capabilities and later producer auto-subscribe.

## Regression Expectations
- Existing join-time RTP capability flows remain valid.
- Producing transports must not require RTP capabilities.
