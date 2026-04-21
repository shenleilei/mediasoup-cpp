# Plain Client Delay-Based TWCC BWE

## Problem Statement

Phase 1 wired the plain client into the `transport-cc` feedback loop and proved that real worker-generated TWCC feedback can reach the sender and influence pacing. However, the phase-1 estimate path still uses packet-loss heuristics rather than packet-arrival delay trends reconstructed from TWCC feedback.

That leaves the plain client with a functional feedback loop but without the core advantage of TWCC-driven send-side BWE:

- congestion is detected mainly after loss has already appeared
- upward and downward estimate movement is only loosely tied to actual packet-arrival timing
- the sender transport controller still lacks a dedicated delay-based estimate core comparable in role to WebRTC/LiveKit send-side BWE

## Business Goal

Introduce a minimal delay-based TWCC send-side bandwidth estimator for the Linux plain client so that pacing decisions react to packet-arrival timing trends rather than only aggregate loss ratios.

## In Scope

- detailed TWCC feedback parsing under `client/`
- sent-packet history needed to map TWCC sequence numbers to local send timestamps and packet sizes
- a minimal delay-based send-side BWE module that:
  - reconstructs received packet timing from TWCC feedback
  - derives arrival-vs-send delay trend samples
  - derives an acknowledged bitrate sample window
  - updates a transport estimate with clamp and startup/bootstrap behavior
- `NetworkThread` integration so `SenderTransportController` pacing uses the new estimate path
- deterministic unit and thread-integration coverage for the new estimator behavior

## Out Of Scope

- full libwebrtc GCC parity
- full probe controller / probe cluster orchestration
- rewriting upper-layer QoS controller logic
- room-level multi-peer congestion coordination
- replacing the accepted packet-class scheduling policy introduced in phase 1

## Scenarios

### Scenario 1: delay growth triggers estimate decrease before heavy loss

When TWCC feedback shows receive-time deltas growing relative to send-time deltas, the sender estimate decreases even if loss is still low.

### Scenario 2: stable feedback allows bounded estimate increase

When TWCC feedback shows stable receive timing and acknowledged throughput remains healthy, the sender estimate increases gradually within configured bounds.

### Scenario 3: malformed or non-actionable feedback is safe

When feedback is malformed or cannot be correlated with recent sent packets, the estimate remains unchanged and diagnostics remain explicit.

## Acceptance Criteria

1. The plain client SHALL parse TWCC feedback into per-packet sequence and receive-time observations, not just received/lost counters.
2. The plain client SHALL maintain sent-packet history for successfully transmitted transport-cc RTP packets so TWCC feedback can be correlated with local send times.
3. The new estimator SHALL use receive-time vs send-time trend samples as a primary congestion signal.
4. The new estimator SHALL derive an acknowledged bitrate sample window and use it in estimate updates.
5. The transport estimate SHALL remain clamped to configured min/max bounds.
6. `SenderTransportController` pacing SHALL consume the new estimate path without regressing the explicit packet-class scheduling contract from phase 1.
7. Tests SHALL cover at least:
   - per-packet TWCC feedback parsing
   - delay-growth driven estimate decrease
   - stable-delay estimate increase
   - clamp-to-min and malformed-feedback no-op behavior

## Non-Functional Requirements

- Keep the estimator small, explicit, and reviewable.
- Keep all raw UDP send execution in `NetworkThread`.
- Preserve existing phase-1 fallback behavior for packet scheduling and queue policy.
- Do not require an external worker or browser stack to unit-test core estimator logic.

## Open Questions

- Active probing remains deferred to a later follow-up once the delay-based estimator itself is verified.
