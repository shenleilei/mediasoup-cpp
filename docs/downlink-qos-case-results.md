# 下行 QoS 逐 Case 最终结果

生成时间：`2026-04-19T06:39:25.020Z`

## 1. 汇总

- 总 Case：`8`
- 已执行：`8`
- 通过：`8`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
- baseline：[D1](#d1)
- bw_sweep：[D2](#d2)
- loss_sweep：[D3](#d3)
- rtt_sweep：[D4](#d4)
- jitter_sweep：[D5](#d5)
- transition：[D6](#d6)
- competition：[D7](#d7)
- zero_demand：[D8](#d8)

## 3. 逐 Case 结果

### D1

| 字段 | 内容 |
|---|---|
| Case ID | `D1` |
| 类型 | `baseline` / priority `P0` |
| 说明 | good network, all consumers visible |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 10000ms |
| 预期 | consumerPaused=false；preferredSpatialLayer=2；recoveryPreferredSpatialLayer≥2 |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstUnpausedConsumer=2026-04-19T06:34:10.830Z；layerStable=2026-04-19T06:34:10.830Z |
| 恢复里程碑 | recoveryTraceSpan=9575ms；recoveryEntries=20 |
| 恢复诊断 | layers=[2], transitions=0, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D2

| 字段 | 内容 |
|---|---|
| Case ID | `D2` |
| 类型 | `bw_sweep` / priority `P0` |
| 说明 | bandwidth drops to 300kbps, expect layer downgrade |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 300kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤0；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=0, preferredTemporalLayer=2, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:34:32.778Z；firstUnpausedConsumer=2026-04-19T06:34:47.905Z；layerStable=2026-04-19T06:34:47.905Z |
| 恢复里程碑 | recoveryTraceSpan=14640ms；recoveryEntries=30 |
| 恢复诊断 | layers=[2], transitions=0, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D3

| 字段 | 内容 |
|---|---|
| Case ID | `D3` |
| 类型 | `loss_sweep` / priority `P0` |
| 说明 | 15% packet loss, expect health degrade |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 15% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤0；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=0, preferredTemporalLayer=0, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:35:14.907Z；firstUnpausedConsumer=2026-04-19T06:35:30.025Z；layerStable=2026-04-19T06:35:32.545Z |
| 恢复里程碑 | recoveryTraceSpan=14620ms；recoveryEntries=30 |
| 恢复诊断 | layers=[0,1,2], transitions=2, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D4

| 字段 | 内容 |
|---|---|
| Case ID | `D4` |
| 类型 | `rtt_sweep` / priority `P0` |
| 说明 | RTT 500ms, expect layer reduction |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 500ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤0；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=0, preferredTemporalLayer=0, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:35:57.011Z；firstUnpausedConsumer=2026-04-19T06:36:12.128Z；layerStable=2026-04-19T06:36:14.648Z |
| 恢复里程碑 | recoveryTraceSpan=14610ms；recoveryEntries=30 |
| 恢复诊断 | layers=[0,1,2], transitions=2, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D5

| 字段 | 内容 |
|---|---|
| Case ID | `D5` |
| 类型 | `jitter_sweep` / priority `P0` |
| 说明 | jitter 80ms, expect layer reduction |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 80ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤0；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=0, preferredTemporalLayer=0, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:36:39.111Z；firstUnpausedConsumer=2026-04-19T06:36:54.228Z；layerStable=2026-04-19T06:36:56.746Z |
| 恢复里程碑 | recoveryTraceSpan=14608ms；recoveryEntries=30 |
| 恢复诊断 | layers=[0,1,2], transitions=2, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D6

| 字段 | 内容 |
|---|---|
| Case ID | `D6` |
| 类型 | `transition` / priority `P0` |
| 说明 | good -> bad -> good, verify recovery without oscillation |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 200kbps / RTT 200ms / loss 10% / jitter 40ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 20000ms |
| 预期 | consumerPaused=false；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=false, preferredSpatialLayer=0, preferredTemporalLayer=0, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:37:21.199Z；firstUnpausedConsumer=2026-04-19T06:37:36.314Z；layerStable=2026-04-19T06:37:38.833Z |
| 恢复里程碑 | recoveryTraceSpan=19650ms；recoveryEntries=40 |
| 恢复诊断 | layers=[0,1,2], transitions=2, final=2 |
| D8 振荡检测 | 无振荡 (seq=-, pause=0, resume=0) |
| D7 竞争结果 | - |

### D7

| 字段 | 内容 |
|---|---|
| Case ID | `D7` |
| 类型 | `competition` / priority `P1` |
| 说明 | one subscriber with two consumers, pinned tile keeps better layer under constrained bw |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 500kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | highPriority gets better layer；recovers after impairment |
| 实际结果 | PASS（high=1 > low=0; priority=220 > 120） |
| impairment 结束 consumer 状态 | sub1(paused=false, layer=0, priority=120)；sub2(paused=false, layer=1, priority=220) |
| recovery 结束 consumer 状态 | sub1(paused=false, layer=2, priority=120)；sub2(paused=false, layer=2, priority=220) |
| 关键时间指标 | - |
| 恢复里程碑 | recoveryTraceSpan=14627ms；recoveryEntries=30 |
| 恢复诊断 | sub1(paused=false, layer=2, priority=120)；sub2(paused=false, layer=2, priority=220) |
| D8 振荡检测 | - |
| D7 竞争结果 | low-priority(sub1): layer=0, priority=120；high-priority(sub2): layer=1, priority=220 |

### D8

| 字段 | 内容 |
|---|---|
| Case ID | `D8` |
| 类型 | `zero_demand` / priority `P0` |
| 说明 | all consumers hidden, expect pauseUpstream after kPauseConfirmMs |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 15000ms |
| 预期 | pauseUpstream=true；resumeUpstream=true；recoveryPreferredSpatialLayer≥2；recovers after impairment |
| 实际结果 | PASS（ok） |
| impairment 结束 consumer 状态 | paused=true, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=1 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-19T06:38:50.487Z；firstPause=2026-04-19T06:38:54.518Z；firstResume=2026-04-19T06:39:00.567Z；firstUnpausedConsumer=2026-04-19T06:39:00.568Z；layerStable=2026-04-19T06:39:00.568Z |
| 恢复里程碑 | pauseLatency=4233ms；resumeLatency=202ms；recoveryTraceSpan=14612ms；recoveryEntries=30 |
| 恢复诊断 | layers=[2], transitions=0, final=2 |
| D8 振荡检测 | 无振荡 (seq=pause->resume, pause=2, resume=1) |
| D7 竞争结果 | - |

