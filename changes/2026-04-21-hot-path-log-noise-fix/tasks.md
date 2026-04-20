# Tasks

1. [completed] Demote normal `/api/resolve` lifecycle tracing from warning to debug.
   - Files: `src/SignalingServerHttp.cpp`
   - Verify: build `mediasoup-sfu`

2. [completed] Demote normal `RoomRegistry` maintenance and resolve tracing from warning to debug while preserving failure warnings.
   - Files: `src/RoomRegistry.cpp`
   - Verify: build `mediasoup-sfu`

3. [completed] Update accepted behavior docs and run verification.
   - Files: `specs/current/runtime-safety.md`
   - Verify: `./build/mediasoup_tests`
