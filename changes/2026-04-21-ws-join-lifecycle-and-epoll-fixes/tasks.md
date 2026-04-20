# Tasks

1. [completed] Harden websocket join lifecycle in signaling layer.
   - Files: `src/SignalingServerWs.cpp`, `src/SignalingSocketState.h`, `src/RoomService.h`, `src/RoomServiceLifecycle.cpp`
   - Outcome: reject repeated join on active/pending session; closed-socket join rollback with session guard.
   - Verify: review-fixes integration tests.

2. [completed] Add explicit epoll error handling in worker event loop.
   - Files: `src/WorkerThread.cpp`, `src/WorkerThread.h`
   - Outcome: explicit `epoll_wait` negative return handling with recoverable/fatal distinction.
   - Verify: unit test + `mediasoup_tests`.

3. [completed] Add/adjust regression tests for issues 1/2/5.
   - Files: `tests/test_review_fixes_integration.cpp`, `tests/test_review_fixes.cpp`
   - Verify: targeted integration filter + full `./build/mediasoup_tests`.

4. [completed] Update accepted runtime behavior spec.
   - Files: `specs/current/runtime-safety.md`
   - Verify: spec reflects final logic.
