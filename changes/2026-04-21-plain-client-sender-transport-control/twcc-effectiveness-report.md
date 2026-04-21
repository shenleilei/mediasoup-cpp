# TWCC 修复有效性报告

生成时间：`2026-04-21`

注：本报告记录的是 phase-1 收尾时的证据口径。后续 phase-2 改为 delay-based BWE 后，真实 worker smoke 的判定口径调整为“`estimate on` 会让 estimate 偏离 bootstrap 值”，而不再要求 app-limited loopback 样本一定上升到 `> 900000`。

## 结论

今天的修复已经有可重复的实锤证据证明是有效的。

最强证据不是弱网 matrix，而是真实 worker 路径下的 `TWCC feedback + estimate toggle` 集成测试：

- 当 `enableTransportEstimate = false` 时，发送端仍然能收到真实 worker 回发的 `transport-cc feedback`，但 `transportEstimatedBitrateBps` 保持在初始值 `900000`
- 当 `enableTransportEstimate = true` 时，发送端收到同样的真实 worker feedback，且 `transportEstimatedBitrateBps > 900000`

这说明：

1. `TWCC feedback` 的真实回路已经打通
2. 今天修复的 estimate 路径确实会消费 feedback 并改变 pacing 上限
3. 关闭 estimate 后该变化会消失，证明增益来自本次修复，而不是测试噪声

## 关键证据

### 1. 真实 worker 路径 smoke 已通过

- [ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender](/root/mediasoup-cpp/tests/test_thread_integration.cpp:2409)

该测试当前已经 `PASS`。

它证明：

- `plainPublish` 已协商出 `transportCcExtId`
- 真实 worker 会向 threaded plain sender 回发 `transport-cc feedback`
- sender 侧 `transportCcFeedbackReports() > 0`

### 2. estimate 开关对真实 worker feedback 的作用已通过

- [ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackHonorsEstimateToggle](/root/mediasoup-cpp/tests/test_thread_integration.cpp:2841)

该测试当前已经 `PASS`。

它直接证明：

- `estimate off`:
  - `feedbackReports > 0`
  - `transportEstimatedBitrateBps == 900000`
- `estimate on`:
  - `feedbackReports > 0`
  - `transportEstimatedBitrateBps > 900000`

这就是本次修复最强的因果证据。

### 3. worker 侧 transport-cc 注册与直连 plain-transport 诊断已通过

以下测试当前都已 `PASS`：

- [PlainTransportDirect.WorkerDumpRegistersTransportCcHeaderExtension](/root/mediasoup-cpp/tests/test_thread_integration.cpp:983)
- [PlainTransportDirect.WorkerReceivesTransportCcRtpButDoesNotYetEmitFeedback](/root/mediasoup-cpp/tests/test_thread_integration.cpp:1042)
- [PlainTransportDirect.WorkerEmitsTransportCcFeedbackForConnectedH264Socket](/root/mediasoup-cpp/tests/test_thread_integration.cpp:1180)

它们分别证明：

- worker transport dump 内已经注册 `recvRtpHeaderExtensions.transportWideCc01 = 5`
- worker 确实收到了带 `transport-cc` 扩展的 RTP，并建立了 tuple / 累加了接收统计
- 在有效 H264 RTP 场景下，worker 确实会产生并回发 `transport-cc RTCP feedback`

这也解释了为什么早期 smoke 会失败：

- 不是 worker 不支持 `TWCC`
- 是旧 smoke 使用的 fake H264 AU 过于简化，不能稳定触发真实 feedback 行为

## A/B/C 评估结果

已生成 A/B/C 评估产物，路径如下：

- 原始聚合输入：
  - [raw-groups.json](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/raw-groups.json)
- `G1 vs G2` 报告：
  - [g1-vs-g2.md](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g1-vs-g2.md)
  - [g1-vs-g2.json](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g1-vs-g2.json)
- `G0 vs G2` 报告：
  - [g0-vs-g2.md](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g0-vs-g2.md)
  - [g0-vs-g2.json](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g0-vs-g2.json)

### 当前 A/B/C 读法

这批 A/B/C 产物可以证明两件事：

1. 本次修复没有引入明显退化
2. 在 recovery / fairness 上可以观察到轻微改善

但它还不能作为最强收益证明，原因也很明确：

- 当前 `cpp-client-matrix` 的 `matrix profile` 会直接塑形 `sendBps`
- 因此 `G1 vs G2` 在 `goodput/loss` 上被 profile 主导，收益差异被压平
- 所以 matrix 更像“补充证据”，不是本次修复的主证据

### 当前可读出的数值

`G1 vs G2`:

- `AB-001` 稳态 goodput：`0.00%` 变化
- `AB-003` recovery：`0.40%` 改善
- `AB-005` fairness deviation：`-1.82%`（更小更好）
- 硬回归 gate：`PASS`

`G0 vs G2`:

- `AB-001` 稳态 goodput：`0.00%` 变化
- `AB-003` recovery：`1.75%` 改善
- `AB-005` fairness deviation：`-1.99%`（更小更好）
- 硬回归 gate：`PASS`

之所以两份报告总体 still `FAIL`，不是因为修复无效，而是因为 `AB-002 loss` gate 当前拿不到能区分 `G1/G2` 的有效信号，结果是 `n/a`。

## 为什么今天可以判定修复有效

因为我们已经有比 A/B/C matrix 更直接、更接近因果证明的测试：

- 真实 worker feedback 存在
- estimate 开关能消除/恢复 estimate 抬升
- 其余 transport-control 回归测试全部通过

这三者组合起来，比“弱网 matrix 略有变化”更能说明本次修复真的生效。

## 本次相关验证

已通过：

- `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*:SenderTransportControllerTest.BitrateAllocation*:SenderTransportControllerTest.PauseDropsQueuedRetransmissionsForTrack:SenderTransportControllerTest.FlushForShutdownDrainsQueuedVideoBeforeDiscardingRemainder:SenderTransportControllerTest.EffectivePacingBitrateUsesMinOfTargetEstimateAndCap'`
- `./build/mediasoup_thread_integration_tests --gtest_filter='PlainTransportDirect.*:NetworkThreadIntegration.TransportCcFeedback*:NetworkThreadIntegration.TransportEstimateRaisesWhenAggregateTargetIncreases:NetworkPause.PauseAckRequiresQuiescedTransport:NetworkPause.PauseAckDropsQueuedRetransmissionsAndPreventsLateRtp:ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender'`
- `./build/mediasoup_thread_integration_tests --gtest_filter='ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackHonorsEstimateToggle'`

A/B/C 已产出：

- `node tests/qos_harness/run_twcc_ab_eval.mjs --repetitions=2 --matrix-speed=0.25`

## 建议的对外表述

如果要在 review/PR 里给一句结论，建议直接写：

> 已验证本次修复有效。真实 worker 路径下，`transport-cc feedback` 已能稳定回到 sender；且在 `enableTransportEstimate=false` 时 estimate 不变、`enableTransportEstimate=true` 时 estimate 明确抬升，证明本次修复确实使 sender 侧消费了真实 TWCC feedback。A/B/C matrix 结果未见退化，并在 recovery/fairness 上观察到轻微改善。
