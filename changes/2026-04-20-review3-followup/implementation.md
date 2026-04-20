# Review3 Triage

This note records the current-code disposition of each `review3.md` item after source inspection on 2026-04-20.

## Scope

- Inspected: RTP capability/serialization, worker/channel lifecycle, Redis parsing, CLI parsing, recorder timing, worker-thread runtime paths, and the concurrency claims called out in `review3.md`.
- Not deeply re-validated with runtime repros in this pass: some theoretical channel re-entry and timer-lifetime concerns.

## Critical

- `1. RTP 扩展 URI 匹配顺序错误`
  Status: confirmed.
  Evidence: `src/RtpParametersFbs.h:78-81` checks `"rtp-stream-id"` before `"repaired-rtp-stream-id"`, so repaired IDs match the earlier substring branch.

- `2. Payload Type 重复定义冲突`
  Status: confirmed.
  Evidence: `src/supportedRtpCapabilities.h:40` and `src/supportedRtpCapabilities.h:52` both assign payload type `108` to different codec entries.

- `3. RoomService 异步任务捕获裸 this`
  Status: not confirmed as the stated UAF.
  Evidence: `src/SignalingServerWs.cpp:63-64` wires `taskPoster_` to the owning `WorkerThread` queue, and `src/WorkerThread.cpp:129-137` stops the thread before `RoomService` destruction. Under the current design, queued continuations are more likely to be dropped on shutdown than to run after destruction.
  Note: this remains a lifecycle semantics concern, but the current code does not show the documented UAF path.

- `4. fork 后 exec 前调用非 async-signal-safe 逻辑`
  Status: confirmed.
  Evidence: `src/Worker.cpp:100-122` performs `std::vector`, `std::to_string`, string concatenation, and `setenv()` in the child after `fork()` and before `execvp()`.

## High

- `1. Channel 发送路径 fd 检查与写入竞态`
  Status: confirmed.
  Evidence: `src/Channel.cpp:120-127` reads `closed_` and writes `producerFd_` without serializing against `src/Channel.cpp:112-113`, which closes and resets the same fd.

- `2. Channel 接收处理可重入`
  Status: fixed in this wave.
  Evidence: `src/Channel.cpp` now removes each buffered message before dispatch, and `tests/test_review_fixes.cpp` covers notification callbacks that re-enter `requestWait()` without replaying the same message.

- `3. Producer/Consumer/Router 使用 Channel 裸指针`
  Status: not confirmed as a live bug.
  Evidence: `src/Worker.cpp:176-197` and `src/Worker.cpp:223-242` close or notify routers before channel teardown; `src/Router.cpp:262-291` closes transports before using the router/channel relationship; `src/Transport.cpp:301-320` closes producers/consumers before transport teardown.
  Note: this is a fragility concern, but the current ownership order still protects the raw pointer lifetime.

- `4. pipe 创建异常路径 fd 泄漏`
  Status: confirmed.
  Evidence: `src/Worker.cpp:73-79` leaks already-created pipe fds when the second `pipe()` or `fork()` fails.

- `5. Redis 回复解析缺少元素类型/空指针防护`
  Status: confirmed.
  Evidence: `src/RoomRegistry.cpp:510-515` and `src/RoomRegistry.cpp:631-634` build `std::string` from reply elements without validating reply element type or `str`.

- `6. CLI 参数解析异常未兜底`
  Status: confirmed.
  Evidence: `src/MainBootstrap.cpp:159-183` calls `std::stoi`/`std::stod` directly on CLI substrings with no exception handling.

## Medium

- `1. Room::touch() 与读取路径锁策略不一致`
  Status: not confirmed.
  Evidence: `src/RoomManager.h:26-58` calls `touch()` while already holding `Room::mutex_`; constructor use is before publication. The current call sites do not show an unsynchronized cross-thread write.

- `2. Room 与 Peer 并发模型不统一`
  Status: not confirmed under the current architecture.
  Evidence: `src/WorkerThread.h:25-31` documents thread affinity for room/router/transport operations. `Peer` remains effectively worker-thread-confined today.

- `3. Room::removePeer() 持锁调用 peer->close()`
  Status: fixed in this wave.
  Evidence: `src/RoomManager.h` now erases the peer under lock and closes it afterward, and `tests/test_room.cpp` covers a `Producer @close` callback that re-enters room state.

- `4. firstRtpTime_ 在不同互斥量下读写`
  Status: confirmed.
  Evidence: `src/Recorder.h:144-145` reads `firstRtpTime_` under `qosMutex_`, while `src/Recorder.h:389-392` writes it under `muxMutex_`.

- `5. WorkerManager::maxRoutersPerWorker_ 读写锁策略不一致`
  Status: not confirmed.
  Evidence: `src/WorkerManager.h:39-40` exposes unlocked accessors, but the value is set during startup and then treated as effectively immutable in current flows.

- `6. epoll_ctl / write(eventfd) 返回值未充分检查`
  Status: confirmed.
  Evidence: `src/WorkerThread.cpp:167`, `src/WorkerThread.cpp:226`, and `src/WorkerThread.cpp:429` ignore the return values of `write(eventfd)` / `epoll_ctl`.

