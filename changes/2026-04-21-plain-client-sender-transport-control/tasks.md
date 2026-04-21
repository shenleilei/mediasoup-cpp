# Tasks

1. [x] Introduce `SenderTransportController` as a dedicated send-side module
   Outcome:
   - `NetworkThread` delegates pacing/queue/send-result policy to a dedicated controller boundary
   - raw UDP socket ownership remains in the network thread
   Files:
   - new `client/SenderTransportController.*` or equivalent
   - `client/NetworkThread.h`
   Verification:
   - `cmake --build build --target mediasoup_thread_integration_tests mediasoup_qos_unit_tests`
   - `cmake --build client/build --target plain-client`

2. [x] Add explicit send-result classification and success-only accounting
   Outcome:
   - send paths distinguish successful send, temporary backpressure, and hard failure
   - only successful sends update RTP/RTCP accounting and retransmission state
   Files:
   - `client/SenderTransportController.*`
   - `client/RtcpHandler.h`
   - `client/PlainClientApp.cpp`
   - `client/PlainClientLegacy.cpp`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='ClientQos*:ThreadModel*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='*Threaded*:*NetworkThread*'`

3. [x] Replace fixed packet-count video pacing with byte-budget pacing driven by transport hints
   Outcome:
   - queued video flush uses byte budget rather than fixed burst packet count
   - control/QoS path can update current transport hints through `NetworkControlCommand::TrackTransportConfig`
   Files:
   - `client/ThreadTypes.h`
   - `client/ThreadedControlHelpers.h`
   - `client/PlainClientThreaded.cpp`
   - `client/SenderTransportController.*`
   - `client/NetworkThread.h`
   Verification:
   - `./build/mediasoup_thread_integration_tests --gtest_filter='*Threaded*:*NetworkThread*'`
   - `./scripts/run_qos_tests.sh cpp-threaded`

4. [x] Keep phase-1 send policies explicit per packet class
   Outcome:
   - control/RTCP, audio RTP, video retransmission, and fresh video follow explicit distinct policies
   - audio uses a bounded deadline-aware pending queue in phase 1 rather than sharing the video queue
   - retransmission is prioritized above fresh video
   Files:
   - `client/SenderTransportController.*`
   - `client/RtcpHandler.h`
   - `client/NetworkThread.h`
   Verification:
   - `./build/mediasoup_qos_unit_tests`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='*Threaded*:*NetworkThread*'`
   - deterministic sender-transport tests cover control/audio/retransmission/fresh-video `Sent/WouldBlock/HardError` behavior and explicitly prove control independent-from-backlog scheduling plus `audio > fresh-video` and `retransmission > fresh-video` ordering

5. [x] Add rollback/fallback control and required observability
   Outcome:
   - implementation has a bounded rollback path that can disable the new transport controller during validation before merge
   - required counters/logs for `WouldBlock`, hard errors, audio deadline drops, and queued video discard/retention are present
   Files:
   - `client/SenderTransportController.*`
   - `client/NetworkThread.h`
   - affected docs under `docs/`
   Verification:
   - `./build/mediasoup_qos_unit_tests`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='*Threaded*:*NetworkThread*'`
   - `./scripts/run_qos_tests.sh cpp-threaded cpp-client-harness:threaded_publish_snapshot`
   - deterministic sender-transport tests assert observability counters/logs for each packet class and verify fallback enablement does not break plain-client startup/smoke behavior

6. [x] Add deterministic transport-control regression coverage and document remaining non-goals
   Outcome:
   - first-phase transport control behavior is regression-tested with fake clock/fake sender coverage, not just timing-sensitive socket behavior
   - docs remain explicit that full TWCC/GCC parity is future work
   Files:
   - `tests/test_thread_integration.cpp`
   - new focused transport-control tests as needed
   - affected docs under `docs/`
   Verification:
   - `./scripts/run_qos_tests.sh client-js cpp-unit cpp-client-harness cpp-threaded`
   - `./scripts/run_qos_tests.sh cpp-client-harness:threaded_publish_snapshot cpp-client-harness:threaded_audio_stats_smoke cpp-client-harness:threaded_multi_video_budget`
   - docs reflect implementation truthfully

7. [x] Expand deterministic TWCC-estimate branch coverage
   Outcome:
   - transport feedback handling covers additive increase, moderate decrease, severe decrease, clamp-to-min, clamp-to-max, and malformed feedback no-op behavior
   - estimate changes are asserted to affect effective pacing bitrate
   Files:
   - `tests/test_thread_integration.cpp`
   - `tests/test_thread_model.cpp`
   - `client/NetworkThread.h` (only if testability seam is needed)
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*:SenderTransportControllerTest*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCcFeedback*'`

8. [x] Add bitrate-allocation matrix tests for packet-class scheduling
   Outcome:
   - deterministic multi-bitrate matrix proves packet-class scheduling contract:
     - control independent from backlog
     - audio preferred over fresh video
     - retransmission preferred over fresh video
     - fresh video availability increases monotonically with higher effective bitrate
   Files:
   - `tests/test_thread_model.cpp`
   - optional new fixture/helper under `tests/`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='SenderTransportControllerTest.BitrateAllocation*'`

9. [x] Add end-to-end TWCC feedback smoke and pause-no-video-leak regression
   Outcome:
   - pause/audio-only state change guarantees no post-ack video RTP leakage including retransmission backlog
   - threaded plain-publish smoke verifies real worker-generated transport-cc feedback in threaded publish flow
   - direct plain-transport diagnostics prove transport-cc header-extension registration, RTP reception, and transport-cc feedback emission for both VP8 and connected H264 sender sockets
   Files:
   - `tests/test_thread_integration.cpp`
   - `tests/test_review_fixes.cpp` (if reused for regression assertions)
   Verification:
   - `./build/mediasoup_thread_integration_tests --gtest_filter='*TWCC*:*Pause*'`
   - `./build/mediasoup_qos_integration_tests --gtest_filter='QosIntegrationTest.PlainPublish*'`
