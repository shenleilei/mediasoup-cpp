# LiveKit vs mediasoup-cpp QoS 深度对比分析

> 基于源码分析：LiveKit `/root/livekit`，mediasoup-cpp `/root/mediasoup-cpp`
> 日期：2026-04-10

---

## 1. 架构层面的根本差异

| 维度 | mediasoup (本仓库) | LiveKit |
|------|---------------------|---------|
| 语言 | 媒体面 C++，控制面 C++ | 全 Go（pion/webrtc） |
| BWE 实现 | 直接嵌入 libwebrtc GoogCC | 自研 BWE（JitterPath 论文 + REMB 双模式） |
| 带宽分配 | `Transport::DistributeAvailableOutgoingBitrate` | 独立的 `StreamAllocator` 事件驱动引擎 |
| 质量评分 | worker 内部 0-10 分制 | 独立的 `ConnectionStats` + `qualityScorer`，E-model MOS |
| 拥塞状态 | 无显式状态，间接推断 | 三态（None / EarlyWarning / Congested） |
| Pacer | libwebrtc 内置 pacer | 可插拔 pacer（PassThrough / NoQueue / LeakyBucket） |
| Probing | libwebrtc 内置 probing | 自研 Prober，支持 padding 和 media 两种探测模式 |

mediasoup 的媒体面和控制面通过 pipe + FlatBuffers IPC 通信，QoS 核心逻辑在 worker 进程内部，控制面只能通过 IPC 间接影响。LiveKit 是单进程 Go 实现，QoS 逻辑和媒体转发在同一进程，可以做更紧密的联动。

---

## 2. 带宽估计（BWE）

### 2.1 mediasoup：依赖 libwebrtc GoogCC

```
TransportCongestionControlClient
  → 包装 GoogCcNetworkController
  → OnTargetTransferRate() 回调输出带宽
  → 内置 pacer 控制发送节奏
```

源码位置：`src/worker/src/RTC/TransportCongestionControlClient.cpp`（583 行）

可调参数有限：
- `initialAvailableBitrate`: 600000 bps
- `TransportCongestionControlMinOutgoingBitrate`: 30000 bps
- `MaxBitrateIncrementFactor`: 1.35
- `MaxPaddingBitrateFactor`: 0.85
- `MaxBitrateMarginFactor`: 0.1（带宽变化 < 10% 时不通知）
- `AvailableBitrateEventInterval`: 1000ms

优点：GoogCC 成熟，经过大规模验证。
缺点：黑盒，无法干预内部决策，不能针对 SFU 场景优化。

### 2.2 LiveKit：双模式自研 BWE

LiveKit 实现了两套完全独立的 BWE：

**模式一：RemoteBWE（基于 REMB）**

源码位置：`pkg/sfu/bwe/remotebwe/remote_bwe.go`（359 行）

```
REMB 估计值 → ChannelObserver → TrendDetector（趋势检测）
                              → NackTracker（NACK 比率监控）
                              → 拥塞状态机
```

- `ChannelObserver` 用 `TrendDetector` 跟踪 REMB 估计值趋势（需要 12 个样本，下降阈值 -0.6）
- `NackTracker` 监控重复 NACK 比率（2-3 秒窗口，阈值 0.08）
- 拥塞检测是趋势 + NACK 双信号融合
- `NackRatioAttenuator`: 0.4（NACK 导致的拥塞，按 NACK 比率衰减带宽）
- `ExpectedUsageThreshold`: 0.95（只有估计值低于期望使用量的 95% 才认为拥塞）

**模式二：SendSideBWE（基于 TWCC，JitterPath 论文）**

源码位置：`pkg/sfu/bwe/sendsidebwe/`（congestion_detector.go 1159 行 + send_side_bwe.go 等）

```
TWCC Feedback → 解析包到达时间
  → 分组（packetGroup）
  → 计算 propagated queuing delay
  → JQR/DQR 区域判定
  → 拥塞状态转换
```

- 基于 JitterPath 论文（https://homepage.iis.sinica.edu.tw/papers/lcs/2114-F.pdf）
- 将包分组，计算组级别的排队延迟
- JQR（Join Queuing Region）= 拥塞区域，DQR（Disjoint Queuing Region）= 非拥塞区域
- 有滞后区间（Indeterminate），避免频繁切换
- 区分排队延迟拥塞和丢包拥塞两种原因
- 拥塞信号配置（JQR 需要连续 4-6 组，持续 400-600ms）

关键差距：LiveKit 的 BWE 完全可控、可调参、可观测。mediasoup 的 GoogCC 是黑盒。

---

