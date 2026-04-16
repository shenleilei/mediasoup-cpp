# 上行 QoS 特性 Code Review 报告

**分支**: `dev/qos` vs `main`  
**Review 日期**: 2026-04-11  
**改动规模**: 58 个文件，+5601/-109 行  
**功能范围**: Publisher 端上行自适应码率控制（Uplink Adaptive Bitrate）

> **文档性质说明**
>
> 这是历史 review 快照，已移入 `docs/archive/`。
> 不应再作为当前 QoS 结论入口使用。
>
> 本文档分为两层：
> - **第一～十章**：原始 review 发现（历史快照），记录初次 review 时的问题和建议。部分问题在后续迭代中已被修复或缓解，以 `[已缓解]` / `[已失效]` 标注。
> - **第十一～十二章**：截至当前代码的有效项判定，以此为准确定实施优先级。
>
> 如需了解当前 QoS 总口径，请优先看 `docs/qos-status.md`。

---

## 一、上行 QoS 核心流程

### 1.1 客户端自适应控制流程

```
RTCPeerConnection.getStats()
         ↓
    signals.js (信号派生)
    - 计算 sendBitrateBps, lossRate, rttMs
    - EWMA 平滑 (alpha=0.35)
    - 判断 cpuLimited / bandwidthLimited
         ↓
    stateMachine.js (状态转换)
    - stable → early_warning (连续2个warning样本)
    - early_warning → congested (连续2个congested样本)
    - congested → recovering (cooldown过期 + 连续5个healthy样本)
         ↓
    planner.js (动作规划)
    - 根据状态决定目标 level
    - 查 ladder 表生成 action
         ↓
    actionExecutor (执行降级/升级)
    - setEncodingParameters (改码率/帧率/分辨率)
    - setMaxSpatialLayer (SVC层级)
    - enterAudioOnly / exitAudioOnly
         ↓
    signalChannel.publish() (上报服务端)
    - 每 2 秒或状态变化时上报 snapshot
```

### 1.2 服务端处理流程

```
clientStats 消息
       ↓
SignalingServer (schema 验证)
       ↓
RoomService.setClientStats()
       ↓
QosRegistry.Upsert() (seq 去重)
       ↓
QosAggregator.Aggregate() (计算 stale/lost)
       ↓
QosOverrideBuilder.BuildForAggregate()
- quality=="poor" → maxLevelClamp=2
- quality=="lost" → forceAudioOnly=true
       ↓
notify_() → 下发 qosOverride 给客户端
```

### 1.3 Ladder 配置（Camera Profile）

| Level | 码率 | 帧率 | 分辨率缩放 | 说明 |
|-------|------|------|-----------|------|
| 0 | 900kbps | 30fps | 1x | 初始质量 |
| 1 | 850kbps | 24fps | 1x | 轻微降级 |
| 2 | 700kbps | 20fps | 1.5x | 中度降级 |
| 3 | 450kbps | 12fps | 2x | 重度降级 |
| 4 | - | - | - | Audio-only |

---

## 二、架构设计

### 1.1 服务端架构

```
SignalingServer (入口验证)
       ↓
RoomService (编排层)
       ↓
┌──────┴──────┐
QosRegistry   QosAggregator
(存储/去重)    (聚合计算)
       ↓
QosOverrideBuilder
(自动决策)
       ↓
notify_/broadcast_
(下发通知)
```

**评价**: 职责分离清晰，每个类可独立测试。使用 signature 去重避免重复通知。

### 1.2 客户端架构

```
RTCStatsReport → signals.js (EWMA平滑)
                      ↓
              stateMachine.js (状态转换，有debounce)
                      ↓
                planner.js (动作规划)
                      ↓
                probe.js (恢复探测)
                      ↓
              controller.js (编排+上报)
```

**评价**: 状态机有明确的 debounce（warning 需 2 个样本，recovery 需 5 个），EWMA 平滑避免瞬时抖动。

---

## 三、上行 QoS 专项问题

### P1 - 上行控制逻辑问题

#### 3.1 降级速度可能过慢

**位置**: `src/client/lib/qos/stateMachine.js`

