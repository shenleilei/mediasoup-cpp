# Tasks

1. [x] Rework sender pacing fairness under retransmission backlog.
   Files: `client/SenderTransportController.h`, `tests/test_thread_model.cpp`, `specs/current/plain-client-send-side-bwe.md`
   Outcome: retransmission remains prioritized but cannot indefinitely starve fresh video.
   Verification: targeted sender-transport bitrate-allocation tests

2. [x] Narrow registry heartbeat maintenance and harden pubsub fallback behavior.
   Files: `src/RoomRegistry.cpp`, `src/RoomRegistrySync.cpp`, `src/RoomRegistryPubSub.cpp`, relevant tests/specs
   Outcome: heartbeat no longer holds one outer command lock across maintenance phases, append failure is explicit, jitter fallback is deterministic.
   Verification: targeted registry tests plus review-fix suite

3. [x] Standardize FFmpeg null-guard behavior.
   Files: `common/ffmpeg/BitstreamFilter.cpp`, `common/ffmpeg/Decoder.cpp`, `common/ffmpeg/Encoder.cpp`, `tests/test_common_ffmpeg.cpp`
   Outcome: wrapper misuse fails fast consistently.
   Verification: targeted FFmpeg unit tests

4. [x] Refactor lifecycle/event cleanup semantics and request-id ownership.
   Files: `src/EventEmitter.h`, `src/Transport.cpp`, `src/Transport.h`, `src/Router.cpp`, `src/Producer.cpp`, `src/Producer.h`, `src/Consumer.cpp`, `src/Consumer.h`, `src/Channel.cpp`, `src/Channel.h`, relevant tests
   Outcome: internal close reasons are explicit, listener ownership is direct, request ID wrap-around is collision-safe, Producer/Consumer cannot be copied or moved.
   Verification: targeted stability/review-fix unit tests

5. [x] Add remaining operational guardrails and accepted-behavior docs.
   Files: `src/Recorder.cpp`, `src/Recorder.h`, `client/RtcpHandler.h`, `client/qos/QosController.h`, `client/PlainClientThreaded.cpp`, `client/PlainClientLegacy.cpp`, `src/SignalingServer.h`, `specs/current/runtime-safety.md`, related tests
   Outcome: recorder, RTCP, QoS, worker-crash behavior, and thread-boundary assumptions are explicit and observable.
   Verification: targeted unit/tests plus main binary build
