# Design

The implementation follows the structure of `/root/livekit/pkg/sfu/bwe/sendsidebwe` as directly as practical in C++.

This change supersedes the earlier local-only experiment under `changes/2026-04-21-plain-client-delay-based-twcc-bwe`. The repository now keeps only the livekit-aligned path as the supported send-side BWE direction.

Planned module mapping:

- `client/sendsidebwe/TwccFeedbackTracker.h`
- `client/sendsidebwe/PacketInfo.h`
- `client/sendsidebwe/PacketTracker.h`
- `client/sendsidebwe/TrafficStats.h`
- `client/sendsidebwe/PacketGroup.h`
- `client/sendsidebwe/ProbePacketGroup.h`
- `client/sendsidebwe/CongestionDetector.h`
- `client/sendsidebwe/SendSideBwe.h`
- `client/ccutils/TrendDetector.h`
- `client/ccutils/ProbeSignal.h`
- `client/ccutils/ProbeRegulator.h`

## Current State

The repository already has the main LiveKit-inspired module split in place:

- TWCC feedback ordering and reference-time expansion
- packet tracking
- packet groups and traffic stats
- congestion state and available-channel-capacity estimation
- probe-related data structures and lifecycle types

That porting work is done. The runtime sender semantics and probe behavior are now also aligned on the supported main path. Remaining work is operational closeout:

- white-box observability depth improvements beyond the currently exposed fields
- final accepted-behavior/spec updates
- residual-difference documentation where the uplink sender intentionally differs from LiveKit's downlink allocator context

## Main-Path Alignment Status

The main-path sender semantics that previously blocked truthful LiveKit alignment have now been addressed:

1. transport-wide sequence stability on retry
2. probe RTP isolation from ordinary media retransmission bookkeeping
3. probe early-stop wiring on the network-thread path
4. probe-trigger math aligned to LiveKit's default padding-probe policy through a sender-side equivalent mapping

The remaining work is no longer on the transport-control main path itself. It is on observability refinement, report-pipeline depth, and final accepted-behavior documentation.

## Explicit Residual Differences

The following differences remain explicit and out of scope for this change:

- the plain-client sender does not implement a LiveKit-style downlink allocator or layer-transition model
- aggregate-constrained multi-track send fairness is still governed by the existing sender/runtime behavior rather than a new weight-aware media scheduler

That means this change aligns the plain-client send-side BWE and padding-probe path, but it does not claim a new weighted multi-track pacing/fairness model.

## NetworkThread Role

`NetworkThread` remains the runtime integration point:

- ask send-side BWE for the next transport-wide sequence number
- rewrite RTP extension with that sequence number
- record successful sends in the BWE tracker
- feed parsed TWCC feedback into the BWE
- publish estimated available channel capacity into `SenderTransportController`
- start probe clusters when application demand exceeds estimated capacity and the BWE allows probing
- send transport-cc-enabled RTP padding probe packets on the network thread
- feed probe completion back into the BWE through the probe observer / finalize path

The previous local `DelayBasedBwe` has been replaced on the `NetworkThread` main path. Remaining work is about making `NetworkThread` and `SenderTransportController` uphold the assumptions of the new BWE path.

## Sequence Allocation Semantics

The most important sender-side invariant is:

- a queued packet gets at most one transport-wide sequence number for its send lifetime

That means:

- sequence allocation may happen when the packet is first prepared for send
- the assigned sequence must be stored with the queued packet
- a local retry after `WouldBlock` must reuse the same sequence
- only a successful send may be committed to the BWE sent-packet tracker

The current "allocate on each attempt" behavior is not acceptable because it converts local socket backpressure into apparent network loss in TWCC feedback.

### Required Shape

The send-side BWE integration must support two distinct actions:

1. allocate or reserve a transport-wide sequence number for a packet
2. record that that numbered packet was actually sent

For the plain client, those actions must not be conflated.

## Probe RTP Isolation

LiveKit's probing path does not treat probe packets as ordinary media packets for retransmission bookkeeping purposes.

The plain client therefore needs the same semantic split:

- ordinary video media packets update retransmission storage and ordinary media byte accounting
- probe RTP packets update probe completion and sender observability, but do not become candidates for ordinary media NACK retransmission

### Required Contract

Probe RTP packets:

- SHALL carry transport-cc
- SHALL be visible to probe accounting and probe completion logic
- SHALL NOT enter `RtpPacketStore`
- SHALL NOT be selected by ordinary NACK retransmission logic
- SHALL NOT inflate ordinary video octet accounting

This requires a dedicated sender-bookkeeping path rather than reusing `onVideoRtpSent(...)` unchanged.

## Probe Lifecycle

The C++ port already contains the core probe objects:

- `ProbePacketGroup`
- `ProbeRegulator`
- `Prober`
- `ProbeObserver`
- `SendSideBwe::{ProbeClusterStarting, ProbeClusterDone, ProbeClusterIsGoalReached, ProbeClusterFinalize}`

The remaining requirement is to wire them on the network-thread side the same way the LiveKit runtime does:

- cluster start
- packet emission
- observer-based completion
- early stop when the goal is reached
- finalization and regulator update

### Required Tick Order

For the network-thread pacing tick, the intended order is:

1. maybe start a probe cluster
2. flush pending probe packets
3. maybe stop the active cluster early if the BWE says the goal is reached
4. finalize any completed or aborted cluster
5. run normal paced media sending

