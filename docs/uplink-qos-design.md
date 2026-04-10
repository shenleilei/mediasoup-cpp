# 上行 QoS 最终设计（端上快环 + 服务端慢环）

> 范围：publisher 上行弱网 QoS
> 当前明确不做：`dynacast` / `subscriber-driven uplink`
> 适用仓库：`/root/mediasoup-cpp`
> 日期：2026-04-10
> 配套实施清单：`docs/uplink-qos-implementation-plan.md`
> Phase 1/2 逐文件拆解：`docs/uplink-qos-phase1-phase2-breakdown.md`
> 当前进展：`docs/uplink-qos-progress.md`
> 测试方案：`docs/uplink-qos-test-plan.md`

---

## 1. 设计结论

本项目的上行 QoS 采用以下最终形态：

- **端上快环**：由 `src/client` 内的新 QoS 子系统负责 1 秒级观测、判定和动作执行。
- **服务端慢环**：由控制面负责策略下发、状态聚合、诊断、录制和兜底限制，不参与 1 秒级微调。
- **统一协议**：保留现有 `clientStats` 信令入口，但将其升级为版本化的 QoS 快照协议。
- **不改 worker 作为 P0 前提**：第一阶段不要求修改 mediasoup worker；QoS 主体在 `src/client` 和控制面完成。
- **测试先行**：QoS 控制器从第一天就必须按“可回放、可注入、可重放”的方式设计，测试不是补充项，而是架构约束。

一句话概括：

```text
Producer / Track / RTCStats
  -> 端上 QoS Controller 快速闭环
  -> clientStats QoS Snapshot
  -> 服务端聚合 / 诊断 / override
  -> 端上执行约束
```

---

## 2. 目标与非目标

### 2.1 目标

本设计要解决的核心问题是：

- 上行弱网时，发布端能够在 1-2 秒内做出稳定、可解释、可恢复的降级动作。
- 摄像头、屏幕共享、音频三类 source 使用不同策略，不再用“一套阈值打天下”。
- 同一 peer 有多路本地轨道时，动作必须协同，优先保音频，避免各轨道独立决策导致冲突。
- 服务端能够看见端上的 QoS 状态、动作和原因，支持排障、录制、监控和人工兜底。
- 最终方案从一开始就支持高质量 UT、双端 contract test、双端协同集成测试和真实弱网 E2E。

### 2.2 非目标

本阶段明确不做以下内容：

- 不做 `dynacast`
- 不做基于订阅需求的上行层裁剪
- 不重写浏览器 / libwebrtc 的底层 BWE
- 不把服务端做成上行的 fast loop 控制器
- 不在第一阶段强依赖 worker 增加新的媒体面控制接口

这意味着第一阶段的基本判断是：

- **端上负责快速执行**
- **服务端负责统一约束和观测**
- **WebRTC 自带 BWE 继续作为底层基础**

---

## 3. 现状与差距

当前 `src/client` 已具备若干关键原语，但缺少 QoS 策略层。

现有能力：

- `Producer.getStats()`
- `Producer.pause()` / `resume()`
- `Producer.replaceTrack()`
- `Producer.setMaxSpatialLayer()`
- `Producer.setRtpEncodingParameters()`
- `disableTrackOnPause`
- `zeroRtpOnPause`

这些能力已经足够承载上行 QoS 的动作层，但当前缺少：

- 周期采样与平滑
- 网络 / CPU 原因识别
- 状态机与滞后机制
- source profile
- 多轨道协同
- QoS trace
- 版本化 QoS 协议
- 面向 QoS 的双端测试体系

因此，正确方向不是继续往 `Producer` / `Transport` 里堆 if/else，而是增加一层独立的 QoS 子系统。

---

## 4. 设计原则

### 4.1 机制与策略分离

`Producer`、`Transport`、`Handler` 继续做机制层，不承担 QoS 策略。

- 机制层负责“能做什么”
- QoS 子系统负责“什么时候做、为什么做、做多少”

### 4.2 快环在端上，慢环在服务端

端上 fast loop 必须满足：

- 不依赖服务端 RTT
- 不依赖控制面定时器
- 能在 1-2 秒内完成一次“观测 -> 判定 -> 动作”

服务端 slow loop 必须满足：

