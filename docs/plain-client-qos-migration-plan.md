# PlainTransport C++ 客户端 QoS 控制移植方案

> 将 `src/client/lib/qos/` 的 JS QoS 控制链路完整移植到 C++ 客户端。

## 1. 背景

当前 C++ 客户端（`client/main.cpp`）已实现：
- WebSocket 信令（join + plainPublish）
- MP4 读取 + H264 RTP 打包
- RTCP SR / NACK / PLI 处理

缺失的核心能力：**发送端自适应码率控制**。当网络变差时，客户端无法自动降级编码参数。

## 2. JS QoS 模块分析

### 2.1 模块依赖图

```
PublisherQosController (controller.js, 705行)
  ├── IntervalQosSampler (sampler.js, 88行)
  │     └── StatsProvider (外部注入: getSnapshot → RawSenderSnapshot)
  ├── deriveSignals (signals.js, 166行)
  │     └── computeEwma / computeLossRate / computeBitrateBps / classifyReason
  ├── evaluateStateTransition (stateMachine.js, 184行)
  │     └── isWarning / isCongested / isHealthy / isRecoveryHealthy
  ├── planActions (planner.js, 145行)
  │     └── resolveTargetLevel → stepToActions
  ├── QosActionExecutor (executor.js, 110行)
  │     └── ActionSink (外部注入: 执行编码参数调整)
  ├── beginProbe / evaluateProbe (probe.js, 70行)
  ├── QosProfile (profiles.js, 268行)
  │     └── DEFAULT_THRESHOLDS + camera/screenShare/audio ladder
  ├── serializeClientQosSnapshot (protocol.js, 351行)
  └── PeerQosCoordinator (coordinator.js, 97行)
        └── buildPeerQosDecision (多 track 优先级)
```

### 2.2 核心数据流

```
每 1s 采样:
  RTCP RR → {loss, rtt}
  本地计数 → {bytesSent, packetsSent}
       ↓
  deriveSignals()
  → {lossEwma, rttEwma, jitterEwma, bitrateUtilization, bandwidthLimited, cpuLimited}
       ↓
  evaluateStateTransition()
  → state: stable | early_warning | congested | recovering
       ↓
  planActions()
  → PlannedQosAction: setEncodingParameters / enterAudioOnly / pauseUpstream / noop
       ↓
  executor.execute()
  → 调整 ffmpeg 编码参数 (maxBitrate / maxFramerate / scaleResolutionDownBy)
       ↓
每 2s 上报:
  serializeClientQosSnapshot() → WebSocket → 服务端
```

### 2.3 状态机

```
                 healthy × N
  ┌──────────── recovering ◄──────────── congested
  │                 │                       ▲
  │            probe fail                   │
  │                 │              congested signal
  │                 ▼                       │
  │              stable ──── warning ───────┘
  │                ▲         signal
  │                │
  └────────────────┘
       probe success
```

- **stable**: loss < 1%, rtt < 150ms → 尝试恢复到更高质量
- **early_warning**: loss ≥ 4% 或 rtt ≥ 220ms → 保持当前 level
- **congested**: loss ≥ 8% 或 rtt ≥ 400ms 或 bandwidth limited → 降级
- **recovering**: 连续 healthy 后尝试升级（probe 机制）

### 2.4 Quality Ladder (camera profile, 5 级)

| Level | 描述 | maxBitrate | maxFramerate | scaleDown |
|-------|------|-----------|-------------|-----------|
| 0 | 最高质量 | 900kbps | 30fps | 1x |
| 1 | 中等质量 | 500kbps | 24fps | 1x |
| 2 | 低质量 | 250kbps | 15fps | 2x |
| 3 | 极低质量 | 100kbps | 10fps | 4x |
| 4 | 纯音频 | enterAudioOnly | - | - |

## 3. C++ 移植方案

### 3.1 文件结构

```
client/
  qos/
    QosTypes.h          ← 所有 enum/struct (types.d.ts)
    QosConstants.h      ← 常量 (constants.js)
    QosSignals.h        ← 信号计算 (signals.js)
    QosProfiles.h       ← Profile 定义和 ladder (profiles.js)
    QosStateMachine.h   ← 状态转换 (stateMachine.js)
    QosPlanner.h        ← 动作规划 (planner.js)
    QosProbe.h          ← 恢复探测 (probe.js)
    QosExecutor.h       ← 动作执行 (executor.js)
    QosProtocol.h       ← 上报序列化 (protocol.js)
    QosCoordinator.h    ← 多 track 协调 (coordinator.js)
    QosController.h     ← 主控制器 (controller.js)
  RtcpHandler.h         ← 已有: SR/NACK/PLI
  main.cpp              ← 集成
```

