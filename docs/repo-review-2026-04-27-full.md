# 全量仓库代码 Review 报告（修订版）

**Review 日期**: 2026-04-27
**修订日期**: 2026-04-28（源码复核与文档一致性修正）
**Review 范围**: `src/`、`client/`、`common/`、`tests/`、构建脚本、QoS harness — 全部自有代码文件逐文件阅读
**Review 维度**: 正确性、并发安全、生命周期、协议语义、安全、API 设计、测试质量

---

## 修订说明

初版含 4 Critical + 6 High + 9 Medium + 6 Low + 6 Architecture = 31 项。
经源码逐行复核，删除 2 条不成立结论（C1、C3），降级 3 条（C4→M-new、H1→M-new、H4→M-new），删除 1 条不成立的 High（H3），并修正文档统计与覆盖说明。
修订后：1 Critical + 3 High + 13 Medium + 7 Low + 6 Architecture = 30 项。

---

## 一、Critical（必须修）

### C1. SourceWorker::loopFile/loopCamera 未检查 avcodec_open2 返回值

**文件**: `client/SourceWorker.h:308, 419`

```cpp
avcodec_open2(vdec, dec, nullptr);  // 返回值被忽略
```

如果 decoder 打开失败，后续 `avcodec_send_packet` 会崩溃或静默产生垃圾数据。项目已有 `common/ffmpeg/Decoder` 封装了 `CheckError`，但 SourceWorker 没有使用。

**修复**: 加返回值检查，或重构为使用 `common/ffmpeg/Decoder`。

---

## 二、High（应该修）

### H1. RoomRegistrySync::scanKeys 长时间持有 command_.mutex

**文件**: `src/RoomRegistrySync.cpp:12-16, 60-157`

根因不只是 `scanKeys()` 自身，而是 `syncAll()` 在 `command_.mutex` 保护下调用 `syncAllUnlocked()`，后者又执行多轮 `SCAN` 和 `MGET`。如果 Redis 有大量 key，整个同步过程需要多轮 round-trip，期间其他操作（resolveRoom、claimRoom、heartbeat）都会被阻塞。

**修复**: 将 SCAN 结果缓存到临时变量，释放锁后再处理；或使用后台线程异步 sync。

---

### H2. SenderTransportController::FlushFreshVideoQueues 音频优先可饿死视频

**文件**: `client/SenderTransportController.h:191-197, 395-495`

问题触发点是 pacing tick 的调用顺序和 `FlushAudioQueue()` 的无上限循环：`OnPacingTick()` 先执行 `FlushAudioQueue()`，而该函数会一直发送到队列为空或阻塞为止。若音频持续到达且总量大，视频队列可能长期得不到发送机会。

**修复**: 在 `FlushAudioQueue` 中加每轮最大发送字节数限制。

---

### H3. Channel::processAvailableData 的 recvBuf_ erase 效率

**文件**: `src/Channel.cpp:394-411, 501-525`

```cpp
recvBuf_.erase(recvBuf_.begin(), recvBuf_.begin() + static_cast<ptrdiff_t>(4 + msgLen));
```

每次处理完一个消息都从头 erase，O(N) 内存移动。高频消息下有大量拷贝开销。`readLoop` (line 524-526) 使用 offset 索引后一次性 erase，更高效。非线程模式应同样使用 offset 方案。

**修复**: `processAvailableData` 改用 offset 索引，处理完后一次性 erase。

---

## 三、Medium（建议修）

### M1. EventEmitter::off(uint64_t id) 暴力遍历

**文件**: `src/EventEmitter.h:31-38`

遍历所有 event 的所有 entries 查找一个 id，O(N*M)。`nextId_` 是单调递增 `uint64_t`，id 冲突在实际中基本不成立，但暴力遍历的复杂度是性能问题。

**修复**: 维护 `id -> event` 反向索引，使 `off(id)` 降为 O(1)。

*(原 C4 修订：once 重入在当前实现中是安全的——`emit()` 在锁内拷贝 `toCall` 并擦除 once 条目后才释放锁调用回调，不会重入触发。Consumer close 双路径调用 `off(id_)` 是冗余遍历而非正确性问题，降为 Medium。)*

