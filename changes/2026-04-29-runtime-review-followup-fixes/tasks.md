# Tasks

1. [x] Guard join finalization against missing worker.
   Files: `src/SignalingServerWs.cpp`, `src/SignalingSocketState.h`, relevant tests
   Outcome: joined socket state is committed only when worker availability is confirmed at completion time.
   Verification: targeted unit coverage plus existing join lifecycle integration target

2. [x] Prevent incomplete room-registry full snapshots from replacing cache state.
   Files: `src/RoomRegistry.h`, `src/RoomRegistrySync.cpp`, `tests/test_room_registry_sync.cpp`
   Outcome: incomplete scan/fetch results are detected and skipped without cache replacement.
   Verification: targeted registry sync test in `mediasoup_review_fix_tests`

3. [x] Validate transport connect responses and surface explicit failure.
   Files: `src/WebRtcTransport.cpp`, `src/PlainTransport.h`, `src/PipeTransport.cpp`, shared helper(s), `tests/test_review_fixes.cpp`
   Outcome: malformed or mismatched connect responses throw instead of returning fake success.
   Verification: targeted unit tests in `mediasoup_tests`

4. [x] Retry recoverable channel write interruptions.
   Files: `src/Channel.h`, `src/Channel.cpp`, `tests/test_review_fixes.cpp`
   Outcome: recoverable write interruption no longer closes the channel.
   Verification: targeted unit test in `mediasoup_tests`

5. [x] Update accepted behavior docs and run targeted verification.
   Files: `specs/current/runtime-safety.md`, change docs as needed
   Outcome: docs match final runtime behavior and verification results are recorded truthfully.
   Verification: `mediasoup_tests` and `mediasoup_review_fix_tests`
