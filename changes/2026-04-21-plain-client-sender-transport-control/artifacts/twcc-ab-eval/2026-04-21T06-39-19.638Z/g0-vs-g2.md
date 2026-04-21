# TWCC A/B 有效性评估结果

生成时间：`2026-04-21T06:39:19.638Z`

## 1. 实验信息

| 字段 | 内容 |
|---|---|
| Baseline 版本 | `c17cab34b87c8f1438ada553f9af560d60e3bd2f` |
| Candidate 版本 | `c17cab34b87c8f1438ada553f9af560d60e3bd2f` |
| Host | `iZbp16494z0s9fdwtwq6pgZ` |
| OS / Kernel | `linux` / `5.10.134-18.al8.x86_64` |
| CPU / Memory | `Intel(R) Xeon(R) Platinum` / `3.5` GB |
| 网络注入 | tc/netem via cpp-client-matrix speed=0.25 |
| 输入素材 | tests/fixtures/media/test_sweep_cpp_matrix.mp4 + threaded multi_video_budget |
| 每场景重复次数 | 2 |
| 场景数量 | 5（PASS=5, FAIL=0） |

## 2. 汇总

| 指标 | Baseline | Candidate | 变化 |
|---|---:|---:|---:|
| 总体 Goodput (kbps) | 591.30 | 591.30 | 0.00% |
| 总体 Loss (%) | 0.00 | 0.00 | - |
| 恢复时间 (ms) | 6488.50 | 6375.00 | -1.75% |
| 稳态抖动 (P90-P50, kbps) | 0.00 | 0.00 | - |
| 队列压力（规范化总和） | - | - | - |
| 多路权重偏差（越小越好） | 0.23 | 0.22 | -1.99% |

结论：`FAIL`

说明：G2 Candidate did not meet every configured gate over G0 Legacy.

## 3. 判定门槛检查

| 规则 | 目标 | 实际 | 结论 |
|---|---|---|---|
| AB-002 congestion loss proxy improves >=20% | >=20% | n/a | `FAIL` |
| AB-003 recovery time not worse, ideally >=15% | not worse, ideally >=15% | 1.75% | `PASS` |
| AB-001 steady-state goodput regression <=5% | >=-5% | 0.00% | `PASS` |
| AB-005 fairness deviation not increased | <=0% | -1.99% | `PASS` |
| transport-control hard regression suite | all pass | pass | `PASS` |

## 4. 场景级结果

| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |
|---|---|---|---|---|---|
| AB-001 | bw 4000kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=880.65, loss=0.00, rec=-ms | gp=880.65, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-002 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=283.50, loss=0.00, rec=-ms | gp=283.50, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-003 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=450.00, loss=0.00, rec=6489ms | gp=450.00, loss=0.00, rec=6375ms | 0.00% | `PASS` |
| AB-004 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms | gp=751.05, loss=0.00, rec=-ms | gp=751.05, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-005 | bw 300kbps / rtt 260ms / loss 0.08 / jitter 24ms | gp=-, loss=-, rec=-ms | gp=-, loss=-, rec=-ms | - | `PASS` |

## 5. 逐场景明细

### AB-001

| 字段 | 内容 |
|---|---|
| 场景名 | steady_state |
| 网络形态 | bw 4000kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| Baseline Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Candidate Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=baseline, case=B1. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-002

| 字段 | 内容 |
|---|---|
| 场景名 | step_down_congestion |
| 网络形态 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| Baseline Goodput (mean/p50/p90) | 283.50 / 283.50 / 283.50 kbps |
| Candidate Goodput (mean/p50/p90) | 283.50 / 283.50 / 283.50 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=impairment, case=T3. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-003

| 字段 | 内容 |
|---|---|
| 场景名 | step_up_recovery |
| 网络形态 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms |
| Baseline Goodput (mean/p50/p90) | 450.00 / 450.00 / 450.00 kbps |
| Candidate Goodput (mean/p50/p90) | 450.00 / 450.00 / 450.00 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | 6489 / 6523 / 6523 ms |
| Candidate Recovery (mean/p50/p90) | 6375 / 6392 / 6392 ms |
| 队列压力 Baseline/Candidate | - / - |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / -1.75% / - / - |
| 结论 | `PASS` |
| 分析 | Phase=recovery, case=T3. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-004

| 字段 | 内容 |
|---|---|
| 场景名 | high_rtt_stability |
| 网络形态 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms |
| Baseline Goodput (mean/p50/p90) | 751.05 / 751.05 / 751.05 kbps |
| Candidate Goodput (mean/p50/p90) | 751.05 / 751.05 / 751.05 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / - / - / - |
| 结论 | `PASS` |
| 分析 | Phase=impairment, case=R4. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-005

| 字段 | 内容 |
|---|---|
| 场景名 | multi_video_budget |
| 网络形态 | bw 300kbps / rtt 260ms / loss 0.08 / jitter 24ms |
| Baseline Goodput (mean/p50/p90) | - / - / - kbps |
| Candidate Goodput (mean/p50/p90) | - / - / - kbps |
| Baseline Loss (mean/p50/p90) | - / - / - % |
| Candidate Loss (mean/p50/p90) | - / - / - % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | - / - |
| 权重偏差 Baseline/Candidate | 0.23 / 0.22 |
| 变化（goodput/loss/recovery/queue/fairness） | - / - / - / - / -1.99% |
| 结论 | `PASS` |
| 分析 | Fairness derived from weighted multi-track trace shares over the final 10 samples. |

## 6. 产物链接

- 原始指标 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/raw-groups.json`
- 聚合对比 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g0-vs-g2.json`
- 日志与 trace：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/raw`