- 统一看见全局状态
- 能下发 policy / override
- 能给监控和录制提供一致的 timeline
- 不承担 1 秒级微调

### 4.3 可测试性优先

QoS 控制器从设计上必须满足：

- 不直接依赖 `Date.now()`
- 不直接依赖裸 `setInterval()`
- 不直接依赖裸 WebSocket
- 不把 `RTCStatsReport` 解析逻辑和状态机逻辑混在一起
- 每次采样、状态变更、动作执行都生成结构化 trace

否则后续无法做好 trace replay、回归测试和阈值调优。

### 4.4 多 source 策略分离

必须按 source 建模：

- `camera`
- `screenShare`
- `audio`

它们的目标完全不同：

- 摄像头优先保连续性和可懂度
- 屏幕共享优先保清晰度和文字可读性
- 音频优先保连通和语音可懂度

---

## 5. 总体架构

```text
┌──────────────────────── Client ────────────────────────┐
│                                                        │
│ MediaStreamTrack -> Producer -> ProducerAdapter        │
│                                │                       │
│                                ▼                       │
│                      PublisherQosController            │
│                  ┌──────────┬──────────┬──────────┐    │
│                  │ Sampler  │ State    │ Planner  │    │
│                  │          │ Machine  │ Executor │    │
│                  └──────────┴──────────┴──────────┘    │
│                           │         │                  │
│                           │         └-> QoS Trace      │
│                           │                            │
│                           └-> QosSnapshotSender        │
│                                     │                  │
└─────────────────────────────────────┼──────────────────┘
                                      │ clientStats(qos)
                                      ▼
┌────────────────────── Server / Control Plane ──────────────────────┐
│                                                                    │
│ SignalingServer -> QosSnapshotValidator -> Room Qos Registry       │
│                                               │                    │
│                                               ├-> stats/getStats   │
│                                               ├-> recorder/timeline│
│                                               ├-> monitoring       │
│                                               └-> QosOverride      │
│                                                        │           │
└────────────────────────────────────────────────────────┼───────────┘
                                                         │
                                                         ▼
                                             client receives override
                                             and clamps local actions
```

### 5.1 快环与慢环边界

| 维度 | 端上快环 | 服务端慢环 |
|------|---------|-----------|
| 周期 | 1 秒级 | 2-10 秒级 |
| 输入 | `getStats()`、track/source、本地动作结果 | `clientStats`、worker stats、session/room 状态 |
| 输出 | 降码率、降 FPS、降分辨率、降 layer、audio-only | policy、override、聚合诊断、timeline |
| 延迟要求 | 极低 | 中等 |
| 失败影响 | 直接影响发送体验 | 影响排障、监控、兜底 |

### 5.2 关键决策

- 端上用**显式状态机**，不做“指标一变就立刻下动作”的反射式逻辑。
- 服务端不做“替端上决定每一步怎么降”的 fast loop，只做 policy / override。
- 客户端 QoS 快照是**协议化对象**，不是随手塞进 `clientStats` 的自由 JSON。
- 多轨道协同由端上 `PeerQosCoordinator` 统一处理，不允许音视频各自独立抢占。

---

## 6. 端上 QoS 设计

### 6.1 模块划分

建议在 `src/client/src/qos/` 下新建独立子系统：

```text
src/client/src/qos/
  index.ts
  types.ts
  constants.ts
  profiles.ts
  protocol.ts
  trace.ts
  clock.ts
  sampler.ts
  signals.ts
  stateMachine.ts
  planner.ts
  executor.ts
  controller.ts
  coordinator.ts
  adapters/
    producerAdapter.ts
    signalChannel.ts
    statsProvider.ts
```

职责划分：

- `sampler.ts`
  - 周期采样
  - 负责调用 stats provider
- `signals.ts`
  - 从原始 stats 提取稳定信号
  - 做 EWMA、delta、趋势计算
- `stateMachine.ts`
  - 只负责状态迁移
  - 不关心具体动作
- `planner.ts`
  - 根据状态和 source profile 决定动作
- `executor.ts`
  - 将动作映射到 `Producer` 原语
- `controller.ts`
  - 串起一次完整循环
- `coordinator.ts`
  - 同一 peer 下多轨道协同
- `protocol.ts`
  - QoS snapshot / policy / override 的 schema 和编码逻辑
