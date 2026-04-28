# Design: Branch Review Critical Fixes

## Context

Full branch review of `codex/2026-04-20-functional-refactors` against `main` identified 4 must-fix and 5 secondary defects. The fixes must be narrow, traceable to the specific bug, and verified by tests that cover the failure mode.

## Proposed Approach

### F1: Fix `unwrapTimestamp` early return (Recorder.cpp:459-474)

**Root cause**: The backward-wrap branch (`ts >= lastTs && ts - lastTs > 0x80000000 && wrapCount > 0`) returns early without updating `lastTs` or adjusting `wrapCount`. Subsequent calls re-enter with stale state.

**Fix**: Remove the early return. Always update `lastTs` and compute `wrapCount` consistently before returning. The backward-wrap case should decrement `wrapCount` and set `lastTs = ts` before computing the result.

**New logic**:
```cpp
uint64_t PeerRecorder::unwrapTimestamp(uint32_t ts, uint32_t baseTs,
    uint32_t& lastTs, uint64_t& wrapCount)
{
    if (ts < lastTs) {
        // Apparent backward jump smaller than half-range: genuine backward packet.
        // Apparent backward jump larger than half-range: forward wrap.
        if (lastTs - ts > 0x80000000) {
            wrapCount++;
        }
    } else {
        // Apparent forward jump larger than half-range: backward wrap (reordered
        // or duplicate from previous cycle). Decrement wrapCount only if we have
        // wraps to give back.
        if (ts - lastTs > 0x80000000 && wrapCount > 0) {
            wrapCount--;
        }
    }
    lastTs = ts;
    uint64_t ticks = (wrapCount << 32) + ts;
    return ticks >= baseTs ? ticks - baseTs : 0;
}
```

**Key invariant**: `lastTs` is always updated before return. `wrapCount` is always consistent with `lastTs`.

**Tradeoff**: With `wrapCount--`, a pathological sequence of alternating forward/backward wraps could oscillate. In practice, RTP timestamps are monotonic from the source; backward wraps only happen on extreme out-of-order delivery. The `wrapCount > 0` guard prevents underflow.

**Test**: Unit test with explicit wrap-around sequences:
- Forward wrap: `lastTs=0xFFFFFFF0, ts=0x00000010` -> `wrapCount` increments
- Backward wrap: `lastTs=0x00000010, ts=0xFFFFFFF0, wrapCount=1` -> `wrapCount` decrements
- Multiple forward wraps
- Out-of-order after wrap

---

### F2: Return error for missing producer in pauseProducer/resumeProducer (RoomServiceMedia.cpp:507-525)

**Root cause**: Both methods call `getProducerById` and if it returns null, still return `{true, {}}`.

**Fix**: Return an error result when the producer is not found:
```cpp
RoomService::Result RoomService::pauseProducer(const std::string& roomId,
    const std::string& producerId)
{
    auto room = roomManager_.getRoom(roomId);
    if (!room) return {false, {}, "", "room not found"};
    auto producer = room->router()->getProducerById(producerId);
    if (!producer) return {false, {}, "", "producer not found"};
    producer->pause();
    return {true, {}};
}
```

Same pattern for `resumeProducer`.

**Tradeoff**: Callers that previously silently succeeded now get errors. This is correct behavior -- any caller depending on silent success was operating on a false assumption. The signaling layer already handles error responses to clients.

**Test**: Unit test verifying error return for non-existent producerId.

---

### F4: Add byte budget to legacy pacingFlush (NetworkThread.h:845-859)

**Root cause**: `pacingFlush()` sends up to `kBurstLimit=8` packets per tick with no byte-budget check. The transport-controller path at line 801-802 respects `mediaBudgetBytes_`, but the legacy path does not.

**Fix**: Compute a byte budget from the configured pacing bitrate and the pacing interval, then stop sending when budget is exhausted:

```cpp
void pacingFlush() {
    // Derive byte budget from pacing bitrate and interval.
    // kPacingIntervalMs = 2, pacingBitrateBps_ holds current target.
    const int64_t budgetBytes = (pacingBitrateBps_ * kPacingIntervalMs) / 8000;
    int64_t spentBytes = 0;
    constexpr int kBurstLimit = 8;
    for (int i = 0; i < kBurstLimit && !pacingQueue_.empty(); ++i) {
        auto& e = pacingQueue_.front();
        if (spentBytes + static_cast<int64_t>(e.len) > budgetBytes) break;
        const auto result = sendMediaPacketWithTransportCc(
            mediasoup::plainclient::PacketClass::VideoMedia,
            nullptr,
            e.data,
            e.len);
        if (result.status == mediasoup::plainclient::SendStatus::Sent && e.len >= 12) {
            rtcp_.onVideoRtpSent(e.data, e.len);
        }
        spentBytes += static_cast<int64_t>(e.len);
        pacingQueue_.pop_front();
    }
}
```

