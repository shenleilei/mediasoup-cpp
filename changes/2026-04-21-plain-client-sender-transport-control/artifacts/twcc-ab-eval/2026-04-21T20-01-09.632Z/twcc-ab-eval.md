# TWCC A/B 最新报告

> 文档性质
>
> 这是 stable TWCC A/B 总入口。
> 它的目标不是替代 pairwise 明细，而是先回答“本次在比什么、怎么读、这次结果大致意味着什么”。

生成时间：`2026-04-21T20:01:09.632Z`

## 1. 先看这里

- 如果只关心 transport estimate / TWCC 主路径本身的净收益，先看 `G1 vs G2`。
- 如果只关心新整条发送路径相对旧路径的整体效果，先看 `G0 vs G2`。
- overall `FAIL` 的意思是“没有满足全部预设 gate”，不自动等于运行路径已经坏了。
- 读法建议：先看本页 Pairwise 结果，再进入对应 pairwise 报告看 gate 和场景级结果。

## 2. 这次运行在比较什么

| 组 | 运行模式 | 关键开关 | 作用 |
|---|---|---|---|
| `G0` | 旧发送路径基线 | `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0` | 作为 legacy path 基线，看整条新发送路径相对旧路径的整体效果。 |
| `G1` | 新发送路径，但关闭 transport estimate | `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`<br>`PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=0` | 作为 controller-only 基线，用来隔离 transport estimate / TWCC 主路径本身的净收益。 |
| `G2` | 新发送路径，并打开 transport estimate | `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`<br>`PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1` | 当前 candidate 路径，包含 transport controller 与 send-side estimate 主路径。 |

## 3. 本次运行

- 脚本：`node tests/qos_harness/run_twcc_ab_eval.mjs`
- 重复次数：`1`
- matrix speed：`0.1`
- 场景：`AB-001,AB-002,AB-003,AB-004,AB-005`
- Host：`iZbp16494z0s9fdwtwq6pgZ`
- OS：`linux 5.10.134-18.al8.x86_64`
- CPU / Memory：`Intel(R) Xeon(R) Platinum` / `3.5` GB

## 4. Pairwise 结果

| Pair | 这组回答什么 | Overall | 这次意味着什么 | Markdown | JSON |
|---|---|---|---|---|---|
| G1 Controller-Only vs G2 Candidate | 这组对比用来隔离 transport estimate / TWCC 主路径本身的净收益。 | `FAIL` | 未通过全部 gate；goodput 基本持平；recovery 变慢 7.59%；阻塞项：AB-002 congestion loss proxy improves >=20%；AB-003 recovery time not worse, ideally >=15%。 | [g1-vs-g2.md](g1-vs-g2.md) | [g1-vs-g2.json](g1-vs-g2.json) |
| G0 Legacy vs G2 Candidate | 这组对比用来看新整条发送路径相对旧路径的整体效果。 | `FAIL` | 未通过全部 gate；goodput 基本持平；recovery 改善 9.96%；阻塞项：AB-002 congestion loss proxy improves >=20%。 | [g0-vs-g2.md](g0-vs-g2.md) | [g0-vs-g2.json](g0-vs-g2.json) |

## 5. 本次结果怎么理解

### G1 Controller-Only vs G2 Candidate

- 这次 pairwise 对比没有通过全部预设 gate；这表示 candidate 还没有在当前门槛下“全面胜出”，不自动等于发送路径损坏。
- Baseline 与 Candidate 使用同一提交，比较的是运行模式，不是两份不同代码。
- 这组对比用来隔离 transport estimate / TWCC 主路径本身的净收益。
- 总体 goodput 基本持平（0.00%）。 恢复时间变慢 7.59%（395.00ms -> 425.00ms）。 多轨公平性基本持平（权重偏差变化 0.00%）。
- 阻塞项：AB-002 congestion loss proxy improves >=20%（目标 >=20%，实际 n/a）；AB-003 recovery time not worse, ideally >=15%（目标 not worse, ideally >=15%，实际 -7.59%）。

### G0 Legacy vs G2 Candidate

- 这次 pairwise 对比没有通过全部预设 gate；这表示 candidate 还没有在当前门槛下“全面胜出”，不自动等于发送路径损坏。
- Baseline 与 Candidate 使用同一提交，比较的是运行模式，不是两份不同代码。
- 这组对比用来看新整条发送路径相对旧路径的整体效果。
- 总体 goodput 基本持平（0.00%）。 恢复时间更快，改善 9.96%（472.00ms -> 425.00ms）。 多轨公平性基本持平（权重偏差变化 0.00%）。
- 阻塞项：AB-002 congestion loss proxy improves >=20%（目标 >=20%，实际 n/a）。

## 6. 推荐阅读顺序

1. 先看本页 `Pairwise 结果`，确定要看哪一组对比。
2. 再看对应 pairwise Markdown 里的 `判定门槛检查` 和 `场景级结果`。
3. 只有在需要定位具体原因时，才继续下钻 raw JSON 与 trace/log。

## 7. 原始产物

- [raw-groups.json](raw-groups.json)
- 原始 trace/log 目录：[changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw](raw)