- `trace.ts`
  - 为测试和排障输出结构化 trace

### 6.2 测试友好的接口约束

控制器必须依赖抽象接口，而不是直接依赖浏览器对象：

```ts
interface QosClock {
  nowMs(): number;
  setInterval(cb: () => void, intervalMs: number): unknown;
  clearInterval(id: unknown): void;
}

interface QosStatsProvider {
  getSnapshot(): Promise<RawSenderSnapshot>;
}

interface QosActionSink {
  apply(action: PlannedQosAction): Promise<QosActionResult>;
}

interface QosSignalChannel {
  publishSnapshot(snapshot: ClientQosSnapshot): Promise<void>;
  onPolicy(cb: (policy: QosPolicy) => void): void;
  onOverride(cb: (override: QosOverride) => void): void;
}
```

**禁止**让控制器直接：

- 直接 `setInterval`
- 直接 `Date.now`
- 直接 `producer.getStats()`
- 直接调用 WebSocket

这样会直接毁掉 UT 和 trace replay。

### 6.3 ProducerAdapter

QoS 控制器不能直接吃 `Producer`，必须通过一个 adapter 隔离动作细节：

职责：

- `getStats()`
- `setEncodingParameters()`
- `setMaxSpatialLayer()`
- `pauseVideoUpstream()`
- `resumeVideoUpstream()`
- `getCurrentEncodingState()`

原因：

- 单测时可以 fake adapter
- 以后即使换浏览器策略或 track 组合，也不会影响状态机本体
- `FakeHandler` 目前的 `getSenderStats()` 返回空 `Map`，不适合作为 QoS 主测试底座；QoS 测试应主要依赖 adapter / provider 注入，而不是继续扩展 `FakeHandler`

### 6.4 采样周期与上报周期

默认策略如下：

| 项目 | camera | screenShare | audio |
|------|--------|-------------|-------|
| 本地采样间隔 | 1000ms | 1000ms | 1000ms |
| 本地最小动作间隔 | 1500ms | 1500ms | 2000ms |
| 本地恢复冷却 | 8000ms | 10000ms | 8000ms |
| QoS 快照上报 | 2000ms | 2000ms | 2000ms |
| 状态变更即时上报 | 是 | 是 | 是 |

选择理由：

- 1 秒采样足够快，且对浏览器 stats 抖动容忍度较高
- 快照不需要每秒都上报，2 秒 + 事件上报即可
- 恢复必须明显慢于降级，避免反复震荡

### 6.5 信号提取

原始 stats 不直接进入状态机，必须先做提取和归一化。

统一信号：

- `sendBitrateBps`
- `targetBitrateBps`
- `packetsSentDelta`
- `packetsLostDelta`
- `lossRate`
- `rttMs`
- `jitterMs`
- `frameWidth`
- `frameHeight`
- `framesPerSecond`
- `qualityLimitationReason`
- `qualityLimitationDurations`
- `retransmittedPacketsSentDelta`

派生信号：

- `bitrateUtilization = sendBitrateBps / max(targetBitrateBps, configuredBitrateBps)`
- `lossEwma`
- `rttEwma`
- `jitterEwma`
- `bandwidthLimited = qualityLimitationReason == "bandwidth"`
- `cpuLimited = qualityLimitationReason == "cpu"`
- `fpsDropRatio`
- `resolutionDropRatio`

推荐平滑：

- `EWMA alpha = 0.35`
- 至少保留最近 6 个样本

### 6.6 原因分类

QoS 必须先判定“为什么差”，再决定“怎么降”。

标准原因枚举：

- `network`
- `cpu`
- `manual`
- `server_override`
- `unknown`

分类规则：

- 连续 2 个样本 `qualityLimitationReason == "cpu"`，优先归类为 `cpu`
- 否则若 `lossEwma`、`rttEwma`、`bitrateUtilization` 明显恶化，归类为 `network`
- 若当前存在 active override，归类为 `server_override`
- 用户主动 pause / mute / mode switch 归类为 `manual`

同一个采样周期只允许一个主原因，避免 planner 分叉。

### 6.7 状态机

内部状态机统一使用：

- `stable`
- `early_warning`
- `congested`
- `recovering`

用户可见质量等级单独映射为：

- `excellent`
- `good`
- `poor`
- `lost`