## 3. 带宽分配 —— 差距最大的模块

### 3.1 mediasoup：简单的优先级迭代

源码位置：`src/worker/src/RTC/Transport.cpp`（DistributeAvailableOutgoingBitrate，约 85 行）

```cpp
// 算法：
// 1. 收集所有 Consumer 及其 priority
// 2. availableBitrate = tccClient->GetAvailableBitrate()
// 3. 第一轮（base allocation）：
//    - 从高优先级到低优先级遍历
//    - 每个 Consumer 调用 IncreaseLayer()，每次只升一层
//    - 扣减 usedBitrate
// 4. 后续轮：高优先级 Consumer 可以多升几层（priority 次）
// 5. 循环直到带宽用完或没有 Consumer 需要更多
// 6. 所有 Consumer 调用 ApplyLayers()
```

局限：
- `DistributeAvailableOutgoingBitrate()` 本身不自动编排 pause/resume/probing
- 控制面暴露了 `consumer.pause()` / `consumer.resume()` / `setPreferredLayers()` API（见 `src/Consumer.cpp:9`），但分配算法不会自动调用它们
- worker 启用了 libwebrtc 的 periodic ALR probing（见 `TransportCongestionControlClient.cpp:70-73`），但不可配置、不可观测
- 没有 deficient 状态追踪
- 没有 cooperative transition（track 间协商让出带宽）
- 没有 overshoot 控制策略

### 3.2 LiveKit：完整的 StreamAllocator 引擎

源码位置：`pkg/sfu/streamallocator/streamallocator.go`（1459 行）

这是 LiveKit QoS 的核心，mediasoup 完全没有对应物。

**状态机：**
```
Stable ←→ Deficient
```
- Stable：所有 track 都在期望层级，正常分配
- Deficient：至少一个 track 低于期望层级，触发 probing 和 boost

**事件驱动架构（13 种信号）：**
- `AllocateTrack` / `AllocateAllTracks`：单 track 或全量分配
- `Estimate`：BWE 估计值更新
- `Feedback`：TWCC 反馈
- `CongestionStateChange`：拥塞状态变化
- `ProbeClusterSwitch` / `SendProbe` / `ClusterComplete`：探测生命周期
- `Resume`：track 恢复
- `SetAllowPause` / `SetChannelCapacity`：外部控制
- `PeriodicPing`：定时检查
- `AdjustState`：状态调整

**核心分配算法 `allocateAllTracks()`：**
```
1. 先分配 exempt track（屏幕共享等），给 optimal allocation
2. 计算剩余 availableChannelCapacity
3. 如果剩余为 0 且允许暂停，暂停所有 managed track
4. 否则，按优先级排序 managed track
5. 从 spatial=0,temporal=0 开始，逐层给每个 track 尝试 ProvisionalAllocate
6. 每个 track 返回是否可行 + 消耗带宽
7. 带宽不够时停止分配
8. 最后 ProvisionalAllocateCommit 应用结果
```

**单 track 分配 `allocateTrack()`：**
```
1. 如果 Stable 且无拥塞，直接 AllocateOptimal
2. 如果 Deficient：
   a. 先尝试 cooperative transition（track 主动让出带宽）
   b. 如果是降级（让出带宽），应用后 boost 其他 deficient track
   c. 如果需要更多带宽：
      - 先用 available headroom 尝试分配
      - headroom 不够，从 "距离期望最近" 的 track 借带宽
      - 借够了就 commit，借不够且允许暂停就暂停
```

**Boost 机制 `maybeBoostDeficientTracks()`：**
```
1. 计算 available headroom
2. 按 "距离期望最远" 排序 deficient track
3. 逐个尝试 AllocateNextHigher
4. 每次提升后重新排序（因为最远的可能变了）
5. 直到 headroom 用完或所有 track 都满足
```

**Overshoot 控制：**
不同场景有不同的 overshoot 策略：
- Optimal 状态：允许 overshoot
- Deficient 状态：不允许
- Exempt track（Deficient 时）：允许
- Probe 中：允许
- Catchup（boost）：不允许
- Boost：允许

---

## 4. 拥塞状态管理

### 4.1 mediasoup：无显式拥塞状态

GoogCC 内部有拥塞检测，但 worker 不对外暴露显式的 congestion state。控制面拿不到 worker 内部的拥塞状态，只能根据 `availableBitrate`、`loss`、`score`、`jitter` 等统计间接推断（见 `src/Transport.cpp:369`、`src/Producer.cpp:121`、`src/RoomService.cpp:575`）。

### 4.2 LiveKit：拥塞状态枚举三态，实际使用因 BWE 路径而异

