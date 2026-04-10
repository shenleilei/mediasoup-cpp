# 上行 QoS Phase 1 / Phase 2 逐文件拆解

> 配套文档：
> - `docs/uplink-qos-design.md`
> - `docs/uplink-qos-implementation-plan.md`
> 目的：把 Phase 1 和 Phase 2 拆成可以直接执行的逐文件开发任务
> 日期：2026-04-10

---

## 1. 适用范围

这份文档只覆盖：

- `PR-1`: client QoS skeleton + types + protocol
- `PR-2`: signals + stateMachine + planner + UT

这份文档刻意**不覆盖**：

- controller 接真实 `Producer`
- 服务端 slow loop
- 双端协同 harness
- audio / screenshare / coordinator
- override / timeline

原因：

- Phase 1 / Phase 2 的目标是把 QoS 的**纯核心和协议地基**打稳
- 这一步不应被接线和媒体细节污染

---

## 2. 当前代码基线

### 2.1 可以复用的客户端能力

`src/client` 当前已经具备：

- `Producer.getStats()`
- `Producer.pause()/resume()`
- `Producer.setMaxSpatialLayer()`
- `Producer.setRtpEncodingParameters()`
- `Producer.replaceTrack()`

这些能力在 Phase 1 / Phase 2 暂时不直接接入，但会决定后续 action 模型的边界。

### 2.2 当前导出方式

目前 `src/client/src/index.ts` 只导出：

- `types`
- `version`
- `Device`
- `detectDevice`
- `parseScalabilityMode`
- `debug`

因此 Phase 1 如果要暴露 QoS 类型或工厂，需要显式规划导出方式，而不是隐式混入已有 `types` 命名空间。

### 2.3 当前测试能力

`src/client` 当前测试基线：

- Jest
- Node 环境
- `FakeHandler`
- 现有测试文件都在 `src/client/src/test/test.ts`

这意味着：

- Phase 1 / Phase 2 可以高质量做 UT
- 但不适合依赖 `FakeHandler.getSenderStats()`，因为它返回空 `Map`
- QoS 核心测试应基于 fake snapshot / fake clock / spy action，而不是继续堆积 `FakeHandler`

---

## 3. Phase 1 目标

Phase 1 的交付目标是：

- 建立 QoS 目录骨架
- 定义协议、核心类型、常量、profile、trace
- 让 QoS 模块可以被独立 import
- 为 Phase 2 提供纯逻辑开发地基

Phase 1 **不做**：

- 真实状态机
- 真实信号提取
- 真实动作规划
- 真实 controller loop
- 真实媒体接入

一句话：

**Phase 1 只建地基和约束，不建业务行为。**

---

## 4. Phase 1 文件级任务

## 4.1 `src/client/src/qos/index.ts`

### 目标

QoS 模块统一入口，负责稳定导出。

### 最小实现

- 导出 `types.ts` 中公开类型
- 导出 `protocol.ts` 的 encode/decode helper
- 导出 `profiles.ts` 的 profile factory
- 导出 `trace.ts` 的 trace 类型和 ring buffer helper

### 不做

- 不导出 controller
- 不导出任何真实媒体依赖对象

### 验收

- 业务层可以 `import * as qos from './qos'`

---

## 4.2 `src/client/src/qos/types.ts`

### 目标

定义 Phase 1/2 所需的公共类型，不含具体实现。

### 必须包含的类型

- `QosSource = 'camera' | 'screenShare' | 'audio'`
- `QosReason = 'network' | 'cpu' | 'manual' | 'server_override' | 'unknown'`
- `QosState = 'stable' | 'early_warning' | 'congested' | 'recovering'`
- `QosQuality = 'excellent' | 'good' | 'poor' | 'lost'`
- `QosSchemaName`
- `RawSenderSnapshot`
- `DerivedQosSignals`
- `ClientQosTrackSnapshot`
- `ClientQosSnapshot`
- `QosPolicy`
- `QosOverride`
- `QosTraceEntry`
- `QosProfile`
- `QosThresholds`

### 建议结构

按以下顺序组织：

1. 基础枚举字面量类型
2. 原始 stats 类型
3. 派生信号类型
4. 协议类型
5. trace 类型
6. profile 类型

### 实现约束

