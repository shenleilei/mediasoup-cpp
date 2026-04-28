# Tasks

1. [x] Persist subscriber RTP capabilities from consuming transport creation.
   - Files: `src/SignalingRequestDispatcher.h`, `src/RoomService.h`, `src/RoomServiceMedia.cpp`
   - Outcome: peers created without join-time RTP capabilities can still auto-subscribe after recv transport creation.
   - Verify: targeted integration test.

2. [x] Update browser demo recv transport creation to send loaded RTP capabilities.
   - Files: `public/qos-demo.js`
   - Outcome: the demo page provides the server the capabilities needed for later auto-subscribe and starts remote video playback in an autoplay-safe state.
   - Verify: browser publish/subscribe repro script.

3. [x] Preserve full consumable RTP encoding metadata in consume requests.
   - Files: `src/Transport.cpp`
   - Outcome: browser consumers receive decodable video after subscription.
   - Verify: browser capacity harness.

4. [x] Add regression coverage for the browser-style signaling order.
   - Files: `tests/test_integration.cpp`
   - Outcome: regression fails without the fix and passes with it.
   - Verify: targeted integration test binary.
