# LiveKit-Aligned Send-Side BWE For Plain Client

## Problem Statement

The current plain-client TWCC estimator is functional but remains a simplified local design. It does not align structurally or behaviorally with LiveKit's send-side BWE implementation in `/root/livekit/pkg/sfu/bwe/sendsidebwe`.

## Goal

Port the LiveKit send-side BWE architecture into the plain client C++ sender path so the client follows the same module boundaries and core estimation logic:

- TWCC feedback time-order protection
- packet tracker
- packet groups and propagated queuing delay
- congestion state machine
- estimation of available channel capacity
- probe-related interfaces and state

## In Scope

- C++ equivalents of LiveKit modules under `client/`
- `NetworkThread` integration for transport-wide sequence allocation, packet-send recording, and TWCC feedback handling
- regression tests covering correctness-critical pieces like out-of-order feedback and reference-time wrap

## Out Of Scope

- exact feature parity with every unrelated LiveKit dependency outside send-side BWE
- browser-side QoS controller changes
- room-level multi-peer coordination
