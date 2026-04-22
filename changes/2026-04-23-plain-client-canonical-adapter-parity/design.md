# Bugfix Design

## Context

The repo now has two different sender environments:

- browser / JS sender, where canonical metrics come from WebRTC stats
- plain-client sender, where canonical metrics must be synthesized locally

The previous wave fixed behavior by adding sender-pressure and removing
plain-client-only runtime mismatches, but the canonical transport snapshot is
still not built from an equivalent source model.

## Fix Strategy

### 1. Add explicit plain-client transport parity fields

Expose local sender transport equivalents from the threaded runtime, for example:

- sender transport delay / RTT-like pressure
- sender transport jitter / variation pressure
- sender limitation reason

These are plain-client-owned fields. They are not browser fields.

### 2. Keep browser reason separate

Do not overload browser `qualityLimitationReason`.

Instead:

- keep `qualityLimitationReason` as the browser/media-stack field
- add a separate sender/local limitation reason for plain-client

That keeps the cross-runtime contract explicit.

### 3. Build a canonical adapter at the snapshot boundary

At the `SenderStatsSnapshot -> CanonicalTransportSnapshot` boundary:

- copy raw RTCP RR RTT/jitter
- copy plain-client local sender-transport equivalents
- derive an explicit sender limitation reason for plain-client

The transport parity fields are intentionally kept explicit in the snapshot.
They are not blindly merged into canonical `rttMs/jitterMs` after a direct
matrix experiment showed that doing so regresses `J3` as `过强`.

This makes the adapter explicit and reviewable.

### 4. Keep behavior stable

The adapter change must not undo the already-fixed behavior:

- sender-pressure early warning
- generation-switch queue preservation
- async confirm probe parity
- no implicit warmup
- no probe sample suppression

## Verification

- build native targets
- run targeted QoS unit tests for signal derivation / controller behavior
- rerun representative matrix cases `B2`, `BW1`, `R4`, `J3`
