# Bugfix Analysis

## Summary

After wave 7:

- threaded-only runtime is in place
- estimate overreaction in send-side BWE can be mitigated by delaying downward estimate updates

But remaining weak-network cases such as `R4` / `J3` still fail as `过弱`.

The remaining mismatch is that the outer QoS loop still depends on browser-style
`qualityLimitationReason=bandwidth` semantics, while plain-client's sender-side
pressure shows up first as queue/pacing stress inside the local transport runtime.

## Observed Behavior

- `SenderStatsSnapshot` can already observe sender-side queue/pacing pressure
- `deriveSignals()` still only treats browser-style `qualityLimitationReason=bandwidth`
  as a formal bandwidth-pressure trigger
- pure sender-side queue pressure therefore does not reliably enter the outer loop

## Expected Behavior

- plain-client SHALL expose an explicit sender-side bandwidth-pressure signal from
  the transport inner loop
- the QoS outer loop SHALL treat that signal as the plain-client equivalent of
  browser-side bandwidth limitation

## Acceptance Criteria

- `CanonicalTransportSnapshot` carries an explicit sender-side pressure flag
- `deriveSignals()` treats that flag as `bandwidthLimited`
- default-timing targeted cases `B2`, `BW1`, `R4`, and `J3` are rechecked after the change
