# 上行 QoS 实施清单

> 配套设计文档：`docs/uplink-qos-design.md`
> Phase 1/2 逐文件拆解：`docs/uplink-qos-phase1-phase2-breakdown.md`
> 当前进展：`docs/uplink-qos-progress.md`
> 目标：把设计拆成可执行的开发阶段、文件改动范围、测试任务和验收标准
> 当前明确不做：`dynacast` / `subscriber-driven uplink`
> 日期：2026-04-10

---

## 1. 实施总原则

### 1.1 交付边界

本实施清单对应的最终交付包含三部分：

- `src/client` 端上的 publisher QoS 快环
- 控制面的 QoS slow loop 与协议
- 一套能够支撑长期迭代的测试矩阵

不接受以下“半成品”交付：

- 只有端上策略，没有服务端聚合
- 只有 `clientStats` 上报，没有状态机
- 只有黑盒测试，没有纯 UT / contract test
- 把 QoS 逻辑散落到 `Producer.ts`、`Transport.ts`、业务层调用代码里

### 1.2 实施策略

整体按以下顺序推进：

1. 先把**可测试的 QoS 核心**做出来
2. 再把**协议和 slow loop**接起来
3. 再补**多 source / 多轨协调**
4. 最后做**override、nightly 弱网 E2E**

原因：

- QoS 最难的是“稳定演进”，不是“第一次跑起来”
- 如果没有 pure core + trace replay，后面每次调阈值都会失控

### 1.3 推荐 PR 切分

推荐至少拆成 7 个 PR，不要一次性大包提交。

| PR | 主题 | 风险 | 是否可独立验收 |
|----|------|------|---------------|
| PR-1 | client QoS skeleton + types + protocol | 低 | 是 |
| PR-2 | signals + stateMachine + planner + UT | 中 | 是 |
| PR-3 | controller + adapter + executor（camera） | 中 | 是 |
| PR-4 | server qos types + validator + registry + protocol | 中 | 是 |
| PR-5 | server/client wire-up + contract + 协同测试 | 高 | 是 |
| PR-6 | audio / screenShare / coordinator | 高 | 是 |
| PR-7 | override + timeline + nightly E2E | 高 | 是 |

---

## 2. 目录与文件规划

## 2.1 客户端新增目录

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

## 2.2 客户端测试目录

```text
src/client/src/test/
  test.qos.protocol.ts
  test.qos.signals.ts
  test.qos.stateMachine.ts
  test.qos.planner.ts
  test.qos.controller.ts
  test.qos.traceReplay.ts
  fixtures/qos/
    camera_mild_congestion.json
    camera_persistent_congestion.json
    camera_recovery.json
    cpu_constrained.json
    screenshare_low_bandwidth.json
    audio_video_audio_only.json
```

说明：

- 现有 Jest `testRegex` 为 `src/test/test.*\\.ts`，所以新测试文件名必须以 `test.` 开头，不需要改 `package.json`。

## 2.3 服务端新增目录

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

## 2.4 服务端测试目录

```text
tests/
  test_qos_protocol.cpp
  test_qos_validator.cpp
  test_qos_registry.cpp
  test_qos_aggregator.cpp
  test_qos_override.cpp
  fixtures/qos_protocol/
    valid_client_v1.json
    invalid_missing_schema.json
    invalid_bad_seq.json
    valid_override_v1.json
    valid_policy_v1.json
```

## 2.5 双端协同测试目录

```text
tests/qos_harness/
  package.json
  run.mjs
  scenarios/
    camera_congested.json
    camera_recover.json
    audio_only.json
    override_clamp.json
```

这层是新的，不放进现有 gtest，而是单独用 Node 跑真实 client code。

---

## 3. 配置与开关规划

## 3.1 客户端配置

建议在 `src/client/src/qos/types.ts` 定义：

```ts
export type UplinkQosOptions = {
  enabled: boolean;
  sampleIntervalMs?: number;
  snapshotIntervalMs?: number;
  allowAudioOnly?: boolean;
  allowVideoPause?: boolean;
  cameraProfile?: string;
  screenShareProfile?: string;
  audioProfile?: string;
  traceBufferSize?: number;
};
```