**Requirement**: A `pacingBitrateBps_` member must exist (or be added) to `NetworkThread`. Currently the transport controller path manages bitrate internally. For the legacy path, the bitrate is the encoding bitrate set by `TrackTransportConfig`. This value is already available from the track state or from the source worker's encoding parameters.

**Tradeoff**: If `pacingBitrateBps_` is 0 (not configured), the budget is 0 and no packets are sent. This is acceptable -- it means the legacy path requires bitrate to be configured, which is already implied by the pacing timer's existence.

**Alternative considered**: Reuse `SenderTransportController::mediaBudgetBytes_` for the legacy path. Rejected because the transport controller is explicitly not active in legacy mode and its budget state would be uninitialized.

**Test**: Integration test where legacy pacing is enabled, verify that packets sent per tick do not exceed `(bitrate * intervalMs) / 8000` bytes.

---

### F6: Safe WorkerThread access in deferred lambda (SignalingServerWs.cpp:446-499)

**Root cause**: The deferred lambda captures `wt` as a raw `WorkerThread*`. The `WorkerThread` objects are owned by `server.workerThreads_` (a `vector<unique_ptr<WorkerThread>>`). When `SignalingServer::stop()` is called, the uWS event loop exits, then `workerThreads_` is destroyed by the caller. If a deferred lambda runs during or after `stop()`, the `wt` pointer may be dangling.

**Fix**: Use `wt->stopping_` flag as a pre-check before dereferencing `wt`. The `WorkerThread::stopping_` atomic is set to `true` in `WorkerThread::stop()` which runs before the `WorkerThread` is destroyed. Additionally, capture a `weak_ptr` to a shared ownership token.

However, `WorkerThread` is currently owned via `unique_ptr`, not `shared_ptr`. Two approaches:

**Option A (minimal, preferred)**: Check `wt->stopping_` before every dereference in the deferred lambda. Since `WorkerThread::stop()` sets `stopping_ = true` before the thread is joined and the object is destroyed, and the deferred lambda runs on the uWS loop thread which is the same thread that calls `stop()`, the `stopping_` check is safe because:
1. The uWS `loop->defer()` callback runs on the uWS loop thread
2. `SignalingServer::stop()` causes the uWS loop to exit
3. After the loop exits, no more deferred callbacks run
4. So there is a happens-before relationship: `stop()` -> loop exit -> no more defers

This means the risk only exists if `stop()` is called from a different thread than the uWS loop thread. Looking at the code, `stop()` just sets a flag and the shutdown timer on the loop triggers `uWS::Loop::get()->defer([] { us_listen_socket_close(...) })`. So `stop()` does not directly destroy `WorkerThread` objects -- the caller does after `run()` returns.

The actual risk: `run()` returns after `app.run()` exits, then the caller destroys `workerThreads_`. But by that point, no more deferred callbacks can run because the uWS loop has stopped. **The risk is theoretically present but protected by the uWS loop lifecycle.**

To make this provably safe, add an explicit guard:

```cpp
loop->defer([&server, wt, wsMap, ws, alive, respStr = std::move(respStr),
    joinOk, joinFailed, newSessionId,
    jRoomId = std::move(jRoomId), jPeerId = std::move(jPeerId),
    joinQosPolicy = std::move(joinQosPolicy)]
{
    // Guard: if the WorkerThread is stopping, the join result is irrelevant
    // and accessing wt would be unsafe during teardown.
    if (wt->stopping_.load(std::memory_order_acquire)) return;
    // ... rest of lambda unchanged ...
});
```

**Option B (stronger)**: Convert `WorkerThread` ownership to `shared_ptr` and capture `shared_ptr<WorkerThread>` in the lambda. This is a larger refactor that touches `SignalingServer`, `WorkerThread`, and all their users. Not justified for this fix.

**Choice**: Option A. It is minimal, correct under the existing lifecycle model, and makes the invariant explicit.