#### 6.7.1 状态迁移规则

`stable -> early_warning`

满足以下任一条件连续 2 个样本：

- `bitrateUtilization < 0.85`
- `lossEwma >= 0.04`
- `rttEwma >= 220ms`
- `bandwidthLimited == true`
- `cpuLimited == true`

`early_warning -> congested`

满足以下任一条件连续 2/3 个样本：

- `bitrateUtilization < 0.65`
- `lossEwma >= 0.08`
- `rttEwma >= 320ms`
- `sendBitrateBps` 明显掉到底层配置以下
- 处于 `early_warning` 后执行过降级动作但仍未改善

`early_warning -> stable`

连续 3 个样本全部健康：

- `bitrateUtilization >= 0.92`
- `lossEwma < 0.03`
- `rttEwma < 180ms`
- `qualityLimitationReason` 不再恶化

`congested -> recovering`

同时满足：

- 已经过了恢复冷却时间
- 连续 5 个样本健康
- 当前没有 active override

`recovering -> stable`

- 恢复一级动作后继续连续 5 个样本健康
- 或者已经完全回到 level 0

`recovering -> congested`

- 恢复后连续 2 个样本再次进入明显恶化

#### 6.7.2 lost 判定

`lost` 不是内部 fast loop 状态，而是对外质量等级：

- 连续 5 秒 `sendBitrateBps` 接近 0 且本地未手动 pause
- 或者连续 15 秒没有新的有效快照

这一定义同时用于服务端 stale/lost 聚合。

### 6.8 PeerQosCoordinator

单轨控制器不够，必须有 peer 级协调器。

职责：

- 音频优先级永远高于视频
- 屏幕共享与摄像头同时存在时，优先保屏幕共享
- 决定是否进入 `audio-only`
- 决定视频恢复顺序
- 聚合 peer 级质量等级

强约束：

- 有音频时，视频必须先于音频被牺牲
- 屏幕共享活跃时，摄像头优先降级
- `audio-only` 必须由协调器统一决策，单个视频 controller 不能直接长期自说自话

### 6.9 动作阶梯

动作必须按 source profile 明确建模，而不是临时拼接。

#### 6.9.1 Camera Profile

默认相机档位：

| Level | 目标 | 动作 |
|------|------|------|
| L0 | 正常 | 使用 publish 初始配置 |
| L1 | 轻度降级 | `maxBitrate = 0.85x`，`maxFramerate = 24` |
| L2 | 中度降级 | `maxBitrate = 0.70x`，`maxFramerate = 20`，`scaleResolutionDownBy = 1.5` |
| L3 | 重度降级 | `maxBitrate = 0.45x`，`maxFramerate = 12`，`scaleResolutionDownBy = 2`，若有 simulcast 则 `setMaxSpatialLayer(0)` |
| L4 | 极限保活 | 进入 `audio-only`，视频 upstream pause，优先使用 `zeroRtpOnPause` |

说明：

- 有 simulcast 时，优先使用 `setMaxSpatialLayer()` 做大台阶收缩
- 无 simulcast 时，仅依赖 `setRtpEncodingParameters()`
- 恢复时严格逐级恢复，不允许 L4 -> L0 直接跳

#### 6.9.2 Screen Share Profile

屏幕共享优先保清晰度，不优先保高帧率。

| Level | 目标 | 动作 |
|------|------|------|
| L0 | 正常 | 初始配置 |
| L1 | 降运动成本 | `maxFramerate = 10` |
| L2 | 继续保文字可读 | `maxFramerate = 5`，`maxBitrate = 0.75x` |
| L3 | 重度降级 | `maxFramerate = 3`，`maxBitrate = 0.55x`，仅在必要时 `scaleResolutionDownBy = 1.25` |
| L4 | 极限保活 | 视频 pause，由协调器决定是否只保音频 |

说明：

- 屏幕共享比摄像头更晚降分辨率
- 如果业务支持“暂停画面占位”，占位图由应用层负责，不属于 QoS 控制器

#### 6.9.3 Audio Profile

音频的基本策略是“尽量不断”。

