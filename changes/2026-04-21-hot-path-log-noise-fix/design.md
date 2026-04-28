# Design: Demote normal hot-path tracing

## Goals

- Reduce warning-channel noise from expected control-plane activity.
- Preserve warning visibility for real failures and degraded operation.

## Approach

1. Demote normal `/api/resolve` request lifecycle logs from `warn` to `debug`.
2. Demote normal `RoomRegistry` maintenance and resolve tracing from `warn` to `debug`.
3. Keep warnings for:
   - Redis unavailable / disconnect / EVAL failure
   - stale redirect cleanup
   - unparseable owner node values
   - unexpected claim results

## Why this is safe

- The change does not alter control flow or data paths.
- It only adjusts operator-facing severity for expected traffic.
- Debug-level traces remain available when diagnosis is needed.

## Non-goals

- Structured logging redesign.
- Changing message text or adding new log fields unless needed.

## Verification

- Build `mediasoup-sfu` and `mediasoup_tests`.
- Spot-check diff to ensure failure warnings remain at `warn`.
