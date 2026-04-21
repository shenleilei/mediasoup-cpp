# Tasks

1. [x] Port LiveKit support utilities and data structures
   Outcome:
   - `client/ccutils/*` and `client/sendsidebwe/*` contain the main LiveKit-aligned data structures and helper logic
   Verification:
   - `cmake --build build --target mediasoup_qos_unit_tests mediasoup_thread_integration_tests`

2. [x] Port packet tracking and TWCC feedback time-order logic
   Outcome:
   - out-of-order feedback detection and reference-time wrap handling are available on the supported path
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*:TwccFeedbackTracker.*:PacketTracker.*'`

3. [x] Port packet-group / traffic-stats / congestion-detector core
   Outcome:
   - packet-group, traffic-stats, congestion-state, and capacity-estimation logic exist in C++
   Verification:
   - `cmake --build build --target mediasoup_qos_unit_tests`

4. [x] Integrate `NetworkThread` with the new send-side BWE path
   Outcome:
   - `NetworkThread` allocates transport-wide sequence numbers through the new BWE path
   - parsed TWCC feedback is fed into the new BWE path
   - the published transport estimate comes from the LiveKit-aligned implementation
   Verification:
   - `cmake --build client/build --target plain-client`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCcFeedback*:ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender'`

5. [x] Add correctness tests for out-of-order feedback, reference-time wrap, and aggregate-target semantics
   Outcome:
   - targeted tests exist for TWCC ordering and aggregate-target publishing behavior
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*:TwccFeedbackTracker.*:PacketTracker.*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCcFeedback*:NetworkThreadIntegration.AggregateTargetIncreaseRaisesEffectivePacingWithoutMutatingEstimate'`

6. [x] Wire probe lifecycle and RTP padding probe sends into the main path
   Outcome:
   - probe cluster start, padding probe emission, and observer-based completion exist on the main path
   Verification:
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.ProbeSendsPaddingRtpWhenTargetExceedsEstimate'`

7. [x] Add positive and negative probe completion tests
   Outcome:
   - send-side probe finalize paths have direct unit coverage for successful, congesting, and incomplete cases
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='SendSideBwe.*'`

8. [x] Fix transport-wide sequence stability for queued packet retries
   Outcome:
   - a queued RTP packet keeps the same transport-wide sequence number across local retries after `WouldBlock`
   - only successful sends are committed to the BWE sent-packet tracker
   Files:
   - `client/NetworkThread.h`
   - `client/SenderTransportController.h`
   - `client/sendsidebwe/SendSideBwe.h`
   - `client/sendsidebwe/PacketTracker.h`
   Verification:
   - add unit tests for retry-after-`WouldBlock` sticky sequence behavior
   - `./build/mediasoup_qos_unit_tests --gtest_filter='*WouldBlock*:*TransportCc*:*PacketTracker*'`

9. [x] Isolate probe RTP from ordinary video retransmission bookkeeping
   Outcome:
   - probe RTP does not enter ordinary `RtpPacketStore`
   - probe RTP is not selected by ordinary media NACK retransmission
   - probe RTP does not inflate ordinary video octet accounting
   Files:
   - `client/RtcpHandler.h`
   - `client/NetworkThread.h`
   Verification:
   - add unit/integration regression tests for probe-store isolation and probe-vs-NACK behavior
   - `./build/mediasoup_qos_unit_tests --gtest_filter='*Probe*:*RtcpHandler*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.Probe*:NetworkPause.*'`

10. [x] Wire probe early-stop on goal reached
   Outcome:
   - active probe clusters stop early when `ProbeClusterIsGoalReached()` becomes true
   - probe completion and finalize paths remain one-shot and deterministic
   Files:
   - `client/NetworkThread.h`
   - `client/sendsidebwe/CongestionDetector.h`
   Verification:
   - add positive and negative early-stop tests
   - `./build/mediasoup_qos_unit_tests --gtest_filter='SendSideBwe.*Probe*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.Probe*'`

11. [x] Align probe-trigger math with LiveKit default padding-probe policy
   Outcome:
   - probe goal construction uses the sender-side equivalent of LiveKit's default padding-probe math
   - default configuration matches LiveKit defaults for `ProbeOveragePct` and `ProbeMinBps`
   Files:
   - `client/NetworkThread.h`
   - `changes/2026-04-21-livekit-aligned-send-side-bwe/design.md`
   - `changes/2026-04-21-livekit-aligned-send-side-bwe/requirements.md`
   Verification:
   - add unit tests for probe goal construction and no-probe conditions
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.Probe*'`

12. [x] Add observability and report-pipeline support for transport effectiveness review
   Outcome:
   - executable validation distinguishes deterministic correctness proof from black-box effectiveness proof
   - the harness/report path exposes the white-box transport metrics that are needed for probe / queue / retransmission effect evaluation
   Files:
   - `client/ThreadTypes.h`
   - `client/NetworkThread.h`
   - `tests/qos_harness/cpp_client_runner.mjs`
   - `tests/qos_harness/run_twcc_ab_eval.mjs`
   - affected generated-report renderers/templates as needed
   Verification:
   - add/report transport-estimate metrics in harness output
   - run targeted A/B evaluation with updated raw/group report output
   - `node tests/qos_harness/run_twcc_ab_eval.mjs --repetitions=2`

13. [ ] Close delivery gaps and update accepted behavior
   Outcome:
   - change docs, rollout notes, and accepted behavior reflect the final implementation truthfully
   - residual differences from LiveKit are documented explicitly
   Files:
   - `changes/2026-04-21-livekit-aligned-send-side-bwe/*`
   - `specs/current/*` as needed
   Verification:
   - `cmake --build build --target mediasoup_qos_unit_tests mediasoup_thread_integration_tests`
   - `cmake --build client/build --target plain-client`
   - `./build/mediasoup_qos_unit_tests`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCcFeedback*:NetworkThreadIntegration.Probe*:ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender:NetworkPause.*'`
   - `./scripts/run_qos_tests.sh cpp-threaded`
   - `./scripts/run_qos_tests.sh cpp-client-harness:threaded_generation_switch cpp-client-harness:threaded_multi_video_budget`
