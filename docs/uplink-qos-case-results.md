# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-19T06:33:48.605Z`

## 1. 汇总

- 总 Case：`43`
- 已执行：`13`
- 通过：`13`
- 失败：`0`
- 错误：`30`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [L5](#l5) | `ERROR` | Attempted to use detached Frame 'A5B48A4CDB3E9FE870094717A5E5CAA4'. |
| [L6](#l6) | `ERROR` | The service is no longer running |
| [L7](#l7) | `ERROR` | The service is no longer running |
| [L8](#l8) | `ERROR` | The service is no longer running |
| [R1](#r1) | `ERROR` | The service is no longer running |
| [R2](#r2) | `ERROR` | The service is no longer running |
| [R3](#r3) | `ERROR` | The service is no longer running |
| [R4](#r4) | `ERROR` | The service is no longer running |
| [R5](#r5) | `ERROR` | The service is no longer running |
| [R6](#r6) | `ERROR` | The service is no longer running |
| [J1](#j1) | `ERROR` | The service is no longer running |
| [J2](#j2) | `ERROR` | The service is no longer running |
| [J3](#j3) | `ERROR` | The service is no longer running |
| [J4](#j4) | `ERROR` | The service is no longer running |
| [J5](#j5) | `ERROR` | The service is no longer running |
| [T1](#t1) | `ERROR` | The service is no longer running |
| [T2](#t2) | `ERROR` | The service is no longer running |
| [T3](#t3) | `ERROR` | The service is no longer running |
| [T4](#t4) | `ERROR` | The service is no longer running |
| [T5](#t5) | `ERROR` | The service is no longer running |
| [T6](#t6) | `ERROR` | The service is no longer running |
| [T7](#t7) | `ERROR` | The service is no longer running |
| [T8](#t8) | `ERROR` | The service is no longer running |
| [T9](#t9) | `ERROR` | The service is no longer running |
| [T10](#t10) | `ERROR` | The service is no longer running |
| [T11](#t11) | `ERROR` | The service is no longer running |
| [S1](#s1) | `ERROR` | The service is no longer running |
| [S2](#s2) | `ERROR` | The service is no longer running |
| [S3](#s3) | `ERROR` | The service is no longer running |
| [S4](#s4) | `ERROR` | The service is no longer running |

## 2. 快速跳转

- 失败 / 错误：[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)、[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)、[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)、[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)、[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)
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
| 恢复里程碑 | start=2026-04-19T06:18:29.063Z；firstRecovering=-；firstStable=417ms (2026-04-19T06:18:29.480Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=417ms (2026-04-19T06:18:29.480Z), rtt<120ms=417ms (2026-04-19T06:18:29.480Z), jitter<28ms=417ms (2026-04-19T06:18:29.480Z), jitter<18ms=417ms (2026-04-19T06:18:29.480Z)；target=target>=120kbps=417ms (2026-04-19T06:18:29.480Z), target>=300kbps=417ms (2026-04-19T06:18:29.480Z), target>=500kbps=417ms (2026-04-19T06:18:29.480Z), target>=700kbps=417ms (2026-04-19T06:18:29.480Z), target>=900kbps=417ms (2026-04-19T06:18:29.480Z)；send=send>=300kbps=417ms (2026-04-19T06:18:29.480Z), send>=500kbps=417ms (2026-04-19T06:18:29.480Z), send>=700kbps=417ms (2026-04-19T06:18:29.480Z), send>=900kbps=417ms (2026-04-19T06:18:29.480Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=454ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=417ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T06:19:37.488Z；firstRecovering=-；firstStable=430ms (2026-04-19T06:19:37.918Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=430ms (2026-04-19T06:19:37.918Z), rtt<120ms=430ms (2026-04-19T06:19:37.918Z), jitter<28ms=430ms (2026-04-19T06:19:37.918Z), jitter<18ms=430ms (2026-04-19T06:19:37.918Z)；target=target>=120kbps=430ms (2026-04-19T06:19:37.918Z), target>=300kbps=430ms (2026-04-19T06:19:37.918Z), target>=500kbps=430ms (2026-04-19T06:19:37.918Z), target>=700kbps=430ms (2026-04-19T06:19:37.918Z), target>=900kbps=430ms (2026-04-19T06:19:37.918Z)；send=send>=300kbps=430ms (2026-04-19T06:19:37.918Z), send>=500kbps=430ms (2026-04-19T06:19:37.918Z), send>=700kbps=430ms (2026-04-19T06:19:37.918Z), send>=900kbps=430ms (2026-04-19T06:19:37.918Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=458ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=430ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:20:45.911Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=414ms (2026-04-19T06:20:46.325Z), rtt<120ms=414ms (2026-04-19T06:20:46.325Z), jitter<28ms=414ms (2026-04-19T06:20:46.325Z), jitter<18ms=-；target=target>=120kbps=414ms (2026-04-19T06:20:46.325Z), target>=300kbps=414ms (2026-04-19T06:20:46.325Z), target>=500kbps=414ms (2026-04-19T06:20:46.325Z), target>=700kbps=414ms (2026-04-19T06:20:46.325Z), target>=900kbps=-；send=send>=300kbps=414ms (2026-04-19T06:20:46.325Z), send>=500kbps=414ms (2026-04-19T06:20:46.325Z), send>=700kbps=914ms (2026-04-19T06:20:46.825Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=454ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=414ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:21:54.345Z；firstRecovering=-；firstStable=403ms (2026-04-19T06:21:54.748Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=403ms (2026-04-19T06:21:54.748Z), rtt<120ms=403ms (2026-04-19T06:21:54.748Z), jitter<28ms=403ms (2026-04-19T06:21:54.748Z), jitter<18ms=403ms (2026-04-19T06:21:54.748Z)；target=target>=120kbps=403ms (2026-04-19T06:21:54.748Z), target>=300kbps=403ms (2026-04-19T06:21:54.748Z), target>=500kbps=403ms (2026-04-19T06:21:54.748Z), target>=700kbps=403ms (2026-04-19T06:21:54.748Z), target>=900kbps=403ms (2026-04-19T06:21:54.748Z)；send=send>=300kbps=403ms (2026-04-19T06:21:54.748Z), send>=500kbps=403ms (2026-04-19T06:21:54.748Z), send>=700kbps=403ms (2026-04-19T06:21:54.748Z), send>=900kbps=403ms (2026-04-19T06:21:54.748Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=459ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=403ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 25 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T06:23:02.765Z；firstRecovering=-；firstStable=16912ms (2026-04-19T06:23:19.677Z)；firstL0=16912ms (2026-04-19T06:23:19.677Z) |
| 恢复诊断 | raw=loss<3%=910ms (2026-04-19T06:23:03.675Z), rtt<120ms=410ms (2026-04-19T06:23:03.175Z), jitter<28ms=11910ms (2026-04-19T06:23:14.675Z), jitter<18ms=12910ms (2026-04-19T06:23:15.675Z)；target=target>=120kbps=410ms (2026-04-19T06:23:03.175Z), target>=300kbps=410ms (2026-04-19T06:23:03.175Z), target>=500kbps=5410ms (2026-04-19T06:23:08.175Z), target>=700kbps=9910ms (2026-04-19T06:23:12.675Z), target>=900kbps=17410ms (2026-04-19T06:23:20.175Z)；send=send>=300kbps=410ms (2026-04-19T06:23:03.175Z), send>=500kbps=410ms (2026-04-19T06:23:03.175Z), send>=700kbps=6910ms (2026-04-19T06:23:09.675Z), send>=900kbps=17410ms (2026-04-19T06:23:20.175Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1450ms, recovering=10950ms, stable=450ms, congested=1950ms, firstAction=1450ms, L0=15450ms, L1=1450ms, L2=1950ms, L3=2450ms, L4=2950ms, audioOnly=-；recovery: warning=410ms, recovering=-, stable=16912ms, congested=-, firstAction=16912ms, L0=16912ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L3)；recovery(评估取 best=stable/L0, current=stable/L1) |
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=stable/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T06:24:11.176Z；firstRecovering=9418ms (2026-04-19T06:24:20.594Z)；firstStable=11917ms (2026-04-19T06:24:23.093Z)；firstL0=13417ms (2026-04-19T06:24:24.593Z) |
| 恢复诊断 | raw=loss<3%=918ms (2026-04-19T06:24:12.094Z), rtt<120ms=1918ms (2026-04-19T06:24:13.094Z), jitter<28ms=8919ms (2026-04-19T06:24:20.095Z), jitter<18ms=22417ms (2026-04-19T06:24:33.593Z)；target=target>=120kbps=418ms (2026-04-19T06:24:11.594Z), target>=300kbps=9918ms (2026-04-19T06:24:21.094Z), target>=500kbps=14417ms (2026-04-19T06:24:25.593Z), target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=418ms (2026-04-19T06:24:11.594Z), send>=500kbps=1418ms (2026-04-19T06:24:12.594Z), send>=700kbps=14918ms (2026-04-19T06:24:26.094Z), send>=900kbps=28917ms (2026-04-19T06:24:40.093Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1455ms, recovering=11954ms, stable=455ms, congested=1955ms, firstAction=1455ms, L0=16454ms, L1=1455ms, L2=1955ms, L3=2454ms, L4=2955ms, audioOnly=-；recovery: warning=15917ms, recovering=9418ms, stable=11917ms, congested=418ms, firstAction=418ms, L0=13417ms, L1=11917ms, L2=10917ms, L3=9418ms, L4=418ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 56 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T06:25:19.576Z；firstRecovering=7927ms (2026-04-19T06:25:27.503Z)；firstStable=10426ms (2026-04-19T06:25:30.002Z)；firstL0=12427ms (2026-04-19T06:25:32.003Z) |
| 恢复诊断 | raw=loss<3%=427ms (2026-04-19T06:25:20.003Z), rtt<120ms=427ms (2026-04-19T06:25:20.003Z), jitter<28ms=1427ms (2026-04-19T06:25:21.003Z), jitter<18ms=13927ms (2026-04-19T06:25:33.503Z)；target=target>=120kbps=427ms (2026-04-19T06:25:20.003Z), target>=300kbps=9927ms (2026-04-19T06:25:29.503Z), target>=500kbps=10426ms (2026-04-19T06:25:30.002Z), target>=700kbps=11426ms (2026-04-19T06:25:31.002Z), target>=900kbps=12927ms (2026-04-19T06:25:32.503Z)；send=send>=300kbps=8427ms (2026-04-19T06:25:28.003Z), send>=500kbps=9927ms (2026-04-19T06:25:29.503Z), send>=700kbps=10426ms (2026-04-19T06:25:30.002Z), send>=900kbps=10426ms (2026-04-19T06:25:30.002Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2957ms, recovering=-, stable=456ms, congested=3457ms, firstAction=2957ms, L0=-, L1=2957ms, L2=3457ms, L3=3956ms, L4=4457ms, audioOnly=-；recovery: warning=14926ms, recovering=7927ms, stable=10426ms, congested=427ms, firstAction=427ms, L0=12427ms, L1=10926ms, L2=9426ms, L3=7927ms, L4=427ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T06:26:27.976Z；firstRecovering=19420ms (2026-04-19T06:26:47.396Z)；firstStable=21920ms (2026-04-19T06:26:49.896Z)；firstL0=23920ms (2026-04-19T06:26:51.896Z) |
| 恢复诊断 | raw=loss<3%=920ms (2026-04-19T06:26:28.896Z), rtt<120ms=1420ms (2026-04-19T06:26:29.396Z), jitter<28ms=18920ms (2026-04-19T06:26:46.896Z), jitter<18ms=18920ms (2026-04-19T06:26:46.896Z)；target=target>=120kbps=15920ms (2026-04-19T06:26:43.896Z), target>=300kbps=19920ms (2026-04-19T06:26:47.896Z), target>=500kbps=21420ms (2026-04-19T06:26:49.396Z), target>=700kbps=22920ms (2026-04-19T06:26:50.896Z), target>=900kbps=25423ms (2026-04-19T06:26:53.399Z)；send=send>=300kbps=19920ms (2026-04-19T06:26:47.896Z), send>=500kbps=21420ms (2026-04-19T06:26:49.396Z), send>=700kbps=21920ms (2026-04-19T06:26:49.896Z), send>=900kbps=23920ms (2026-04-19T06:26:51.896Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1939ms, recovering=-, stable=439ms, congested=6439ms, firstAction=1939ms, L0=3439ms, L1=1939ms, L2=6439ms, L3=6939ms, L4=7439ms, audioOnly=-；recovery: warning=26420ms, recovering=19420ms, stable=21920ms, congested=420ms, firstAction=420ms, L0=23920ms, L1=22420ms, L2=20920ms, L3=19420ms, L4=420ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 61 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T06:27:36.372Z；firstRecovering=19435ms (2026-04-19T06:27:55.807Z)；firstStable=435ms (2026-04-19T06:27:36.807Z)；firstL0=23935ms (2026-04-19T06:28:00.307Z) |
| 恢复诊断 | raw=loss<3%=435ms (2026-04-19T06:27:36.807Z), rtt<120ms=435ms (2026-04-19T06:27:36.807Z), jitter<28ms=435ms (2026-04-19T06:27:36.807Z), jitter<18ms=24935ms (2026-04-19T06:28:01.307Z)；target=target>=120kbps=13435ms (2026-04-19T06:27:49.807Z), target>=300kbps=19935ms (2026-04-19T06:27:56.307Z), target>=500kbps=21435ms (2026-04-19T06:27:57.807Z), target>=700kbps=22935ms (2026-04-19T06:27:59.307Z), target>=900kbps=24435ms (2026-04-19T06:28:00.807Z)；send=send>=300kbps=435ms (2026-04-19T06:27:36.807Z), send>=500kbps=435ms (2026-04-19T06:27:36.807Z), send>=700kbps=935ms (2026-04-19T06:27:37.307Z), send>=900kbps=935ms (2026-04-19T06:27:37.307Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4453ms, recovering=12953ms, stable=453ms, congested=4953ms, firstAction=4453ms, L0=17453ms, L1=4453ms, L2=4953ms, L3=5453ms, L4=5953ms, audioOnly=-；recovery: warning=1435ms, recovering=19435ms, stable=435ms, congested=1935ms, firstAction=1435ms, L0=23935ms, L1=1435ms, L2=1935ms, L3=2435ms, L4=2935ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:28:44.777Z；firstRecovering=-；firstStable=422ms (2026-04-19T06:28:45.199Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=921ms (2026-04-19T06:28:45.698Z), rtt<120ms=422ms (2026-04-19T06:28:45.199Z), jitter<28ms=422ms (2026-04-19T06:28:45.199Z), jitter<18ms=422ms (2026-04-19T06:28:45.199Z)；target=target>=120kbps=422ms (2026-04-19T06:28:45.199Z), target>=300kbps=422ms (2026-04-19T06:28:45.199Z), target>=500kbps=422ms (2026-04-19T06:28:45.199Z), target>=700kbps=422ms (2026-04-19T06:28:45.199Z), target>=900kbps=422ms (2026-04-19T06:28:45.199Z)；send=send>=300kbps=422ms (2026-04-19T06:28:45.199Z), send>=500kbps=422ms (2026-04-19T06:28:45.199Z), send>=700kbps=422ms (2026-04-19T06:28:45.199Z), send>=900kbps=422ms (2026-04-19T06:28:45.199Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=459ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=422ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:29:53.221Z；firstRecovering=-；firstStable=396ms (2026-04-19T06:29:53.617Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=396ms (2026-04-19T06:29:53.617Z), rtt<120ms=396ms (2026-04-19T06:29:53.617Z), jitter<28ms=396ms (2026-04-19T06:29:53.617Z), jitter<18ms=396ms (2026-04-19T06:29:53.617Z)；target=target>=120kbps=396ms (2026-04-19T06:29:53.617Z), target>=300kbps=396ms (2026-04-19T06:29:53.617Z), target>=500kbps=396ms (2026-04-19T06:29:53.617Z), target>=700kbps=396ms (2026-04-19T06:29:53.617Z), target>=900kbps=396ms (2026-04-19T06:29:53.617Z)；send=send>=300kbps=396ms (2026-04-19T06:29:53.617Z), send>=500kbps=396ms (2026-04-19T06:29:53.617Z), send>=700kbps=396ms (2026-04-19T06:29:53.617Z), send>=900kbps=396ms (2026-04-19T06:29:53.617Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=431ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=396ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:31:01.648Z；firstRecovering=-；firstStable=417ms (2026-04-19T06:31:02.065Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=917ms (2026-04-19T06:31:02.565Z), rtt<120ms=417ms (2026-04-19T06:31:02.065Z), jitter<28ms=417ms (2026-04-19T06:31:02.065Z), jitter<18ms=417ms (2026-04-19T06:31:02.065Z)；target=target>=120kbps=417ms (2026-04-19T06:31:02.065Z), target>=300kbps=417ms (2026-04-19T06:31:02.065Z), target>=500kbps=417ms (2026-04-19T06:31:02.065Z), target>=700kbps=417ms (2026-04-19T06:31:02.065Z), target>=900kbps=417ms (2026-04-19T06:31:02.065Z)；send=send>=300kbps=417ms (2026-04-19T06:31:02.065Z), send>=500kbps=417ms (2026-04-19T06:31:02.065Z), send>=700kbps=417ms (2026-04-19T06:31:02.065Z), send>=900kbps=417ms (2026-04-19T06:31:02.065Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=452ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=417ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T06:32:10.066Z；firstRecovering=-；firstStable=3416ms (2026-04-19T06:32:13.482Z)；firstL0=3416ms (2026-04-19T06:32:13.482Z) |
| 恢复诊断 | raw=loss<3%=416ms (2026-04-19T06:32:10.482Z), rtt<120ms=416ms (2026-04-19T06:32:10.482Z), jitter<28ms=416ms (2026-04-19T06:32:10.482Z), jitter<18ms=416ms (2026-04-19T06:32:10.482Z)；target=target>=120kbps=416ms (2026-04-19T06:32:10.482Z), target>=300kbps=416ms (2026-04-19T06:32:10.482Z), target>=500kbps=416ms (2026-04-19T06:32:10.482Z), target>=700kbps=416ms (2026-04-19T06:32:10.482Z), target>=900kbps=3916ms (2026-04-19T06:32:13.982Z)；send=send>=300kbps=416ms (2026-04-19T06:32:10.482Z), send>=500kbps=416ms (2026-04-19T06:32:10.482Z), send>=700kbps=416ms (2026-04-19T06:32:10.482Z), send>=900kbps=416ms (2026-04-19T06:32:10.482Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6455ms, recovering=-, stable=455ms, congested=-, firstAction=6455ms, L0=-, L1=6455ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=416ms, recovering=-, stable=3416ms, congested=-, firstAction=3416ms, L0=3416ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 前置 Case | - |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：Attempted to use detached Frame 'A5B48A4CDB3E9FE870094717A5E5CAA4'. |
| 实际 QoS 状态 | 执行错误：Attempted to use detached Frame 'A5B48A4CDB3E9FE870094717A5E5CAA4'. |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：Attempted to use detached Frame 'A5B48A4CDB3E9FE870094717A5E5CAA4'. |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 前置 Case | - |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 前置 Case | - |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 40% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 前置 Case | - |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 60% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R1

| 字段 | 内容 |
|---|---|
| Case ID | `R1` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 50ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R2

| 字段 | 内容 |
|---|---|
| Case ID | `R2` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 80ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 120ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 250ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 350ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J1

| 字段 | 内容 |
|---|---|
| Case ID | `J1` |
| 前置 Case | - |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 10ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J2

| 字段 | 内容 |
|---|---|
| Case ID | `J2` |
| 前置 Case | - |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 20ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 前置 Case | - |
| 类型 | `jitter_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 前置 Case | - |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 前置 Case | - |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 100ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T1

| 字段 | 内容 |
|---|---|
| Case ID | `T1` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 2000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 stable / early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 前置 Case | - |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 前置 Case | - |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 前置 Case | - |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| recovery 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 允许持续降级，不要求恢复；最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 前置 Case | - |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | - |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 8000ms / recovery 32000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 前置 Case | - |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 200ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 前置 Case | - |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | ERROR：The service is no longer running |
| 实际 QoS 状态 | 执行错误：The service is no longer running |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：The service is no longer running |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

