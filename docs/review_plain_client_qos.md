# Full Code Review — dev/linux_client (2026-04-19 Re-review)

> **Scope**: All C++ source under `src/`, `client/`, `tests/` on `dev/linux_client` branch
> **Methodology**: Line-by-line review of every `.cpp`/`.h` file, covering: resource lifecycle, thread safety, protocol correctness, test quality
> **Baseline**: Latest commit `1d096086` ("feat(client): finalize plain-client qos parity and docs")

---

## Executive Summary

| Severity | Count | Key Theme |
|----------|-------|-----------|
| **P0** | 9 | Resource leaks, data races, corrupted IPC messages |
| **P1** | 14 | Missing cleanup, silent logic failures, potential UB |
| **P2** | 12 | Code quality, edge cases, missing tests |

**Most critical cluster**: Transport/Producer/Consumer destructors are **no-ops** (`~Transport()` is defaulted), yet the code overwrites `shared_ptr` fields as if destruction equals cleanup. Every assignment to `peer->sendTransport`, `peer->recvTransport`, etc., must be preceded by `close()` + cascading cleanup.

---

## P0 — Must Fix Before Merge

### P0-1: Transport resource leak on replacement
- **File**: `src/RoomService.cpp:313–315, 379–381, 457, 583`
- **Issue**: `createTransport()`, `createPlainTransport()`, `plainPublish()`, `plainSubscribe()` assign new transports to peer slots without calling `close()` on the old transport. Since `Transport::~Transport()` is a no-op, the worker-side resources (ports, ICE, DTLS, memory) are permanently leaked.
- **Fix**: Before assigning, call `oldTransport->close()` and clean up associated producers/consumers.

### P0-2: Stale consumers left in peer map when recv transport is replaced
- **File**: `src/RoomService.cpp:319–343, 385–410`
- **Issue**: When a recv transport is replaced, old consumers remain in `peer->consumers` referencing a dead transport. Subsequent pause/resume calls target stale consumer IDs, and the map grows without bound.
- **Fix**: Before creating new recv transport, close and erase all consumers associated with the old transport.

### P0-3: Recorder transport never closed on cleanup
- **File**: `src/RoomService.cpp:200, 255, 927`
- **Issue**: In join() reconnect, leave(), and cleanupRoomResources(), recorder transports are erased from `recorderTransports_` but `close()` is never called. Worker-side transport is leaked.
- **Fix**: Call `close()` before erasing.

### P0-4: Data race on `videoTracks` — reader thread vs main thread
- **File**: `client/main.cpp:1078–1083, 1193–1213, 1326 ff.`
- **Issue**: `ws.onNotification` lambda runs on the WsClient reader thread, mutating `videoTracks` (qosCtrl state, videoSuppressed, encBitrate). The main loop reads/writes `videoTracks` concurrently with zero synchronization. This is a data race (UB per C++ standard).
- **Fix**: Protect with a mutex, or queue notifications to the main thread.

### P0-5: Data race + use-after-free on `g_rtcp` global pointer
- **File**: `client/main.cpp:372–377, 1084, 1648`
- **Issue**: `g_rtcp` is set on main thread, used from RTCP send path, but `ws.onNotification` callbacks (which trigger PLI/NACK via RTCP) fire from the reader thread. `g_rtcp` is set to `nullptr` at L1648 without any fence — if any callback is in flight, this is use-after-free.
- **Fix**: Use `std::atomic<RtcpContext*>` or restructure ownership.

### P0-6: Worker `closed_` is not atomic — data race
- **File**: `src/Worker.h:65, Worker.cpp:164,171,219–221,237,252`
- **Issue**: `closed_` is plain `bool`, read/written from waitThread, calling thread (`close()`), and WorkerThread event loop. Both `close()` and `workerDied()` set it with no synchronization.
- **Fix**: Make `std::atomic<bool>`.

### P0-7: `close()` and `workerDied()` race on `routers_`
- **File**: `src/Worker.cpp:171–217, 219–235`
- **Issue**: Both iterate and clear `routers_` concurrently from different threads — undefined behavior.
- **Fix**: Protect `routers_` with a mutex, or use `closed_.exchange(true)` as a gate.

### P0-8: `Channel::notify()` uses stale FlatBuffer offset
- **File**: `src/Channel.cpp:88–122, Channel.h:104–108`
- **Issue**: `notify()` accepts a pre-built `bodyOffset`, then acquires `builderMutex_` and calls `builder_.Clear()` — the offset is now invalid. Any caller passing non-zero `bodyOffset` produces a corrupt message.
- **Fix**: Use `BuildRequestBodyFn` pattern (like `requestBuildWithId`) so body is built inside the lock.

