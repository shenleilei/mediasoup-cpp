# Code Review: `dev/linux_client` → `dev/downside_qos`

> 审查范围: 10 commits (`d6a3891..1d09608`), 36 files, +9534/-4 lines
> 审查日期: 2026-04-18
> 审查方法: 逐文件对比代码逻辑，非泛泛 style review

---

## 一、变更概览

| 模块 | 变更文件 | 新增/修改 | 核心功能 |
|------|---------|----------|---------|
| 服务端 Transport | `Peer.h`, `RoomService.cpp/.h`, `SignalingServer.cpp` | 修改 | PlainTransport 生命周期管理 |
| 服务端 Codec | `src/main.cpp` | 修改 | 新增 H264 Baseline profile |
| 服务端 QoS 校验 | `QosValidator.cpp` | 修改 | track scope 必须带 trackId |
| JS 协议层 | `protocol.js` | 修改 | 同步 trackId 校验逻辑 |
| 客户端 RTCP | `RtcpHandler.h` | 新增 | SR/RR/NACK/PLI 处理 |
| 客户端 QoS 引擎 | `client/qos/*.h` (11文件) | 新增 | 完整 QoS 状态机从 JS 迁移到 C++ |
| 客户端主程序 | `client/main.cpp` | 新增 | PlainTransport MP4 推流 + QoS 集成 |
| 测试 | `test_client_qos.cpp` + 其他 | 新增/修改 | 1816行客户端 QoS 单元测试 |
| 测试脚手架 | `tests/qos_harness/*.mjs` (8文件) | 新增 | E2E 测试运行器 |

---

## 二、逻辑问题详情（按严重程度排序）

### 🔴 P0 — 高严重度

#### 问题 1: PlainTransport 覆盖赋值导致 Worker 侧资源泄漏

**文件**: `src/RoomService.cpp` — `plainPublish()` 第457行、`plainSubscribe()` 第582行、`createPlainTransport()` 第380行

**代码**:
```cpp
// plainPublish() 第457行
auto transport = room->router()->createPlainTransport(opts);
peer->plainSendTransport = transport;   // ← 直接赋值，旧 transport 未 close
```

**问题分析**:

如果同一个 peer 多次调用 `plainPublish`（例如客户端断线重连后重新推流），`peer->plainSendTransport` 被直接覆盖。旧的 `shared_ptr` 引用计数归零后析构，但 **mediasoup worker 进程侧的 transport 未收到 close 命令**。

对比 `Peer::close()` 方法（Peer.h 第30-34行）的写法：
```cpp
if (sendTransport) { sendTransport->close(); sendTransport.reset(); }  // ← 先 close 再 reset
if (recvTransport) { recvTransport->close(); recvTransport.reset(); }
if (plainSendTransport) { plainSendTransport->close(); plainSendTransport.reset(); }
```

`close()` 方法明确先调用 `close()` 向 worker 发送关闭命令，再 `reset()` 释放指针。这说明项目已经建立了"析构 ≠ close"的约定。但 `plainPublish` / `plainSubscribe` / `createPlainTransport` 三处都没有遵循。

**影响**: worker 进程中的旧 transport 和关联的 producer/consumer 残留，占用端口和内存，且无法通过正常接口清理。

**修复方向**:
```cpp
// 在赋值前检查并 close 旧 transport
if (peer->plainSendTransport) {
    // 需要同时清理该 transport 上的 producers
    peer->plainSendTransport->close();
    peer->plainSendTransport.reset();
}
auto transport = room->router()->createPlainTransport(opts);
peer->plainSendTransport = transport;
```

**注意**: 仅 close transport 可能不够——旧 transport 上创建的 producer 也在 `peer->producers` 和 `router` 中注册过。需要清理关联 producer/consumer，否则 `indexPeerProducers` 中的 owner cache 会指向已关闭的 producer。

---

#### 问题 2: plainPublish 中 H264 profile 选择可能不匹配客户端

**文件**: `src/RoomService.cpp` 第461-468行 + `src/main.cpp` 第321-324行 + `client/main.cpp` 第973行

**服务端 codec 列表** (`src/main.cpp` 第321-324行):
```cpp
{{"mimeType", "video/H264"}, {"parameters", {{"profile-level-id", "4d0032"}, ...}}},  // Main profile — 排在第一
{{"mimeType", "video/H264"}, {"parameters", {{"profile-level-id", "42e01f"}, ...}}},  // Baseline — 排在第二
```

