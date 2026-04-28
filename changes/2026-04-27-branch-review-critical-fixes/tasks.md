# Tasks: Branch Review Critical Fixes

## Tier 1 — Must Fix

### Task 1: Fix Recorder timestamp unwrap state corruption (F1)

**Outcome**: `unwrapTimestamp` always updates `lastTs` and `wrapCount` before returning; no early-return path leaves state inconsistent.

**Files**:
- `src/Recorder.cpp` (lines 459-474)

**Steps**:
1. Replace the early-return backward-wrap branch with a `wrapCount--` + fallthrough to `lastTs = ts`
2. Add unit test in `tests/test_review_fixes.cpp`:
   - Forward wrap: `lastTs=0xFFFFFFF0, ts=0x00000010` -> wrapCount increments
   - Backward wrap: `lastTs=0x00000010, ts=0xFFFFFFF0` with `wrapCount=1` -> wrapCount decrements, lastTs updated
   - Multiple sequential forward wraps
   - Out-of-order packet after wrap (backward jump < half-range, no wrapCount change)
3. Build and run: `cmake --build build --target mediasoup_review_fix_tests && ./build/mediasoup_review_fix_tests --gtest_filter=*UnwrapTimestamp*`

**Verification**: Unit tests pass; existing recording tests pass.

---

### Task 2: Return error for missing producer in pauseProducer/resumeProducer (F2)

**Outcome**: `pauseProducer` and `resumeProducer` return `{false, {}, "", "producer not found"}` when `getProducerById` returns null.

**Files**:
- `src/RoomServiceMedia.cpp` (lines 507-525)

**Steps**:
1. Add null check for `producer` after `getProducerById` in both methods, returning error on null
2. Add unit test in `tests/test_review_fixes.cpp`:
   - `pauseProducer` with non-existent producerId returns error
   - `resumeProducer` with non-existent producerId returns error
3. Build and run: `cmake --build build --target mediasoup_review_fix_tests && ./build/mediasoup_review_fix_tests --gtest_filter=*PauseResumeMissingProducer*`

**Verification**: Unit tests pass; existing QoS integration tests pass.

---

### Task 3: Add byte budget to legacy pacingFlush (F4)

**Outcome**: `pacingFlush()` respects a byte budget derived from pacing bitrate; no burst beyond budget per tick.

**Files**:
- `client/NetworkThread.h` (lines 845-859, and related members)

**Steps**:
1. Add `int64_t pacingBitrateBps_ = 0;` member to `NetworkThread` (or locate existing equivalent)
2. Wire `pacingBitrateBps_` from `TrackTransportConfig` updates in the control command handler (line 694-699)
3. Modify `pacingFlush()` to compute `budgetBytes = (pacingBitrateBps_ * kPacingIntervalMs) / 8000` and break when `spentBytes + e.len > budgetBytes`
4. Add integration test in `tests/test_thread_integration.cpp`:
   - Configure legacy pacing with known bitrate
   - Verify that bytes sent per pacing tick do not exceed budget
5. Build and run: `cmake --build build --target mediasoup_thread_integration_tests && ./build/mediasoup_thread_integration_tests --gtest_filter=*LegacyPacingBudget*`

**Verification**: Integration test passes; existing plain client tests pass.

---

### Task 4: Guard WorkerThread* in deferred lambda with stopping_ check (F6)

**Outcome**: Deferred lambda in `SignalingServerWs.cpp:446` checks `wt->stopping_` before dereferencing; use-after-free is prevented during shutdown.

**Files**:
- `src/SignalingServerWs.cpp` (lines 446-499)

**Steps**:
1. Add `if (wt->stopping_.load(std::memory_order_acquire)) return;` at the top of the deferred lambda (after the capture list, before any `wt->` dereference)
2. Verify `WorkerThread::stopping_` is accessible from `SignalingServerWs.cpp` (it is a public `std::atomic<bool>` member at `WorkerThread.h:138`)
3. Build and run: `cmake --build build --target mediasoup_integration_tests && ./build/mediasoup_integration_tests`
4. Verify existing shutdown test path still passes

