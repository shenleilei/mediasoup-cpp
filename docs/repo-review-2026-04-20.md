# Repository Review Report (2026-04-20)

## Scope

- Standards read before review:
  - `docs/aicoding/PROJECT_STANDARD.md`
  - `docs/aicoding/PLANS.md`
  - `docs/aicoding/REVIEW.md`
- Primary code inspected:
  - `src/` (runtime/server core, QoS, registry, worker/channel, recorder)
  - `scripts/` + `CMakeLists.txt` (test/verification entrypoints)
- Verification commands executed:
  - `cmake --build build -j"$(nproc)" --target mediasoup-sfu mediasoup_tests mediasoup_qos_unit_tests`
  - `./build/mediasoup_tests`
  - `./build/mediasoup_qos_unit_tests`
- Command outcomes:
  - Build: success
  - `mediasoup_tests`: 169 passed, 0 failed
  - `mediasoup_qos_unit_tests`: 225 passed, 0 failed

## Findings

### High

1. WebSocket repeated-join path leaves stale session mappings and can leak old room membership.
   - Evidence:
     - `RegisterJoinedSocket()` rewrites `PerSocketData` and inserts only the *new* key, but does not remove any prior key that already pointed to the same socket: `src/SignalingSocketState.h:114`, `src/SignalingSocketState.h:126`, `src/SignalingSocketState.h:131`.
     - Join success path always calls `RegisterJoinedSocket()` and does not unregister previous membership first: `src/SignalingServerWs.cpp:435`, `src/SignalingServerWs.cpp:436`.
     - Disconnect cleanup removes only `sd->roomId/sd->peerId` (current session), so older keys can survive indefinitely: `src/SignalingServerWs.cpp:469`, `src/SignalingServerWs.cpp:472`, `src/SignalingServerWs.cpp:473`.
   - Impact:
     - A single socket that joins multiple rooms/identities without closing can leave stale entries in `wsMap->peers` / `roomPeers`, causing misrouted notifications and stale room/peer lifecycle state.
     - On close, only the latest room/peer is force-left, so earlier joined peer state may remain orphaned.

2. Join completion is dropped on early disconnect, which can leave ghost peers/rooms.
   - Evidence:
     - Join assigns a worker-thread room dispatch before the async join completes: `src/SignalingServerWs.cpp:338`, `src/SignalingServerWs.cpp:343`.
     - Worker-side join mutates room state immediately (`RoomService::join` creates room and inserts peer): `src/SignalingServerWs.cpp:389`, `src/RoomServiceLifecycle.cpp:46`, `src/RoomServiceLifecycle.cpp:56`.
     - Final main-loop completion path exits early when socket is already closed (`alive == false`) and skips both registration and failure cleanup: `src/SignalingServerWs.cpp:428`, `src/SignalingServerWs.cpp:431`, `src/SignalingServerWs.cpp:435`.
     - Close path only issues `leave()` when `sd->peerId` is already set, but that is done only in `RegisterJoinedSocket` (which is skipped on early return): `src/SignalingServerWs.cpp:436`, `src/SignalingServerWs.cpp:467`.
   - Impact:
     - A client that disconnects during join can still create room/peer state on worker thread without a mapped websocket session.
     - This can leak ghost participants and stale room dispatch ownership, and may prevent idle-room cleanup because the room is no longer empty.

### Medium

3. Recorder timestamp conversion can overflow signed 32-bit delta on long sessions.
   - Evidence:
     - Audio/video RTP timestamp deltas are cast to `int32_t` before rescale: `src/Recorder.cpp:460`, `src/Recorder.cpp:498`.
     - Negative values are clamped to `0` by `ClampNonNegativePts`, masking overflow: `common/ffmpeg/AvTime.h:16`.
   - Impact:
     - Around ~6.6h (90k video clock) / ~12.4h (48k audio clock), timestamp math can wrap into negative domain and collapse PTS progression, risking mux timeline corruption for long recordings.

4. Recording output filenames are only second-granularity, allowing collisions for rapid reconnects.
   - Evidence:
     - Output path format is `peerId + "_" + epoch_seconds + ".webm"`: `src/RoomRecordingHelpers.cpp:67`, `src/RoomRecordingHelpers.cpp:69`.
     - This function is used directly when creating new peer recorder instances: `src/RoomRecordingHelpers.cpp:243`, `src/RoomRecordingHelpers.cpp:244`.
   - Impact:
     - If the same peer restarts recording within the same second, file path can collide and overwrite/truncate prior recording artifacts.

5. Worker thread event loop ignores `epoll_wait` error paths and can enter silent degraded spin.
   - Evidence:
     - `epoll_wait()` return value is used directly as loop upper bound with no `nfds < 0` handling: `src/WorkerThread.cpp:283`, `src/WorkerThread.cpp:285`.
   - Impact:
     - Transient/fatal epoll errors (for example `EINTR`, `EBADF`) are not logged or handled explicitly.
     - In non-recoverable cases this can lead to an opaque busy loop without progressing worker events.

6. Room routing trust boundary is weak: user-controlled `clientIp` is accepted directly.
   - Evidence:
     - HTTP resolve path trusts `clientIp` query parameter first: `src/SignalingServerHttp.cpp:25`.
     - WebSocket join path trusts `data.clientIp` before remote address fallback: `src/SignalingRequestDispatcher.h:51`, `src/SignalingRequestDispatcher.h:53`.
     - This value is passed into registry room claim/resolve decisions: `src/RoomServiceLifecycle.cpp:15`, `src/SignalingServerHttp.cpp:74`.
   - Impact:
     - Untrusted clients can spoof geo/location inputs and bias room placement/redirect behavior.
     - In multi-node deployments this weakens routing integrity unless traffic is strictly gated behind a trusted proxy.

### Low

7. Hot-path operational logs are emitted at `warn` level for normal flow, inflating noise and log I/O.
   - Evidence:
     - `/api/resolve` normal request lifecycle logs at `warn`: `src/SignalingServerHttp.cpp:53`, `src/SignalingServerHttp.cpp:71`, `src/SignalingServerHttp.cpp:84`, `src/SignalingServerHttp.cpp:103`.
     - Registry heartbeat/update normal phases are also logged at `warn`: `src/RoomRegistry.cpp:54`, `src/RoomRegistry.cpp:59`, `src/RoomRegistry.cpp:66`, `src/RoomRegistry.cpp:79`, `src/RoomRegistry.cpp:85`.
   - Impact:
     - In production traffic, warning channels may be flooded by expected behavior, reducing signal quality for real incidents.

## Open Questions

1. Is “one WebSocket connection may join at most one active room/peer session” a required contract?
   - If yes, server should reject second `join` on an already-bound socket explicitly.
   - If no, current `wsMap` membership model needs multi-session-safe bookkeeping.

2. Are >6h continuous recordings in scope for production support?
   - If yes, recorder PTS arithmetic should be widened to avoid signed 32-bit delta overflow.

## Verification Gaps

1. This pass did not execute browser/integration/full-regression suites (`scripts/run_all_tests.sh`, `scripts/run_qos_tests.sh` full matrix), so cross-process and browser QoS paths were not revalidated end-to-end.
2. Findings above are confirmed by source inspection; no dedicated reproduction test was added in this review-only task.
3. Vendored upstream trees (`src/mediasoup-worker-src/`, `third_party/`) were not deeply audited in this pass.

## Summary

- Build and two major unit suites are green.
- 7 actionable findings were identified (2 high, 4 medium, 1 low).
- Highest-priority follow-up remains WebSocket join/session lifecycle hardening, especially disconnect-during-join cleanup and multi-join mapping consistency.
