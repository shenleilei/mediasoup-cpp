# WsClient Hardening

## Summary

The plain-client `WsClient` still has several protocol and runtime-safety gaps:

- client writes assume a single `send()` completes the whole frame
- incoming Ping control frames are not answered with Pong
- fragmented text messages using continuation frames are not reassembled
- incoming frame lengths are accepted without an upper bound, allowing unbounded allocation
- the class still exposes mutable internal state publicly

## Reproduction

1. Review `client/WsClient.cpp` and `client/WsClient.h`.
2. Observe that `sendText()` performs a single `::send(...)` call for the entire frame.
3. Observe that `recvText()` treats every non-text opcode except close as `Timeout`.
4. Observe that `recvText()` allocates `std::string data(len, 0)` directly from the wire length with no maximum.
5. Observe that `WsClient` members such as `fd`, `running_`, `pendingRequests_`, and `onNotification` are public.

## Observed Behavior

- a partial kernel write can truncate a client frame on the wire
- Ping frames are consumed without sending the required Pong
- fragmented server text messages are dropped instead of reassembled
- a malicious or buggy server can force arbitrarily large client allocations
- external code can mutate `WsClient` internals directly and bypass its locking rules

## Expected Behavior

- client WebSocket writes MUST loop until the entire masked frame is written or the connection fails
- Ping control frames MUST trigger a Pong response with the same payload
- fragmented text messages MUST be reassembled across continuation frames before JSON parsing
- incoming frame sizes MUST be bounded and oversized frames MUST fail safely
- `WsClient` internal transport and synchronization state SHOULD be private, with explicit accessors/setters for supported integration points

## Known Scope

- `client/WsClient.h`
- `client/WsClient.cpp`
- `client/PlainClientApp.cpp`
- `tests/test_ws_client.cpp`
- any directly affected client build wiring

## Must Not Regress

- existing request/response semantics for `request()` and `requestAsync()`
- current notification dispatch behavior used by `PlainClientApp`
- prompt failure of pending async requests when the connection closes
- existing plain-client integration behavior that depends on `WsClient`

## Suspected Root Cause

Confidence: high.

`WsClient` was originally a small internal helper, and the implementation kept the happy path only:

- write-side code assumed short frames and cooperative kernels
- read-side code only handled whole text frames and close frames
- the class stayed as a `struct`-like API even after growing internal concurrency state

## Acceptance Criteria

### Requirement 1

`WsClient` SHALL send complete client frames or fail the connection explicitly.

#### Scenario: kernel write completes only part of a frame

- WHEN a WebSocket frame write requires multiple `send()` calls
- THEN `WsClient` SHALL continue writing until the full frame is sent
- AND it SHALL not silently truncate the outgoing frame

### Requirement 2

`WsClient` SHALL handle WebSocket control and fragmentation semantics needed by the plain client.

#### Scenario: server sends Ping

- WHEN the server sends a Ping frame
- THEN the client SHALL send a Pong frame with the same payload
- AND normal message processing SHALL continue

#### Scenario: server sends a fragmented text message

- WHEN the server sends a text frame followed by continuation frames
- THEN the client SHALL reassemble the full message and deliver it once

### Requirement 3

`WsClient` SHALL reject oversized incoming frames safely.

#### Scenario: oversized incoming frame

- WHEN the server advertises a frame size above the configured client limit
- THEN the client SHALL fail the connection without attempting an unbounded allocation
- AND pending requests SHALL fail promptly

### Requirement 4

`WsClient` SHALL expose only supported integration points and keep mutable transport state private.

#### Scenario: notification integration

- WHEN `PlainClientApp` registers for notifications
- THEN it SHALL do so through an explicit `WsClient` API rather than by mutating public state

## Regression Expectations

- existing `WsClient` close/fail-pending behavior remains covered
- new regression coverage is added for:
  - Ping to Pong handling
  - fragmented text message reassembly
  - oversized frame rejection
  - continued request completion after Ping/continuation handling
- manual smoke checks are optional if targeted automated tests remain sufficient