| Level | 目标 | 动作 |
|------|------|------|
| L0 | 正常 | 初始音频配置 |
| L1 | 轻度收缩 | `maxBitrate = 32000`，`adaptivePtime = true` |
| L2 | 中度收缩 | `maxBitrate = 24000` |
| L3 | 重度收缩 | `maxBitrate = 16000` |
| L4 | 保活状态 | 尽量维持发送，不自动 mute；只有明确断连 / 手动操作才停 |

说明：

- 音频不自动进入“静音保活”，因为用户体验通常比低码率更差
- 若要动态启用 Opus DTX/FEC，这属于后续增强项，需要单独考虑重新协商或 codec 参数路径

### 6.10 动作执行规则

动作执行器必须保证：

- 幂等：同一动作重复下发不产生噪音
- 顺序化：同一 producer 上只允许一个动作队列
- 可回滚：执行失败时保留已知配置并产生日志
- 可观测：每次动作都写 trace，包含前后配置和结果

优先级：

1. `server_override`
2. `manual`
3. `cpu`
4. `network`
5. `recovery`

### 6.11 QoS Trace

每次循环必须输出结构化 trace：

```json
{
  "seq": 42,
  "tsMs": 1712736000123,
  "trackId": "camera-main",
  "source": "camera",
  "stateBefore": "early_warning",
  "stateAfter": "congested",
  "reason": "network",
  "signals": {
    "sendBitrateBps": 380000,
    "targetBitrateBps": 900000,
    "lossRate": 0.11,
    "rttMs": 260
  },
  "plannedAction": {
    "type": "setEncodingParameters",
    "level": 3
  },
  "applied": true
}
```

QoS 没有 trace，就没有可维护性。

---

## 7. 服务端慢环设计

### 7.1 角色定位

服务端控制面做这些事：

- 校验客户端 QoS 快照
- 聚合最新 QoS 状态
- 与 worker stats 合并
- 写入 recorder / timeline
- 下发 policy / override
- 做 stale / lost 兜底判断

服务端**不做**这些事：

- 不负责每秒决定是否降一级
- 不直接替端上做 bitrate/fps/resolution 微调
- 不重建浏览器端的瞬时状态机

### 7.2 服务端模块划分

建议在控制面增加：

```text
src/qos/
  QosTypes.h
  QosSnapshot.h/cpp
  QosValidator.h/cpp
  QosPolicy.h/cpp
  QosOverride.h/cpp
  QosRegistry.h/cpp
  QosAggregator.h/cpp
  QosTimeline.h/cpp
```

职责：

- `QosValidator`
  - schema/version 验证
  - 字段归一化
  - seq 去重和乱序丢弃
- `QosRegistry`
  - 保存 peer/track 最新 QoS 快照
- `QosAggregator`
  - 生成 peer 级聚合状态
  - 计算 stale/lost
- `QosTimeline`
  - 落盘给 recorder / 调试
- `QosPolicy`
  - join 时下发默认策略
- `QosOverride`
  - 服务端兜底限制

### 7.3 与现有链路的关系

保留现有 `clientStats` 入口，但其 payload 改为版本化 QoS 对象。

现有链路：

- [src/SignalingServer.cpp](/root/mediasoup-cpp/src/SignalingServer.cpp)
- [src/RoomService.cpp](/root/mediasoup-cpp/src/RoomService.cpp)

目标改造：

- `clientStats` 不再只是“客户端随便发一坨数据”
- 变成 `schema + seq + peerState + tracks[] + lastAction`
- `collectPeerStats()` 和 `statsReport` 统一带上最新 QoS 聚合结果

### 7.4 Stale / Lost 规则

服务端聚合必须考虑客户端上报停止的场景。

规则：

- 距离最新 QoS 快照 `> 6000ms`：标记 `stale = true`
- 距离最新 QoS 快照 `> 15000ms`：聚合质量变为 `lost`
- peer 离开或 session 被替换：立即清理 QoS registry

### 7.5 服务端 override

服务端 override 只做兜底，不做 fast loop。

允许的 override：

- `maxLevelClamp`
- `forceAudioOnly`
- `disableRecovery`
- `ttlMs`

不允许的 override：

- 每秒发一次“把 maxBitrate 改成多少”
- 直接驱动每一个瞬时动作

服务端 override 适用场景：

- 房间级保护
- 特定设备兼容性问题
- 灰度实验
- 紧急熔断

---

## 8. QoS 协议

### 8.1 设计原则