## 3.2 服务端配置

建议在控制面配置中加入：

- `qos.enabled`
- `qos.sampleIntervalMs`
- `qos.snapshotIntervalMs`
- `qos.allowAudioOnly`
- `qos.allowVideoPause`
- `qos.overrideEnabled`
- `qos.staleAfterMs`
- `qos.lostAfterMs`

这些配置第一版可以放在现有配置结构里，后续再考虑拆到独立配置段。

## 3.3 特性开关策略

必须支持双侧独立开关：

- client 可以启用 QoS 但不上报
- server 可以接收 QoS 快照但不下发 override
- 双侧都开时才进入完整模式

这样便于灰度：

- 本地开发
- staging
- 局部房间
- 局部设备型号

---

## 4. Phase 1：Client Skeleton 与协议基础

## 4.1 目标

建立可测试的 QoS 子系统骨架，但先不接真实 `Producer`。

这一阶段的核心产物：

- `types.ts`
- `constants.ts`
- `protocol.ts`
- `trace.ts`
- `clock.ts`
- `profiles.ts`
- 对应 Jest UT

## 4.2 需要新增/修改的文件

新增：

- `src/client/src/qos/index.ts`
- `src/client/src/qos/types.ts`
- `src/client/src/qos/constants.ts`
- `src/client/src/qos/protocol.ts`
- `src/client/src/qos/trace.ts`
- `src/client/src/qos/clock.ts`
- `src/client/src/qos/profiles.ts`
- `src/client/src/test/test.qos.protocol.ts`

可选修改：

- `src/client/src/index.ts`

建议在 `index.ts` 中显式导出 QoS 类型与工厂函数，但**不要**在这一阶段自动接入 `Transport` 或 `Producer`。

## 4.3 必做测试

- 协议 encode/decode
- schema/version 校验
- seq 单调与类型校验
- trace record append / trim
- profile 默认值与 profile merge

## 4.4 验收标准

- `npm test` 中新增 QoS 测试通过
- 生成的 protocol fixture 可被后续服务端复用
- 尚未接入真实媒体，不影响现有 client 行为

---

## 5. Phase 2：Pure Core（signals + stateMachine + planner）

## 5.1 目标

把 QoS 的高复杂度部分做成纯逻辑模块。

这一阶段禁止直接依赖：

- 真实 `Producer`
- 真实 `RTCRtpSender`
- 真实 WebSocket
- 真实时钟

## 5.2 需要新增的文件

- `src/client/src/qos/sampler.ts`
- `src/client/src/qos/signals.ts`
- `src/client/src/qos/stateMachine.ts`
- `src/client/src/qos/planner.ts`
- `src/client/src/test/test.qos.signals.ts`
- `src/client/src/test/test.qos.stateMachine.ts`
- `src/client/src/test/test.qos.planner.ts`
- `src/client/src/test/test.qos.traceReplay.ts`

## 5.3 实现内容

`signals.ts`

- 原始 snapshot -> 统一信号提取
- delta / EWMA / utilization
- network / cpu reason classifier

`stateMachine.ts`

- `stable`
- `early_warning`
- `congested`
- `recovering`
- 对外 quality map

`planner.ts`

- `camera` profile 的动作阶梯
- 先只做 `camera`
- 产出 `PlannedQosAction[]`

## 5.4 必做测试

- 表驱动测试
- trace replay
- 固定 seed 随机序列测试
- 不变量测试

不变量最少覆盖：

- 不允许 `stable -> recovering`
- 不允许 cooldown 内恢复
- 不允许相同行为重复执行
- reason 分类不能在同一采样周期多主因并存

## 5.5 验收标准

- 纯 UT 覆盖率达到：
  - 行覆盖率 >= 90%
  - 分支覆盖率 >= 85%
- 可用 fixture 回放典型弱网场景

---

## 6. Phase 3：Client Controller 接入（camera）

## 6.1 目标

把 pure core 接到 `src/client` 的真实控制原语上，但只接 `camera`。

## 6.2 需要新增/修改的文件

新增：

