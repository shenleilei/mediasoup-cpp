# Problem Statement

`src/RoomServiceStats.cpp` still mixes several different responsibilities:

- QoS ingest and automatic override/connection-quality logic
- peer stats collection and room stats broadcast
- registry heartbeat / resolve / node-load view helpers
- downlink snapshot ingest

That makes the file harder to review and change safely, especially now that other nearby runtime hotspots have already been split into narrower modules.

## Business Goal

Narrow `RoomServiceStats.cpp` so stats-reporting responsibilities, QoS control responsibilities, registry-view helpers, and downlink ingest do not all live in one implementation file.

## In Scope

- `src/RoomServiceStats.cpp`
- new focused `RoomService*.cpp` implementation files as needed
- `src/RoomServiceDownlink.cpp`
- `CMakeLists.txt`
- affected live docs

## Out Of Scope

- changing QoS behavior
- changing stats payloads
- changing registry behavior
- redesigning `RoomService` public APIs

## Acceptance Criteria

1. `RoomServiceStats.cpp` SHALL primarily contain peer stats collection and stats broadcast responsibilities.
2. QoS-specific ingest / override / connection-quality / room-pressure logic SHALL move to a dedicated implementation file.
3. Registry/load-view helpers (`heartbeatRegistry`, `resolveRoom`, `getNodeLoad`, `getDefaultQosPolicy`) SHALL no longer live in `RoomServiceStats.cpp`.
4. `setDownlinkClientStats()` SHALL live with the downlink implementation rather than the stats-reporting implementation.
5. Existing behavior and tests SHALL remain unchanged.
