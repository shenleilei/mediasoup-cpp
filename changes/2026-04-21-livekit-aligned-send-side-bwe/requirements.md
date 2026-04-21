# LiveKit-Aligned Send-Side BWE For Plain Client

## Problem Statement

The repository previously had a partial C++ port of LiveKit-style send-side BWE modules under `client/sendsidebwe/` and `client/ccutils/`, but runtime sender semantics, probe isolation, and probe-trigger behavior were not yet aligned on the main path.

This change closes those main-path gaps so the plain client can truthfully claim LiveKit-aligned send-side BWE behavior for its supported transport-estimate path.

## Goal

Keep the plain-client send-side BWE integration aligned so the supported main path follows LiveKit's module boundaries and the relevant sender-side runtime semantics:

- TWCC feedback time-order protection
- packet tracker
- packet groups and propagated queuing delay
- congestion state machine
- estimation of available channel capacity
- probe-related interfaces and state
- transport-wide sequence allocation semantics for retried queued packets
- probe lifecycle and default padding-probe trigger behavior

## In Scope

- C++ equivalents of LiveKit modules under `client/`
- `NetworkThread` integration for transport-wide sequence allocation, packet-send recording, TWCC feedback handling, and probe lifecycle
- `SenderTransportController` queue semantics where they affect transport-wide sequence stability
- RTP/RTCP sender bookkeeping changes needed to isolate probe RTP from ordinary media retransmission semantics
- regression tests covering correctness-critical pieces like out-of-order feedback, reference-time wrap, retry semantics, and probe behavior
- explicit default probe policy aligned to LiveKit's padding-probe path

## Out Of Scope

- exact feature parity with every unrelated LiveKit dependency outside send-side BWE
- browser-side QoS controller changes
- room-level multi-peer coordination
- a literal port of LiveKit's downlink `streamallocator` object model into the plain-client sender
- a new weight-aware multi-track media scheduler under aggregate transport caps
- non-default probing strategies beyond the sender-side equivalent of LiveKit's default padding-probe behavior

## Sender-Side Equivalence Boundary

LiveKit computes probe goals inside a downlink allocator with access to deficient tracks and next-higher-layer transitions.

The plain client is an uplink sender and does not have that same object model. For this change, "aligned with LiveKit" means:

- the BWE core and probe lifecycle follow the LiveKit send-side BWE implementation
- the probe trigger math follows the default padding-probe policy using a sender-side equivalent mapping
- carrier-track selection is sender-side specific, but deterministic and documented

It does not mean introducing LiveKit's full downlink allocation context where the plain client has no equivalent runtime objects.

## Acceptance Criteria

1. A queued RTP packet SHALL keep a stable transport-wide sequence number across local retries after `WouldBlock` until that packet is either successfully sent or discarded.
2. Only packets that were actually sent successfully SHALL be recorded into the send-side BWE sent-packet tracker.
3. Parsed TWCC feedback SHALL continue to handle out-of-order feedback reports and transport-cc reference-time wrap on the supported main path.
4. Probe RTP packets SHALL NOT enter the ordinary video retransmission packet store or ordinary media NACK retransmission path.
5. Probe RTP packets SHALL use transport-cc sequence allocation and SHALL still be observable by the probe observer / probe completion logic.
6. The network thread SHALL support probe early-stop when the send-side BWE reports that the active probe cluster goal has been reached.
7. The default probe-trigger math SHALL follow the sender-side equivalent of LiveKit's default padding-probe policy:
   - `availableBandwidthBps` from the published transport estimate
   - `expectedUsageBps` from current sender usage
   - `desiredIncreaseBps = max(transitionDeltaBps * ProbeOveragePct / 100, ProbeMinBps)`
   - `desiredBps = expectedUsageBps + desiredIncreaseBps`
8. The default probe configuration SHALL remain safe-by-default and rollbackable through the existing transport-estimate enablement controls.
9. New or changed sender semantics SHALL have targeted unit or integration regression coverage for:
   - retry-after-`WouldBlock` transport sequence stability
   - probe early-stop on goal reached
   - probe isolation from ordinary retransmission bookkeeping
   - LiveKit-aligned probe goal construction