```javascript
case 'stable': {
    if (nextContext.consecutiveWarningSamples >= 2) {
        nextState = 'early_warning';  // 需要 2 个样本
    }
    break;
}
case 'early_warning': {
    if (nextContext.consecutiveCongestedSamples >= 2) {
        nextState = 'congested';  // 又需要 2 个样本
    }
    break;
}
```

**问题**: 从 stable 到 congested 需要至少 4 个样本（4 秒），期间丢包可能已经很严重。

**修复建议**: 
- 添加"紧急降级"路径：如果 lossRate > 15% 或 RTT > 500ms，直接跳到 congested
- 或者在 early_warning 状态就开始预防性降级

#### 3.2 planner 的 level 跳跃是单步的

**位置**: `src/client/lib/qos/planner.js`

```javascript
case 'congested': {
    targetLevel = currentLevel + 1;  // 只 +1
    break;
}
```

**问题**: 严重拥塞时（如 lossRate=20%），每次只降一级，需要多个周期才能降到合适的 level。

**修复建议**: 根据拥塞严重程度决定跳跃幅度：
- lossRate > 15% → 跳 2 级
- lossRate > 25% → 直接到 audio-only

#### 3.3 恢复探测（probe）没有考虑网络波动 `[部分缓解]`

**位置**: `src/client/lib/qos/probe.js`

```javascript
if (nextContext.badSamples >= 2) {
    return { result: 'failed' };  // 2 个 bad 样本就失败
}
if (nextContext.healthySamples >= 3) {
    return { result: 'successful' };  // 3 个 healthy 样本就成功
}
```

**原始问题**: 网络波动时，可能刚升级就遇到一个 bad 样本，导致频繁 probe 失败 → rollback → cooldown 循环。

**当前状态**: 部分缓解。controller 现在在 active probe 期间冻结进一步升档（`recoveryProbeInProgress` 时返回 `noop`），避免了之前最明显的连升导致 flap 路径。见 `controller.js:259`。

**残留风险**: probe 判定阈值仍可能偏敏感（2 个 bad 样本即失败），网络波动场景下仍有 rollback/cooldown 循环的可能，但频率已大幅降低。建议后续考虑滑动窗口或指数退避。

#### 3.4 Audio-only 模式下的 bitrateUtilization 计算有问题 `[已缓解]`

**位置**: `src/client/lib/qos/controller.js`

**原始问题**: `getTransitionSignals()` 的 bitrateUtilization hack 只在 `congested` 状态生效，转到 `recovering` 后不生效，可能导致恢复判断不准确。

**当前状态**: 已缓解。`getTransitionSignals()` 现在增加了 `networkRecovered` 前置检查（loss/RTT/bandwidth/CPU 全部达标才触发），并且同时修正 `jitterEwma`，逻辑更完整。回归测试已覆盖 audio-only 恢复路径（见 `test.qos.controller.js` 中 `exitAudioOnly` 相关用例）。

**残留风险**: 仍然是 hack 而非根本解法，但当前实现已足够安全。

#### 3.5 服务端 override 的 maxLevelClamp 可能与客户端 ladder 不匹配

**位置**: `src/qos/QosOverride.cpp`

```cpp
if (aggregate.quality == "poor") {
    auto overrideData = MakeOverride("server_auto_poor", 10000u, 2u, ...);
    // maxLevelClamp = 2，假设客户端 ladder 有 5 级
}
```

**问题**: 服务端硬编码 `maxLevelClamp=2`，但不知道客户端实际用的是什么 profile。如果客户端用的是 3 级的 profile，clamp 到 2 就没意义。

**修复建议**: 
- 方案 A: 客户端上报 `levelCount`，服务端按比例计算 clamp
- 方案 B: 使用语义化的 clamp（如 `"medium"`, `"aggressive"`）而不是数字

---

## 四、问题清单（原有）

### P0 - 安全漏洞

#### 4.1 `setQosOverride` 无权限控制

**位置**: `src/RoomService.cpp`

```cpp
Result RoomService::setQosOverride(
    const std::string& roomId, const std::string& targetPeerId, const json& overrideData)
{
    // 只检查 room/peer 存在，不检查调用者身份
    auto room = roomManager_.getRoom(roomId);
    auto peer = room->getPeer(targetPeerId);
    // ...直接发送 override
}
```