**plainPublish codec 选择** (`RoomService.cpp` 第461-466行):
```cpp
for (auto& c : caps.codecs) {
    if (c.mimeType == "video/H264" && videoPt == 0)
        videoPt = c.preferredPayloadType;  // ← 取第一个匹配的 H264
}
```

**客户端编码器** (`client/main.cpp` 第975行):
```cpp
av_opt_set(newEncoder->priv_data, "profile", "baseline", 0);  // ← 固定 Baseline
```

**逻辑问题**:

`plainPublish` 取 router 中**第一个** H264 codec 的 payload type。但 router 的 codec 列表中 Main profile (`4d0032`) 排在 Baseline (`42e01f`) 前面。如果 router 给两个 profile 分配了不同的 payload type，producer 注册的 PT 可能对应 Main profile，而客户端实际发送的是 Baseline 编码的数据。

**当前为什么没出错**: 客户端使用 `plainPublish` 返回的 `videoPt` 发送 RTP 包（main.cpp 第1056行），所以 PT 值匹配 producer 注册值。但 RTP 声明的 codec profile 与实际编码的 profile 不一致——mediasoup worker 可能基于 profile 做 codec negotiation 或 simulcast routing 决策时会出问题。

**修复方向**: `plainPublish` 中应明确指定选择 Baseline profile 的 H264 codec，或者让客户端告知期望的 profile-level-id。

---

### 🟡 P1 — 中严重度

#### 问题 3: SignalingServer 默认 SSRC 固定值可导致多客户端 SSRC 碰撞

**文件**: `src/SignalingServer.cpp` 第861-866行

```cpp
if (videoSsrcs.empty()) {
    videoSsrcs.push_back(data.value("videoSsrc", 1111u));  // ← 固定默认值
}
result = rs->plainPublish(roomId, peerId, videoSsrcs,
    data.value("audioSsrc", 2222u));  // ← 固定默认值
```

**逻辑问题**:

默认 SSRC 值 `1111` 和 `2222` 是**硬编码常量**。如果同一房间有两个不同的 plain client 都没指定 SSRC，它们的 producer 会使用完全相同的 SSRC。mediasoup worker 对同一 router 内的 SSRC 冲突的处理未定义——可能导致 RTP 包被错误路由到错误的 consumer。

**对比**: WebRTC 路径中 SSRC 由浏览器 SDP 协商自动分配，不会有此问题。PlainTransport 路径是新引入的，缺少这层保护。

**当前客户端不受影响**: `client/main.cpp` 第1046-1049行生成了唯一 SSRC (`11111111 + i * 1111`)，不依赖默认值。但服务端接口的默认值本身是有问题的。

**修复方向**: 服务端在未提供 SSRC 时应随机生成（例如 `std::random_device` + uniform distribution），而不是使用固定默认值。

---

#### 问题 4: PLI 响应重传整帧可能造成突发拥塞

**文件**: `client/RtcpHandler.h` — `processIncomingRtcp()` 中 PLI 处理

```cpp
if ((fmt == 1 || fmt == 4) && stream && !stream->lastKeyframe.empty() && sendH264Fn) {
    sendH264Fn(udpFd, stream->lastKeyframe.data(), stream->lastKeyframe.size(),
        stream->payloadType, stream->lastKeyframeTs, stream->ssrc, *stream->seqPtr);
}
```

**逻辑问题**:

`lastKeyframe` 缓存的是编码器输出的**完整 NAL 单元数据**（可能 50KB+），然后通过 `sendH264Fn` 进行 RTP 分片发送。与 NACK 重传（每次只发单个 RTP 包，约 1.4KB）不同，PLI 响应会**瞬间突发发送数十个 RTP 包**。

在已经拥塞的网络中（PLI 通常因为丢包触发），这个突发会进一步加剧拥塞，形成正反馈循环。

**对比**: mediasoup worker 自身处理 PLI 时是请求编码器产生新的 IDR 帧，逐帧发送而非突发重传。

**缓解措施**: 作为初始实现可以接受，但应加入发送速率限制（pacing），或者至少限制 PLI 响应频率（例如同一 stream 的 PLI 每 2 秒最多响应一次）。

---