- 版本化
- 明确 schema 名称
- 允许向后兼容扩展
- 支持 seq 去重
- 同一个 schema 在 JS 和 C++ 两边必须共用 fixture 做 contract test

### 8.2 客户端上报：`clientStats`

沿用现有方法名 `clientStats`，但 payload 固定为：

```json
{
  "schema": "mediasoup.qos.client.v1",
  "seq": 42,
  "tsMs": 1712736000123,
  "peerState": {
    "mode": "audio-video",
    "quality": "poor",
    "stale": false
  },
  "tracks": [
    {
      "localTrackId": "camera-main",
      "producerId": "abcd1234",
      "kind": "video",
      "source": "camera",
      "state": "congested",
      "reason": "network",
      "quality": "poor",
      "ladderLevel": 3,
      "signals": {
        "sendBitrateBps": 380000,
        "targetBitrateBps": 900000,
        "lossRate": 0.11,
        "rttMs": 260,
        "jitterMs": 24,
        "frameWidth": 640,
        "frameHeight": 360,
        "framesPerSecond": 12,
        "qualityLimitationReason": "bandwidth"
      },
      "lastAction": {
        "type": "setEncodingParameters",
        "applied": true
      }
    }
  ]
}
```

协议要求：

- `seq` 单调递增
- 服务端只接受最新 `seq`
- `schema` 不正确直接拒绝
- `producerId` 可选，但 publish 后必须补齐

### 8.3 服务端下发：`qosPolicy`

join 成功后和 policy 更新时，服务端下发：

```json
{
  "notification": true,
  "method": "qosPolicy",
  "data": {
    "schema": "mediasoup.qos.policy.v1",
    "sampleIntervalMs": 1000,
    "snapshotIntervalMs": 2000,
    "allowAudioOnly": true,
    "allowVideoPause": true,
    "profiles": {
      "camera": "default",
      "screenShare": "clarity-first",
      "audio": "speech-first"
    }
  }
}
```

### 8.4 服务端兜底：`qosOverride`

```json
{
  "notification": true,
  "method": "qosOverride",
  "data": {
    "schema": "mediasoup.qos.override.v1",
    "scope": "peer",
    "trackId": null,
    "maxLevelClamp": 2,
    "forceAudioOnly": false,
    "disableRecovery": false,
    "ttlMs": 10000,
    "reason": "room_protection"
  }
}
```

### 8.5 `getStats` / `statsReport`

服务端聚合输出中新增：

- `qos`
- `qosStale`
- `qosLastUpdatedMs`

原则：

- 不替代 worker stats
- 与 worker stats 并列
- 让控制面、录制和排障可以同时看到“媒体面观察”和“端上观察”

---

## 9. 测试设计

### 9.1 总原则

QoS 测试必须分层，不允许把信心全部寄托在黑盒测试上。

强制分层：

1. **纯 UT**
2. **控制器组件测试**
3. **协议 contract test**
4. **双端协同集成测试**
5. **真实弱网 E2E**

测试越往上越少、越慢、越贵；越往下越密、越快、越稳定。

### 9.2 可测试性是架构约束

以下要求必须在实现阶段就落地：

- 所有 QoS 模块都能在 fake clock 下运行
- 所有状态迁移都能用 trace replay 重放
- 所有协议都能用 golden fixture 校验
- 所有动作都能通过 `ActionSink` spy 断言
- 所有 override / policy 都能脱离真实 WebSocket 测

若做不到，说明模块设计不合格。

### 9.3 客户端 UT（Jest）

位置：

- `src/client/src/test/test.qos.signals.ts`
- `src/client/src/test/test.qos.stateMachine.ts`
- `src/client/src/test/test.qos.planner.ts`
- `src/client/src/test/test.qos.controller.ts`
- `src/client/src/test/test.qos.protocol.ts`
- `src/client/src/test/test.qos.traceReplay.ts`

说明：

- 当前 `src/client` 的 Jest `testRegex` 约束测试文件名必须以 `test` 开头，因此新增文件必须遵循 `test.qos.*.ts` 命名。

客户端 UT 必测项：

- 原始 stats 到信号提取是否正确
- EWMA / delta / lossRate 计算是否正确
- 网络 / CPU 原因分类是否正确
- 状态迁移是否符合滞后规则
- 恢复节奏是否符合 cooldown
- 不同 profile 下动作阶梯是否正确
- 相同行为是否幂等
- override 是否压过本地 planner
- trace 输出是否完整

