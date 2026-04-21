# Downlink QoS v1 Design

## 1. Goal

This document describes `subscriber/downlink QoS v1` for this project.

Current QoS implementation is primarily publisher/uplink oriented:

- browser-side publisher controller
- server-side `qosPolicy / qosOverride / automatic override`
- uplink-oriented weak-network matrix and targeted harnesses

The next largest capability gap is subscriber/downlink QoS.

This document only covers `v1`, which focuses on per-subscriber downlink control.

For the dedicated `v2` design, see:

- [downlink-qos-v2-design_cn.md](v2-design_cn.md)

## 2. Why This Matters

Further uplink-only tuning has diminishing returns.

What users feel most directly in real calls is usually downstream behavior:

- whether remote video is sharp enough
- whether important speakers remain smooth
- whether hidden videos stop wasting bandwidth
- whether recovery is fast but not oscillatory

The current project already has enough primitives to support a practical first version.

## 3. Existing Building Blocks

### 3.1 Existing downlink observations

The project already collects or exposes:

- browser `recvTransport` stats
- browser `inboundVideo / inboundAudio`
- browser `videoFreeze`
- server-side transport and consumer stats

Relevant files:

- [RoomServiceStats.cpp](../../../src/RoomServiceStats.cpp)
- [qos-demo.js](../../../public/qos-demo.js)

### 3.2 Existing server-side control actions

The server already has the key per-consumer knobs needed for downlink QoS:

- `pause()`
- `resume()`
- `setPreferredLayers(spatialLayer, temporalLayer)`
- `setPriority(priority)`
- `requestKeyFrame()`

Relevant files:

- [Consumer.h](../../../src/Consumer.h)
- [Consumer.cpp](../../../src/Consumer.cpp)

### 3.3 Current limitation

The current client QoS controller is publisher-only.

Relevant files:

- [controller.js](../../../src/client/lib/qos/controller.js)
- [types.d.ts](../../../src/client/lib/qos/types.d.ts)

Current action model contains only uplink/publisher actions such as:

- `setEncodingParameters`
- `setMaxSpatialLayer`
- `enterAudioOnly`
- `pauseUpstream`
- `resumeUpstream`

## 4. V1 Scope

`v1` should implement:

- per-subscriber video allocation
- visibility-aware subscription reduction
- size-aware preferred layer selection
- downlink health-based degradation and recovery

`v1` should not implement higher-level room-wide demand/supply closure yet.

That work is documented separately in:

- [downlink-qos-v2-design_cn.md](v2-design_cn.md)

## 5. High-Level Model

For each subscriber peer:

1. client reports downlink state + UI hints
2. server stores latest snapshot
3. server computes desired receive quality per remote consumer
4. server applies consumer-side actions

This means QoS becomes bidirectional:

- uplink QoS controls publisher behavior
- downlink QoS controls subscriber receive behavior

## 6. Proposed Client Snapshot Schema

Add a new schema, separate from uplink:

- `mediasoup.qos.downlink.client.v1`

Suggested payload:

```json
{
  "schema": "mediasoup.qos.downlink.client.v1",
  "seq": 12,
  "tsMs": 1710000000000,
  "subscriberPeerId": "alice",
  "transport": {
    "availableIncomingBitrate": 1200000,
    "currentRoundTripTime": 0.09
  },
  "subscriptions": [
    {
      "consumerId": "consumer-1",
      "producerId": "producer-1",
      "publisherPeerId": "bob",
      "kind": "video",
      "source": "camera",
      "visible": true,
      "pinned": false,
      "activeSpeaker": true,
      "isScreenShare": false,
      "targetWidth": 640,
      "targetHeight": 360,
      "packetsLost": 3,
      "jitter": 0.02,
      "framesPerSecond": 25,
      "frameWidth": 640,
      "frameHeight": 360,
      "freezeRate": 0.01,
      "recentFreezeRate": 0.00
    }
  ]
}
```

Key idea:

- uplink QoS reports what the sender is experiencing
- downlink QoS reports what the subscriber is actually receiving and displaying

## 7. New Server Components

### 7.1 `DownlinkQosRegistry`

Responsibility:

- store latest downlink snapshot per subscriber peer
- reject stale / malformed snapshots
- expose current per-subscriber state to controllers

Suggested files:

- `src/DownlinkQosRegistry.h`
- `src/DownlinkQosRegistry.cpp`

### 7.2 `SubscriberQosController`

Responsibility:

- compute desired receive quality for each consumer owned by a subscriber
- decide downgrade / upgrade / pause / resume
- maintain recovery state to avoid oscillation

Suggested files:

- `src/SubscriberQosController.h`
- `src/SubscriberQosController.cpp`