- 不在这个文件里写 helper 逻辑
- 不引入浏览器类型之外的运行时依赖
- 保持可被 server contract fixture 映射

### 验收

- 所有下游文件只从这里 import 基础 QoS 类型

---

## 4.3 `src/client/src/qos/constants.ts`

### 目标

集中定义默认配置和 schema 名称。

### 必须包含

- schema 常量：
  - `QOS_CLIENT_SCHEMA_V1`
  - `QOS_POLICY_SCHEMA_V1`
  - `QOS_OVERRIDE_SCHEMA_V1`
- 默认周期：
  - `DEFAULT_SAMPLE_INTERVAL_MS`
  - `DEFAULT_SNAPSHOT_INTERVAL_MS`
  - `DEFAULT_RECOVERY_COOLDOWN_MS`
- trace:
  - `DEFAULT_TRACE_BUFFER_SIZE`

### 实现约束

- 所有阈值常量先只放**跨 profile 的全局默认值**
- camera / audio / screenshare 的差异化阈值放到 `profiles.ts`

### 验收

- schema 名称和默认周期均可被 UT 引用

---

## 4.4 `src/client/src/qos/profiles.ts`

### 目标

定义 profile 的静态默认配置。

### Phase 1 范围

先只建结构，不实现具体 planner。

### 必须提供的函数

- `getDefaultCameraProfile(): QosProfile`
- `getDefaultScreenShareProfile(): QosProfile`
- `getDefaultAudioProfile(): QosProfile`
- `resolveProfile(source, override?)`

### Camera Profile 在 Phase 1 的最低内容

- source
- level count
- sample interval
- snapshot interval
- recovery cooldown
- 阈值占位
- action ladder 占位描述

### 注意

Profile 数据结构必须服务于后续 planner，不要写成“只给 UI 看的文档对象”。

### 验收

- 后续 Phase 2 可以直接拿 profile 驱动 planner

---

## 4.5 `src/client/src/qos/protocol.ts`

### 目标

实现 QoS 协议的 encode/decode 和基本 validation。

### 必须提供的函数

- `isClientQosSnapshot(value): boolean`
- `isQosPolicy(value): boolean`
- `isQosOverride(value): boolean`
- `serializeClientQosSnapshot(snapshot): unknown`
- `parseClientQosSnapshot(payload): ClientQosSnapshot`
- `parseQosPolicy(payload): QosPolicy`
- `parseQosOverride(payload): QosOverride`

### 约束

- 这里只做**轻量结构校验**
- 不做完整业务语义判断
- 不做 seq 新旧判断
- 不做时间相关逻辑

### 实现细节建议

- 使用本地 type guards
- 错误信息要明确，便于后续 contract test

### 验收

- 客户端侧 protocol fixture 能跑通

---

## 4.6 `src/client/src/qos/trace.ts`

### 目标

提供 trace 记录与 ring buffer 工具。

### 必须提供的结构

- `QosTraceBuffer`
- `appendTraceEntry(entry)`
- `getTraceEntries()`
- `clearTrace()`

### 实现约束

- ring buffer 必须固定上限
- 插入 O(1)
- 不引入日志框架依赖

### Phase 1 只做

- 数据结构
- buffer 规则
- 不做 trace pretty printer

### 验收

- trace buffer 可在 UT 中断言顺序和裁剪行为

---

## 4.7 `src/client/src/qos/clock.ts`

### 目标

为 Phase 2 的 fake clock 和真实 clock 提供抽象。

### 必须定义

- `QosClock` interface
- `SystemQosClock`
- `ManualQosClock` 或 `FakeQosClock`

### 验收

- Phase 2 状态机和回放测试不需要依赖真实 `Date.now()`

---

## 4.8 `src/client/src/index.ts`

### 目标

决定如何导出 QoS。

### 推荐改法

新增：

```ts
export * as qos from './qos';
```

### 不推荐改法

- 把 QoS 类型塞进现有 `types` namespace
- 在根入口直接导出大量散装 QoS 符号

理由：

- QoS 是一个独立子系统
- 命名空间导出更干净

### 验收

- `import { qos } from 'mediasoup-client'` 成立

---

## 4.9 `src/client/src/test/test.qos.protocol.ts`

### 目标

覆盖 Phase 1 协议与基础结构。

### 最低用例

