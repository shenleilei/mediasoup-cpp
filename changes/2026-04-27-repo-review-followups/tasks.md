# Tasks

1. [x] Fix Recorder timestamp conversion overflow issue.
   - Files: `src/Recorder.cpp`, `common/ffmpeg/AvTime.h` (or similar)
   - Outcome: Widened timestamp deltas to 64-bit int to prevent overflow.
   - Verify: Check compilation and unit tests.

2. [x] Fix Recording output filename collisions.
   - Files: `src/RoomRecordingHelpers.cpp`
   - Outcome: Use a higher-resolution timestamp or UUID for file names to prevent collisions.
   - Verify: Test recording file generation logic.

3. [x] Fix room routing trust boundary.
   - Files: `src/SignalingServerHttp.cpp`, `src/SignalingRequestDispatcher.h`
   - Outcome: Do not trust `clientIp` provided directly from the client; fallback to socket peer address first.
   - Verify: Test with and without the parameter to ensure correct IP resolution.
