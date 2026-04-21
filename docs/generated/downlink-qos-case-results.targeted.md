# 下行 QoS 逐 Case 最终结果

生成时间：`2026-04-14T08:54:24.710Z`

## 1. 汇总

- 总 Case：`2`
- 已执行：`2`
- 通过：`2`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
- competition：[D7](#d7)
- zero_demand：[D8](#d8)

## 3. 逐 Case 结果

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
| 恢复里程碑 | recoveryTraceSpan=14689ms；recoveryEntries=30 |
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
| impairment 结束 consumer 状态 | paused=true, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| recovery 结束 consumer 状态 | paused=false, preferredSpatialLayer=2, preferredTemporalLayer=2, priority=220 |
| 关键时间指标 | firstClamp=2026-04-14T08:53:49.960Z；firstPause=2026-04-14T08:53:54.059Z；firstResume=2026-04-14T08:54:00.147Z；firstUnpausedConsumer=2026-04-14T08:54:00.152Z；layerStable=2026-04-14T08:54:00.152Z |
| 恢复里程碑 | pauseLatency=4302ms；resumeLatency=204ms；recoveryTraceSpan=14688ms；recoveryEntries=30 |
| 恢复诊断 | layers=[2], transitions=0, final=2 |
| D8 振荡检测 | 无振荡 (seq=pause->resume, pause=2, resume=1) |
| D7 竞争结果 | - |
