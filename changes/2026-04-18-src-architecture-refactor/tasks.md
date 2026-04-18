## Tasks

1. Baseline and guardrails
   Status:
   - completed
   Outcome:
   - confirm the active `src/` hotspots and lock the refactor scope to production code
   - identify the representative verification suites that cover join, transport, stats, registry, and QoS flows
   Main files or modules touched:
   - `changes/2026-04-18-src-architecture-refactor/*`
   Verification:
   - documentation review only

2. Extract shared transport and RTP stats helpers
   Status:
   - completed
   Outcome:
   - shared helper(s) exist for RTP stats JSON projection and FlatBuffers RTP parameter building
   - `Transport.cpp`, `Producer.cpp`, and `Consumer.cpp` lose duplicated conversion code
   - `src/` and `client/` share QoS schema constants and contract limits where the wire contract is identical
   - `client/CMakeLists.txt` can be reused from a parent build without depending on `CMAKE_SOURCE_DIR`
   Main files or modules touched:
   - `src/Transport.cpp`
   - `src/Producer.cpp`
   - `src/Consumer.cpp`
   - `src/qos/*`
   - `client/CMakeLists.txt`
   - `client/qos/*`
   - new helper header/source files under `src/`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup_lib mediasoup_tests -j1`
   - `cmake -S client -B client/build-refactor-check && cmake --build client/build-refactor-check -j1`
   - targeted unit tests for transport and stats surfaces where available

3. Extract room session and media orchestration helpers
   Status:
   - completed
   Outcome:
   - `RoomService` delegates join/leave/reconnect cleanup and transport/produce/consume flows to focused collaborators
   Main files or modules touched:
   - `src/RoomService.h`
   - `src/RoomServiceLifecycle.cpp`
   - `src/RoomServiceMedia.cpp`
   - `src/RoomMediaHelpers.h`
   - `src/RoomRecordingHelpers.{h,cpp}`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup_integration_tests mediasoup_e2e_tests mediasoup_thread_integration_tests -j1`
   - representative join, produce, consume, and reconnect tests

4. Extract room stats and recording pipeline
   Status:
   - completed
   Outcome:
   - stats collection, stats broadcast scheduling, and recorder-facing hooks are isolated from session/media logic
   Main files or modules touched:
   - `src/RoomServiceStats.cpp`
   - `src/RoomStatsQosHelpers.h`
   - `src/RoomRecordingHelpers.{h,cpp}`
   - `src/RoomCleanupHelpers.h`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup_qos_integration_tests mediasoup_qos_accuracy_tests mediasoup_qos_recording_accuracy_tests -j1`
   - representative stats and recording accuracy tests

5. Extract room QoS and downlink planning pipeline
   Status:
   - completed
   Outcome:
   - QoS ingestion, override lifecycle, and downlink planning are separated from generic room control methods
   Main files or modules touched:
   - `src/RoomServiceDownlink.cpp`
   - `src/RoomServiceStats.cpp`
   - `src/qos/*` as needed
   - `src/RoomDownlinkHelpers.h`
   - `src/qos/QosContract.h`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup_tests mediasoup_qos_integration_tests -j1`
   - representative downlink allocator and QoS integration tests

6. Split signaling edge handlers
   Status:
   - completed
   Outcome:
   - WebSocket dispatch, session bookkeeping, HTTP routes, metrics, and static file serving are decomposed behind helper modules
   Main files or modules touched:
   - `src/SignalingServer.cpp`
   - `src/SignalingServer.h`
   - `src/SignalingServerWs.{h,cpp}`
   - `src/SignalingServerHttp.{h,cpp}`
   - `src/SignalingServerRuntime.cpp`
   - `src/SignalingSocketState.h`
   - `src/SignalingRequestDispatcher.h`
   - `src/StaticFileResponder.h`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup-sfu mediasoup_multinode_tests mediasoup_topology_tests mediasoup_review_fix_tests -j1`
   - representative join, resolve, health, metrics, and recording file-serving tests if available

7. Split bootstrap and header-heavy infrastructure
   Status:
   - completed
   Outcome:
   - `main.cpp` becomes a thin bootstrap entry
   - `RoomRegistry` and `WorkerThread` implementation move out of headers where safe
   Main files or modules touched:
   - `src/main.cpp`
   - `src/MainBootstrap.{h,cpp}`
   - `src/RuntimeDaemon.{h,cpp}`
   - `src/RoomRegistry.{h,cpp}`
   - `src/WorkerThread.{h,cpp}`
   Verification:
   - `cmake --build build-refactor-check --target mediasoup-sfu mediasoup_tests mediasoup_multinode_tests mediasoup_topology_tests -j1`

8. Final pass and delivery review
   Status:
   - completed
   Outcome:
   - docs reflect the refactored module boundaries
   - remaining debt and deferred items are documented explicitly
   Main files or modules touched:
   - `README.md`
   - `docs/*`
   - change documents
   Verification:
   - run the agreed build and targeted tests from prior phases
   - review against `docs/aicoding/DELIVERY_CHECKLIST.md`
