# Design

## Context

`WsClient` is still small enough to fix directly without introducing a larger networking abstraction.

The goal is to harden the existing blocking socket client while preserving its role in the plain-client app.

## Root Cause

The implementation only models the simplest successful case:

- one blocking write per frame
- one whole text frame per incoming message
- no explicit control-frame handling
- direct public access to internals from call sites

That is sufficient for simple local tests but not robust against real WebSocket behavior.

## Fix Strategy

### 1. Centralize raw frame IO

Add internal helpers in `WsClient.cpp` for:

- writing all bytes of a buffer with retry-on-`EINTR`
- reading exact byte counts with retry-on-`EINTR`
- sending client frames with masking for text and Pong replies

`sendText()` should use the shared frame writer rather than a single raw `send()`.

### 2. Rework `recvText()` into a frame-processing loop

Keep the public API shape, but internally:

- poll with a rolling timeout budget
- parse frame headers and extended lengths
- enforce a maximum incoming frame size before allocation
- reply to Ping frames with Pong and continue reading
- ignore Pong frames
- reassemble text plus continuation frames until `FIN`
- treat malformed continuation sequences or oversized frames as connection failure

This keeps `readerLoop()` behavior simple while giving it correct wire semantics.

### 3. Privatize mutable state and expose explicit hooks

Move transport state, queues, and locks to `private`.

Expose only the supported integration point needed by the plain client:

- a setter for the notification callback

This removes direct mutation of internal fields without changing higher-level plain-client behavior.

### 4. Extend focused unit tests

Expand `tests/test_ws_client.cpp` using the local test server to cover:

- Ping followed by a valid response, including observing client Pong
- fragmented text response reassembly
- oversized frame rejection and prompt async failure
- existing remote-close pending-request behavior

These tests are sufficient for this wave; no broader plain-client integration change is required.

## Risk Assessment

- frame parsing changes can accidentally break the current happy-path response flow
- replying to Ping from the reader thread must not deadlock with normal writes
- oversized-frame handling must fail fast without leaving pending requests stuck
- privatizing members requires touching `PlainClientApp`, so the change must stay API-compatible there

## Test Strategy

- extend `tests/test_ws_client.cpp` with protocol-focused unit tests
- run `mediasoup_tests` after the `WsClient` and `PlainClientApp` changes
- if the plain-client build wiring reveals a compile-only integration issue, resolve it in the same wave

## Observability

- no new runtime logs are required
- failure paths should continue to propagate as request failures rather than silent hangs

## Rollout Notes

- no schema, protocol, or server behavior changes are intended
- this is a client-side hardening wave only
