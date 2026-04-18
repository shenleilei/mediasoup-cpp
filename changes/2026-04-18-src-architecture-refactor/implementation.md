## Implementation Plan

This change is `src/` first, with `client/` included only where there is direct overlap in build infrastructure or QoS wire-contract definitions. Existing test-harness refactors are not the execution focus for this change.

## Progress Update

Completed so far:

- shared QoS contract constants extracted to `src/qos/QosContract.h`
- `client/CMakeLists.txt` rooted on `CMAKE_CURRENT_LIST_DIR` so it works standalone and as a parent-subdirectory include
- `src/` and `client/` QoS schema and max-track constants unified onto the shared contract header
- shared RTP stats JSON helper extracted for `Producer` and `Consumer`
- shared FlatBuffers RTP parameter and mapping builder extracted for `Transport`
- static file/path resolution and streaming helper extracted from `SignalingServer.cpp` into `src/StaticFileResponder.h`
- `RoomService` media auto-subscribe and existing-producer consumption paths extracted into `src/RoomMediaHelpers.h`
- `SignalingServer` request parsing and room-service method dispatch extracted into `src/SignalingRequestDispatcher.h`
- `RoomService` stats/QoS JSON assembly, default QoS policy, override-clear payloads, and peer-key conventions extracted into `src/RoomStatsQosHelpers.h`
- `RoomService` downlink planning input collection, stale-demand merge, subscriber-plan application, producer owner/track resolution, and track-override bookkeeping extracted into `src/RoomDownlinkHelpers.h`
- `RoomService` recording setup / recorder cleanup / old-recording-dir cleanup extracted into `src/RoomRecordingHelpers.{h,cpp}`
- `RoomService` room-scoped state cleanup extracted into `src/RoomCleanupHelpers.h`, and repeated room-destroy flow collapsed into one internal path
- `SignalingServer` socket/session state, ws room indexes, stale-session validation, join registration, close unregister, and notify/broadcast delivery extracted into `src/SignalingSocketState.h`
- `WorkerThread` heavy method implementations migrated from `src/WorkerThread.h` into `src/WorkerThread.cpp`
- `main.cpp` bootstrap wiring reduced by extracting runtime option parsing / worker-thread pool creation into `src/MainBootstrap.{h,cpp}`
- daemonization and startup-status plumbing extracted from `main.cpp` into `src/RuntimeDaemon.{h,cpp}`
- `SignalingServer.h` reduced to forward declarations for heavy runtime types instead of pulling full implementations into every include chain
- `RoomRegistry` heavy Redis/cache/subscriber implementation migrated from `src/RoomRegistry.h` into `src/RoomRegistry.cpp`, with header dependencies reduced to declarations
- `RoomService` join/leave, room health, idle-room GC, and room-destroy cleanup migrated into `src/RoomServiceLifecycle.cpp`
- `RoomService` transport, produce/consume, plain transport, consumer control, and auto-record orchestration migrated into `src/RoomServiceMedia.cpp`
- `RoomService` downlink / publisher-supply planning member implementations migrated into `src/RoomServiceDownlink.cpp`
- `RoomService` stats / QoS / broadcast member implementations migrated into `src/RoomServiceStats.cpp`
- `SignalingServer` HTTP routes migrated into `src/SignalingServerHttp.cpp`
- `SignalingServer` websocket route registration and socket/session orchestration migrated into `src/SignalingServerWs.cpp`
- `SignalingServer` runtime snapshot / dispatch / registry worker machinery migrated into `src/SignalingServerRuntime.cpp`

Current remaining work:

- keep `SignalingServer.cpp` as a thin assembly unit and avoid letting new edge logic drift back into it
- keep `RoomService.h` as the stable facade while trimming any residual helper-heavy include coupling around the split slices when follow-up behavior work touches them
- decide later whether `Recorder.h` or `RoomManager.h` needs a similar split after the current module boundaries have stabilized
- keep docs, specs, and change records aligned with the split runtime layout

## Execution Order

### Wave 1

Establish helper seams in the media adapter layer before touching orchestration:

- extract shared RTP stats to JSON helper
- extract shared FlatBuffers RTP builder helper
- extract shared QoS contract constants reused by `src/` and `client/`
- fix `client` CMake path rooting so `client` can be included from a parent build
- keep behavior and signatures unchanged

Reason:

- smallest write set
- lowest regression risk
- reduces duplicate logic before higher-level file splits

### Wave 2

Break `RoomService` into private collaborators while keeping `RoomService` as the public facade:

- session and reconnect cleanup
- transport and media operations
- stats and recorder pipeline

Reason:

- preserves call sites
- lets the refactor move logic incrementally
- avoids touching `SignalingServer` and `WorkerThread` too early

### Wave 3

Extract QoS and downlink planning from `RoomService` into dedicated room-domain coordinators.

Reason:

- QoS code already has strong local domain types under `src/qos/`
- the main problem is orchestration placement, not algorithm design

### Wave 4

Refactor `SignalingServer` into composition of narrower edge modules:

- WebSocket request parser and validator
- method dispatcher
- HTTP route handlers
- static file responder
- metrics/health snapshot helpers

Reason:

- current file mixes unrelated concerns
- once room-domain seams exist, signaling dispatch becomes much easier to thin out

### Wave 5

Refactor runtime bootstrap and move header-heavy implementations to `.cpp` files:

- runtime config and daemon helpers
- registry implementation split
- worker-thread implementation split

Reason:

- these are broad file moves and should be done after behavior-heavy logic has stable seams

## Non-Negotiable Invariants During Execution

- preserve same-room-to-same-worker-thread dispatch
- preserve stale-session rejection
- preserve local degrade behavior when Redis is unavailable
- preserve join/reconnect cleanup semantics
- preserve QoS planner cadence and deferred wake-up behavior

## Expected First Concrete Changes

1. Add a shared helper for producer and consumer RTP stats serialization.
2. Add a shared helper for `Transport.cpp` FlatBuffers RTP parameter and mapping building.
3. Add a shared QoS contract header for schema constants and hard limits used by both `src` and `client`.
4. Replace duplicated code in:
   - `src/Producer.cpp`
   - `src/Consumer.cpp`
   - `src/Transport.cpp`
   - `src/qos/*`
   - `client/qos/*`

## Verification Rhythm

After each wave:

- build the affected targets first
- run the smallest representative test slice that covers the changed subsystem
- only then start the next wave

Recommended representative verification set:

- `mediasoup_tests`
- `mediasoup_integration_tests`
- `mediasoup_e2e_tests`
- `mediasoup_qos_integration_tests`
- `mediasoup_thread_integration_tests`
- `mediasoup_multinode_tests`
- `mediasoup_topology_tests`
- `mediasoup_review_fix_tests`

## Deferred Until After Src Stabilization

- cleanup-only test refactors
- broad naming sweeps
- optional `Recorder.h` split if not required by the room stats extraction