### 9.3.1 Trace Replay

这套 QoS 必须支持 trace replay 测试。

做法：

- 将真实或人工构造的 stats 序列存为 fixture
- 用 fake clock 按样本时间推进
- 验证最终状态、动作序列和 trace

建议新增：

```text
src/client/src/test/fixtures/qos/
  camera_mild_congestion.json
  camera_persistent_congestion.json
  camera_recovery.json
  cpu_constrained.json
  screenshare_low_bandwidth.json
  audio_video_audio_only.json
```

### 9.3.2 随机序列与不变量测试

除表驱动测试外，增加固定 seed 的随机序列测试，验证以下不变量：

- 不允许 `stable -> recovering`
- 不允许恢复快于配置的 cooldown
- 不允许连续重复下发完全相同动作
- `server_override` 激活期间，本地 planner 不能越权恢复
- `audio-only` 模式下不得自动恢复视频，除非协调器和 policy 都允许

这类测试不需要引入额外框架，使用固定 seed 生成器即可。

### 9.4 服务端 UT（gtest）

新增建议：

- `tests/test_qos_protocol.cpp`
- `tests/test_qos_validator.cpp`
- `tests/test_qos_registry.cpp`
- `tests/test_qos_aggregator.cpp`
- `tests/test_qos_override.cpp`

这些文件会自动并入 `mediasoup_tests`，因为现有 `CMakeLists.txt` 对 `tests/test_*.cpp` 使用 glob。

服务端 UT 必测项：

- schema/version 校验
- 非法 payload 拒绝
- `seq` 乱序 / 重复丢弃
- stale/lost 计算
- reconnect / leave 清理
- latest snapshot 合并逻辑
- override TTL 到期
- `collectPeerStats()` 聚合输出是否包含 qos

### 9.5 协议 Contract Test

双端协同首先要解决“协议不漂移”。

做法：

新增共享 fixture：

```text
tests/fixtures/qos_protocol/
  valid_client_v1.json
  invalid_missing_schema.json
  invalid_bad_seq.json
  valid_override_v1.json
  valid_policy_v1.json
```

要求：

- Jest 读取这些 fixture，验证 `protocol.ts`
- gtest 读取同一批 fixture，验证 `QosValidator`
- 任一侧修改 schema，必须同步修改 fixture

这层测试比黑盒更重要，因为它能最快阻止“双端都改了，但语义不一致”的错误。

### 9.6 双端协同集成测试

这层测试是本设计的重点。

目标：

- 用**真实服务端**
- 用**真实 QoS controller 代码**
- 但不用真实弱网媒体流量

实现方式：

- 增加一个最小 Node 测试 harness
- 启动真实 `mediasoup-sfu`
- harness 通过 WebSocket 完成 `join`
- harness 内部实例化真实 `PublisherQosController`
- `StatsProvider` 使用可控 fake trace
- `SignalChannel` 使用真实协议收发
- 验证服务端的 `getStats` / `statsReport` / `qosOverride`

建议新增目录：

```text
tests/qos_harness/
  run.mjs
  scenarios/
    camera_congested.json
    camera_recover.json
    audio_only.json
    override_clamp.json
```

必测双端场景：

- 客户端 QoS 快照能被服务端接受并聚合
- 服务端拒绝过期 / 乱序 `seq`
- 服务端 `qosOverride` 能被客户端 controller 接收并生效
- reconnect 后旧 session 快照被拒绝
- `audio-only` 状态能被服务端正确记录
- stale/lost 在服务端按时间推进正确触发

### 9.7 黑盒集成测试（现有 gtest）

现有：

- [tests/test_qos_integration.cpp](/root/mediasoup-cpp/tests/test_qos_integration.cpp)

后续要扩展的重点不是“多几个普通 stats 用例”，而是：

- `clientStats` 上报 QoS 快照
- `getStats` 能查询 QoS 聚合结果
- `statsReport` 含 QoS 字段
- session 替换 / leave 后 QoS 清理
- recorder timeline 含 QoS 状态变化

这层适合测服务端接线正确，不适合测细粒度状态机。

### 9.8 真实弱网 E2E

