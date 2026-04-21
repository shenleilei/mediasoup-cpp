# Design

## Context

The repository already separates upper-layer sender QoS policy from lower media-transport mechanics:

- browser/libwebrtc senders and `mediasoup-worker` own the baseline transport repair and congestion-control stack
- repository-owned client QoS adds higher-layer policy, adaptation, and control-plane semantics

That split is accurate for browser senders and for the worker-side transport stack. The Linux plain client, however, lives in a different position:

- it mirrors upper-layer QoS controller behavior closely enough to pass parity and matrix coverage
- it owns its own RTP/RTCP sender path instead of delegating to browser/libwebrtc sender internals
- it therefore lacks the sender transport control layer that browser/libwebrtc senders already have

This change does **not** attempt to rebuild full libwebrtc sender congestion control inside the plain client. It introduces the missing architectural layer so future work can evolve from "upper-layer QoS only" toward "QoS plus transport execution control" without another large rewrite.

## Reference Review

This design is grounded in three reviewed baselines.

### 1. mediasoup-worker

Worker-side transport code already separates:

- `TransportCongestionControlServer`
- `TransportCongestionControlClient`
- `SenderBandwidthEstimator`
- pacing/probing integration through libwebrtc

The important lesson is architectural rather than literal: transport estimation, pacing, probing, and send execution live in a dedicated subsystem, not scattered across raw socket send call sites.

### 2. WebRTC sender stack

libwebrtc sender transport control keeps distinct modules for:

- feedback ingestion
- target transfer-rate calculation
- probing
- pacer configuration
- paced send execution

Its pacer model is byte/time-budget-based and packet-type-aware. The plain client should not clone the whole stack now, but phase 1 should adopt the same kind of layering:

- upper-layer policy chooses adaptation intent
- transport-control layer chooses send scheduling/execution
- raw socket sender performs the actual datagram send

### 3. LiveKit operational model

LiveKit exposes congestion-control and batch-I/O tuning as system-level transport/runtime concerns. This reinforces that send batching and transport scheduling are architecture-level capabilities, not just a handful of `send()` wrappers.

## Current Gap

Today the plain client has:

- upper-layer QoS controller and coordinator parity
- encoder/source adaptation
- RTCP RR/NACK/PLI handling
- a fixed video pacing deque inside `NetworkThread`

But it still lacks a dedicated send-side transport control layer. `NetworkThread` therefore mixes:

- socket ownership
- queue ownership
- pacing policy
- send-result semantics
- RTCP accounting side effects
- pause/resume execution

That mix makes three things hard:

1. applying upper-layer QoS decisions to transport behavior in a principled way
2. handling nonblocking UDP send semantics correctly
3. later adding feedback-driven estimation or probing without rewriting the send path again

## Design Goal

Introduce a **SenderTransportController** module that sits between upper-layer QoS/source outputs and raw UDP socket send execution.

This module is intentionally narrower than libwebrtc GCC. It is phaseable:

- phase 1: correctness, transport-control boundary, packet-class policy, byte-budget pacing
- later phases: feedback-driven estimate, probing, richer scheduling

## Explicit Goals

Phase 1 must deliver:

- explicit send-result classification
- explicit packet-class send policies
- byte-budget-driven video pacing instead of fixed packet-count bursts
- transport-hint wiring from upper-layer QoS/control into transport execution
- success-only accounting for RTP/RTCP state

Phase 1 must **not** claim:

- full browser/libwebrtc sender congestion-control parity
- full TWCC packet-feedback estimate
- full probe controller
- final audio/video fairness model for all future scenarios

## Phase-1 Decisions

The earlier draft left three important points open. For phase 1, they are now fixed:

- audio uses a bounded deadline-aware pending queue
- `ENOBUFS` is treated as `WouldBlock`
- transport hints are sourced only from upper-layer target bitrate and paused state

These decisions keep the first landing reviewable and prevent policy logic from leaking into the transport controller.

## Ownership And Placement

### Ownership

`SenderTransportController` is owned by `NetworkThread`.

Rationale:

- the UDP socket already belongs to the network thread
- pending packet lifetime and send-result handling must stay single-threaded
- queue mutation, pacing state, and RTCP side effects should not cross thread boundaries

### Relationship To Existing Modules

- `PublisherQosController`
  - remains the source of upper-layer policy/adaptation intent
- `SourceWorker`
  - remains the owner of encoder/source adaptation
- `NetworkThread`
  - remains the owner of the UDP socket and event loop
- `SenderTransportController`
  - becomes the owner of send-side queueing, pacing, and send-result policy

### Non-Responsibilities

`SenderTransportController` must not absorb:

