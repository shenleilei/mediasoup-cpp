# Bugfix: Branch Review Critical Fixes

## Symptom

Branch `codex/04-20-functional-refactors` had multiple correctness defects and latent risks identified during full branch-to-main review. The defects span recording timestamps, producer lifecycle signaling, client pacing, and server-side pointer lifetime.

## Fix Status

| ID | Description | Status | Fixed In |
|----|-------------|--------|----------|
| F1 | Recorder `unwrapTimestamp` early return leaves state inconsistent | **Fixed** | This change |
| F2 | `pauseProducer`/`resumeProducer` silent success on missing producer | **Fixed** | `e624362` |
| F4 | Legacy `pacingFlush()` no byte budget | **Fixed** | `e624362` |
| F6 | `WorkerThread*` raw pointer capture in deferred lambda | **Fixed** | `e624362` |
| F7 | Round-robin index not advanced on paused/empty tracks | **Fixed** | `e624362` |
| F3 | `handleWorkerDeath` no threaded-mode guard | **Fixed** | `e624362` |
| F5 | No assertion preventing post-start `registerVideoTrack` | **Fixed** | `e624362` |
| F8 | `WsClient::close()` race conditions | **Fixed** | `b470211` |
| F11 | netem_guard stale lock TOCTOU | **Mitigated** | `e624362` (re-read metadata; rename-based fix deferred) |

## F1 Detail: Recorder timestamp unwrap state corruption

### Reproduction
- Any recording session where RTP timestamps wrap around (90kHz clock wraps at ~47.7s; 8kHz audio wraps at ~537s)
- Backward-wrap branch in `unwrapTimestamp` returned early without updating `lastTs`, corrupting all subsequent timestamps
- Observable as garbled playback (frames out of order, A/V desync, or ffmpeg muxer rejection)

### Root Cause

`src/Recorder.cpp:459-474`: The backward-wrap branch (`ts >= lastTs && ts - lastTs > 0x80000000 && wrapCount > 0`) returned early with `((wrapCount - 1) << 32) + ts` without updating `lastTs`. Subsequent calls re-entered with stale `lastTs`, producing incorrect timestamps.

### Fix

Removed the early return. The backward-wrap branch now decrements `wrapCount` and falls through to the unified `lastTs = ts` update:

```cpp
if (ts - lastTs > 0x80000000 && wrapCount > 0) {
    wrapCount--;  // was: early return with (wrapCount-1)
}
lastTs = ts;  // always updated
uint64_t ticks = (wrapCount << 32) + ts;
return ticks >= baseTs ? ticks - baseTs : 0;
```

Also moved `unwrapTimestamp` from `private` to `public` in `Recorder.h` (it is a pure static function with no `this` dependency) to enable direct unit testing.

### Test Coverage

5 new test cases in `tests/test_review_fixes.cpp`:
- `UnwrapTimestamp.ForwardWrapUpdatesState` — wrapCount increments, lastTs updated
- `UnwrapTimestamp.BackwardWrapUpdatesLastTsAndWrapCount` — the F1 bug case: lastTs must be updated, wrapCount decremented
- `UnwrapTimestamp.BackwardWrapGuardedAtWrapCountZero` — no underflow below zero
- `UnwrapTimestamp.SmallBackwardJumpNoWrap` — genuine reorder within half-range
- `UnwrapTimestamp.MonotonicSequenceNoWrap` — normal increasing sequence

### Regression Expectations

- 182 unit tests pass (including 5 new)
- 24 review-fix integration tests pass
- Existing recording and QoS tests pass