这层必须有，但只能放 nightly / release gate。

目标：

- 验证真实浏览器 sender stats
- 验证 `qualityLimitationReason`
- 验证真实弱网下动作是否生效

建议方案：

- Headless Chromium / Playwright
- Linux `tc netem`
- 固定场景：
  - 轻度丢包
  - 持续高 RTT
  - 抖动
  - 突然限速
  - 恢复
  - CPU 受限（单独场景）

判定标准不做毫秒级断言，只断言：

- 在约定时间窗内进入目标状态
- 动作顺序正确
- 最终恢复路径正确
- 服务端 timeline 与客户端 trace 一致

### 9.9 CI 分层

| 层级 | 工具 | 位置 | PR 必跑 | Nightly |
|------|------|------|---------|---------|
| Client UT | Jest | `src/client` | 是 | 是 |
| Server UT | gtest | `mediasoup_tests` | 是 | 是 |
| Protocol Contract | Jest + gtest | `tests/fixtures/qos_protocol` | 是 | 是 |
| 双端协同 | Node harness + 真实 SFU | `tests/qos_harness` | 是（快子集） | 是（全量） |
| 服务端黑盒 | gtest | `mediasoup_qos_integration_tests` | 是 | 是 |
| 真实弱网 E2E | Browser + netem | 单独 job | 否 | 是 |

### 9.9.1 覆盖率门槛

QoS 相关模块必须设置单独门槛：

- 客户端 QoS 核心模块：行覆盖率 >= 90%，分支覆盖率 >= 85%
- 服务端 QoS 核心模块：行覆盖率 >= 90%，分支覆盖率 >= 85%

没有达到门槛，不允许标记为“可上线 QoS”。

---

## 10. 实施顺序

### 10.1 Phase 1：端上快环骨架

目标：

- 建立 `src/client/src/qos/`
- 建立 controller / coordinator / protocol / trace 基础骨架
- 只先打通 `camera` 路径

验收：

- client UT 完整
- trace replay 跑通

### 10.2 Phase 2：服务端 slow loop

目标：

- `clientStats` QoS schema
- registry / aggregator / stale/lost
- `getStats` / `statsReport` / recorder timeline

验收：

- gtest + 双端协同 contract 全通过

### 10.3 Phase 3：source profile 完整化

目标：

- `screenShare`
- `audio`
- `PeerQosCoordinator`

验收：

- camera/audio/screenshare 三类 profile 全覆盖

### 10.4 Phase 4：override 与 nightly E2E

目标：

- `qosOverride`
- browser + netem
- nightly trace corpus

验收：

- 真实弱网 E2E 稳定

---

## 11. 风险与规避

### 11.1 浏览器 stats 不一致

风险：

- 各浏览器对 `qualityLimitationReason`、`remote-inbound-rtp` 字段支持不完全一致

规避：

- 抽 `StatsProvider`
- source-specific fallback
- 将字段缺失视为 capability 差异，而不是异常

### 11.2 阈值震荡

风险：

- 弱网和恢复反复切换，导致频繁降级 / 恢复

规避：

- EWMA
- 连续样本判断
- 恢复冷却
- 明确的逐级恢复

### 11.3 双端语义漂移

风险：

- 客户端和服务端都“支持 QoS”，但协议字段或质量语义不一致

规避：

- 共享 fixture
- contract test
- schema version

### 11.4 黑盒测试过度依赖时间

风险：

- 测试 flaky，调试成本高

规避：

- 将大部分复杂度压到 UT / trace replay
- 黑盒只测接线和关键协同语义

---

## 12. 最终要求

以下要求在实现时必须满足：

1. 不允许把 QoS 逻辑直接塞进 `Producer.ts` / `Transport.ts`。
2. 不允许 QoS controller 直接依赖真实时钟或裸 WebSocket。
3. 不允许没有 trace。
4. 不允许只有黑盒测试，没有 pure UT / contract test。
5. 不允许 source 共用一套简单阈值。
6. 不允许音频与视频独立抢占资源，不经协调器直接进入长期模式切换。

这份文档对应的最终交付，不是“做一个能降码率的功能”，而是：

- 一套可解释的上行 QoS 架构
- 一套可维护的双端协议
- 一套可回归的测试体系

这三者缺一不可。