- QoS state-machine logic
- encoder ladder/profile logic
- WebSocket signaling ownership
- room/server policy semantics

## Control And Data Flow

### Upper Layer

`PublisherQosController` remains the source of upper-layer QoS actions:

- `SetEncodingParameters`
- `PauseUpstream`
- `ResumeUpstream`
- `EnterAudioOnly`
- `ExitAudioOnly`

Those actions still apply to:

- source worker encoder configuration
- track pause/resume state

### New Transport Hint Path

The plain client already has a dormant `NetworkControlCommand::TrackTransportConfig`.

Phase 1 uses that path to carry **transport hints**, not policy logic:

- track index
- SSRC
- paused / active state
- media class
- current target bitrate for pacing purposes
- future room for priority/urgency hints

This lets the upper layer describe "what the current intended send profile is" without moving policy logic into the network thread.

Phase 1 transport hints come only from:

- per-track target bitrate after upper-layer QoS/controller action application
- paused / active track state

Phase 1 transport hints do **not** come directly from:

- local observed send bitrate
- server-observed producer bitrate
- ad hoc transport-layer self-feedback loops

### Network Layer

`NetworkThread` remains the owner of:

- UDP event loop
- RTCP parsing
- transport controller execution

But it should stop owning ad hoc pacing policy directly.

## Packet Classes And Transport Priorities

One of the main corrections from the earlier draft is that packet priority and send policy must be explicit. The plain client cannot treat all send paths as if they were equivalent.

Phase 1 uses these packet classes:

1. `Control`
   - RTCP SR / control traffic / startup transport-control packets
2. `AudioRtp`
3. `VideoRetransmission`
4. `VideoMedia`

Priority order:

1. `Control`
2. `AudioRtp`
3. `VideoRetransmission`
4. `VideoMedia`

This intentionally tracks WebRTC sender-side intent:

- control is small and highly time-sensitive
- audio is delay-sensitive
- retransmission is usually more valuable than sending fresh low-priority video
- fresh video is the bulk elastic traffic

## Send-Result Classification

Phase 1 standardizes raw UDP send outcomes as:

- `Sent`
- `WouldBlock`
- `HardError`

`EINTR` is retried internally and does not leave the helper as a first-class external result.

Recommended mapping:

- `WouldBlock`
  - `EAGAIN`
  - `EWOULDBLOCK`
  - `ENOBUFS` in phase 1
- `HardError`
  - `EMSGSIZE`
  - `EINVAL`
  - `EBADF`
  - `ENOTCONN`
  - other terminal send errors

The key rule is:

- **only successfully sent packets may update RTP/RTCP accounting, retransmission store, or sender-report state**

## Per-Class `WouldBlock` And `HardError` Contract

The prior draft was too vague here. Phase 1 now fixes the contract explicitly.

### 1. Control / RTCP

- `WouldBlock`
  - packet is not counted as sent
  - bounded retry/throttling applies
  - repeated `WouldBlock` must not create a hot loop
- `HardError`
  - packet is dropped
  - error is logged/counted with packet class and errno
  - session shutdown is not triggered automatically in phase 1

### 2. Audio RTP

- `WouldBlock`
  - packet may remain in the bounded audio queue until its deadline expires
  - packet is not counted as sent
- `HardError`
  - packet is dropped
  - error is logged/counted
  - no send-success accounting is updated

### 3. Video Retransmission

- `WouldBlock`
  - packet remains pending in the retransmission queue
  - packet is not counted as retransmitted
- `HardError`
  - packet is dropped
  - retransmission counters are not advanced
  - error is logged/counted

### 4. Fresh Video Media

- `WouldBlock`
  - packet remains pending in the per-track video queue
  - packet-send accounting does not advance
- `HardError`
  - packet is dropped
  - send-success accounting does not advance
  - error is logged/counted

These rules make the implementation and tests directly checkable and avoid per-path improvisation during coding.

## Queue Model

Another correction from the earlier draft: phase 1 should not use one undifferentiated FIFO queue for audio, video, and control.

### 1. Control / RTCP

- not merged into the normal media pacing queue
- best-effort immediate send path
- bounded retry/throttling where required
- independent of video pacing backlog

### 2. Audio RTP

Phase 1 introduces a **small deadline-aware audio pending queue** rather than either:

- immediate drop on `WouldBlock`, or
- a full-blown unified scheduler

Rules:

- audio packets are queued only briefly
- queue depth is bounded by media time, not just element count
- packets older than a small deadline window are dropped explicitly

Recommended initial deadline:

- `1..2` audio packetization intervals (`ptime`)

Rationale:

- audio is delay-sensitive
- sending stale audio late is usually worse than dropping it visibly
- this gives the transport layer bounded tolerance to transient `WouldBlock` without silently pretending audio was sent

### 3. Video Retransmission

- separate queue from fresh video
- higher priority than fresh video
- still datagram-atomic
- can consume pacing budget with higher urgency semantics

### 4. Video Media

- per-track pending queues
- round-robin across active tracks within the same priority class
- paced by byte budget

This keeps multi-track behavior explicit without dragging a full weighted fair scheduler into phase 1.

## Why Not A Single Unified Pending Queue

The previous idea of a single generic pending queue was intentionally rejected for phase 1.

Why:

- audio and video have different lateness tolerance
- control traffic should not wait behind media backlog
- retransmissions need stricter urgency than fresh video
- one FIFO queue would bake in the wrong scheduler before the transport-control boundary is even clean

Future phases may unify packet representation while keeping multiple logical queues, but phase 1 keeps policy explicit.

## Pacing Model

### Current Problem

The current fixed `kBurstLimit` packet-count pacing is too blunt:

- RTP packet sizes vary materially
- packet count is not the right transport budget unit
- "N packets per tick" does not correspond to any bitrate target in a stable way

### Phase-1 Pacing Rule

Phase 1 replaces fixed packet-count pacing with:

- **byte budget per pacing interval**
- consumed by packet sends, still datagram-atomic

Transport control therefore uses:

- budget in bytes over time
- execution in whole UDP packets

In other words:

- budget is byte-based
- packet transmission remains datagram-atomic

### Budget Source In Phase 1

Phase 1 does not have a feedback-driven estimate yet. The pacing budget is therefore derived from active **transport hints**:

- per-track target bitrate from the upper-layer QoS/encoder state
- paused tracks contribute no active send target
- aggregate active target drives the initial byte budget

This means phase 1 is:

- **encoder-driven transport budgeting**

not yet:

- **feedback-driven transport budgeting**

### Budget Overshoot

A bounded budget overshoot is acceptable for:

- `Control`
- `AudioRtp`
- optionally `VideoRetransmission`

But not for normal `VideoMedia`.

Rationale:

- control/audio are more time-sensitive than fresh video
- a small negative budget/debt is easier to reason about than starving those classes behind video bulk traffic

## Audio Sending Policy

The question "how many audio packets should be sent" is intentionally answered in media-time terms, not raw packet-count terms.

Phase 1 policy:

- audio production cadence is still determined by audio media timing/ptime
- the transport controller decides whether the ready audio packet:
  - sends now
  - waits briefly in the bounded audio queue
  - expires and is dropped

The transport controller should not batch audio by arbitrary packet count. Audio is still packetized and sent as individual RTP datagrams, but scheduling is driven by:

- packet readiness in media time
- class priority
- bounded lateness

## Video Scheduling Policy

Phase 1 video scheduling is intentionally simpler than a full GCC scheduler:

- fresh video uses per-track queues
- within `VideoMedia`, use round-robin across active tracks
- each packet consumes byte budget
- if the next packet does not fit, it remains queued until a later pacing interval

This is enough to replace the fixed packet-count pacing rule without dragging in a full weighted transport allocator.

## RTCP And Accounting Contract

`RtcpContext` currently conflates:

- retransmission packet store
- last keyframe cache
- RR-derived stats
- SR send-time tracking

Phase 1 does not fully split `RtcpContext`, but it tightens the contract:

- packet-send accounting and retransmission-store updates happen only after successful send
- SR timing maps update only after successful SR send
- NACK retransmission counters reflect only actual retransmits
- repeated `WouldBlock` on SR/control must not create a hot loop

For SR specifically, phase 1 separates:

- last attempt time
- last successful send time

so retry throttling and send-success accounting are not conflated.

## Security Considerations

Phase 1 is not a security feature, but send-path changes can still create security-relevant regressions if they are careless.

Required constraints:

- packet classification and queueing MUST NOT permit unbounded queue growth under repeated `WouldBlock`
- audio/video/control packet retention MUST remain bounded so a malicious or broken peer cannot trivially force memory growth by suppressing socket progress
- hard send errors MUST be surfaced in diagnostics rather than silently hidden, so invalid socket state or misuse is visible during testing and operation
- new transport hints MUST remain internal descriptive control data and MUST NOT become an unvalidated external input path
- the fallback path used during implementation MUST preserve existing socket and session ownership rules rather than creating parallel active send paths

This phase does not add new wire protocols, auth/authz behavior, or secret material handling.

## Observability

Phase 1 must expose enough diagnostics to debug transport regressions without attaching a packet sniffer first.

Required observability:

- counter/log for `WouldBlock` by packet class
- counter/log for hard send failure by packet class and errno
- counter/log for audio deadline expiry drops
- counter/log for queued video drops due to hard error
- counter/log for retransmission send success vs retransmission drop
- warning or structured debug when SR/control retry throttling engages repeatedly

Logging should be rate-limited on hot paths; counters/aggregates should remain queryable in tests.

## Rollout And Mitigation

Phase 1 is a risky send-path change even though it stops short of full GCC.

Mitigation plan:

- land the controller boundary and correctness fixes incrementally
- keep a short-lived fallback path during implementation that can disable the new transport controller and use the legacy fixed-burst sender if validation uncovers regressions before merge
- do not claim full transport parity in docs or release notes for this landing

Rollback strategy for the implementation wave:

- revert the `SenderTransportController` integration patch set as one unit if regression evidence shows transport or QoS-path instability
- preserve the existing parity/matrix entrypoints so regressions are visible immediately

## sendmmsg Placement

`sendmmsg` is an execution optimization, not the architectural center of the fix.

Phase 1 may place `sendmmsg` only in the **queued fresh-video flush path**, because:

- video media is already batch-shaped
- success-prefix semantics map cleanly to queue fronts
- it avoids forcing audio/control/retransmission into the same batching policy

Rules:

- build a batch only from the currently selected front packets
- send
- commit only the successful prefix
- retain the unsent suffix
- never update accounting for packets outside the successful prefix

If schedule pressure suggests deferring `sendmmsg`, the transport-controller boundary still stands. The architecture should not depend on the optimization landing in the same patch.

## Module Surface

The concrete implementation may vary, but the phase-1 controller should conceptually provide:

- `UpdateTrackTransportHint(...)`
- `EnqueueVideoMediaPacket(...)`
- `EnqueueAudioPacket(...)`
- `EnqueueVideoRetransmissionPacket(...)`
- `SendControlPacket(...)`
- `OnPacingTick(nowMs)`
- `OnWouldBlockRecovery(nowMs)` if later needed
- counters/diagnostics accessors

The main point is explicit ownership and semantics, not the exact C++ type names.

## Implementation Phasing

### Phase 1: Transport Control Foundation

Ship:

- `SenderTransportController` module
- explicit packet classes and priorities
- explicit send-result classification
- transport hint path
- small bounded audio queue
- per-track queued video with round-robin scheduling
- byte-budget video pacing
- success-only accounting semantics

Do not ship yet:

- full TWCC feedback estimate
- acknowledged-bitrate estimator
- probe controller
- final weighted fairness model for all media classes

### Phase 2: Feedback-Driven Estimate

Add:

- feedback ingestion path suitable for plain-client sender operation
- transport estimate that can cap aggregate pacing budget beneath encoder targets

### Phase 3: Probing And Richer Scheduling

Add:

- explicit probing control
- richer fairness/weight model if product needs justify it
- revisit whether audio should count into the aggregate pacing budget in the same way as video in all cases

## Risks

- transport hints must remain descriptive, not become a second policy engine inside the network thread
- a too-ambitious phase 1 could collapse into a partial GCC rewrite; the phased boundary prevents that
- a single undifferentiated queue would encode the wrong scheduler and make later work harder
- leaving accounting semantics wrong would make later feedback-driven estimation untrustworthy

## Test Strategy

### 1. Deterministic Unit Tests

Use:

- fake clock
- fake packet sender / fake batch sender
- no dependence on filling a real kernel UDP buffer to trigger `EAGAIN`

Cover:

- send-result classification
- success-only accounting
- queue retention on `WouldBlock`
- hard-error policy
- audio deadline expiry
- retransmission priority over fresh video
- byte-budget pacing by bytes rather than packet count
- `sendmmsg` successful-prefix commit semantics
- `ENOBUFS` classification as `WouldBlock`

### 2. Component Tests

Exercise:

- `NetworkThread + SenderTransportController + RtcpContext`

Cover:

- transport hints updating pacing budget
- pause/resume interaction with queued video
- RTCP/SR throttling on repeated `WouldBlock`
- NACK retransmission accounting
- hard-error handling per packet class

### 3. Integration Tests

Keep and extend:

- threaded plain-publish integration
- existing QoS parity/matrix coverage

Add targeted transport-control integration for:

- queued video under pause/resume
- bounded audio backlog under transient backpressure
- RTCP/control path still working under pacing traffic
- existing plain-client QoS semantics still matching accepted parity entrypoints

### 4. Documentation Checks

- architecture docs updated only after implementation lands
- parity docs remain explicit that phase 1 improves transport execution semantics but does not claim full browser/libwebrtc sender transport parity