### 7.3 `DownlinkAllocator`

Responsibility:

- combine UI hints and network condition into final target actions
- rank consumers by importance
- distribute degradation in the least harmful order

Suggested files:

- `src/DownlinkAllocator.h`
- `src/DownlinkAllocator.cpp`

## 8. Action Model

`SubscriberQosController` should emit only these actions in `v1`:

- `pauseConsumer`
- `resumeConsumer`
- `setPreferredLayers`
- `setPriority`
- `requestKeyFrame`
- `noop`

These map directly to existing server capabilities.

## 9. Allocation Strategy

### 9.1 First stage: UI-driven target

Each remote consumer gets a desired target based on subscriber UI state:

- hidden/offscreen:
  pause video consumer
- very small tile:
  lowest spatial layer
- normal grid tile:
  medium spatial layer
- pinned speaker:
  high spatial layer
- screenshare:
  highest priority and highest acceptable layer

### 9.2 Second stage: network-health correction

Then adjust based on actual downlink health:

- rising freeze rate:
  degrade quickly
- FPS collapse:
  degrade
- high loss / jitter / RTT:
  degrade lower-priority consumers first
- low `availableIncomingBitrate`:
  reduce lower-priority consumers before touching pinned/screen-share

### 9.3 Recovery

Recovery should be gradual:

- upgrade one step at a time
- after layer upgrade, request keyframe
- require a few healthy samples before next upgrade
- use stronger conditions for “return to full quality” than for “stop emergency degradation”

## 10. Priority Model

Suggested default consumer priorities:

- screenshare: `255`
- pinned: `220`
- active speaker: `180`
- visible grid: `120`
- hidden: `0`

This is simple enough for `v1` and already aligns with perceived user value.

## 11. V2 Reference

This document does not define `v2`.

The dedicated `v2` design lives in:

- [downlink-qos-v2-design_cn.md](v2-design_cn.md)

## 12. Suggested File Changes

### New files

- `src/DownlinkQosRegistry.h`
- `src/DownlinkQosRegistry.cpp`
- `src/SubscriberQosController.h`
- `src/SubscriberQosController.cpp`
- `src/DownlinkAllocator.h`
- `src/DownlinkAllocator.cpp`
- `src/client/lib/qos/downlinkProtocol.js`
- `src/client/lib/qos/downlinkTypes.d.ts`
- `src/client/lib/qos/downlinkSampler.js`
- `src/client/lib/qos/downlinkHints.js`

### Existing files to update

- [SignalingServerWs.cpp](../../../src/SignalingServerWs.cpp)
  add a `downlinkClientStats` method
- [RoomService.h](../../../src/RoomService.h)
- [RoomServiceStats.cpp](../../../src/RoomServiceStats.cpp)
  store snapshots and update the downlink registries
- [RoomServiceDownlink.cpp](../../../src/RoomServiceDownlink.cpp)
  run subscriber controllers and the room-scoped planner
- [qos-demo.js](../../../public/qos-demo.js)
  first demo entry point for end-to-end verification

## 13. Testing Plan

### 13.1 Unit tests

- hidden consumer -> `pauseConsumer`
- pinned consumer keeps higher target than grid consumer
- freeze/loss/jitter cause downgrade
- recovery upgrades one layer at a time
- screenshare remains highest priority

### 13.2 Integration tests

- two subscribers request different target sizes and receive different preferred layers
- hidden subscriber pauses remote video consumer
- recovery triggers keyframe on upgrade
- lower-priority consumers degrade before pinned consumer

### 13.3 Browser harness

- one large tile + one small tile
- one pinned speaker + one normal tile
- hidden video transitions
- downlink weak-network + freeze scenarios

## 14. Recommended Implementation Order

1. define downlink snapshot schema
2. add browser-side reporting in demo / harness
3. implement `DownlinkQosRegistry`
4. implement simple `DownlinkAllocator`
5. wire server actions:
   - `pause/resume`
   - `setPreferredLayers`
6. add recovery + keyframe
7. add priority
8. only then move to [downlink-qos-v2-design_cn.md](v2-design_cn.md)

## 15. Acceptance Criteria For V1

`v1` should be considered successful only if all of the following are true:

- hidden videos are paused server-side
- large and small tiles receive different preferred layers
- pinned video is preserved longer under constrained downlink
- freeze/loss/jitter degradation is visible and stable
- recovery is stepwise and does not oscillate aggressively

## 16. Summary

The highest-value next step is not more uplink-only tuning.

The right next step is `subscriber/downlink QoS v1`.

Once `v1` is stable, move to the dedicated `v2` design in [downlink-qos-v2-design_cn.md](v2-design_cn.md).