---

### M2. PublisherQosController::activeOverrides_ 暂停时无法清理过期项

**文件**: `client/qos/QosController.h:78-120, 449-475`

每次收到非零 TTL 的 override 都 `activeOverrides_[key] = {...}`。`handleOverride()` 本身会调 `mergeOverrides()` 清理过期项，`onSample()` 中 `getActiveOverride()` 也会触发清理。因此**在 override 消息持续到达或 track 仍在采样时不会泄漏**。

但如果 track 暂停（`onSample` 不再被调用）且不再有新 override 消息到达，已过期的条目不会被清理。风险有限：暂停时 map 通常很小，且 override 消息本身会触发清理。

**修复**: 加容量上限（如 256），或在 track 暂停恢复时主动清理一次。

*(原 H1 修订：`handleOverride()` 调 `mergeOverrides()` 确实会清理过期项，"无界增长"说法不成立；只在"暂停+无消息"组合下有残留风险，降为 Medium。)*

---

### M3. RoomServiceQos::maybeSendRoomPressureOverrides O(N) peer 遍历

**文件**: `src/RoomServiceQos.cpp`

对每个 peer 做 `qosRegistry_.Get(roomId, peerId)` 查找。`qosRegistry_.Get()` 本身是哈希查找 O(1)，外层按 peer 线性遍历是 O(N)。这是性能担忧，不是已证明的 bug——在当前规模下 O(N) 可接受，大房间下可能成为瓶颈。

**修复**: 使用预计算的 peer 列表索引或批量查询。

*(原 H4 修订：从 High 降为 Medium，因为核心查找是 O(1) 哈希，线性遍历是预期行为，尚未证明是实际瓶颈。)*

---

### M4. SourceWorker 原始 FFmpeg 指针未用 RAII

**文件**: `client/SourceWorker.h:471-473`

`encoder_`、`scaledFrame_`、`swsCtx_` 是原始指针，依赖循环退出时手动释放。项目已有 `common/ffmpeg/AvPtr.h` 的 RAII 封装（`CodecContextPtr`、`FramePtr`），但 SourceWorker 没有使用。这是代码库最大的复用缺口。

---

### M5. SourceWorker 的 rejectFirstSetEncodingTrackIndex_ 受环境变量控制

**文件**: `client/SourceWorker.h:483-484`

```cpp
std::optional<uint32_t> rejectFirstSetEncodingTrackIndex_ =
    loadOptionalTrackIndexEnv("QOS_TEST_REJECT_FIRST_SET_ENCODING_TRACK_INDEX");
```

生产代码中通过环境变量注入测试行为。如果生产环境误设此变量，第一个 SetEncoding 请求会被拒绝。

**修复**: 仅在 debug build 中生效，或加前缀检查（如 `QOS_TEST_`）。

---

### M6. Consumer::handleNotification 中 PRODUCER_CLOSE 发射 @close 事件

**文件**: `src/Consumer.cpp:114-115`

```cpp
emitter_.emit("@close");
emitter_.emit("producerclose");
```

`@close` 通常是 API close 的专属事件。从 handleNotification 发射 `@close` 可能导致监听者误以为 Consumer 被 API 关闭，而非 producer 断开。

**修复**: 只发射 `"producerclose"`，不发射 `"@close"`；或使用不同的事件名。

---

### M7. Consumer 没有禁用拷贝/移动

**文件**: `src/Consumer.h`

Consumer 持有裸指针 `Channel*` 和 `EventEmitter`，但没有声明拷贝/移动构造函数为 delete。如果意外拷贝，两个对象会共享同一个 `channel_`。

**修复**: `Consumer(const Consumer&) = delete; Consumer& operator=(const Consumer&) = delete;`

---

### M8. QosController 缩进不一致

**文件**: `client/qos/QosController.h:643-645`

```cpp
		int snapshotSeq_ = 0;       // 双 tab
		int sampleIntervalMs_ = ...; // 双 tab
		int snapshotIntervalMs_ = ...; // 双 tab
```

与周围成员的缩进不一致。

---

