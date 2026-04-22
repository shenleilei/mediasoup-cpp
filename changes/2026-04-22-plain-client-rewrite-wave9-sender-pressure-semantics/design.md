# Bugfix Design

## Context

The browser path and LiveKit-style architecture both rely on a key principle:

- the transport inner loop should first digest low-level sender pressure
- the QoS outer loop should consume a stable congestion semantic, not a pile of raw counters

Plain-client currently lacks that semantic layer.

## Fix Strategy

### 1. Introduce a sender-pressure semantic

Add a dedicated sender-pressure concept to the transport/runtime model, for example:

- none
- early-warning
- congested

It should be based on sender-side facts such as:

- persistent fresh-video backlog
- pacing clamp relative to aggregate target
- sustained send under-run
- optional retransmission / would-block pressure

The final implementation should keep two separate ideas:

- `warning`: sustained sender backlog under elevated RTT/jitter even when transport estimate is not explicitly clamped yet
- `congested`: stronger transport-owned evidence such as pacing clamp / would-block retention

### 2. Keep the signal inner-loop-owned

The transport inner loop should decide the sender-pressure state.

The outer loop should only consume it, similar to how JS consumes browser-produced
`qualityLimitationReason=bandwidth`.

### 3. Expose it through the canonical snapshot

Add the semantic to:

- `SenderStatsSnapshot`
- `CanonicalTransportSnapshot`

Then update `deriveSignals()` so `bandwidthLimited` can be triggered by:

- browser-style bandwidth limitation reason, or
- the strong (`congested`) sender-pressure semantic from the inner loop

The weaker sender-pressure warning still participates in the QoS state machine,
but it must not be collapsed into `bandwidthLimited`; otherwise the outer loop
cannot distinguish `early_warning` from `congested`.

### 4. Preserve queued fresh media across generation switches

When encoder recreation bumps `configGeneration`, the network thread should still
preserve already queued fresh video RTP for the same SSRC. Dropping that queue
creates RTP sequence gaps that the remote side reports as loss, which made `J3`
look worse only because plain-client adapted.

### 5. Align threaded runtime behavior with JS controller semantics

The parity review identified three plain-client-only runtime behaviors that must
be removed or aligned:

- the async `confirmAction()` path must start recovery probes with the same
  tuning that the sync path uses
- the default controller runtime must not hide its first reactive samples behind
  an implicit warmup window
- probe activity must not suppress local sampling windows

These are not threshold tweaks. They are runtime semantic mismatches.

## Verification

- build native targets
- run targeted unit tests
- rerun default-timing representative weak-network cases