- `src/client/src/qos/adapters/producerAdapter.ts`
- `src/client/src/qos/adapters/statsProvider.ts`
- `src/client/src/qos/adapters/signalChannel.ts`
- `src/client/src/qos/executor.ts`
- `src/client/src/qos/controller.ts`
- `src/client/src/test/test.qos.controller.ts`

修改：

- `src/client/src/index.ts`

不建议直接修改：

- `src/client/src/Producer.ts`
- `src/client/src/Transport.ts`

除非仅增加最小的类型导出或注释性辅助方法，否则不应把 QoS 逻辑揉进这两个类。

## 6.3 推荐接入方式

新增工厂函数：

```ts
createPublisherQosController({
  producer,
  source,
  options,
  signalChannel,
})
```

理由：

- 对现有 API 冲击最小
- 易于灰度
- 易于在业务层显式启停
- 不污染 mediasoup-client 核心对象职责

## 6.4 动作执行范围

这一阶段只允许以下动作：

- `setRtpEncodingParameters`
- `setMaxSpatialLayer`
- `pause` / `resume` 视频上行

不做：

- codec 重协商
- DTX/FEC 动态切换
- dynacast

## 6.5 必做测试

- fake clock + fake stats provider
- spy action sink
- 幂等测试
- 队列顺序测试
- 执行失败回退测试

## 6.6 验收标准

- camera 路径能完成：
  - 正常
  - 轻度弱网降级
  - 持续弱网进入 audio-only
  - 恢复逐级回升
- 不影响现有 `Producer`/`Transport` 用例

---

## 7. Phase 4：服务端 QoS Protocol 与 Slow Loop

## 7.1 目标

建立服务端的 QoS 结构、协议校验和聚合能力。

## 7.2 需要新增/修改的文件

新增：

- `src/qos/QosTypes.h`
- `src/qos/QosSnapshot.h`
- `src/qos/QosSnapshot.cpp`
- `src/qos/QosValidator.h`
- `src/qos/QosValidator.cpp`
- `src/qos/QosRegistry.h`
- `src/qos/QosRegistry.cpp`
- `src/qos/QosAggregator.h`
- `src/qos/QosAggregator.cpp`
- `tests/test_qos_protocol.cpp`
- `tests/test_qos_validator.cpp`
- `tests/test_qos_registry.cpp`
- `tests/test_qos_aggregator.cpp`

修改：

- `src/SignalingServer.cpp`
- `src/SignalingServer.h`
- `src/RoomService.cpp`
- `src/RoomService.h`

## 7.3 服务端实现范围

必须完成：

- `clientStats` QoS payload 校验
- `seq` 去重 / 乱序丢弃
- room/peer 级最新快照保存
- stale/lost 聚合
- `getStats` 返回最新 qos 聚合
- `statsReport` 携带 qos

这一阶段先不做：

- `qosOverride`
- recorder timeline

## 7.4 必做测试

- schema/version
- seq 顺序
- 非法 payload
- reconnect / leave 清理
- stale/lost 时间推进

## 7.5 验收标准

- `mediasoup_tests` 中新增 server QoS UT 全通过
- 现有 `clientStats` 链路保持兼容
- `getStats` / `statsReport` 可看到 qos 聚合字段

---

## 8. Phase 5：双端接线与协同测试

## 8.1 目标

把真实 client controller 和真实 server 协议接起来。

## 8.2 需要新增/修改的文件

新增：

- `tests/fixtures/qos_protocol/*`
- `tests/qos_harness/package.json`
- `tests/qos_harness/run.mjs`
- `tests/qos_harness/scenarios/*`

修改：

- `src/client/src/qos/adapters/signalChannel.ts`
- `src/SignalingServer.cpp`
- `src/RoomService.cpp`
- `tests/test_qos_integration.cpp`

## 8.3 协同测试目标

必须覆盖：

- 客户端发送合法 QoS 快照，服务端成功聚合
- 客户端发送错误 schema，服务端拒绝
- 旧 session / stale session 发送 QoS 快照被拒绝
- `getStats` 查到 client-side qos
- `statsReport` 广播 qos

## 8.4 必做测试

客户端侧：

- `protocol.ts` 读取共享 fixture

服务端侧：

- `QosValidator` 读取同一批 fixture

双端侧：

