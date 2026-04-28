# Implementation Notes

## Finding Mapping

### High

- retransmission starvation of fresh video:
  fixed in `client/SenderTransportController.h`
  verified by `SenderTransportControllerTest.RetransmissionIsSentBeforeFreshVideo` and
  `SenderTransportControllerTest.BitrateAllocationPrioritizesRetransmissionWithoutStarvingFreshVideoAcrossBitrates`
  accepted behavior updated in `specs/current/plain-client-send-side-bwe.md`

- RoomRegistry heartbeat command mutex hold time:
  narrowed by moving `registerNode()` / `evictDeadNodes()` to self-owned command critical sections
  in `src/RoomRegistry.cpp`, `src/RoomRegistryConnection.cpp`, and `src/RoomRegistrySync.cpp`

### Medium

- FFmpeg null-guard inconsistency:
  fixed in `common/ffmpeg/BitstreamFilter.cpp`, `Decoder.cpp`, and `Encoder.cpp`
  verified by targeted `test_common_ffmpeg.cpp` misuse tests

- Consumer producer-close internal close semantics:
  `@close` now carries an explicit terminal reason string
  verified by `ConsumerCloseReasonTest.ProducerCloseCarriesTerminalReasonOnInternalCloseEvent`

- EventEmitter `off(id)` complexity and cleanup-exception hiding:
  `EventEmitter` now keeps an ID-to-event index and exposes `emitChecked()` for cleanup-critical events
  verified by `EventEmitterTest.*`

- Channel request ID wrap-around collision:
  fixed in `src/Channel.cpp`
  verified by `ChannelRequestIdTest.WraparoundSkipsPendingRequestIds`

- Router / Transport channel listener ownership:
  direct listener IDs now live on `Transport`, `Producer`, and `Consumer`
  cleanup no longer relies on broad `off(eventName)` in those paths

- Transport duplicated cleanup drift:
  shared `cleanupOwnedEntities()` / `emitTerminalClose()` logic in `src/Transport.cpp`

- Producer / Consumer copy-move API misuse:
  copy / move disabled in headers

- RoomRegistry dead-node eviction append failure:
  explicit `appendCommand()` failure handling in `src/RoomRegistrySync.cpp`

- RoomRegistry subscriber jitter fallback:
  deterministic non-zero fallback in `src/RoomRegistryPubSub.cpp`

- Recorder pending audio silent drops:
  warning + counter in `src/Recorder.cpp` / `src/Recorder.h`

- RTCP SR wall-clock rollback risk:
  monotonic-anchor NTP derivation in `client/RtcpHandler.h`
  verified by `RtcpHandler.NtpClockDoesNotMoveBackwardAcrossCalls`

- QoS override expiry during paused tracks:
  `housekeep()` path in `client/qos/QosController.h`
  invoked from both threaded and legacy plain-client control loops
  verified by `ClientQosControllerTest.HousekeepExpiresOverridesWithoutNewSamples`

- destroyedRooms_ thread boundary:
  explicit main-thread comment in `src/SignalingServer.h`

### Architecture / Verification

- worker crash / room recovery strategy:
  accepted behavior documented in `specs/current/runtime-safety.md`
  existing integration coverage remains:
  `IntegrationTest.WorkerCrashRecoverySendsServerRestart`
  `IntegrationTest.WorkerRespawnAllowsNewRooms`

- browser demo / C++ plain-client end-to-end handling:
  `public/qos-demo.js` now treats `serverRestart` as an automatic recovery event:
  clear pending WS requests, preserve live local capture when possible, reconnect/rejoin with retry, and republish automatically if the user was already publishing.
  `client/PlainClientApp.cpp` now treats `serverRestart` as a session restart trigger:
  stop the active send loop, tear down session resources, retry session initialization with bounded backoff, and resume the selected run mode automatically.
  verification now includes:
  - `ThreadedPlainPublishIntegrationTest.PlainClientRecoversAfterServerRestart`
  - `tests/qos_harness/browser_public_interop.mjs`
    - `public-demo web->web recovers after worker restart`
    - `public-demo plain-client->web recovers after worker restart`

- risk-to-test mapping:
  this change doc now maps each corrected runtime risk to the specific targeted automated checks used in this pass

## Verification Commands Run

- `cmake --build build --target mediasoup_tests mediasoup_qos_unit_tests mediasoup_review_fix_tests mediasoup-sfu -j1`
- `./build/mediasoup_tests --gtest_filter='EventEmitterTest.*:DecoderSharedTest.ReceiveFrameRejectsNullOutputFrame:EncoderSharedTest.ReceivePacketRejectsNullOutputPacket:BitstreamFilterSharedTest.CreateRejectsNullCodecParameters:ChannelRequestIdTest.*:ConsumerCloseReasonTest.*:SocketPendingJoinStateTest.*:ChannelWriteRetryTest.*:TransportConnectValidationTest.*'`
- `./build/mediasoup_qos_unit_tests --gtest_filter='RtcpHandler.NtpClockDoesNotMoveBackwardAcrossCalls:SenderTransportControllerTest.RetransmissionIsSentBeforeFreshVideo:SenderTransportControllerTest.BitrateAllocationPreservesAudioPriorityWithoutStarvingFreshVideo:SenderTransportControllerTest.BitrateAllocationPrioritizesRetransmissionWithoutStarvingFreshVideoAcrossBitrates:ClientQosControllerTest.HousekeepExpiresOverridesWithoutNewSamples'`
- `./build/mediasoup_review_fix_tests --gtest_filter='ReviewFixIntegration.RepeatedJoinOnSameSocketRejected:ReviewFixIntegration.EarlyCloseJoinDoesNotLeaveGhostParticipants:RoomRegistrySyncIntegration.*'`
- `./build/mediasoup_thread_integration_tests --gtest_filter='ThreadedPlainPublishIntegrationTest.PlainClientRecoversAfterServerRestart'`
- `node tests/qos_harness/browser_public_interop.mjs`