### M9. QosCoordinator::buildCoordinationOverrides 冗余计算 PeerDecision

**文件**: `client/qos/QosCoordinator.h:200`

`buildCoordinationOverrides` 内部调用 `buildPeerDecision(tracks)`，但调用者通常已经计算过。应将已有的 `PeerDecision` 作为参数传入。

---

### M10. SignalingSocketState::RegisterJoinedSocket O(N) 扫描

**文件**: `src/SignalingSocketState.h:146-160`

为清理 stale mapping 遍历 `wsMap->peers` 全部条目。大房间下 O(N)。

**修复**: 维护 `socket -> key` 反向索引。

---

### M11. Transport::close() vs routerClosed() 重复清理路径

**文件**: `src/Transport.cpp`

两方法做近乎相同的清理（close producers/consumers），只差一个 close request 和事件名。如果一条路径更新而另一条没更新，会产生不一致。

**修复**: 提取共享清理逻辑到私有方法。

---

### M12. common/ffmpeg/Decoder::ReceiveFrame 和 Encoder::SendFrame 无 null 检查

**文件**: `common/ffmpeg/Decoder.cpp:53`, `common/ffmpeg/Encoder.cpp:40`

`ReceiveFrame(AVFrame* frame)` 和 `SendFrame(const AVFrame* frame)` 不检查 `frame` 是否为 null，直接传给 FFmpeg API。如果传入 null，会 segfault。

**修复**: 加 null 检查并抛出异常。

---

### M13. common/ffmpeg/BitstreamFilter::Create 无 inputParameters null 检查

**文件**: `common/ffmpeg/BitstreamFilter.cpp:36`

`avcodec_parameters_copy(context->par_in, inputParameters)` 在 `inputParameters` 为 null 时会 segfault。

**修复**: 加 null 检查。

---

## 四、Low（可改进）

### L1. Channel::nextId_ wrap-around 与 pending request id 冲突

**文件**: `src/Channel.h:148`, `src/Channel.cpp:244`

`nextId_` 在 4294967295 后回到 1，可能与旧的 pending request id 冲突。实际风险极低。

---

### L2. SourceWorker 的 printf 日志应使用项目 Logger

**文件**: `client/SourceWorker.h` 全文

所有日志使用 `printf` 而非项目的 `Logger` 或 `spdlog`。与项目其他代码风格不一致。

---

### L3. RoomRegistryPubSub::/dev/urandom 不可用导致雷群保护失效

**文件**: `src/RoomRegistryPubSub.cpp:48-52`

如果 `/dev/urandom` 不可用（受限容器），所有节点同时 sync，失去雷群保护。

---

### L4. Recorder pendingAudio_ 满时无日志

**文件**: `src/Recorder.cpp:423-428`

当 H264 未产生 IDR 帧导致 pendingAudio_ 满时，音频包被静默丢弃，无任何日志，影响调试。

---

### L5. RtcpHandler::getNtpNow 使用 gettimeofday 非单调

**文件**: `client/RtcpHandler.h:35-44`

`gettimeofday()` 可因 NTP 调整回退，导致 SR 时间戳非单调。

---

### L6. browser_public_interop.mjs finally 中 browser.close() 可能抛 TypeError

**文件**: `tests/qos_harness/browser_public_interop.mjs:262-264`

如果 `launchBrowser()` 失败，`browser` 为 undefined，`finally` 中 `browser.close()` 抛 TypeError，遮蔽原始错误。

---

### L7. PlainClientThreaded 音频线程独立打开同一文件

**文件**: `client/PlainClientThreaded.cpp:87`

音频线程创建自己的 `InputFormat` 打开同一 MP4 文件，导致文件 I/O 翻倍。

---

## 五、架构级观察

### A1. Consumer/Producer 缺乏统一生命周期管理

close 路径分散在 `RoomServiceMedia::closeTransport`、`Consumer::close()`、`Consumer::transportClosed()`、`Consumer::handleNotification(PRODUCER_CLOSE)` 四处。没有统一的 "dying -> dead" 状态机，容易遗漏清理步骤。建议引入 `State::Closing` 中间态。

---