**Verification**: Build succeeds; existing integration tests pass.

---

## Tier 2 — Secondary Fixes

### Task 5: Advance round-robin index past paused tracks (F7)

**Outcome**: `FlushFreshVideoQueues` advances `nextVideoTrackIndex_` even when no packet was sent from the starting range; avoids O(n) re-scan of paused prefix.

**Files**:
- `client/SenderTransportController.h` (lines 441-489)

**Steps**:
1. After the inner for-loop, if `!sentPacket`, advance `nextVideoTrackIndex_` and break
2. Add unit test in `tests/test_thread_model.cpp`:
   - 3 tracks, track 0 paused, verify track 1 gets priority on next tick without re-scanning track 0
3. Build and run: `cmake --build build --target mediasoup_thread_model_tests && ./build/mediasoup_thread_model_tests --gtest_filter=*RoundRobin*`

**Verification**: Unit test passes; existing thread model tests pass.

---

### Task 6: Guard handleWorkerDeath against double-reap (F3)

**Outcome**: `handleWorkerDeath` uses `WNOHANG` in detached thread; logs gracefully if child already reaped; asserts non-threaded mode.

**Files**:
- `src/Worker.cpp` (lines 384-405)

**Steps**:
1. Add `MS_ASSERT(!threaded_, "handleWorkerDeath called in threaded mode")` at top
2. Replace blocking `waitpid(p, &status, 0)` with `WNOHANG` + brief retry + `ECHILD` handling
3. Build and run: `cmake --build build --target mediasoup_integration_tests && ./build/mediasoup_integration_tests`

**Verification**: Build succeeds; existing integration tests pass.

---

### Task 7: Fix TOCTOU race in netem_guard stale lock removal (F11)

**Outcome**: `removeStaleLock` uses atomic rename to close the check-then-remove gap.

**Files**:
- `tests/qos_harness/netem_guard.mjs` (lines 87-91)

**Steps**:
1. Replace `fs.rmSync(lockPath, ...)` with `fs.renameSync(lockPath, retirePath)` + `fs.rmSync(retirePath, ...)`
2. Handle `ENOENT` (already gone) and `ENOTEMPTY` (re-acquired) gracefully
3. Extend `tests/qos_harness/test.netem_guard.mjs` with a test for concurrent acquire after stale removal
4. Run: `node --test tests/qos_harness/test.netem_guard.mjs`

**Verification**: Test passes.

---

### Task 8: Add assertion for tracks_ lifecycle (F5)

**Outcome**: `registerVideoTrack` asserts that it is called before `start()`; the contract is documented in a comment.

**Files**:
- `client/NetworkThread.h` (lines 143-161)

**Steps**:
1. Add `assert(!running_ && "registerVideoTrack must be called before start()");` at top of `registerVideoTrack`
2. Add comment on `findTrack()` noting that returned pointer is valid only while `tracks_` is not modified
3. Build and run: `cmake --build build --target mediasoup_thread_integration_tests && ./build/mediasoup_thread_integration_tests`

**Verification**: Build succeeds; existing tests pass.

---

## Task Dependency Graph

```
Task 1 (F1) ─── independent
Task 2 (F2) ─── independent
Task 3 (F4) ─── independent
Task 4 (F6) ─── depends on confirming WorkerThread::stopping_ is accessible (trivial)
Task 5 (F7) ─── independent
Task 6 (F3) ─── independent
Task 7 (F11) ─── independent
Task 8 (F5) ─── independent
```

Tasks 1-4 can be parallelized. Tasks 5-8 can be parallelized with each other and with Tasks 1-4.

## Execution Order

1. Tasks 1, 2, 3, 4 in parallel (Tier 1)
2. Full test suite run after Tier 1 is complete
3. Tasks 5, 6, 7, 8 in parallel (Tier 2)
4. Full test suite run after Tier 2 is complete
5. Final verification: `scripts/run_all_tests.sh`
