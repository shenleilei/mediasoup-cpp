## Reverse Analysis

### Current Runtime Layers

The production runtime is already organized around a useful shape:

1. `src/main.cpp`
   Loads config, daemonizes, initializes logging, sets up geo routing, constructs the registry, creates worker threads, and starts the signaling server.
2. `src/SignalingServer.cpp`
   Owns the uWebSockets edge, socket session state, room-to-thread dispatch, WebSocket method handling, HTTP APIs, metrics, health, and static file serving.
3. `src/WorkerThread.h`
   Owns one event loop thread, worker processes, epoll, timers, queued tasks, room manager, and room service.
4. `src/RoomService.cpp`
   Owns almost all room-domain orchestration: join/leave, transport creation, produce/consume, recording hooks, stats collection, QoS ingestion, room QoS broadcast, and downlink planning.
5. `src/Transport.cpp`, `src/Producer.cpp`, `src/Consumer.cpp`, `src/Router.cpp`, `src/Worker.cpp`, `src/Channel.cpp`
   Wrap mediasoup worker IPC and runtime objects.
6. `src/RoomRegistry.h`, `src/GeoRouter.h`
   Handle multi-node routing and geo-aware selection.
7. `src/qos/*`
   Contain mostly focused algorithm and state components.

### Current Hotspots

- `src/RoomService.cpp` is the main god object. It mixes room lifecycle, media orchestration, stats, recording, QoS, and planner scheduling.
- `src/SignalingServer.cpp` mixes edge protocol code with infrastructure code and file-serving utilities.
- `src/main.cpp` mixes bootstrap policy and process-runtime mechanics.
- `src/RoomRegistry.h` and `src/WorkerThread.h` keep large implementations in headers, increasing compile coupling and hiding ownership boundaries.
- `src/Transport.cpp`, `src/Producer.cpp`, and `src/Consumer.cpp` each contain JSON and FlatBuffers translation logic that should live in shared adapter helpers.

### Observed Invariants

- same `roomId` must stay on the same `WorkerThread`
- room logic is expected to run serially on a worker-thread event loop
- reconnect replaces the prior session and stale requests are rejected by session id
- registry failures degrade to local behavior instead of hard-failing room operations
- QoS and downlink planning are best-effort control loops and must not block the main edge loop

## Refactor Strategy

The refactor should proceed in layers from lowest-risk extraction to highest-risk orchestration split.

### Phase 1: Shared Adapter Extraction

Goal: remove low-risk duplication without changing runtime ownership.

Changes:

- extract shared RTP stats to JSON conversion from `Producer::getStats()` and `Consumer::getStats()`
- extract shared notification argument decoding used by transport-owned child entities
- extract FlatBuffers RTP parameter and mapping builders from `Transport.cpp`
- extract shared QoS protocol constants used by both `src/` and `client/`
- fix `client` build wiring so repository-level helpers can be reused from standalone or parent builds

Why first:

- this is low-risk
- it reduces duplication immediately
- it creates helper seams before larger file splits
- it gives `client/` a safe path to reuse `src` contract headers without pulling in server-only runtime code

### Phase 2: Split RoomService By Responsibility

Goal: turn `RoomService` into a coordinator instead of a feature bucket.

Proposed internal modules:

- `RoomSessionService`
  - join
  - leave
  - reconnect cleanup
  - room lifecycle hooks
- `RoomMediaService`
  - create/connect transport
  - plain transport workflows
  - produce/consume
  - consumer control operations
- `RoomStatsService`
  - collectPeerStats
  - broadcastStats
  - recorder attachment
  - old recording cleanup
- `RoomQosService`
  - clientStats ingestion
  - room/peer QoS notifications
  - override lifecycle
- `RoomDownlinkPlannerService`
  - downlink stats ingestion
  - dirty-room queueing
  - deferred planning
  - publisher supply plan application

`RoomService` remains the public facade initially and delegates to extracted collaborators.

### Phase 3: Split SignalingServer Edge Concerns

Goal: isolate protocol edge code from infrastructure and HTTP utilities.

Proposed internal modules:

- `WsSessionState` or `SocketSessionRegistry`
  - per-socket identity
  - session id allocation and stale-session validation
- `WsMethodDispatcher`
  - parse validated request payloads into room-service calls
- `HttpRouteHandlers`
  - `/api/resolve`
  - `/healthz`
  - `/metrics`
  - recordings index and static asset serving
- `StaticFileResponder`
  - path resolution
  - content-type mapping
  - streaming helpers

This keeps `SignalingServer` as composition and lifecycle glue instead of a mixed transport and utility file.

### Phase 4: Bootstrap And Registry Split

Goal: reduce startup complexity and make runtime policy explicit.

Proposed modules:

- `RuntimeConfigLoader`
  - config file read
  - CLI override merge
- `DaemonRuntime`
  - daemonize
  - pid file
  - startup notification pipe
- `RuntimeBootstrap`
  - geo DB resolution
  - registry construction
  - worker-thread allocation plan

For the registry:

- move `RoomRegistry` implementation to `RoomRegistry.cpp`
- separate Redis command helpers, cache state, and subscriber loop helpers

### Shared Contract Boundary

Not every `client/` QoS type should move into `src/`, because the plain client has controller, planner, and probe state that is not server-side logic.

The reusable slice is narrower:

- schema constants
- contract-level hard limits
- JSON field validation and serialization helpers where the wire contract is identical

That shared slice should live in a contract-oriented header layer that `src/` and `client/` can both include.

### Phase 5: Header Implementation Reduction

Goal: reduce rebuild cost and improve module clarity after behavior is stabilized.

Primary candidates:

- move `RoomRegistry` implementation out of `src/RoomRegistry.h`
- move `WorkerThread` implementation out of `src/WorkerThread.h`
- optionally move `RoomManager` implementation out of `src/RoomManager.h`
- defer `Recorder.h` split unless Phase 2 proves it blocks `RoomStatsService`

## Target Architecture

```text
main.cpp
  -> RuntimeConfigLoader
  -> RuntimeBootstrap
  -> SignalingServer

SignalingServer
  -> SocketSessionRegistry
  -> WsMethodDispatcher
  -> HttpRouteHandlers
  -> WorkerThreadPool

WorkerThread
  -> WorkerManager
  -> RoomManager
  -> RoomService facade

RoomService facade
  -> RoomSessionService
  -> RoomMediaService
  -> RoomStatsService
  -> RoomQosService
  -> RoomDownlinkPlannerService

Media adapters
  -> Router / Transport / Producer / Consumer / Worker / Channel

Infra
  -> RoomRegistry
  -> GeoRouter
  -> qos/*
```

## Sequencing Constraints

1. Extract helpers before moving logic across files.
2. Keep existing public class names and method signatures until call sites are stable.
3. Do not refactor tests as a primary stream during `src/` extraction.
4. Run build and targeted tests after each phase; do not batch all structural moves into one unverified sweep.

## Risks And Mitigations

- Risk: hidden thread-affinity assumptions inside `RoomService`.
  Mitigation: keep `RoomService` as facade first and move logic behind delegating calls.

- Risk: regressions in reconnect, auto-subscribe, or QoS planner timing.
  Mitigation: refactor by feature slice and run representative integration tests after each slice.

- Risk: merge conflict with existing dirty worktree, especially `src/qos/DownlinkAllocator.cpp`.
  Mitigation: avoid unrelated edits and isolate each refactor wave to a narrow file set.
