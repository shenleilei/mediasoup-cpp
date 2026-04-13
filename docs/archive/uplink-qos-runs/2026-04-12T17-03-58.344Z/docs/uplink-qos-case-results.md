# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T17:03:58.344Z`

## 1. 汇总

- 总 Case：`43`
- 已执行：`43`
- 通过：`43`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:07:05.789Z；firstRecovering=-；firstStable=141ms (2026-04-12T16:07:05.930Z)；firstL0=15141ms (2026-04-12T16:07:20.930Z) |
| 恢复诊断 | raw=loss<3%=141ms (2026-04-12T16:07:05.930Z), rtt<120ms=141ms (2026-04-12T16:07:05.930Z), jitter<28ms=141ms (2026-04-12T16:07:05.930Z), jitter<18ms=141ms (2026-04-12T16:07:05.930Z)；target=target>=120kbps=141ms (2026-04-12T16:07:05.930Z), target>=300kbps=141ms (2026-04-12T16:07:05.930Z), target>=500kbps=141ms (2026-04-12T16:07:05.930Z), target>=700kbps=141ms (2026-04-12T16:07:05.930Z), target>=900kbps=141ms (2026-04-12T16:07:05.930Z)；send=send>=300kbps=141ms (2026-04-12T16:07:05.930Z), send>=500kbps=141ms (2026-04-12T16:07:05.930Z), send>=700kbps=141ms (2026-04-12T16:07:05.930Z), send>=900kbps=141ms (2026-04-12T16:07:05.930Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=335ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=5141ms, recovering=-, stable=141ms, congested=-, firstAction=5141ms, L0=15141ms, L1=5141ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:08:14.642Z；firstRecovering=-；firstStable=269ms (2026-04-12T16:08:14.911Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=269ms (2026-04-12T16:08:14.911Z), rtt<120ms=269ms (2026-04-12T16:08:14.911Z), jitter<28ms=269ms (2026-04-12T16:08:14.911Z), jitter<18ms=269ms (2026-04-12T16:08:14.911Z)；target=target>=120kbps=269ms (2026-04-12T16:08:14.911Z), target>=300kbps=269ms (2026-04-12T16:08:14.911Z), target>=500kbps=269ms (2026-04-12T16:08:14.911Z), target>=700kbps=269ms (2026-04-12T16:08:14.911Z), target>=900kbps=269ms (2026-04-12T16:08:14.911Z)；send=send>=300kbps=269ms (2026-04-12T16:08:14.911Z), send>=500kbps=269ms (2026-04-12T16:08:14.911Z), send>=700kbps=269ms (2026-04-12T16:08:14.911Z), send>=900kbps=269ms (2026-04-12T16:08:14.911Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=387ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=269ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L1) |
| 实际动作 | setEncodingParameters（共 103 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=stable/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-12T16:09:23.901Z；firstRecovering=3303ms (2026-04-12T16:09:27.204Z)；firstStable=5803ms (2026-04-12T16:09:29.704Z)；firstL0=7803ms (2026-04-12T16:09:31.704Z) |
| 恢复诊断 | raw=loss<3%=803ms (2026-04-12T16:09:24.704Z), rtt<120ms=303ms (2026-04-12T16:09:24.204Z), jitter<28ms=303ms (2026-04-12T16:09:24.204Z), jitter<18ms=-；target=target>=120kbps=3803ms (2026-04-12T16:09:27.704Z), target>=300kbps=3803ms (2026-04-12T16:09:27.704Z), target>=500kbps=28303ms (2026-04-12T16:09:52.204Z), target>=700kbps=29803ms (2026-04-12T16:09:53.704Z), target>=900kbps=-；send=send>=300kbps=3803ms (2026-04-12T16:09:27.704Z), send>=500kbps=3803ms (2026-04-12T16:09:27.704Z), send>=700kbps=28303ms (2026-04-12T16:09:52.204Z), send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2185ms, recovering=-, stable=179ms, congested=2679ms, firstAction=2185ms, L0=-, L1=2185ms, L2=2679ms, L3=3179ms, L4=3679ms, audioOnly=-；recovery: warning=10303ms, recovering=3303ms, stable=5803ms, congested=303ms, firstAction=303ms, L0=7803ms, L1=6303ms, L2=4803ms, L3=3303ms, L4=303ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:10:33.348Z；firstRecovering=-；firstStable=137ms (2026-04-12T16:10:33.485Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=137ms (2026-04-12T16:10:33.485Z), rtt<120ms=137ms (2026-04-12T16:10:33.485Z), jitter<28ms=137ms (2026-04-12T16:10:33.485Z), jitter<18ms=137ms (2026-04-12T16:10:33.485Z)；target=target>=120kbps=137ms (2026-04-12T16:10:33.485Z), target>=300kbps=137ms (2026-04-12T16:10:33.485Z), target>=500kbps=137ms (2026-04-12T16:10:33.485Z), target>=700kbps=137ms (2026-04-12T16:10:33.485Z), target>=900kbps=137ms (2026-04-12T16:10:33.485Z)；send=send>=300kbps=137ms (2026-04-12T16:10:33.485Z), send>=500kbps=137ms (2026-04-12T16:10:33.485Z), send>=700kbps=137ms (2026-04-12T16:10:33.485Z), send>=900kbps=137ms (2026-04-12T16:10:33.485Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=355ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=137ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 79 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:11:42.595Z；firstRecovering=18899ms (2026-04-12T16:12:01.494Z)；firstStable=20899ms (2026-04-12T16:12:03.494Z)；firstL0=23398ms (2026-04-12T16:12:05.993Z) |
| 恢复诊断 | raw=loss<3%=401ms (2026-04-12T16:11:42.996Z), rtt<120ms=401ms (2026-04-12T16:11:42.996Z), jitter<28ms=14899ms (2026-04-12T16:11:57.494Z), jitter<18ms=15899ms (2026-04-12T16:11:58.494Z)；target=target>=120kbps=401ms (2026-04-12T16:11:42.996Z), target>=300kbps=19399ms (2026-04-12T16:12:01.994Z), target>=500kbps=20899ms (2026-04-12T16:12:03.494Z), target>=700kbps=22399ms (2026-04-12T16:12:04.994Z), target>=900kbps=23898ms (2026-04-12T16:12:06.493Z)；send=send>=300kbps=19399ms (2026-04-12T16:12:01.994Z), send>=500kbps=20899ms (2026-04-12T16:12:03.494Z), send>=700kbps=22399ms (2026-04-12T16:12:04.994Z), send>=900kbps=22399ms (2026-04-12T16:12:04.994Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1839ms, recovering=-, stable=339ms, congested=2339ms, firstAction=1839ms, L0=-, L1=1839ms, L2=2339ms, L3=2840ms, L4=3339ms, audioOnly=-；recovery: warning=-, recovering=18899ms, stable=20899ms, congested=401ms, firstAction=401ms, L0=23398ms, L1=21899ms, L2=20398ms, L3=18899ms, L4=401ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 78 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:12:51.944Z；firstRecovering=18740ms (2026-04-12T16:13:10.684Z)；firstStable=21240ms (2026-04-12T16:13:13.184Z)；firstL0=23240ms (2026-04-12T16:13:15.184Z) |
| 恢复诊断 | raw=loss<3%=240ms (2026-04-12T16:12:52.184Z), rtt<120ms=240ms (2026-04-12T16:12:52.184Z), jitter<28ms=14740ms (2026-04-12T16:13:06.684Z), jitter<18ms=15240ms (2026-04-12T16:13:07.184Z)；target=target>=120kbps=12240ms (2026-04-12T16:13:04.184Z), target>=300kbps=19240ms (2026-04-12T16:13:11.184Z), target>=500kbps=20740ms (2026-04-12T16:13:12.684Z), target>=700kbps=22240ms (2026-04-12T16:13:14.184Z), target>=900kbps=25240ms (2026-04-12T16:13:17.184Z)；send=send>=300kbps=19240ms (2026-04-12T16:13:11.184Z), send>=500kbps=20740ms (2026-04-12T16:13:12.684Z), send>=700kbps=20740ms (2026-04-12T16:13:12.684Z), send>=900kbps=22240ms (2026-04-12T16:13:14.184Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2331ms, recovering=-, stable=331ms, congested=2831ms, firstAction=2331ms, L0=-, L1=2331ms, L2=2831ms, L3=3331ms, L4=3831ms, audioOnly=-；recovery: warning=-, recovering=18740ms, stable=21240ms, congested=240ms, firstAction=240ms, L0=23240ms, L1=21740ms, L2=20240ms, L3=18740ms, L4=240ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 95 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:14:09.227Z；firstRecovering=20818ms (2026-04-12T16:14:30.045Z)；firstStable=21818ms (2026-04-12T16:14:31.045Z)；firstL0=25318ms (2026-04-12T16:14:34.545Z) |
| 恢复诊断 | raw=loss<3%=318ms (2026-04-12T16:14:09.545Z), rtt<120ms=318ms (2026-04-12T16:14:09.545Z), jitter<28ms=14318ms (2026-04-12T16:14:23.545Z), jitter<18ms=16318ms (2026-04-12T16:14:25.545Z)；target=target>=120kbps=2318ms (2026-04-12T16:14:11.545Z), target>=300kbps=21319ms (2026-04-12T16:14:30.546Z), target>=500kbps=22818ms (2026-04-12T16:14:32.045Z), target>=700kbps=24318ms (2026-04-12T16:14:33.545Z), target>=900kbps=25818ms (2026-04-12T16:14:35.045Z)；send=send>=300kbps=21319ms (2026-04-12T16:14:30.546Z), send>=500kbps=23318ms (2026-04-12T16:14:32.545Z), send>=700kbps=23318ms (2026-04-12T16:14:32.545Z), send>=900kbps=24318ms (2026-04-12T16:14:33.545Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3830ms, recovering=-, stable=330ms, congested=4330ms, firstAction=3830ms, L0=-, L1=3830ms, L2=4330ms, L3=4830ms, L4=5330ms, audioOnly=-；recovery: warning=-, recovering=20818ms, stable=21818ms, congested=318ms, firstAction=318ms, L0=25318ms, L1=23818ms, L2=22318ms, L3=20818ms, L4=318ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 129 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-12T16:15:36.170Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=122ms (2026-04-12T16:15:36.292Z), rtt<120ms=2622ms (2026-04-12T16:15:38.792Z), jitter<28ms=14622ms (2026-04-12T16:15:50.792Z), jitter<18ms=16122ms (2026-04-12T16:15:52.292Z)；target=target>=120kbps=12122ms (2026-04-12T16:15:48.292Z), target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=622ms (2026-04-12T16:15:36.792Z), send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2802ms, recovering=-, stable=302ms, congested=4302ms, firstAction=2802ms, L0=-, L1=2802ms, L2=4302ms, L3=4802ms, L4=5302ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=122ms, firstAction=122ms, L0=-, L1=-, L2=-, L3=-, L4=122ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 179 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-12T16:17:28.481Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=789ms (2026-04-12T16:17:29.270Z), rtt<120ms=289ms (2026-04-12T16:17:28.770Z), jitter<28ms=28289ms (2026-04-12T16:17:56.770Z), jitter<18ms=29789ms (2026-04-12T16:17:58.270Z)；target=target>=120kbps=11289ms (2026-04-12T16:17:39.770Z), target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=789ms (2026-04-12T16:17:29.270Z), send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4313ms, recovering=-, stable=313ms, congested=4813ms, firstAction=4313ms, L0=-, L1=4313ms, L2=4813ms, L3=5313ms, L4=5813ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=289ms, firstAction=289ms, L0=-, L1=-, L2=-, L3=-, L4=289ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:18:37.566Z；firstRecovering=-；firstStable=118ms (2026-04-12T16:18:37.684Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=118ms (2026-04-12T16:18:37.684Z), rtt<120ms=118ms (2026-04-12T16:18:37.684Z), jitter<28ms=118ms (2026-04-12T16:18:37.684Z), jitter<18ms=118ms (2026-04-12T16:18:37.684Z)；target=target>=120kbps=118ms (2026-04-12T16:18:37.684Z), target>=300kbps=118ms (2026-04-12T16:18:37.684Z), target>=500kbps=118ms (2026-04-12T16:18:37.684Z), target>=700kbps=118ms (2026-04-12T16:18:37.684Z), target>=900kbps=118ms (2026-04-12T16:18:37.684Z)；send=send>=300kbps=118ms (2026-04-12T16:18:37.684Z), send>=500kbps=118ms (2026-04-12T16:18:37.684Z), send>=700kbps=118ms (2026-04-12T16:18:37.684Z), send>=900kbps=118ms (2026-04-12T16:18:37.684Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=335ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=118ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:19:46.644Z；firstRecovering=-；firstStable=87ms (2026-04-12T16:19:46.731Z)；firstL0=2587ms (2026-04-12T16:19:49.231Z) |
| 恢复诊断 | raw=loss<3%=87ms (2026-04-12T16:19:46.731Z), rtt<120ms=87ms (2026-04-12T16:19:46.731Z), jitter<28ms=87ms (2026-04-12T16:19:46.731Z), jitter<18ms=87ms (2026-04-12T16:19:46.731Z)；target=target>=120kbps=87ms (2026-04-12T16:19:46.731Z), target>=300kbps=87ms (2026-04-12T16:19:46.731Z), target>=500kbps=87ms (2026-04-12T16:19:46.731Z), target>=700kbps=87ms (2026-04-12T16:19:46.731Z), target>=900kbps=87ms (2026-04-12T16:19:46.731Z)；send=send>=300kbps=87ms (2026-04-12T16:19:46.731Z), send>=500kbps=87ms (2026-04-12T16:19:46.731Z), send>=700kbps=87ms (2026-04-12T16:19:46.731Z), send>=900kbps=87ms (2026-04-12T16:19:46.731Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=283ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=1087ms, recovering=-, stable=87ms, congested=-, firstAction=1087ms, L0=2587ms, L1=1087ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:20:55.590Z；firstRecovering=-；firstStable=159ms (2026-04-12T16:20:55.749Z)；firstL0=16687ms (2026-04-12T16:21:12.277Z) |
| 恢复诊断 | raw=loss<3%=159ms (2026-04-12T16:20:55.749Z), rtt<120ms=159ms (2026-04-12T16:20:55.749Z), jitter<28ms=159ms (2026-04-12T16:20:55.749Z), jitter<18ms=159ms (2026-04-12T16:20:55.749Z)；target=target>=120kbps=159ms (2026-04-12T16:20:55.749Z), target>=300kbps=159ms (2026-04-12T16:20:55.749Z), target>=500kbps=159ms (2026-04-12T16:20:55.749Z), target>=700kbps=159ms (2026-04-12T16:20:55.749Z), target>=900kbps=159ms (2026-04-12T16:20:55.749Z)；send=send>=300kbps=159ms (2026-04-12T16:20:55.749Z), send>=500kbps=159ms (2026-04-12T16:20:55.749Z), send>=700kbps=159ms (2026-04-12T16:20:55.749Z), send>=900kbps=159ms (2026-04-12T16:20:55.749Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=381ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=1159ms, recovering=-, stable=159ms, congested=-, firstAction=1159ms, L0=16687ms, L1=1159ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:22:04.774Z；firstRecovering=-；firstStable=14453ms (2026-04-12T16:22:19.227Z)；firstL0=14453ms (2026-04-12T16:22:19.227Z) |
| 恢复诊断 | raw=loss<3%=953ms (2026-04-12T16:22:05.727Z), rtt<120ms=453ms (2026-04-12T16:22:05.227Z), jitter<28ms=453ms (2026-04-12T16:22:05.227Z), jitter<18ms=453ms (2026-04-12T16:22:05.227Z)；target=target>=120kbps=453ms (2026-04-12T16:22:05.227Z), target>=300kbps=453ms (2026-04-12T16:22:05.227Z), target>=500kbps=453ms (2026-04-12T16:22:05.227Z), target>=700kbps=453ms (2026-04-12T16:22:05.227Z), target>=900kbps=14953ms (2026-04-12T16:22:19.727Z)；send=send>=300kbps=453ms (2026-04-12T16:22:05.227Z), send>=500kbps=453ms (2026-04-12T16:22:05.227Z), send>=700kbps=453ms (2026-04-12T16:22:05.227Z), send>=900kbps=453ms (2026-04-12T16:22:05.227Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2350ms, recovering=-, stable=350ms, congested=-, firstAction=2350ms, L0=4350ms, L1=2350ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=453ms, recovering=-, stable=14453ms, congested=-, firstAction=14453ms, L0=14453ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 44 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:23:13.766Z；firstRecovering=3652ms (2026-04-12T16:23:17.418Z)；firstStable=4152ms (2026-04-12T16:23:17.918Z)；firstL0=8148ms (2026-04-12T16:23:21.914Z) |
| 恢复诊断 | raw=loss<3%=652ms (2026-04-12T16:23:14.418Z), rtt<120ms=152ms (2026-04-12T16:23:13.918Z), jitter<28ms=152ms (2026-04-12T16:23:13.918Z), jitter<18ms=152ms (2026-04-12T16:23:13.918Z)；target=target>=120kbps=4152ms (2026-04-12T16:23:17.918Z), target>=300kbps=5651ms (2026-04-12T16:23:19.417Z), target>=500kbps=7153ms (2026-04-12T16:23:20.919Z), target>=700kbps=7153ms (2026-04-12T16:23:20.919Z), target>=900kbps=8655ms (2026-04-12T16:23:22.421Z)；send=send>=300kbps=4152ms (2026-04-12T16:23:17.918Z), send>=500kbps=4152ms (2026-04-12T16:23:17.918Z), send>=700kbps=4152ms (2026-04-12T16:23:17.918Z), send>=900kbps=5651ms (2026-04-12T16:23:19.417Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1338ms, recovering=-, stable=345ms, congested=4343ms, firstAction=1338ms, L0=-, L1=1338ms, L2=4343ms, L3=4842ms, L4=5342ms, audioOnly=-；recovery: warning=-, recovering=3652ms, stable=4152ms, congested=152ms, firstAction=152ms, L0=8148ms, L1=6652ms, L2=5151ms, L3=3652ms, L4=152ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 60 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:24:23.204Z；firstRecovering=10677ms (2026-04-12T16:24:33.881Z)；firstStable=13177ms (2026-04-12T16:24:36.381Z)；firstL0=15177ms (2026-04-12T16:24:38.381Z) |
| 恢复诊断 | raw=loss<3%=1178ms (2026-04-12T16:24:24.382Z), rtt<120ms=178ms (2026-04-12T16:24:23.382Z), jitter<28ms=6178ms (2026-04-12T16:24:29.382Z), jitter<18ms=7678ms (2026-04-12T16:24:30.882Z)；target=target>=120kbps=4678ms (2026-04-12T16:24:27.882Z), target>=300kbps=11178ms (2026-04-12T16:24:34.382Z), target>=500kbps=13177ms (2026-04-12T16:24:36.381Z), target>=700kbps=14179ms (2026-04-12T16:24:37.383Z), target>=900kbps=15678ms (2026-04-12T16:24:38.882Z)；send=send>=300kbps=11178ms (2026-04-12T16:24:34.382Z), send>=500kbps=11178ms (2026-04-12T16:24:34.382Z), send>=700kbps=11178ms (2026-04-12T16:24:34.382Z), send>=900kbps=14179ms (2026-04-12T16:24:37.383Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1598ms, recovering=-, stable=97ms, congested=3595ms, firstAction=1598ms, L0=-, L1=1598ms, L2=3595ms, L3=4095ms, L4=4596ms, audioOnly=-；recovery: warning=-, recovering=10677ms, stable=13177ms, congested=178ms, firstAction=178ms, L0=15177ms, L1=13678ms, L2=12177ms, L3=10677ms, L4=178ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:25:32.549Z；firstRecovering=23281ms (2026-04-12T16:25:55.830Z)；firstStable=25280ms (2026-04-12T16:25:57.829Z)；firstL0=27783ms (2026-04-12T16:26:00.332Z) |
| 恢复诊断 | raw=loss<3%=280ms (2026-04-12T16:25:32.829Z), rtt<120ms=280ms (2026-04-12T16:25:32.829Z), jitter<28ms=17280ms (2026-04-12T16:25:49.829Z), jitter<18ms=17280ms (2026-04-12T16:25:49.829Z)；target=target>=120kbps=11780ms (2026-04-12T16:25:44.329Z), target>=300kbps=23781ms (2026-04-12T16:25:56.330Z), target>=500kbps=25280ms (2026-04-12T16:25:57.829Z), target>=700kbps=26782ms (2026-04-12T16:25:59.331Z), target>=900kbps=28281ms (2026-04-12T16:26:00.830Z)；send=send>=300kbps=23781ms (2026-04-12T16:25:56.330Z), send>=500kbps=24283ms (2026-04-12T16:25:56.832Z), send>=700kbps=25280ms (2026-04-12T16:25:57.829Z), send>=900kbps=25280ms (2026-04-12T16:25:57.829Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1848ms, recovering=-, stable=351ms, congested=5348ms, firstAction=1848ms, L0=3349ms, L1=1848ms, L2=5848ms, L3=6347ms, L4=6848ms, audioOnly=-；recovery: warning=-, recovering=23281ms, stable=25280ms, congested=280ms, firstAction=280ms, L0=27783ms, L1=26279ms, L2=24780ms, L3=23281ms, L4=280ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 121 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:26:57.554Z；firstRecovering=24542ms (2026-04-12T16:27:22.096Z)；firstStable=26042ms (2026-04-12T16:27:23.596Z)；firstL0=29042ms (2026-04-12T16:27:26.596Z) |
| 恢复诊断 | raw=loss<3%=42ms (2026-04-12T16:26:57.596Z), rtt<120ms=42ms (2026-04-12T16:26:57.596Z), jitter<28ms=17042ms (2026-04-12T16:27:14.596Z), jitter<18ms=19542ms (2026-04-12T16:27:17.096Z)；target=target>=120kbps=17042ms (2026-04-12T16:27:14.596Z), target>=300kbps=25043ms (2026-04-12T16:27:22.597Z), target>=500kbps=26542ms (2026-04-12T16:27:24.096Z), target>=700kbps=28042ms (2026-04-12T16:27:25.596Z), target>=900kbps=29542ms (2026-04-12T16:27:27.096Z)；send=send>=300kbps=25043ms (2026-04-12T16:27:22.597Z), send>=500kbps=25043ms (2026-04-12T16:27:22.597Z), send>=700kbps=26542ms (2026-04-12T16:27:24.096Z), send>=900kbps=26542ms (2026-04-12T16:27:24.096Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2336ms, recovering=-, stable=338ms, congested=2838ms, firstAction=2336ms, L0=-, L1=2336ms, L2=2838ms, L3=3336ms, L4=3836ms, audioOnly=-；recovery: warning=-, recovering=24542ms, stable=26042ms, congested=42ms, firstAction=42ms, L0=29042ms, L1=27542ms, L2=26042ms, L3=24542ms, L4=42ms, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:28:06.544Z；firstRecovering=-；firstStable=116ms (2026-04-12T16:28:06.660Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=116ms (2026-04-12T16:28:06.660Z), rtt<120ms=116ms (2026-04-12T16:28:06.660Z), jitter<28ms=116ms (2026-04-12T16:28:06.660Z), jitter<18ms=116ms (2026-04-12T16:28:06.660Z)；target=target>=120kbps=116ms (2026-04-12T16:28:06.660Z), target>=300kbps=116ms (2026-04-12T16:28:06.660Z), target>=500kbps=116ms (2026-04-12T16:28:06.660Z), target>=700kbps=116ms (2026-04-12T16:28:06.660Z), target>=900kbps=116ms (2026-04-12T16:28:06.660Z)；send=send>=300kbps=116ms (2026-04-12T16:28:06.660Z), send>=500kbps=116ms (2026-04-12T16:28:06.660Z), send>=700kbps=116ms (2026-04-12T16:28:06.660Z), send>=900kbps=116ms (2026-04-12T16:28:06.660Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=332ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=116ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:29:15.622Z；firstRecovering=-；firstStable=40ms (2026-04-12T16:29:15.662Z)；firstL0=18040ms (2026-04-12T16:29:33.662Z) |
| 恢复诊断 | raw=loss<3%=40ms (2026-04-12T16:29:15.662Z), rtt<120ms=40ms (2026-04-12T16:29:15.662Z), jitter<28ms=40ms (2026-04-12T16:29:15.662Z), jitter<18ms=40ms (2026-04-12T16:29:15.662Z)；target=target>=120kbps=40ms (2026-04-12T16:29:15.662Z), target>=300kbps=40ms (2026-04-12T16:29:15.662Z), target>=500kbps=40ms (2026-04-12T16:29:15.662Z), target>=700kbps=40ms (2026-04-12T16:29:15.662Z), target>=900kbps=18540ms (2026-04-12T16:29:34.162Z)；send=send>=300kbps=40ms (2026-04-12T16:29:15.662Z), send>=500kbps=40ms (2026-04-12T16:29:15.662Z), send>=700kbps=40ms (2026-04-12T16:29:15.662Z), send>=900kbps=40ms (2026-04-12T16:29:15.662Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3796ms, recovering=-, stable=296ms, congested=-, firstAction=3796ms, L0=19825ms, L1=3796ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=2040ms, recovering=-, stable=40ms, congested=-, firstAction=2040ms, L0=18040ms, L1=2040ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:30:24.793Z；firstRecovering=-；firstStable=10464ms (2026-04-12T16:30:35.257Z)；firstL0=10464ms (2026-04-12T16:30:35.257Z) |
| 恢复诊断 | raw=loss<3%=464ms (2026-04-12T16:30:25.257Z), rtt<120ms=964ms (2026-04-12T16:30:25.757Z), jitter<28ms=464ms (2026-04-12T16:30:25.257Z), jitter<18ms=464ms (2026-04-12T16:30:25.257Z)；target=target>=120kbps=464ms (2026-04-12T16:30:25.257Z), target>=300kbps=464ms (2026-04-12T16:30:25.257Z), target>=500kbps=464ms (2026-04-12T16:30:25.257Z), target>=700kbps=1464ms (2026-04-12T16:30:26.257Z), target>=900kbps=10964ms (2026-04-12T16:30:35.757Z)；send=send>=300kbps=464ms (2026-04-12T16:30:25.257Z), send>=500kbps=464ms (2026-04-12T16:30:25.257Z), send>=700kbps=464ms (2026-04-12T16:30:25.257Z), send>=900kbps=2464ms (2026-04-12T16:30:27.257Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=19772ms, recovering=-, stable=272ms, congested=-, firstAction=19772ms, L0=-, L1=19772ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=464ms, recovering=-, stable=10464ms, congested=-, firstAction=10464ms, L0=10464ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:31:34.098Z；firstRecovering=-；firstStable=15824ms (2026-04-12T16:31:49.922Z)；firstL0=15824ms (2026-04-12T16:31:49.922Z) |
| 恢复诊断 | raw=loss<3%=324ms (2026-04-12T16:31:34.422Z), rtt<120ms=1324ms (2026-04-12T16:31:35.422Z), jitter<28ms=324ms (2026-04-12T16:31:34.422Z), jitter<18ms=324ms (2026-04-12T16:31:34.422Z)；target=target>=120kbps=324ms (2026-04-12T16:31:34.422Z), target>=300kbps=324ms (2026-04-12T16:31:34.422Z), target>=500kbps=324ms (2026-04-12T16:31:34.422Z), target>=700kbps=324ms (2026-04-12T16:31:34.422Z), target>=900kbps=16324ms (2026-04-12T16:31:50.422Z)；send=send>=300kbps=324ms (2026-04-12T16:31:34.422Z), send>=500kbps=324ms (2026-04-12T16:31:34.422Z), send>=700kbps=324ms (2026-04-12T16:31:34.422Z), send>=900kbps=324ms (2026-04-12T16:31:34.422Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1241ms, recovering=-, stable=241ms, congested=-, firstAction=1241ms, L0=-, L1=1241ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=324ms, recovering=-, stable=15824ms, congested=-, firstAction=15824ms, L0=15824ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 71 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:32:43.578Z；firstRecovering=4675ms (2026-04-12T16:32:48.253Z)；firstStable=5175ms (2026-04-12T16:32:48.753Z)；firstL0=9189ms (2026-04-12T16:32:52.767Z) |
| 恢复诊断 | raw=loss<3%=174ms (2026-04-12T16:32:43.752Z), rtt<120ms=1175ms (2026-04-12T16:32:44.753Z), jitter<28ms=174ms (2026-04-12T16:32:43.752Z), jitter<18ms=174ms (2026-04-12T16:32:43.752Z)；target=target>=120kbps=174ms (2026-04-12T16:32:43.752Z), target>=300kbps=6675ms (2026-04-12T16:32:50.253Z), target>=500kbps=6675ms (2026-04-12T16:32:50.253Z), target>=700kbps=11175ms (2026-04-12T16:32:54.753Z), target>=900kbps=25674ms (2026-04-12T16:33:09.252Z)；send=send>=300kbps=5175ms (2026-04-12T16:32:48.753Z), send>=500kbps=5175ms (2026-04-12T16:32:48.753Z), send>=700kbps=5175ms (2026-04-12T16:32:48.753Z), send>=900kbps=5674ms (2026-04-12T16:32:49.252Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1690ms, recovering=-, stable=189ms, congested=3189ms, firstAction=1690ms, L0=-, L1=1690ms, L2=3189ms, L3=3690ms, L4=4190ms, audioOnly=-；recovery: warning=11674ms, recovering=4675ms, stable=5175ms, congested=174ms, firstAction=174ms, L0=9189ms, L1=7674ms, L2=6175ms, L3=4675ms, L4=174ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:33:53.197Z；firstRecovering=3988ms (2026-04-12T16:33:57.185Z)；firstStable=4488ms (2026-04-12T16:33:57.685Z)；firstL0=8490ms (2026-04-12T16:34:01.687Z) |
| 恢复诊断 | raw=loss<3%=988ms (2026-04-12T16:33:54.185Z), rtt<120ms=988ms (2026-04-12T16:33:54.185Z), jitter<28ms=488ms (2026-04-12T16:33:53.685Z), jitter<18ms=488ms (2026-04-12T16:33:53.685Z)；target=target>=120kbps=1989ms (2026-04-12T16:33:55.186Z), target>=300kbps=4488ms (2026-04-12T16:33:57.685Z), target>=500kbps=7488ms (2026-04-12T16:34:00.685Z), target>=700kbps=11489ms (2026-04-12T16:34:04.686Z), target>=900kbps=25999ms (2026-04-12T16:34:19.196Z)；send=send>=300kbps=4488ms (2026-04-12T16:33:57.685Z), send>=500kbps=4488ms (2026-04-12T16:33:57.685Z), send>=700kbps=4488ms (2026-04-12T16:33:57.685Z), send>=900kbps=5996ms (2026-04-12T16:33:59.193Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1158ms, recovering=-, stable=158ms, congested=1658ms, firstAction=1158ms, L0=-, L1=1158ms, L2=1658ms, L3=2158ms, L4=2659ms, audioOnly=-；recovery: warning=10990ms, recovering=3988ms, stable=4488ms, congested=488ms, firstAction=488ms, L0=8490ms, L1=6988ms, L2=5490ms, L3=3988ms, L4=488ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:35:02.195Z；firstRecovering=-；firstStable=133ms (2026-04-12T16:35:02.328Z)；firstL0=3633ms (2026-04-12T16:35:05.828Z) |
| 恢复诊断 | raw=loss<3%=133ms (2026-04-12T16:35:02.328Z), rtt<120ms=133ms (2026-04-12T16:35:02.328Z), jitter<28ms=133ms (2026-04-12T16:35:02.328Z), jitter<18ms=133ms (2026-04-12T16:35:02.328Z)；target=target>=120kbps=133ms (2026-04-12T16:35:02.328Z), target>=300kbps=133ms (2026-04-12T16:35:02.328Z), target>=500kbps=133ms (2026-04-12T16:35:02.328Z), target>=700kbps=133ms (2026-04-12T16:35:02.328Z), target>=900kbps=133ms (2026-04-12T16:35:02.328Z)；send=send>=300kbps=133ms (2026-04-12T16:35:02.328Z), send>=500kbps=133ms (2026-04-12T16:35:02.328Z), send>=700kbps=133ms (2026-04-12T16:35:02.328Z), send>=900kbps=133ms (2026-04-12T16:35:02.328Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=331ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=2133ms, recovering=-, stable=133ms, congested=-, firstAction=2133ms, L0=3633ms, L1=2133ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:36:11.192Z；firstRecovering=-；firstStable=121ms (2026-04-12T16:36:11.313Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=121ms (2026-04-12T16:36:11.313Z), rtt<120ms=121ms (2026-04-12T16:36:11.313Z), jitter<28ms=121ms (2026-04-12T16:36:11.313Z), jitter<18ms=1621ms (2026-04-12T16:36:12.813Z)；target=target>=120kbps=121ms (2026-04-12T16:36:11.313Z), target>=300kbps=121ms (2026-04-12T16:36:11.313Z), target>=500kbps=121ms (2026-04-12T16:36:11.313Z), target>=700kbps=121ms (2026-04-12T16:36:11.313Z), target>=900kbps=121ms (2026-04-12T16:36:11.313Z)；send=send>=300kbps=121ms (2026-04-12T16:36:11.313Z), send>=500kbps=121ms (2026-04-12T16:36:11.313Z), send>=700kbps=121ms (2026-04-12T16:36:11.313Z), send>=900kbps=121ms (2026-04-12T16:36:11.313Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=340ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=121ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:37:20.301Z；firstRecovering=-；firstStable=15505ms (2026-04-12T16:37:35.806Z)；firstL0=15505ms (2026-04-12T16:37:35.806Z) |
| 恢复诊断 | raw=loss<3%=5ms (2026-04-12T16:37:20.306Z), rtt<120ms=5ms (2026-04-12T16:37:20.306Z), jitter<28ms=5ms (2026-04-12T16:37:20.306Z), jitter<18ms=12505ms (2026-04-12T16:37:32.806Z)；target=target>=120kbps=5ms (2026-04-12T16:37:20.306Z), target>=300kbps=5ms (2026-04-12T16:37:20.306Z), target>=500kbps=5ms (2026-04-12T16:37:20.306Z), target>=700kbps=5ms (2026-04-12T16:37:20.306Z), target>=900kbps=16005ms (2026-04-12T16:37:36.306Z)；send=send>=300kbps=5ms (2026-04-12T16:37:20.306Z), send>=500kbps=5ms (2026-04-12T16:37:20.306Z), send>=700kbps=5ms (2026-04-12T16:37:20.306Z), send>=900kbps=505ms (2026-04-12T16:37:20.806Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2328ms, recovering=-, stable=328ms, congested=-, firstAction=2328ms, L0=-, L1=2328ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=5ms, recovering=-, stable=15505ms, congested=-, firstAction=15505ms, L0=15505ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:38:29.330Z；firstRecovering=5087ms (2026-04-12T16:38:34.417Z)；firstStable=7087ms (2026-04-12T16:38:36.417Z)；firstL0=9587ms (2026-04-12T16:38:38.917Z) |
| 恢复诊断 | raw=loss<3%=87ms (2026-04-12T16:38:29.417Z), rtt<120ms=87ms (2026-04-12T16:38:29.417Z), jitter<28ms=2087ms (2026-04-12T16:38:31.417Z), jitter<18ms=3087ms (2026-04-12T16:38:32.417Z)；target=target>=120kbps=87ms (2026-04-12T16:38:29.417Z), target>=300kbps=5587ms (2026-04-12T16:38:34.917Z), target>=500kbps=7087ms (2026-04-12T16:38:36.417Z), target>=700kbps=8590ms (2026-04-12T16:38:37.920Z), target>=900kbps=10087ms (2026-04-12T16:38:39.417Z)；send=send>=300kbps=5587ms (2026-04-12T16:38:34.917Z), send>=500kbps=7087ms (2026-04-12T16:38:36.417Z), send>=700kbps=7087ms (2026-04-12T16:38:36.417Z), send>=900kbps=8590ms (2026-04-12T16:38:37.920Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1849ms, recovering=-, stable=349ms, congested=2886ms, firstAction=1849ms, L0=-, L1=1849ms, L2=2886ms, L3=3350ms, L4=3849ms, audioOnly=-；recovery: warning=-, recovering=5087ms, stable=7087ms, congested=87ms, firstAction=87ms, L0=9587ms, L1=8087ms, L2=6587ms, L3=5087ms, L4=87ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L1) |
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=stable/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-12T16:39:38.704Z；firstRecovering=7218ms (2026-04-12T16:39:45.922Z)；firstStable=9726ms (2026-04-12T16:39:48.430Z)；firstL0=11719ms (2026-04-12T16:39:50.423Z) |
| 恢复诊断 | raw=loss<3%=217ms (2026-04-12T16:39:38.921Z), rtt<120ms=1218ms (2026-04-12T16:39:39.922Z), jitter<28ms=3718ms (2026-04-12T16:39:42.422Z), jitter<18ms=5218ms (2026-04-12T16:39:43.922Z)；target=target>=120kbps=217ms (2026-04-12T16:39:38.921Z), target>=300kbps=7718ms (2026-04-12T16:39:46.422Z), target>=500kbps=12218ms (2026-04-12T16:39:50.922Z), target>=700kbps=29725ms (2026-04-12T16:40:08.429Z), target>=900kbps=-；send=send>=300kbps=7718ms (2026-04-12T16:39:46.422Z), send>=500kbps=7718ms (2026-04-12T16:39:46.422Z), send>=700kbps=10718ms (2026-04-12T16:39:49.422Z), send>=900kbps=29725ms (2026-04-12T16:40:08.429Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1699ms, recovering=-, stable=199ms, congested=2198ms, firstAction=1699ms, L0=-, L1=1699ms, L2=2198ms, L3=2699ms, L4=3198ms, audioOnly=-；recovery: warning=14225ms, recovering=7218ms, stable=9726ms, congested=217ms, firstAction=217ms, L0=11719ms, L1=10218ms, L2=8717ms, L3=7218ms, L4=217ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 66 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:40:47.741Z；firstRecovering=16057ms (2026-04-12T16:41:03.798Z)；firstStable=18057ms (2026-04-12T16:41:05.798Z)；firstL0=20557ms (2026-04-12T16:41:08.298Z) |
| 恢复诊断 | raw=loss<3%=57ms (2026-04-12T16:40:47.798Z), rtt<120ms=57ms (2026-04-12T16:40:47.798Z), jitter<28ms=7057ms (2026-04-12T16:40:54.798Z), jitter<18ms=14057ms (2026-04-12T16:41:01.798Z)；target=target>=120kbps=57ms (2026-04-12T16:40:47.798Z), target>=300kbps=16557ms (2026-04-12T16:41:04.298Z), target>=500kbps=18057ms (2026-04-12T16:41:05.798Z), target>=700kbps=19557ms (2026-04-12T16:41:07.298Z), target>=900kbps=21057ms (2026-04-12T16:41:08.798Z)；send=send>=300kbps=16557ms (2026-04-12T16:41:04.298Z), send>=500kbps=18057ms (2026-04-12T16:41:05.798Z), send>=700kbps=18557ms (2026-04-12T16:41:06.298Z), send>=900kbps=18557ms (2026-04-12T16:41:06.298Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1334ms, recovering=13834ms, stable=334ms, congested=2334ms, firstAction=1334ms, L0=-, L1=1334ms, L2=2334ms, L3=2834ms, L4=3334ms, audioOnly=-；recovery: warning=-, recovering=16057ms, stable=18057ms, congested=57ms, firstAction=57ms, L0=20557ms, L1=19057ms, L2=17557ms, L3=16057ms, L4=57ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 81 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-12T16:41:56.990Z；firstRecovering=19314ms (2026-04-12T16:42:16.304Z)；firstStable=21814ms (2026-04-12T16:42:18.804Z)；firstL0=23814ms (2026-04-12T16:42:20.804Z) |
| 恢复诊断 | raw=loss<3%=315ms (2026-04-12T16:41:57.305Z), rtt<120ms=315ms (2026-04-12T16:41:57.305Z), jitter<28ms=315ms (2026-04-12T16:41:57.305Z), jitter<18ms=16815ms (2026-04-12T16:42:13.805Z)；target=target>=120kbps=315ms (2026-04-12T16:41:57.305Z), target>=300kbps=21314ms (2026-04-12T16:42:18.304Z), target>=500kbps=21314ms (2026-04-12T16:42:18.304Z), target>=700kbps=22816ms (2026-04-12T16:42:19.806Z), target>=900kbps=24314ms (2026-04-12T16:42:21.304Z)；send=send>=300kbps=19815ms (2026-04-12T16:42:16.805Z), send>=500kbps=21314ms (2026-04-12T16:42:18.304Z), send>=700kbps=21314ms (2026-04-12T16:42:18.304Z), send>=900kbps=22816ms (2026-04-12T16:42:19.806Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1845ms, recovering=-, stable=344ms, congested=2340ms, firstAction=1845ms, L0=-, L1=1845ms, L2=2340ms, L3=2844ms, L4=3344ms, audioOnly=-；recovery: warning=26315ms, recovering=19314ms, stable=21814ms, congested=315ms, firstAction=315ms, L0=23814ms, L1=22314ms, L2=20815ms, L3=19314ms, L4=315ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 97 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:43:14.619Z；firstRecovering=21973ms (2026-04-12T16:43:36.592Z)；firstStable=24473ms (2026-04-12T16:43:39.092Z)；firstL0=26473ms (2026-04-12T16:43:41.092Z) |
| 恢复诊断 | raw=loss<3%=1473ms (2026-04-12T16:43:16.092Z), rtt<120ms=473ms (2026-04-12T16:43:15.092Z), jitter<28ms=14973ms (2026-04-12T16:43:29.592Z), jitter<18ms=25973ms (2026-04-12T16:43:40.592Z)；target=target>=120kbps=13473ms (2026-04-12T16:43:28.092Z), target>=300kbps=22973ms (2026-04-12T16:43:37.592Z), target>=500kbps=23973ms (2026-04-12T16:43:38.592Z), target>=700kbps=25475ms (2026-04-12T16:43:40.094Z), target>=900kbps=26973ms (2026-04-12T16:43:41.592Z)；send=send>=300kbps=22473ms (2026-04-12T16:43:37.092Z), send>=500kbps=23973ms (2026-04-12T16:43:38.592Z), send>=700kbps=25475ms (2026-04-12T16:43:40.094Z), send>=900kbps=25475ms (2026-04-12T16:43:40.094Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4228ms, recovering=-, stable=228ms, congested=4728ms, firstAction=4228ms, L0=-, L1=4228ms, L2=4728ms, L3=5228ms, L4=5728ms, audioOnly=-；recovery: warning=-, recovering=21973ms, stable=24473ms, congested=473ms, firstAction=473ms, L0=26473ms, L1=24973ms, L2=23473ms, L3=21973ms, L4=473ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:44:23.729Z；firstRecovering=-；firstStable=15962ms (2026-04-12T16:44:39.691Z)；firstL0=15962ms (2026-04-12T16:44:39.691Z) |
| 恢复诊断 | raw=loss<3%=962ms (2026-04-12T16:44:24.691Z), rtt<120ms=462ms (2026-04-12T16:44:24.191Z), jitter<28ms=462ms (2026-04-12T16:44:24.191Z), jitter<18ms=462ms (2026-04-12T16:44:24.191Z)；target=target>=120kbps=462ms (2026-04-12T16:44:24.191Z), target>=300kbps=462ms (2026-04-12T16:44:24.191Z), target>=500kbps=462ms (2026-04-12T16:44:24.191Z), target>=700kbps=462ms (2026-04-12T16:44:24.191Z), target>=900kbps=16463ms (2026-04-12T16:44:40.192Z)；send=send>=300kbps=462ms (2026-04-12T16:44:24.191Z), send>=500kbps=462ms (2026-04-12T16:44:24.191Z), send>=700kbps=462ms (2026-04-12T16:44:24.191Z), send>=900kbps=462ms (2026-04-12T16:44:24.191Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6623ms, recovering=-, stable=123ms, congested=-, firstAction=6623ms, L0=-, L1=6623ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=462ms, recovering=-, stable=15962ms, congested=-, firstAction=15962ms, L0=15962ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 56 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:45:32.696Z；firstRecovering=9167ms (2026-04-12T16:45:41.863Z)；firstStable=11669ms (2026-04-12T16:45:44.365Z)；firstL0=13668ms (2026-04-12T16:45:46.364Z) |
| 恢复诊断 | raw=loss<3%=662ms (2026-04-12T16:45:33.358Z), rtt<120ms=162ms (2026-04-12T16:45:32.858Z), jitter<28ms=4663ms (2026-04-12T16:45:37.359Z), jitter<18ms=7168ms (2026-04-12T16:45:39.864Z)；target=target>=120kbps=9670ms (2026-04-12T16:45:42.366Z), target>=300kbps=11167ms (2026-04-12T16:45:43.863Z), target>=500kbps=11167ms (2026-04-12T16:45:43.863Z), target>=700kbps=12668ms (2026-04-12T16:45:45.364Z), target>=900kbps=14168ms (2026-04-12T16:45:46.864Z)；send=send>=300kbps=9670ms (2026-04-12T16:45:42.366Z), send>=500kbps=9670ms (2026-04-12T16:45:42.366Z), send>=700kbps=9670ms (2026-04-12T16:45:42.366Z), send>=900kbps=11167ms (2026-04-12T16:45:43.863Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2324ms, recovering=-, stable=324ms, congested=3824ms, firstAction=2324ms, L0=-, L1=2324ms, L2=3824ms, L3=4324ms, L4=4824ms, audioOnly=-；recovery: warning=-, recovering=9167ms, stable=11669ms, congested=162ms, firstAction=162ms, L0=13668ms, L1=12169ms, L2=10667ms, L3=9167ms, L4=162ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T16:46:42.030Z；firstRecovering=-；firstStable=15351ms (2026-04-12T16:46:57.381Z)；firstL0=15351ms (2026-04-12T16:46:57.381Z) |
| 恢复诊断 | raw=loss<3%=352ms (2026-04-12T16:46:42.382Z), rtt<120ms=1351ms (2026-04-12T16:46:43.381Z), jitter<28ms=352ms (2026-04-12T16:46:42.382Z), jitter<18ms=352ms (2026-04-12T16:46:42.382Z)；target=target>=120kbps=352ms (2026-04-12T16:46:42.382Z), target>=300kbps=352ms (2026-04-12T16:46:42.382Z), target>=500kbps=352ms (2026-04-12T16:46:42.382Z), target>=700kbps=352ms (2026-04-12T16:46:42.382Z), target>=900kbps=15851ms (2026-04-12T16:46:57.881Z)；send=send>=300kbps=352ms (2026-04-12T16:46:42.382Z), send>=500kbps=352ms (2026-04-12T16:46:42.382Z), send>=700kbps=352ms (2026-04-12T16:46:42.382Z), send>=900kbps=352ms (2026-04-12T16:46:42.382Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2252ms, recovering=-, stable=253ms, congested=-, firstAction=2252ms, L0=-, L1=2252ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=352ms, recovering=-, stable=15351ms, congested=-, firstAction=15351ms, L0=15351ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:47:51.046Z；firstRecovering=-；firstStable=16586ms (2026-04-12T16:48:07.632Z)；firstL0=16586ms (2026-04-12T16:48:07.632Z) |
| 恢复诊断 | raw=loss<3%=86ms (2026-04-12T16:47:51.132Z), rtt<120ms=1586ms (2026-04-12T16:47:52.632Z), jitter<28ms=1586ms (2026-04-12T16:47:52.632Z), jitter<18ms=8086ms (2026-04-12T16:47:59.132Z)；target=target>=120kbps=86ms (2026-04-12T16:47:51.132Z), target>=300kbps=86ms (2026-04-12T16:47:51.132Z), target>=500kbps=86ms (2026-04-12T16:47:51.132Z), target>=700kbps=86ms (2026-04-12T16:47:51.132Z), target>=900kbps=17086ms (2026-04-12T16:48:08.132Z)；send=send>=300kbps=86ms (2026-04-12T16:47:51.132Z), send>=500kbps=86ms (2026-04-12T16:47:51.132Z), send>=700kbps=86ms (2026-04-12T16:47:51.132Z), send>=900kbps=2086ms (2026-04-12T16:47:53.132Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1867ms, recovering=-, stable=367ms, congested=-, firstAction=1867ms, L0=-, L1=1867ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=86ms, recovering=-, stable=16586ms, congested=-, firstAction=16586ms, L0=16586ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=congested/L4)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 57 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-12T16:49:01.508Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=155ms, firstAction=155ms, L0=-, L1=-, L2=-, L3=-, L4=155ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 243 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:51:36.566Z；firstRecovering=21915ms (2026-04-12T16:51:58.481Z)；firstStable=22915ms (2026-04-12T16:51:59.481Z)；firstL0=26416ms (2026-04-12T16:52:02.982Z) |
| 恢复诊断 | raw=loss<3%=417ms (2026-04-12T16:51:36.983Z), rtt<120ms=417ms (2026-04-12T16:51:36.983Z), jitter<28ms=15915ms (2026-04-12T16:51:52.481Z), jitter<18ms=16915ms (2026-04-12T16:51:53.481Z)；target=target>=120kbps=12918ms (2026-04-12T16:51:49.484Z), target>=300kbps=22415ms (2026-04-12T16:51:58.981Z), target>=500kbps=23915ms (2026-04-12T16:52:00.481Z), target>=700kbps=25414ms (2026-04-12T16:52:01.980Z), target>=900kbps=26915ms (2026-04-12T16:52:03.481Z)；send=send>=300kbps=417ms (2026-04-12T16:51:36.983Z), send>=500kbps=417ms (2026-04-12T16:51:36.983Z), send>=700kbps=1415ms (2026-04-12T16:51:37.981Z), send>=900kbps=25414ms (2026-04-12T16:52:01.980Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=38206ms, firstAction=38206ms, L0=-, L1=-, L2=-, L3=-, L4=38206ms, audioOnly=-；recovery: warning=-, recovering=21915ms, stable=22915ms, congested=417ms, firstAction=417ms, L0=26416ms, L1=24915ms, L2=23415ms, L3=21915ms, L4=417ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 246 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T16:55:58.855Z；firstRecovering=23285ms (2026-04-12T16:56:22.140Z)；firstStable=24785ms (2026-04-12T16:56:23.640Z)；firstL0=27785ms (2026-04-12T16:56:26.640Z) |
| 恢复诊断 | raw=loss<3%=285ms (2026-04-12T16:55:59.140Z), rtt<120ms=285ms (2026-04-12T16:55:59.140Z), jitter<28ms=17785ms (2026-04-12T16:56:16.640Z), jitter<18ms=18285ms (2026-04-12T16:56:17.140Z)；target=target>=120kbps=12285ms (2026-04-12T16:56:11.140Z), target>=300kbps=23785ms (2026-04-12T16:56:22.640Z), target>=500kbps=25285ms (2026-04-12T16:56:24.140Z), target>=700kbps=27785ms (2026-04-12T16:56:26.640Z), target>=900kbps=-；send=send>=300kbps=785ms (2026-04-12T16:55:59.640Z), send>=500kbps=24285ms (2026-04-12T16:56:23.140Z), send>=700kbps=26785ms (2026-04-12T16:56:25.640Z), send>=900kbps=29788ms (2026-04-12T16:56:28.643Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=112296ms, firstAction=112296ms, L0=-, L1=-, L2=-, L3=-, L4=112296ms, audioOnly=-；recovery: warning=-, recovering=23285ms, stable=24785ms, congested=285ms, firstAction=285ms, L0=27785ms, L1=26285ms, L2=24785ms, L3=23285ms, L4=285ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 246 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-12T16:59:10.567Z；firstRecovering=22830ms (2026-04-12T16:59:33.397Z)；firstStable=24831ms (2026-04-12T16:59:35.398Z)；firstL0=27343ms (2026-04-12T16:59:37.910Z) |
| 恢复诊断 | raw=loss<3%=831ms (2026-04-12T16:59:11.398Z), rtt<120ms=1831ms (2026-04-12T16:59:12.398Z), jitter<28ms=18330ms (2026-04-12T16:59:28.897Z), jitter<18ms=20830ms (2026-04-12T16:59:31.397Z)；target=target>=120kbps=13329ms (2026-04-12T16:59:23.896Z), target>=300kbps=23330ms (2026-04-12T16:59:33.897Z), target>=500kbps=24831ms (2026-04-12T16:59:35.398Z), target>=700kbps=27832ms (2026-04-12T16:59:38.399Z), target>=900kbps=-；send=send>=300kbps=331ms (2026-04-12T16:59:10.898Z), send>=500kbps=831ms (2026-04-12T16:59:11.398Z), send>=700kbps=1831ms (2026-04-12T16:59:12.398Z), send>=900kbps=29331ms (2026-04-12T16:59:39.898Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=44888ms, firstAction=44888ms, L0=-, L1=-, L2=-, L3=44888ms, L4=45388ms, audioOnly=-；recovery: warning=29831ms, recovering=22830ms, stable=24831ms, congested=331ms, firstAction=331ms, L0=27343ms, L1=25830ms, L2=24330ms, L3=22830ms, L4=331ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 21 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T17:00:04.828Z；firstRecovering=6753ms (2026-04-12T17:00:11.581Z)；firstStable=7252ms (2026-04-12T17:00:12.080Z)；firstL0=11254ms (2026-04-12T17:00:16.082Z) |
| 恢复诊断 | raw=loss<3%=252ms (2026-04-12T17:00:05.080Z), rtt<120ms=252ms (2026-04-12T17:00:05.080Z), jitter<28ms=252ms (2026-04-12T17:00:05.080Z), jitter<18ms=252ms (2026-04-12T17:00:05.080Z)；target=target>=120kbps=252ms (2026-04-12T17:00:05.080Z), target>=300kbps=8755ms (2026-04-12T17:00:13.583Z), target>=500kbps=10259ms (2026-04-12T17:00:15.087Z), target>=700kbps=11254ms (2026-04-12T17:00:16.082Z), target>=900kbps=11753ms (2026-04-12T17:00:16.581Z)；send=send>=300kbps=7252ms (2026-04-12T17:00:12.080Z), send>=500kbps=7252ms (2026-04-12T17:00:12.080Z), send>=700kbps=7252ms (2026-04-12T17:00:12.080Z), send>=900kbps=8755ms (2026-04-12T17:00:13.583Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3329ms, recovering=-, stable=332ms, congested=4329ms, firstAction=3329ms, L0=-, L1=3329ms, L2=4329ms, L3=4829ms, L4=5330ms, audioOnly=-；recovery: warning=-, recovering=6753ms, stable=7252ms, congested=252ms, firstAction=252ms, L0=11254ms, L1=9753ms, L2=8252ms, L3=6753ms, L4=252ms, audioOnly=- |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | [B1](#b1) |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 132 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T17:01:39.491Z；firstRecovering=23908ms (2026-04-12T17:02:03.399Z)；firstStable=25408ms (2026-04-12T17:02:04.899Z)；firstL0=28429ms (2026-04-12T17:02:07.920Z) |
| 恢复诊断 | raw=loss<3%=408ms (2026-04-12T17:01:39.899Z), rtt<120ms=408ms (2026-04-12T17:01:39.899Z), jitter<28ms=16427ms (2026-04-12T17:01:55.918Z), jitter<18ms=17908ms (2026-04-12T17:01:57.399Z)；target=target>=120kbps=14408ms (2026-04-12T17:01:53.899Z), target>=300kbps=24950ms (2026-04-12T17:02:04.441Z), target>=500kbps=25908ms (2026-04-12T17:02:05.399Z), target>=700kbps=27408ms (2026-04-12T17:02:06.899Z), target>=900kbps=28908ms (2026-04-12T17:02:08.399Z)；send=send>=300kbps=908ms (2026-04-12T17:01:40.399Z), send>=500kbps=2408ms (2026-04-12T17:01:41.899Z), send>=700kbps=27408ms (2026-04-12T17:02:06.899Z), send>=900kbps=27408ms (2026-04-12T17:02:06.899Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4806ms, recovering=-, stable=306ms, congested=6306ms, firstAction=4806ms, L0=-, L1=4806ms, L2=6306ms, L3=6806ms, L4=7306ms, audioOnly=-；recovery: warning=-, recovering=23908ms, stable=25408ms, congested=408ms, firstAction=408ms, L0=28429ms, L1=26908ms, L2=25408ms, L3=23908ms, L4=408ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-12T17:02:33.974Z；firstRecovering=-；firstStable=19822ms (2026-04-12T17:02:53.796Z)；firstL0=19822ms (2026-04-12T17:02:53.796Z) |
| 恢复诊断 | raw=loss<3%=822ms (2026-04-12T17:02:34.796Z), rtt<120ms=1322ms (2026-04-12T17:02:35.296Z), jitter<28ms=13322ms (2026-04-12T17:02:47.296Z), jitter<18ms=14323ms (2026-04-12T17:02:48.297Z)；target=target>=120kbps=322ms (2026-04-12T17:02:34.296Z), target>=300kbps=322ms (2026-04-12T17:02:34.296Z), target>=500kbps=2322ms (2026-04-12T17:02:36.296Z), target>=700kbps=8822ms (2026-04-12T17:02:42.796Z), target>=900kbps=20323ms (2026-04-12T17:02:54.297Z)；send=send>=300kbps=322ms (2026-04-12T17:02:34.296Z), send>=500kbps=322ms (2026-04-12T17:02:34.296Z), send>=700kbps=322ms (2026-04-12T17:02:34.296Z), send>=900kbps=1823ms (2026-04-12T17:02:35.797Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1233ms, recovering=-, stable=233ms, congested=-, firstAction=1233ms, L0=-, L1=1233ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=322ms, recovering=-, stable=19822ms, congested=-, firstAction=19822ms, L0=19822ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L2, current=congested/L2)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 44 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L2，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T17:03:28.059Z；firstRecovering=8011ms (2026-04-12T17:03:36.070Z)；firstStable=10514ms (2026-04-12T17:03:38.573Z)；firstL0=12518ms (2026-04-12T17:03:40.577Z) |
| 恢复诊断 | raw=loss<3%=13ms (2026-04-12T17:03:28.072Z), rtt<120ms=513ms (2026-04-12T17:03:28.572Z), jitter<28ms=3513ms (2026-04-12T17:03:31.572Z), jitter<18ms=5512ms (2026-04-12T17:03:33.571Z)；target=target>=120kbps=13ms (2026-04-12T17:03:28.072Z), target>=300kbps=13ms (2026-04-12T17:03:28.072Z), target>=500kbps=13ms (2026-04-12T17:03:28.072Z), target>=700kbps=13016ms (2026-04-12T17:03:41.075Z), target>=900kbps=13514ms (2026-04-12T17:03:41.573Z)；send=send>=300kbps=13ms (2026-04-12T17:03:28.072Z), send>=500kbps=13ms (2026-04-12T17:03:28.072Z), send>=700kbps=13ms (2026-04-12T17:03:28.072Z), send>=900kbps=13ms (2026-04-12T17:03:28.072Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1816ms, recovering=-, stable=317ms, congested=4817ms, firstAction=1816ms, L0=-, L1=1816ms, L2=4817ms, L3=-, L4=-, audioOnly=-；recovery: warning=15018ms, recovering=8011ms, stable=10514ms, congested=13ms, firstAction=13ms, L0=12518ms, L1=11014ms, L2=9513ms, L3=13ms, L4=513ms, audioOnly=- |

