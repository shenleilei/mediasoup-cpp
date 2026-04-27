# Tasks

1. [x] Make missing producer pause/resume fail explicitly.
   - Files: `src/RoomServiceMedia.cpp`, relevant tests
   - Outcome: no silent success on missing producer.
   - Verify: targeted RoomService/regression test

2. [x] Add a byte budget guard to legacy pacing and improve fresh-video fairness.
   - Files: `client/NetworkThread.h`, `client/SenderTransportController.h`, relevant tests if feasible
   - Outcome: legacy pacing no longer emits uncontrolled bursts and queue scans rotate fairly.
   - Verify: build plus impacted targeted runtime regressions

3. [x] Harden deferred join lifetime and netem stale lock cleanup.
   - Files: `src/SignalingServerWs.cpp`, `tests/qos_harness/netem_guard.mjs`, related tests
   - Outcome: deferred signaling captures are lifetime-safe and lock cleanup is less racy.
   - Verify: targeted tests/build

4. [x] Re-run targeted verification and document the result.
   - Files: change docs/tests as needed
   - Outcome: fixes are verified without introducing new regressions.
   - Verify: targeted reruns