#### 问题 5: 客户端 packetsLost 数据源切换时可能导致 loss rate 跳变

**文件**: `client/main.cpp` 第1476-1507行

```cpp
// 1. 先从 RTCP RR 读取 packetsLost（累积值）
snap.packetsLost = rtcpStream->rrCumulativeLost;

// 2. 如果 server stats 可用，用 server 数据覆盖（有基线偏移处理）
if (cachedPeerStats.has_value() && !track.producerId.empty()) {
    ...
    snap.packetsLost = parsed->packetsLost;  // ← 经过基线偏移后的值
}
```

**逻辑问题**:

`packetsLost` 在两个数据源之间切换：
- **RTCP RR**: `rrCumulativeLost` 是原始累积值，**无基线偏移**
- **Server stats**: 经过 `lossBase` 偏移处理，是从客户端启动时开始的增量

当 server stats 可用 → 不可用（例如 getStats 请求超时）→ 再次可用时：
1. 某次 sample 使用 server stats (`packetsLost = 500 - 200 = 300`)
2. 下次 sample server stats 不可用，回退到 RTCP RR (`packetsLost = rrCumulativeLost = 580`)
3. `deriveSignals` 中 `counterDelta(580, 300) = 280`，但实际增量可能只有几个包

这会导致 `lossRate` 和 `lossEwma` 产生一个异常大的跳变，可能误触状态机从 Stable 直接转到 EarlyWarning/Congested。

**修复方向**: 应分别为两个数据源维护独立的 `prevSnapshot`，或者在切换时重置 delta 计算。

---

### 🟢 P2 — 低严重度

#### 问题 6: createPlainTransport 的 `consuming` 参数未使用

**文件**: `src/RoomService.cpp` 第366-413行

```cpp
RoomService::Result RoomService::createPlainTransport(const std::string& roomId,
    const std::string& peerId, bool producing, bool consuming)  // ← consuming 参数
{
    ...
    if (producing) peer->plainSendTransport = transport;
    else           peer->plainRecvTransport = transport;  // ← 只用了 producing 的反面
    ...
    if (!producing && peer->plainRecvTransport) {  // ← 又是 producing 的反面
```

`consuming` 参数在函数体中**从未被引用**。调用方总是传入与 `producing` 互补的值，但函数逻辑只用 `producing` 来决定是 send 还是 recv transport。

**影响**: 接口签名误导调用方以为两个参数独立；如果将来有人传入 `producing=true, consuming=true`（期望双向 transport），实际只会创建 send transport。

**修复方向**: 删除 `consuming` 参数，或者实现双向 transport 支持。

---

#### 问题 7: buildSenderReport length 字段计算注释有歧义

**文件**: `client/RtcpHandler.h` 第95-97行

```cpp
// Length = 6 words (header 1 + SSRC 1 + SR body 5) → length field = 6
buf[2] = 0; buf[3] = 6;
```

**RFC 3550 定义**: length 字段 = "包总长度 / 4 - 1"。SR 包（无 report block）总长度 = 28 bytes = 7 个 32-bit words，所以 length = 7 - 1 = **6**。计算结果**正确**，但注释中 "header 1 + SSRC 1 + SR body 5 = 7, length field = 7-1 = 6" 会更清晰。当前注释说 "6 words → length field = 6" 会让人误以为 length 等于 word count 而非 word count - 1。

---

## 三、模块级逻辑正确性验证

### 3.1 客户端 QoS 状态机（`QosStateMachine.h`）

与 JS 实现对比验证：

| 转换条件 | C++ | JS | 一致性 |
|---------|-----|-----|-------|
| Stable → EarlyWarning | ≥2 consecutive warning samples | ≥2 | ✅ |
| EarlyWarning → Congested | ≥2 consecutive congested samples | ≥2 | ✅ |
| EarlyWarning → Stable | ≥3 consecutive healthy samples | ≥3 | ✅ |
| Congested → Recovering | cooldown elapsed + (≥5 recovery OR ≥2 fast recovery) | 同 | ✅ |
| Recovering → Stable | ≥5 consecutive healthy samples | ≥5 | ✅ |
| Recovering → Congested | ≥2 consecutive congested samples | ≥2 | ✅ |