### A2. Channel 双模式（threaded/non-threaded）增加认知负担

两条路径的 timeout 处理、close 逻辑、re-entry 行为各不相同。所有新代码走 threaded 模式。建议标记 non-threaded 路径为 deprecated，长期移除。

---

### A3. QoS 子系统 header-only 设计

`client/qos/` 下所有文件都是 `.h` header-only，总代码量超过 2000 行。导致修改任何 QoS 文件触发大量重编译、无法做独立单元测试二进制、断点调试困难。建议拆分为 .h/.cpp 对。

---

### A4. SourceWorker 与 common/ffmpeg 层脱节

项目有精心设计的 `common/ffmpeg/` RAII 层，但 SourceWorker 完全没用，直接使用原始 FFmpeg API。`initEncoder`、`loopFile`、`loopCamera` 中的 FFmpeg 操作本应使用 `Decoder`、`Encoder` 封装，避免 C1（avcodec_open2 返回值未检查）这类问题。

---

### A5. 测试中缺少并发验证手段

项目有大量多线程代码（NetworkThread、Prober worker、WsClient reader、SourceWorker），但没有任何 TSan 运行记录。SpscQueue 的并发测试只验证功能正确性，没有验证数据竞争自由。这是整个项目最大的验证缺口。

---

### A6. Worker 重启后丢失全部房间状态

`WorkerThread::onWorkerDied` (WorkerThread.cpp) 重启 Worker 后，所有 rooms/routers 丢失。新 Worker 无之前房间信息，RoomService 和 RoomManager 仍引用旧（已死）Worker 的 routers，后续操作全部失败。这是架构层面的硬伤。

---

## 六、按模块统计

| 模块 | Critical | High | Medium | Low |
|------|----------|------|--------|-----|
| src/ (server) | 0 | 2 (H1, H3) | 6 (M1, M3, M6, M7, M10, M11) | 3 (L1, L3, L4) |
| client/ (plain client) | 1 (C1) | 1 (H2) | 5 (M2, M4, M5, M8, M9) | 3 (L2, L5, L7) |
| common/ (shared) | 0 | 0 | 2 (M12, M13) | 0 |
| tests/ (harness) | 0 | 0 | 0 | 1 (L6) |
| 架构 | 0 | 0 | 0 | 0 |
| **合计** | **1** | **3** | **13** | **7** |

加上 6 条架构观察（A1-A6），总计 30 项发现。

---

## 七、最高优先级修复建议

| 优先级 | 项 | 理由 |
|--------|------|------|
| 1 | C1 SourceWorker avcodec_open2 返回值 | 运行时崩溃风险 |
| 2 | H1 RoomRegistrySync scanKeys 持锁 | 阻塞全部 Redis 操作 |
| 3 | H2 FlushAudioQueue 可饿死视频 | 视频流完全停滞 |
| 4 | H3 Channel recvBuf_ erase 效率 | 高频场景性能瓶颈 |
| 5 | A5 引入 TSan | 并发验证空白 |
| 6 | A4 SourceWorker 用 common/ffmpeg 封装 | 消除 C1 的根因 |

---

## 八、覆盖边界与验证缺口

### 已源码复核的高优先级条目

- `Consumer` / `EventEmitter` / `Channel` 路径：用于复核原 C1、原 C4、现 H3。
- `SourceWorker` 路径：用于复核现 C1、原 C3、M4、M5、A4。
- `PublisherQosController` / `QosPlanner` / `RoomServiceQos`：用于复核原 H1、原 H3、原 H4。
- `RoomRegistrySync` / `SenderTransportController`：用于复核现 H1、H2。

### 本轮未逐条深挖的范围

- `M6-M13`、`L1-L7`、`A1-A6` 中，除与上述高优先级条目直接相关的交叉引用外，其余仍主要基于代码阅读结论，未在本轮文档修订时重新逐项复核。
- 因此这些条目更适合作为“待修候选项”或“待二次确认项”，不应被误读为与高优先级条目同等确定。

### 未执行的验证