**Tradeoff**: `stopping_` is a `std::atomic<bool>` on `WorkerThread`. Accessing it from the uWS loop thread is safe (it is atomic). The `stopping_` flag is set before the `WorkerThread` thread is joined, which happens before the `WorkerThread` object is destroyed. So checking `stopping_` is always safe as long as the `WorkerThread` object still exists. Since the uWS loop stops before `WorkerThread` objects are destroyed, this is provably safe.

**Test**: This is a shutdown-time race that is hard to test deterministically. A stress test that rapidly creates/closes rooms while shutting down the server could exercise the path, but the fix is primarily a correctness guard. Verify that the `stopping_` check compiles and the existing shutdown integration test still passes.

---

### Secondary Fixes

#### F7: Advance round-robin index on paused/empty tracks (SenderTransportController.h:451)

**Fix**: After the inner for-loop completes without sending, advance `nextVideoTrackIndex_` to the next track after the starting position. This prevents re-scanning the same paused tracks every tick:

```cpp
void FlushFreshVideoQueues()
{
    if (trackStates_.empty()) return;
    while (mediaBudgetBytes_ > 0) {
        bool sentPacket = false;
        const size_t trackCount = trackStates_.size();
        for (size_t attempt = 0; attempt < trackCount; ++attempt) {
            const size_t index = (nextVideoTrackIndex_ + attempt) % trackCount;
            auto& trackState = trackStates_[index];
            if (trackState.paused || trackState.videoQueue.empty()) continue;
            // ... existing send logic ...
            nextVideoTrackIndex_ = (index + 1) % trackCount;
            sentPacket = true;
            break;
        }
        // Advance past the scanned range even if nothing was sent,
        // so the next tick starts from a different track.
        if (!sentPacket) {
            nextVideoTrackIndex_ = nextVideoTrackIndex_ % trackCount;
            break;  // Nothing to send from any track
        }
    }
}
```

**Tradeoff**: When all tracks are paused, we still break out immediately. The `nextVideoTrackIndex_` stays at the same position, but since nothing changed, re-scanning the same set is unavoidable. The fix only matters when *some* tracks are active -- it prevents the O(n) re-scan of paused prefix tracks on each tick.

#### F3: Guard handleWorkerDeath against double-reap (Worker.cpp:384-402)

**Fix**: Use `WNOHANG` in the detached thread to avoid blocking forever, and log if the child was already reaped:

```cpp
void Worker::handleWorkerDeath() {
    if (closed_.load(std::memory_order_acquire)) return;
    MS_WARN(logger_, "worker pipe closed [pid:{}], delegating to detached reaper", pid_);
    pid_t p = pid_;
    auto l = logger_;
    std::thread([p, l]() {
        int status = 0;
        pid_t ret = ::waitpid(p, &status, WNOHANG);
        if (ret == 0) {
            // Child not yet reaped; wait briefly then try again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ret = ::waitpid(p, &status, 0);
        }
        if (ret == -1 && errno == ECHILD) {
            MS_WARN(l, "worker process [pid:{}] already reaped elsewhere", p);
            return;
        }
        if (ret > 0) {
            if (WIFSIGNALED(status)) {
                MS_ERROR(l, "worker process [pid:{}] killed by signal {}", p, WTERMSIG(status));
            } else if (WIFEXITED(status)) {
                MS_ERROR(l, "worker process [pid:{}] exited with code {}", p, WEXITSTATUS(status));
            }
        }
    }).detach();
    workerDied("worker process pipe closed (presumed dead)");
}
```

Add an assertion that `handleWorkerDeath` is only called in non-threaded mode:
```cpp
MS_ASSERT(!threaded_, "handleWorkerDeath called in threaded mode");
```

#### F11: Fix TOCTOU race in netem_guard stale lock removal (netem_guard.mjs:87-91)

**Fix**: Use an atomic rename-and-check pattern instead of separate check-then-remove:

```javascript
function removeStaleLock(lockPath, staleAfterMs) {
  const metadataPath = path.join(lockPath, 'owner.json');
  const metadata = safeReadJson(metadataPath);
  const acquiredAtMs = Date.parse(metadata?.acquiredAt ?? '');
  const ageMs = Number.isFinite(acquiredAtMs) ? Date.now() - acquiredAtMs : Number.POSITIVE_INFINITY;
  const staleByAge = ageMs >= staleAfterMs;
  const staleByPid = !isProcessAlive(metadata?.pid);

  if (!staleByAge && !staleByPid) {
    return false;
  }

  // Rename before removing to close the TOCTOU gap: if another process
  // recreated the lock directory between our staleness check and rmSync,
  // the rename will fail with ENOENT (already gone) or ENOTEMPTY
  // (re-acquired), both of which we handle gracefully.
  const retirePath = lockPath + '.retiring';
  try {
    fs.renameSync(lockPath, retirePath);
  } catch (e) {
    if (e.code === 'ENOENT' || e.code === 'ENOTEMPTY') return false;
    throw e;
  }
  try {
    fs.rmSync(retirePath, { recursive: true, force: true });
  } catch {
    // Best-effort cleanup; .retiring dir is inert
  }
  return true;
}
```