- valid client snapshot
- invalid schema
- invalid missing field
- valid policy
- valid override
- trace buffer capacity
- profile resolve

### 验收

- Jest 中独立运行稳定

---

## 5. Phase 1 推荐提交顺序

建议按以下 commit 顺序完成：

1. `qos/types.ts + constants.ts`
2. `qos/profiles.ts + qos/clock.ts`
3. `qos/protocol.ts`
4. `qos/trace.ts`
5. `qos/index.ts + client index export`
6. `test.qos.protocol.ts`

这样回滚和 review 都最清晰。

---

## 6. Phase 1 完成标准

Phase 1 完成时必须满足：

- QoS 目录存在且结构稳定
- 所有基础类型、schema、profile、trace、clock 已具备
- 根入口已能按 namespace 暴露 QoS
- 协议 UT 完整
- 不影响现有 client 行为

如果 Phase 1 结束后已经开始接真实媒体，说明范围失控。

---

## 7. Phase 2 目标

Phase 2 的交付目标是：

- 完成 pure core：
  - signals
  - stateMachine
  - planner
- 建立 trace replay 体系
- 建立不变量测试

Phase 2 **仍然不做**：

- controller 接 `Producer`
- 真实 WebSocket 协议收发
- 服务端接线

一句话：

**Phase 2 把“可决策的纯核心”做完，但仍然不碰真实接线。**

---

## 8. Phase 2 文件级任务

## 8.1 `src/client/src/qos/sampler.ts`

### 目标

定义采样驱动器的最小形态，为后续 controller 服务。

### Phase 2 只做

- `QosStatsProvider` interface
- 一次采样结果封装
- 基于 `QosClock` 的采样 loop helper

### 不做

- 不直接绑定真实 `Producer`
- 不做 signal channel

### 验收

- fake provider + fake clock 下可驱动样本流

---

## 8.2 `src/client/src/qos/signals.ts`

### 目标

从原始 snapshot 提取稳定信号。

### 必须提供的函数

- `computeDelta(current, previous)`
- `computeBitrate(...)`
- `computeLossRate(...)`
- `computeEwma(...)`
- `deriveSignals(raw, prev, profile)`
- `classifyReason(derived, context)`

### Phase 2 最低输入

`RawSenderSnapshot` 至少要支持：

- timestamp
- bytesSent
- packetsSent
- packetsLost
- targetBitrate
- roundTripTime
- jitter
- frameWidth
- frameHeight
- framesPerSecond
- qualityLimitationReason

### Phase 2 最低输出

- `sendBitrateBps`
- `targetBitrateBps`
- `bitrateUtilization`
- `lossRate`
- `lossEwma`
- `rttMs`
- `rttEwma`
- `jitterMs`
- `cpuLimited`
- `bandwidthLimited`
- `reason`

### 必做测试

- bitrate 计算正确
- lossRate 计算正确
- EWMA 正确
- 缺失字段时 fallback 正确
- `qualityLimitationReason=cpu` 优先级正确

### 验收

- 纯输入输出，不依赖外部状态

---

## 8.3 `src/client/src/qos/stateMachine.ts`

### 目标

实现纯状态迁移逻辑。

### 必须提供的结构

- `QosStateMachineContext`
- `evaluateStateTransition(previousState, signals, profile, nowMs)`
- `mapStateToQuality(state, signals)`

### 设计要求

- 不产生动作
- 只做状态变更判定
- 必须使用连续样本计数与 cooldown

### 建议内部字段

- `state`
- `enteredAtMs`
- `lastCongestedAtMs`
- `lastRecoveryAtMs`
- `consecutiveHealthySamples`
- `consecutiveWarningSamples`
- `consecutiveCongestedSamples`

### 必做测试

- `stable -> early_warning`
- `early_warning -> congested`
- `early_warning -> stable`
- `congested -> recovering`
- `recovering -> stable`
- `recovering -> congested`
- cooldown 阻止过早恢复
- illegal transition 不发生

### 验收

- 所有状态迁移都由纯函数或纯对象方法驱动

---

## 8.4 `src/client/src/qos/planner.ts`

### 目标

根据状态和 profile 规划动作。

### Phase 2 范围

只做 `camera`。

### 必须提供的函数

- `planActions(input): PlannedQosAction[]`
- `planRecovery(input): PlannedQosAction[]`