**风险**: 任意 peer 可对其他 peer 发送 `forceAudioOnly: true`，造成 DoS。

**修复建议**: 添加权限检查，只允许自己对自己设置，或 moderator 角色对他人设置。

---

### P1 - 功能缺陷

#### 4.2 seq 单调递增假设过严

**位置**: `src/qos/QosRegistry.cpp`

```cpp
if (snapshot.seq <= it->second.snapshot.seq) {
    *rejectReason = "stale-seq";
    return false;
}
```

**风险**: 客户端刷新页面但 WebSocket 未断时，seq 从 0 开始，所有后续 snapshot 被拒绝。

**修复建议**:
- 方案 A: 检测 seq 回退超过阈值（如 >1000）时接受重置
- 方案 B: 使用 `(tsMs, seq)` 复合排序

#### 4.3 Room pressure override 发给所有人

**位置**: `src/RoomService.cpp`

```cpp
void RoomService::maybeSendRoomPressureOverrides(...) {
    for (auto& peerId : room->getPeerIds()) {  // 包括"肇事者"
        // ...发送 maxLevelClamp=1
    }
}
```

**风险**: 网络差的 peer 自己也收到降级指令，但可能已经在更低 level。

**修复建议**: 排除已经是 poor/lost 的 peer。

#### 4.4 Peer-level 和 Room-level override 叠加语义未定义

客户端可能同时收到:
- `server_auto_poor` → `maxLevelClamp=2`
- `server_room_pressure` → `maxLevelClamp=1`

**风险**: 客户端行为未定义。

**修复建议**: 协议文档明确叠加语义，建议取 `min`。

#### 4.5 TTL 无服务端强制执行

**位置**: `src/qos/QosTypes.h`

```cpp
struct QosOverride {
    uint32_t ttlMs{ 0u };  // 只告诉客户端，服务端不清理
};
```

**风险**: 客户端 bug 导致不清理过期 override 时，无法恢复。

**修复建议**: 服务端定期清理过期 `autoQosOverrideRecords_` 并发送 clear 通知。

---

### P2 - 性能问题

#### 4.6 clientStats 双重验证

**位置**: `src/SignalingServer.cpp` + `src/RoomService.cpp`

```cpp
// SignalingServer.cpp
auto qosParse = qos::QosValidator::ParseClientSnapshot(data);  // 第一次

// RoomService.cpp
auto parsed = qos::QosValidator::ParseClientSnapshot(stats);   // 第二次
```

**修复建议**: SignalingServer 解析后传递 `ClientQosSnapshot` 而不是 `json`。

#### 4.7 Room broadcast 无 rate limit

**位置**: `src/RoomService.cpp`

```cpp
void RoomService::setClientStats(...) {
    maybeBroadcastRoomQosState(roomId);  // 每次 clientStats 都触发
}
```

**风险**: 10 个 peer × 5 次/秒 = 50 次 room aggregate 计算/秒。

**修复建议**: 添加 rate limit，如每秒最多 1 次。

---

### P2 - 代码质量

#### 4.8 Magic numbers 散落各处

**位置**: `src/qos/QosOverride.cpp`

```cpp
MakeOverride("server_auto_poor", 10000u, 2u, ...);      // 10秒, level 2
MakeOverride("server_auto_lost", 15000u, ...);          // 15秒
MakeOverride("server_room_pressure", 8000u, 1u, ...);   // 8秒, level 1
```

**修复建议**: 提取到 `QosTypes.h` 作为常量。

#### 4.9 `getDefaultQosPolicy` 硬编码

**位置**: `src/RoomService.cpp`

无法 per-room 或 per-peer 配置。

**修复建议**: 从配置文件或 Room 属性读取。

---

## 五、测试覆盖评估 `[历史快照 — 见 uplink-qos-final-report.md 获取最新状态]`

> **注意**: 以下数据为初次 review 时的快照。后续已大幅补充服务端单测、集成测试、客户端 JS 单测、Node/browser harness 及 browser matrix。最新测试覆盖状态请参考 `docs/uplink-qos-final-report.md`。

### 5.1 现有测试（初次 review 时）