**`mapStateToQuality` 逻辑**:
- Congested + sendBitrateBps ≤ 0 → `Quality::Lost` ✅（audio-only 下视频 send = 0 不应报 Lost，因为 audio-only 时状态机已经不在 Congested）
- 其他映射: Stable→Excellent, EarlyWarning→Good, Congested→Poor, Recovering→Good ✅

**`isFastRecoveryHealthy` 逻辑**:
```cpp
bool sendReady = targetBitrateBps <= 0.0 || s.sendBitrateBps >= targetBitrateBps * 0.85;
```
当 `targetBitrateBps ≤ 0`（audio-only 模式下视频 target=0），`sendReady = true`，允许恢复。✅ 正确：audio-only 下不应因 send bitrate 不足阻止恢复。

---

### 3.2 信号计算（`QosSignals.h`）

**`deriveSignals` 关键路径**:

1. **RTT unavailable 处理** (`roundTripTimeMs < 0`):
   - 使用前一个 sample 的 RTT 值，EWMA 也保持前值
   - ✅ 正确：避免了 RTT=-1 污染 EWMA 计算

2. **Jitter unavailable 处理** (`jitterMs < 0`):
   - 同 RTT 策略
   - ✅

3. **带宽受限判定**:
   ```cpp
   s.bandwidthLimited = (cur.qualityLimitationReason == Bandwidth)
       && s.bitrateUtilization < NETWORK_CONGESTED_UTILIZATION;
   ```
   - 需要同时满足 WebRTC 报告的 `qualityLimitationReason` 为 Bandwidth 且实际利用率低于阈值
   - ✅ 防止了 `qualityLimitationReason` 误报

4. **`computeLossRate` 公式**: `lostDelta / (sentDelta + lostDelta)`
   - 这是**接收端视角**的丢包率计算（RFC 3550 fraction lost 的变体）
   - 与 JS 一致 ✅

---

### 3.3 Planner 逻辑（`QosPlanner.h`）

**`resolveTargetLevel` 状态→级别映射**:
```
Stable:       target = cur - 1  （恢复一级）
EarlyWarning: target = max(1, cur)  （至少保持 level 1）
Congested:    target = cur + 1  （降级一级）
Recovering:   target = cur - 1  （恢复一级）
```

**`planActions` 关键边界**:
```cpp
if (input.source == Source::Camera && cur == maxLevel
    && input.state == State::Congested && !input.inAudioOnlyMode)
    return planActionsForLevel(input, maxLevel);
```
Camera 在最高级别 + Congested + 未进入 audio-only 时，输出 `EnterAudioOnly` action。
✅ 这确保了 Camera track 在极端拥塞下进入 audio-only 模式。

**`planActionsForLevel` action 顺序**:
1. ExitAudioOnly（如果从高级别恢复到低级别且当前在 audio-only）
2. ResumeUpstream
3. SetEncodingParameters
4. SetMaxSpatialLayer
5. EnterAudioOnly
6. PauseUpstream

✅ 顺序合理：先退出 audio-only 再调编码参数，先调参数再进入 audio-only。

---

### 3.4 Probe 验证逻辑（`QosProbe.h`）

```cpp
if (isProbeHealthy(signals, thresholds)) {
    ctx.healthySamples++;
    ctx.badSamples = 0;      // ← 重置 bad 计数
} else if (isProbeBad(signals, thresholds)) {
    ctx.healthySamples = 0;  // ← 重置 healthy 计数
    ctx.badSamples++;
}
```

**边界分析**: 当信号既不 healthy 也不 bad（处于中间地带）时，两个计数器都不变化。这意味着 Probe 在中间状态可以无限持续，不会超时。

**是否有问题**: Controller 层在 `onSample` 中有 probe-in-progress 时的 noop 保护，且 probe 在 recovery 时启动——如果网络再次恶化会触发 `isProbeBad`。所以实际上 probe 不会永远卡住。但缺少显式超时机制是一个设计风险。

---

### 3.5 Executor 幂等性（`QosExecutor.h`）

```cpp
bool execute(const PlannedAction& action) {
    if (action.type == ActionType::Noop) return true;
    std::string key = makeKey(action);
    if (key == lastKey_) return true; // idempotent
    ...
}
```

`makeKey` 包含: action type + level + bitrate + fps + scale + ptime + spatialLayer。

✅ 正确实现了幂等性——相同参数不会重复执行编码器重配置。

