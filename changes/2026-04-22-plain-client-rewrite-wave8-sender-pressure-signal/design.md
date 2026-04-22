# Bugfix Design

## Context

The browser / JS QoS pipeline consumes browser-produced congestion semantics like
`qualityLimitationReason=bandwidth`.

Plain-client has no browser sender, so it must synthesize an equivalent signal
from its own transport inner loop.

## Fix Strategy

### 1. Keep browser semantics intact

Do not overload `qualityLimitationReason` with plain-client-only meaning.
That field should continue to represent browser/media-stack limitation reason.

### 2. Add explicit sender-side pressure to the canonical snapshot

Expose a boolean sender-side pressure signal in:

- `SenderStatsSnapshot`
- `CanonicalTransportSnapshot`

This signal is derived in the transport inner loop from sender-side facts such as:

- persistent fresh-video backlog
- pacing clamp relative to aggregate target

### 3. Let outer QoS consume the explicit signal

Update `deriveSignals()` so `bandwidthLimited` becomes true when either:

- browser-style `qualityLimitationReason=bandwidth` and low utilization, or
- explicit sender-side bandwidth pressure is present

This keeps browser behavior unchanged while giving plain-client a first-class
equivalent to browser bandwidth limitation.

## Verification

- build native targets
- run targeted QoS unit tests
- rerun default-timing `B2`, `BW1`, `R4`, `J3`
