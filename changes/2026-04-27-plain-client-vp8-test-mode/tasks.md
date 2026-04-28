# Tasks

1. [x] Add explicit plain-client codec selection with H264 default and VP8 test mode.
   - Files: `client/PlainClientApp.*`, related client runtime files
   - Outcome: automated harness can request VP8 while default manual behavior remains H264.
   - Verify: plain-client starts and logs selected codec mode.

2. [x] Make plain publish server path create matching VP8 or H264 producers.
   - Files: `src/SignalingRequestDispatcher.h`, `src/RoomService.h`, `src/RoomServiceMedia.cpp`
   - Outcome: plain-client VP8 mode can publish a browser-consumable VP8 producer.
   - Verify: targeted plain publish path test / black-box harness.

3. [x] Implement codec-aware RTP send path in plain-client for VP8 mode.
   - Files: client send / packetization / RTCP related modules
   - Outcome: plain-client can send valid VP8 RTP and respond to keyframe requests sufficiently for the interop gate.
   - Verify: plain-client -> web black-box rendering.

4. [x] Add black-box interop harness using the real public demo page.
   - Files: `tests/qos_harness/browser_public_interop.mjs`
   - Outcome: verifies `web -> web` and `plain-client -> web` real rendering.
   - Verify: direct harness run.

5. [x] Wire the interop harness into `scripts/run_qos_tests.sh` and therefore `run_all_tests.sh`.
   - Files: `scripts/run_qos_tests.sh`
   - Outcome: the QoS/browser regression entry path always covers video interoperability first.
   - Verify: targeted `run_qos_tests.sh` invocation and delegated `run_all_tests.sh` group.