- 未运行单元测试、集成测试、QoS harness、TSan、性能基准。
- 所有性能类判断（如 `H2`、`H3`、`M1`、`M3`）当前主要来自代码路径分析，而非 benchmark 证据。
- `A5` 指出的并发验证缺口仍然存在；本修订版只是把该缺口明确记录下来，并未补上验证证据。

---

## 九、结论摘要

修订后，这份报告里当前最值得优先处理的现状问题仍是：`SourceWorker` 的 FFmpeg 失败路径未显式处理、`RoomRegistrySync` 的 Redis 同步全程持锁、`SenderTransportController` 的音频优先可能饿死视频、以及 `Channel` 非线程路径的热缓冲低效擦除。其余 Medium/Low/Architecture 条目里混有优化项和待二次确认项，排期时应与上述已复核高优先级问题分开对待。

---

## 附录：初版误报说明

以下条目经源码复核后删除或降级，记录理由如下：

| 原编号 | 原严重度 | 结论 | 理由 |
|--------|----------|------|------|
| 原 C1 | Critical | **删除** | Worker 拒绝时 `requestWait` 经 `promise.set_exception` 抛异常，`paused_` 根本不会被更新，不存在"设了但 worker 不接受"的不一致 |
| 原 C3 | Critical | **删除** | RTP 时间戳 `rtpTs = (uint32_t)(ptsSec * 90000)` 直接来自源文件 PTS 或 steady_clock，与 encoder 内部 timebase 无关，`encoderRecreated_` 不影响时间戳计算 |
| 原 C4 | Critical | **降为 M1** | once 重入在当前实现中安全（锁内擦除后才释放锁调用回调）；`off(id)` 暴力遍历是性能问题非正确性问题；id 单调递增下"误删新监听器同 id"不成立 |
| 原 H1 | High | **降为 M2** | `handleOverride()` 每次调 `mergeOverrides()` 会清理过期项，"无界增长"不成立；仅"暂停+无消息"组合下有残留风险 |
| 原 H3 | High | **删除** | Camera profile maxLevel ladder step 的 `enterAudioOnly=true`，Congested+maxLevel 走的就是降级到 audio-only，不是"卡在最高级不降级" |
| 原 H4 | High | **降为 M3** | 核心查找 `qosRegistry_.Get()` 是 O(1) 哈希，线性遍历 peers 是预期行为，尚未证明是实际瓶颈 |

---

## 附录：2026-04-28 修复记录

以下条目在 `changes/2026-04-28-client-quality-systematic-refactor/` 中已修复：

| 编号 | 严重度 | 状态 | 修复说明 |
|------|--------|------|----------|
| C1 | Critical | **已修复** | `SourceWorker` 全面使用 `common/ffmpeg` RAII 封装（`Decoder::OpenFromParameters`、`Encoder::Create`），`avcodec_open2` 返回值通过 `CheckError` 显式处理。 |
| M4 | Medium | **已修复** | `SourceWorker` 成员变量 `encoder_`、`scaledFrame_`、`swsCtx_` 及 `runLoop` 局部变量全部替换为 RAII 类型（`ffmpeg::Encoder`、`FramePtr`、`SwsContextPtr`、`PacketPtr`、`InputFormat`）。 |
| M5 | Medium | **已修复** | `loadOptionalTrackIndexEnv` 及 `PlainClientSupport.cpp` 中所有测试环境变量读取均已用 `#ifdef MEDIASOUP_TEST_HOOKS` 守卫，非 test build 强制返回 `nullopt`/`nullptr`。 |
| L2 | Low | **已修复** | `SourceWorker`、`PlainClientApp`、`PlainClientLegacy`、`PlainClientThreaded`、`QosController.h`、`NetworkThread.h` 中全部 `printf`/`fprintf` 已替换为 `spdlog`。 |
| A4 | Architecture | **已修复** | `SourceWorker` 与 `common/ffmpeg` 层已对接，消除原始 FFmpeg API 的直接使用；公共循环骨架 `runLoop(SourceKind, InputFormat, int)` 提取完成。 |

**未修复（本 change set 范围外）**：H1、H2、H3、M1-M3、M6-M13、L1、L3-L7、A1-A3、A5-A6 仍保持原状。
