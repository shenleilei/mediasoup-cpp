# TWCC A/B 测试说明

## 1. 目的

`tests/qos_harness/run_twcc_ab_eval.mjs` 用来评估 plain-client 发送侧传输控制改动的有效性。

它回答的是两类问题：

- 新的 send-side transport-control / TWCC estimate 路径是否比旧路径更好
- 新路径是否在稳态、拥塞、恢复和多轨场景里引入明显退化

这个测试不是协议连通性 smoke。它做的是一组可重复的 A/B 对照实验，并输出 pairwise 报告。

## 2. 分组

脚本固定生成 3 组运行配置：

- `G0`：旧发送路径基线
  - `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0`
- `G1`：新发送路径，但关闭 transport estimate
  - `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`
  - `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=0`
- `G2`：新发送路径，打开 transport estimate
  - `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`
  - `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1`

脚本最终输出两份主要 pairwise 报告：

- `G1 vs G2`
  - 用来隔离 transport estimate / TWCC 主路径本身的净收益
- `G0 vs G2`
  - 用来看新整条发送路径相对旧路径的整体效果

## 3. 场景

当前 A/B 场景定义如下：

- `AB-001`
  - 稳态场景
  - 来源：`B1` 的 `baseline` phase
  - 目标：验证稳态无明显退化
- `AB-002`
  - 带宽骤降 / 拥塞阶段
  - 来源：`T3` 的 `impairment` phase
  - 目标：验证拥塞期 loss / queue pressure 表现
- `AB-003`
  - 带宽恢复阶段
  - 来源：`T3` 的 `recovery` phase
  - 目标：验证恢复速度
- `AB-004`
  - 高 RTT 稳定性
  - 来源：`R4` 的 `impairment` phase
  - 目标：验证高时延下稳定性
- `AB-005`
  - 多视频轨公平性
  - 来源：`multi_video_budget` threaded harness
  - 目标：验证预算分配与公平性

## 4. 指标

脚本同时输出黑盒和白盒指标。

黑盒指标：

- `goodput`
- `loss`
- `recovery time`
- `stability`
- `fairness deviation`

白盒指标：

- `senderUsageBps`
- `transportEstimateBps`
- `effectivePacingBps`
- `feedbackReports`
- `probePackets`
- `probeActive`
- `probeClusterStarts`
- `probeClusterCompletes`
- `probeClusterEarlyStops`
- `probeBytesSent`
- `wouldBlockTotal`
- `queuedVideoRetentions`
- `audioDeadlineDrops`
- `retransmissionDrops`
- `retransmissionSent`
- `queuedFreshVideoPackets`
- `queuedAudioPackets`
- `queuedRetransmissionPackets`

## 5. 产物

每次运行会同时写两类产物。

时间戳原始产物：

- `changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/<timestamp>/`
- 这里保留该次运行的：
  - `raw-groups.json`
  - `g1-vs-g2.json`
  - `g1-vs-g2.md`
  - `g0-vs-g2.json`
  - `g0-vs-g2.md`
  - `twcc-ab-eval.md`

稳定文档入口：

- `docs/generated/twcc-ab-report.md`
- `docs/generated/twcc-ab-g1-vs-g2.md`
- `docs/generated/twcc-ab-g1-vs-g2.json`
- `docs/generated/twcc-ab-g0-vs-g2.md`
- `docs/generated/twcc-ab-g0-vs-g2.json`
- `docs/generated/twcc-ab-raw-groups.json`

其中：

- `docs/generated/twcc-ab-report.md` 是最新稳定入口
- 时间戳目录下的 `twcc-ab-eval.md` 是该次运行自己的索引页

## 6. 默认入口

默认全量 QoS 入口现在已经包含这个 A/B 步骤：

- `scripts/run_qos_tests.sh all`
- `scripts/run_all_tests.sh`

具体接法是：

- `scripts/run_qos_tests.sh` 新增了 `cpp-client-ab` 分组
- 这个分组默认包含在 QoS 全量集合里
- `scripts/run_all_tests.sh` 的 `qos` 组会委托到 `scripts/run_qos_tests.sh all`
- 因此默认 full regression 会刷新 `docs/generated/twcc-ab-report.md`

`docs/full-regression-test-results.md` 也会链接到最新稳定 A/B 报告。

## 7. 默认运行参数

通过 `run_qos_tests.sh` / `run_all_tests.sh` 触发时，默认参数是：

- `TWCC_AB_REPETITIONS=1`
- `TWCC_AB_MATRIX_SPEED=0.1`
- `TWCC_AB_CASES=AB-001,AB-002,AB-003,AB-004,AB-005`

可以通过环境变量覆盖，例如：

```bash
TWCC_AB_REPETITIONS=2 \
TWCC_AB_MATRIX_SPEED=0.25 \
TWCC_AB_CASES=AB-001,AB-003,AB-005 \
./scripts/run_qos_tests.sh cpp-client-ab
```

如果只想单独刷新这份报告，可以直接运行：

```bash
./scripts/run_qos_tests.sh cpp-client-ab
```

## 8. 使用建议

- 看最新结果，优先打开 `docs/generated/twcc-ab-report.md`
- 看具体 pairwise 明细，再进入 `docs/generated/twcc-ab-g1-vs-g2.md` 或 `docs/generated/twcc-ab-g0-vs-g2.md`
- 要定位单次运行原始 trace / raw metrics，再进入时间戳目录下的 `raw-groups.json`
