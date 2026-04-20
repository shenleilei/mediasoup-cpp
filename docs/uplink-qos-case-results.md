# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-19T20:58:23.877Z`

## 1. 汇总

- 总 Case：`43`
- 已执行：`43`
- 通过：`41`
- 失败：`2`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [R1](#r1) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强 |
| [S4](#s4) | `FAIL` | stateMatch=true, levelMatch=false, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[R1](#r1)、[S4](#s4)
- baseline：[B1](#b1)、[B2](#b2)、[B3](#b3)
- bw_sweep：[BW1](#bw1)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)
- loss_sweep：[L1](#l1)、[L2](#l2)、[L3](#l3)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)
- rtt_sweep：[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)
- jitter_sweep：[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)
- burst：[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)

## 3. 逐 Case 结果

### B1

| 字段 | 内容 |
|---|---|
| Case ID | `B1` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:07:06.762Z；firstRecovering=-；firstStable=257ms (2026-04-19T20:07:07.019Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=257ms (2026-04-19T20:07:07.019Z), rtt<120ms=257ms (2026-04-19T20:07:07.019Z), jitter<28ms=257ms (2026-04-19T20:07:07.019Z), jitter<18ms=257ms (2026-04-19T20:07:07.019Z)；target=target>=120kbps=257ms (2026-04-19T20:07:07.019Z), target>=300kbps=257ms (2026-04-19T20:07:07.019Z), target>=500kbps=257ms (2026-04-19T20:07:07.019Z), target>=700kbps=257ms (2026-04-19T20:07:07.019Z), target>=900kbps=257ms (2026-04-19T20:07:07.019Z)；send=send>=300kbps=257ms (2026-04-19T20:07:07.019Z), send>=500kbps=257ms (2026-04-19T20:07:07.019Z), send>=700kbps=257ms (2026-04-19T20:07:07.019Z), send>=900kbps=714ms (2026-04-19T20:07:07.476Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=308ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=257ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### B2

| 字段 | 内容 |
|---|---|
| Case ID | `B2` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P1` |
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| impairment 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L0 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:08:15.725Z；firstRecovering=-；firstStable=201ms (2026-04-19T20:08:15.926Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=701ms (2026-04-19T20:08:16.426Z), rtt<120ms=201ms (2026-04-19T20:08:15.926Z), jitter<28ms=201ms (2026-04-19T20:08:15.926Z), jitter<18ms=201ms (2026-04-19T20:08:15.926Z)；target=target>=120kbps=201ms (2026-04-19T20:08:15.926Z), target>=300kbps=201ms (2026-04-19T20:08:15.926Z), target>=500kbps=201ms (2026-04-19T20:08:15.926Z), target>=700kbps=201ms (2026-04-19T20:08:15.926Z), target>=900kbps=201ms (2026-04-19T20:08:15.926Z)；send=send>=300kbps=201ms (2026-04-19T20:08:15.926Z), send>=500kbps=201ms (2026-04-19T20:08:15.926Z), send>=700kbps=201ms (2026-04-19T20:08:15.926Z), send>=900kbps=701ms (2026-04-19T20:08:16.426Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=338ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=201ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### B3

| 字段 | 内容 |
|---|---|
| Case ID | `B3` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P0` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| recovery 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=early_warning/L1, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 1 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=early_warning/L1。 |
| 恢复里程碑 | start=2026-04-19T20:09:24.523Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=312ms (2026-04-19T20:09:24.835Z), rtt<120ms=312ms (2026-04-19T20:09:24.835Z), jitter<28ms=312ms (2026-04-19T20:09:24.835Z), jitter<18ms=-；target=target>=120kbps=312ms (2026-04-19T20:09:24.835Z), target>=300kbps=312ms (2026-04-19T20:09:24.835Z), target>=500kbps=312ms (2026-04-19T20:09:24.835Z), target>=700kbps=812ms (2026-04-19T20:09:25.335Z), target>=900kbps=-；send=send>=300kbps=312ms (2026-04-19T20:09:24.835Z), send>=500kbps=312ms (2026-04-19T20:09:24.835Z), send>=700kbps=312ms (2026-04-19T20:09:24.835Z), send>=900kbps=4312ms (2026-04-19T20:09:28.835Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=369ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=312ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### BW1

| 字段 | 内容 |
|---|---|
| Case ID | `BW1` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 3000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:10:33.402Z；firstRecovering=-；firstStable=278ms (2026-04-19T20:10:33.680Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=278ms (2026-04-19T20:10:33.680Z), rtt<120ms=278ms (2026-04-19T20:10:33.680Z), jitter<28ms=278ms (2026-04-19T20:10:33.680Z), jitter<18ms=278ms (2026-04-19T20:10:33.680Z)；target=target>=120kbps=278ms (2026-04-19T20:10:33.680Z), target>=300kbps=278ms (2026-04-19T20:10:33.680Z), target>=500kbps=278ms (2026-04-19T20:10:33.680Z), target>=700kbps=278ms (2026-04-19T20:10:33.680Z), target>=900kbps=720ms (2026-04-19T20:10:34.122Z)；send=send>=300kbps=278ms (2026-04-19T20:10:33.680Z), send>=500kbps=278ms (2026-04-19T20:10:33.680Z), send>=700kbps=278ms (2026-04-19T20:10:33.680Z), send>=900kbps=278ms (2026-04-19T20:10:33.680Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=367ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=278ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### BW3

| 字段 | 内容 |
|---|---|
| Case ID | `BW3` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 27 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:11:42.343Z；firstRecovering=-；firstStable=2737ms (2026-04-19T20:11:45.080Z)；firstL0=14734ms (2026-04-19T20:11:57.077Z) |
| 恢复诊断 | raw=loss<3%=235ms (2026-04-19T20:11:42.578Z), rtt<120ms=235ms (2026-04-19T20:11:42.578Z), jitter<28ms=1727ms (2026-04-19T20:11:44.070Z), jitter<18ms=13234ms (2026-04-19T20:11:55.577Z)；target=target>=120kbps=235ms (2026-04-19T20:11:42.578Z), target>=300kbps=235ms (2026-04-19T20:11:42.578Z), target>=500kbps=235ms (2026-04-19T20:11:42.578Z), target>=700kbps=4727ms (2026-04-19T20:11:47.070Z), target>=900kbps=15733ms (2026-04-19T20:11:58.076Z)；send=send>=300kbps=235ms (2026-04-19T20:11:42.578Z), send>=500kbps=2737ms (2026-04-19T20:11:45.080Z), send>=700kbps=7727ms (2026-04-19T20:11:50.070Z), send>=900kbps=15240ms (2026-04-19T20:11:57.583Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1344ms, recovering=11856ms, stable=354ms, congested=1844ms, firstAction=1344ms, L0=16352ms, L1=1344ms, L2=1844ms, L3=2344ms, L4=2844ms, audioOnly=-；recovery: warning=235ms, recovering=-, stable=2737ms, congested=-, firstAction=3727ms, L0=14734ms, L1=3727ms, L2=-, L3=-, L4=-, audioOnly=- |

### BW4

| 字段 | 内容 |
|---|---|
| Case ID | `BW4` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 800kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 56 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:12:51.129Z；firstRecovering=14261ms (2026-04-19T20:13:05.390Z)；firstStable=2764ms (2026-04-19T20:12:53.893Z)；firstL0=2764ms (2026-04-19T20:12:53.893Z) |
| 恢复诊断 | raw=loss<3%=761ms (2026-04-19T20:12:51.890Z), rtt<120ms=1805ms (2026-04-19T20:12:52.934Z), jitter<28ms=11764ms (2026-04-19T20:13:02.893Z), jitter<18ms=13759ms (2026-04-19T20:13:04.888Z)；target=target>=120kbps=261ms (2026-04-19T20:12:51.390Z), target>=300kbps=261ms (2026-04-19T20:12:51.390Z), target>=500kbps=16263ms (2026-04-19T20:13:07.392Z), target>=700kbps=17262ms (2026-04-19T20:13:08.391Z), target>=900kbps=18265ms (2026-04-19T20:13:09.394Z)；send=send>=300kbps=261ms (2026-04-19T20:12:51.390Z), send>=500kbps=2764ms (2026-04-19T20:12:53.893Z), send>=700kbps=17262ms (2026-04-19T20:13:08.391Z), send>=900kbps=18265ms (2026-04-19T20:13:09.394Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1345ms, recovering=16338ms, stable=363ms, congested=1843ms, firstAction=1345ms, L0=-, L1=1345ms, L2=1843ms, L3=2341ms, L4=2842ms, audioOnly=-；recovery: warning=261ms, recovering=14261ms, stable=2764ms, congested=5760ms, firstAction=2764ms, L0=2764ms, L1=5266ms, L2=5760ms, L3=6261ms, L4=6763ms, audioOnly=- |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=recovering/L2)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T20:14:00.033Z；firstRecovering=295ms (2026-04-19T20:14:00.328Z)；firstStable=2794ms (2026-04-19T20:14:02.827Z)；firstL0=2294ms (2026-04-19T20:14:02.327Z) |
| 恢复诊断 | raw=loss<3%=794ms (2026-04-19T20:14:00.827Z), rtt<120ms=295ms (2026-04-19T20:14:00.328Z), jitter<28ms=14294ms (2026-04-19T20:14:14.327Z), jitter<18ms=24294ms (2026-04-19T20:14:24.327Z)；target=target>=120kbps=295ms (2026-04-19T20:14:00.328Z), target>=300kbps=15294ms (2026-04-19T20:14:15.327Z), target>=500kbps=19314ms (2026-04-19T20:14:19.347Z), target>=700kbps=20794ms (2026-04-19T20:14:20.827Z), target>=900kbps=-；send=send>=300kbps=295ms (2026-04-19T20:14:00.328Z), send>=500kbps=7352ms (2026-04-19T20:14:07.385Z), send>=700kbps=18795ms (2026-04-19T20:14:18.828Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1854ms, recovering=15854ms, stable=354ms, congested=2355ms, firstAction=1854ms, L0=-, L1=1854ms, L2=2355ms, L3=2867ms, L4=3368ms, audioOnly=-；recovery: warning=4799ms, recovering=295ms, stable=2794ms, congested=5294ms, firstAction=794ms, L0=2294ms, L1=794ms, L2=5294ms, L3=5795ms, L4=6295ms, audioOnly=- |

### BW6

| 字段 | 内容 |
|---|---|
| Case ID | `BW6` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 87 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:15:08.810Z；firstRecovering=23813ms (2026-04-19T20:15:32.623Z)；firstStable=26313ms (2026-04-19T20:15:35.123Z)；firstL0=27812ms (2026-04-19T20:15:36.622Z) |
| 恢复诊断 | raw=loss<3%=312ms (2026-04-19T20:15:09.122Z), rtt<120ms=312ms (2026-04-19T20:15:09.122Z), jitter<28ms=23312ms (2026-04-19T20:15:32.122Z), jitter<18ms=24312ms (2026-04-19T20:15:33.122Z)；target=target>=120kbps=2813ms (2026-04-19T20:15:11.623Z), target>=300kbps=24812ms (2026-04-19T20:15:33.622Z), target>=500kbps=25812ms (2026-04-19T20:15:34.622Z), target>=700kbps=26858ms (2026-04-19T20:15:35.668Z), target>=900kbps=28812ms (2026-04-19T20:15:37.622Z)；send=send>=300kbps=24812ms (2026-04-19T20:15:33.622Z), send>=500kbps=25812ms (2026-04-19T20:15:34.622Z), send>=700kbps=26313ms (2026-04-19T20:15:35.123Z), send>=900kbps=26313ms (2026-04-19T20:15:35.123Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2403ms, recovering=-, stable=405ms, congested=2903ms, firstAction=2403ms, L0=-, L1=2403ms, L2=2903ms, L3=3403ms, L4=3904ms, audioOnly=-；recovery: warning=-, recovering=23813ms, stable=26313ms, congested=312ms, firstAction=312ms, L0=27812ms, L1=26313ms, L2=25312ms, L3=23813ms, L4=312ms, audioOnly=- |

### BW7

| 字段 | 内容 |
|---|---|
| Case ID | `BW7` |
| 前置 Case | [B1](#b1) |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 200kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T20:16:17.725Z；firstRecovering=15209ms (2026-04-19T20:16:32.934Z)；firstStable=17712ms (2026-04-19T20:16:35.437Z)；firstL0=19211ms (2026-04-19T20:16:36.936Z) |
| 恢复诊断 | raw=loss<3%=206ms (2026-04-19T20:16:17.931Z), rtt<120ms=206ms (2026-04-19T20:16:17.931Z), jitter<28ms=206ms (2026-04-19T20:16:17.931Z), jitter<18ms=15708ms (2026-04-19T20:16:33.433Z)；target=target>=120kbps=12207ms (2026-04-19T20:16:29.932Z), target>=300kbps=15708ms (2026-04-19T20:16:33.433Z), target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=729ms (2026-04-19T20:16:18.454Z), send>=500kbps=729ms (2026-04-19T20:16:18.454Z), send>=700kbps=729ms (2026-04-19T20:16:18.454Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4315ms, recovering=-, stable=311ms, congested=4804ms, firstAction=4315ms, L0=-, L1=4315ms, L2=4804ms, L3=5305ms, L4=5805ms, audioOnly=-；recovery: warning=21713ms, recovering=15209ms, stable=17712ms, congested=206ms, firstAction=206ms, L0=19211ms, L1=17712ms, L2=16709ms, L3=15209ms, L4=206ms, audioOnly=- |

### L1

| 字段 | 内容 |
|---|---|
| Case ID | `L1` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:17:26.688Z；firstRecovering=-；firstStable=213ms (2026-04-19T20:17:26.901Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=213ms (2026-04-19T20:17:26.901Z), rtt<120ms=213ms (2026-04-19T20:17:26.901Z), jitter<28ms=213ms (2026-04-19T20:17:26.901Z), jitter<18ms=213ms (2026-04-19T20:17:26.901Z)；target=target>=120kbps=213ms (2026-04-19T20:17:26.901Z), target>=300kbps=213ms (2026-04-19T20:17:26.901Z), target>=500kbps=213ms (2026-04-19T20:17:26.901Z), target>=700kbps=213ms (2026-04-19T20:17:26.901Z), target>=900kbps=213ms (2026-04-19T20:17:26.901Z)；send=send>=300kbps=213ms (2026-04-19T20:17:26.901Z), send>=500kbps=213ms (2026-04-19T20:17:26.901Z), send>=700kbps=213ms (2026-04-19T20:17:26.901Z), send>=900kbps=213ms (2026-04-19T20:17:26.901Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=350ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=213ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L2

| 字段 | 内容 |
|---|---|
| Case ID | `L2` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:18:35.567Z；firstRecovering=-；firstStable=281ms (2026-04-19T20:18:35.848Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=281ms (2026-04-19T20:18:35.848Z), rtt<120ms=281ms (2026-04-19T20:18:35.848Z), jitter<28ms=281ms (2026-04-19T20:18:35.848Z), jitter<18ms=281ms (2026-04-19T20:18:35.848Z)；target=target>=120kbps=281ms (2026-04-19T20:18:35.848Z), target>=300kbps=281ms (2026-04-19T20:18:35.848Z), target>=500kbps=281ms (2026-04-19T20:18:35.848Z), target>=700kbps=281ms (2026-04-19T20:18:35.848Z), target>=900kbps=281ms (2026-04-19T20:18:35.848Z)；send=send>=300kbps=281ms (2026-04-19T20:18:35.848Z), send>=500kbps=281ms (2026-04-19T20:18:35.848Z), send>=700kbps=281ms (2026-04-19T20:18:35.848Z), send>=900kbps=281ms (2026-04-19T20:18:35.848Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=330ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=281ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L3

| 字段 | 内容 |
|---|---|
| Case ID | `L3` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 2% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:19:44.492Z；firstRecovering=-；firstStable=223ms (2026-04-19T20:19:44.715Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=723ms (2026-04-19T20:19:45.215Z), rtt<120ms=223ms (2026-04-19T20:19:44.715Z), jitter<28ms=223ms (2026-04-19T20:19:44.715Z), jitter<18ms=2723ms (2026-04-19T20:19:47.215Z)；target=target>=120kbps=223ms (2026-04-19T20:19:44.715Z), target>=300kbps=223ms (2026-04-19T20:19:44.715Z), target>=500kbps=223ms (2026-04-19T20:19:44.715Z), target>=700kbps=223ms (2026-04-19T20:19:44.715Z), target>=900kbps=223ms (2026-04-19T20:19:44.715Z)；send=send>=300kbps=223ms (2026-04-19T20:19:44.715Z), send>=500kbps=223ms (2026-04-19T20:19:44.715Z), send>=700kbps=223ms (2026-04-19T20:19:44.715Z), send>=900kbps=223ms (2026-04-19T20:19:44.715Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=313ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=223ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L4

| 字段 | 内容 |
|---|---|
| Case ID | `L4` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:20:53.415Z；firstRecovering=-；firstStable=11146ms (2026-04-19T20:21:04.561Z)；firstL0=11146ms (2026-04-19T20:21:04.561Z) |
| 恢复诊断 | raw=loss<3%=144ms (2026-04-19T20:20:53.559Z), rtt<120ms=144ms (2026-04-19T20:20:53.559Z), jitter<28ms=144ms (2026-04-19T20:20:53.559Z), jitter<18ms=2133ms (2026-04-19T20:20:55.548Z)；target=target>=120kbps=144ms (2026-04-19T20:20:53.559Z), target>=300kbps=144ms (2026-04-19T20:20:53.559Z), target>=500kbps=144ms (2026-04-19T20:20:53.559Z), target>=700kbps=144ms (2026-04-19T20:20:53.559Z), target>=900kbps=11629ms (2026-04-19T20:21:05.044Z)；send=send>=300kbps=144ms (2026-04-19T20:20:53.559Z), send>=500kbps=144ms (2026-04-19T20:20:53.559Z), send>=700kbps=144ms (2026-04-19T20:20:53.559Z), send>=900kbps=144ms (2026-04-19T20:20:53.559Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4268ms, recovering=-, stable=264ms, congested=-, firstAction=4268ms, L0=-, L1=4268ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=144ms, recovering=-, stable=11146ms, congested=-, firstAction=11146ms, L0=11146ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 49 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:22:02.155Z；firstRecovering=3785ms (2026-04-19T20:22:05.940Z)；firstStable=4285ms (2026-04-19T20:22:06.440Z)；firstL0=8286ms (2026-04-19T20:22:10.441Z) |
| 恢复诊断 | raw=loss<3%=788ms (2026-04-19T20:22:02.943Z), rtt<120ms=285ms (2026-04-19T20:22:02.440Z), jitter<28ms=285ms (2026-04-19T20:22:02.440Z), jitter<18ms=285ms (2026-04-19T20:22:02.440Z)；target=target>=120kbps=1285ms (2026-04-19T20:22:03.440Z), target>=300kbps=5806ms (2026-04-19T20:22:07.961Z), target>=500kbps=7285ms (2026-04-19T20:22:09.440Z), target>=700kbps=7285ms (2026-04-19T20:22:09.440Z), target>=900kbps=10786ms (2026-04-19T20:22:12.941Z)；send=send>=300kbps=4285ms (2026-04-19T20:22:06.440Z), send>=500kbps=4285ms (2026-04-19T20:22:06.440Z), send>=700kbps=4285ms (2026-04-19T20:22:06.440Z), send>=900kbps=4785ms (2026-04-19T20:22:06.940Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1383ms, recovering=-, stable=361ms, congested=2863ms, firstAction=1383ms, L0=-, L1=1383ms, L2=2863ms, L3=3361ms, L4=3861ms, audioOnly=-；recovery: warning=10786ms, recovering=3785ms, stable=4285ms, congested=285ms, firstAction=285ms, L0=8286ms, L1=6785ms, L2=5285ms, L3=3785ms, L4=285ms, audioOnly=- |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 51 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:23:10.947Z；firstRecovering=7339ms (2026-04-19T20:23:18.286Z)；firstStable=9840ms (2026-04-19T20:23:20.787Z)；firstL0=11839ms (2026-04-19T20:23:22.786Z) |
| 恢复诊断 | raw=loss<3%=339ms (2026-04-19T20:23:11.286Z), rtt<120ms=339ms (2026-04-19T20:23:11.286Z), jitter<28ms=6840ms (2026-04-19T20:23:17.787Z), jitter<18ms=13339ms (2026-04-19T20:23:24.286Z)；target=target>=120kbps=7839ms (2026-04-19T20:23:18.786Z), target>=300kbps=7839ms (2026-04-19T20:23:18.786Z), target>=500kbps=10842ms (2026-04-19T20:23:21.789Z), target>=700kbps=11839ms (2026-04-19T20:23:22.786Z), target>=900kbps=12377ms (2026-04-19T20:23:23.324Z)；send=send>=300kbps=7839ms (2026-04-19T20:23:18.786Z), send>=500kbps=7839ms (2026-04-19T20:23:18.786Z), send>=700kbps=7839ms (2026-04-19T20:23:18.786Z), send>=900kbps=9339ms (2026-04-19T20:23:20.286Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2896ms, recovering=-, stable=399ms, congested=4396ms, firstAction=2896ms, L0=-, L1=2896ms, L2=4396ms, L3=4896ms, L4=5396ms, audioOnly=-；recovery: warning=-, recovering=7339ms, stable=9840ms, congested=339ms, firstAction=339ms, L0=11839ms, L1=10339ms, L2=8840ms, L3=7339ms, L4=339ms, audioOnly=- |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 40% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 61 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:24:19.932Z；firstRecovering=12279ms (2026-04-19T20:24:32.211Z)；firstStable=14781ms (2026-04-19T20:24:34.713Z)；firstL0=15779ms (2026-04-19T20:24:35.711Z) |
| 恢复诊断 | raw=loss<3%=279ms (2026-04-19T20:24:20.211Z), rtt<120ms=279ms (2026-04-19T20:24:20.211Z), jitter<28ms=11779ms (2026-04-19T20:24:31.711Z), jitter<18ms=16280ms (2026-04-19T20:24:36.212Z)；target=target>=120kbps=12779ms (2026-04-19T20:24:32.711Z), target>=300kbps=13779ms (2026-04-19T20:24:33.711Z), target>=500kbps=14334ms (2026-04-19T20:24:34.266Z), target>=700kbps=15282ms (2026-04-19T20:24:35.214Z), target>=900kbps=16280ms (2026-04-19T20:24:36.212Z)；send=send>=300kbps=12779ms (2026-04-19T20:24:32.711Z), send>=500kbps=12779ms (2026-04-19T20:24:32.711Z), send>=700kbps=12779ms (2026-04-19T20:24:32.711Z), send>=900kbps=14334ms (2026-04-19T20:24:34.266Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3841ms, recovering=-, stable=338ms, congested=4341ms, firstAction=3841ms, L0=-, L1=3841ms, L2=4341ms, L3=4841ms, L4=5338ms, audioOnly=-；recovery: warning=-, recovering=12279ms, stable=14781ms, congested=279ms, firstAction=279ms, L0=15779ms, L1=14781ms, L2=13779ms, L3=12279ms, L4=279ms, audioOnly=- |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 前置 Case | [B1](#b1) |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 60% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 59 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:25:28.792Z；firstRecovering=10286ms (2026-04-19T20:25:39.078Z)；firstStable=12767ms (2026-04-19T20:25:41.559Z)；firstL0=14810ms (2026-04-19T20:25:43.602Z) |
| 恢复诊断 | raw=loss<3%=268ms (2026-04-19T20:25:29.060Z), rtt<120ms=268ms (2026-04-19T20:25:29.060Z), jitter<28ms=9769ms (2026-04-19T20:25:38.561Z), jitter<18ms=13267ms (2026-04-19T20:25:42.059Z)；target=target>=120kbps=10777ms (2026-04-19T20:25:39.569Z), target>=300kbps=12267ms (2026-04-19T20:25:41.059Z), target>=500kbps=12767ms (2026-04-19T20:25:41.559Z), target>=700kbps=13769ms (2026-04-19T20:25:42.561Z), target>=900kbps=15328ms (2026-04-19T20:25:44.120Z)；send=send>=300kbps=3270ms (2026-04-19T20:25:32.062Z), send>=500kbps=10777ms (2026-04-19T20:25:39.569Z), send>=700kbps=10777ms (2026-04-19T20:25:39.569Z), send>=900kbps=11269ms (2026-04-19T20:25:40.061Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2862ms, recovering=-, stable=361ms, congested=3361ms, firstAction=2862ms, L0=-, L1=2862ms, L2=3361ms, L3=3859ms, L4=4359ms, audioOnly=-；recovery: warning=-, recovering=10286ms, stable=12767ms, congested=268ms, firstAction=268ms, L0=14810ms, L1=13267ms, L2=11767ms, L3=10286ms, L4=268ms, audioOnly=- |

### R1

| 字段 | 内容 |
|---|---|
| Case ID | `R1` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 50ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"state":"stable","maxLevel":1}；实际 impairment 评估值=early_warning/L1，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-19T20:26:37.551Z；firstRecovering=-；firstStable=329ms (2026-04-19T20:26:37.880Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=329ms (2026-04-19T20:26:37.880Z), rtt<120ms=329ms (2026-04-19T20:26:37.880Z), jitter<28ms=329ms (2026-04-19T20:26:37.880Z), jitter<18ms=329ms (2026-04-19T20:26:37.880Z)；target=target>=120kbps=329ms (2026-04-19T20:26:37.880Z), target>=300kbps=329ms (2026-04-19T20:26:37.880Z), target>=500kbps=329ms (2026-04-19T20:26:37.880Z), target>=700kbps=329ms (2026-04-19T20:26:37.880Z), target>=900kbps=329ms (2026-04-19T20:26:37.880Z)；send=send>=300kbps=329ms (2026-04-19T20:26:37.880Z), send>=500kbps=329ms (2026-04-19T20:26:37.880Z), send>=700kbps=329ms (2026-04-19T20:26:37.880Z), send>=900kbps=329ms (2026-04-19T20:26:37.880Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=383ms, recovering=-, stable=12881ms, congested=-, firstAction=12881ms, L0=12881ms, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=329ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R2

| 字段 | 内容 |
|---|---|
| Case ID | `R2` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 80ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:27:46.558Z；firstRecovering=-；firstStable=103ms (2026-04-19T20:27:46.661Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=603ms (2026-04-19T20:27:47.161Z), rtt<120ms=103ms (2026-04-19T20:27:46.661Z), jitter<28ms=103ms (2026-04-19T20:27:46.661Z), jitter<18ms=103ms (2026-04-19T20:27:46.661Z)；target=target>=120kbps=103ms (2026-04-19T20:27:46.661Z), target>=300kbps=103ms (2026-04-19T20:27:46.661Z), target>=500kbps=103ms (2026-04-19T20:27:46.661Z), target>=700kbps=103ms (2026-04-19T20:27:46.661Z), target>=900kbps=103ms (2026-04-19T20:27:46.661Z)；send=send>=300kbps=103ms (2026-04-19T20:27:46.661Z), send>=500kbps=103ms (2026-04-19T20:27:46.661Z), send>=700kbps=103ms (2026-04-19T20:27:46.661Z), send>=900kbps=103ms (2026-04-19T20:27:46.661Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5692ms, recovering=-, stable=190ms, congested=-, firstAction=5692ms, L0=16193ms, L1=5692ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=103ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 120ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:28:55.354Z；firstRecovering=-；firstStable=270ms (2026-04-19T20:28:55.624Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=270ms (2026-04-19T20:28:55.624Z), rtt<120ms=1768ms (2026-04-19T20:28:57.122Z), jitter<28ms=270ms (2026-04-19T20:28:55.624Z), jitter<18ms=270ms (2026-04-19T20:28:55.624Z)；target=target>=120kbps=270ms (2026-04-19T20:28:55.624Z), target>=300kbps=270ms (2026-04-19T20:28:55.624Z), target>=500kbps=270ms (2026-04-19T20:28:55.624Z), target>=700kbps=768ms (2026-04-19T20:28:56.122Z), target>=900kbps=768ms (2026-04-19T20:28:56.122Z)；send=send>=300kbps=270ms (2026-04-19T20:28:55.624Z), send>=500kbps=270ms (2026-04-19T20:28:55.624Z), send>=700kbps=270ms (2026-04-19T20:28:55.624Z), send>=900kbps=270ms (2026-04-19T20:28:55.624Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=331ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=270ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:30:04.368Z；firstRecovering=-；firstStable=11583ms (2026-04-19T20:30:15.951Z)；firstL0=11583ms (2026-04-19T20:30:15.951Z) |
| 恢复诊断 | raw=loss<3%=84ms (2026-04-19T20:30:04.452Z), rtt<120ms=1586ms (2026-04-19T20:30:05.954Z), jitter<28ms=84ms (2026-04-19T20:30:04.452Z), jitter<18ms=84ms (2026-04-19T20:30:04.452Z)；target=target>=120kbps=84ms (2026-04-19T20:30:04.452Z), target>=300kbps=84ms (2026-04-19T20:30:04.452Z), target>=500kbps=84ms (2026-04-19T20:30:04.452Z), target>=700kbps=84ms (2026-04-19T20:30:04.452Z), target>=900kbps=12080ms (2026-04-19T20:30:16.448Z)；send=send>=300kbps=84ms (2026-04-19T20:30:04.452Z), send>=500kbps=84ms (2026-04-19T20:30:04.452Z), send>=700kbps=581ms (2026-04-19T20:30:04.949Z), send>=900kbps=3582ms (2026-04-19T20:30:07.950Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4212ms, recovering=-, stable=228ms, congested=-, firstAction=4212ms, L0=-, L1=4212ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=84ms, recovering=-, stable=11583ms, congested=-, firstAction=11583ms, L0=11583ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 250ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 69 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:31:13.453Z；firstRecovering=4694ms (2026-04-19T20:31:18.147Z)；firstStable=5198ms (2026-04-19T20:31:18.651Z)；firstL0=9196ms (2026-04-19T20:31:22.649Z) |
| 恢复诊断 | raw=loss<3%=193ms (2026-04-19T20:31:13.646Z), rtt<120ms=1694ms (2026-04-19T20:31:15.147Z), jitter<28ms=193ms (2026-04-19T20:31:13.646Z), jitter<18ms=193ms (2026-04-19T20:31:13.646Z)；target=target>=120kbps=193ms (2026-04-19T20:31:13.646Z), target>=300kbps=5198ms (2026-04-19T20:31:18.651Z), target>=500kbps=6694ms (2026-04-19T20:31:20.147Z), target>=700kbps=9710ms (2026-04-19T20:31:23.163Z), target>=900kbps=24739ms (2026-04-19T20:31:38.192Z)；send=send>=300kbps=5198ms (2026-04-19T20:31:18.651Z), send>=500kbps=5198ms (2026-04-19T20:31:18.651Z), send>=700kbps=5198ms (2026-04-19T20:31:18.651Z), send>=900kbps=7200ms (2026-04-19T20:31:20.653Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1821ms, recovering=-, stable=320ms, congested=3317ms, firstAction=1821ms, L0=-, L1=1821ms, L2=3317ms, L3=3818ms, L4=4316ms, audioOnly=-；recovery: warning=11693ms, recovering=4694ms, stable=5198ms, congested=193ms, firstAction=193ms, L0=9196ms, L1=7696ms, L2=6195ms, L3=4694ms, L4=193ms, audioOnly=- |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 前置 Case | [B1](#b1) |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 350ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:32:22.415Z；firstRecovering=5198ms (2026-04-19T20:32:27.613Z)；firstStable=5699ms (2026-04-19T20:32:28.114Z)；firstL0=9699ms (2026-04-19T20:32:32.114Z) |
| 恢复诊断 | raw=loss<3%=199ms (2026-04-19T20:32:22.614Z), rtt<120ms=1699ms (2026-04-19T20:32:24.114Z), jitter<28ms=199ms (2026-04-19T20:32:22.614Z), jitter<18ms=199ms (2026-04-19T20:32:22.614Z)；target=target>=120kbps=199ms (2026-04-19T20:32:22.614Z), target>=300kbps=5699ms (2026-04-19T20:32:28.114Z), target>=500kbps=8200ms (2026-04-19T20:32:30.615Z), target>=700kbps=11700ms (2026-04-19T20:32:34.115Z), target>=900kbps=12198ms (2026-04-19T20:32:34.613Z)；send=send>=300kbps=5699ms (2026-04-19T20:32:28.114Z), send>=500kbps=5699ms (2026-04-19T20:32:28.114Z), send>=700kbps=6199ms (2026-04-19T20:32:28.614Z), send>=900kbps=10199ms (2026-04-19T20:32:32.614Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1805ms, recovering=-, stable=310ms, congested=2305ms, firstAction=1805ms, L0=-, L1=1805ms, L2=2305ms, L3=2804ms, L4=3305ms, audioOnly=-；recovery: warning=12198ms, recovering=5198ms, stable=5699ms, congested=199ms, firstAction=199ms, L0=9699ms, L1=8200ms, L2=6698ms, L3=5198ms, L4=199ms, audioOnly=- |

### J1

| 字段 | 内容 |
|---|---|
| Case ID | `J1` |
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 10ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:33:31.184Z；firstRecovering=-；firstStable=363ms (2026-04-19T20:33:31.547Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=363ms (2026-04-19T20:33:31.547Z), rtt<120ms=363ms (2026-04-19T20:33:31.547Z), jitter<28ms=363ms (2026-04-19T20:33:31.547Z), jitter<18ms=363ms (2026-04-19T20:33:31.547Z)；target=target>=120kbps=363ms (2026-04-19T20:33:31.547Z), target>=300kbps=363ms (2026-04-19T20:33:31.547Z), target>=500kbps=363ms (2026-04-19T20:33:31.547Z), target>=700kbps=363ms (2026-04-19T20:33:31.547Z), target>=900kbps=363ms (2026-04-19T20:33:31.547Z)；send=send>=300kbps=363ms (2026-04-19T20:33:31.547Z), send>=500kbps=363ms (2026-04-19T20:33:31.547Z), send>=700kbps=363ms (2026-04-19T20:33:31.547Z), send>=900kbps=363ms (2026-04-19T20:33:31.547Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=379ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=363ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J2

| 字段 | 内容 |
|---|---|
| Case ID | `J2` |
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 20ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:34:40.073Z；firstRecovering=-；firstStable=204ms (2026-04-19T20:34:40.277Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=698ms (2026-04-19T20:34:40.771Z), rtt<120ms=204ms (2026-04-19T20:34:40.277Z), jitter<28ms=204ms (2026-04-19T20:34:40.277Z), jitter<18ms=1698ms (2026-04-19T20:34:41.771Z)；target=target>=120kbps=204ms (2026-04-19T20:34:40.277Z), target>=300kbps=204ms (2026-04-19T20:34:40.277Z), target>=500kbps=204ms (2026-04-19T20:34:40.277Z), target>=700kbps=204ms (2026-04-19T20:34:40.277Z), target>=900kbps=204ms (2026-04-19T20:34:40.277Z)；send=send>=300kbps=204ms (2026-04-19T20:34:40.277Z), send>=500kbps=204ms (2026-04-19T20:34:40.277Z), send>=700kbps=204ms (2026-04-19T20:34:40.277Z), send>=900kbps=204ms (2026-04-19T20:34:40.277Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=304ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=204ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:35:48.834Z；firstRecovering=-；firstStable=12805ms (2026-04-19T20:36:01.639Z)；firstL0=12805ms (2026-04-19T20:36:01.639Z) |
| 恢复诊断 | raw=loss<3%=806ms (2026-04-19T20:35:49.640Z), rtt<120ms=1806ms (2026-04-19T20:35:50.640Z), jitter<28ms=306ms (2026-04-19T20:35:49.140Z), jitter<18ms=9847ms (2026-04-19T20:35:58.681Z)；target=target>=120kbps=306ms (2026-04-19T20:35:49.140Z), target>=300kbps=306ms (2026-04-19T20:35:49.140Z), target>=500kbps=306ms (2026-04-19T20:35:49.140Z), target>=700kbps=306ms (2026-04-19T20:35:49.140Z), target>=900kbps=13306ms (2026-04-19T20:36:02.140Z)；send=send>=300kbps=306ms (2026-04-19T20:35:49.140Z), send>=500kbps=306ms (2026-04-19T20:35:49.140Z), send>=700kbps=306ms (2026-04-19T20:35:49.140Z), send>=900kbps=306ms (2026-04-19T20:35:49.140Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2870ms, recovering=-, stable=372ms, congested=-, firstAction=2870ms, L0=-, L1=2870ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=306ms, recovering=-, stable=12805ms, congested=-, firstAction=12805ms, L0=12805ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:36:57.612Z；firstRecovering=-；firstStable=17805ms (2026-04-19T20:37:15.417Z)；firstL0=17805ms (2026-04-19T20:37:15.417Z) |
| 恢复诊断 | raw=loss<3%=306ms (2026-04-19T20:36:57.918Z), rtt<120ms=1811ms (2026-04-19T20:36:59.423Z), jitter<28ms=1307ms (2026-04-19T20:36:58.919Z), jitter<18ms=14808ms (2026-04-19T20:37:12.420Z)；target=target>=120kbps=306ms (2026-04-19T20:36:57.918Z), target>=300kbps=306ms (2026-04-19T20:36:57.918Z), target>=500kbps=306ms (2026-04-19T20:36:57.918Z), target>=700kbps=306ms (2026-04-19T20:36:57.918Z), target>=900kbps=18305ms (2026-04-19T20:37:15.917Z)；send=send>=300kbps=306ms (2026-04-19T20:36:57.918Z), send>=500kbps=306ms (2026-04-19T20:36:57.918Z), send>=700kbps=306ms (2026-04-19T20:36:57.918Z), send>=900kbps=306ms (2026-04-19T20:36:57.918Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3413ms, recovering=-, stable=416ms, congested=-, firstAction=3413ms, L0=-, L1=3413ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=306ms, recovering=-, stable=17805ms, congested=-, firstAction=17805ms, L0=17805ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 100ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:38:06.549Z；firstRecovering=3289ms (2026-04-19T20:38:09.838Z)；firstStable=5787ms (2026-04-19T20:38:12.336Z)；firstL0=6787ms (2026-04-19T20:38:13.336Z) |
| 恢复诊断 | raw=loss<3%=288ms (2026-04-19T20:38:06.837Z), rtt<120ms=1787ms (2026-04-19T20:38:08.336Z), jitter<28ms=2787ms (2026-04-19T20:38:09.336Z), jitter<18ms=12287ms (2026-04-19T20:38:18.836Z)；target=target>=120kbps=288ms (2026-04-19T20:38:06.837Z), target>=300kbps=3793ms (2026-04-19T20:38:10.342Z), target>=500kbps=5288ms (2026-04-19T20:38:11.837Z), target>=700kbps=6289ms (2026-04-19T20:38:12.838Z), target>=900kbps=7789ms (2026-04-19T20:38:14.338Z)；send=send>=300kbps=3793ms (2026-04-19T20:38:10.342Z), send>=500kbps=3793ms (2026-04-19T20:38:10.342Z), send>=700kbps=5288ms (2026-04-19T20:38:11.837Z), send>=900kbps=5288ms (2026-04-19T20:38:11.837Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1361ms, recovering=-, stable=362ms, congested=1863ms, firstAction=1361ms, L0=-, L1=1361ms, L2=1863ms, L3=2361ms, L4=2861ms, audioOnly=-；recovery: warning=9287ms, recovering=3289ms, stable=5787ms, congested=288ms, firstAction=288ms, L0=6787ms, L1=5787ms, L2=4805ms, L3=3289ms, L4=288ms, audioOnly=- |

### T1

| 字段 | 内容 |
|---|---|
| Case ID | `T1` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 2000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 stable / early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:39:15.319Z；firstRecovering=-；firstStable=6276ms (2026-04-19T20:39:21.595Z)；firstL0=6276ms (2026-04-19T20:39:21.595Z) |
| 恢复诊断 | raw=loss<3%=777ms (2026-04-19T20:39:16.096Z), rtt<120ms=431ms (2026-04-19T20:39:15.750Z), jitter<28ms=431ms (2026-04-19T20:39:15.750Z), jitter<18ms=1779ms (2026-04-19T20:39:17.098Z)；target=target>=120kbps=431ms (2026-04-19T20:39:15.750Z), target>=300kbps=431ms (2026-04-19T20:39:15.750Z), target>=500kbps=431ms (2026-04-19T20:39:15.750Z), target>=700kbps=431ms (2026-04-19T20:39:15.750Z), target>=900kbps=6780ms (2026-04-19T20:39:22.099Z)；send=send>=300kbps=431ms (2026-04-19T20:39:15.750Z), send>=500kbps=777ms (2026-04-19T20:39:16.096Z), send>=700kbps=777ms (2026-04-19T20:39:16.096Z), send>=900kbps=1274ms (2026-04-19T20:39:16.593Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6849ms, recovering=-, stable=337ms, congested=-, firstAction=6849ms, L0=-, L1=6849ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=431ms, recovering=-, stable=6276ms, congested=-, firstAction=6276ms, L0=6276ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 71 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:40:24.281Z；firstRecovering=8786ms (2026-04-19T20:40:33.067Z)；firstStable=11287ms (2026-04-19T20:40:35.568Z)；firstL0=13301ms (2026-04-19T20:40:37.582Z) |
| 恢复诊断 | raw=loss<3%=285ms (2026-04-19T20:40:24.566Z), rtt<120ms=285ms (2026-04-19T20:40:24.566Z), jitter<28ms=8285ms (2026-04-19T20:40:32.566Z), jitter<18ms=20295ms (2026-04-19T20:40:44.576Z)；target=target>=120kbps=285ms (2026-04-19T20:40:24.566Z), target>=300kbps=285ms (2026-04-19T20:40:24.566Z), target>=500kbps=12785ms (2026-04-19T20:40:37.066Z), target>=700kbps=28309ms (2026-04-19T20:40:52.590Z), target>=900kbps=29792ms (2026-04-19T20:40:54.073Z)；send=send>=300kbps=285ms (2026-04-19T20:40:24.566Z), send>=500kbps=285ms (2026-04-19T20:40:24.566Z), send>=700kbps=11287ms (2026-04-19T20:40:35.568Z), send>=900kbps=14785ms (2026-04-19T20:40:39.066Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1378ms, recovering=12378ms, stable=378ms, congested=1879ms, firstAction=1378ms, L0=16878ms, L1=1378ms, L2=1879ms, L3=2377ms, L4=2878ms, audioOnly=-；recovery: warning=15786ms, recovering=8786ms, stable=11287ms, congested=285ms, firstAction=285ms, L0=13301ms, L1=11784ms, L2=285ms, L3=784ms, L4=1291ms, audioOnly=- |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=recovering/L3)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 65 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:41:33.073Z；firstRecovering=299ms (2026-04-19T20:41:33.372Z)；firstStable=3301ms (2026-04-19T20:41:36.374Z)；firstL0=4302ms (2026-04-19T20:41:37.375Z) |
| 恢复诊断 | raw=loss<3%=299ms (2026-04-19T20:41:33.372Z), rtt<120ms=299ms (2026-04-19T20:41:33.372Z), jitter<28ms=299ms (2026-04-19T20:41:33.372Z), jitter<18ms=14803ms (2026-04-19T20:41:47.876Z)；target=target>=120kbps=299ms (2026-04-19T20:41:33.372Z), target>=300kbps=1805ms (2026-04-19T20:41:34.878Z), target>=500kbps=2299ms (2026-04-19T20:41:35.372Z), target>=700kbps=3301ms (2026-04-19T20:41:36.374Z), target>=900kbps=21801ms (2026-04-19T20:41:54.874Z)；send=send>=300kbps=299ms (2026-04-19T20:41:33.372Z), send>=500kbps=1805ms (2026-04-19T20:41:34.878Z), send>=700kbps=2299ms (2026-04-19T20:41:35.372Z), send>=900kbps=3301ms (2026-04-19T20:41:36.374Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1897ms, recovering=19390ms, stable=398ms, congested=2393ms, firstAction=1897ms, L0=-, L1=1897ms, L2=2393ms, L3=2891ms, L4=3390ms, audioOnly=-；recovery: warning=6798ms, recovering=299ms, stable=3301ms, congested=7304ms, firstAction=1300ms, L0=4302ms, L1=2807ms, L2=1300ms, L3=7799ms, L4=8305ms, audioOnly=- |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:42:41.848Z；firstRecovering=-；firstStable=2797ms (2026-04-19T20:42:44.645Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=281ms (2026-04-19T20:42:42.129Z), rtt<120ms=281ms (2026-04-19T20:42:42.129Z), jitter<28ms=281ms (2026-04-19T20:42:42.129Z), jitter<18ms=1781ms (2026-04-19T20:42:43.629Z)；target=target>=120kbps=281ms (2026-04-19T20:42:42.129Z), target>=300kbps=281ms (2026-04-19T20:42:42.129Z), target>=500kbps=281ms (2026-04-19T20:42:42.129Z), target>=700kbps=281ms (2026-04-19T20:42:42.129Z), target>=900kbps=281ms (2026-04-19T20:42:42.129Z)；send=send>=300kbps=281ms (2026-04-19T20:42:42.129Z), send>=500kbps=281ms (2026-04-19T20:42:42.129Z), send>=700kbps=281ms (2026-04-19T20:42:42.129Z), send>=900kbps=281ms (2026-04-19T20:42:42.129Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3347ms, recovering=-, stable=348ms, congested=-, firstAction=3347ms, L0=13346ms, L1=3347ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=281ms, recovering=-, stable=2797ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 65 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:43:50.601Z；firstRecovering=12842ms (2026-04-19T20:44:03.443Z)；firstStable=15315ms (2026-04-19T20:44:05.916Z)；firstL0=16317ms (2026-04-19T20:44:06.918Z) |
| 恢复诊断 | raw=loss<3%=814ms (2026-04-19T20:43:51.415Z), rtt<120ms=313ms (2026-04-19T20:43:50.914Z), jitter<28ms=12313ms (2026-04-19T20:44:02.914Z), jitter<18ms=14314ms (2026-04-19T20:44:04.915Z)；target=target>=120kbps=13310ms (2026-04-19T20:44:03.911Z), target>=300kbps=13310ms (2026-04-19T20:44:03.911Z), target>=500kbps=14812ms (2026-04-19T20:44:05.413Z), target>=700kbps=15813ms (2026-04-19T20:44:06.414Z), target>=900kbps=16813ms (2026-04-19T20:44:07.414Z)；send=send>=300kbps=13310ms (2026-04-19T20:44:03.911Z), send>=500kbps=14812ms (2026-04-19T20:44:05.413Z), send>=700kbps=14812ms (2026-04-19T20:44:05.413Z), send>=900kbps=15813ms (2026-04-19T20:44:06.414Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2389ms, recovering=-, stable=385ms, congested=2883ms, firstAction=2389ms, L0=-, L1=2389ms, L2=2883ms, L3=3384ms, L4=3883ms, audioOnly=-；recovery: warning=-, recovering=12842ms, stable=15315ms, congested=313ms, firstAction=313ms, L0=16317ms, L1=15315ms, L2=14314ms, L3=12842ms, L4=313ms, audioOnly=- |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:44:59.547Z；firstRecovering=-；firstStable=13154ms (2026-04-19T20:45:12.701Z)；firstL0=13154ms (2026-04-19T20:45:12.701Z) |
| 恢复诊断 | raw=loss<3%=155ms (2026-04-19T20:44:59.702Z), rtt<120ms=1655ms (2026-04-19T20:45:01.202Z), jitter<28ms=155ms (2026-04-19T20:44:59.702Z), jitter<18ms=155ms (2026-04-19T20:44:59.702Z)；target=target>=120kbps=155ms (2026-04-19T20:44:59.702Z), target>=300kbps=155ms (2026-04-19T20:44:59.702Z), target>=500kbps=155ms (2026-04-19T20:44:59.702Z), target>=700kbps=155ms (2026-04-19T20:44:59.702Z), target>=900kbps=13691ms (2026-04-19T20:45:13.238Z)；send=send>=300kbps=155ms (2026-04-19T20:44:59.702Z), send>=500kbps=155ms (2026-04-19T20:44:59.702Z), send>=700kbps=155ms (2026-04-19T20:44:59.702Z), send>=900kbps=671ms (2026-04-19T20:45:00.218Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1710ms, recovering=-, stable=210ms, congested=-, firstAction=1710ms, L0=-, L1=1710ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=155ms, recovering=-, stable=13154ms, congested=-, firstAction=13154ms, L0=13154ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 前置 Case | [B1](#b1) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=recovering/L3)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 86 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:46:08.293Z；firstRecovering=2799ms (2026-04-19T20:46:11.092Z)；firstStable=5302ms (2026-04-19T20:46:13.595Z)；firstL0=7299ms (2026-04-19T20:46:15.592Z) |
| 恢复诊断 | raw=loss<3%=799ms (2026-04-19T20:46:09.092Z), rtt<120ms=305ms (2026-04-19T20:46:08.598Z), jitter<28ms=2302ms (2026-04-19T20:46:10.595Z), jitter<18ms=15797ms (2026-04-19T20:46:24.090Z)；target=target>=120kbps=305ms (2026-04-19T20:46:08.598Z), target>=300kbps=3300ms (2026-04-19T20:46:11.593Z), target>=500kbps=6299ms (2026-04-19T20:46:14.592Z), target>=700kbps=21799ms (2026-04-19T20:46:30.092Z), target>=900kbps=22800ms (2026-04-19T20:46:31.093Z)；send=send>=300kbps=3300ms (2026-04-19T20:46:11.593Z), send>=500kbps=3300ms (2026-04-19T20:46:11.593Z), send>=700kbps=5798ms (2026-04-19T20:46:14.091Z), send>=900kbps=9304ms (2026-04-19T20:46:17.597Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6880ms, recovering=383ms, stable=2380ms, congested=7378ms, firstAction=1379ms, L0=4382ms, L1=2880ms, L2=1379ms, L3=7879ms, L4=8387ms, audioOnly=-；recovery: warning=9805ms, recovering=2799ms, stable=5302ms, congested=305ms, firstAction=305ms, L0=7299ms, L1=5798ms, L2=4299ms, L3=2799ms, L4=305ms, audioOnly=- |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 前置 Case | [B3](#b3) |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| recovery 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 允许持续降级，不要求恢复；最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 39 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-19T20:47:17.131Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=374ms, recovering=-, stable=-, congested=1370ms, firstAction=1370ms, L0=-, L1=-, L2=1370ms, L3=1870ms, L4=2369ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T9

| 字段 | 内容 |
|---|---|
| Case ID | `T9` |
| 前置 Case | - |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| 持续时间 | baseline 15000ms / impairment 100000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 226 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T20:49:15.775Z；firstRecovering=17797ms (2026-04-19T20:49:33.572Z)；firstStable=20294ms (2026-04-19T20:49:36.069Z)；firstL0=22295ms (2026-04-19T20:49:38.070Z) |
| 恢复诊断 | raw=loss<3%=797ms (2026-04-19T20:49:16.572Z), rtt<120ms=1794ms (2026-04-19T20:49:17.569Z), jitter<28ms=17295ms (2026-04-19T20:49:33.070Z), jitter<18ms=18295ms (2026-04-19T20:49:34.070Z)；target=target>=120kbps=14294ms (2026-04-19T20:49:30.069Z), target>=300kbps=20795ms (2026-04-19T20:49:36.570Z), target>=500kbps=24807ms (2026-04-19T20:49:40.582Z), target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=19798ms (2026-04-19T20:49:35.573Z), send>=500kbps=21295ms (2026-04-19T20:49:37.070Z), send>=700kbps=24807ms (2026-04-19T20:49:40.582Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4382ms, recovering=23880ms, stable=2383ms, congested=4889ms, firstAction=4382ms, L0=40879ms, L1=4382ms, L2=4889ms, L3=5381ms, L4=5880ms, audioOnly=-；recovery: warning=24807ms, recovering=17797ms, stable=20294ms, congested=298ms, firstAction=298ms, L0=22295ms, L1=20795ms, L2=19297ms, L3=17797ms, L4=298ms, audioOnly=- |

### T10

| 字段 | 内容 |
|---|---|
| Case ID | `T10` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| 持续时间 | baseline 15000ms / impairment 100000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 245 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T20:51:44.656Z；firstRecovering=19282ms (2026-04-19T20:52:03.938Z)；firstStable=21782ms (2026-04-19T20:52:06.438Z)；firstL0=23809ms (2026-04-19T20:52:08.465Z) |
| 恢复诊断 | raw=loss<3%=282ms (2026-04-19T20:51:44.938Z), rtt<120ms=1285ms (2026-04-19T20:51:45.941Z), jitter<28ms=18782ms (2026-04-19T20:52:03.438Z), jitter<18ms=19782ms (2026-04-19T20:52:04.438Z)；target=target>=120kbps=11282ms (2026-04-19T20:51:55.938Z), target>=300kbps=19782ms (2026-04-19T20:52:04.438Z), target>=500kbps=21282ms (2026-04-19T20:52:05.938Z), target>=700kbps=25782ms (2026-04-19T20:52:10.438Z), target>=900kbps=26282ms (2026-04-19T20:52:10.938Z)；send=send>=300kbps=20282ms (2026-04-19T20:52:04.938Z), send>=500kbps=20282ms (2026-04-19T20:52:04.938Z), send>=700kbps=22283ms (2026-04-19T20:52:06.939Z), send>=900kbps=25782ms (2026-04-19T20:52:10.438Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2880ms, recovering=-, stable=2377ms, congested=3378ms, firstAction=2880ms, L0=-, L1=2880ms, L2=3378ms, L3=3878ms, L4=4378ms, audioOnly=-；recovery: warning=26282ms, recovering=19282ms, stable=21782ms, congested=282ms, firstAction=282ms, L0=23809ms, L1=22283ms, L2=20792ms, L3=19282ms, L4=282ms, audioOnly=- |

### T11

| 字段 | 内容 |
|---|---|
| Case ID | `T11` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| 持续时间 | baseline 15000ms / impairment 100000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 224 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:54:13.607Z；firstRecovering=16671ms (2026-04-19T20:54:30.278Z)；firstStable=19172ms (2026-04-19T20:54:32.779Z)；firstL0=21171ms (2026-04-19T20:54:34.778Z) |
| 恢复诊断 | raw=loss<3%=672ms (2026-04-19T20:54:14.279Z), rtt<120ms=1171ms (2026-04-19T20:54:14.778Z), jitter<28ms=16171ms (2026-04-19T20:54:29.778Z), jitter<18ms=22171ms (2026-04-19T20:54:35.778Z)；target=target>=120kbps=11671ms (2026-04-19T20:54:25.278Z), target>=300kbps=17171ms (2026-04-19T20:54:30.778Z), target>=500kbps=18671ms (2026-04-19T20:54:32.278Z), target>=700kbps=20174ms (2026-04-19T20:54:33.781Z), target>=900kbps=21671ms (2026-04-19T20:54:35.278Z)；send=send>=300kbps=17671ms (2026-04-19T20:54:31.278Z), send>=500kbps=18671ms (2026-04-19T20:54:32.278Z), send>=700kbps=20174ms (2026-04-19T20:54:33.781Z), send>=900kbps=20671ms (2026-04-19T20:54:34.278Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4234ms, recovering=16234ms, stable=2238ms, congested=4732ms, firstAction=4234ms, L0=-, L1=4234ms, L2=4732ms, L3=5232ms, L4=5732ms, audioOnly=-；recovery: warning=-, recovering=16671ms, stable=19172ms, congested=171ms, firstAction=171ms, L0=21171ms, L1=19671ms, L2=18171ms, L3=16671ms, L4=171ms, audioOnly=- |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 前置 Case | [B1](#b1) |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:55:07.311Z；firstRecovering=-；firstStable=16317ms (2026-04-19T20:55:23.628Z)；firstL0=16317ms (2026-04-19T20:55:23.628Z) |
| 恢复诊断 | raw=loss<3%=815ms (2026-04-19T20:55:08.126Z), rtt<120ms=313ms (2026-04-19T20:55:07.624Z), jitter<28ms=313ms (2026-04-19T20:55:07.624Z), jitter<18ms=313ms (2026-04-19T20:55:07.624Z)；target=target>=120kbps=313ms (2026-04-19T20:55:07.624Z), target>=300kbps=313ms (2026-04-19T20:55:07.624Z), target>=500kbps=313ms (2026-04-19T20:55:07.624Z), target>=700kbps=313ms (2026-04-19T20:55:07.624Z), target>=900kbps=16814ms (2026-04-19T20:55:24.125Z)；send=send>=300kbps=313ms (2026-04-19T20:55:07.624Z), send>=500kbps=313ms (2026-04-19T20:55:07.624Z), send>=700kbps=313ms (2026-04-19T20:55:07.624Z), send>=900kbps=313ms (2026-04-19T20:55:07.624Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2912ms, recovering=-, stable=412ms, congested=-, firstAction=2912ms, L0=-, L1=2912ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=313ms, recovering=-, stable=16317ms, congested=-, firstAction=16317ms, L0=16317ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | [B1](#b1) |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 8000ms / recovery 32000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 54 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T20:56:04.158Z；firstRecovering=20825ms (2026-04-19T20:56:24.983Z)；firstStable=23328ms (2026-04-19T20:56:27.486Z)；firstL0=25332ms (2026-04-19T20:56:29.490Z) |
| 恢复诊断 | raw=loss<3%=328ms (2026-04-19T20:56:04.486Z), rtt<120ms=1328ms (2026-04-19T20:56:05.486Z), jitter<28ms=328ms (2026-04-19T20:56:04.486Z), jitter<18ms=328ms (2026-04-19T20:56:04.486Z)；target=target>=120kbps=13826ms (2026-04-19T20:56:17.984Z), target>=300kbps=21330ms (2026-04-19T20:56:25.488Z), target>=500kbps=22830ms (2026-04-19T20:56:26.988Z), target>=700kbps=28328ms (2026-04-19T20:56:32.486Z), target>=900kbps=-；send=send>=300kbps=328ms (2026-04-19T20:56:04.486Z), send>=500kbps=838ms (2026-04-19T20:56:04.996Z), send>=700kbps=838ms (2026-04-19T20:56:04.996Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4383ms, recovering=-, stable=397ms, congested=4877ms, firstAction=4383ms, L0=-, L1=4383ms, L2=4877ms, L3=5375ms, L4=5876ms, audioOnly=-；recovery: warning=27826ms, recovering=20825ms, stable=23328ms, congested=328ms, firstAction=328ms, L0=25332ms, L1=23826ms, L2=22326ms, L3=20825ms, L4=328ms, audioOnly=- |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 前置 Case | [B1](#b1) |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 200ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T20:56:59.939Z；firstRecovering=-；firstStable=19334ms (2026-04-19T20:57:19.273Z)；firstL0=19334ms (2026-04-19T20:57:19.273Z) |
| 恢复诊断 | raw=loss<3%=301ms (2026-04-19T20:57:00.240Z), rtt<120ms=801ms (2026-04-19T20:57:00.740Z), jitter<28ms=301ms (2026-04-19T20:57:00.240Z), jitter<18ms=13301ms (2026-04-19T20:57:13.240Z)；target=target>=120kbps=301ms (2026-04-19T20:57:00.240Z), target>=300kbps=301ms (2026-04-19T20:57:00.240Z), target>=500kbps=801ms (2026-04-19T20:57:00.740Z), target>=700kbps=12324ms (2026-04-19T20:57:12.263Z), target>=900kbps=19804ms (2026-04-19T20:57:19.743Z)；send=send>=300kbps=301ms (2026-04-19T20:57:00.240Z), send>=500kbps=301ms (2026-04-19T20:57:00.240Z), send>=700kbps=301ms (2026-04-19T20:57:00.240Z), send>=900kbps=4801ms (2026-04-19T20:57:04.740Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1866ms, recovering=-, stable=365ms, congested=-, firstAction=1866ms, L0=-, L1=1866ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=301ms, recovering=-, stable=19334ms, congested=-, firstAction=19334ms, L0=19334ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 前置 Case | [B1](#b1) |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=true, levelMatch=false, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 21 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["early_warning","congested"],"maxLevel":2}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=true, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-19T20:57:53.698Z；firstRecovering=5322ms (2026-04-19T20:57:59.020Z)；firstStable=7814ms (2026-04-19T20:58:01.512Z)；firstL0=8824ms (2026-04-19T20:58:02.522Z) |
| 恢复诊断 | raw=loss<3%=317ms (2026-04-19T20:57:54.015Z), rtt<120ms=817ms (2026-04-19T20:57:54.515Z), jitter<28ms=1816ms (2026-04-19T20:57:55.514Z), jitter<18ms=2817ms (2026-04-19T20:57:56.515Z)；target=target>=120kbps=317ms (2026-04-19T20:57:54.015Z), target>=300kbps=5824ms (2026-04-19T20:57:59.522Z), target>=500kbps=7318ms (2026-04-19T20:58:01.016Z), target>=700kbps=8320ms (2026-04-19T20:58:02.018Z), target>=900kbps=9320ms (2026-04-19T20:58:03.018Z)；send=send>=300kbps=5824ms (2026-04-19T20:57:59.522Z), send>=500kbps=5824ms (2026-04-19T20:57:59.522Z), send>=700kbps=7318ms (2026-04-19T20:58:01.016Z), send>=900kbps=8320ms (2026-04-19T20:58:02.018Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1879ms, recovering=-, stable=379ms, congested=2378ms, firstAction=1879ms, L0=-, L1=1879ms, L2=2378ms, L3=2878ms, L4=3380ms, audioOnly=-；recovery: warning=-, recovering=5322ms, stable=7814ms, congested=317ms, firstAction=317ms, L0=8824ms, L1=7814ms, L2=6817ms, L3=5322ms, L4=317ms, audioOnly=- |