**注意**: `key == lastKey_` 只比较**最后一次**执行的 action。如果有 A→B→A 的序列，第三次 A 会被重新执行（因为 lastKey_ = B）。这是正确的——因为 A→B 改变了编码器状态，需要从 B 恢复到 A。

---

### 3.6 RTCP 解析正确性（`RtcpHandler.h`）

**RTP 头解析** (`onVideoRtpSent`):
```
pkt[2..3] = Sequence Number    — RFC 3550 bytes 2-3 ✅
pkt[4..7] = Timestamp          — RFC 3550 bytes 4-7 ✅
pkt[8..11] = SSRC              — RFC 3550 bytes 8-11 ✅
```

**RR Report Block 解析** (`processIncomingRtcp`):
```
rbOff+0..3   = Report SSRC     ✅
rbOff+4      = Fraction Lost   ✅
rbOff+5..7   = Cumulative Lost ✅ (24-bit, 正确取3字节)
rbOff+12..15 = Jitter          ✅
rbOff+16..19 = LSR             ✅
rbOff+20..23 = DLSR            ✅
```

**RTT 计算**:
```cpp
double dlsrMs = (double)dlsr / 65536.0 * 1000.0;
double elapsed = (double)(nowRtcp - it->second);
stream->rrRttMs = std::max(0.0, elapsed - dlsrMs);
```
- DLSR 单位: 1/65536 秒 → 乘以 1000 转毫秒 ✅
- RTT = (收到 RR 的时间 - 发送对应 SR 的时间) - DLSR ✅
- `max(0.0, ...)` 防止负值 ✅

**NTP compact timestamp (NTP middle 32 bits)** (`maybeSendSR`):
```cpp
uint32_t ntpMid = ((uint32_t)buf[10] << 24) | ((uint32_t)buf[11] << 16)
    | ((uint32_t)buf[12] << 8) | buf[13];
```
SR 包中 NTP 时间戳占 bytes 8-15。`buf[10..13]` = NTP seconds 低2字节 + NTP fraction 高2字节 = compact NTP timestamp。与 RR 中的 LSR 字段格式匹配。✅

---

### 3.7 QoS Controller override 处理（`QosController.h`）

**`handleOverride` 清除策略** (ttlMs == 0 时):

| reason 值 | 清除范围 | 正确性 |
|-----------|---------|--------|
| `"server_ttl_expired"` | 清除所有 override | ✅ server 认为全部过期 |
| `startsWith("downlink_v2_")` 或 `"downlink_v3_"` | 仅清除 downlink 类 | ✅ 不影响手动 override |
| 不以 `"server_"` 开头 | 清除所有非 server_ 前缀 | ✅ 手动清除不影响 server |
| 以 `"server_"` 开头 | 按前缀匹配清除 | ✅ 精确清除 |

**`mergeOverrides` 逻辑**: 遍历所有未过期 override，取 `maxLevelClamp` 最小值、其他字段取 OR。`pauseUpstream` 与 `resumeUpstream` 同时存在时 `resumeUpstream = false`。✅

---

### 3.8 线程安全分析（`client/main.cpp`）

**潜在竞态**: WS reader thread 与主循环线程。

分析 `ws.onNotification` 回调链:
1. WS reader thread 收到消息 → 存入 `pendingNotifications_` (加锁)
2. 主循环调用 `ws.dispatchNotifications()` → 从 `pendingNotifications_` 取出 (加锁)，在**主线程**执行回调
3. 回调中调用 `track.qosCtrl->handlePolicy()` / `handleOverride()`

因此 `handlePolicy` 和 `onSample` 都在主线程执行。✅ **不存在竞态条件**。

---

### 3.9 服务端 QoS 校验 — C++ 与 JS 协议一致性

**C++ 侧** (`QosValidator.cpp` 第347-350行):
```cpp
if (scope == "track" && (!payload.contains("trackId") || payload["trackId"].is_null())) {
    return MakeError<QosOverride>("track scope requires trackId");
}
```

**JS 侧** (`protocol.js` 新增):
```javascript
if (scope === 'track' &&
    (!('trackId' in obj) || obj.trackId === null || obj.trackId === undefined)) {
    throw new TypeError('qosOverride.trackId is required when scope is track');
}
```

✅ 两端逻辑完全一致：scope=track 时必须提供非 null 的 trackId。

---

