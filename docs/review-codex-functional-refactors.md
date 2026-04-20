# Code Review: `codex/2026-04-20-functional-refactors`

> **Review date:** 2026-04-20  
> **Base branch:** `main`  
> **Commits:** 4 (`7d43c04`, `594e541`, `657b7a2`, `60516f1`)  
> **Scope:** 82 files changed, +7971 / −3792 lines

---

## 1 变更概览

| 提交 | 主题 | 涉及模块 |
|------|------|----------|
| `7d43c04` | client: split plain-client runtime and shared media helpers | `client/`, `common/`, `cmake/`, `tests/` |
| `594e541` | qos: acknowledge stats after worker processing | `src/SignalingServerWs.cpp`, `src/SignalingSocketState.h`, `src/RoomService*`, `specs/` |
| `657b7a2` | registry: split room registry internals and harden resolve path | `src/RoomRegistry*`, `src/Recorder.*` |
| `60516f1` | docs: refresh architecture notes and add non-qos test alias | `docs/`, `scripts/`, `specs/` |

### 1.1 重构方向总结

| 重构区域 | 原始状态 | 重构后 |
|----------|----------|--------|
| **client/main.cpp** | 2273 行单文件 | `PlainClientApp.{h,cpp}` + `WsClient.{h,cpp}` + `PlainClientSupport.{h,cpp}` + `PlainClientLegacy.cpp` + `PlainClientThreaded.cpp` + 6 行 `main.cpp` |
| **src/RoomRegistry** | 单 `.cpp` + raw `redisContext*` + flat `cacheMutex_` | `CommandConnection` / `CacheView` 封装 + 拆分 `Connection`/`PubSub`/`Sync` 三个 `.cpp` |
| **src/RoomServiceStats** | 单 523 行 `.cpp` | 拆成 `Stats`/`Qos`/`Downlink`/`RegistryView` 四文件；`setClientStats` / `setDownlinkClientStats` 返回 `Result` |
| **src/Recorder** | header-only 实现 + raw `AVFormatContext*` | `.h` 声明 + `.cpp` 实现；使用 `common/ffmpeg` RAII 包装 |
| **common/ffmpeg/** | 不存在 | FFmpeg RAII adapter 层（`AvPtr.h`, `InputFormat`, `OutputFormat`, `Decoder`, `Encoder`, `BitstreamFilter`） |
| **common/media/** | 不存在 | 共享 H.264/RTP helper（`AnnexB`, `Avcc`, `H264Packetizer`, `RtpHeader`） |
| **SignalingServerWs** | `clientStats` / `downlinkClientStats` 立即响应 | worker 处理完成后 defer 响应 + `stored` / `reason` 语义 |

---

## 2 Issue 汇总

### 2.0 严重度说明

| 等级 | 含义 |
|------|------|
| 🔴 **Critical** | 会导致运行时崩溃、数据损坏或违反协议规范 |
| 🟠 **High** | 存在潜在 bug 或重要行为变更需要关注 |
| 🟡 **Medium** | 设计缺陷、线程安全隐患或效率问题 |
| 🟢 **Low** | 代码风格、可维护性或边缘场景 |

---

### 2.1 🔴 Critical Issues

#### C-1. FU-A Indicator 丢失 F (Forbidden) 位

| 项 | 值 |
|---|---|
| 文件 | `common/media/rtp/H264Packetizer.cpp:55` |
| 影响 | H.264 RTP FU-A 分片不符合 RFC 6184 §5.8 |

```cpp
const uint8_t fuIndicator = static_cast<uint8_t>((naluHeader & 0x60) | 28);
//                                                 ^^^^
// 0x60 只取 NRI (bit 6-5)，丢失 F bit (bit 7)
// 应为 0xE0 以保留 F|NRI
```

- `0x60` 只保留 NRI（bit 5-6），F 位（bit 7）被丢弃。
- 合规流 F 总是 0，实际影响低。但当 F=1 的错误 NAL 被分片后再重组，F 信息会丢失。
- **修复**：`naluHeader & 0xE0`。

---

#### C-2. `OutputFormat::Close()` 不写 Trailer 导致容器损坏

| 项 | 值 |
|---|---|
| 文件 | `common/ffmpeg/OutputFormat.cpp:91-99` |
| 影响 | 录制文件容器截断/无法播放 |

```cpp
void OutputFormat::Close() {
    if (!ctx_) return;
    // ❌ 未检查 headerWritten_，未调用 av_write_trailer
    if (ioOpened_ && ctx_->pb)
        avio_closep(&ctx_->pb);
    avformat_free_context(ctx_);
    // ...
}
```

- 若 `WriteHeader()` 已调用但未调用 `WriteTrailer()`，析构时直接 free context 会产生不完整容器。
- Recorder.cpp 的 `finalizeMuxer()` 已 try/catch 调用 `WriteTrailer()`，所以正常路径没问题。但异常退出或忘记调用 `finalizeMuxer()` 时会触发此问题。
- **修复**：在 `Close()` 中若 `headerWritten_` 则 try/catch 调用 `av_write_trailer`。

---

#### C-3. `Decoder::SendPacket` / `Encoder::SendFrame` 对 EAGAIN 抛异常

| 项 | 值 |
|---|---|
| 文件 | `common/ffmpeg/Decoder.cpp:32`, `common/ffmpeg/Encoder.cpp:30`, `common/ffmpeg/BitstreamFilter.cpp:31` |
| 影响 | 正常 codec pump 循环无法实现 |

```cpp
void Decoder::SendPacket(const AVPacket* packet) {
    CheckError(avcodec_send_packet(context_.get(), packet), "avcodec_send_packet");
    // AVERROR(EAGAIN) 是正常流控信号，表示需要先 ReceiveFrame drain
    // 但 CheckError 会抛 runtime_error
}
```

- `ReceiveFrame()` / `ReceivePacket()` 正确处理了 EAGAIN（返回 `false`），但 Send 端没有。
- 不对称行为使得正确的 send→drain 循环很难写。
- **修复**：Send 系列也应对 EAGAIN 返回 `false` 或 `std::optional`，而不是抛异常。

---

### 2.2 🟠 High Issues

#### H-1. `RoomRegistry::start()` 中 `registerNode()` 未持锁

| 项 | 值 |
|---|---|
| 文件 | `src/RoomRegistry.cpp:37-42` |
| 影响 | 与 `heartbeat()` 并发时存在 data race |

```cpp
void RoomRegistry::start()
{
    registerNode();   // ❌ 无 command_.mutex
    syncAll();        // syncAll 内部加锁
    startSubscriber();
}
```

- `registerNode()` 调用 `command_.command()` 和 `publishNodeUpdate()`，均访问 Redis 连接。
- 当前 `start()` 在单线程初始化阶段调用（`MainBootstrap.cpp:329`），所以无实际 race。
- 但违反了 `heartbeat()` / `updateLoad()` 等所有公共方法的锁协议。
- **修复**：在 `start()` 内 `registerNode()` 前加 `std::lock_guard<std::mutex> lock(command_.mutex);`。

---

#### H-2. `av_bsf_alloc` 返回值被忽略

| 项 | 值 |
|---|---|
| 文件 | `common/ffmpeg/AvPtr.h:64-68` |
| 影响 | 分配失败时无错误传播 |

```cpp
inline BitstreamFilterContextPtr MakeBitstreamFilterContext(const AVBitStreamFilter* filter) {
    AVBSFContext* context = nullptr;
    if (filter) av_bsf_alloc(filter, &context);  // ❌ 返回值被忽略
    return BitstreamFilterContextPtr(context);
}
```

- OOM 时 `context` 为 null，下游 `BitstreamFilter::Create()` 会检查 null 并抛异常。
- 但错误码丢失（可能是 `AVERROR(ENOMEM)` 或其他），错误消息不准确。
- **修复**：应 `CheckError(av_bsf_alloc(...), "av_bsf_alloc")`。

---

#### H-3. `setClientStats` / `setDownlinkClientStats` 响应语义变更

| 项 | 值 |
|---|---|
| 文件 | `src/SignalingServerWs.cpp`, `src/RoomServiceQos.cpp:63-69` |
| 影响 | 协议行为变更 |

**旧行为**:
- `clientStats`: 立即返回 `{ok:true, data: {}}`，fire-and-forget 投递到 worker
- `downlinkClientStats`: 立即返回 `{ok:true}`，立即设置 `lastAcceptedSeq`

**新行为**:
- 两者均 defer 到 worker 处理完成后才返回 `{ok:true/false, data: {stored: true/false, reason: "..."}}`
- `setClientStats` 新增了 room/peer 存在性校验（旧代码无条件入库）
- `lastAcceptedSeq` 延迟到 worker 确认后才更新

这是 **有意的改进**，符合新的 `specs/current/qos-signaling.md` 规范。但：
1. 客户端如果对 `clientStats` 设置了短超时，会收到更多超时
2. 旧代码 `lastAcceptedSeq` 被乐观更新后不回退（如果 worker 拒绝），导致重试的同 seq 被误拒。新代码修复了这个问题 ✅

---

#### H-4. `WsClient` 所有成员变量为 public

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.h:52-60` |
| 影响 | 封装性丧失 |

`fd`, `running_`, `nextId_`, `readerThread_`, `stateMutex_`, `sendMutex_`, `pendingRequests_` 等全部 public。任何消费者都可以直接修改内部状态绕过锁保护。

- 原始代码用 `struct`（默认 public），但既然已迁移到 `class`，应该设为 `private`。

---

#### H-5. `NetworkThread` 整个实现在 header 文件

| 项 | 值 |
|---|---|
| 文件 | `client/NetworkThread.h` (~530 行) |
| 影响 | 编译时间、代码膨胀 |

完整的 `loop()`、`processIncomingRtcp()`、`parseRR()`、`drainQueues()` 等实现全在 header。因为方法在 class body 内所以隐式 inline，不会链接出错。但显著增加编译时间。

---

### 2.3 🟡 Medium Issues

#### M-1. 重复的 `BuildStatsStoreResponseData` 辅助函数

| 项 | 值 |
|---|---|
| 文件 | `src/RoomServiceQos.cpp:10`, `src/RoomServiceDownlink.cpp:10` |
| 影响 | 代码重复 |

两个文件在匿名 namespace 中定义了相同的 helper：

```cpp
json BuildStatsStoreResponseData(bool stored, std::string reason = "")
```

**修复**：提取到 `RoomStatsQosHelpers.h`。

---

#### M-2. `CacheView` 封装被 `findBestNodeCached` / `hasRemoteNodeCached` 打破

| 项 | 值 |
|---|---|
| 文件 | `src/RoomRegistrySync.cpp:170, 205` |
| 影响 | 封装泄漏 |

这两个方法直接 lock `cache_.mutex` 并遍历 `cache_.nodes`，绕过了 `CacheView` 的 accessor 方法。

```cpp
std::lock_guard<std::mutex> lock(cache_.mutex);
for (auto& [nodeId, info] : cache_.nodes) { ... }
```

**修复**：在 `CacheView` 上添加迭代 helper（如 `forEachNode`）。

---

#### M-3. RTP Header Parser 忽略 Padding 位

| 项 | 值 |
|---|---|
| 文件 | `common/media/rtp/RtpHeader.h:25-47` |
| 影响 | 带 padding 的 RTP 包解析错误 |

若 padding bit (`data[0] & 0x20`) 为 1，最后一个字节表示 padding 长度，应从 `payloadSize` 中减去。当前代码忽略了这一点。

---

#### M-4. RTP Header Parser 不验证 Version

| 项 | 值 |
|---|---|
| 文件 | `common/media/rtp/RtpHeader.h:25` |
| 影响 | 非 RTP 数据可能被误解析 |

RTP version 应为 2 (`data[0] >> 6 == 2`)。当前无校验。

---

#### M-5. `IsAdvancingDownlinkSeq` baseline 改用 `pendingSeq`

| 项 | 值 |
|---|---|
| 文件 | `src/SignalingSocketState.h:52-60` |
| 影响 | 语义改变（改进） |

```cpp
// 旧
if (!state.hasAcceptedSeq) return true;
if (seq > state.lastAcceptedSeq) return true;

// 新
const uint64_t baselineSeq = state.pending ? state.pendingSeq : state.lastAcceptedSeq;
if (!state.pending && !state.hasAcceptedSeq) return true;
if (seq > baselineSeq) return true;
```

当 `pending=true` 时，以 `pendingSeq` 为基准而非 `lastAcceptedSeq`。这防止了 in-flight 期间的重复请求被接受。✅ 正确改进。

---

#### M-6. `MarkAcceptedDownlinkSeq` 不清除 `pending` 标志

| 项 | 值 |
|---|---|
| 文件 | `src/SignalingSocketState.h:69-73` |
| 影响 | `pending` 在 worker 确认和 planner 应用之间一直为 true |

```cpp
inline void MarkAcceptedDownlinkSeq(DownlinkStatsRateLimitState& state, uint64_t seq) {
    state.lastAcceptedSeq = seq;
    state.hasAcceptedSeq = true;
    // pending NOT cleared here
}
```

`pending` 只在 `downlinkSnapshotApplied` callback 或 `ClearPendingDownlinkSeqIfMatches` 中清除。这个行为是有意的，但应该有注释说明。

---

#### M-7. WebSocket Ping 帧未响应 Pong

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.cpp:162-163` |
| 影响 | 违反 RFC 6455 §5.5.2 |

```cpp
if (opcode == 0x8) return RecvTextStatus::Closed;
if (opcode != 0x1) return RecvTextStatus::Timeout; // Ping (0x9) 被当作 Timeout
```

Ping frame 被消费但不发送 Pong。服务端可能因此断开连接。**前置问题**，原始代码同样存在。

---

#### M-8. WebSocket Continuation Frame 静默丢弃

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.cpp:163` |
| 影响 | 分片消息数据丢失 |

opcode `0x0` (continuation) 被当作 `Timeout` 返回，payload 丢失。**前置问题**。

---

#### M-9. `WsClient::sendText` 不处理 partial write

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.cpp:120` |
| 影响 | 大消息可能被截断 |

`::send(fd, frame.data(), frame.size(), 0)` 在内核缓冲区满时可能返回小于请求的字节数。**前置问题**。

---

#### M-10. `recvText` 无最大帧大小限制

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.cpp:154` |
| 影响 | 恶意服务端可 OOM 客户端 |

`std::string data(len, 0)` 中 `len` 来自网络，64 位无限制。**前置问题**。

---

#### M-11. `signal()` 代替 `sigaction()`

| 项 | 值 |
|---|---|
| 文件 | `client/PlainClientApp.cpp:192-194` |
| 影响 | `signal()` 重入行为未定义 |

**前置问题**。

---

#### M-12. FFmpeg adapter null guard 不一致

| 项 | 值 |
|---|---|
| 文件 | `common/ffmpeg/InputFormat.cpp`, `OutputFormat.cpp`, `Decoder.cpp`, `Encoder.cpp` |
| 影响 | moved-from 对象调用时 segfault |

部分方法有 null guard（`InputFormat::FindFirstStreamIndex`, `OutputFormat::Close`），部分无（`InputFormat::FindStreamInfo`, `OutputFormat::NewStream` 等）。应统一。

---

#### M-13. 缺少 `<cstdlib>` include

| 项 | 值 |
|---|---|
| 文件 | `client/PlainClientApp.cpp:160, 116-119` |
| 影响 | `std::atoi` / `std::strtol` / `std::getenv` 依赖传递 include |

---

#### M-14. Comedia Probe 在 threaded 模式只发 track 0

| 项 | 值 |
|---|---|
| 文件 | `client/NetworkThread.h:101-110` |
| 影响 | 多轨道 comedia probe 不完整 |

Legacy 路径 (`PlainClientLegacy.cpp:58-66`) 对所有 track 发送 probe，但 NetworkThread 的 `sendComediaProbe()` 只发 `tracks_[0]`。**前置问题**。

---

### 2.4 🟢 Low Issues

#### L-1. 冗余 `(void)lock;`

| 项 | 值 |
|---|---|
| 文件 | `src/RoomRegistry.cpp:139` (resolveRoom 内部) |

```cpp
std::lock_guard<std::mutex> lock(command_.mutex);
(void)lock;   // 无意义
```

---

#### L-2. Header 中使用 `using json = nlohmann::json;`

| 项 | 值 |
|---|---|
| 文件 | `client/WsClient.h:16`, `client/PlainClientSupport.h:12` |
| 影响 | 命名空间污染 |

---

#### L-3. `size_t` → `uint32_t` 截断

| 项 | 值 |
|---|---|
| 文件 | `common/media/h264/Avcc.cpp:14` |
| 影响 | 理论上 >4GB NAL 截断，实际不可能 |

---

#### L-4. Recorder.h 中残留 socket 系统头

| 项 | 值 |
|---|---|
| 文件 | `src/Recorder.h` |
| 影响 | 不必要的 header 耦合 |

`<arpa/inet.h>`, `<netinet/in.h>`, `<poll.h>`, `<sys/socket.h>`, `<unistd.h>` 仅在 `.cpp` 使用，但留在 `.h` 中。

---

#### L-5. `PlainClientThreaded.cpp` 中 static peerSnapshotSeq

| 项 | 值 |
|---|---|
| 文件 | `client/PlainClientThreaded.cpp:510` |
| 影响 | 多次调用 `RunThreadedMode` 时 seq 不重置 |

`static int peerSnapshotSeq = 0;` 在函数作用域，但 `static` 使其跨调用保持。对 main 不影响，但对测试环境有隐患。

---

#### L-6. CMakeLists.txt 未列出 header 文件

| 项 | 值 |
|---|---|
| 文件 | `client/CMakeLists.txt:39-46` |
| 影响 | IDE 集成和依赖追踪不完整 |

---

## 3 测试覆盖缺口

| 缺口 | 严重度 | 所属文件 |
|------|--------|----------|
| RTP sequence number wrap-around (0xFFFF → 0x0000) | 🟡 Medium | `test_common_media.cpp` |
| NAL 恰好等于 MTU (1200 bytes) 的边界测试 | 🟡 Medium | `test_common_media.cpp` |
| FU-A F-bit 保留验证（会暴露 C-1） | 🟡 Medium | `test_common_media.cpp` |
| RTP padding bit 解析 | 🟡 Medium | `test_common_media.cpp` |
| RTP CSRC / extension 解析 | 🟢 Low | `test_common_media.cpp` |
| 单字节 NAL | 🟢 Low | `test_common_media.cpp` |
| 仅含 3-byte start code 的流 | 🟢 Low | `test_common_media.cpp` |
| FFmpeg adapter 层的单元测试 | 🟢 Low | 缺失 |

---

## 4 ✅ 做得好的部分

1. **CacheView 封装** — 每个 accessor 内部加锁，消除了原始的 raw `cacheMutex_` / `nodeCache_` / `roomCache_` 散落访问模式
2. **CommandConnection** — RAII 析构 disconnect，不可拷贝，与 cache 正确分离
3. **QoS stats 确认语义** — 正确实现了新的 `qos-signaling.md` 规范，deferred response 的 `alive` shared_ptr guard 模式正确
4. **Downlink seq 跟踪改进** — `IsAdvancingDownlinkSeq` 使用 `pendingSeq` 作为 in-flight baseline，修复了旧代码乐观更新后不回退的问题
5. **FFmpeg RAII 封装** — `AvPtr.h` 为每种 FFmpeg 类型定义了正确的 deleter，消除了手动释放
6. **Recorder 现代化** — `std::optional<msff::OutputFormat>` 替代 raw `AVFormatContext*`，`WriteTrailer` 有 error handling
7. **函数完整性** — 旧 `RoomServiceStats.cpp` 的全部 15 个函数在 4 个新拆分文件中全部覆盖
8. **文档同步** — `DEVELOPMENT.md`, `architecture_cn.md`, `linux-client-architecture_cn.md` 准确反映新文件布局
9. **`non-qos` 测试别名** — 务实的 CI 快捷方式
10. **client 拆分忠实保留了全部原始功能** — Legacy / Threaded 路径完整，WsClient 改进了 `RecvTextStatus` 枚举替代空字符串错误信号
11. **锁序正确** — `command_.mutex → cache_.mutex`，无循环依赖，无死锁风险

---

## 5 修复优先级建议

| 优先级 | Issue | 建议 |
|--------|-------|------|
| P0 | C-1 FU-A F-bit | `0x60` → `0xE0` |
| P0 | C-2 OutputFormat::Close trailer | `Close()` 中 headerWritten_ 时 try/catch `av_write_trailer` |
| P0 | C-3 EAGAIN throws | Send 系列对 EAGAIN 返回 false / optional |
| P1 | H-1 start() 未持锁 | `registerNode()` 前加 lock_guard |
| P1 | H-2 av_bsf_alloc | 改为 `CheckError(av_bsf_alloc(...))` |
| P1 | M-1 重复 helper | 提取到 `RoomStatsQosHelpers.h` |
| P2 | M-2 CacheView 封装 | 添加 `forEachNode` helper |
| P2 | M-3/M-4 RTP padding/version | 增加 padding 处理 + version 校验 |
| P2 | H-4 WsClient 封装 | 成员改为 private |
| P3 | 其余 Low | 按便利性修复 |

---

## 6 结论

重构方向正确，四个维度的拆分均有意义：

- **server src/** 的 RoomRegistry 和 RoomService 拆分提高了可维护性和关注点分离
- **client/** 的 monolithic main.cpp 拆分使 legacy/threaded 路径独立可测
- **common/** 层的引入消除了 src/ 和 client/ 之间的代码重复
- **QoS stats deferred response** 修复了旧代码中乐观 seq 更新不回退的逻辑缺陷

3 个 Critical issue（C-1 ~ C-3）均在新引入的 `common/` 代码中，应在合并前修复。其余 High/Medium issue 建议按优先级在后续迭代中处理。
