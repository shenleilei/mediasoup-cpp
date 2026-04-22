# Design

## Context

The repository already contains:

- current status and accepted scope for plain-client QoS
- TWCC / transport-control alignment study against LiveKit
- a boundary review that highlights the current contract risks

What is still missing is a rewrite-first roadmap that says:

- what the clean target architecture is
- what old runtime code should not be preserved
- how cutover should happen
- how to sequence the rewrite without leaving compatibility seams behind

## Chosen Output

Add:

- `docs/qos/plain-client/livekit-alignment-plan_cn.md`

and link it from:

- `docs/qos/plain-client/README.md`

Track the work through:

- `changes/2026-04-22-plain-client-livekit-alignment-plan/`

## Target Architecture To Describe

The document should describe a replacement architecture, not an adaptation layer.

### 1. Transport Inner Loop Is The Primary Truth

The canonical sender transport state should come from local transport-path components:

- `NetworkThread`
- `RtcpHandler`
- `SendSideBwe`
- `SenderTransportController`

This layer owns:

- packet send history
- TWCC feedback handling
- RTT / jitter / retransmission observations
- transport estimate / pacing authority

### 2. QoS Policy Is The Outer Loop

The QoS state machine remains useful, but it should not act as the transport estimator itself.

Its job is to decide:

- quality level
- audio-only mode
- pause/resume
- source/encoder adaptation

based on already normalized transport signals.

### 3. Observation Sources Are Separate From Control Sources

The plan should explicitly demote sources like server `getStats` to:

- observation
- debugging
- reporting
- reconciliation

instead of using them as first-class control inputs.

### 4. Boundary Normalization Must Be Centralized

The document should call for one shared normalization layer that:

- defines sender counter contracts
- handles source switches intentionally
- removes duplicated `lossBase`-style logic from both legacy and threaded paths over time

### 5. Old Runtime Paths Are Disposable

The document should explicitly say:

- legacy sender QoS logic is not a long-term compatibility target
- current duplicated boundary code should be treated as temporary reference material
- once the new path is feature-complete, the old runtime path should be deleted rather than wrapped

## Phasing Strategy

The document should structure the implementation as 4 to 6 phases with:

- objective
- files/modules
- intended outcome
- validation focus
- deletion/cutover expectations when applicable

The phases should move from:

1. contract definition
2. new-path construction
3. new-path cutover
4. old-path deletion

instead of gradual compatibility layering.

## Scope Control

This plan should explicitly reject:

- exact byte-for-byte imitation of LiveKit internals as a first objective
- mixing broad worker upgrade work into the same roadmap
- using report green-ness alone as proof that the architecture is sound
- preserving compatibility wrappers after the new path is ready

## Verification Strategy

The planning change itself is docs-only.

Verification should cover:

- `git diff --check`
- manual read-through
- confirm the new plan is linked from the plain-client QoS doc entrypoint

## Risks

- the plan could become too abstract to implement from
- the plan could describe a rewrite but fail to define a real cutover/deletion point
- the plan could overpromise alignment with LiveKit beyond what is necessary

## Mitigation

- keep each phase tied to concrete modules
- state non-goals explicitly
- define the cutover/deletion point explicitly
- frame alignment as architectural/behavioral alignment, not clone-the-code alignment