- Node harness 启真实 `mediasoup-sfu`
- Controller 用 fake trace 驱动真实协议

## 8.5 验收标准

- 双端 contract test 通过
- 双端协同测试通过
- 现有 `mediasoup_qos_integration_tests` 不回退

---

## 9. Phase 6：Audio / ScreenShare / Coordinator

## 9.1 目标

补齐非 camera 路径，以及 peer 级协调器。

## 9.2 需要新增/修改的文件

新增或扩展：

- `src/client/src/qos/coordinator.ts`
- `src/client/src/qos/profiles.ts`
- `src/client/src/test/test.qos.coordinator.ts`
- 新 trace fixtures

## 9.3 必做功能

- audio profile
- screenShare profile
- `PeerQosCoordinator`
- `audio-only` 统一决策
- 恢复顺序控制

## 9.4 必做测试

- 音视频并存，弱网时先牺牲视频
- 屏幕共享与摄像头并存，优先保屏幕共享
- audio-only 进入和退出
- 多轨恢复顺序

## 9.5 验收标准

- 三类 source 均有独立 profile
- 多轨场景不出现互相打架的动作
- trace 中可见 coordinator 决策

---

## 10. Phase 7：Override、Timeline 与 Nightly E2E

## 10.1 目标

完成 slow loop 的剩余能力和真实弱网验证。

## 10.2 需要新增/修改的文件

新增：

- `src/qos/QosPolicy.h/cpp`
- `src/qos/QosOverride.h/cpp`
- `src/qos/QosTimeline.h/cpp`
- `tests/test_qos_override.cpp`

修改：

- `src/SignalingServer.cpp`
- `src/RoomService.cpp`
- `src/Recorder.h`

测试基础设施：

- browser-based weak network harness
- netem scripts
- nightly workflow

## 10.3 必做功能

- `qosPolicy`
- `qosOverride`
- recorder QoS timeline
- browser + `tc netem`

## 10.4 必做测试

- override clamp
- override TTL 到期
- policy update
- timeline 落盘
- nightly 弱网 E2E

## 10.5 验收标准

- 服务端可下发 override，客户端可正确执行并 trace
- recorder 能看到 QoS timeline
- nightly 弱网场景稳定

---

## 11. 具体测试矩阵

## 11.1 PR 必跑

| 类别 | 命令 / 套件 |
|------|-------------|
| Client UT | `cd src/client && npm test` |
| Server UT | `./build/mediasoup_tests` |
| Server QoS 黑盒 | `./build/mediasoup_qos_integration_tests` |
| 双端快协同 | `node tests/qos_harness/run.mjs --quick` |

## 11.2 Nightly 必跑

| 类别 | 内容 |
|------|------|
| 双端全量协同 | 全 scenario |
| Browser 弱网 E2E | Chromium + netem |
| 回放回归 | 全 trace fixture replay |

## 11.3 覆盖率要求

客户端 QoS 核心：

- 行覆盖率 >= 90%
- 分支覆盖率 >= 85%

服务端 QoS 核心：

- 行覆盖率 >= 90%
- 分支覆盖率 >= 85%

---

## 12. 每个 PR 的验收模板

每个 QoS PR 必须在描述中回答以下问题：

1. 这次改动属于哪个 phase / 哪个 PR 切片？
2. 新增了哪些模块？是否仍满足机制/策略分离？
3. 新增了哪些 pure UT？
4. 是否新增了共享 fixture？
5. 是否影响已有 client/server API？
6. trace 是否覆盖新增逻辑？
7. 是否需要新配置项或 feature flag？

没有这 7 项，不建议合并。

---

## 13. Definition of Done

只有同时满足以下条件，才可认为“上行 QoS 第一阶段可上线”：

- camera / audio / screenShare 三类 source 均有稳定 profile
- 端上快环在 weak network trace replay 下稳定
- client/server 协议有共享 contract fixture
- 服务端能聚合并暴露 qos 状态
- reconnect / stale / lost 行为清晰
- 双端协同测试稳定
- nightly 浏览器弱网测试稳定

如果缺少其中任一项，这套 QoS 仍属于“开发中能力”，不应宣称可上线。