### 必须支持的动作类型

- `setEncodingParameters`
- `setMaxSpatialLayer`
- `enterAudioOnly`
- `exitAudioOnly`
- `noop`

### 注意

`planner.ts` 只能决定“该做什么”，不能决定“怎么调用 Producer”。

### 必做测试

- 各 level 的动作是否匹配 profile
- audio-only 进入时机
- recovery 是否逐级
- override clamp 下不越界
- 相同 level 不重复规划

### 验收

- planner 输出稳定、可预测、无副作用

---

## 8.5 `src/client/src/test/test.qos.signals.ts`

### 最低用例

- 基础 bitrate / loss / RTT 提取
- EWMA 与缺失值 fallback
- CPU reason vs network reason 分类
- 多样本趋势输入

### Fixture 建议

- 直接用小型 table-driven case
- 不需要一开始就上大 replay fixture

---

## 8.6 `src/client/src/test/test.qos.stateMachine.ts`

### 最低用例

- 每一条合法迁移
- 每一条非法迁移
- cooldown
- 连续样本计数
- quality map

### 风格建议

- 用 table-driven
- 不要把测试写成一堆长篇步骤式脚本

---

## 8.7 `src/client/src/test/test.qos.planner.ts`

### 最低用例

- camera L0-L4
- no-op 场景
- 恢复逐级
- clamp
- audio-only gate

---

## 8.8 `src/client/src/test/test.qos.traceReplay.ts`

### 目标

建立未来调阈值时最关键的回归资产。

### Phase 2 最低要求

至少提供以下 3 个 replay fixture：

- `camera_mild_congestion.json`
- `camera_persistent_congestion.json`
- `camera_recovery.json`

### 每个 replay 测什么

- 最终状态
- 动作序列
- trace 序列长度
- 不变量

### 重要约束

replay 测试必须用 fake clock，不允许真实 sleep。

---

## 9. Phase 2 推荐提交顺序

建议按以下 commit 顺序完成：

1. `sampler.ts + QosStatsProvider`
2. `signals.ts + test.qos.signals.ts`
3. `stateMachine.ts + test.qos.stateMachine.ts`
4. `planner.ts + test.qos.planner.ts`
5. `fixtures/qos/* + test.qos.traceReplay.ts`

不要把 signals、stateMachine、planner 一次性混成一个 commit。

---

## 10. Phase 2 完成标准

Phase 2 完成时必须满足：

- pure core 已完成
- `camera` 路径可通过纯逻辑回放得到稳定结果
- replay fixture 已建立
- 随机序列与不变量测试已建立
- 覆盖率达到设计门槛

如果 Phase 2 结束后没有 replay fixture，这个 phase 不算完成。

---

## 11. Phase 1 / Phase 2 禁止事项

在这两个 phase 中，明确禁止：

1. 直接修改 `Producer.ts` / `Transport.ts` 添加 QoS 分支逻辑。
2. 直接依赖真实 `Producer.getStats()` 写状态机。
3. 在协议层里塞入时间和业务判断。
4. 在 planner 里直接调用执行动作。
5. 用 `setTimeout`/sleep 写测试。
6. 只写 happy path，不写不变量测试。

这些问题一旦在 Phase 1 / 2 出现，后面很难收回来。

---

## 12. Phase 1 / Phase 2 交付检查表

提交 PR 前必须逐项打勾：

- [ ] QoS 类型全部收敛在 `types.ts`
- [ ] schema 名称全部收敛在 `constants.ts`
- [ ] 协议测试已覆盖 invalid payload
- [ ] `QosClock` 已抽象，测试不依赖真实时间
- [ ] `signals.ts` 不依赖外部运行时对象
- [ ] `stateMachine.ts` 不产生动作
- [ ] `planner.ts` 不执行动作
- [ ] trace replay fixture 已入库
- [ ] 至少一组随机序列不变量测试已入库
- [ ] 没有把 QoS 逻辑揉进 `Producer.ts` / `Transport.ts`

---

## 13. Phase 2 结束后的下一步入口

只有当 Phase 1 / 2 全部完成后，才开始：

- `producerAdapter.ts`
- `executor.ts`
- `controller.ts`
- 真实 `Producer` 接线

否则 controller 会被迫吞下不稳定的 pure core，后面改动成本会急剧上升。