### P0-9: QoS loss rate formula is wrong
- **File**: `client/qos/QosSignals.h:22`
- **Issue**: Loss rate computed as `lost / (sent + lost)` instead of the standard `lost / sent`. This underestimates loss, causing the QoS controller to react late to congestion.
- **Fix**: Change denominator to `sent`.

---

## P1 — Should Fix

### P1-1: `plainPublish` doesn't clean up prior producers/transport
- **File**: `src/RoomService.cpp:432–567`
- **Issue**: Calling `plainPublish()` twice replaces transport (leaking) and adds new producers while old ones accumulate.

### P1-2: No SSRF validation on plainSubscribe target IP
- **File**: `src/RoomService.cpp:569–586, 415–428`
- **Issue**: Client-specified `recvIp` is used without validation. Malicious client can relay RTP to internal addresses.
- **Fix**: Validate IP is public/routable.

### P1-3: x264 bitrate changes at runtime are silently ignored
- **File**: `client/main.cpp:1155–1157`
- **Issue**: Setting `bit_rate` on an already-opened `AVCodecContext` has no effect for libx264 — it reads these only at `avcodec_open2()`. The `requiresRecreate` check only triggers on dimension/fps changes, so pure bitrate changes never take effect.
- **Fix**: Force encoder recreation on bitrate change, or use x264's runtime reconfiguration API.

### P1-4: `recvText` ignores partial recv failures → possible OOM
- **File**: `client/main.cpp:180–186`
- **Issue**: `recv()` return values ignored. Partial read leaves `len` with garbage → enormous allocation.

### P1-5: H264 baseline profile may mismatch server negotiation
- **File**: `client/main.cpp:975`
- **Issue**: Encoder hardcoded to "baseline" but mediasoup defaults to `profile-level-id=42e01f`. Mismatch may cause receiver rejection.

### P1-6: Audio retransmission buffer never populated
- **File**: `client/RtcpHandler.h:172, client/main.cpp:436`
- **Issue**: `rtcp.audioStore` exists but `sendOpus()` never fills it. Audio NACKs can never be retransmitted.

### P1-7: Notification handler captures `shared_ptr` by value → prevents cleanup
- **File**: `src/Transport.cpp:231,321, Router.cpp:152`
- **Issue**: Channel emitter listeners capture `shared_ptr<Producer/Consumer>`. If object is dropped without explicit `close()`, the emitter retains the `shared_ptr` forever.
- **Fix**: Capture `std::weak_ptr` in notification handlers.

### P1-8: `Router::close()` sends close-transport before close-router
- **File**: `src/Router.cpp:256–282`
- **Issue**: Closes all transports first (each sends ROUTER_CLOSE_TRANSPORT), then sends WORKER_CLOSE_ROUTER. Worker receives close-transport for entities it already destroyed.
- **Fix**: Send WORKER_CLOSE_ROUTER first, or skip individual transport close commands.

### P1-9: `Channel::readThread_` destruction may call `std::terminate`
- **File**: `src/Channel.cpp:68–71`
- **Issue**: If `close()` is called from within a notification handler (readThread), the thread is not joined or detached. Destroying a joinable `std::thread` calls `std::terminate`.
- **Fix**: Add `readThread_.detach()` in the self-thread case.

### P1-10: QoS peer mode logic: ANY audio-only → should be ALL audio-only
- **File**: `client/qos/QosProtocol.h:162`
- **Issue**: Peer mode check uses ANY track audio-only instead of ALL tracks audio-only. A mixed audio+video peer incorrectly gets classified as audio-only.

### P1-11: QoS coordinator hardcoded max level
- **File**: `client/qos/QosCoordinator.h:229`
- **Issue**: Hardcoded `4` for max profile level instead of `profile.levelCount - 1`.

### P1-12: Transport/Router/Producer/Consumer `closed_` not atomic
- **File**: `src/Transport.h:44, Router.h:65, Producer.h:55, Consumer.h:59`
- **Issue**: In threaded mode, notification handlers fire on readThread while user thread calls `close()`. Data race.

### P1-13: Double-promise-set race on request timeout
- **File**: `src/Channel.cpp:229–238`
- **Issue**: Timeout and readThread can both attempt to set the same promise, causing `promise_already_satisfied` exception.

### P1-14: Redundant double parsing of client stats
- **File**: `src/SignalingServer.cpp:604 + RoomService.cpp:1034`
- **Issue**: Snapshot parsed twice — once for validation, discarded, then re-parsed in worker thread. Wastes CPU on the most frequent message.