拥塞状态枚举定义了三态，但 RemoteBWE 当前实现只使用 None 和 Congested 两态；EarlyWarning 仅在 SendSideBWE 的 congestionDetector 中使用。

**SendSideBWE（三态）**（见 `congestion_detector.go:951`）：
```
None → EarlyWarning → Congested
                    ↘ None (如果 DQR 恢复)
```

**RemoteBWE（二态）**（见 `remote_bwe.go:167`）：
```
None ↔ Congested
```

源码位置：`pkg/sfu/bwe/bwe.go`

```go
type CongestionState int
const (
    CongestionStateNone CongestionState = iota
    CongestionStateEarlyWarning
    CongestionStateCongested
)
```

行为：
- `None`：正常，StreamAllocator 在 Stable 状态
- `EarlyWarning`：检测到拥塞趋势但还不严重，hold 住当前分配不降级
- `Congested`：确认拥塞，降低 committedChannelCapacity，触发 allocateAllTracks

拥塞原因区分：
- `QueuingDelay`：排队延迟增加（SendSideBWE 的 JQR 检测）
- `Loss`：NACK 比率超阈值（RemoteBWE 的 NackTracker）

容量估算因 BWE 路径不同而不同：

**RemoteBWE 容量估算**（见 `remote_bwe.go:197`）：
- loss 场景：`expectedUsage × (1 - nackRatioAttenuator × nackRatio)`
- 非 loss 场景（estimate 下降趋势）：提交 `lastReceivedEstimate`（即 REMB 值）

**SendSideBWE 容量估算**（见 `congestion_detector.go:1068`）：
- 从 TWCC packet groups 聚合 acknowledged bitrate 作为 `estimatedAvailableChannelCapacity`
- 排队延迟拥塞时取 contributing groups 的 acknowledged bitrate
- 丢包拥塞时取 loss measurement 范围内 groups 的 acknowledged bitrate
- 不使用 REMB 值

---

## 5. 质量评分

### 5.1 mediasoup：`RtpStreamRecv::UpdateScore()`

源码位置：`src/worker/src/RTC/RtpStreamRecv.cpp`（约 116 行）

```
lost = expected - received
repairedRatio = repaired / received
repairedWeight = (1 / (repairedRatio + 1))^4 × (repaired / retransmitted)
effectiveLost = lost - repaired × repairedWeight
deliveredRatio = (received - effectiveLost) / received
score = round(deliveredRatio^4 × 10)
```

- 0-10 整数分
- 只考虑丢包和 NACK 修复效率
- 不考虑 RTT、jitter、码率匹配度
- 不区分编解码器
- `deliveredRatio^4` 使得评分对丢包敏感（5% 丢包 ≈ 8 分，10% ≈ 6.5 分）

### 5.2 LiveKit：E-model + 多维评分

源码位置：`pkg/sfu/connectionquality/scorer.go`（641 行）+ `connectionstats.go`（519 行）

**Packet Score（基于 E-model 简化版）：**
```
effectiveDelay = RTT/2 + jitter×2
delayEffect = effectiveDelay / 40  (或 (effectiveDelay-120)/10 当 > 160ms)
lossEffect = packetLossRate × codecWeight
packetScore = 100 - delayEffect - lossEffect
```

**Bitrate Score：**
```
bitrateScore = 100 - 20 × log(expectedBits / actualBits)
```
- 实际码率 vs 期望码率的对数比
- GOOD 拐点约 2.7x，POOR 拐点约 20.1x

**Layer Distance Penalty：**
```
每差一个 spatial layer 扣 35 分（cDistanceWeight = 35.0）
```

**编解码器感知权重：**
| 编解码器 | 丢包权重 | 无 FEC | 有 FEC |
|---------|---------|--------|--------|
| Opus | 8.0 | 2.5% → GOOD | 3.75% → GOOD |
| RED | 4.0 | 5% → GOOD | 7.5% → GOOD |
| Video | 10.0 | 2% → GOOD | - |

**评分平滑：**
```
increaseFactor = 0.4  // 恢复慢（保守）
decreaseFactor = 0.8  // 下降快（敏感）
```

**质量等级映射：**
```
score >= 80 → EXCELLENT
score >= 40 → GOOD
score >= 20 → POOR
score < 20  → LOST
```

**状态感知：**
- mute 时重置为满分（除非已经 LOST）
- pause 时设为最低分
- layerMute 时重置码率和层距离聚合器

