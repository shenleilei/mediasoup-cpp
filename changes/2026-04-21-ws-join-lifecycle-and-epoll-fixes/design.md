# Design: Join lifecycle hardening and epoll error branching

## Goals

- Eliminate repeated-join ambiguity on a single websocket session.
- Ensure join success cannot outlive a closed websocket without cleanup.
- Make event-loop syscall error behavior explicit and diagnosable.

## Approach

### 1) Repeated join guard

In WS message handling, before dispatching `join` to worker:

- Track pending join state separately from committed socket session state.
- If current socket already has either:
  - a valid mapped session (`HasMappedSession(...)`), or
  - a pending join in flight,
  reject with deterministic error (`already joined`).

This enforces one active or in-flight joined session per websocket connection while keeping non-join requests bound only to committed session state.

### 2) Join-close race rollback

In join completion defer callback:

- Always perform join-failure room-unassign cleanup first (independent of socket alive state).
- Never dereference raw `ws` in deferred cleanup before checking the shared `alive` flag.
- If `joinOk` but `alive == false`, post rollback task to worker thread:
  - check room/peer exists and `peer->sessionId == newSessionId`;
  - call `leave(roomId, peerId)` only on exact match.

Session guard ensures we do not remove a newer replacement session.

### 3) epoll_wait error handling

In `WorkerThread::loop()`:

- `nfds == 0`: continue timeout loop.
- `nfds < 0 && errno == EINTR`: continue.
- `nfds < 0` otherwise: log error (unless stopping), break loop to avoid silent spin.

Add a small helper function in `WorkerThread.h/.cpp` for explicit errno classification and testability.

## Why this is safe

- Repeated-join reject aligns with current single-session socket state model (`PerSocketData` stores one room/peer/session).
- Session-guarded rollback prevents ghost peers without harming valid reconnect replacement.
- Explicit epoll error branch turns undefined implicit behavior into deterministic handling.

## Non-goals

- Multi-session-per-WebSocket protocol redesign.
- Broad worker-loop refactor beyond error-path handling.

## Verification

- Add integration test: second `join` on same websocket rejected, original session remains functional.
- Add integration stress test: repeated join-then-immediate-close does not leave ghost participants.
- Add unit test for epoll errno classifier.
- Run: `./build/mediasoup_tests` (and targeted test filters during iteration).
