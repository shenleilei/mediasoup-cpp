## Problem Statement

The current `src/` production code has grown around a working runtime model, but several core files now combine too many responsibilities:

- request parsing, session validation, WebSocket dispatch, HTTP endpoints, metrics, and static file serving in `src/SignalingServer.cpp`
- room lifecycle, transport orchestration, recording, stats collection, QoS ingestion, QoS notification, and downlink planning in `src/RoomService.cpp`
- runtime bootstrap, daemonization, config parsing, public IP detection, geo setup, registry setup, and worker-thread pool setup in `src/main.cpp`
- Redis command handling, cache management, subscriber thread management, node selection, and claim/resolve logic in `src/RoomRegistry.h`

This makes the code harder to reason about, increases compile coupling, and raises regression risk when changing one subsystem.

The adjacent `client/` code also duplicates part of the QoS wire contract and build wiring instead of reusing the same repository-level abstractions and protocol constants.

## Business Goal

Refactor the `src/` production code and the overlapping `client/` contract/build surfaces so the control plane remains behaviorally compatible while becoming easier to maintain, verify, and extend.

## In Scope

- `src/` production code
- `client/` code only where it duplicates or should reuse `src`/repo-level contract or build infrastructure
- module boundary cleanup and responsibility split
- extraction of pure helpers and coordinators from large files
- movement of large implementation bodies out of headers where safe
- compile dependency reduction
- shared protocol-contract extraction between `src` and `client` where the wire contract is identical
- preservation of current runtime behavior and current external request contracts

## Out Of Scope

- changing browser or C++ client protocols
- changing Redis key formats or room-routing semantics
- rewriting tests as a primary goal
- replacing mediasoup worker IPC format
- adding new product behavior unless required to preserve existing invariants during refactor
- merging client-only runtime logic into `src/` when the semantics are not actually shared

## Scenarios

### Scenario 1: WebSocket request evolution

A developer adds or modifies a signaling method without editing a thousand-line mixed transport and HTTP file.

### Scenario 2: Room-level behavior change

A developer changes join, produce, consume, stats, or QoS behavior in isolated room-domain modules without touching unrelated recording or registry code.

### Scenario 3: Runtime debugging

An operator can reason about bootstrap, registry, and worker-thread ownership boundaries directly from module names and file placement.

## Acceptance Criteria

1. `src/main.cpp`, `src/SignalingServer.cpp`, and `src/RoomService.cpp` each have materially narrower responsibilities than the current baseline.
2. Large header-implemented runtime components are reduced so implementation details no longer force broad recompilation for small logic changes where practical.
3. Shared JSON and FlatBuffers translation logic used by transport, producer, and consumer code is centralized instead of duplicated.
4. Room-domain logic is split into focused internal modules for session/lifecycle, transport/media, stats/recording, and QoS/downlink planning.
5. Signaling edge logic is split so WebSocket method dispatch, HTTP routes, session bookkeeping, and file-serving utilities are not implemented as one monolith.
6. Existing unit and integration targets covering affected `src/` behavior still pass after each refactor wave.
7. When `client/` and `src/` use the same QoS schema names, hard limits, or JSON contract fields, those values are defined once and reused.
8. `client/CMakeLists.txt` remains usable both standalone and when included from a parent CMake project.

## Non-Functional Requirements

- No intentional behavior regressions
- No reduction in current health, metrics, or logging coverage
- No new global mutable state unless clearly isolated and documented
- Respect the existing same-room-to-same-worker-thread invariant
- Preserve current stale-session rejection behavior

## Edge Cases And Failure Cases

- worker process death while room state is active
- Redis unavailable or degraded during room claim and resolve
- reconnect replacing an existing peer session
- stats collection under timeout pressure
- downlink planner wake-up, defer, and stale-snapshot cleanup
- file-serving path traversal and aborted HTTP streams

## Open Questions

- Whether `Recorder.h` should be split in the same change or deferred after `RoomService` extraction
- Whether `RoomManager` locking should be preserved as-is or reduced after worker-thread ownership is made explicit