### 3.2 前置改动：实时编码模式

当前 `main.cpp` 用 `ffmpeg -c:v copy` 直接读 MP4 帧，无法动态调码率。需要改为：

```
MP4 → avformat 解封装 → avcodec 解码 → AVFrame
  → libx264 编码器 (可动态调参) → RTP 打包 → UDP
```

QoS 执行器通过修改 x264 编码器参数实现降级：
- `maxBitrate` → `avcodec_set_parameter("b", newBitrate)`
- `maxFramerate` → 跳帧（每 N 帧编码一帧）
- `scaleResolutionDownBy` → `sws_scale` 缩放

### 3.3 Stats 来源

JS 版本从 `RTCPeerConnection.getStats()` 获取 stats。C++ 客户端从以下来源获取：

| 信号 | 来源 |
|------|------|
| bytesSent / packetsSent | 本地 RtcpContext 计数 |
| packetsLost | RTCP RR 中的 cumulative lost |
| roundTripTimeMs | RTCP RR 中的 DLSR 计算 |
| jitterMs | RTCP RR 中的 interarrival jitter |
| targetBitrateBps | 当前编码器配置的目标码率 |
| qualityLimitationReason | 本地判断（编码器队列满=cpu，丢包高=bandwidth） |

### 3.4 分阶段实施计划

#### Phase 1: 类型 + 信号 + 状态机（纯逻辑，可单测）

文件: `QosTypes.h`, `QosConstants.h`, `QosSignals.h`, `QosProfiles.h`, `QosStateMachine.h`

- 移植所有 enum/struct 定义
- 移植 EWMA、loss rate、bitrate 计算
- 移植状态转换逻辑
- 移植 camera/screenShare/audio profile 定义

预计: ~500 行 C++

#### Phase 2: 规划 + 探测 + 执行（动作链路）

文件: `QosPlanner.h`, `QosProbe.h`, `QosExecutor.h`, `QosCoordinator.h`

- 移植 level 选择和 action 生成
- 移植 probe 生命周期
- 实现 C++ 版 ActionSink（对接 ffmpeg 编码器）
- 移植多 track 优先级协调

预计: ~400 行 C++

#### Phase 3: 控制器 + 协议上报

文件: `QosController.h`, `QosProtocol.h`

- 移植 PublisherQosController 主循环
- 移植 clientQosSnapshot 序列化
- 对接 WebSocket 上报

预计: ~500 行 C++

#### Phase 4: 实时编码 + 集成

文件: `main.cpp` 改造

- MP4 解码 → x264 实时编码
- RtcpContext 提取 RR stats → QosController 输入
- QosExecutor → 动态调整 x264 参数
- 定时采样 + 上报循环

预计: ~300 行 C++

#### Phase 5: 测试验证

- 单测: signals / stateMachine / planner 的输入输出对齐 JS 版本
- 集成测试: 用 `tc netem` 模拟丢包/延迟，验证自动降级/恢复
- 浏览器端验证: 观察视频质量随网络变化自动调整

## 4. 与 JS 版本的差异

| 项目 | JS 版本 | C++ 版本 |
|------|---------|---------|
| Stats 来源 | RTCPeerConnection.getStats() | RTCP RR + 本地计数 |
| 编码控制 | RTCRtpSender.setParameters() | ffmpeg libx264 参数 |
| 定时器 | setInterval | std::thread sleep 循环 |
| 上报通道 | mediasoup-client signaling | 直接 WebSocket JSON |
| qualityLimitationReason | 浏览器原生 | 本地推断 |
| CPU 检测 | 浏览器 qualityLimitationReason=cpu | 编码耗时 > 帧间隔 |

## 5. 风险和注意事项

1. **RTCP RR 精度**: Worker 发 RR 的频率可能不够高（默认 5s），需要确认或调整
2. **编码器动态调参**: x264 不是所有参数都能运行时修改，可能需要重建编码器
3. **RTT 计算**: 需要 SR 的 NTP timestamp 和 RR 的 LSR/DLSR 配合，当前 SR 已实现
4. **线程安全**: QoS 采样和编码在同一线程，不需要锁
5. **Profile 对齐**: C++ 版本的阈值和 ladder 必须和 JS 版本完全一致
