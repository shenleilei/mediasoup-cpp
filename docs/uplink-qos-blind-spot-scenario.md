# 上行 QoS 极端盲区场景分析

日期：`2026-04-13`

> **文档性质**
>
> 这是一个专项场景分析文档，回答下面这个问题：
> 当上行从高质量网络突然掉进移动信号盲区，并在较长时间后恢复时，
> 当前 QoS 系统理论上会怎么变化，实际 browser loopback matrix 又观察到了什么，
> 以及我们最终是如何修正恢复逻辑的。
>
> 这份文档不替代 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md) 的签收口径；
> 也不替代 [uplink-qos-boundaries.md](/root/mediasoup-cpp/docs/uplink-qos-boundaries.md) 的职责边界说明。

## 1. 场景定义

这里讨论的目标场景是：

- baseline：
  `8Mbps / 20ms RTT / 0.1% loss / 1ms jitter`
- impairment：
  突然进入盲区，变成
  `200kbps / 500ms RTT / 20% loss / 50ms jitter`
- impairment 持续：
  `100s`
- recovery：
  网络恢复回 baseline 级别

为了把这个场景落到自动化矩阵中，当前仓库新增了 3 个 blind-spot transition case，
并且现在已经纳入默认 matrix 集合：

- [sweep_cases.json](/root/mediasoup-cpp/tests/qos_harness/scenarios/sweep_cases.json#L46) `T9`
  baseline=`8Mbps / 20ms / 0.1% / 1ms`
- [sweep_cases.json](/root/mediasoup-cpp/tests/qos_harness/scenarios/sweep_cases.json#L47) `T10`
  baseline=`15Mbps / 15ms / 0.1% / 1ms`
- [sweep_cases.json](/root/mediasoup-cpp/tests/qos_harness/scenarios/sweep_cases.json#L48) `T11`
  baseline=`30Mbps / 10ms / 0.1% / 1ms`

当前已实际执行并复核的是 `T9`。

## 2. 理论时间线推导

### 2.1 推导前提

下面这部分是“按生产默认 QoS 策略推导”的理论时间线，不是 matrix harness 的直接实测。

推导前提主要来自：

- 默认策略下发：
  [RoomService.cpp](/root/mediasoup-cpp/src/RoomService.cpp#L846)
  `sampleIntervalMs=1000`
  `snapshotIntervalMs=2000`
- 默认 camera 阈值与 ladder：
  [profiles.js](/root/mediasoup-cpp/src/client/lib/qos/profiles.js#L8)
  [profiles.js](/root/mediasoup-cpp/src/client/lib/qos/profiles.js#L22)
- 默认状态机 debounce / recovery 条件：
  [stateMachine.js](/root/mediasoup-cpp/src/client/lib/qos/stateMachine.js#L83)
  [constants.js](/root/mediasoup-cpp/src/client/lib/qos/constants.js#L7)

核心参数是：

- `warnLossRate=4%`
- `congestedLossRate=8%`
- `warnRttMs=220`
- `congestedRttMs=320`
- `warnJitterMs=30`
- `congestedJitterMs=60`
- `stable -> early_warning` 需要连续 `2` 个 warning 样本
- `early_warning -> congested` 需要连续 `2` 个 congested 样本
- `congested -> recovering` 需要 `8s cooldown + 5` 个 healthy 样本

### 2.2 理论推导结果

如果按生产默认策略推导，这个场景大致会是下面的时间线：

1. `t < 0s`
   baseline 保持 `stable/L0`。

2. `t ≈ 1s ~ 2s`
   因为 `20% loss`、`500ms RTT` 已远超 warning 阈值，客户端大概率进入 `early_warning`；
   通常先出现 `L0 -> L1` 的轻度降级。

3. `t ≈ 3s ~ 4s`
   继续满足 congested 条件后，客户端进入 `congested`。

4. `t ≈ 4s ~ 6s`
   本地动作继续下探：
   `L1 -> L2 -> L3 -> L4`。
   对默认 camera profile 而言，`L4` 是 `enterAudioOnly`，
   即视频上行基本停掉，只保留音频或最小生存路径。

5. `t ≈ 5s ~ 15s`
   如果服务端还能持续收到 `clientStats`：

   - `poor` 会触发 `server_auto_poor`
   - `lost` 会触发 `server_auto_lost`

   对应逻辑在：
   [QosOverride.cpp](/root/mediasoup-cpp/src/qos/QosOverride.cpp#L48)

6. `t = 0s ~ 100s`
   在整段坏网期间，系统理论上会长期维持在重保护态，
   也就是 `congested/L4` 或接近 `L4` 的状态。

7. `t = 100s`
   网络恢复。

8. `t ≈ 105s`
   如果恢复后采样重新变健康，客户端理论上大概需要 `5` 个 healthy 样本，
   才会从 `congested` 进入 `recovering`。

9. `t ≈ 105s ~ 120s+`
   恢复不是一步到 `L0`，而是 probe 式逐级回升。
   如果恢复窗口足够干净，理论上通常希望在后续 `10s ~ 20s` 内逐步回到 `stable/L0`。

### 2.3 理论推导的简短结论

按生产默认策略的直觉，系统应当表现为：

- 几秒内快速打到重保护。
- 在长时间盲区中维持保护。
- 网络恢复后，先经过一个短暂观察窗口，再逐级恢复。

## 3. `T9` 的实测口径

### 3.1 `T9` 的执行方式

本次复核命令是：

```bash
./scripts/run_qos_tests.sh matrix --matrix-cases=T9
```

脚本中的 matrix 参数透传见：

- [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh#L43)
- [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh#L428)
- [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh#L500)

### 3.2 `T9` 与生产默认策略的关键差异

`T9` 不是直接跑生产真实链路，而是 browser loopback matrix harness。
因此它和上面的理论推导有几个重要差异：

1. 采样频率更高
   [loopback-entry.js](/root/mediasoup-cpp/tests/qos_harness/browser/loopback-entry.js#L320)
   这里用的是：
   `sampleIntervalMs=500`
   `snapshotIntervalMs=5000`

2. loopback 使用的是专门调过的 camera profile
   [loopback-entry.js](/root/mediasoup-cpp/tests/qos_harness/browser/loopback-entry.js#L238)
   阈值与生产默认并不完全相同。

3. loopback 的 `L4` 不是 `audio-only`
   [loopback-entry.js](/root/mediasoup-cpp/tests/qos_harness/browser/loopback-entry.js#L291)
   当前 harness 的 `L4` 是
   `minimum video keepalive`
   即保留一个很低码率、低帧率、强降采样的视频生存态，
   而不是生产默认 profile 里的 `enterAudioOnly`。

这意味着：

- 理论推导里的“进入 audio-only”不能直接套到 `T9`。
- `T9` 更像是在验证“极端弱网下 browser loopback 的重保护和恢复能力”，
  而不是在一比一复刻生产默认上行行为。

## 4. 第一次 `T9` 实测暴露的问题

在第一次引入 `T9` 时，case 能进入 `congested/L4`，但在 `30s` recovery window 内始终回不到 baseline。

归档结果见：

- [第一次失败快照](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T11-47-26.985Z/docs/generated/uplink-qos-case-results.targeted.md)
- [第一次失败 JSON](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T11-47-26.985Z/docs/generated/uplink-qos-matrix-report.targeted.json)

当时的关键结论是：

- baseline：
  `stable/L0`
- impairment：
  `peak=congested/L4`
- recovery：
  `best=congested/L4`
- verdict：
  `FAIL`
  原因是：
  `recoveryPassed=false`
  `analysis=恢复过慢`

### 4.1 当时的根因分析

根因不在 `loss` 或 `RTT`，主要在 `jitter`：

- recovery 开始后，`loss` 和 `RTT` 恢复很快
- 但 `jitter` 的原始值和 `jitterEwma` 拖尾很长
- loopback profile 的 `stableJitterMs` 又收得比较紧，为 `18ms`

也就是说，当时的状态机卡在了这样一种状态：

- `loss` 好了
- `RTT` 好了
- `jitter` 也在变好
- 但 `jitterEwma` 还没低到“最终稳定阈值”

结果就是：

`congested -> recovering` 被“回到 stable 的严格 jitter 条件”一起绑住了。`

## 5. 实施的修正

本次没有粗暴放宽“回到 stable”的 jitter 阈值，而是把恢复拆成了两阶段：

1. `congested -> recovering`
   允许使用更宽松的 recovery jitter 门槛开始恢复。

2. `recovering -> stable`
   继续使用原来的严格 stable jitter 门槛。

实现上，当前做的是：

- 新增 `isRecoveryHealthy()`
- 在 `congested -> recovering` 时，使用 `warnJitterMs` 作为更宽松的 jitter 门槛
- 但 `recovering -> stable` 仍然使用 `stableJitterMs`

对应代码在：

- [stateMachine.js](/root/mediasoup-cpp/src/client/lib/qos/stateMachine.js)

关键变化是：

- [stateMachine.js](/root/mediasoup-cpp/src/client/lib/qos/stateMachine.js) 中新增了 `isRecoveryHealthy()`
- `QosStateMachineContext` 新增了 `consecutiveRecoverySamples`
- `congested -> recovering` 现在不再依赖 `consecutiveHealthySamples`
  而是依赖 `consecutiveRecoverySamples`

这个改法的目标是：

- 不放松“最终回到 stable”的标准
- 但允许系统在 `loss/RTT` 已恢复、`jitter` 已明显回落时尽早开始 recovery probing

### 5.1 为什么这个改法比直接放宽 `18ms` 更稳

我没有直接把 loopback 的 `stableJitterMs` 从 `18` 改到 `25` 之类，
原因是那会同时改掉：

- 正常场景的稳定判定
- 边界 jitter case 的敏感度
- 现有 `J3 / J4 / J5` 的经验对齐口径

而“两阶段恢复”只改变：

- 什么时候允许从 `congested` 开始进入 `recovering`

并没有改变：

- 什么时候才算真正回到 `stable`

这是一种更有滞后的恢复策略，而不是简单把整体系统变迟钝。

### 5.2 继续修正：stable 尾段升级也纳入 probe

在后续 targeted rerun 里，`T9` 一度出现过这样一种现象：

- recovery phase 的 `best` 已经到 `stable/L0`
- 但 recovery 结束瞬时的 `current` 又回落到 `congested/L4`

这说明问题不只在 `congested -> recovering` 的入口门槛。
进一步复盘 trace 后，发现根因是：

- probe 节流原本只在 `recovering` 状态里生效
- 一旦状态机刚回到 `stable`，controller 仍可能继续每个 sample 往上提档
- 这些 `stable` 态升级没有等待前一个 probe 完成，容易在尾段 overshoot

因此这次又补了一层节流：

- 任何“放松保护”的动作
  包括：
  `level` 下降
  `exitAudioOnly`
  都会启动 probe
- 只要 probe 还在进行中，controller 就维持当前 level，不继续下一步升级

对应代码在：

- [controller.js](/root/mediasoup-cpp/src/client/lib/qos/controller.js)

这个修正的直接目标是：

- 避免 recovery 尾段从 `L3/L2/L1` 连跳回 `L0`
- 把“恢复到更高等级”也纳入和 `recovering` 同样的观察窗口
- 让 `T9` 不只是出现“短暂恢复”，而是能在 case 结束时真正稳定在 `stable/L0`

### 5.3 相关验证

为这个改动补充了单测：

- [test.qos.stateMachine.js](/root/mediasoup-cpp/src/client/lib/test/test.qos.stateMachine.js)
  新增了：
  - recovery jitter 门槛比 stable 门槛更宽松
  - `recovering` 不会因为宽松门槛直接跳回 `stable`
- [test.qos.controller.js](/root/mediasoup-cpp/src/client/lib/test/test.qos.controller.js)
  新增 / 修正了：
  - audio-only 恢复用例
  - `stable` 态升级也必须跑完 probe，不能连续提档

本次直接执行通过的单测是：

- `lib/test/test.qos.stateMachine.js`
- `lib/test/test.qos.controller.js`

## 6. 修正后的 `T9` 结果

修正后重新执行：

```bash
./scripts/run_qos_tests.sh matrix --matrix-cases=T9
```

当前 targeted 结果见：

- [当前 targeted case report](/root/mediasoup-cpp/docs/generated/uplink-qos-case-results.targeted.md)
- [当前 targeted matrix json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json)

归档快照见：

- [第一次修正后通过快照](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T12-16-10.821Z/docs/generated/uplink-qos-case-results.targeted.md)
- [第一次修正后通过 JSON](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T12-16-10.821Z/docs/generated/uplink-qos-matrix-report.targeted.json)
- [最新稳定收尾快照](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T12-55-55.178Z/docs/generated/uplink-qos-case-results.targeted.md)
- [最新稳定收尾 JSON](/root/mediasoup-cpp/docs/archive/uplink-qos-runs/2026-04-12T12-55-55.178Z/docs/generated/uplink-qos-matrix-report.targeted.json)

关键结果如下：

- baseline：
  `stable/L0`
- impairment：
  `peak=congested/L4`
- recovery：
  `best=stable/L0`
- recovery 结束瞬时：
  `current=stable/L0`
- verdict：
  `PASS`

### 6.1 关键时间点

这次最有价值的时间点是：

- impairment：
  `t_detect_congested = 36157ms`
- recovery：
  首次进入 `recovering` 约在 `23.4s`
- recovery：
  首次回到 `stable` 约在 `24.4s`
- recovery：
  首次执行到 `L0` 约在 `27.9s`

也就是说，修正后的行为更接近我们希望的样子：

- 在极端坏网下，系统仍会打到重保护
- 网络恢复后，不必死等 `jitterEwma` 完全压到最终 stable 阈值以下才开始动
- 而且 recovery 尾段不会再因为连续提档过快而轻易反复回落

### 6.2 `2026-04-13` 的第二轮 recovery fast-path 修正

在上一轮修正后，`T9` 虽然已经能 `PASS`，但 recovery 仍然偏慢：

- `firstRecovering ≈ 24.152s`
- `firstStable ≈ 25.652s`
- `firstL0 ≈ 28.652s`

这轮继续做的不是“放宽 steady-state jitter 语义”，而是只动 recovery path：

1. `congested -> recovering` 增加 raw-jitter fast-path
   当 `loss/rtt/bandwidth` 都已经恢复、只是 `jitterEwma` 尾巴还在拖时，
   允许用更快的 raw jitter 观测启动 recovery。

2. recovery probe 改为支持动态 `2/3 sample`
   第一次离开 `L4` 仍然保守；
   但如果前一个 recovery probe 成功且信号足够干净，
   下一次 probe 可以用 `2 sample` 成功，而不是固定 `3 sample`。

这轮对应的代码点主要是：

- [stateMachine.js](/root/mediasoup-cpp/src/client/lib/qos/stateMachine.js)
  - 新增 `isFastRecoveryHealthy()`
  - `QosStateMachineContext` 增加 `consecutiveFastRecoverySamples`
- [probe.js](/root/mediasoup-cpp/src/client/lib/qos/probe.js)
  - probe 成功/失败 sample 数改为可配置
- [controller.js](/root/mediasoup-cpp/src/client/lib/qos/controller.js)
  - 新增 `isStrongRecoverySignal()`
  - recovery chain 连续成功后，对下一次 probe 启用更快的 `2 sample` 成功门槛

本次直接执行通过的单测是：

- `lib/test/test.qos.stateMachine.js`
- `lib/test/test.qos.probe.js`
- `lib/test/test.qos.controller.js`

本轮重新执行 `T9` targeted 的命令是：

```bash
node tests/qos_harness/run_matrix.mjs --cases=T9
node tests/qos_harness/render_case_report.mjs --input=docs/generated/uplink-qos-matrix-report.targeted.json --output=docs/generated/uplink-qos-case-results.targeted.md
```

latest targeted artifact 为：

- [最新 targeted case report](/root/mediasoup-cpp/docs/generated/uplink-qos-case-results.targeted.md)
- [最新 targeted matrix json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json)

这次的关键结果变成：

- `firstRecovering = 17.848s`
- `firstStable = 20.349s`
- `firstL0 = 22.349s`

和上一轮相比，大约改善了：

- `firstRecovering`: `-6.304s`
- `firstStable`: `-5.303s`
- `firstL0`: `-6.303s`

而且 latest targeted 仍然保持：

- `impairment peak = congested/L4`
- `recovery best = stable/L0`
- `recovery current = stable/L0`
- `verdict = PASS`

从 latest report 里还能直接看到一个更重要的结论：

- raw `jitter<28ms` 约在 `17.348s`
- `firstRecovering` 约在 `17.848s`

也就是说，这轮修正之后，状态机开始 recovery 的时刻已经基本贴近 raw jitter 恢复点，
主要瓶颈不再是“迟迟进不了 recovering”，而是 recovery chain 后半段的逐级提档本身。

### 6.3 组合 targeted regression 的结果

为了确认这轮 recovery fast-path 不是只对 isolated `T9` 有利，
随后又执行了一轮组合 targeted regression：

```bash
./scripts/run_qos_tests.sh matrix --matrix-cases=T9,T10,T11,J3,J4,J5,BW2,T1,S4 --matrix-include-extended
```

对应 artifact 为：

- [组合 targeted case report](/root/mediasoup-cpp/docs/generated/uplink-qos-case-results.targeted.md)
- [组合 targeted matrix json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json)

这轮的关键信息是：

- `9 / 9 PASS`
- `J3 / J4 / J5` 没有被 recovery fast-path 带弱
- `BW2 / T1 / S4` 这轮也都通过

blind-spot 三个 case 的恢复时间大致是：

- `T9`
  `firstRecovering=20.049s`
  `firstStable=22.549s`
  `firstL0=24.549s`
- `T10`
  `firstRecovering=19.872s`
  `firstStable=22.372s`
  `firstL0=24.372s`
- `T11`
  `firstRecovering=19.061s`
  `firstStable=21.561s`
  `firstL0=23.561s`

但这轮也暴露出 recovery 的下一个问题：

- `T10 / T11`
  recovery window 内 `best=stable/L0`
  但 case 结束时 `current=early_warning/L1`

也就是说，第二轮 fast-path 目前已经证明：

- 恢复启动可以更快
- steady-state jitter 护栏没有被明显破坏

但还没有完全解决：

- recovery tail 的最终收尾稳定性

### 6.4 tail-fix targeted rerun 的结果

在继续针对 `T10 / T11` 的 recovery tail 做小范围修正后，
又单独执行了一轮 targeted rerun：

```bash
node tests/qos_harness/run_matrix.mjs --cases=T10,T11,J3,J4,J5
node tests/qos_harness/render_case_report.mjs --input=docs/generated/uplink-qos-matrix-report.targeted.json --output=docs/generated/uplink-qos-case-results.targeted.md
```

这轮 artifact 的 `generatedAt` 为：

- `2026-04-13T00:57:28.724Z`

关键结果是：

- `5 / 5 PASS`
- `T10`
  `best=stable/L0`
  `current=stable/L0`
- `T11`
  `best=stable/L0`
  `current=stable/L0`

也就是说，上一轮组合 targeted regression 中出现的：

- `best=stable/L0`
- 但 case 结束时 `current=early_warning/L1`

这一类 tail oscillation，在 latest `T10 / T11` targeted rerun 中没有再复现。

这轮还有两个补充观察：

- `J3`
  仍为 `early_warning/L1`
- `J5`
  仍为 `congested/L4`
- `J4`
  这轮落在 `early_warning/L1`
  没有复现上一轮 `congested/L4`

因此更稳妥的解读是：

- tail-fix 对 `T10 / T11` 当前是有效的
- `J3 / J5` 的 steady-state 语义保持住了
- `J4` 仍然存在 loopback runner 级别的可变性，但结果仍落在 case contract 允许区间内

### 6.5 `BW2,T9,T10,T11` 复核结果

随后又执行了一轮更小的 targeted rerun：

```bash
./scripts/run_qos_tests.sh matrix --matrix-cases=BW2,T9,T10,T11 --matrix-include-extended
```

对应 artifact 为：

- [latest targeted case report](/root/mediasoup-cpp/docs/generated/uplink-qos-case-results.targeted.md)
- [latest targeted matrix json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json)

这轮的关键信息是：

- `4` 个 case 中 `3 PASS / 1 FAIL`
- `BW2 = FAIL`
- `T9 / T10 / T11 = PASS`

其中：

- `BW2`
  再次出现 `impairment peak=congested/L4`
  `best=stable/L0`
  但 strict default expectation 下仍判 `FAIL`
- `T10`
  `best=stable/L0`
  `current=stable/L0`
- `T11`
  `best=stable/L0`
  `current=stable/L0`

这说明：

- `BW2` 目前依然是典型的 mixed-evidence boundary case；
  不能因为上一轮组合 targeted 通过，就把它视作已经稳定收敛
- `T10 / T11` 的 tail oscillation 在这轮没有再复现

但 `T11` 仍然明显慢于 `T9 / T10`，而且慢点已经可以从 raw recovery diagnostics 看出来：

- `T9`
  `jitter<28ms ≈ 17.616s`
  `target>=120kbps ≈ 13.616s`
  `firstRecovering ≈ 18.116s`
- `T10`
  `jitter<28ms ≈ 19.371s`
  `target>=120kbps ≈ 12.871s`
  `firstRecovering ≈ 19.871s`
- `T11`
  `jitter<28ms ≈ 23.181s`
  `target>=120kbps ≈ 17.181s`
  `firstRecovering ≈ 23.681s`

也就是说，当前 `T11` 比 `T9 / T10` 更慢，不再主要像状态机迟迟不放行，
而更像是：

- raw jitter 自身恢复更慢
- 浏览器 target bitrate 回升更晚

这也是为什么在 latest `T11` rerun 中：

- tail 已经能稳住
- 但整体恢复时间仍然偏长

## 7. 理论推导、首次实测、两轮修正后结果的对照

| 项目 | 理论推导 | 第一次 `T9` | 第一轮修正后 `T9` | 第二轮 fast-path 后 `T9` |
|---|---|---|---|---|
| baseline | `stable/L0` | `stable/L0` | `stable/L0` | `stable/L0` |
| 坏网期最终保护强度 | 应进入 `congested/L4` | `congested/L4` | `congested/L4` | `congested/L4` |
| `L4` 语义 | 更接近 `audio-only` | loopback keepalive video | loopback keepalive video | loopback keepalive video |
| recovery 是否能启动 | 理论上应能逐步启动 | 启动过慢，被卡住 | 已能启动 | 更早启动 |
| recovery window 内 best | 理论上应明显回升 | `congested/L4` | `stable/L0` | `stable/L0` |
| 结束时 current | 希望恢复 | `congested/L4` | `stable/L0` | `stable/L0` |
| 首次进入 `recovering` | 理论上应较早启动 | - | `24.152s` | `17.848s` |
| 首次回到 `stable` | 理论上应明显回升 | - | `25.652s` | `20.349s` |
| 首次回到 `L0` | 理论上应逐级回升 | - | `28.652s` | `22.349s` |
| 最终判断 | 希望恢复 | `FAIL` | `PASS` | `PASS` |

## 8. 当前建议口径

建议把这个专项场景对内表述为：

`我们已经把“高质量网络突入长时移动盲区再恢复”的极端场景正式纳入默认 matrix。初始版本的 T9 暴露出 recovery 过慢问题，第一轮修正解决了“能不能恢复”的问题，第二轮修正则继续解决“恢复开始得太慢”的问题。当前策略是：steady-state jitter 语义保持不变，recovery path 单独使用更快的 raw-jitter fast-path，并在 recovery probe 连续成功时启用更快的 2-sample probe。latest T9 targeted rerun 已把 firstRecovering / firstStable / firstL0 分别拉到约 17.8s / 20.3s / 22.3s，同时仍保持 stable/L0 收尾。`

## 9. 后续建议

建议后续按下面顺序继续推进：

1. 继续复核 `T10 / T11`
   当前第二轮 fast-path 只实测了 `T9`；
   还需要看 `15Mbps / 30Mbps` baseline 下是否也能稳定受益，而不是引入新的尾部波动。

2. 回归 `J3 / J4 / J5`
   确认这轮 recovery-only 优化没有把 steady-state jitter 护栏变钝。

3. 回归 `BW2 / T1 / S4`
   确认 recovery fast-path 没有把已知 loopback boundary case 的波动进一步放大。

4. 增加更贴近现实的 `40s` blind-spot case
   因为 `40s` 在真实移动网络里比 `100s` 更常见。

5. 把更激进的后续变体继续保留在 `extended` 集中
   当前 `T9 / T10 / T11` 已进入默认集合，但如果后续再加更长 recovery window 或更夸张 baseline，仍建议先放 `extended`。

6. 如果后续还要进一步提高贴近生产的可信度，建议新增：
   browser -> real SFU + netem 的长时转场 runner。