**参与者级别聚合：**
```go
// participant.go
func GetConnectionQuality() {
    minQuality = EXCELLENT
    for each published track:
        quality = track.GetConnectionScoreAndQuality()
        minQuality = min(minQuality, quality)
    for each subscribed track:
        quality = downTrack.GetConnectionScoreAndQuality()
        minQuality = min(minQuality, quality)
}
```

---

## 6. Probing（带宽探测）

### 6.1 mediasoup：libwebrtc ALR probing（黑盒）

worker 启用了 libwebrtc 的 `EnablePeriodicAlrProbing(true)`（见 `TransportCongestionControlClient.cpp:70-73`），在应用发送码率低于估计带宽时自动发送探测包。但探测策略不可配置、不可观测，控制面无法主动触发或控制探测。

### 6.2 LiveKit：自研 Prober + ProbeController

源码位置：`pkg/sfu/ccutils/prober.go`（596 行）

**两种探测模式：**
- `ProbeModePadding`：发送 padding-only RTP 包（默认）
- `ProbeModeMedia`：恢复暂停的流作为探测

**配置：**
```yaml
probe_overage_pct: 120  # 探测目标 = 期望带宽 × 120%
probe_min_bps: 200000   # 最低探测速率 200kbps
```

**探测生命周期：**
```
1. StreamAllocator 检测到 Deficient 状态
2. 检查 BWE.CanProbe()（非拥塞 + ProbeController 允许）
3. Prober 创建 probe cluster
4. Pacer 开始发送探测包
5. BWE 监控探测期间的拥塞信号
6. Pacer 报告 cluster 完成
7. BWE.ProbeClusterFinalize() 返回三态信号：
   - NotCongesting → 提升 committedChannelCapacity → boost deficient tracks
   - Congesting → 保持当前容量
   - Inconclusive → 保持当前容量
```

**探测期间的保护：**
- 冻结 channel observer，防止探测流量干扰正常拥塞检测
- 如果探测期间检测到拥塞，标记 `activeProbeCongesting`，不更新容量
- 探测结束后恢复正常观测

**ProbeSignal 评估（SendSideBWE）：**
```
propagatedQueuingDelay > 50ms 或 weightedLoss > 0.25 → Congesting
propagatedQueuingDelay < 20ms 且 weightedLoss < 0.1  → NotCongesting
其他 → Inconclusive
```

---

## 7. Pacer

### 7.1 mediasoup：libwebrtc 内置

`RtpTransportControllerSend` 内置 pacer，控制发送节奏。不可替换。

### 7.2 LiveKit：可插拔 pacer

源码位置：`pkg/sfu/pacer/`

三种实现：
- `PassThrough`：直接发送，无排队
- `NoQueue`：不排队但有速率控制
- `LeakyBucket`：漏桶算法，平滑发送

Pacer 同时负责 probe cluster 的观测（`PacerProbeObserver`），在探测包发送完成后通知 StreamAllocator。

---

## 8. 控制面 QoS 对比

### 8.1 mediasoup-cpp 控制面

源码位置：`src/RoomService.cpp`

```
broadcastStats() → collectPeerStats() → 遍历 peer 的 transport/producer/consumer
  → 通过 Channel IPC 调 worker getStats（有超时保护）
  → 合并 clientStats（客户端上报）
  → 广播 statsReport 给房间内所有 peer
  → 写入 recorder QoS 文件
```

特点：
- 定时采集（timer 驱动）
- 有 per-peer 预算（2 秒）防止阻塞
- 支持客户端 stats 上报和合并（见 `SignalingServer.cpp:492`、`RoomService.cpp:569`）
- 这是**监控/上报层**，不是**控制层**

### 8.2 LiveKit 控制面

- `ConnectionStats` 每 5 秒更新一次评分
- 评分通过 `OnStatsUpdate` 回调上报到 telemetry
- `GetConnectionQuality()` 聚合参与者级别质量
- 质量变化通过信令推送给客户端
- StreamAllocator 直接根据 BWE 和拥塞状态做分配决策
- **没有客户端主动上报 stats 的信令入口**，QoS 数据全部来自服务端侧 RTCP/TWCC 采集链路（`ConnectionStats` → `OnStatsUpdate` → telemetry）

---

## 9. 差距总结

