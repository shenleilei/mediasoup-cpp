# TWCC A/B 有效性评估结果

生成时间：`2026-04-21T20:01:09.632Z`

## 1. 实验信息

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

## 2. 汇总

| 指标 | Baseline | Candidate | 变化 |
|---|---:|---:|---:|
| 总体 Goodput (kbps) | 789.81 | 789.81 | 0.00% |
| 总体 Loss (%) | 0.00 | 0.00 | - |
| 恢复时间 (ms) | 395.00 | 425.00 | 7.59% |
| 稳态抖动 (P90-P50, kbps) | 0.00 | 0.00 | - |
| 队列压力（规范化总和） | 0.00 | 0.00 | - |
| 多路权重偏差（越小越好） | 0.24 | 0.24 | 0.00% |

结论：`FAIL`

说明：G2 Candidate did not meet every configured gate over G1 Controller-Only.

## 3. 判定门槛检查

| 规则 | 目标 | 实际 | 结论 |
|---|---|---|---|
| AB-002 congestion loss proxy improves >=20% | >=20% | n/a | `FAIL` |
| AB-003 recovery time not worse, ideally >=15% | not worse, ideally >=15% | -7.59% | `FAIL` |
| AB-001 steady-state goodput regression <=5% | >=-5% | 0.00% | `PASS` |
| AB-005 fairness deviation not increased | <=0% | 0.00% | `PASS` |
| transport-control hard regression suite | all pass | pass | `PASS` |

## 4. 场景级结果

| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |
|---|---|---|---|---|---|
| AB-001 | bw 4000kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=880.65, loss=0.00, rec=-ms | gp=880.65, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-002 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=582.08, loss=0.00, rec=-ms | gp=582.08, loss=0.00, rec=-ms | 0.00% | `PASS` |
| AB-003 | bw 500kbps / rtt 25ms / loss 0.1 / jitter 5ms | gp=880.65, loss=0.00, rec=395ms | gp=880.65, loss=0.00, rec=425ms | 0.00% | `PASS` |
| AB-004 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms | gp=815.85, loss=0.00, rec=-ms | gp=815.85, loss=0.00, rec=-ms | 0.00% | `PASS` |
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
| 白盒 Baseline | senderUsage=878432 / estimate=0 / pacing=900000 / fb=- / probePkts=- / probeStarts=- / probeDone=- / probeEarlyStop=- / probeBytes=- / rtxSent=- / qFresh=0 / qAudio=0 / qRtx=0 / probeActive=no |
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
| Baseline Goodput (mean/p50/p90) | 582.08 / 582.08 / 582.08 kbps |
| Candidate Goodput (mean/p50/p90) | 582.08 / 582.08 / 582.08 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=892400 / estimate=0 / pacing=900000 / fb=2 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=7 / qAudio=0 / qRtx=0 / probeActive=no |
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
| Baseline Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Candidate Goodput (mean/p50/p90) | 880.65 / 880.65 / 880.65 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | 395 / 395 / 395 ms |
| Candidate Recovery (mean/p50/p90) | 425 / 425 / 425 ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=901392 / estimate=0 / pacing=900000 / fb=9 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=227 / qAudio=0 / qRtx=0 / probeActive=no |
| 白盒 Candidate | senderUsage=0 / estimate=970266 / pacing=900000 / fb=10 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=220 / qFresh=256 / qAudio=0 / qRtx=0 / probeActive=no |
| 权重偏差 Baseline/Candidate | - / - |
| 变化（goodput/loss/recovery/queue/fairness） | 0.00% / - / 7.59% / - / - |
| 结论 | `PASS` |
| 分析 | Phase=recovery, case=T3. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples. |

### AB-004

| 字段 | 内容 |
|---|---|
| 场景名 | high_rtt_stability |
| 网络形态 | bw 4000kbps / rtt 180ms / loss 0.1 / jitter 5ms |
| Baseline Goodput (mean/p50/p90) | 815.85 / 815.85 / 815.85 kbps |
| Candidate Goodput (mean/p50/p90) | 815.85 / 815.85 / 815.85 kbps |
| Baseline Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Candidate Loss (mean/p50/p90) | 0.00 / 0.00 / 0.00 % |
| Baseline Recovery (mean/p50/p90) | - / - / - ms |
| Candidate Recovery (mean/p50/p90) | - / - / - ms |
| 队列压力 Baseline/Candidate | 0 / 0 |
| 白盒 Baseline | senderUsage=924720 / estimate=0 / pacing=900000 / fb=10 / probePkts=0 / probeStarts=0 / probeDone=0 / probeEarlyStop=0 / probeBytes=0 / rtxSent=0 / qFresh=8 / qAudio=0 / qRtx=0 / probeActive=no |
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

## 6. 产物链接

- 原始指标 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw-groups.json`
- 聚合对比 JSON：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/g1-vs-g2.json`
- 日志与 trace：`changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T20-01-09.632Z/raw`
