# Design

## Goal

Apply the review fixes narrowly without changing the overall plain-client architecture.

## Approach

### 1. Explicit plain transport replacement cleanup

Before assigning a new plain send/recv transport to a peer:

- close the existing transport if present
- remove closed local producers and consumers from the peer maps
- clean producer-owner and producer-demand caches for any removed producers

This keeps worker state and local indexing aligned without introducing a larger transport-lifecycle refactor.

### 2. Baseline H264 selection in `plainPublish()`

The linux client encoder is pinned to H264 Baseline. `plainPublish()` will:

- select the router H264 codec whose parameters match Baseline packetization-mode 1
- propagate that codec profile into the producer RTP parameters

If no compatible Baseline codec exists, the request should fail clearly instead of silently registering the wrong codec contract.

### 3. Fresh-keyframe PLI handling

PLI/FIR should not replay a cached keyframe with stale timing.

- `RtcpContext` will expose a callback that requests a fresh keyframe for a specific SSRC
- non-threaded mode will set the matching track to force the next encoded frame to keyframe
- threaded mode already has a force-keyframe path, so the immediate cached replay will be removed and the worker command path will be used

NACK retransmission remains cache-based; only PLI/FIR changes.

### 4. Signed RTCP RR cumulative loss parsing

Introduce a shared helper for signed 24-bit RTCP RR cumulative-loss decoding and use it in:

- `client/RtcpHandler.h`
- `client/NetworkThread.h`

This keeps threaded and non-threaded code paths consistent.

### 5. QoS loss-counter source switching

The QoS controller currently keeps one previous snapshot. When the packets-lost source changes between local RTCP and server stats, delta math must be reset.

Add a narrow controller method to prime the counter baseline with the current snapshot before `onSample()` when the source changes. This preserves existing state-machine context while preventing invalid cross-source deltas.

## Verification

- targeted unit tests for RTCP parsing and PLI behavior
- targeted unit/integration tests for plain transport replacement and Baseline H264 selection
- QoS unit tests for source-switch loss baseline reset
- build/tests for affected suites