| 能力 | mediasoup-cpp | LiveKit | 差距程度 |
|------|:---:|:---:|------|
| BWE 算法 | ✅ GoogCC（黑盒） | ✅ 自研双模式（可控） | 中 |
| 带宽分配引擎 | ⚠️ 简单优先级迭代 | ✅ 完整 StreamAllocator | **最大** |
| Track 暂停/恢复 | ⚠️ API 存在但分配器不自动编排 | ✅ StreamAllocator 自动编排 | 大 |
| Cooperative Transition | ❌ | ✅ | 大 |
| 拥塞状态机 | ❌ 无显式状态 | ✅ 三态枚举（SendSideBWE 三态，RemoteBWE 二态） | 大 |
| EarlyWarning Hold | ❌ | ✅ 仅 SendSideBWE 路径 | 中 |
| 主动 Probing | ⚠️ libwebrtc ALR probing（黑盒） | ✅ 可控双模式 | 大 |
| 质量评分 | ⚠️ 丢包^4 简单公式 | ✅ E-model + 码率 + 层距离 | 中 |
| 编解码器感知评分 | ❌ | ✅ Opus/RED/Video 不同权重 | 中 |
| FEC 感知评分 | ❌ | ✅ | 小 |
| Pacer | ⚠️ libwebrtc 内置 | ✅ 可插拔 | 小 |
| 上下行联动 | ❌ | ⚠️ 有基础 | 都不够 |
| 控制面 QoS 监控 | ✅ 完整 | ✅ 完整 | 持平 |
| 客户端 stats 集成 | ✅ 显式 clientStats 信令入口 | ❌ 无对等入口，服务端自采集 | mediasoup-cpp 领先 |

---

## 10. 建议优先级

### P0：StreamAllocator（最大差距，可在控制面实现）

在 mediasoup-cpp 控制面实现带宽分配引擎：
- 维护每个 subscriber transport 的 channel capacity（从 worker stats 中获取）
- 实现 Stable / Deficient 状态机
- 支持 track 暂停/恢复（通过 `consumer.pause()` / `consumer.resume()` IPC）
- 支持层级调整（通过 `consumer.setPreferredLayers()` IPC）
- 按优先级逐层分配，带宽不够时暂停低优先级 track
- 有多余带宽时 boost deficient track

不需要改 worker，完全在控制面通过现有 IPC 接口实现。

### P1：拥塞状态机

在控制面维护三态拥塞模型：
- 利用 worker 上报的 score 变化趋势
- 利用 transport stats 中的丢包率、RTT 趋势
- EarlyWarning 时 hold 住当前分配
- Congested 时触发重新分配

### P2：质量评分增强

在控制面实现 E-model 评分：
- 综合 RTT、jitter、丢包、码率匹配度
- 编解码器感知权重
- 层距离惩罚
- 替代 worker 的简单 0-10 score

### P3：Probing

优先级较低，实现方式：
- 在控制面通过恢复暂停的 consumer 作为 media probe
- 或通过创建 padding producer 发送探测包
- 监控探测期间的 stats 变化判断结果

---

## 附录：关键源码位置

### mediasoup worker QoS 核心文件

| 文件 | 行数 | 说明 |
|------|------|------|
| `src/worker/src/RTC/Transport.cpp` | 3094 | 带宽分配、RTCP 处理 |
| `src/worker/src/RTC/SimulcastConsumer.cpp` | 1744 | Simulcast 层切换 |
| `src/worker/src/RTC/RtpStreamRecv.cpp` | 947 | 质量评分、NACK 触发 |
| `src/worker/src/RTC/TransportCongestionControlClient.cpp` | 583 | GoogCC 包装 |
| `src/worker/src/RTC/TransportCongestionControlServer.cpp` | 483 | TWCC Feedback 生成 |
| `src/worker/src/RTC/NackGenerator.cpp` | 379 | NACK 丢包检测 |

### LiveKit QoS 核心文件

| 文件 | 行数 | 说明 |
|------|------|------|
| `pkg/sfu/forwarder.go` | 2358 | 层选择、provisional allocation |
| `pkg/sfu/downtrack.go` | 2665 | 下行 track 管理 |
| `pkg/sfu/streamallocator/streamallocator.go` | 1459 | 带宽分配引擎 |
| `pkg/sfu/bwe/sendsidebwe/congestion_detector.go` | 1159 | 自研拥塞检测 |
| `pkg/sfu/connectionquality/scorer.go` | 641 | E-model 质量评分 |
| `pkg/sfu/ccutils/prober.go` | 596 | 带宽探测 |
| `pkg/sfu/connectionquality/connectionstats.go` | 519 | 连接质量统计 |
| `pkg/sfu/bwe/remotebwe/remote_bwe.go` | 359 | REMB BWE |

### mediasoup-cpp 控制面 QoS 文件

| 文件 | 说明 |
|------|------|
| `src/RoomService.cpp` | broadcastStats、collectPeerStats、setClientStats |
| `src/Recorder.h` | QoS timeline 写入 |
| `src/SignalingServer.cpp` | stats timer、clientStats 接收 |
