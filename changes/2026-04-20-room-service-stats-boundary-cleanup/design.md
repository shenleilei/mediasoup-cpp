# Design

## Goal

Split `RoomServiceStats.cpp` by responsibility without changing `RoomService`'s public surface.

## Proposed File Layout

- `RoomServiceStats.cpp`
  - `collectPeerStats`
  - `broadcastStats`
  - `continueBroadcastStats`
  - `broadcastStatsForRoom`
- `RoomServiceQos.cpp`
  - `setClientStats`
  - `maybeSendAutomaticQosOverride`
  - `maybeNotifyConnectionQuality`
  - `maybeBroadcastRoomQosState`
  - `maybeSendRoomPressureOverrides`
  - `cleanupExpiredQosOverrides`
- `RoomServiceRegistryView.cpp`
  - `heartbeatRegistry`
  - `resolveRoom`
  - `getNodeLoad`
  - `getDefaultQosPolicy`
- `RoomServiceDownlink.cpp`
  - also owns `setDownlinkClientStats`

## Why This Split

- stats report collection/broadcast is a different concern from QoS control decisions
- registry/load helpers are view/infrastructure concerns, not peer stats logic
- downlink ingest belongs with the rest of the downlink planner state

## Constraints

- keep `RoomService.h` method declarations stable
- move code mostly as-is
- avoid introducing new abstractions unless they reduce obvious duplication

## Verification Strategy

- rebuild `mediasoup-sfu`
- rebuild targeted QoS integration tests
- run targeted `clientStats` / `statsReport` / `downlinkClientStats` integration cases