### 3.10 Controller 中 audio-only 状态转换的信号修正

**`getTransitionSignals` 方法**（QosController.h）:

当处于 audio-only + Congested 状态时，如果网络已恢复（loss 低、RTT 低、无 bandwidth/cpu limitation），代码人为将 `bitrateUtilization` 和 `jitterEwma` 调整到刚好通过 healthy 检查的水平：

```cpp
ts.bitrateUtilization = std::max(ts.bitrateUtilization, t.stableBitrateUtilization);
ts.jitterEwma = std::min(ts.jitterEwma, std::max(0.0, sj - 0.001));
```

**为什么需要**: 进入 audio-only 后视频编码器停止，send bitrate ≈ 0，`bitrateUtilization` 极低（denominator 仍为视频目标码率），jitter 停滞在旧值。如果不修正，状态机会因为低 utilization 持续判定为 bandwidth limited，永远无法退出 Congested。

**guard 条件**: 如果 `forceAudioOnly` 是由 server override 主动触发的，则不修正——尊重 server 的决策。✅ 正确。

---

## 四、Peer.h 变更验证

新增字段:
```cpp
std::shared_ptr<PlainTransport> plainSendTransport;
std::shared_ptr<PlainTransport> plainRecvTransport;
```

**`close()` 方法**: 正确地在 WebRtc transport 之后 close plain transport。✅

**`getTransport()` 方法**: 四个 transport 都参与 ID 匹配查找。✅

**`toJson()` 方法**: 未包含 plain transport 信息——如果客户端需要查询 peer 状态中的 plain transport，需要扩展。当前不影响功能。

---

## 五、测试覆盖评估

`tests/test_client_qos.cpp` 包含 **53 个 TEST case**，覆盖:

| 模块 | 测试数 | 关键场景 |
|------|--------|---------|
| Signals 计算 | 10 | counter delta、bitrate、loss rate、EWMA、CPU/bandwidth reason |
| StateMachine | 13 | 全部 6 种转换路径 + fast recovery + jitter gate + 边界 |
| Planner | 7 | 各 source 的降级/恢复/audio-only/override clamp |
| Probe | 3 | 成功/失败/inconclusive |
| Coordinator | 4 | 多 track 优先级、牺牲策略、peer quality |
| Budget Allocator | 4 | 优先级分配、权重分配、coordination override、screenshare restore |
| Protocol | 8 | snapshot 解析/序列化、policy/override 校验、track limit |
| Profiles | 2 | 阈值/ladder 与 JS 一致性、resolve 函数 |
| Executor | 5 | action 执行、noop 跳过、幂等性、scale 变更重执行、reset |
| Controller (集成) | 12 | warmup、degrade、snapshot、override clamp/clear、policy、coordination cap |

**缺失的测试**:
- 没有测试 `plainPublish` / `plainSubscribe` 的服务端逻辑
- 没有测试 RTCP 解析正确性（`RtcpHandler.h` 全无单元测试）
- 没有测试 loss base 偏移逻辑（问题 5 的场景）

---

## 六、总结与建议

### 必须修复（合并前）

| # | 问题 | 文件 | 修复复杂度 |
|---|------|------|-----------|
| 1 | plainTransport 覆盖赋值未 close 旧 transport | `RoomService.cpp` | 中（需处理关联 producer） |
| 2 | H264 profile 选择可能不匹配客户端编码 | `RoomService.cpp` | 低 |

### 建议修复（合并后）

| # | 问题 | 文件 | 修复复杂度 |
|---|------|------|-----------|
| 3 | 默认 SSRC 固定值碰撞风险 | `SignalingServer.cpp` | 低 |
| 4 | PLI 突发重传无速率限制 | `RtcpHandler.h` | 中 |
| 5 | packetsLost 数据源切换跳变 | `client/main.cpp` | 中 |
| 6 | `consuming` 参数未使用 | `RoomService.cpp` | 低 |

### 可安全合并的部分

- **客户端 QoS 引擎** (`client/qos/*`): 与 JS 实现严格一致，测试覆盖充分
- **Peer.h 改动**: close/getTransport 逻辑正确
- **JS protocol.js**: trackId 校验与 C++ 侧对齐
- **QosValidator.cpp**: track scope 校验逻辑正确
- **测试代码**: 1816 行覆盖核心 QoS 路径
