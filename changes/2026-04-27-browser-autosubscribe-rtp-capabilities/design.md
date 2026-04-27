# Design

## Root Cause
The browser demo joins before mediasoup-client `Device.load()`, so the `join` request does not send subscriber `rtpCapabilities`.

Server-side auto-subscribe later consumes producers using `peer->rtpCapabilities`. For browser subscribers that only create a recv transport after `Device.load()`, the stored capabilities remain empty, and `Transport::consume()` fails with `no compatible codecs`.

## Chosen Fix
Use the consuming `createWebRtcTransport` request as the point where browser subscribers provide their loaded RTP capabilities.

### Changes
1. Extend the signaling dispatcher and `RoomService::createTransport()` to accept optional `rtpCapabilities` data.
2. When creating a consuming transport, update `peer->rtpCapabilities` from the provided capabilities before any auto-subscribe work.
3. Update `public/qos-demo.js` to send `state.device.rtpCapabilities` when creating the recv transport.
4. Align `Transport::consume()` consumable RTP encoding serialization with the upstream mediasoup Node implementation so the worker receives the full encoding metadata.
5. Start the demo page's remote video cards muted so autoplay can start reliably after subscription.
6. Add an integration regression test for:
   - `join` without RTP capabilities
   - recv transport creation with RTP capabilities
   - later producer publish causing `newConsumer`

## Why This Approach
- Keeps the fix narrow and aligned with the browser flow that already knows its capabilities at recv transport creation time.
- Avoids adding a separate signaling method or changing publish-side behavior.
- Preserves existing clients that already send RTP capabilities during `join`.

## Non-goals
- Reworking the overall join handshake.
- Replacing server-side auto-subscribe with client-driven consume for all peers.
