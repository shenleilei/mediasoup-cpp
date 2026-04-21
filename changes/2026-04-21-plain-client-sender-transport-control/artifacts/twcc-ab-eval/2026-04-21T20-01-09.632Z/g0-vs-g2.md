# TWCC A/B 有效性评估结果

> 文档性质
>
> 这是一份 pairwise TWCC A/B 报告。
> 它回答的是：在当前一组预设 gate 下，candidate runtime mode 相对 baseline runtime mode 是否足够好。
>
> 这里的 overall `FAIL` 只表示“至少一条 gate 没过”，
> 不自动等于 “TWCC 路径已经坏了” 或 “功能不可用”。

## 1. 先看这里

- 这次 pairwise 对比没有通过全部预设 gate；这表示 candidate 还没有在当前门槛下“全面胜出”，不自动等于发送路径损坏。
- Baseline 与 Candidate 使用同一提交，比较的是运行模式，不是两份不同代码。
- 这组对比用来看新整条发送路径相对旧路径的整体效果。
- 总体 goodput 基本持平（0.00%）。 恢复时间更快，改善 9.96%（472.00ms -> 425.00ms）。 多轨公平性基本持平（权重偏差变化 0.00%）。
- 阻塞项：AB-002 congestion loss proxy improves >=20%（目标 >=20%，实际 n/a）。

## 2. 比较对象

| 角色 | 标签 | 运行模式 | 关键开关 |
|---|---|---|---|
| Baseline | `G0 Legacy` | 旧发送路径基线 | `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0` |
| Candidate | `G2 Candidate` | 新发送路径，并打开 transport estimate | `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`<br>`PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1` |

- 这组对比回答：G0 Legacy -> G2 Candidate 是否满足当前 TWCC A/B 门槛。
- Baseline 侧重点：作为 legacy path 基线，看整条新发送路径相对旧路径的整体效果。
- Candidate 侧重点：当前 candidate 路径，包含 transport controller 与 send-side estimate 主路径。

## 3. 怎么读这份报告

1. 先看“先看这里”和“汇总”，判断这次是 gate 没过，还是出现了明显的吞吐/恢复退化。
2. 再看“判定门槛检查”，它直接解释 overall `PASS` / `FAIL` 的原因。
3. 然后看“场景级结果”，判断问题主要落在稳态、拥塞、恢复还是多轨公平性。
4. “逐场景明细”里的白盒字段只在定位原因时再看，不是第一次阅读的入口。

## 4. 实验信息

| 字段 | 内容 |
|---|---|
| Baseline 版本 | `ae2994c62c3141532f432003f8f445e53cc419e4` |
| Candidate 版本 | `ae2994c62c3141532f432003f8f445e53cc419e4` |
| Host | `iZbp16494z0s9fdwtwq6pgZ` |
| OS / Kernel | `linux` / `5.10.134-18.al8.x86_64` |
| CPU / Memory | `Intel(R) Xeon(R) Platinum` / `3.5` GB |
| 网络注入 | tc/netem via cpp-client-matrix speed=0.1 |
| 输入素材 | tests/fixtures/media/test_sweep_cpp_matrix.mp4 + threaded multi_video_budget |
| 每场景重复次数 | 1 |
| 场景数量 | 5（PASS=5, FAIL=0） |
| 版本解读 | baseline 与 candidate 为同一提交；本次比较的是运行模式而不是两份不同代码 |

## 5. 场景在测什么

| 场景 | 含义 | 重点看什么 |
|---|---|---|
| AB-001 稳态 | 看 steady-state goodput 与抖动有没有明显退化。 | goodput / stability |
| AB-002 拥塞阶段 | 看带宽骤降时 loss proxy 和 queue pressure 是否改善。 | loss / queue pressure / feedback |
| AB-003 恢复阶段 | 看恢复到可用发送状态的速度是否更快，或至少不变差。 | recovery time |
| AB-004 高 RTT 稳定性 | 看高时延下是否保持稳定，不引入额外退化。 | goodput / stability |
| AB-005 多轨公平性 | 看多视频轨预算分配是否更偏，还是保持原有公平性。 | fairness deviation |

## 6. 汇总

| 指标 | Baseline | Candidate | 变化 |
|---|---:|---:|---:|
| 总体 Goodput (kbps) | 789.81 | 789.81 | 0.00% |
| 总体 Loss (%) | 0.00 | 0.00 | - |
| 恢复时间 (ms) | 472.00 | 425.00 | -9.96% |
| 稳态抖动 (P90-P50, kbps) | 0.00 | 0.00 | - |
| 队列压力（规范化总和） | 0.00 | 0.00 | - |
| 多路权重偏差（越小越好） | 0.24 | 0.24 | 0.00% |

结论：`FAIL`

说明：G2 Candidate did not meet every configured gate over G0 Legacy.

## 7. 判定门槛检查

| 规则 | 目标 | 实际 | 结论 |
|---|---|---|---|
| AB-002 congestion loss proxy improves >=20% | >=20% | n/a | `FAIL` |
| AB-003 recovery time not worse, ideally >=15% | not worse, ideally >=15% | 9.96% | `PASS` |
| AB-001 steady-state goodput regression <=5% | >=-5% | 0.00% | `PASS` |
| AB-005 fairness deviation not increased | <=0% | 0.00% | `PASS` |
| transport-control hard regression suite | all pass | pass | `PASS` |

## 8. 场景级结果

| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |
|---|---|---|---|---|---|
| AB-001 | bw 4000kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=880.65, loss=0.00, rec=-ms | gp=880.65, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-002 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=582.08, loss=0.00, rec=-ms | gp=582.08, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-003 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=880.65, loss=0.00, rec=472ms | gp=880.65, loss=0.00, rec=425ms | 0.00% | `PASS` |
| AB-004 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms | gp=815.85, loss=0.00, rec=-ms | gp=815.85, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-005 | bw 300kbps / rtt 260ms / loss 0.08 / jitter 24ms | gp=-, loss=-, rec=-ms | gp=-, loss=-, rec=-ms | - | `PASS` |

## 9. 逐场景明细

### AB-001

| 字段 | 内容 |
|---|---|
| 场景名 | steady_state |
| 网络形态 | bw 4000kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| 这场景在看什么 | 看 steady-state goodput 与抖动有没有明显退化。 |
| Baseline Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Candidate Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 白盒 Baseline | senderUsage=894368 / estimate=0 / pacing=0 / fb=- / probePkts=- / probeStarts=- / probeDone=- / probeEarlyStop=- / probeBytes=- / rtxSent=- / qFresh=0 / qAudio=0 / qRtx=0 / probeActive=no |
| 白盒 Candidate | senderUsage=765920 / estimate=764943 / pacing=764943 / fb=- / probePkts=- / probeStarts=- / probeDone=- / probeEarlyStop=- / probeBytes=- / rtxSent=- / qFresh=13 / qAudio=0 / qRtx=0 / probeActive=no |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=baseline, case=B1. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-002

| 字段 | 内容 |
|---|---|
| 场景名 | step_down_congestion |
| 网络形态 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| 这场景在看什么 | 看带宽骤降时 loss proxy 和 queue pressure 是否改善。 |
| Baseline Goodput (mean/p50/p90) | 582.08 / 582.08 / 582.08 kbps |
| Candidate Goodput (mean/p50/p90) | 582.08 / 582.08 / 582.08 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=1044832 / estimate=0 / pacing=0 / fb=3 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=0 / qAudio=0 / qRtx=0 / probeActive=no |
| 白盒 Candidate | senderUsage=900288 / estimate=970266 / pacing=900000 / fb=3 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=3 / qFresh=9 / qAudio=0 / qRtx=0 / probeActive=no |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=impairment, case=T3. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-003

| 字段 | 内容 |
|---|---|
| 场景名 | step_up_recovery |
| 网络形态 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| 这场景在看什么 | 看恢复到可用发送状态的速度是否更快，或至少不变差。 |
| Baseline Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Candidate Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | 472 / 472 / 472 ms |
| Candidate Recovery (mean/p50/p90) | 425 / 425 / 425 ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=888096 / estimate=0 / pacing=0 / fb=16 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=0 / qAudio=0 / qRtx=0 / probeActive=no |
| 白盒 Candidate | senderUsage=0 / estimate=970266 / pacing=900000 / fb=10 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=220 / qFresh=256 / qAudio=0 / qRtx=0 / probeActive=no |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / -9.96% / - / - |
| 结论 | `PASS` |
| 分析 | Phase=recovery, case=T3. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-004

| 字段 | 内容 |
|---|---|
| 场景名 | high_rtt_stability |
| 网络形态 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms |
| 这场景在看什么 | 看高时延下是否保持稳定，不引入额外退化。 |
| Baseline Goodput (mean/p50/p90) | 815.85 / 815.85 / 815.85 kbps |
| Candidate Goodput (mean/p50/p90) | 815.85 / 815.85 / 815.85 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=1030192 / estimate=0 / pacing=0 / fb=10 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=0 / qAudio=0 / qRtx=0 / probeActive=no |
| 白盒 Candidate | senderUsage=775744 / estimate=772211 / pacing=772211 / fb=10 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=2 / qFresh=88 / qAudio=0 / qRtx=0 / probeActive=no |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=impairment, case=R4. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-005

| 字段 | 内容 |
|---|---|
| 场景名 | multi_video_budget |
| 网络形态 | bw 300kbps / rtt 260ms / loss 0.08 / jitter 24ms |
| 这场景在看什么 | 看多视频轨预算分配是否更偏，还是保持原有公平性。 |
| Baseline Goodput (mean/p50/p90) | - / - / - kbps |
| Candidate Goodput (mean/p50/p90) | - / - / - kbps |
| Baseline Loss (mean/p50/p90) | - / - / - % |
| Candidate Loss (mean/p50/p90) | - / - / - % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 白盒 Baseline | senderUsage=- / estimate=- / pacing=- / fb=- / probePkts=- / probeStarts=- / probeDone=- / probeEarlyStop=- / probeBytes=- / rtxSent=- / qFresh=- / qAudio=- / qRtx=- / probeActive=- |
| 白盒 Candidate | senderUsage=- / estimate=- / pacing=- / fb=- / probePkts=- / probeStarts=- / probeDone=- / probeEarlyStop=- / probeBytes=- / rtxSent=- / qFresh=- / qAudio=- / qRtx=- / probeActive=- |
| 权重偏差 Baseline/Candidate | 0.24 / 0.24 |
| 变化（goodput/loss/recovery/queue/fairness） | - / - / - / - / 0.00% |
| 结论 | `PASS` |
| 分析 | Fairness derived from weighted multi-track trace shares over the final 10 samples. |

## 10. 产物链接

- 原始指标 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw-groups.json`
- 聚合对比 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/g0-vs-g2.json`
- 日志与 trace：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw`
