# Bugfix Analysis

## Summary

After the sender-pressure and runtime-parity fixes:

- representative weak-network cases `B2`, `BW1`, `R4`, and `J3` now pass
- but plain-client still does not fully align with the JS QoS snapshot contract

The remaining gaps are structural:

- JS sender QoS prefers candidate-pair RTT and fresh remote jitter
- plain-client still feeds raw RTCP RR RTT/jitter directly into the canonical snapshot
- plain-client has no browser `qualityLimitationReason`, so it still lacks an explicit local equivalent for sender limitation reason

This wave addresses the structure first:

- export explicit local sender-transport parity fields
- export an explicit sender limitation reason
- keep those fields separate from browser-only fields

Directly folding local transport delay/jitter into canonical `rttMs/jitterMs`
was tested and rejected in the same wave because it regressed `J3` as `过强`.

## Observed Behavior

- JS `ProducerSenderStatsProvider` derives canonical sender transport metrics from:
  - candidate-pair RTT first
  - remote-inbound RTT/jitter as fallback
  - outbound `qualityLimitationReason`
- plain-client currently derives canonical sender transport metrics from:
  - RTCP RR RTT
  - RTCP RR jitter
  - sender-pressure as a separate side-channel

This makes runtime behavior closer, but the canonical snapshot itself still does
not describe transport truth the same way JS does.

## Expected Behavior

- plain-client SHALL expose explicit local sender-transport equivalents for:
  - transport delay / RTT-like pressure
  - transport jitter / variation pressure
  - sender limitation reason
- canonical sender QoS derivation SHALL consume the explicit sender limitation
  semantics without overloading browser-only fields
- explicit local sender-transport timing fields SHALL be available for review,
  trace, and future parity work without destabilizing the current QoS state machine
- browser-only fields SHALL remain browser-only; plain-client SHALL not silently
  overload them with a different meaning

## Acceptance Criteria

- the threaded runtime exports explicit local sender-transport parity metrics
- `CanonicalTransportSnapshot` carries those explicit local equivalents
- `deriveSignals()` consumes the explicit sender limitation reason without
  overloading browser-only fields
- representative matrix cases remain stable after the adapter change
