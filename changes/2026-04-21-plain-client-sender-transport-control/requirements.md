# Plain Client Sender Transport Control

## Problem Statement

The Linux `PlainTransport` C++ client has already reached broad parity with the JS/browser QoS stack at the policy layer:

- controller / planner / coordinator / protocol behavior is mirrored
- policy, override, stale-seq, and matrix harness scenarios are covered
- encoder/source actions are applied for bitrate/fps/scale and pause/resume semantics

However, the transport execution layer below those QoS decisions is still too thin:

- `NetworkThread` uses a fixed packet-count pacing loop rather than an explicit transport-control module
- audio, video, RTCP control, and startup probe sends do not have clearly separated send policies
- send-result handling is not modeled explicitly enough to distinguish successful send, temporary socket backpressure, and hard send failure
- there is no dedicated place to later add feedback-driven transport estimation, probing, or pacing refinement without reworking the entire client send path

Today the plain client therefore achieves **upper-layer QoS policy parity** with JS, but not **sender transport control parity** with browser/libwebrtc senders.

## Business Goal

Introduce a dedicated sender transport control layer for the plain client so that:

- upper-layer QoS decisions apply to transport behavior in a principled way
- transport send semantics are correct and diagnosable
- the client can evolve toward feedback-driven pacing and estimation without rewriting the plain-client runtime around a new framework

## In Scope

- Linux plain-client send-side architecture under `client/`
- a new sender transport control boundary between QoS / source workers and raw socket send calls
- first-phase transport control capabilities:
  - explicit send-result classification
  - explicit per-class send policy separation for video media, audio media, and RTCP/control traffic
  - byte-budget-driven video pacing instead of fixed packet-count bursts
  - transport-hint wiring from QoS/control outcomes into the send-side pacing budget
  - transport-control observability and regression coverage
- design and tasking for later feedback-driven transport estimation / probing work

## Out Of Scope

- embedding or vendoring the full libwebrtc sender congestion-control stack into the plain client
- replacing `PublisherQosController` or reworking the accepted upper-layer QoS policy logic
- changing the plain-client external CLI contract
- redesigning the whole plain-client runtime into a generic media framework
- implementing full TWCC/GCC parity in the first landing
- room-level or global peer bitrate coordination beyond the existing per-peer QoS controller/coordinator behavior

## Scenarios

### Scenario 1: QoS policy drives transport behavior through a dedicated layer

When `PublisherQosController` or peer coordination changes the effective per-track send intent, the plain client does not rely on ad hoc fixed bursts or direct socket sends in `NetworkThread`; instead, transport behavior is mediated by a dedicated sender transport control layer.

### Scenario 2: Send outcomes are modeled explicitly

When the plain client attempts to send media or control traffic over the nonblocking UDP socket, the system distinguishes:

- send succeeded
- socket would block / transient backpressure
- hard send error

and applies path-specific behavior instead of treating all results as equivalent.

### Scenario 3: Video pacing uses byte budget instead of packet count

When queued video RTP packets are flushed, the scheduler uses a byte budget derived from active transport hints rather than a fixed `kBurstLimit` packet count.

### Scenario 4: The first landing remains incremental

When this work lands, it improves the plain-client send-side architecture without claiming full browser/libwebrtc sender-BWE parity and without forcing an immediate rewrite of audio scheduling, TWCC packet feedback, or full probing control.

## Acceptance Criteria

1. The plain client SHALL introduce a dedicated sender transport control boundary between upper-layer QoS/source decisions and raw UDP socket send calls.
2. The first landing SHALL define and use explicit send-result classification covering successful send, temporary backpressure, and hard send failure.
3. The first landing SHALL define explicit send policies for:
   - fresh video RTP media
   - video RTP retransmission
   - audio RTP media
   - RTCP/control traffic
4. The first landing SHALL define a priority contract in which RTCP/control traffic is scheduled independently from media backlog, audio RTP outranks fresh video, and video retransmission outranks fresh video.
5. The first landing SHALL define per-class behavior for `WouldBlock` and `HardError`, including whether packets are retained, dropped, retried, or counted as sent.
6. Video pacing SHALL stop using a fixed packet-count burst as its primary send budget and SHALL instead use a byte-based pacing budget derived from active transport hints.
7. Upper-layer QoS actions SHALL remain the source of encoder/source adaptation decisions; this change SHALL not fork or replace the accepted QoS controller semantics.
8. The design SHALL preserve a clear extension path for later TWCC/feedback-driven transport estimation and probing without another major transport rewrite.

## Non-Functional Requirements

- Keep the first landing reviewable and incremental.
- Preserve existing plain-client publish/session contracts.
- Keep the network thread as the sole owner of raw UDP send execution.
- Avoid mixing high-level QoS policy logic into the transport-control execution layer.
- Add enough diagnostics and tests to reason about backpressure and hard send failures in production and in CI.
- Phase 1 SHALL include a rollback path that can disable the new transport controller and fall back to the current fixed-burst sender behavior during development if verification reveals regressions before merge.
- Phase 1 observability SHALL include explicit counters or logs for:
  - `WouldBlock` occurrences by packet class
  - hard send failures by packet class and errno
  - audio deadline drops
  - queued video retention or discard events

## Edge Cases And Failure Cases

- nonblocking UDP send returns `EINTR`
- nonblocking UDP send returns `EAGAIN` / `EWOULDBLOCK`
- kernel send buffer pressure returns `ENOBUFS`
- oversize datagram or invalid socket state returns `EMSGSIZE` / `EINVAL` / similar hard errors
- startup comedia probing encounters temporary backpressure
- RTCP sender-report transmission is temporarily blocked repeatedly
- NACK retransmission attempts occur while the socket is under backpressure
- video is paused/resumed while packets are already queued for send
- future feedback-driven estimation arrives after the first-phase controller boundary is in place

## Phase-1 Decisions

- Audio SHALL use a bounded deadline-aware pending queue in phase 1 rather than immediate best-effort send or a full shared media scheduler.
- `ENOBUFS` SHALL be treated as temporary backpressure (`WouldBlock`) in phase 1, with explicit diagnostics so the classification can be revisited later if deployment evidence requires it.
- Transport hints SHALL be driven in phase 1 only by upper-layer per-track target bitrate and paused/active state; local/server-observed send signals remain QoS/controller inputs but SHALL NOT directly rewrite transport hints in the first landing.

## Open Questions

- None for the phase-1 change scope. Deferred work remains in later phases of the design rather than as blocking open questions for this landing.