| 类型 | 数量 | 覆盖 |
|------|------|------|
| 单元测试 | 18 | Validator/Registry/Aggregator/Override |
| 集成测试 | 24 | clientStats/override/broadcast/policy |
| 场景测试 | 7 | qos_harness/scenarios/*.json |

### 5.2 缺失的测试场景

| 场景 | 风险等级 | 说明 |
|------|----------|------|
| seq 重置/回绕 | 高 | 客户端刷新后 QoS 失效 |
| peer+room override 叠加 | 高 | 客户端行为未验证 |
| 高频 clientStats | 中 | 性能问题 |
| TTL 过期清理 | 中 | 客户端永久降级 |
| peer leave 后 room aggregate | 中 | 残留 stale 数据 |
| cleanupRoomResources 状态清理 | 低 | 内存泄漏 |

### 5.3 建议补充的测试

```cpp
// 1. seq 重置场景
TEST_F(QosIntegrationTest, SeqResetAfterClientReconnect) {
    sendClientStats(alice, seq=100, quality="good");
    sendClientStats(alice, seq=1, quality="poor");  // 模拟刷新
    EXPECT_EQ(getStats(alice)["qos"]["seq"], 1);    // 应该接受
}

// 2. override 叠加场景
TEST_F(QosIntegrationTest, PeerAndRoomOverrideStacking) {
    // 触发 peer override (maxLevelClamp=2)
    // 触发 room override (maxLevelClamp=1)
    // 验证客户端收到两个 override，取 min
}

// 3. 高频上报性能
TEST_F(QosIntegrationTest, HighFrequencyClientStats) {
    for (int i = 0; i < 100; i++) {
        sendClientStats(alice, seq=i);
    }
    // 验证服务端不崩溃，响应时间合理
}
```

---

## 六、协议一致性

### 6.1 常量对比

| 常量 | 服务端 | 客户端 | 一致 |
|------|--------|--------|------|
| MAX_SEQ | 9007199254740991 | Number.MAX_SAFE_INTEGER | ✅ |
| MAX_TRACKS | 32 | 32 | ✅ |
| SAMPLE_INTERVAL | 1000ms | 1000ms | ✅ |
| SNAPSHOT_INTERVAL | 2000ms | 2000ms | ✅ |

### 6.2 Schema 版本

- `mediasoup.qos.client.v1` ✅
- `mediasoup.qos.policy.v1` ✅
- `mediasoup.qos.override.v1` ✅

### 6.3 枚举值一致性

所有枚举值（mode/quality/state/reason/source）客户端与服务端一致。

---

## 七、评分 `[历史快照 — 当前综合评分见第十二章]`

| 维度 | 分数 | 说明 |
|------|------|------|
| 架构设计 | 8/10 | 分层清晰，职责明确 |
| 代码实现 | 7/10 | 有 magic numbers，双重验证 |
| 测试覆盖 | 5/10 | 主路径有，边界场景缺失 |
| 安全性 | 4/10 | 有权限漏洞 |
| **综合** | **6/10** | |

---

## 八、修复建议优先级

### 合并前必须修复

1. **P0**: `setQosOverride` 添加权限控制
2. **P1**: seq 重置处理

### 合并后尽快修复

3. **P1**: Room pressure 排除 poor peer
4. **P1**: 定义 override 叠加语义
5. **P1**: TTL 服务端强制执行

### 后续迭代

6. **P2**: 消除 clientStats 双重验证
7. **P2**: Room broadcast rate limit
8. **P2**: 提取 magic numbers
9. **P2**: 补充边界测试

---

---

## 九、补充问题

### 8.1 客户端 override 过期清理是被动的 `[已失效]`

**位置**: `src/client/lib/qos/controller.js`

**原始问题**: `getActiveOverride()` 只在访问时清理过期 override，如果客户端停止采样（如页面后台），恢复前台后可能应用过期的 override。

**当前状态**: 已失效。当前实现在 `processSample()` 开头即调用 `getActiveOverride(nowMs)` 清理过期值（见 `controller.js:211`），在进入任何状态转换或规划逻辑之前完成清理。恢复前台后首次采样即会清除过期 override。此条不再是现状缺陷。

### 8.2 RoomQosAggregate 的 quality 计算有中间状态不一致

**位置**: `src/qos/QosRoomAggregator.cpp`

```cpp
for (const auto& peer : peers) {
    if (peer.quality == "poor") aggregate.poorPeers++;
    if (peer.quality == "lost" || peer.lost) aggregate.lostPeers++;  // lost 被计两次？
    quality = MinQuality(quality, peer.quality);
}
aggregate.quality = (aggregate.lostPeers > 0u) ? "lost" : quality;
```

**问题**: 如果 `peer.quality == "lost"`，它会同时增加 `lostPeers`，但不会增加 `poorPeers`。这是正确的，但如果 `peer.lost == true` 而 `peer.quality == "poor"`，会导致 `poorPeers++` 和 `lostPeers++` 都执行，计数语义不清晰。

**修复建议**: 使用 `else if` 明确互斥关系，或者文档说明 `lostPeers` 包含 `quality=="lost"` 和 `lost==true` 两种情况。

### 8.3 客户端状态机的 recovering 状态可能卡住

**位置**: `src/client/lib/qos/stateMachine.js`

```javascript
case 'recovering': {
    if (nextContext.consecutiveHealthySamples >= 5) {
        nextState = 'stable';
    } else if (nextContext.consecutiveCongestedSamples >= 2) {
        nextState = 'congested';
    }
    break;
}
```

**问题**: 如果网络一直在 warning 和 healthy 之间波动（不是 congested），recovering 状态会一直持续，无法回到 stable。

**修复建议**: 添加 recovering 状态的超时机制，如 30 秒后强制回到 early_warning。

### 8.4 QosRegistry 无线程安全保护

**位置**: `src/qos/QosRegistry.h`

```cpp
class QosRegistry {
    std::unordered_map<std::string, PeerMap> rooms_;  // 无 mutex
};
```

**问题**: 虽然当前 RoomService 是单线程调用，但如果未来有多线程访问（如 stats broadcast 和 clientStats 并发），会有数据竞争。

**修复建议**: 
- 方案 A: 文档明确 QosRegistry 只能单线程访问
- 方案 B: 添加 `std::shared_mutex` 保护

### 8.5 客户端 probe 失败后的 cooldown 可能过长 `[部分缓解]`

**位置**: `src/client/lib/qos/controller.js`

```javascript
if (probeEvaluation.result === 'failed') {
    await this.rollbackProbe(signals, nowMs, stateBefore);
    this.recoverySuppressedUntilMs = nowMs + this.profile.recoveryCooldownMs;  // 8秒
}
```

**原始问题**: 每次 probe 失败都重置 8 秒 cooldown，可能导致频繁 probe → 失败 → cooldown 循环。

**当前状态**: 部分缓解。controller 在 probe 进行中冻结升档（`recoveryProbeInProgress` → `noop`），减少了连续触发 probe 的频率。但 cooldown 仍为固定 8 秒，未使用指数退避。

**残留建议**: 使用指数退避（如 8s → 16s → 32s，最大 60s），作为后续算法调优项。

### 8.6 服务端 auto override 的 signature 去重可能误判

**位置**: `src/RoomService.cpp`

```cpp
const auto refreshInterval = static_cast<int64_t>(it->second.ttlMs) / 2;
if (it->second.signature == automatic->signature &&
    (nowMs - it->second.sentAtMs) < std::max<int64_t>(1000, refreshInterval)) {
    return;  // 跳过发送
}
```

**问题**: signature 是 JSON dump，如果字段顺序变化（虽然 nlohmann::json 是有序的），可能导致相同语义的 override 被认为不同。

**修复建议**: signature 应该只包含关键字段（reason, maxLevelClamp, forceAudioOnly, disableRecovery），而不是整个 JSON。

### 8.7 客户端没有处理 qosPolicy 的 profiles 字段

**位置**: `src/client/lib/qos/controller.js`

```javascript
handlePolicy(policy) {
    this.sampleIntervalMs = policy.sampleIntervalMs;
    this.snapshotIntervalMs = policy.snapshotIntervalMs;
    this.allowAudioOnly = policy.allowAudioOnly;
    this.allowVideoPause = policy.allowVideoPause;
    // policy.profiles 被忽略了
}
```

**原始问题**: 服务端发送的 `profiles.camera/screenShare/audio` 字段没有被使用，客户端无法动态切换 profile。

**当前状态**: 已修复。`handlePolicy()` 现在会按 `policy.profiles[source]` 读取目标 profile，并通过 `resolveProfileByName()` 切换到真实 profile。回归测试已验证切换后 `name / recoveryCooldownMs / thresholds` 等实际参数发生变化，而不是只改显示名。

### 8.8 缺少 QoS 指标的可观测性

**问题**: 服务端没有暴露 QoS 相关的 metrics：
- 每秒处理的 clientStats 数量
- 被拒绝的 clientStats 数量（虽然有 `rejectedClientStats_` 但没有暴露）
- 当前 poor/lost peer 数量
- auto override 触发次数

**修复建议**: 添加 Prometheus metrics 或在 `/stats` API 中暴露。

---

## 十、测试场景补充建议

### 9.1 应该添加的边界测试

```cpp
// seq 到达 MAX_SAFE_INTEGER
TEST_F(QosIntegrationTest, SeqAtMaxSafeInteger) {
    json report = makeClientReport();
    report["seq"] = 9007199254740991ULL;  // MAX_SAFE_INTEGER
    EXPECT_TRUE(alice.ws->request("clientStats", report).value("ok", false));
    
    report["seq"] = 9007199254740992ULL;  // MAX_SAFE_INTEGER + 1
    auto resp = alice.ws->request("clientStats", report);
    EXPECT_FALSE(resp.value("ok", true));  // 应该被拒绝
}

// 32 个 track 边界
TEST_F(QosIntegrationTest, MaxTracksPerSnapshot) {
    json report = makeClientReport();
    report["tracks"] = json::array();
    for (int i = 0; i < 32; i++) {
        report["tracks"].push_back(makeTrack("track-" + std::to_string(i)));
    }
    EXPECT_TRUE(alice.ws->request("clientStats", report).value("ok", false));
    
    report["tracks"].push_back(makeTrack("track-32"));  // 第 33 个
    auto resp = alice.ws->request("clientStats", report);
    EXPECT_FALSE(resp.value("ok", true));  // 应该被拒绝
}
```

### 9.2 应该添加的状态转换测试

```cpp
// poor → lost → good 的完整状态转换
TEST_F(QosIntegrationTest, QualityStateTransitions) {
    auto alice = joinRoom(testRoom_, "alice");
    
    // 1. good 状态
    sendClientStats(alice, seq=1, quality="good");
    auto override1 = alice.ws->waitNotification("qosOverride", 1000);
    EXPECT_TRUE(override1.empty());  // good 不应该触发 override
    
    // 2. poor 状态
    sendClientStats(alice, seq=2, quality="poor");
    auto override2 = alice.ws->waitNotification("qosOverride", 3000);
    EXPECT_EQ(override2["data"]["reason"], "server_auto_poor");
    
    // 3. lost 状态
    sendClientStats(alice, seq=3, quality="lost");
    auto override3 = alice.ws->waitNotification("qosOverride", 3000);
    EXPECT_EQ(override3["data"]["reason"], "server_auto_lost");
    EXPECT_TRUE(override3["data"]["forceAudioOnly"].get<bool>());
    
    // 4. 恢复到 good
    sendClientStats(alice, seq=4, quality="good");
    auto override4 = alice.ws->waitNotification("qosOverride", 3000);
    EXPECT_EQ(override4["data"]["reason"], "server_auto_clear");
    EXPECT_EQ(override4["data"]["ttlMs"], 0);
}
```

### 9.3 应该添加的并发测试

```cpp
// 多 peer 同时发送 clientStats
TEST_F(QosIntegrationTest, ConcurrentClientStats) {
    std::vector<TestClient> clients;
    for (int i = 0; i < 10; i++) {
        clients.push_back(joinRoom(testRoom_, "peer-" + std::to_string(i)));
    }
    
    // 并发发送
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 100; j++) {
                clients[i].ws->request("clientStats", makeClientReport(j));
            }
        });
    }
    for (auto& t : threads) t.join();
    
    // 验证服务端没有崩溃，状态一致
    for (int i = 0; i < 10; i++) {
        auto stats = getStats(clients[i]);
        EXPECT_TRUE(stats.contains("qos"));
    }
}
```

---

## 十一、Review 有效项清单（实施指南）

基于本轮验证和实际代码状态，将 review 意见重新分类：

### 必须先修（阻塞正式上线）

| ID | 问题 | 位置 | 说明 |
|----|------|------|------|
| **P0-1** | `setQosOverride` 无权限控制 | `src/RoomService.cpp` | 任意 peer 可对其他 peer 发送 forceAudioOnly，安全漏洞 |
| **P1-1** | seq 重置被拒绝 | `src/qos/QosRegistry.cpp` | 客户端刷新后 seq 从 0 开始，所有后续 snapshot 被 stale-seq 拒绝 |

### 可以边替换边修（不阻塞 demo 切换）

| ID | 问题 | 位置 | 说明 |
|----|------|------|------|
| **P1-3** | Room pressure 发给所有人 | `src/RoomService.cpp` | 应排除已 poor/lost 的 peer，或明确语义 |
| **P1-4** | maxLevelClamp 硬编码 | `src/qos/QosOverride.cpp` | 服务端不知道客户端 ladder 有几级，建议用语义化 clamp |
| **P2-1** | 降级节奏偏保守 | `src/client/lib/qos/stateMachine.js` | stable→congested 需要 4 秒，算法调优问题 |
| **P2-2** | 拥塞时单步 +1 level | `src/client/lib/qos/planner.js` | 严重拥塞下反应不够激进，影响体验 |
| **P2-3** | probe 恢复逻辑 | `src/client/lib/qos/probe.js` | 网络波动时可能频繁失败，第二阶段调优 |

### 已被本轮验证覆盖 / 可降级处理

| 原意见 | 当前状态 | 对应章节标注 |
|--------|----------|-------------|
| 测试覆盖 5/10 | **已过时** — 已补充服务端单测、集成测试、client JS 单测、Node/browser harness、browser matrix。见 `docs/uplink-qos-final-report.md` | 第五章标注 `[历史快照]` |
| audio-only 恢复 hack (§3.4) | **已缓解** — `getTransitionSignals()` 增加 `networkRecovered` 前置检查和 jitter 修正，回归测试已覆盖 | §3.4 标注 `[已缓解]` |
| `handlePolicy()` 未消费 profiles (§8.7) | **已修复** — `handlePolicy()` 已接入 `policy.profiles[source]`，并通过 `resolveProfileByName()` 切换真实 profile；回归测试已覆盖 | §8.7 更新为“已修复” |
| override clear / TTL clear 行为 | **已修复** — 客户端已支持按 reason 前缀清理自动 override，`server_ttl_expired` 可清空全部 active overrides，回归测试已覆盖 | controller override merge/clear 相关用例 |
| override 过期清理 (§8.1) | **已失效** — `processSample()` 开头即调用 `getActiveOverride()` 清理过期值 | §8.1 标注 `[已失效]` |
| probe flap 风险 (§3.3, §8.5) | **部分缓解** — probe 期间冻结升档，残留风险为阈值偏敏感和固定 cooldown | §3.3/§8.5 标注 `[部分缓解]` |
| clientStats 双重验证 | **降级** — 性能优化，不影响功能 | |
| Room broadcast rate limit | **降级** — 性能优化，不影响功能 | |
| Magic numbers | **降级** — 代码质量，不影响功能 | |
| QosRegistry 线程安全 | **降级** — 当前单线程调用，无实际风险 | |

---

## 十二、结论

**必须先修 2 项**：P0-1（权限控制）、P1-1（seq 重置）

**可以边替换边修 5 项**：Room pressure 语义、maxLevelClamp、降级节奏、拥塞跳跃、probe 逻辑

**已过时/降级/已缓解/已修复 10 项**：测试覆盖、audio-only hack（已缓解）、profiles 闭环（已修复）、override clear / TTL clear（已修复）、override 过期清理（已失效）、probe flap（部分缓解）、双重验证、rate limit、magic numbers、线程安全

**判断**：
- 这份 review 要认真对待
- 但不因为这份 review 就不能把 public 1v1 页面切到新前端 QoS 库
- P0/P1 里和权限、seq reset 相关的意见，要严肃处理
- 算法调优类意见作为切换后的下一阶段优化

**评分**：考虑到本轮已补充的测试和修正，综合评分从 6/10 调整为 **7/10**。