**Tradeoff**: On some filesystems (e.g., NFS), `renameSync` may not be atomic. For CI environments using local filesystems (the typical case for netem), this is reliable.

#### F5: Add assertion and documentation for tracks_ lifecycle (NetworkThread.h:143-161)

**Fix**: Add an `assert(!running_)` at the top of `registerVideoTrack()`. Add a comment documenting the contract:

```cpp
void registerVideoTrack(
    uint32_t trackIndex,
    uint32_t ssrc,
    uint8_t pt,
    uint8_t transportCcExtensionId = 0)
{
    // Tracks must be registered before start(). After start(), tracks_ is
    // accessed from the network thread and push_back would invalidate
    // pointers held by findTrack().
    assert(!running_);
    // ... rest unchanged ...
}
```

Also update `findTrack()` comment to note the lifetime constraint.

#### F8: Add shutdown timeout to WsClient::close() (WsClient.cpp:536-569)

**Deferred to secondary tier per review feedback.** If addressed, the fix is to add a timed join with a fallback to detach:

```cpp
if (readerToJoin.joinable()) {
    // Give reader thread up to 2s to exit after shutdown
    // Fallback: detach if it doesn't exit (should not happen on Linux)
    // ... timed join implementation ...
}
```

Not included in the initial fix batch.

---

## Module Boundaries

| Fix | Module | Boundary Impact |
|-----|--------|----------------|
| F1 | `src/Recorder.cpp` | Internal to PeerRecorder; no API change |
| F2 | `src/RoomServiceMedia.cpp` | Return value changes from success to error; signaling clients must handle error |
| F4 | `client/NetworkThread.h` | Internal to legacy pacing; no API change |
| F6 | `src/SignalingServerWs.cpp` | Adds `stopping_` check; `WorkerThread::stopping_` must be accessible (already `std::atomic<bool>` public member) |
| F7 | `client/SenderTransportController.h` | Internal to round-robin; no API change |
| F3 | `src/Worker.cpp` | Internal to Worker; adds assertion and WNOHANG |
| F11 | `tests/qos_harness/netem_guard.mjs` | Internal to lock mechanism; no harness API change |
| F5 | `client/NetworkThread.h` | Adds assertion only; no API change |

## Failure Handling

- F1: If the new logic produces a negative tick value (impossible but defensive), the existing `ticks >= baseTs` guard returns 0, which ffmpeg will handle as a zero PTS.
- F2: Error returns are the correct behavior. The signaling layer already serializes error responses.
- F4: If `pacingBitrateBps_` is 0, no packets are sent. This is correct -- it means pacing is not configured and the application must set it.
- F6: If `stopping_` is true, the deferred lambda returns early. The join result is discarded, which is correct because the server is shutting down.

## Security Considerations

- F2: Revealing "producer not found" to the client is low risk -- it mirrors "room not found" which is already exposed.
- F6: No security impact; the guard prevents crashes, not data exposure.
- F11: The rename-based lock removal prevents one CI process from deleting another's valid lock.

## Testing Strategy

| Fix | Test Type | Test Location |
|-----|-----------|---------------|
| F1 | Unit test with wrap sequences | `tests/test_review_fixes.cpp` (new test case) |
| F2 | Unit test for error return | `tests/test_review_fixes.cpp` (new test case) |
| F4 | Integration test with legacy pacing | `tests/test_thread_integration.cpp` (extend existing) |
| F6 | Compile + existing shutdown integration test | Verify existing tests pass |
| F7 | Unit test with paused tracks | `tests/test_thread_model.cpp` (extend existing) |
| F3 | Compile + existing worker lifecycle test | Verify existing tests pass |
| F11 | Unit test for netem guard lock mechanism | `tests/qos_harness/test.netem_guard.mjs` (extend existing) |
| F5 | Compile-time assertion | Verify existing tests pass |

## Rollout Notes

- F2 is a behavior change: clients that depend on silent success for missing producers will now receive errors. This is a correct change but downstream consumers should be aware.
- All other fixes are internal with no API or wire-protocol changes.
