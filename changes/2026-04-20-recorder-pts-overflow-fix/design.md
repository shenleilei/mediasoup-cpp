# Design: Recorder RTP delta widening

## Context

Current recorder PTS calculation computes RTP delta as:

- `static_cast<int32_t>(ts - baseTs)`

This causes signed overflow once delta exceeds `INT32_MAX`.

## Goals

- Preserve existing modulo-32 RTP subtraction behavior.
- Remove signed truncation risk.
- Keep change minimal and localized.

## Approach

1. Add a small helper in `PeerRecorder`:
   - `rtpTicksSinceBase(uint32_t ts, uint32_t baseTs) -> uint64_t`
   - Compute as `static_cast<uint32_t>(ts - baseTs)` then widen to `uint64_t`.
2. Use helper in both audio and video PTS paths before `RescaleQ`.
3. Add unit tests through `PeerRecorderTestAccess` for boundary and wraparound cases.

## Why this is safe

- RTP timestamp subtraction is already modulo-32 arithmetic due to unsigned types.
- Widening after modulo subtraction avoids signed reinterpretation while preserving existing wraparound semantics.
- Scope is limited to two call sites in recorder PTS calculation.

## Non-goals

- Redesigning recorder clock model.
- Multi-wrap (>2^32 ticks) absolute unwrapping changes in this patch.

## Verification

- Build and run `mediasoup_qos_unit_tests` and `mediasoup_tests`.
