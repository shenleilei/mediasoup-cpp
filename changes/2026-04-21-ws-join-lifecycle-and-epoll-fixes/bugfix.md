# Bugfix: WS join lifecycle and WorkerThread epoll error handling

## Symptom

The review identified three defects:

1. Same WebSocket can `join` repeatedly, leaving stale socket mappings and old room membership risk.
2. If socket disconnects during async `join`, worker-side join may commit while main-loop completion is skipped, leaving ghost peer/room state.
5. `WorkerThread::loop()` does not explicitly handle `epoll_wait` errors (`nfds < 0`).

## Reproduction

- Defect 1: send two `join` requests on the same WebSocket without reconnecting.
- Defect 2: send `join`, close socket immediately, repeat under load; observe potential stale participants.
- Defect 5: force/observe `epoll_wait` returning negative errno (e.g. `EINTR`, `EBADF`) and no explicit branch handling.

## Observed behavior

- Defect 1: session mapping can become inconsistent if a socket rebinds identity without explicit lifecycle close.
- Defect 2: early-close join can create worker peer state without registered websocket session.
- Defect 5: error path is implicit, risking silent degraded loop behavior.

## Expected behavior

- A WebSocket has at most one active joined session; repeated `join` on active session is rejected.
- Join completion path always performs lifecycle cleanup when the socket is already closed.
- Worker event loop handles `epoll_wait` errors explicitly and safely.

## Suspected scope

- `src/SignalingServerWs.cpp`
- `src/SignalingSocketState.h`
- `src/WorkerThread.cpp`, `src/WorkerThread.h`
- tests in `tests/test_review_fixes_integration.cpp` and `tests/test_review_fixes.cpp`

## Known non-affected behavior

- Normal reconnect with new WebSocket + same peerId remains supported.
- Existing sessionId stale-request protection for non-join methods remains unchanged.

## Acceptance criteria

- Second `join` on an already-joined WebSocket returns error and keeps original session usable.
- Closed-socket join completion triggers conditional rollback (`sessionId` guarded) to avoid ghost peer.
- `epoll_wait` negative returns are handled explicitly (`EINTR` continue, fatal errors logged and loop exits).
- Targeted tests pass and existing unit suites remain green.

## Regression expectations

- No regression for reconnect replacement behavior.
- No regression for room assignment/unassignment lifecycle.