---

## P2 — Nice to Have

### P2-1: Hardcoded default SSRCs 1111/2222
- **File**: `src/SignalingServer.cpp:862–866`

### P2-2: `cleanupRoomResources` O(N) linear scan
- **File**: `src/RoomService.cpp:921–967`

### P2-3: `autoRecord` codec PT defaults may be wrong
- **File**: `src/RoomService.cpp:809–810`

### P2-4: `contentTypeForPath` substring match
- **File**: `src/SignalingServer.cpp:125–132`

### P2-5: `sendOpus` 4KB stack buffer on embedded target
- **File**: `client/main.cpp:433`

### P2-6: SIGINT doesn't interrupt sleeps → slow shutdown
- **File**: `client/main.cpp:51, 1312`

### P2-7: WebSocket key not validated
- **File**: `client/main.cpp:133`

### P2-8: Child process inherits all fds — no CLOEXEC
- **File**: `src/Worker.cpp:82–124`

### P2-9: Pipe fd leak on partial pipe() failure
- **File**: `src/Worker.cpp:73`

### P2-10: Header extension URI uses `string::find` — can match substrings
- **File**: `src/Transport.cpp:92–113`

### P2-11: FlatBuffers null pointer dereference risk on string fields
- **File**: `src/Router.cpp:107–108`

### P2-12: `WorkerManager::setMaxRoutersPerWorker` unsynchronized
- **File**: `src/WorkerManager.h:39`

---

## Test Coverage Assessment

### Critical Gaps

| Gap | Severity |
|-----|----------|
| No unit tests for Transport/Consumer/Producer lifecycle | HIGH |
| No PlainTransport-specific integration test (connect, rtcpMux, tuple validation, error paths) | HIGH |
| No concurrent multi-track QoS state machine test | MEDIUM |
| No test for full Congested→Stable recovery path | MEDIUM |
| Budget allocator with zero/negative weights untested | MEDIUM |
| QoS protocol version mismatch handling untested | MEDIUM |

### Test Quality Issues

| Issue | File |
|-------|------|
| `FutureDestructorBlocksWithStdAsync` proves old bug exists, doesn't test the fix | test_review_fixes.cpp |
| "within 5s" error message but actual timeout is 15,000ms | test_review_fixes_integration.cpp |
| `usleep`-based synchronization → flaky on slow CI | Multiple integration tests |
| `Room("test-room", nullptr)` — null router will crash if any code path touches it | test_review_fixes.cpp |
| JS harness uses real `tc netem` → requires root, Linux-only | cpp_client_runner.mjs |
| No time mocking for deterministic QoS testing | test_client_qos.cpp |

---

## Relation to Question 1: Can `unique_ptr` + custom deleter solve the resource problem?

**Partially, but not fully.** The core P0 resource leak issues (P0-1 through P0-3) stem from destructors being no-ops while code relies on RAII.

### What `unique_ptr + deleter` would solve
- Ensures `close()` is always called when a transport goes out of scope or is replaced
- Eliminates the entire class of "forgot to call close()" bugs

### What it would NOT solve
- **Cascading cleanup**: When a transport is replaced, its associated producers/consumers must also be closed and removed from peer maps. A deleter on the transport pointer alone won't clean up the peer's `consumers`/`producers` maps.
- **Ownership model**: The codebase uses `shared_ptr` extensively — transports are shared across router maps, peer maps, and notification handler captures. Switching to `unique_ptr` requires redesigning the ownership graph.
- **Cross-object consistency**: Closing a transport must also update the router's internal state, notify other peers, clean up recordings, etc.

### Recommended approach
1. **Keep `shared_ptr`** for the shared ownership model
2. **Add a `replaceTransport()` helper** that atomically: closes old transport → closes stale consumers/producers → assigns new transport → re-subscribes
3. **Make `Transport::~Transport()` call `close()` if not already closed** (defensive RAII)
4. **Use `weak_ptr`** in notification handler captures to break reference cycles

---

---

## Supplementary Findings (2026-04-19 Deep Dive)

> Additional P0/P1 issues discovered in areas not fully covered by the initial review.

### P0-10: Uncaught `json::type_error` in WebSocket handler crashes the entire server
- **File**: `src/SignalingServer.cpp:558`
- **Issue**: `req["request"].get<bool>()` is called outside any `try-catch`. If a client sends `{"request": 1}` or `{"request": "true"}` or `{"request": null}`, `get<bool>()` throws `json::type_error`. uWS does not wrap the callback in try-catch — the exception propagates to `std::terminate()`. **Any unauthenticated WebSocket client can crash the server with a single malformed message.**
- **Fix**: Change to `req.value("request", false)` or wrap the entire message handler in try-catch.

