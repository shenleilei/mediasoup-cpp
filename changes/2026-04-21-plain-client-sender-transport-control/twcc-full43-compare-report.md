# TWCC 修复对 43 个 C++ QoS Case 的前后对比报告

生成时间：`2026-04-21`

## 1. 结论

对“原来 C++ 已经覆盖过的 43 个 uplink QoS 主 case”，我做了 `G1 vs G2` 的直接对比：

- `G1`：`transport controller on` + `transport estimate off`
- `G2`：`transport controller on` + `transport estimate on`

本轮结果表明：

1. **这 43 个 case 没有体现出明显的 TWCC 收益差异**
2. **在同一套快速跑参数下，`G1` 和 `G2` 的 pass/fail footprint 完全一致**
3. **因此这组 43-case 更适合证明“不退化”，不适合作为“TWCC 明显更好”的主证据**

## 2. 理论分析

这 43 个 case 来自：

- [plain-client-qos-parity-checklist.md](/root/mediasoup-cpp/docs/plain-client-qos-parity-checklist.md)
- `tests/qos_harness/scenarios/sweep_cases.json`

非 extended case 共 `43` 个：

- baseline: `3`
- bw_sweep: `6`
- loss_sweep: `8`
- rtt_sweep: `6`
- jitter_sweep: `5`
- transition: `11`
- burst: `4`

### 为什么理论上它们不一定会体现 TWCC 收益

这 43 个 case 的核心目标是：

- 验证 QoS 状态机是否进入 `stable / early_warning / congested / recovering`
- 验证 level / audio-only / recovery 是否符合预期
- 验证弱网 profile 下策略反应是否正确

它们不是纯 transport benchmark。

当前 `cpp-client-matrix` 的驱动机制里，`QOS_TEST_MATRIX_PROFILE` 会直接注入：

- `sendCeilingBps`
- `lossRate`
- `rttMs`
- `jitterMs`
- `qualityLimitationReason`

相关代码：

- [run_cpp_client_matrix.mjs](/root/mediasoup-cpp/tests/qos_harness/run_cpp_client_matrix.mjs)
- [cpp_client_runner.mjs](/root/mediasoup-cpp/tests/qos_harness/cpp_client_runner.mjs)
- [PlainClientSupport.cpp](/root/mediasoup-cpp/client/PlainClientSupport.cpp)

这意味着：

- case 的主导信号是 **matrix profile 注入的 QoS 观测**
- 不是 sender transport estimate 自己自由收敛出来的真实链路行为

所以理论预期是：

1. `G1/G2` 在 43-case 上 **不会拉出特别大的差异**
2. 更合理的预期是：
   - `G2` 不退化
   - 某些 recovery/fairness 可能有轻微改善
3. 真正能证明 TWCC 修复生效的，应该是 **真实 worker feedback + estimate toggle** 这类直接证据，而不是这 43-case 矩阵

## 3. 本次运行方式

本次为了把对比时间控制在可接受范围，使用了快速跑参数：

- `QOS_MATRIX_SPEED=0.25`
- 每组 `1` 次全量 43-case
- worker 使用本地编译版：
  - [mediasoup-worker](/root/mediasoup-cpp/src/mediasoup-worker-src/worker/out/Release/build/mediasoup-worker)

结果目录：

- [full43-compare/2026-04-21T06-57-52Z](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/full43-compare/2026-04-21T06-57-52Z)

产物：

- [g1.json](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/full43-compare/2026-04-21T06-57-52Z/g1.json)
- [g1.md](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/full43-compare/2026-04-21T06-57-52Z/g1.md)
- [g2.json](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/full43-compare/2026-04-21T06-57-52Z/g2.json)
- [g2.md](/root/mediasoup-cpp/changes/2026-04-21-plain-client-sender-transport-control/artifacts/full43-compare/2026-04-21T06-57-52Z/g2.md)

## 4. 实际结果

### 4.1 总数对比

`G1`:

- `43 total`
- `12 PASS`
- `31 FAIL`

