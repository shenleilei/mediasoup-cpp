# Design

## Context

These fixes address system-level lifecycle and contract bugs found during full-repository review.

## Approach

### 1. WebRTC transport replacement cleanup

Extend the same explicit replacement strategy already used for plain transports:

- before assigning a new send/recv WebRTC transport, close the previous one if present
- remove closed peer-owned producers/consumers from peer maps
- remove closed producers from router producer indexes and RoomService owner/demand caches

This keeps worker state, router indexes, and peer-owned state consistent after retries or reconnects.

### 2. Peer-owned object cleanup listeners

When a producer or consumer is inserted into a peer-owned map:

- register a lightweight cleanup listener on local close events
- remove the object from `peer->producers` / `peer->consumers` when it closes locally

This gives direct local-close paths a consistent cleanup mechanism without waiting for later sweeps.

### 3. Consumer producer-close semantics

When `CONSUMER_PRODUCER_CLOSE` arrives:

- mark the consumer closed
- unregister channel notifications for that consumer
- emit the local close signal used by cleanup listeners
- emit the public `producerclose` event

This matches the vendored mediasoup reference semantics closely enough for local lifecycle correctness.

### 4. Strict consumer codec compatibility

Update `ortc::getConsumerRtpParameters()` to align with remote capability negotiation:

- only keep codecs that strictly match the remote capabilities
- do not fall back to unmatched codecs
- remove RTX codecs that do not have an associated matched media codec
- throw if there is no compatible media codec

Existing tests that asserted permissive fallback should be updated to assert rejection instead.

## Verification

- integration tests for repeated WebRTC transport creation and consumer cleanup after producer close
- updated ORTC unit tests for strict incompatibility rejection
- existing integration smoke tests for produce/consume flows remain green
