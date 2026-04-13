# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T15:33:19.265Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`0`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [BW2](#bw2) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[BW2](#bw2)
- bw_sweep：[BW2](#bw2)

## 3. 逐 Case 结果

### BW2

| 字段 | 内容 |
|---|---|
| Case ID | `BW2` |
| 前置 Case | - |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 2000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=recovering/L3)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 19 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["stable","early_warning"],"maxLevel":1}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-12T15:32:48.988Z；firstRecovering=512ms (2026-04-12T15:32:49.500Z)；firstStable=5519ms (2026-04-12T15:32:54.507Z)；firstL0=8519ms (2026-04-12T15:32:57.507Z) |
| 恢复诊断 | raw=loss<3%=1519ms (2026-04-12T15:32:50.507Z), rtt<120ms=512ms (2026-04-12T15:32:49.500Z), jitter<28ms=512ms (2026-04-12T15:32:49.500Z), jitter<18ms=13520ms (2026-04-12T15:33:02.508Z)；target=target>=120kbps=512ms (2026-04-12T15:32:49.500Z), target>=300kbps=2519ms (2026-04-12T15:32:51.507Z), target>=500kbps=8519ms (2026-04-12T15:32:57.507Z), target>=700kbps=12520ms (2026-04-12T15:33:01.508Z), target>=900kbps=12520ms (2026-04-12T15:33:01.508Z)；send=send>=300kbps=512ms (2026-04-12T15:32:49.500Z), send>=500kbps=1519ms (2026-04-12T15:32:50.507Z), send>=700kbps=3519ms (2026-04-12T15:32:52.507Z), send>=900kbps=6520ms (2026-04-12T15:32:55.508Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2789ms, recovering=18797ms, stable=788ms, congested=6788ms, firstAction=2789ms, L0=-, L1=2789ms, L2=6788ms, L3=7789ms, L4=8788ms, audioOnly=-；recovery: warning=13520ms, recovering=512ms, stable=5519ms, congested=-, firstAction=2519ms, L0=8519ms, L1=5519ms, L2=2519ms, L3=-, L4=-, audioOnly=- |