Early-stop support is part of the aligned lifecycle, not an optional optimization.

## Probe Trigger Policy

LiveKit's default padding-probe policy is computed inside a downlink stream allocator using:

- the currently committed channel capacity
- expected bandwidth usage
- the next higher transition for a deficient track
- `ProbeOveragePct`
- `ProbeMinBps`

The plain client uplink sender has no deficient-track / next-higher-layer transition object model. A sender-side equivalent mapping is therefore required.

### Sender-Side Equivalent Mapping

For the plain client, the default aligned padding-probe policy is:

- `availableBandwidthBps = transportEstimatedBitrateBps_`
- `expectedUsageBps = current sender usage`
- `transitionDeltaBps = max(aggregateTargetBitrateBps - expectedUsageBps, 0)`
- `desiredIncreaseBps = max(transitionDeltaBps * ProbeOveragePct / 100, ProbeMinBps)`
- `desiredBps = expectedUsageBps + desiredIncreaseBps`

Default configuration should match LiveKit defaults:

- `ProbeMode = padding`
- `ProbeOveragePct = 120`
- `ProbeMinBps = 200000`

### Carrier Track Selection

The sender has no allocator-owned deficient track list. The sender-side equivalent is:

- choose an active video track with a transport-cc extension id
- use that track as the probe carrier

This is an intentional runtime-specific mapping, not a design deviation that should be hidden.

## Rollback And Safe Defaults

The existing runtime controls remain the rollback boundary:

- `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER`
- `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE`

### Default Runtime Mode

The supported default runtime mode is the fully enabled path:

- `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`
- `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1`

In the shorthand used during validation, that means the repository default is `G2`, not `G0` or `G1`.

Defaults must remain safe:

- no probing when estimate publishing is disabled
- no probe cluster when there is no valid transport-cc-capable video carrier
- no behavior that depends on probe support for plain-client startup or session establishment

## Verification Strategy

The completed sender-path alignment still requires targeted verification in addition to the already-existing TWCC smoke tests, and the remaining follow-up work requires transport-effect observability.

Verification must be split into two layers:

1. sender-semantics correctness
2. runtime effectiveness

Those two layers must not be conflated.

### Unit-Level

- retry-after-`WouldBlock` reuses the same transport-wide sequence
- successful send is the only path that records into the BWE sent-packet tracker
- probe packet bookkeeping does not enter ordinary retransmission storage
- probe finalize waits for completion, handles congestion, and respects goal-reached stop behavior
- probe goal construction follows the configured overage/minimum policy

The local-backpressure / false-loss problem MUST be proven here (or in a deterministic integration test with a fake sender), not in a black-box QoS matrix run. The current matrix and harness flows do not deterministically force kernel-level `WouldBlock` in a way that is suitable as the primary proof for this bug class.

### Integration-Level

- real worker TWCC feedback still reaches the plain sender
- probe RTP is emitted with transport-cc and probe completion still works
- pause / shutdown / generation-switch regressions remain green
- ordinary video retransmission logic does not pick up probe packets

### Runtime Effectiveness

Effectiveness review needs both black-box and white-box evidence.

#### Black-Box

Black-box metrics should answer:

- does recovery get faster after bandwidth returns?
- does steady-state sending remain stable?
- are probe packets emitted and stopped at the expected times?

With the current repository tooling, the executable black-box signals available immediately are:

- `QOS_TRACE`-derived `sendBps`
- `lossRate`
- `rttMs`
- `jitterMs`
- state / level / action timing

Those are still useful, but they are proxies. They are not yet a full transport-internal observability set.

#### White-Box

To prove the transport-layer behavior directly, the following white-box values are needed:

- `transportEstimatedBitrateBps`
- `effectivePacingBitrateBps`
- `transportCcFeedbackReports`
- probe cluster start / completion / early-stop counts
- probe bytes sent
- retransmission counts
- queued fresh-video / retransmission / audio pressure counters

At the time of this design update, the main path already exposes:

- `transportEstimatedBitrateBps`
- `effectivePacingBitrateBps`
- `transportCcFeedbackReports`
- sender-usage bitrate
- queue and retransmission counters
- probe activity and probe packet counts
- probe cluster start / completion / early-stop counts
- probe bytes sent

Further follow-up may still improve report presentation, but the main observability path is already sufficient for targeted effectiveness review.

### Report-Pipeline Boundary

The current QoS report chain in `tests/qos_harness/` primarily consumes `QOS_TRACE` samples and derives:

- send-rate proxies
- QoS state/action timing
- fairness from client-side trace samples

It does not yet automatically surface probe-lifecycle counters or queue/retransmission counters from the sender path.

Therefore:

- correctness fixes may land with deterministic unit/integration proof first
- effect-evaluation docs must distinguish "currently executable metrics" from "metrics that require added instrumentation"
- any future A/B effectiveness report that claims probe-lifecycle or queue-pressure improvements must first add the necessary observability to the harness/report path

## Completion Criteria

This change can be called "aligned with LiveKit" for the plain-client main path only when:

- the core send-side BWE modules are active on the main path
- retry semantics no longer create false TWCC loss
- probe packets are isolated from ordinary media retransmission semantics
- probe early-stop and finalization are both wired
- probe-trigger math follows the sender-side equivalent of LiveKit's default padding-probe policy

Anything short of that should be described as "partially ported" rather than "aligned".
