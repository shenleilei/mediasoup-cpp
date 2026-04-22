# Bugfix Analysis

## Summary

After wave 8 investigations:

- estimate overreaction in the send-side BWE path is identified and partially mitigated
- some default-timing weak-network cases recover after that mitigation
- remaining failures such as `R4` / `J3` show that the outer loop still does not consume a browser-equivalent sender-side congestion semantic
- `J3` also exposed a separate runtime bug: generation-switch handling was dropping already queued fresh video packets, which created artificial RTP sequence gaps and self-inflicted loss spikes after a downgrade action
- JS parity review also exposed three runtime mismatches after the sender-pressure fix:
  - the threaded async `confirmAction()` path was not carrying the same probe/recovery tuning as the sync path
  - plain-client still kept a default `warmupSamples=5`, which JS does not
  - threaded plain-client was suppressing local samples around probe activity, which JS does not

The missing layer is a stable sender-pressure signal between the transport inner loop and the QoS outer loop.

## Observed Behavior

- browser / JS QoS consumes browser-produced congestion semantics such as `qualityLimitationReason=bandwidth`
- plain-client outer loop still depends mostly on raw RTT / jitter / loss plus browser-style bandwidth reason
- plain-client inner-loop sender stress can exist without being translated into a stable outer-loop input

## Expected Behavior

- plain-client transport inner loop SHALL produce a first-class sender-side congestion semantic
- outer-loop QoS SHALL consume that semantic as the plain-client equivalent of browser bandwidth limitation
- the signal SHALL be more stable than raw queue length and more specific than raw RTT/jitter snapshots
- encoder generation switches SHALL NOT discard already queued fresh video RTP packets for the same SSRC; otherwise plain-client creates synthetic loss that browser/JS sender adaptation does not create
- the threaded async runtime SHALL use the same recovery-probe tuning as the main QoS controller path
- plain-client SHALL NOT keep an implicit warmup delay that JS QoS does not keep
- plain-client SHALL NOT drop local sampling windows just because a probe is active

## Acceptance Criteria

- a distinct sender-pressure semantic exists in the plain-client transport/runtime path
- the semantic is derived in the inner loop, not improvised directly in `deriveSignals()`
- default-timing representative cases `B2`, `BW1`, `R4`, and `J3` are rechecked against the new semantics
- generation-switch handling is rechecked so downgrade actions no longer manufacture the loss spike that previously pushed `J3` from `early_warning` into `congested`
- async `confirmAction()` parity, default warmup, and probe sampling behavior are rechecked against JS-oriented expectations
