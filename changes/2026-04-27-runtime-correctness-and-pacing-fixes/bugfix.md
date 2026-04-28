# Bugfix Plan: Runtime Correctness And Pacing Fixes

## Symptom
Several runtime and transport support paths still contain correctness or pacing defects:

- producer pause/resume APIs report success even when the target producer does not exist
- legacy pacing path can burst packets without a byte-budget guard
- WebSocket room join defers capture `WorkerThread*` without a strong lifetime guarantee
- round-robin fresh-video flushing rescans paused/empty tracks inefficiently
- supporting runtime guards such as netem stale-lock handling still have race windows

## Reproduction
1. Inspect `src/RoomServiceMedia.cpp` pause/resume producer helpers on missing producer.
2. Inspect `client/NetworkThread.h` legacy `pacingFlush()` path.
3. Inspect `src/SignalingServerWs.cpp` deferred join response lambda capture set.
4. Inspect `client/SenderTransportController.h` fresh-video queue scan behavior.
5. Inspect `tests/qos_harness/netem_guard.mjs` stale lock deletion path.

## Observed Behavior
- pause/resume producer operations silently succeed on missing producer.
- Legacy pacing sends a fixed packet burst without a pacing byte budget.
- Deferred lambdas rely on raw `WorkerThread*` lifetime assumptions.
- Fresh-video queue scan rechecks paused/empty tracks repeatedly.
- netem stale lock deletion is vulnerable to stale-check/remove races.

## Expected Behavior
- Producer pause/resume SHALL fail explicitly when the producer is missing.
- Legacy pacing SHALL respect a byte budget per tick.
- Deferred room-join response handling SHALL not depend on raw pointer lifetime assumptions.
- Round-robin queue scans SHALL advance fairly and avoid needless rescans.
- netem stale lock cleanup SHALL avoid deleting a lock that another process just acquired.

## Suspected Scope
- `src/RoomServiceMedia.cpp`
- `client/NetworkThread.h`
- `src/SignalingServerWs.cpp`
- `client/SenderTransportController.h`
- `tests/qos_harness/netem_guard.mjs`
- Related tests under `tests/`

## Acceptance Criteria
- Missing producer pause/resume returns explicit failure and is covered by regression tests.
- Legacy pacing path uses a byte budget guard rather than a raw packet burst limit alone.
- Deferred join response handling no longer captures a raw `WorkerThread*` without a strong lifetime strategy.
- Fresh-video queue scan advances fairly across paused/empty tracks.
- netem stale lock cleanup is hardened against stale-check/remove races.

## Regression Expectations
- Existing recorder, signaling, and QoS regressions continue to pass.