### P0-11: Invalid `std::sort` comparator violates strict weak ordering — UB / crash
- **File**: `src/RoomRegistry.h:776–780`
- **Issue**: The no-geo comparator returns `true` when `a.address == self` regardless of `b`, violating irreflexivity (`comp(x, x)` must return `false`). If the current node is a candidate, `comp(self, self)` returns `true`. This is UB for `std::sort` and can cause crashes, infinite loops, or out-of-bounds access in libstdc++/libc++ partitioning.
- **Fix**: Restructure comparator with proper tiebreaking:
  ```cpp
  if (a.rooms != b.rooms) return a.rooms < b.rooms;
  bool aIsSelf = (a.address == self);
  bool bIsSelf = (b.address == self);
  if (aIsSelf != bIsSelf) return aIsSelf;
  return a.address < b.address;
  ```

### P0-12: Null dereference in `Recorder::initMuxer` if `avformat_new_stream` fails
- **File**: `src/Recorder.h:160–161, 177–178`
- **Issue**: `avformat_new_stream()` can return `nullptr` (memory pressure). The return value is immediately dereferenced (`audioStream_->id = 0`), causing a guaranteed crash.
- **Fix**: Add null checks after each `avformat_new_stream()` call.

### P1-15: Out-of-bounds read in `Recorder::setH264Extradata` with malformed SPS
- **File**: `src/Recorder.h:211`
- **Issue**: `h264Sps_[1]`, `[2]`, `[3]` are accessed after only checking `h264Sps_.empty()`. A malformed RTP stream could deliver an SPS with 1–3 bytes → heap buffer over-read from network input.
- **Fix**: Check `if (h264Sps_.size() < 4) return false;`

### P1-16: Data race on `Recorder::sock_` between `stop()` and `recvLoop()`
- **File**: `src/Recorder.h:110, 260`
- **Issue**: `stop()` writes `sock_ = -1` on calling thread while `recvLoop()` reads it on recv thread. Plain `int` with no synchronization → data race (UB). If fd is recycled before thread exits, `poll()`/`recv()` operates on wrong fd.
- **Fix**: Make `sock_` `std::atomic<int>` or use a shutdown pipe.

### P1-17: Data race on `Recorder::outputPath_` in `appendQosSnapshot()`
- **File**: `src/Recorder.h:131, 123`
- **Issue**: `appendQosSnapshot()` reads `outputPath_.empty()` without holding `qosMutex_` while `stop()` clears it under `qosMutex_`. Concurrent read/write of `std::string` is UB.
- **Fix**: Move check inside `qosMutex_` scope.

### P1-18: `Peer::close()` is not thread-safe — double-close of transports
- **File**: `src/Peer.h:29–43`
- **Issue**: `closed` is plain `bool` checked/set without synchronization. `Room::replacePeer()` and `Room::close()` can race, both passing the guard, leading to double-close of WebRTC transports → worker crash.
- **Fix**: Make `closed` `std::atomic<bool>` and use `exchange(true)`.

### P1-19: Data race in `RoomRegistry` — reading cache sizes after releasing mutex
- **File**: `src/RoomRegistry.h:737`
- **Issue**: `syncAllUnlocked()` accesses `nodeCache_.size()` after the `cacheMutex_` scope ends. Subscriber thread can concurrently modify maps via `handlePubSubMessage()` → data race.
- **Fix**: Capture sizes inside the mutex scope.

### P1-20: `produce()` auto-subscribe skips plain-transport subscribers
- **File**: `src/RoomService.cpp:651–655`
- **Issue**: `produce()` only checks `other->recvTransport` (WebRTC) when auto-subscribing. Peers with only `plainRecvTransport` are silently skipped and permanently miss the new stream. `plainPublish()` correctly checks both.
- **Fix**: Check both `recvTransport` and `plainRecvTransport`.

### P1-21: No duplicate consumer guard — unbounded resource consumption
- **File**: `src/RoomService.cpp:687–713`
- **Issue**: `consume()` creates consumers unconditionally for any `producerId`, even if the peer already has one. Client can call `consume()` in a loop to amplify server load per producer.
- **Fix**: Check if peer already has a consumer for the same producerId before creating.

### P1-22: `setClientStats()` can resurrect QoS state for departed peers
- **File**: `src/RoomService.cpp:1031–1053`
- **Issue**: Late-arriving stats after `leave()` re-insert entries into `qosRegistry_`. These zombie entries accumulate, leaking memory proportional to churn rate. `setDownlinkClientStats()` correctly validates room/peer existence first.
- **Fix**: Validate room/peer existence before upserting.

