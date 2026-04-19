# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-19T14:54:40.419Z`

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
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:03:21.471Z；firstRecovering=-；firstStable=259ms (2026-04-19T14:03:21.730Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=259ms (2026-04-19T14:03:21.730Z), rtt<120ms=259ms (2026-04-19T14:03:21.730Z), jitter<28ms=259ms (2026-04-19T14:03:21.730Z), jitter<18ms=259ms (2026-04-19T14:03:21.730Z)；target=target>=120kbps=259ms (2026-04-19T14:03:21.730Z), target>=300kbps=259ms (2026-04-19T14:03:21.730Z), target>=500kbps=259ms (2026-04-19T14:03:21.730Z), target>=700kbps=259ms (2026-04-19T14:03:21.730Z), target>=900kbps=259ms (2026-04-19T14:03:21.730Z)；send=send>=300kbps=259ms (2026-04-19T14:03:21.730Z), send>=500kbps=259ms (2026-04-19T14:03:21.730Z), send>=700kbps=259ms (2026-04-19T14:03:21.730Z), send>=900kbps=259ms (2026-04-19T14:03:21.730Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=383ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=259ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:04:30.570Z；firstRecovering=-；firstStable=223ms (2026-04-19T14:04:30.793Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=223ms (2026-04-19T14:04:30.793Z), rtt<120ms=223ms (2026-04-19T14:04:30.793Z), jitter<28ms=223ms (2026-04-19T14:04:30.793Z), jitter<18ms=223ms (2026-04-19T14:04:30.793Z)；target=target>=120kbps=223ms (2026-04-19T14:04:30.793Z), target>=300kbps=223ms (2026-04-19T14:04:30.793Z), target>=500kbps=223ms (2026-04-19T14:04:30.793Z), target>=700kbps=223ms (2026-04-19T14:04:30.793Z), target>=900kbps=223ms (2026-04-19T14:04:30.793Z)；send=send>=300kbps=223ms (2026-04-19T14:04:30.793Z), send>=500kbps=223ms (2026-04-19T14:04:30.793Z), send>=700kbps=223ms (2026-04-19T14:04:30.793Z), send>=900kbps=223ms (2026-04-19T14:04:30.793Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=354ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=223ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:05:39.539Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=240ms (2026-04-19T14:05:39.779Z), rtt<120ms=240ms (2026-04-19T14:05:39.779Z), jitter<28ms=794ms (2026-04-19T14:05:40.333Z), jitter<18ms=-；target=target>=120kbps=240ms (2026-04-19T14:05:39.779Z), target>=300kbps=240ms (2026-04-19T14:05:39.779Z), target>=500kbps=240ms (2026-04-19T14:05:39.779Z), target>=700kbps=240ms (2026-04-19T14:05:39.779Z), target>=900kbps=-；send=send>=300kbps=240ms (2026-04-19T14:05:39.779Z), send>=500kbps=240ms (2026-04-19T14:05:39.779Z), send>=700kbps=240ms (2026-04-19T14:05:39.779Z), send>=900kbps=240ms (2026-04-19T14:05:39.779Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=320ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=240ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:06:48.613Z；firstRecovering=-；firstStable=228ms (2026-04-19T14:06:48.841Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=228ms (2026-04-19T14:06:48.841Z), rtt<120ms=228ms (2026-04-19T14:06:48.841Z), jitter<28ms=228ms (2026-04-19T14:06:48.841Z), jitter<18ms=228ms (2026-04-19T14:06:48.841Z)；target=target>=120kbps=228ms (2026-04-19T14:06:48.841Z), target>=300kbps=228ms (2026-04-19T14:06:48.841Z), target>=500kbps=228ms (2026-04-19T14:06:48.841Z), target>=700kbps=228ms (2026-04-19T14:06:48.841Z), target>=900kbps=228ms (2026-04-19T14:06:48.841Z)；send=send>=300kbps=228ms (2026-04-19T14:06:48.841Z), send>=500kbps=228ms (2026-04-19T14:06:48.841Z), send>=700kbps=228ms (2026-04-19T14:06:48.841Z), send>=900kbps=228ms (2026-04-19T14:06:48.841Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=344ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=228ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 65 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:07:57.718Z；firstRecovering=7024ms (2026-04-19T14:08:04.742Z)；firstStable=9519ms (2026-04-19T14:08:07.237Z)；firstL0=11517ms (2026-04-19T14:08:09.235Z) |
| 恢复诊断 | raw=loss<3%=16ms (2026-04-19T14:07:57.734Z), rtt<120ms=16ms (2026-04-19T14:07:57.734Z), jitter<28ms=5516ms (2026-04-19T14:08:03.234Z), jitter<18ms=7024ms (2026-04-19T14:08:04.742Z)；target=target>=120kbps=16ms (2026-04-19T14:07:57.734Z), target>=300kbps=7517ms (2026-04-19T14:08:05.235Z), target>=500kbps=9016ms (2026-04-19T14:08:06.734Z), target>=700kbps=12018ms (2026-04-19T14:08:09.736Z), target>=900kbps=13520ms (2026-04-19T14:08:11.238Z)；send=send>=300kbps=16ms (2026-04-19T14:07:57.734Z), send>=500kbps=7517ms (2026-04-19T14:08:05.235Z), send>=700kbps=9016ms (2026-04-19T14:08:06.734Z), send>=900kbps=11023ms (2026-04-19T14:08:08.741Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2622ms, recovering=11618ms, stable=116ms, congested=3118ms, firstAction=2622ms, L0=15619ms, L1=2622ms, L2=3118ms, L3=3620ms, L4=4121ms, audioOnly=-；recovery: warning=14034ms, recovering=7024ms, stable=9519ms, congested=16ms, firstAction=16ms, L0=11517ms, L1=10015ms, L2=8517ms, L3=7024ms, L4=16ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 57 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:09:06.669Z；firstRecovering=13276ms (2026-04-19T14:09:19.945Z)；firstStable=277ms (2026-04-19T14:09:06.946Z)；firstL0=16778ms (2026-04-19T14:09:23.447Z) |
| 恢复诊断 | raw=loss<3%=775ms (2026-04-19T14:09:07.444Z), rtt<120ms=277ms (2026-04-19T14:09:06.946Z), jitter<28ms=12776ms (2026-04-19T14:09:19.445Z), jitter<18ms=13790ms (2026-04-19T14:09:20.459Z)；target=target>=120kbps=277ms (2026-04-19T14:09:06.946Z), target>=300kbps=1777ms (2026-04-19T14:09:08.446Z), target>=500kbps=15278ms (2026-04-19T14:09:21.947Z), target>=700kbps=16283ms (2026-04-19T14:09:22.952Z), target>=900kbps=17281ms (2026-04-19T14:09:23.950Z)；send=send>=300kbps=277ms (2026-04-19T14:09:06.946Z), send>=500kbps=2779ms (2026-04-19T14:09:09.448Z), send>=700kbps=16283ms (2026-04-19T14:09:22.952Z), send>=900kbps=16283ms (2026-04-19T14:09:22.952Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1854ms, recovering=15866ms, stable=354ms, congested=2353ms, firstAction=1854ms, L0=19859ms, L1=1854ms, L2=2353ms, L3=2854ms, L4=3354ms, audioOnly=-；recovery: warning=2779ms, recovering=13276ms, stable=277ms, congested=3281ms, firstAction=2779ms, L0=16778ms, L1=2779ms, L2=3281ms, L3=3773ms, L4=4280ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=recovering/L3)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 63 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:10:15.605Z；firstRecovering=260ms (2026-04-19T14:10:15.865Z)；firstStable=1754ms (2026-04-19T14:10:17.359Z)；firstL0=3754ms (2026-04-19T14:10:19.359Z) |
| 恢复诊断 | raw=loss<3%=260ms (2026-04-19T14:10:15.865Z), rtt<120ms=260ms (2026-04-19T14:10:15.865Z), jitter<28ms=260ms (2026-04-19T14:10:15.865Z), jitter<18ms=21780ms (2026-04-19T14:10:37.385Z)；target=target>=120kbps=260ms (2026-04-19T14:10:15.865Z), target>=300kbps=1754ms (2026-04-19T14:10:17.359Z), target>=500kbps=18771ms (2026-04-19T14:10:34.376Z), target>=700kbps=19754ms (2026-04-19T14:10:35.359Z), target>=900kbps=20754ms (2026-04-19T14:10:36.359Z)；send=send>=300kbps=260ms (2026-04-19T14:10:15.865Z), send>=500kbps=7755ms (2026-04-19T14:10:23.360Z), send>=700kbps=19754ms (2026-04-19T14:10:35.359Z), send>=900kbps=19754ms (2026-04-19T14:10:35.359Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3315ms, recovering=19316ms, stable=313ms, congested=3820ms, firstAction=3315ms, L0=-, L1=3315ms, L2=3820ms, L3=4314ms, L4=4820ms, audioOnly=-；recovery: warning=6258ms, recovering=260ms, stable=1754ms, congested=6773ms, firstAction=755ms, L0=3754ms, L1=1754ms, L2=755ms, L3=7255ms, L4=7755ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L1, current=stable/L1) |
| 实际动作 | setEncodingParameters（共 88 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L1。 |
| 恢复里程碑 | start=2026-04-19T14:11:24.532Z；firstRecovering=26231ms (2026-04-19T14:11:50.763Z)；firstStable=28781ms (2026-04-19T14:11:53.313Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=724ms (2026-04-19T14:11:25.256Z), rtt<120ms=224ms (2026-04-19T14:11:24.756Z), jitter<28ms=25724ms (2026-04-19T14:11:50.256Z), jitter<18ms=-；target=target>=120kbps=2758ms (2026-04-19T14:11:27.290Z), target>=300kbps=27227ms (2026-04-19T14:11:51.759Z), target>=500kbps=28225ms (2026-04-19T14:11:52.757Z), target>=700kbps=29768ms (2026-04-19T14:11:54.300Z), target>=900kbps=-；send=send>=300kbps=26724ms (2026-04-19T14:11:51.256Z), send>=500kbps=28781ms (2026-04-19T14:11:53.313Z), send>=700kbps=28781ms (2026-04-19T14:11:53.313Z), send>=900kbps=29768ms (2026-04-19T14:11:54.300Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2828ms, recovering=-, stable=330ms, congested=4330ms, firstAction=2828ms, L0=-, L1=2828ms, L2=4330ms, L3=4834ms, L4=5334ms, audioOnly=-；recovery: warning=-, recovering=26231ms, stable=28781ms, congested=224ms, firstAction=224ms, L0=-, L1=29227ms, L2=27813ms, L3=26231ms, L4=224ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 90 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:12:33.498Z；firstRecovering=25192ms (2026-04-19T14:12:58.690Z)；firstStable=27697ms (2026-04-19T14:13:01.195Z)；firstL0=29692ms (2026-04-19T14:13:03.190Z) |
| 恢复诊断 | raw=loss<3%=193ms (2026-04-19T14:12:33.691Z), rtt<120ms=193ms (2026-04-19T14:12:33.691Z), jitter<28ms=193ms (2026-04-19T14:12:33.691Z), jitter<18ms=-；target=target>=120kbps=11694ms (2026-04-19T14:12:45.192Z), target>=300kbps=27192ms (2026-04-19T14:13:00.690Z), target>=500kbps=27697ms (2026-04-19T14:13:01.195Z), target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=692ms (2026-04-19T14:12:34.190Z), send>=500kbps=27697ms (2026-04-19T14:13:01.195Z), send>=700kbps=27697ms (2026-04-19T14:13:01.195Z), send>=900kbps=29692ms (2026-04-19T14:13:03.190Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1762ms, recovering=-, stable=259ms, congested=2763ms, firstAction=1762ms, L0=-, L1=1762ms, L2=2763ms, L3=3259ms, L4=3757ms, audioOnly=-；recovery: warning=-, recovering=25192ms, stable=27697ms, congested=193ms, firstAction=193ms, L0=29692ms, L1=28193ms, L2=26745ms, L3=25192ms, L4=193ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:13:42.422Z；firstRecovering=-；firstStable=285ms (2026-04-19T14:13:42.707Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=285ms (2026-04-19T14:13:42.707Z), rtt<120ms=285ms (2026-04-19T14:13:42.707Z), jitter<28ms=285ms (2026-04-19T14:13:42.707Z), jitter<18ms=285ms (2026-04-19T14:13:42.707Z)；target=target>=120kbps=285ms (2026-04-19T14:13:42.707Z), target>=300kbps=285ms (2026-04-19T14:13:42.707Z), target>=500kbps=285ms (2026-04-19T14:13:42.707Z), target>=700kbps=285ms (2026-04-19T14:13:42.707Z), target>=900kbps=285ms (2026-04-19T14:13:42.707Z)；send=send>=300kbps=285ms (2026-04-19T14:13:42.707Z), send>=500kbps=285ms (2026-04-19T14:13:42.707Z), send>=700kbps=285ms (2026-04-19T14:13:42.707Z), send>=900kbps=285ms (2026-04-19T14:13:42.707Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=353ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=285ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:14:51.290Z；firstRecovering=-；firstStable=257ms (2026-04-19T14:14:51.547Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=257ms (2026-04-19T14:14:51.547Z), rtt<120ms=257ms (2026-04-19T14:14:51.547Z), jitter<28ms=257ms (2026-04-19T14:14:51.547Z), jitter<18ms=257ms (2026-04-19T14:14:51.547Z)；target=target>=120kbps=257ms (2026-04-19T14:14:51.547Z), target>=300kbps=257ms (2026-04-19T14:14:51.547Z), target>=500kbps=257ms (2026-04-19T14:14:51.547Z), target>=700kbps=257ms (2026-04-19T14:14:51.547Z), target>=900kbps=257ms (2026-04-19T14:14:51.547Z)；send=send>=300kbps=257ms (2026-04-19T14:14:51.547Z), send>=500kbps=257ms (2026-04-19T14:14:51.547Z), send>=700kbps=257ms (2026-04-19T14:14:51.547Z), send>=900kbps=257ms (2026-04-19T14:14:51.547Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=337ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=257ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:16:00.506Z；firstRecovering=-；firstStable=205ms (2026-04-19T14:16:00.711Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=205ms (2026-04-19T14:16:00.711Z), rtt<120ms=205ms (2026-04-19T14:16:00.711Z), jitter<28ms=205ms (2026-04-19T14:16:00.711Z), jitter<18ms=205ms (2026-04-19T14:16:00.711Z)；target=target>=120kbps=205ms (2026-04-19T14:16:00.711Z), target>=300kbps=205ms (2026-04-19T14:16:00.711Z), target>=500kbps=205ms (2026-04-19T14:16:00.711Z), target>=700kbps=205ms (2026-04-19T14:16:00.711Z), target>=900kbps=205ms (2026-04-19T14:16:00.711Z)；send=send>=300kbps=205ms (2026-04-19T14:16:00.711Z), send>=500kbps=205ms (2026-04-19T14:16:00.711Z), send>=700kbps=205ms (2026-04-19T14:16:00.711Z), send>=900kbps=205ms (2026-04-19T14:16:00.711Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=271ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=205ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:17:09.494Z；firstRecovering=-；firstStable=137ms (2026-04-19T14:17:09.631Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=1134ms (2026-04-19T14:17:10.628Z), rtt<120ms=137ms (2026-04-19T14:17:09.631Z), jitter<28ms=137ms (2026-04-19T14:17:09.631Z), jitter<18ms=14133ms (2026-04-19T14:17:23.627Z)；target=target>=120kbps=137ms (2026-04-19T14:17:09.631Z), target>=300kbps=137ms (2026-04-19T14:17:09.631Z), target>=500kbps=137ms (2026-04-19T14:17:09.631Z), target>=700kbps=137ms (2026-04-19T14:17:09.631Z), target>=900kbps=643ms (2026-04-19T14:17:10.137Z)；send=send>=300kbps=137ms (2026-04-19T14:17:09.631Z), send>=500kbps=137ms (2026-04-19T14:17:09.631Z), send>=700kbps=137ms (2026-04-19T14:17:09.631Z), send>=900kbps=137ms (2026-04-19T14:17:09.631Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5792ms, recovering=-, stable=297ms, congested=-, firstAction=5792ms, L0=9797ms, L1=5792ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=643ms, recovering=-, stable=137ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 24 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:18:18.362Z；firstRecovering=6720ms (2026-04-19T14:18:25.082Z)；firstStable=8719ms (2026-04-19T14:18:27.081Z)；firstL0=10219ms (2026-04-19T14:18:28.581Z) |
| 恢复诊断 | raw=loss<3%=720ms (2026-04-19T14:18:19.082Z), rtt<120ms=220ms (2026-04-19T14:18:18.582Z), jitter<28ms=220ms (2026-04-19T14:18:18.582Z), jitter<18ms=3227ms (2026-04-19T14:18:21.589Z)；target=target>=120kbps=1720ms (2026-04-19T14:18:20.082Z), target>=300kbps=8719ms (2026-04-19T14:18:27.081Z), target>=500kbps=9719ms (2026-04-19T14:18:28.081Z), target>=700kbps=9719ms (2026-04-19T14:18:28.081Z), target>=900kbps=11220ms (2026-04-19T14:18:29.582Z)；send=send>=300kbps=7219ms (2026-04-19T14:18:25.581Z), send>=500kbps=7219ms (2026-04-19T14:18:25.581Z), send>=700kbps=7219ms (2026-04-19T14:18:25.581Z), send>=900kbps=7719ms (2026-04-19T14:18:26.081Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2283ms, recovering=-, stable=283ms, congested=18295ms, firstAction=2283ms, L0=-, L1=2283ms, L2=18295ms, L3=18787ms, L4=19283ms, audioOnly=-；recovery: warning=12719ms, recovering=6720ms, stable=8719ms, congested=220ms, firstAction=220ms, L0=10219ms, L1=9219ms, L2=8219ms, L3=6720ms, L4=220ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:19:27.165Z；firstRecovering=5806ms (2026-04-19T14:19:32.971Z)；firstStable=8309ms (2026-04-19T14:19:35.474Z)；firstL0=9306ms (2026-04-19T14:19:36.471Z) |
| 恢复诊断 | raw=loss<3%=307ms (2026-04-19T14:19:27.472Z), rtt<120ms=307ms (2026-04-19T14:19:27.472Z), jitter<28ms=5305ms (2026-04-19T14:19:32.470Z), jitter<18ms=16805ms (2026-04-19T14:19:43.970Z)；target=target>=120kbps=6306ms (2026-04-19T14:19:33.471Z), target>=300kbps=7808ms (2026-04-19T14:19:34.973Z), target>=500kbps=8807ms (2026-04-19T14:19:35.972Z), target>=700kbps=9807ms (2026-04-19T14:19:36.972Z), target>=900kbps=25806ms (2026-04-19T14:19:52.971Z)；send=send>=300kbps=6306ms (2026-04-19T14:19:33.471Z), send>=500kbps=6306ms (2026-04-19T14:19:33.471Z), send>=700kbps=6306ms (2026-04-19T14:19:33.471Z), send>=900kbps=7808ms (2026-04-19T14:19:34.973Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2367ms, recovering=-, stable=366ms, congested=2865ms, firstAction=2367ms, L0=-, L1=2367ms, L2=2865ms, L3=3367ms, L4=3868ms, audioOnly=-；recovery: warning=11809ms, recovering=5806ms, stable=8309ms, congested=307ms, firstAction=307ms, L0=9306ms, L1=8309ms, L2=7307ms, L3=5806ms, L4=307ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 78 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T14:20:35.922Z；firstRecovering=18336ms (2026-04-19T14:20:54.258Z)；firstStable=20836ms (2026-04-19T14:20:56.758Z)；firstL0=22336ms (2026-04-19T14:20:58.258Z) |
| 恢复诊断 | raw=loss<3%=836ms (2026-04-19T14:20:36.758Z), rtt<120ms=377ms (2026-04-19T14:20:36.299Z), jitter<28ms=17881ms (2026-04-19T14:20:53.803Z), jitter<18ms=24835ms (2026-04-19T14:21:00.757Z)；target=target>=120kbps=13336ms (2026-04-19T14:20:49.258Z), target>=300kbps=18838ms (2026-04-19T14:20:54.760Z), target>=500kbps=20367ms (2026-04-19T14:20:56.289Z), target>=700kbps=21335ms (2026-04-19T14:20:57.257Z), target>=900kbps=22836ms (2026-04-19T14:20:58.758Z)；send=send>=300kbps=18838ms (2026-04-19T14:20:54.760Z), send>=500kbps=18838ms (2026-04-19T14:20:54.760Z), send>=700kbps=18838ms (2026-04-19T14:20:54.760Z), send>=900kbps=20367ms (2026-04-19T14:20:56.289Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1407ms, recovering=-, stable=391ms, congested=2460ms, firstAction=1407ms, L0=-, L1=1407ms, L2=2460ms, L3=2891ms, L4=3391ms, audioOnly=-；recovery: warning=24835ms, recovering=18336ms, stable=20836ms, congested=377ms, firstAction=377ms, L0=22336ms, L1=20836ms, L2=19836ms, L3=18336ms, L4=377ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 69 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:21:44.970Z；firstRecovering=18203ms (2026-04-19T14:22:03.173Z)；firstStable=20701ms (2026-04-19T14:22:05.671Z)；firstL0=21701ms (2026-04-19T14:22:06.671Z) |
| 恢复诊断 | raw=loss<3%=267ms (2026-04-19T14:21:45.237Z), rtt<120ms=267ms (2026-04-19T14:21:45.237Z), jitter<28ms=17701ms (2026-04-19T14:22:02.671Z), jitter<18ms=17701ms (2026-04-19T14:22:02.671Z)；target=target>=120kbps=14201ms (2026-04-19T14:21:59.171Z), target>=300kbps=18701ms (2026-04-19T14:22:03.671Z), target>=500kbps=20201ms (2026-04-19T14:22:05.171Z), target>=700kbps=21219ms (2026-04-19T14:22:06.189Z), target>=900kbps=22254ms (2026-04-19T14:22:07.224Z)；send=send>=300kbps=18701ms (2026-04-19T14:22:03.671Z), send>=500kbps=20201ms (2026-04-19T14:22:05.171Z), send>=700kbps=20201ms (2026-04-19T14:22:05.171Z), send>=900kbps=21219ms (2026-04-19T14:22:06.189Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1279ms, recovering=-, stable=280ms, congested=6282ms, firstAction=1279ms, L0=-, L1=1279ms, L2=6282ms, L3=6781ms, L4=7279ms, audioOnly=-；recovery: warning=-, recovering=18203ms, stable=20701ms, congested=267ms, firstAction=267ms, L0=21701ms, L1=20701ms, L2=19705ms, L3=18203ms, L4=267ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:22:53.823Z；firstRecovering=-；firstStable=278ms (2026-04-19T14:22:54.101Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=278ms (2026-04-19T14:22:54.101Z), rtt<120ms=278ms (2026-04-19T14:22:54.101Z), jitter<28ms=278ms (2026-04-19T14:22:54.101Z), jitter<18ms=278ms (2026-04-19T14:22:54.101Z)；target=target>=120kbps=278ms (2026-04-19T14:22:54.101Z), target>=300kbps=278ms (2026-04-19T14:22:54.101Z), target>=500kbps=278ms (2026-04-19T14:22:54.101Z), target>=700kbps=278ms (2026-04-19T14:22:54.101Z), target>=900kbps=278ms (2026-04-19T14:22:54.101Z)；send=send>=300kbps=278ms (2026-04-19T14:22:54.101Z), send>=500kbps=278ms (2026-04-19T14:22:54.101Z), send>=700kbps=278ms (2026-04-19T14:22:54.101Z), send>=900kbps=278ms (2026-04-19T14:22:54.101Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=342ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=278ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:24:02.773Z；firstRecovering=-；firstStable=207ms (2026-04-19T14:24:02.980Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=207ms (2026-04-19T14:24:02.980Z), rtt<120ms=207ms (2026-04-19T14:24:02.980Z), jitter<28ms=207ms (2026-04-19T14:24:02.980Z), jitter<18ms=207ms (2026-04-19T14:24:02.980Z)；target=target>=120kbps=207ms (2026-04-19T14:24:02.980Z), target>=300kbps=207ms (2026-04-19T14:24:02.980Z), target>=500kbps=207ms (2026-04-19T14:24:02.980Z), target>=700kbps=207ms (2026-04-19T14:24:02.980Z), target>=900kbps=701ms (2026-04-19T14:24:03.474Z)；send=send>=300kbps=207ms (2026-04-19T14:24:02.980Z), send>=500kbps=207ms (2026-04-19T14:24:02.980Z), send>=700kbps=207ms (2026-04-19T14:24:02.980Z), send>=900kbps=207ms (2026-04-19T14:24:02.980Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1866ms, recovering=-, stable=366ms, congested=-, firstAction=1866ms, L0=14870ms, L1=1866ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=207ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:25:11.668Z；firstRecovering=-；firstStable=15291ms (2026-04-19T14:25:26.959Z)；firstL0=15291ms (2026-04-19T14:25:26.959Z) |
| 恢复诊断 | raw=loss<3%=298ms (2026-04-19T14:25:11.966Z), rtt<120ms=1784ms (2026-04-19T14:25:13.452Z), jitter<28ms=298ms (2026-04-19T14:25:11.966Z), jitter<18ms=11819ms (2026-04-19T14:25:23.487Z)；target=target>=120kbps=298ms (2026-04-19T14:25:11.966Z), target>=300kbps=298ms (2026-04-19T14:25:11.966Z), target>=500kbps=298ms (2026-04-19T14:25:11.966Z), target>=700kbps=298ms (2026-04-19T14:25:11.966Z), target>=900kbps=15790ms (2026-04-19T14:25:27.458Z)；send=send>=300kbps=298ms (2026-04-19T14:25:11.966Z), send>=500kbps=298ms (2026-04-19T14:25:11.966Z), send>=700kbps=298ms (2026-04-19T14:25:11.966Z), send>=900kbps=2784ms (2026-04-19T14:25:14.452Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6884ms, recovering=-, stable=377ms, congested=-, firstAction=6884ms, L0=-, L1=6884ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=298ms, recovering=-, stable=15291ms, congested=-, firstAction=15291ms, L0=15291ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:26:20.562Z；firstRecovering=-；firstStable=15279ms (2026-04-19T14:26:35.841Z)；firstL0=15279ms (2026-04-19T14:26:35.841Z) |
| 恢复诊断 | raw=loss<3%=267ms (2026-04-19T14:26:20.829Z), rtt<120ms=1899ms (2026-04-19T14:26:22.461Z), jitter<28ms=267ms (2026-04-19T14:26:20.829Z), jitter<18ms=267ms (2026-04-19T14:26:20.829Z)；target=target>=120kbps=267ms (2026-04-19T14:26:20.829Z), target>=300kbps=267ms (2026-04-19T14:26:20.829Z), target>=500kbps=267ms (2026-04-19T14:26:20.829Z), target>=700kbps=267ms (2026-04-19T14:26:20.829Z), target>=900kbps=15772ms (2026-04-19T14:26:36.334Z)；send=send>=300kbps=267ms (2026-04-19T14:26:20.829Z), send>=500kbps=267ms (2026-04-19T14:26:20.829Z), send>=700kbps=267ms (2026-04-19T14:26:20.829Z), send>=900kbps=1297ms (2026-04-19T14:26:21.859Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2323ms, recovering=-, stable=323ms, congested=-, firstAction=2323ms, L0=-, L1=2323ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=267ms, recovering=-, stable=15279ms, congested=-, firstAction=15279ms, L0=15279ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 68 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:27:29.509Z；firstRecovering=4208ms (2026-04-19T14:27:33.717Z)；firstStable=4721ms (2026-04-19T14:27:34.230Z)；firstL0=8720ms (2026-04-19T14:27:38.229Z) |
| 恢复诊断 | raw=loss<3%=251ms (2026-04-19T14:27:29.760Z), rtt<120ms=1205ms (2026-04-19T14:27:30.714Z), jitter<28ms=251ms (2026-04-19T14:27:29.760Z), jitter<18ms=251ms (2026-04-19T14:27:29.760Z)；target=target>=120kbps=251ms (2026-04-19T14:27:29.760Z), target>=300kbps=4721ms (2026-04-19T14:27:34.230Z), target>=500kbps=6210ms (2026-04-19T14:27:35.719Z), target>=700kbps=23208ms (2026-04-19T14:27:52.717Z), target>=900kbps=24216ms (2026-04-19T14:27:53.725Z)；send=send>=300kbps=4721ms (2026-04-19T14:27:34.230Z), send>=500kbps=4721ms (2026-04-19T14:27:34.230Z), send>=700kbps=5211ms (2026-04-19T14:27:34.720Z), send>=900kbps=6210ms (2026-04-19T14:27:35.719Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1307ms, recovering=-, stable=307ms, congested=3806ms, firstAction=1307ms, L0=-, L1=1307ms, L2=3806ms, L3=4307ms, L4=4806ms, audioOnly=-；recovery: warning=11210ms, recovering=4208ms, stable=4721ms, congested=251ms, firstAction=251ms, L0=8720ms, L1=7212ms, L2=5717ms, L3=4208ms, L4=251ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:28:38.674Z；firstRecovering=6199ms (2026-04-19T14:28:44.873Z)；firstStable=6699ms (2026-04-19T14:28:45.373Z)；firstL0=10699ms (2026-04-19T14:28:49.373Z) |
| 恢复诊断 | raw=loss<3%=199ms (2026-04-19T14:28:38.873Z), rtt<120ms=2699ms (2026-04-19T14:28:41.373Z), jitter<28ms=199ms (2026-04-19T14:28:38.873Z), jitter<18ms=199ms (2026-04-19T14:28:38.873Z)；target=target>=120kbps=199ms (2026-04-19T14:28:38.873Z), target>=300kbps=6699ms (2026-04-19T14:28:45.373Z), target>=500kbps=8202ms (2026-04-19T14:28:46.876Z), target>=700kbps=12199ms (2026-04-19T14:28:50.873Z), target>=900kbps=26700ms (2026-04-19T14:29:05.374Z)；send=send>=300kbps=6699ms (2026-04-19T14:28:45.373Z), send>=500kbps=6699ms (2026-04-19T14:28:45.373Z), send>=700kbps=7201ms (2026-04-19T14:28:45.875Z), send>=900kbps=7201ms (2026-04-19T14:28:45.875Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2283ms, recovering=-, stable=282ms, congested=2782ms, firstAction=2283ms, L0=-, L1=2283ms, L2=2782ms, L3=3285ms, L4=3867ms, audioOnly=-；recovery: warning=13199ms, recovering=6199ms, stable=6699ms, congested=199ms, firstAction=199ms, L0=10699ms, L1=9199ms, L2=7699ms, L3=6199ms, L4=199ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:29:47.539Z；firstRecovering=-；firstStable=263ms (2026-04-19T14:29:47.802Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=263ms (2026-04-19T14:29:47.802Z), rtt<120ms=263ms (2026-04-19T14:29:47.802Z), jitter<28ms=263ms (2026-04-19T14:29:47.802Z), jitter<18ms=263ms (2026-04-19T14:29:47.802Z)；target=target>=120kbps=263ms (2026-04-19T14:29:47.802Z), target>=300kbps=263ms (2026-04-19T14:29:47.802Z), target>=500kbps=263ms (2026-04-19T14:29:47.802Z), target>=700kbps=263ms (2026-04-19T14:29:47.802Z), target>=900kbps=263ms (2026-04-19T14:29:47.802Z)；send=send>=300kbps=263ms (2026-04-19T14:29:47.802Z), send>=500kbps=263ms (2026-04-19T14:29:47.802Z), send>=700kbps=263ms (2026-04-19T14:29:47.802Z), send>=900kbps=263ms (2026-04-19T14:29:47.802Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=329ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=263ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:30:56.417Z；firstRecovering=-；firstStable=265ms (2026-04-19T14:30:56.682Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=265ms (2026-04-19T14:30:56.682Z), rtt<120ms=265ms (2026-04-19T14:30:56.682Z), jitter<28ms=265ms (2026-04-19T14:30:56.682Z), jitter<18ms=1765ms (2026-04-19T14:30:58.182Z)；target=target>=120kbps=265ms (2026-04-19T14:30:56.682Z), target>=300kbps=265ms (2026-04-19T14:30:56.682Z), target>=500kbps=265ms (2026-04-19T14:30:56.682Z), target>=700kbps=265ms (2026-04-19T14:30:56.682Z), target>=900kbps=265ms (2026-04-19T14:30:56.682Z)；send=send>=300kbps=265ms (2026-04-19T14:30:56.682Z), send>=500kbps=265ms (2026-04-19T14:30:56.682Z), send>=700kbps=265ms (2026-04-19T14:30:56.682Z), send>=900kbps=265ms (2026-04-19T14:30:56.682Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=324ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=265ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:32:05.305Z；firstRecovering=-；firstStable=17199ms (2026-04-19T14:32:22.504Z)；firstL0=17199ms (2026-04-19T14:32:22.504Z) |
| 恢复诊断 | raw=loss<3%=192ms (2026-04-19T14:32:05.497Z), rtt<120ms=192ms (2026-04-19T14:32:05.497Z), jitter<28ms=3192ms (2026-04-19T14:32:08.497Z), jitter<18ms=3192ms (2026-04-19T14:32:08.497Z)；target=target>=120kbps=192ms (2026-04-19T14:32:05.497Z), target>=300kbps=192ms (2026-04-19T14:32:05.497Z), target>=500kbps=192ms (2026-04-19T14:32:05.497Z), target>=700kbps=192ms (2026-04-19T14:32:05.497Z), target>=900kbps=17693ms (2026-04-19T14:32:22.998Z)；send=send>=300kbps=192ms (2026-04-19T14:32:05.497Z), send>=500kbps=192ms (2026-04-19T14:32:05.497Z), send>=700kbps=192ms (2026-04-19T14:32:05.497Z), send>=900kbps=3710ms (2026-04-19T14:32:09.015Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2299ms, recovering=-, stable=298ms, congested=-, firstAction=2299ms, L0=-, L1=2299ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=192ms, recovering=-, stable=17199ms, congested=-, firstAction=17199ms, L0=17199ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 46 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:33:14.205Z；firstRecovering=3739ms (2026-04-19T14:33:17.944Z)；firstStable=6239ms (2026-04-19T14:33:20.444Z)；firstL0=7239ms (2026-04-19T14:33:21.444Z) |
| 恢复诊断 | raw=loss<3%=239ms (2026-04-19T14:33:14.444Z), rtt<120ms=1739ms (2026-04-19T14:33:15.944Z), jitter<28ms=3239ms (2026-04-19T14:33:17.444Z), jitter<18ms=10740ms (2026-04-19T14:33:24.945Z)；target=target>=120kbps=239ms (2026-04-19T14:33:14.444Z), target>=300kbps=4239ms (2026-04-19T14:33:18.444Z), target>=500kbps=5739ms (2026-04-19T14:33:19.944Z), target>=700kbps=6739ms (2026-04-19T14:33:20.944Z), target>=900kbps=7757ms (2026-04-19T14:33:21.962Z)；send=send>=300kbps=4239ms (2026-04-19T14:33:18.444Z), send>=500kbps=4239ms (2026-04-19T14:33:18.444Z), send>=700kbps=5739ms (2026-04-19T14:33:19.944Z), send>=900kbps=5739ms (2026-04-19T14:33:19.944Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1796ms, recovering=-, stable=310ms, congested=4296ms, firstAction=1796ms, L0=-, L1=1796ms, L2=4296ms, L3=4796ms, L4=5319ms, audioOnly=-；recovery: warning=9741ms, recovering=3739ms, stable=6239ms, congested=239ms, firstAction=239ms, L0=7239ms, L1=6239ms, L2=5239ms, L3=3739ms, L4=239ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 71 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:34:23.141Z；firstRecovering=4663ms (2026-04-19T14:34:27.804Z)；firstStable=7161ms (2026-04-19T14:34:30.302Z)；firstL0=9196ms (2026-04-19T14:34:32.337Z) |
| 恢复诊断 | raw=loss<3%=273ms (2026-04-19T14:34:23.414Z), rtt<120ms=1177ms (2026-04-19T14:34:24.318Z), jitter<28ms=4179ms (2026-04-19T14:34:27.320Z), jitter<18ms=18666ms (2026-04-19T14:34:41.807Z)；target=target>=120kbps=273ms (2026-04-19T14:34:23.414Z), target>=300kbps=5160ms (2026-04-19T14:34:28.301Z), target>=500kbps=10285ms (2026-04-19T14:34:33.426Z), target>=700kbps=23664ms (2026-04-19T14:34:46.805Z), target>=900kbps=24660ms (2026-04-19T14:34:47.801Z)；send=send>=300kbps=5160ms (2026-04-19T14:34:28.301Z), send>=500kbps=5669ms (2026-04-19T14:34:28.810Z), send>=700kbps=6664ms (2026-04-19T14:34:29.805Z), send>=900kbps=10285ms (2026-04-19T14:34:33.426Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1747ms, recovering=-, stable=245ms, congested=2740ms, firstAction=1747ms, L0=-, L1=1747ms, L2=2740ms, L3=3244ms, L4=3735ms, audioOnly=-；recovery: warning=11686ms, recovering=4663ms, stable=7161ms, congested=273ms, firstAction=273ms, L0=9196ms, L1=7661ms, L2=6162ms, L3=4663ms, L4=273ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:35:31.941Z；firstRecovering=-；firstStable=298ms (2026-04-19T14:35:32.239Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=298ms (2026-04-19T14:35:32.239Z), rtt<120ms=298ms (2026-04-19T14:35:32.239Z), jitter<28ms=298ms (2026-04-19T14:35:32.239Z), jitter<18ms=1292ms (2026-04-19T14:35:33.233Z)；target=target>=120kbps=298ms (2026-04-19T14:35:32.239Z), target>=300kbps=298ms (2026-04-19T14:35:32.239Z), target>=500kbps=298ms (2026-04-19T14:35:32.239Z), target>=700kbps=298ms (2026-04-19T14:35:32.239Z), target>=900kbps=298ms (2026-04-19T14:35:32.239Z)；send=send>=300kbps=298ms (2026-04-19T14:35:32.239Z), send>=500kbps=298ms (2026-04-19T14:35:32.239Z), send>=700kbps=298ms (2026-04-19T14:35:32.239Z), send>=900kbps=785ms (2026-04-19T14:35:32.726Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=347ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=298ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 43 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:36:40.715Z；firstRecovering=5784ms (2026-04-19T14:36:46.499Z)；firstStable=7784ms (2026-04-19T14:36:48.499Z)；firstL0=9868ms (2026-04-19T14:36:50.583Z) |
| 恢复诊断 | raw=loss<3%=284ms (2026-04-19T14:36:40.999Z), rtt<120ms=284ms (2026-04-19T14:36:40.999Z), jitter<28ms=284ms (2026-04-19T14:36:40.999Z), jitter<18ms=1784ms (2026-04-19T14:36:42.499Z)；target=target>=120kbps=284ms (2026-04-19T14:36:40.999Z), target>=300kbps=6284ms (2026-04-19T14:36:46.999Z), target>=500kbps=7784ms (2026-04-19T14:36:48.499Z), target>=700kbps=8823ms (2026-04-19T14:36:49.538Z), target>=900kbps=10284ms (2026-04-19T14:36:50.999Z)；send=send>=300kbps=6284ms (2026-04-19T14:36:46.999Z), send>=500kbps=7784ms (2026-04-19T14:36:48.499Z), send>=700kbps=8823ms (2026-04-19T14:36:49.538Z), send>=900kbps=8823ms (2026-04-19T14:36:49.538Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1344ms, recovering=10344ms, stable=344ms, congested=1863ms, firstAction=1344ms, L0=14853ms, L1=1344ms, L2=1863ms, L3=2354ms, L4=2844ms, audioOnly=-；recovery: warning=-, recovering=5784ms, stable=7784ms, congested=284ms, firstAction=284ms, L0=9868ms, L1=8284ms, L2=7284ms, L3=5784ms, L4=284ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 61 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:37:49.495Z；firstRecovering=15332ms (2026-04-19T14:38:04.827Z)；firstStable=300ms (2026-04-19T14:37:49.795Z)；firstL0=1804ms (2026-04-19T14:37:51.299Z) |
| 恢复诊断 | raw=loss<3%=300ms (2026-04-19T14:37:49.795Z), rtt<120ms=300ms (2026-04-19T14:37:49.795Z), jitter<28ms=14801ms (2026-04-19T14:38:04.296Z), jitter<18ms=21303ms (2026-04-19T14:38:10.798Z)；target=target>=120kbps=300ms (2026-04-19T14:37:49.795Z), target>=300kbps=15801ms (2026-04-19T14:38:05.296Z), target>=500kbps=17302ms (2026-04-19T14:38:06.797Z), target>=700kbps=18804ms (2026-04-19T14:38:08.299Z), target>=900kbps=20299ms (2026-04-19T14:38:09.794Z)；send=send>=300kbps=802ms (2026-04-19T14:37:50.297Z), send>=500kbps=17302ms (2026-04-19T14:38:06.797Z), send>=700kbps=18804ms (2026-04-19T14:38:08.299Z), send>=900kbps=18804ms (2026-04-19T14:38:08.299Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2396ms, recovering=16893ms, stable=397ms, congested=2893ms, firstAction=2396ms, L0=-, L1=2396ms, L2=2893ms, L3=3399ms, L4=3893ms, audioOnly=-；recovery: warning=4299ms, recovering=15332ms, stable=300ms, congested=4810ms, firstAction=1804ms, L0=1804ms, L1=4299ms, L2=4810ms, L3=5299ms, L4=5799ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:38:58.423Z；firstRecovering=-；firstStable=14127ms (2026-04-19T14:39:12.550Z)；firstL0=14127ms (2026-04-19T14:39:12.550Z) |
| 恢复诊断 | raw=loss<3%=125ms (2026-04-19T14:38:58.548Z), rtt<120ms=125ms (2026-04-19T14:38:58.548Z), jitter<28ms=125ms (2026-04-19T14:38:58.548Z), jitter<18ms=7126ms (2026-04-19T14:39:05.549Z)；target=target>=120kbps=125ms (2026-04-19T14:38:58.548Z), target>=300kbps=125ms (2026-04-19T14:38:58.548Z), target>=500kbps=125ms (2026-04-19T14:38:58.548Z), target>=700kbps=627ms (2026-04-19T14:38:59.050Z), target>=900kbps=14625ms (2026-04-19T14:39:13.048Z)；send=send>=300kbps=125ms (2026-04-19T14:38:58.548Z), send>=500kbps=125ms (2026-04-19T14:38:58.548Z), send>=700kbps=125ms (2026-04-19T14:38:58.548Z), send>=900kbps=125ms (2026-04-19T14:38:58.548Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3803ms, recovering=-, stable=303ms, congested=-, firstAction=3803ms, L0=-, L1=3803ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=125ms, recovering=-, stable=14127ms, congested=-, firstAction=14127ms, L0=14127ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:40:07.206Z；firstRecovering=6807ms (2026-04-19T14:40:14.013Z)；firstStable=9306ms (2026-04-19T14:40:16.512Z)；firstL0=10306ms (2026-04-19T14:40:17.512Z) |
| 恢复诊断 | raw=loss<3%=308ms (2026-04-19T14:40:07.514Z), rtt<120ms=308ms (2026-04-19T14:40:07.514Z), jitter<28ms=6307ms (2026-04-19T14:40:13.513Z), jitter<18ms=16806ms (2026-04-19T14:40:24.012Z)；target=target>=120kbps=7306ms (2026-04-19T14:40:14.512Z), target>=300kbps=8806ms (2026-04-19T14:40:16.012Z), target>=500kbps=9808ms (2026-04-19T14:40:17.014Z), target>=700kbps=10306ms (2026-04-19T14:40:17.512Z), target>=900kbps=12806ms (2026-04-19T14:40:20.012Z)；send=send>=300kbps=7306ms (2026-04-19T14:40:14.512Z), send>=500kbps=7306ms (2026-04-19T14:40:14.512Z), send>=700kbps=7306ms (2026-04-19T14:40:14.512Z), send>=900kbps=8806ms (2026-04-19T14:40:16.012Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1874ms, recovering=-, stable=372ms, congested=3372ms, firstAction=1874ms, L0=-, L1=1874ms, L2=3372ms, L3=3872ms, L4=4374ms, audioOnly=-；recovery: warning=12806ms, recovering=6807ms, stable=9306ms, congested=308ms, firstAction=308ms, L0=10306ms, L1=9306ms, L2=8306ms, L3=6807ms, L4=308ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:41:16.001Z；firstRecovering=-；firstStable=3294ms (2026-04-19T14:41:19.295Z)；firstL0=3294ms (2026-04-19T14:41:19.295Z) |
| 恢复诊断 | raw=loss<3%=295ms (2026-04-19T14:41:16.296Z), rtt<120ms=1794ms (2026-04-19T14:41:17.795Z), jitter<28ms=295ms (2026-04-19T14:41:16.296Z), jitter<18ms=295ms (2026-04-19T14:41:16.296Z)；target=target>=120kbps=295ms (2026-04-19T14:41:16.296Z), target>=300kbps=295ms (2026-04-19T14:41:16.296Z), target>=500kbps=295ms (2026-04-19T14:41:16.296Z), target>=700kbps=295ms (2026-04-19T14:41:16.296Z), target>=900kbps=6921ms (2026-04-19T14:41:22.922Z)；send=send>=300kbps=295ms (2026-04-19T14:41:16.296Z), send>=500kbps=295ms (2026-04-19T14:41:16.296Z), send>=700kbps=295ms (2026-04-19T14:41:16.296Z), send>=900kbps=2305ms (2026-04-19T14:41:18.306Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1874ms, recovering=-, stable=384ms, congested=-, firstAction=1874ms, L0=-, L1=1874ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=295ms, recovering=-, stable=3294ms, congested=-, firstAction=3294ms, L0=3294ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:42:24.777Z；firstRecovering=-；firstStable=5813ms (2026-04-19T14:42:30.590Z)；firstL0=5813ms (2026-04-19T14:42:30.590Z) |
| 恢复诊断 | raw=loss<3%=313ms (2026-04-19T14:42:25.090Z), rtt<120ms=313ms (2026-04-19T14:42:25.090Z), jitter<28ms=813ms (2026-04-19T14:42:25.590Z), jitter<18ms=2814ms (2026-04-19T14:42:27.591Z)；target=target>=120kbps=313ms (2026-04-19T14:42:25.090Z), target>=300kbps=313ms (2026-04-19T14:42:25.090Z), target>=500kbps=313ms (2026-04-19T14:42:25.090Z), target>=700kbps=313ms (2026-04-19T14:42:25.090Z), target>=900kbps=6313ms (2026-04-19T14:42:31.090Z)；send=send>=300kbps=313ms (2026-04-19T14:42:25.090Z), send>=500kbps=313ms (2026-04-19T14:42:25.090Z), send>=700kbps=1314ms (2026-04-19T14:42:26.091Z), send>=900kbps=6813ms (2026-04-19T14:42:31.590Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2365ms, recovering=-, stable=365ms, congested=-, firstAction=2365ms, L0=-, L1=2365ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=313ms, recovering=-, stable=5813ms, congested=-, firstAction=5813ms, L0=5813ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:43:33.518Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=341ms, recovering=-, stable=-, congested=1341ms, firstAction=1341ms, L0=-, L1=-, L2=1341ms, L3=1841ms, L4=2341ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 241 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T14:45:32.315Z；firstRecovering=17245ms (2026-04-19T14:45:49.560Z)；firstStable=19746ms (2026-04-19T14:45:52.061Z)；firstL0=21743ms (2026-04-19T14:45:54.058Z) |
| 恢复诊断 | raw=loss<3%=745ms (2026-04-19T14:45:33.060Z), rtt<120ms=1749ms (2026-04-19T14:45:34.064Z), jitter<28ms=16759ms (2026-04-19T14:45:49.074Z), jitter<18ms=17744ms (2026-04-19T14:45:50.059Z)；target=target>=120kbps=13743ms (2026-04-19T14:45:46.058Z), target>=300kbps=18245ms (2026-04-19T14:45:50.560Z), target>=500kbps=20746ms (2026-04-19T14:45:53.061Z), target>=700kbps=21244ms (2026-04-19T14:45:53.559Z), target>=900kbps=22246ms (2026-04-19T14:45:54.561Z)；send=send>=300kbps=18245ms (2026-04-19T14:45:50.560Z), send>=500kbps=20244ms (2026-04-19T14:45:52.559Z), send>=700kbps=20746ms (2026-04-19T14:45:53.061Z), send>=900kbps=21244ms (2026-04-19T14:45:53.559Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4812ms, recovering=-, stable=2316ms, congested=5312ms, firstAction=4812ms, L0=-, L1=4812ms, L2=5312ms, L3=5813ms, L4=6315ms, audioOnly=-；recovery: warning=24247ms, recovering=17245ms, stable=19746ms, congested=248ms, firstAction=248ms, L0=21743ms, L1=20244ms, L2=18743ms, L3=17245ms, L4=248ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 231 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:48:01.089Z；firstRecovering=17321ms (2026-04-19T14:48:18.410Z)；firstStable=19820ms (2026-04-19T14:48:20.909Z)；firstL0=20821ms (2026-04-19T14:48:21.910Z) |
| 恢复诊断 | raw=loss<3%=326ms (2026-04-19T14:48:01.415Z), rtt<120ms=1332ms (2026-04-19T14:48:02.421Z), jitter<28ms=16821ms (2026-04-19T14:48:17.910Z), jitter<18ms=16821ms (2026-04-19T14:48:17.910Z)；target=target>=120kbps=11321ms (2026-04-19T14:48:12.410Z), target>=300kbps=17821ms (2026-04-19T14:48:18.910Z), target>=500kbps=19333ms (2026-04-19T14:48:20.422Z), target>=700kbps=20321ms (2026-04-19T14:48:21.410Z), target>=900kbps=22821ms (2026-04-19T14:48:23.910Z)；send=send>=300kbps=18322ms (2026-04-19T14:48:19.411Z), send>=500kbps=18322ms (2026-04-19T14:48:19.411Z), send>=700kbps=19333ms (2026-04-19T14:48:20.422Z), send>=900kbps=21336ms (2026-04-19T14:48:22.425Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3387ms, recovering=27433ms, stable=2392ms, congested=3893ms, firstAction=3387ms, L0=-, L1=3387ms, L2=3893ms, L3=4388ms, L4=4887ms, audioOnly=-；recovery: warning=22821ms, recovering=17321ms, stable=19820ms, congested=326ms, firstAction=326ms, L0=20821ms, L1=19820ms, L2=18822ms, L3=17321ms, L4=326ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 230 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-19T14:50:29.930Z；firstRecovering=19289ms (2026-04-19T14:50:49.219Z)；firstStable=21774ms (2026-04-19T14:50:51.704Z)；firstL0=23773ms (2026-04-19T14:50:53.703Z) |
| 恢复诊断 | raw=loss<3%=274ms (2026-04-19T14:50:30.204Z), rtt<120ms=2275ms (2026-04-19T14:50:32.205Z), jitter<28ms=18773ms (2026-04-19T14:50:48.703Z), jitter<18ms=18773ms (2026-04-19T14:50:48.703Z)；target=target>=120kbps=13773ms (2026-04-19T14:50:43.703Z), target>=300kbps=19773ms (2026-04-19T14:50:49.703Z), target>=500kbps=21273ms (2026-04-19T14:50:51.203Z), target>=700kbps=23773ms (2026-04-19T14:50:53.703Z), target>=900kbps=-；send=send>=300kbps=20273ms (2026-04-19T14:50:50.203Z), send>=500kbps=21273ms (2026-04-19T14:50:51.203Z), send>=700kbps=23300ms (2026-04-19T14:50:53.230Z), send>=900kbps=25773ms (2026-04-19T14:50:55.703Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4366ms, recovering=18366ms, stable=2366ms, congested=4867ms, firstAction=4366ms, L0=21866ms, L1=4366ms, L2=4867ms, L3=5369ms, L4=5870ms, audioOnly=-；recovery: warning=26273ms, recovering=19289ms, stable=21774ms, congested=274ms, firstAction=274ms, L0=23773ms, L1=22288ms, L2=20773ms, L3=19289ms, L4=274ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 22 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:51:23.780Z；firstRecovering=8717ms (2026-04-19T14:51:32.497Z)；firstStable=9215ms (2026-04-19T14:51:32.995Z)；firstL0=13216ms (2026-04-19T14:51:36.996Z) |
| 恢复诊断 | raw=loss<3%=217ms (2026-04-19T14:51:23.997Z), rtt<120ms=217ms (2026-04-19T14:51:23.997Z), jitter<28ms=1720ms (2026-04-19T14:51:25.500Z), jitter<18ms=4720ms (2026-04-19T14:51:28.500Z)；target=target>=120kbps=217ms (2026-04-19T14:51:23.997Z), target>=300kbps=217ms (2026-04-19T14:51:23.997Z), target>=500kbps=217ms (2026-04-19T14:51:23.997Z), target>=700kbps=12215ms (2026-04-19T14:51:35.995Z), target>=900kbps=13716ms (2026-04-19T14:51:37.496Z)；send=send>=300kbps=217ms (2026-04-19T14:51:23.997Z), send>=500kbps=217ms (2026-04-19T14:51:23.997Z), send>=700kbps=217ms (2026-04-19T14:51:23.997Z), send>=900kbps=715ms (2026-04-19T14:51:24.495Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2887ms, recovering=-, stable=391ms, congested=-, firstAction=2887ms, L0=-, L1=2887ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=8717ms, stable=9215ms, congested=217ms, firstAction=217ms, L0=13216ms, L1=11715ms, L2=217ms, L3=715ms, L4=1215ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 46 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:52:20.575Z；firstRecovering=15337ms (2026-04-19T14:52:35.912Z)；firstStable=17896ms (2026-04-19T14:52:38.471Z)；firstL0=19338ms (2026-04-19T14:52:39.913Z) |
| 恢复诊断 | raw=loss<3%=338ms (2026-04-19T14:52:20.913Z), rtt<120ms=1339ms (2026-04-19T14:52:21.914Z), jitter<28ms=338ms (2026-04-19T14:52:20.913Z), jitter<18ms=338ms (2026-04-19T14:52:20.913Z)；target=target>=120kbps=3341ms (2026-04-19T14:52:23.916Z), target>=300kbps=15837ms (2026-04-19T14:52:36.412Z), target>=500kbps=17896ms (2026-04-19T14:52:38.471Z), target>=700kbps=18338ms (2026-04-19T14:52:38.913Z), target>=900kbps=19837ms (2026-04-19T14:52:40.412Z)；send=send>=300kbps=837ms (2026-04-19T14:52:21.412Z), send>=500kbps=17896ms (2026-04-19T14:52:38.471Z), send>=700kbps=17896ms (2026-04-19T14:52:38.471Z), send>=900kbps=18338ms (2026-04-19T14:52:38.913Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2393ms, recovering=-, stable=393ms, congested=2893ms, firstAction=2393ms, L0=-, L1=2393ms, L2=2893ms, L3=3394ms, L4=3893ms, audioOnly=-；recovery: warning=-, recovering=15337ms, stable=17896ms, congested=338ms, firstAction=338ms, L0=19338ms, L1=17896ms, L2=16858ms, L3=15337ms, L4=338ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-19T14:53:16.446Z；firstRecovering=-；firstStable=13716ms (2026-04-19T14:53:30.162Z)；firstL0=13716ms (2026-04-19T14:53:30.162Z) |
| 恢复诊断 | raw=loss<3%=216ms (2026-04-19T14:53:16.662Z), rtt<120ms=716ms (2026-04-19T14:53:17.162Z), jitter<28ms=4716ms (2026-04-19T14:53:21.162Z), jitter<18ms=9216ms (2026-04-19T14:53:25.662Z)；target=target>=120kbps=216ms (2026-04-19T14:53:16.662Z), target>=300kbps=216ms (2026-04-19T14:53:16.662Z), target>=500kbps=216ms (2026-04-19T14:53:16.662Z), target>=700kbps=2716ms (2026-04-19T14:53:19.162Z), target>=900kbps=14216ms (2026-04-19T14:53:30.662Z)；send=send>=300kbps=216ms (2026-04-19T14:53:16.662Z), send>=500kbps=216ms (2026-04-19T14:53:16.662Z), send>=700kbps=716ms (2026-04-19T14:53:17.162Z), send>=900kbps=7217ms (2026-04-19T14:53:23.663Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1770ms, recovering=-, stable=270ms, congested=-, firstAction=1770ms, L0=-, L1=1770ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=216ms, recovering=-, stable=13716ms, congested=-, firstAction=13716ms, L0=13716ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 26 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L2，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-19T14:54:10.240Z；firstRecovering=9258ms (2026-04-19T14:54:19.498Z)；firstStable=11268ms (2026-04-19T14:54:21.508Z)；firstL0=12770ms (2026-04-19T14:54:23.010Z) |
| 恢复诊断 | raw=loss<3%=758ms (2026-04-19T14:54:10.998Z), rtt<120ms=258ms (2026-04-19T14:54:10.498Z), jitter<28ms=4259ms (2026-04-19T14:54:14.499Z), jitter<18ms=6759ms (2026-04-19T14:54:16.999Z)；target=target>=120kbps=258ms (2026-04-19T14:54:10.498Z), target>=300kbps=258ms (2026-04-19T14:54:10.498Z), target>=500kbps=12272ms (2026-04-19T14:54:22.512Z), target>=700kbps=12770ms (2026-04-19T14:54:23.010Z), target>=900kbps=13261ms (2026-04-19T14:54:23.501Z)；send=send>=300kbps=258ms (2026-04-19T14:54:10.498Z), send>=500kbps=258ms (2026-04-19T14:54:10.498Z), send>=700kbps=258ms (2026-04-19T14:54:10.498Z), send>=900kbps=258ms (2026-04-19T14:54:10.498Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1821ms, recovering=-, stable=322ms, congested=4822ms, firstAction=1821ms, L0=-, L1=1821ms, L2=4822ms, L3=-, L4=-, audioOnly=-；recovery: warning=15258ms, recovering=9258ms, stable=11268ms, congested=258ms, firstAction=258ms, L0=12770ms, L1=11758ms, L2=10760ms, L3=258ms, L4=758ms, audioOnly=- |