- `7. ortc.h 头文件内 static 容器导致多 TU 副本`
  Status: confirmed.
  Evidence: `src/ortc.h:12-15` defines `static const std::vector<uint8_t> DynamicPayloadTypes` in a header.

- `8. 延迟任务队列 std::move(const&) 实际未移动`
  Status: confirmed.
  Evidence: `src/WorkerThread.cpp:344` moves from `delayedTasks_.top().fn`, but `priority_queue::top()` returns a `const&`, so this becomes a copy.

- `9. 计时器资源回收不完整`
  Status: fixed in this wave.
  Evidence: `src/SignalingServer.cpp` now tracks and closes all background timer handles after `app.run()`, and `src/SignalingServerHttp.cpp` no longer self-closes only the shutdown timer while leaving the recurring timers implicit.

- `10. MIME 类型判断采用子串匹配`
  Status: confirmed.
  Evidence: `src/StaticFileResponder.h:80-85` uses `find()` instead of suffix matching.

- `11. GeoRouter 可拷贝语义未显式禁用`
  Status: rejected.
  Evidence: `src/GeoRouter.h:339` contains `std::mutex mutex_`, so copy construction is already implicitly deleted by the type system.

- `12. geo_ 指针访问同步策略不统一`
  Status: not confirmed.
  Evidence: `src/MainBootstrap.cpp:288` sets `geo_` during startup, then runtime code only reads it (`src/RoomRegistry.cpp:721`, `src/RoomRegistry.cpp:731`).

## Low

- `1. signal() 可移植性与语义一致性弱于 sigaction()`
  Status: confirmed.
  Evidence: `src/main.cpp:18-20`.

- `2. 日志级别偏高导致噪声`
  Status: confirmed.
  Evidence: `src/SignalingServerRuntime.cpp:160`, `src/SignalingServerRuntime.cpp:175`, `src/SignalingServerRuntime.cpp:184`, and `src/SignalingServerRuntime.cpp:203` log routine queue lifecycle at `warn`.

- `3. catch (...) 直接吞异常导致可观测性差`
  Status: confirmed.
  Evidence: `src/RoomService.h:66` and `src/SignalingServerRuntime.cpp:161` swallow exceptions without logging; `src/SignalingServerRuntime.cpp:136` at least logs before rethrowing.

- `4. gethostname 截断边界处理不严谨`
  Status: confirmed.
  Evidence: `src/MainBootstrap.cpp:202-204` does not check the return code and relies on implicit string termination after `gethostname()`.

- `5. FFmpeg 部分接口已过时`
  Status: confirmed but low priority.
  Evidence: `src/Recorder.h:432` and `src/Recorder.h:468` still use `av_init_packet`.

- `6. 部分时间相关逻辑同函数内多次取时`
  Status: not promoted.
  Evidence: the document cites a style/perf smell, but this pass did not identify a concrete regression or correctness defect worth carrying into the current change.

## Summary

- Confirmed high-confidence defects: 12
- Needs deeper follow-up before coding: 3
- Not confirmed / rejected under the current codebase: 9
- Low-priority but real cleanup items: 4

The implementation scope for this change should follow the confirmed items first, starting with RTP correctness and worker/channel/runtime hardening.

## Implemented In This Wave

The current code now fixes the following previously confirmed items:

- RTP repaired stream ID URI mapping order in `src/RtpParametersFbs.h`
- duplicate non-zero payload type advertisement in `src/supportedRtpCapabilities.h`
- post-`fork()` unsafe worker setup by switching worker spawn to `posix_spawnp()` in `src/Worker.cpp`
- partial pipe resource leaks during worker spawn failure handling in `src/Worker.cpp`
- channel send/close fd race by duplicating the producer fd before writes in `src/Channel.cpp`
- Redis reply element validation via `src/RoomRegistryReplyUtils.h` and `src/RoomRegistry.cpp`
- CLI numeric parse hardening and `gethostname()` boundary handling in `src/MainBootstrap.cpp`
- recorder first-RTP timestamp synchronization in `src/Recorder.h`
- unchecked worker-thread eventfd/epoll/timer syscalls in `src/WorkerThread.cpp`
- multi-TU dynamic payload pool duplication in `src/ortc.h`
- delayed-task closure copy regression in `src/WorkerThread.cpp`
- MIME suffix matching in `src/StaticFileResponder.h`
- non-threaded channel notification re-entry in `src/Channel.cpp`
- `Room::removePeer()` lock scope in `src/RoomManager.h`
- explicit recurring uWS timer cleanup in `src/SignalingServer.cpp` and `src/SignalingServerHttp.cpp`

This wave also cleaned up a few low-priority confirmed items:

- `sigaction()` now replaces `signal()` in `src/main.cpp`
- swallowed inline registry-task exceptions now log in `src/RoomService.h` and `src/SignalingServerRuntime.cpp`
- noisy routine registry queue lifecycle logs were lowered from `warn` to `debug` in `src/SignalingServerRuntime.cpp`
- deprecated `av_init_packet()` stack initialization was removed in `src/Recorder.h`

## Remaining Review3 Debt

No additional confirmed `review3.md` items are intentionally deferred from this implementation wave.