### P1-23: Deferred task lambdas capture raw `this` → use-after-free on early destruction
- **File**: `src/RoomService.cpp:1546, 1693–1698`
- **Issue**: `continueBroadcastStats()` and `scheduleDownlinkPlanning()` post lambdas capturing bare `this`. If RoomService is destroyed while tasks are queued, callbacks dereference freed memory.
- **Fix**: Use `std::weak_ptr` or a shared alive-flag pattern.

### P1-24: No rate limiting on WebSocket connections/messages — DoS
- **File**: `src/SignalingServer.cpp:541–556`
- **Issue**: No limits on concurrent connections, message rate, or resource-creating methods. A single client can flood `join`/`produce`/`consume` to exhaust worker resources.
- **Fix**: Add per-IP connection limits, per-connection message rate limiting, and cap total connections.

### P1-25: Unbounded registry task queue — memory exhaustion + heartbeat starvation
- **File**: `src/SignalingServer.cpp:378–413`
- **Issue**: `enqueueRegistryTask()` has no queue size limit. Flooding `/api/resolve` requests grows the queue without bound, causing OOM and starving Redis heartbeat tasks → node marked dead.
- **Fix**: Cap queue depth; return HTTP 503 when full.

### P1-26: Missing cross-peer authorization — any peer can modify another's QoS
- **File**: `src/SignalingServer.cpp:819–832`
- **Issue**: `getStats`, `setQosOverride`, `setQosPolicy` accept a target `peerId` from the caller with no authorization check. Any peer in a room can degrade another peer's quality or read their connection stats.
- **Fix**: Restrict cross-peer operations to admin/moderator role.

### P1-27: `statsTimer` and `redisTimer` not closed on shutdown — event loop hangs
- **File**: `src/SignalingServer.cpp:1194–1243, 1250–1257`
- **Issue**: Shutdown timer closes listen socket and itself, but `statsTimer` and `redisTimer` are never closed. Active timers keep the uWS event loop alive → `run()` never returns.
- **Fix**: Close all timers in the shutdown callback.

### P1-28: Signed integer overflow (UB) in `DownlinkQosRegistry::Upsert`
- **File**: `src/qos/DownlinkQosRegistry.cpp:39`
- **Issue**: `snapshot.tsMs + kTsBackwardToleranceMs` performs signed `int64_t` addition on client-controlled value. Near `INT64_MAX`, this is signed overflow → UB.
- **Fix**: Rewrite as equivalent subtraction or clamp input.

### P1-29: Integer truncation in `QosValidator` — `sampleIntervalMs` can become 0
- **File**: `src/qos/QosValidator.cpp:317–318`
- **Issue**: `static_cast<uint32_t>(x.get<uint64_t>())` without upper-bound check. Value `4294967296` (2³²) truncates to **0**, causing tight polling loop (CPU DoS).
- **Fix**: Reject values exceeding `UINT32_MAX`.

### P1-30: `std::getenv` called in hot allocation loop — thread-safety UB
- **File**: `src/qos/SubscriberBudgetAllocator.cpp:28–45, 77`
- **Issue**: `estimateLayerBitrateBps` calls `std::getenv()` on every invocation (O(N × layers) per allocation). `std::getenv` is not thread-safe if any thread calls `setenv`/`putenv` (POSIX UB).
- **Fix**: Cache env var value at startup with `std::call_once`.

---

## Updated Summary

| Severity | Previous Count | New Findings | Total |
|----------|---------------|-------------|-------|
| **P0** | 9 | 3 | **12** |
| **P1** | 14 | 16 | **30** |
| **P2** | 12 | 0 | **12** |

---

## Priority Fix Order

1. **P0-10**: Server crash from single malformed WebSocket message (easiest P0 to exploit)
2. **P0-11**: `std::sort` UB / crash in RoomRegistry
3. **P0-4, P0-5, P0-6, P0-7**: Thread safety fixes (data races = UB)
4. **P0-1, P0-2, P0-3**: Resource leak fixes (worker-side leaks accumulate)
5. **P0-8**: Channel IPC corruption fix
6. **P0-9**: QoS formula fix
7. **P0-12**: Recorder null dereference
8. **P1-24, P1-25**: DoS prevention (rate limiting)
9. **P1-26**: Cross-peer authorization
10. **P1-3**: x264 runtime bitrate (core feature broken)
11. **P1-2**: SSRF validation
12. **P1-7, P1-9, P1-12, P1-13, P1-16–P1-19**: Remaining thread safety issues
13. Everything else