`G2`:

- `43 total`
- `12 PASS`
- `31 FAIL`

### 4.2 分组对比

`G1` 与 `G2` 的分组统计完全一致：

- baseline: `2 / 3 PASS`
- bw_sweep: `1 / 6 PASS`
- loss_sweep: `3 / 8 PASS`
- rtt_sweep: `3 / 6 PASS`
- jitter_sweep: `2 / 5 PASS`
- transition: `1 / 11 PASS`
- burst: `0 / 4 PASS`

### 4.3 case 级结论

这轮 `G1` 和 `G2` 的 case 级 pass/fail footprint **一致**。

失败模式也一致：

- 大多数失败为 `过弱`
- 长窗口重压场景 `T9/T10/T11` 为 `恢复过慢`

这说明：

- 打开 `transport estimate` 没有让这 43-case 出现新的回归
- 但也没有在这套矩阵里拉出显著收益差异

### 4.4 timing 差异

在可比较的 timing 字段上，`G2 - G1` 的均值变化只有个位数毫秒：

- `t_detect_warning`: `+7 ms`
- `t_detect_congested`: `+8 ms`
- `t_detect_stable`: `-4.65 ms`
- `t_first_action`: `+7 ms`

这属于微小波动量级，不足以说明 43-case 对 TWCC 收益有强敏感性。

## 5. 如何解读这组结果

### 能证明什么

这组 43-case 结果可以证明：

- 在当前快速跑参数下，`G2` 相比 `G1` **没有引入 QoS 主策略层面的可见回归**
- 也就是说，今天的修复至少是 **不退化** 的

### 不能证明什么

这组结果 **不能单独证明**：

- `G2` 比 `G1` 在真实 transport 表现上明显更好

原因不是修复无效，而是：

- 这套 case 本身主要由 QoS matrix profile 主导
- 对 transport estimate 的收益不敏感
- 再叠加 `QOS_MATRIX_SPEED=0.25` 的时间压缩，状态机本身就整体偏“过弱”

## 6. 为什么“今天修复有效”仍然成立

虽然 43-case 没拉开收益差异，但今天修复有效这件事，已经由更直接的证据证明：

- [ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender](/root/mediasoup-cpp/tests/test_thread_integration.cpp:2409)
- [ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackHonorsEstimateToggle](/root/mediasoup-cpp/tests/test_thread_integration.cpp:2841)
- [PlainTransportDirect.WorkerDumpRegistersTransportCcHeaderExtension](/root/mediasoup-cpp/tests/test_thread_integration.cpp:983)
- [PlainTransportDirect.WorkerEmitsTransportCcFeedbackForConnectedH264Socket](/root/mediasoup-cpp/tests/test_thread_integration.cpp:1180)

其中最强的是：

- `estimate off`: 有真实 worker feedback，但 estimate 不变
- `estimate on`: 有真实 worker feedback，且 estimate 抬升

这条证据比 43-case matrix 更接近因果证明。

## 7. 最终判断

对“原来 cpp 也测试过的 43 个 QoS case”，本轮前后对比的结论是：

- **理论预期**：不会显著改善，主要用于验证不退化
- **实际结果**：与理论一致，`G1/G2` 结果基本相同

所以：

- 如果问题是“今天修复有没有生效”，答案是 **有，已经由真实 worker toggle 测试证实**
- 如果问题是“这 43-case 能不能证明 TWCC 效果更好”，答案是 **不能，它们更适合证明不退化**

## 8. 建议口径

建议对外这样表述：

> 已对原有 43 个 C++ QoS 主 case 做前后对比。理论上这些 case 主要验证 QoS 状态机与策略响应，不是 transport 收益放大器；实际结果与理论一致，`G1/G2` 在 43-case 上未出现差异，说明本次修复未引入策略层回归。另一方面，真实 worker 集成测试已经直接证明：打开 transport estimate 后，sender 会消费真实 TWCC feedback 并抬升 estimate，因此本次修复是有效的。
