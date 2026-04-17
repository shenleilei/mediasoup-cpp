# 上行 QoS 逐 Case 最终结果

> 当前状态说明
>
> 这是 browser uplink full matrix 的逐 case 展开页。
> 如果只想先看当前总口径和入口，先看 [qos-status.md](./qos-status.md)。

生成时间：`2026-04-13T03:34:38.058Z`

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
| 恢复里程碑 | start=2026-04-13T02:41:19.483Z；firstRecovering=-；firstStable=186ms (2026-04-13T02:41:19.669Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=186ms (2026-04-13T02:41:19.669Z), rtt<120ms=186ms (2026-04-13T02:41:19.669Z), jitter<28ms=186ms (2026-04-13T02:41:19.669Z), jitter<18ms=186ms (2026-04-13T02:41:19.669Z)；target=target>=120kbps=186ms (2026-04-13T02:41:19.669Z), target>=300kbps=186ms (2026-04-13T02:41:19.669Z), target>=500kbps=186ms (2026-04-13T02:41:19.669Z), target>=700kbps=186ms (2026-04-13T02:41:19.669Z), target>=900kbps=186ms (2026-04-13T02:41:19.669Z)；send=send>=300kbps=186ms (2026-04-13T02:41:19.669Z), send>=500kbps=186ms (2026-04-13T02:41:19.669Z), send>=700kbps=186ms (2026-04-13T02:41:19.669Z), send>=900kbps=186ms (2026-04-13T02:41:19.669Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=347ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=186ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T02:42:28.190Z；firstRecovering=-；firstStable=294ms (2026-04-13T02:42:28.484Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=294ms (2026-04-13T02:42:28.484Z), rtt<120ms=294ms (2026-04-13T02:42:28.484Z), jitter<28ms=294ms (2026-04-13T02:42:28.484Z), jitter<18ms=294ms (2026-04-13T02:42:28.484Z)；target=target>=120kbps=294ms (2026-04-13T02:42:28.484Z), target>=300kbps=294ms (2026-04-13T02:42:28.484Z), target>=500kbps=294ms (2026-04-13T02:42:28.484Z), target>=700kbps=294ms (2026-04-13T02:42:28.484Z), target>=900kbps=294ms (2026-04-13T02:42:28.484Z)；send=send>=300kbps=294ms (2026-04-13T02:42:28.484Z), send>=500kbps=294ms (2026-04-13T02:42:28.484Z), send>=700kbps=294ms (2026-04-13T02:42:28.484Z), send>=900kbps=294ms (2026-04-13T02:42:28.484Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=396ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=294ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 25 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:43:37.279Z；firstRecovering=14851ms (2026-04-13T02:43:52.130Z)；firstStable=17351ms (2026-04-13T02:43:54.630Z)；firstL0=18351ms (2026-04-13T02:43:55.630Z) |
| 恢复诊断 | raw=loss<3%=351ms (2026-04-13T02:43:37.630Z), rtt<120ms=351ms (2026-04-13T02:43:37.630Z), jitter<28ms=351ms (2026-04-13T02:43:37.630Z), jitter<18ms=-；target=target>=120kbps=351ms (2026-04-13T02:43:37.630Z), target>=300kbps=351ms (2026-04-13T02:43:37.630Z), target>=500kbps=351ms (2026-04-13T02:43:37.630Z), target>=700kbps=17851ms (2026-04-13T02:43:55.130Z), target>=900kbps=18851ms (2026-04-13T02:43:56.130Z)；send=send>=300kbps=351ms (2026-04-13T02:43:37.630Z), send>=500kbps=351ms (2026-04-13T02:43:37.630Z), send>=700kbps=351ms (2026-04-13T02:43:37.630Z), send>=900kbps=351ms (2026-04-13T02:43:37.630Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=171ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=351ms, recovering=14851ms, stable=17351ms, congested=4851ms, firstAction=4851ms, L0=18351ms, L1=17351ms, L2=4851ms, L3=5351ms, L4=5851ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T02:44:46.399Z；firstRecovering=-；firstStable=168ms (2026-04-13T02:44:46.567Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=168ms (2026-04-13T02:44:46.567Z), rtt<120ms=168ms (2026-04-13T02:44:46.567Z), jitter<28ms=168ms (2026-04-13T02:44:46.567Z), jitter<18ms=168ms (2026-04-13T02:44:46.567Z)；target=target>=120kbps=168ms (2026-04-13T02:44:46.567Z), target>=300kbps=168ms (2026-04-13T02:44:46.567Z), target>=500kbps=168ms (2026-04-13T02:44:46.567Z), target>=700kbps=168ms (2026-04-13T02:44:46.567Z), target>=900kbps=168ms (2026-04-13T02:44:46.567Z)；send=send>=300kbps=168ms (2026-04-13T02:44:46.567Z), send>=500kbps=168ms (2026-04-13T02:44:46.567Z), send>=700kbps=168ms (2026-04-13T02:44:46.567Z), send>=900kbps=168ms (2026-04-13T02:44:46.567Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=358ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=168ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 71 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:45:55.577Z；firstRecovering=9319ms (2026-04-13T02:46:04.896Z)；firstStable=11819ms (2026-04-13T02:46:07.396Z)；firstL0=13819ms (2026-04-13T02:46:09.396Z) |
| 恢复诊断 | raw=loss<3%=319ms (2026-04-13T02:45:55.896Z), rtt<120ms=1319ms (2026-04-13T02:45:56.896Z), jitter<28ms=7819ms (2026-04-13T02:46:03.396Z), jitter<18ms=18819ms (2026-04-13T02:46:14.396Z)；target=target>=120kbps=319ms (2026-04-13T02:45:55.896Z), target>=300kbps=819ms (2026-04-13T02:45:56.396Z), target>=500kbps=1819ms (2026-04-13T02:45:57.396Z), target>=700kbps=13319ms (2026-04-13T02:46:08.896Z), target>=900kbps=29819ms (2026-04-13T02:46:25.396Z)；send=send>=300kbps=319ms (2026-04-13T02:45:55.896Z), send>=500kbps=319ms (2026-04-13T02:45:55.896Z), send>=700kbps=10319ms (2026-04-13T02:46:05.896Z), send>=900kbps=13819ms (2026-04-13T02:46:09.396Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1349ms, recovering=14349ms, stable=349ms, congested=1849ms, firstAction=1349ms, L0=18849ms, L1=1349ms, L2=1849ms, L3=2349ms, L4=2850ms, audioOnly=-；recovery: warning=319ms, recovering=9319ms, stable=11819ms, congested=1319ms, firstAction=1319ms, L0=13819ms, L1=1319ms, L2=1819ms, L3=2319ms, L4=2819ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 80 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-13T02:47:04.846Z；firstRecovering=14721ms (2026-04-13T02:47:19.567Z)；firstStable=17221ms (2026-04-13T02:47:22.067Z)；firstL0=19222ms (2026-04-13T02:47:24.068Z) |
| 恢复诊断 | raw=loss<3%=723ms (2026-04-13T02:47:05.569Z), rtt<120ms=1222ms (2026-04-13T02:47:06.068Z), jitter<28ms=14224ms (2026-04-13T02:47:19.070Z), jitter<18ms=14224ms (2026-04-13T02:47:19.070Z)；target=target>=120kbps=222ms (2026-04-13T02:47:05.068Z), target>=300kbps=15221ms (2026-04-13T02:47:20.067Z), target>=500kbps=18721ms (2026-04-13T02:47:23.567Z), target>=700kbps=20722ms (2026-04-13T02:47:25.568Z), target>=900kbps=-；send=send>=300kbps=222ms (2026-04-13T02:47:05.068Z), send>=500kbps=15221ms (2026-04-13T02:47:20.067Z), send>=700kbps=18721ms (2026-04-13T02:47:23.567Z), send>=900kbps=21721ms (2026-04-13T02:47:26.567Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1833ms, recovering=15833ms, stable=334ms, congested=2333ms, firstAction=1833ms, L0=-, L1=1833ms, L2=2333ms, L3=2844ms, L4=3333ms, audioOnly=-；recovery: warning=222ms, recovering=14721ms, stable=17221ms, congested=723ms, firstAction=723ms, L0=19222ms, L1=17722ms, L2=723ms, L3=1222ms, L4=1722ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 74 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:48:14.333Z；firstRecovering=15999ms (2026-04-13T02:48:30.332Z)；firstStable=18498ms (2026-04-13T02:48:32.831Z)；firstL0=20499ms (2026-04-13T02:48:34.832Z) |
| 恢复诊断 | raw=loss<3%=1000ms (2026-04-13T02:48:15.333Z), rtt<120ms=500ms (2026-04-13T02:48:14.833Z), jitter<28ms=15498ms (2026-04-13T02:48:29.831Z), jitter<18ms=20499ms (2026-04-13T02:48:34.832Z)；target=target>=120kbps=13999ms (2026-04-13T02:48:28.332Z), target>=300kbps=16498ms (2026-04-13T02:48:30.831Z), target>=500kbps=17999ms (2026-04-13T02:48:32.332Z), target>=700kbps=19499ms (2026-04-13T02:48:33.832Z), target>=900kbps=21001ms (2026-04-13T02:48:35.334Z)；send=send>=300kbps=1000ms (2026-04-13T02:48:15.333Z), send>=500kbps=17999ms (2026-04-13T02:48:32.332Z), send>=700kbps=19499ms (2026-04-13T02:48:33.832Z), send>=900kbps=19499ms (2026-04-13T02:48:33.832Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1851ms, recovering=-, stable=351ms, congested=2351ms, firstAction=1851ms, L0=-, L1=1851ms, L2=2351ms, L3=2851ms, L4=3351ms, audioOnly=-；recovery: warning=-, recovering=15999ms, stable=18498ms, congested=500ms, firstAction=500ms, L0=20499ms, L1=18999ms, L2=17498ms, L3=15999ms, L4=500ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 72 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:49:24.817Z；firstRecovering=15011ms (2026-04-13T02:49:39.828Z)；firstStable=17511ms (2026-04-13T02:49:42.328Z)；firstL0=19011ms (2026-04-13T02:49:43.828Z) |
| 恢复诊断 | raw=loss<3%=11ms (2026-04-13T02:49:24.828Z), rtt<120ms=1011ms (2026-04-13T02:49:25.828Z), jitter<28ms=14511ms (2026-04-13T02:49:39.328Z), jitter<18ms=15511ms (2026-04-13T02:49:40.328Z)；target=target>=120kbps=7511ms (2026-04-13T02:49:32.328Z), target>=300kbps=15511ms (2026-04-13T02:49:40.328Z), target>=500kbps=17012ms (2026-04-13T02:49:41.829Z), target>=700kbps=18011ms (2026-04-13T02:49:42.828Z), target>=900kbps=19511ms (2026-04-13T02:49:44.328Z)；send=send>=300kbps=15511ms (2026-04-13T02:49:40.328Z), send>=500kbps=17012ms (2026-04-13T02:49:41.829Z), send>=700kbps=17511ms (2026-04-13T02:49:42.328Z), send>=900kbps=18011ms (2026-04-13T02:49:42.828Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2855ms, recovering=-, stable=355ms, congested=3355ms, firstAction=2855ms, L0=-, L1=2855ms, L2=3355ms, L3=3855ms, L4=4355ms, audioOnly=-；recovery: warning=-, recovering=15011ms, stable=17511ms, congested=11ms, firstAction=11ms, L0=19011ms, L1=17511ms, L2=16511ms, L3=15011ms, L4=11ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 123 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:51:01.209Z；firstRecovering=16111ms (2026-04-13T02:51:17.320Z)；firstStable=18611ms (2026-04-13T02:51:19.820Z)；firstL0=20611ms (2026-04-13T02:51:21.820Z) |
| 恢复诊断 | raw=loss<3%=111ms (2026-04-13T02:51:01.320Z), rtt<120ms=111ms (2026-04-13T02:51:01.320Z), jitter<28ms=15611ms (2026-04-13T02:51:16.820Z), jitter<18ms=21611ms (2026-04-13T02:51:22.820Z)；target=target>=120kbps=13111ms (2026-04-13T02:51:14.320Z), target>=300kbps=16612ms (2026-04-13T02:51:17.821Z), target>=500kbps=18111ms (2026-04-13T02:51:19.320Z), target>=700kbps=19611ms (2026-04-13T02:51:20.820Z), target>=900kbps=21611ms (2026-04-13T02:51:22.820Z)；send=send>=300kbps=611ms (2026-04-13T02:51:01.820Z), send>=500kbps=1111ms (2026-04-13T02:51:02.320Z), send>=700kbps=18611ms (2026-04-13T02:51:19.820Z), send>=900kbps=21111ms (2026-04-13T02:51:22.320Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4309ms, recovering=-, stable=309ms, congested=4809ms, firstAction=4309ms, L0=-, L1=4309ms, L2=4809ms, L3=5309ms, L4=5809ms, audioOnly=-；recovery: warning=-, recovering=16111ms, stable=18611ms, congested=111ms, firstAction=111ms, L0=20611ms, L1=19112ms, L2=17611ms, L3=16111ms, L4=111ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T02:52:10.146Z；firstRecovering=-；firstStable=203ms (2026-04-13T02:52:10.349Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=203ms (2026-04-13T02:52:10.349Z), rtt<120ms=203ms (2026-04-13T02:52:10.349Z), jitter<28ms=203ms (2026-04-13T02:52:10.349Z), jitter<18ms=203ms (2026-04-13T02:52:10.349Z)；target=target>=120kbps=203ms (2026-04-13T02:52:10.349Z), target>=300kbps=203ms (2026-04-13T02:52:10.349Z), target>=500kbps=203ms (2026-04-13T02:52:10.349Z), target>=700kbps=203ms (2026-04-13T02:52:10.349Z), target>=900kbps=203ms (2026-04-13T02:52:10.349Z)；send=send>=300kbps=203ms (2026-04-13T02:52:10.349Z), send>=500kbps=203ms (2026-04-13T02:52:10.349Z), send>=700kbps=203ms (2026-04-13T02:52:10.349Z), send>=900kbps=203ms (2026-04-13T02:52:10.349Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=348ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=203ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T02:53:18.942Z；firstRecovering=-；firstStable=215ms (2026-04-13T02:53:19.157Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=215ms (2026-04-13T02:53:19.157Z), rtt<120ms=215ms (2026-04-13T02:53:19.157Z), jitter<28ms=215ms (2026-04-13T02:53:19.157Z), jitter<18ms=215ms (2026-04-13T02:53:19.157Z)；target=target>=120kbps=215ms (2026-04-13T02:53:19.157Z), target>=300kbps=215ms (2026-04-13T02:53:19.157Z), target>=500kbps=215ms (2026-04-13T02:53:19.157Z), target>=700kbps=215ms (2026-04-13T02:53:19.157Z), target>=900kbps=215ms (2026-04-13T02:53:19.157Z)；send=send>=300kbps=215ms (2026-04-13T02:53:19.157Z), send>=500kbps=215ms (2026-04-13T02:53:19.157Z), send>=700kbps=215ms (2026-04-13T02:53:19.157Z), send>=900kbps=215ms (2026-04-13T02:53:19.157Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=358ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=215ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T02:54:27.723Z；firstRecovering=-；firstStable=204ms (2026-04-13T02:54:27.927Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=204ms (2026-04-13T02:54:27.927Z), rtt<120ms=204ms (2026-04-13T02:54:27.927Z), jitter<28ms=204ms (2026-04-13T02:54:27.927Z), jitter<18ms=204ms (2026-04-13T02:54:27.927Z)；target=target>=120kbps=204ms (2026-04-13T02:54:27.927Z), target>=300kbps=204ms (2026-04-13T02:54:27.927Z), target>=500kbps=204ms (2026-04-13T02:54:27.927Z), target>=700kbps=204ms (2026-04-13T02:54:27.927Z), target>=900kbps=204ms (2026-04-13T02:54:27.927Z)；send=send>=300kbps=204ms (2026-04-13T02:54:27.927Z), send>=500kbps=204ms (2026-04-13T02:54:27.927Z), send>=700kbps=204ms (2026-04-13T02:54:27.927Z), send>=900kbps=204ms (2026-04-13T02:54:27.927Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=346ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=204ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:55:36.742Z；firstRecovering=-；firstStable=3478ms (2026-04-13T02:55:40.220Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=978ms (2026-04-13T02:55:37.720Z), rtt<120ms=478ms (2026-04-13T02:55:37.220Z), jitter<28ms=478ms (2026-04-13T02:55:37.220Z), jitter<18ms=13979ms (2026-04-13T02:55:50.721Z)；target=target>=120kbps=478ms (2026-04-13T02:55:37.220Z), target>=300kbps=478ms (2026-04-13T02:55:37.220Z), target>=500kbps=478ms (2026-04-13T02:55:37.220Z), target>=700kbps=478ms (2026-04-13T02:55:37.220Z), target>=900kbps=478ms (2026-04-13T02:55:37.220Z)；send=send>=300kbps=478ms (2026-04-13T02:55:37.220Z), send>=500kbps=478ms (2026-04-13T02:55:37.220Z), send>=700kbps=478ms (2026-04-13T02:55:37.220Z), send>=900kbps=478ms (2026-04-13T02:55:37.220Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2356ms, recovering=-, stable=356ms, congested=-, firstAction=2356ms, L0=15356ms, L1=2356ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=478ms, recovering=-, stable=3478ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 35 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:56:45.571Z；firstRecovering=5186ms (2026-04-13T02:56:50.757Z)；firstStable=5686ms (2026-04-13T02:56:51.257Z)；firstL0=9686ms (2026-04-13T02:56:55.257Z) |
| 恢复诊断 | raw=loss<3%=186ms (2026-04-13T02:56:45.757Z), rtt<120ms=186ms (2026-04-13T02:56:45.757Z), jitter<28ms=186ms (2026-04-13T02:56:45.757Z), jitter<18ms=186ms (2026-04-13T02:56:45.757Z)；target=target>=120kbps=1186ms (2026-04-13T02:56:46.757Z), target>=300kbps=7186ms (2026-04-13T02:56:52.757Z), target>=500kbps=8686ms (2026-04-13T02:56:54.257Z), target>=700kbps=10186ms (2026-04-13T02:56:55.757Z), target>=900kbps=17186ms (2026-04-13T02:57:02.757Z)；send=send>=300kbps=5686ms (2026-04-13T02:56:51.257Z), send>=500kbps=5686ms (2026-04-13T02:56:51.257Z), send>=700kbps=5686ms (2026-04-13T02:56:51.257Z), send>=900kbps=6186ms (2026-04-13T02:56:51.757Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2839ms, recovering=-, stable=339ms, congested=11339ms, firstAction=2839ms, L0=-, L1=2839ms, L2=11339ms, L3=11839ms, L4=12339ms, audioOnly=-；recovery: warning=12186ms, recovering=5186ms, stable=5686ms, congested=186ms, firstAction=186ms, L0=9686ms, L1=8186ms, L2=6686ms, L3=5186ms, L4=186ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:57:54.330Z；firstRecovering=5218ms (2026-04-13T02:57:59.548Z)；firstStable=7718ms (2026-04-13T02:58:02.048Z)；firstL0=9218ms (2026-04-13T02:58:03.548Z) |
| 恢复诊断 | raw=loss<3%=718ms (2026-04-13T02:57:55.048Z), rtt<120ms=218ms (2026-04-13T02:57:54.548Z), jitter<28ms=4718ms (2026-04-13T02:57:59.048Z), jitter<18ms=13718ms (2026-04-13T02:58:08.048Z)；target=target>=120kbps=218ms (2026-04-13T02:57:54.548Z), target>=300kbps=7218ms (2026-04-13T02:58:01.548Z), target>=500kbps=8218ms (2026-04-13T02:58:02.548Z), target>=700kbps=8718ms (2026-04-13T02:58:03.048Z), target>=900kbps=11718ms (2026-04-13T02:58:06.048Z)；send=send>=300kbps=5718ms (2026-04-13T02:58:00.048Z), send>=500kbps=5718ms (2026-04-13T02:58:00.048Z), send>=700kbps=5718ms (2026-04-13T02:58:00.048Z), send>=900kbps=6218ms (2026-04-13T02:58:00.548Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1872ms, recovering=-, stable=372ms, congested=3872ms, firstAction=1872ms, L0=-, L1=1872ms, L2=3872ms, L3=4372ms, L4=4872ms, audioOnly=-；recovery: warning=11718ms, recovering=5218ms, stable=7718ms, congested=218ms, firstAction=218ms, L0=9218ms, L1=7718ms, L2=6718ms, L3=5218ms, L4=218ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T02:59:03.328Z；firstRecovering=16977ms (2026-04-13T02:59:20.305Z)；firstStable=19477ms (2026-04-13T02:59:22.805Z)；firstL0=20478ms (2026-04-13T02:59:23.806Z) |
| 恢复诊断 | raw=loss<3%=978ms (2026-04-13T02:59:04.306Z), rtt<120ms=478ms (2026-04-13T02:59:03.806Z), jitter<28ms=15977ms (2026-04-13T02:59:19.305Z), jitter<18ms=15977ms (2026-04-13T02:59:19.305Z)；target=target>=120kbps=11978ms (2026-04-13T02:59:15.306Z), target>=300kbps=17478ms (2026-04-13T02:59:20.806Z), target>=500kbps=18977ms (2026-04-13T02:59:22.305Z), target>=700kbps=19977ms (2026-04-13T02:59:23.305Z), target>=900kbps=20978ms (2026-04-13T02:59:24.306Z)；send=send>=300kbps=17478ms (2026-04-13T02:59:20.806Z), send>=500kbps=17478ms (2026-04-13T02:59:20.806Z), send>=700kbps=18977ms (2026-04-13T02:59:22.305Z), send>=900kbps=18977ms (2026-04-13T02:59:22.305Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2830ms, recovering=-, stable=329ms, congested=3329ms, firstAction=2830ms, L0=-, L1=2830ms, L2=3329ms, L3=3830ms, L4=4329ms, audioOnly=-；recovery: warning=-, recovering=16977ms, stable=19477ms, congested=478ms, firstAction=478ms, L0=20478ms, L1=19477ms, L2=18477ms, L3=16977ms, L4=478ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:00:13.805Z；firstRecovering=12006ms (2026-04-13T03:00:25.811Z)；firstStable=14507ms (2026-04-13T03:00:28.312Z)；firstL0=16506ms (2026-04-13T03:00:30.311Z) |
| 恢复诊断 | raw=loss<3%=507ms (2026-04-13T03:00:14.312Z), rtt<120ms=7ms (2026-04-13T03:00:13.812Z), jitter<28ms=11507ms (2026-04-13T03:00:25.312Z), jitter<18ms=15507ms (2026-04-13T03:00:29.312Z)；target=target>=120kbps=12506ms (2026-04-13T03:00:26.311Z), target>=300kbps=14006ms (2026-04-13T03:00:27.811Z), target>=500kbps=14006ms (2026-04-13T03:00:27.811Z), target>=700kbps=15507ms (2026-04-13T03:00:29.312Z), target>=900kbps=17007ms (2026-04-13T03:00:30.812Z)；send=send>=300kbps=12506ms (2026-04-13T03:00:26.311Z), send>=500kbps=12506ms (2026-04-13T03:00:26.311Z), send>=700kbps=12506ms (2026-04-13T03:00:26.311Z), send>=900kbps=14006ms (2026-04-13T03:00:27.811Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5508ms, recovering=-, stable=8ms, congested=6008ms, firstAction=5508ms, L0=-, L1=5508ms, L2=6008ms, L3=6508ms, L4=7007ms, audioOnly=-；recovery: warning=-, recovering=12006ms, stable=14507ms, congested=7ms, firstAction=7ms, L0=16506ms, L1=15007ms, L2=13507ms, L3=12006ms, L4=7ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:01:22.634Z；firstRecovering=-；firstStable=186ms (2026-04-13T03:01:22.820Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=186ms (2026-04-13T03:01:22.820Z), rtt<120ms=186ms (2026-04-13T03:01:22.820Z), jitter<28ms=186ms (2026-04-13T03:01:22.820Z), jitter<18ms=186ms (2026-04-13T03:01:22.820Z)；target=target>=120kbps=186ms (2026-04-13T03:01:22.820Z), target>=300kbps=186ms (2026-04-13T03:01:22.820Z), target>=500kbps=186ms (2026-04-13T03:01:22.820Z), target>=700kbps=186ms (2026-04-13T03:01:22.820Z), target>=900kbps=186ms (2026-04-13T03:01:22.820Z)；send=send>=300kbps=186ms (2026-04-13T03:01:22.820Z), send>=500kbps=186ms (2026-04-13T03:01:22.820Z), send>=700kbps=186ms (2026-04-13T03:01:22.820Z), send>=900kbps=186ms (2026-04-13T03:01:22.820Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=386ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=186ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:02:31.541Z；firstRecovering=-；firstStable=106ms (2026-04-13T03:02:31.647Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=106ms (2026-04-13T03:02:31.647Z), rtt<120ms=106ms (2026-04-13T03:02:31.647Z), jitter<28ms=106ms (2026-04-13T03:02:31.647Z), jitter<18ms=106ms (2026-04-13T03:02:31.647Z)；target=target>=120kbps=106ms (2026-04-13T03:02:31.647Z), target>=300kbps=106ms (2026-04-13T03:02:31.647Z), target>=500kbps=106ms (2026-04-13T03:02:31.647Z), target>=700kbps=106ms (2026-04-13T03:02:31.647Z), target>=900kbps=106ms (2026-04-13T03:02:31.647Z)；send=send>=300kbps=106ms (2026-04-13T03:02:31.647Z), send>=500kbps=106ms (2026-04-13T03:02:31.647Z), send>=700kbps=106ms (2026-04-13T03:02:31.647Z), send>=900kbps=106ms (2026-04-13T03:02:31.647Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3334ms, recovering=-, stable=334ms, congested=-, firstAction=3334ms, L0=13833ms, L1=3334ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=106ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:03:40.546Z；firstRecovering=-；firstStable=3019ms (2026-04-13T03:03:43.565Z)；firstL0=3019ms (2026-04-13T03:03:43.565Z) |
| 恢复诊断 | raw=loss<3%=19ms (2026-04-13T03:03:40.565Z), rtt<120ms=1519ms (2026-04-13T03:03:42.065Z), jitter<28ms=19ms (2026-04-13T03:03:40.565Z), jitter<18ms=19ms (2026-04-13T03:03:40.565Z)；target=target>=120kbps=19ms (2026-04-13T03:03:40.565Z), target>=300kbps=19ms (2026-04-13T03:03:40.565Z), target>=500kbps=19ms (2026-04-13T03:03:40.565Z), target>=700kbps=19ms (2026-04-13T03:03:40.565Z), target>=900kbps=4519ms (2026-04-13T03:03:45.065Z)；send=send>=300kbps=19ms (2026-04-13T03:03:40.565Z), send>=500kbps=19ms (2026-04-13T03:03:40.565Z), send>=700kbps=19ms (2026-04-13T03:03:40.565Z), send>=900kbps=519ms (2026-04-13T03:03:41.065Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4818ms, recovering=-, stable=318ms, congested=-, firstAction=4818ms, L0=-, L1=4818ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=19ms, recovering=-, stable=3019ms, congested=-, firstAction=3019ms, L0=3019ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:04:49.645Z；firstRecovering=-；firstStable=14903ms (2026-04-13T03:05:04.548Z)；firstL0=14903ms (2026-04-13T03:05:04.548Z) |
| 恢复诊断 | raw=loss<3%=902ms (2026-04-13T03:04:50.547Z), rtt<120ms=1403ms (2026-04-13T03:04:51.048Z), jitter<28ms=403ms (2026-04-13T03:04:50.048Z), jitter<18ms=10403ms (2026-04-13T03:05:00.048Z)；target=target>=120kbps=403ms (2026-04-13T03:04:50.048Z), target>=300kbps=403ms (2026-04-13T03:04:50.048Z), target>=500kbps=403ms (2026-04-13T03:04:50.048Z), target>=700kbps=4403ms (2026-04-13T03:04:54.048Z), target>=900kbps=15403ms (2026-04-13T03:05:05.048Z)；send=send>=300kbps=403ms (2026-04-13T03:04:50.048Z), send>=500kbps=403ms (2026-04-13T03:04:50.048Z), send>=700kbps=403ms (2026-04-13T03:04:50.048Z), send>=900kbps=902ms (2026-04-13T03:04:50.547Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2282ms, recovering=-, stable=282ms, congested=-, firstAction=2282ms, L0=-, L1=2282ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=403ms, recovering=-, stable=14903ms, congested=-, firstAction=14903ms, L0=14903ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:05:58.996Z；firstRecovering=4264ms (2026-04-13T03:06:03.260Z)；firstStable=4764ms (2026-04-13T03:06:03.760Z)；firstL0=8764ms (2026-04-13T03:06:07.760Z) |
| 恢复诊断 | raw=loss<3%=264ms (2026-04-13T03:05:59.260Z), rtt<120ms=1264ms (2026-04-13T03:06:00.260Z), jitter<28ms=264ms (2026-04-13T03:05:59.260Z), jitter<18ms=264ms (2026-04-13T03:05:59.260Z)；target=target>=120kbps=264ms (2026-04-13T03:05:59.260Z), target>=300kbps=4764ms (2026-04-13T03:06:03.760Z), target>=500kbps=6264ms (2026-04-13T03:06:05.260Z), target>=700kbps=9264ms (2026-04-13T03:06:08.260Z), target>=900kbps=24764ms (2026-04-13T03:06:23.760Z)；send=send>=300kbps=4764ms (2026-04-13T03:06:03.760Z), send>=500kbps=4764ms (2026-04-13T03:06:03.760Z), send>=700kbps=5264ms (2026-04-13T03:06:04.260Z), send>=900kbps=7264ms (2026-04-13T03:06:06.260Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1758ms, recovering=-, stable=258ms, congested=3258ms, firstAction=1758ms, L0=-, L1=1758ms, L2=3258ms, L3=3758ms, L4=4258ms, audioOnly=-；recovery: warning=11264ms, recovering=4264ms, stable=4764ms, congested=264ms, firstAction=264ms, L0=8764ms, L1=7264ms, L2=5764ms, L3=4264ms, L4=264ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 72 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:07:08.452Z；firstRecovering=4518ms (2026-04-13T03:07:12.970Z)；firstStable=5018ms (2026-04-13T03:07:13.470Z)；firstL0=9018ms (2026-04-13T03:07:17.470Z) |
| 恢复诊断 | raw=loss<3%=18ms (2026-04-13T03:07:08.470Z), rtt<120ms=1018ms (2026-04-13T03:07:09.470Z), jitter<28ms=18ms (2026-04-13T03:07:08.470Z), jitter<18ms=18ms (2026-04-13T03:07:08.470Z)；target=target>=120kbps=18ms (2026-04-13T03:07:08.470Z), target>=300kbps=5018ms (2026-04-13T03:07:13.470Z), target>=500kbps=6518ms (2026-04-13T03:07:14.970Z), target>=700kbps=10518ms (2026-04-13T03:07:18.970Z), target>=900kbps=24018ms (2026-04-13T03:07:32.470Z)；send=send>=300kbps=5018ms (2026-04-13T03:07:13.470Z), send>=500kbps=5018ms (2026-04-13T03:07:13.470Z), send>=700kbps=5018ms (2026-04-13T03:07:13.470Z), send>=900kbps=6518ms (2026-04-13T03:07:14.970Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1188ms, recovering=-, stable=189ms, congested=2188ms, firstAction=1188ms, L0=-, L1=1188ms, L2=2188ms, L3=2688ms, L4=3188ms, audioOnly=-；recovery: warning=11518ms, recovering=4518ms, stable=5018ms, congested=18ms, firstAction=18ms, L0=9018ms, L1=7519ms, L2=6018ms, L3=4518ms, L4=18ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:08:17.296Z；firstRecovering=-；firstStable=131ms (2026-04-13T03:08:17.427Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=131ms (2026-04-13T03:08:17.427Z), rtt<120ms=131ms (2026-04-13T03:08:17.427Z), jitter<28ms=131ms (2026-04-13T03:08:17.427Z), jitter<18ms=131ms (2026-04-13T03:08:17.427Z)；target=target>=120kbps=131ms (2026-04-13T03:08:17.427Z), target>=300kbps=131ms (2026-04-13T03:08:17.427Z), target>=500kbps=131ms (2026-04-13T03:08:17.427Z), target>=700kbps=131ms (2026-04-13T03:08:17.427Z), target>=900kbps=131ms (2026-04-13T03:08:17.427Z)；send=send>=300kbps=131ms (2026-04-13T03:08:17.427Z), send>=500kbps=131ms (2026-04-13T03:08:17.427Z), send>=700kbps=131ms (2026-04-13T03:08:17.427Z), send>=900kbps=131ms (2026-04-13T03:08:17.427Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=346ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=131ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:09:26.231Z；firstRecovering=-；firstStable=117ms (2026-04-13T03:09:26.348Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=117ms (2026-04-13T03:09:26.348Z), rtt<120ms=117ms (2026-04-13T03:09:26.348Z), jitter<28ms=117ms (2026-04-13T03:09:26.348Z), jitter<18ms=117ms (2026-04-13T03:09:26.348Z)；target=target>=120kbps=117ms (2026-04-13T03:09:26.348Z), target>=300kbps=117ms (2026-04-13T03:09:26.348Z), target>=500kbps=117ms (2026-04-13T03:09:26.348Z), target>=700kbps=117ms (2026-04-13T03:09:26.348Z), target>=900kbps=117ms (2026-04-13T03:09:26.348Z)；send=send>=300kbps=117ms (2026-04-13T03:09:26.348Z), send>=500kbps=117ms (2026-04-13T03:09:26.348Z), send>=700kbps=117ms (2026-04-13T03:09:26.348Z), send>=900kbps=117ms (2026-04-13T03:09:26.348Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=296ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=117ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:10:35.145Z；firstRecovering=-；firstStable=7575ms (2026-04-13T03:10:42.720Z)；firstL0=7575ms (2026-04-13T03:10:42.720Z) |
| 恢复诊断 | raw=loss<3%=75ms (2026-04-13T03:10:35.220Z), rtt<120ms=75ms (2026-04-13T03:10:35.220Z), jitter<28ms=1075ms (2026-04-13T03:10:36.220Z), jitter<18ms=3575ms (2026-04-13T03:10:38.720Z)；target=target>=120kbps=75ms (2026-04-13T03:10:35.220Z), target>=300kbps=75ms (2026-04-13T03:10:35.220Z), target>=500kbps=75ms (2026-04-13T03:10:35.220Z), target>=700kbps=75ms (2026-04-13T03:10:35.220Z), target>=900kbps=8075ms (2026-04-13T03:10:43.220Z)；send=send>=300kbps=75ms (2026-04-13T03:10:35.220Z), send>=500kbps=75ms (2026-04-13T03:10:35.220Z), send>=700kbps=75ms (2026-04-13T03:10:35.220Z), send>=900kbps=575ms (2026-04-13T03:10:35.720Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=12373ms, recovering=-, stable=373ms, congested=-, firstAction=12373ms, L0=-, L1=12373ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=75ms, recovering=-, stable=7575ms, congested=-, firstAction=7575ms, L0=7575ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 67 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:11:44.033Z；firstRecovering=5099ms (2026-04-13T03:11:49.132Z)；firstStable=7099ms (2026-04-13T03:11:51.132Z)；firstL0=9599ms (2026-04-13T03:11:53.632Z) |
| 恢复诊断 | raw=loss<3%=99ms (2026-04-13T03:11:44.132Z), rtt<120ms=1599ms (2026-04-13T03:11:45.632Z), jitter<28ms=99ms (2026-04-13T03:11:44.132Z), jitter<18ms=3099ms (2026-04-13T03:11:47.132Z)；target=target>=120kbps=99ms (2026-04-13T03:11:44.132Z), target>=300kbps=5599ms (2026-04-13T03:11:49.632Z), target>=500kbps=7099ms (2026-04-13T03:11:51.132Z), target>=700kbps=10599ms (2026-04-13T03:11:54.632Z), target>=900kbps=25599ms (2026-04-13T03:12:09.632Z)；send=send>=300kbps=5599ms (2026-04-13T03:11:49.632Z), send>=500kbps=5599ms (2026-04-13T03:11:49.632Z), send>=700kbps=6099ms (2026-04-13T03:11:50.132Z), send>=900kbps=7599ms (2026-04-13T03:11:51.632Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2372ms, recovering=-, stable=372ms, congested=5872ms, firstAction=2372ms, L0=-, L1=2372ms, L2=5872ms, L3=6372ms, L4=6872ms, audioOnly=-；recovery: warning=12099ms, recovering=5099ms, stable=7099ms, congested=99ms, firstAction=99ms, L0=9599ms, L1=8099ms, L2=6599ms, L3=5099ms, L4=99ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 69 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:12:52.997Z；firstRecovering=3530ms (2026-04-13T03:12:56.527Z)；firstStable=6030ms (2026-04-13T03:12:59.027Z)；firstL0=8030ms (2026-04-13T03:13:01.027Z) |
| 恢复诊断 | raw=loss<3%=30ms (2026-04-13T03:12:53.027Z), rtt<120ms=1530ms (2026-04-13T03:12:54.527Z), jitter<28ms=3030ms (2026-04-13T03:12:56.027Z), jitter<18ms=18030ms (2026-04-13T03:13:11.027Z)；target=target>=120kbps=30ms (2026-04-13T03:12:53.027Z), target>=300kbps=4030ms (2026-04-13T03:12:57.027Z), target>=500kbps=7530ms (2026-04-13T03:13:00.527Z), target>=700kbps=22030ms (2026-04-13T03:13:15.027Z), target>=900kbps=23030ms (2026-04-13T03:13:16.027Z)；send=send>=300kbps=4030ms (2026-04-13T03:12:57.027Z), send>=500kbps=4030ms (2026-04-13T03:12:57.027Z), send>=700kbps=7030ms (2026-04-13T03:13:00.027Z), send>=900kbps=22030ms (2026-04-13T03:13:15.027Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1302ms, recovering=-, stable=302ms, congested=2302ms, firstAction=1302ms, L0=-, L1=1302ms, L2=2302ms, L3=2802ms, L4=3302ms, audioOnly=-；recovery: warning=10530ms, recovering=3530ms, stable=6030ms, congested=30ms, firstAction=30ms, L0=8030ms, L1=6530ms, L2=5030ms, L3=3530ms, L4=30ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:14:01.892Z；firstRecovering=-；firstStable=15078ms (2026-04-13T03:14:16.970Z)；firstL0=15078ms (2026-04-13T03:14:16.970Z) |
| 恢复诊断 | raw=loss<3%=81ms (2026-04-13T03:14:01.973Z), rtt<120ms=81ms (2026-04-13T03:14:01.973Z), jitter<28ms=81ms (2026-04-13T03:14:01.973Z), jitter<18ms=12578ms (2026-04-13T03:14:14.470Z)；target=target>=120kbps=81ms (2026-04-13T03:14:01.973Z), target>=300kbps=81ms (2026-04-13T03:14:01.973Z), target>=500kbps=81ms (2026-04-13T03:14:01.973Z), target>=700kbps=81ms (2026-04-13T03:14:01.973Z), target>=900kbps=15578ms (2026-04-13T03:14:17.470Z)；send=send>=300kbps=81ms (2026-04-13T03:14:01.973Z), send>=500kbps=81ms (2026-04-13T03:14:01.973Z), send>=700kbps=81ms (2026-04-13T03:14:01.973Z), send>=900kbps=1079ms (2026-04-13T03:14:02.971Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3374ms, recovering=-, stable=374ms, congested=-, firstAction=3374ms, L0=-, L1=3374ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=81ms, recovering=-, stable=15078ms, congested=-, firstAction=15078ms, L0=15078ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 64 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:15:10.984Z；firstRecovering=5405ms (2026-04-13T03:15:16.389Z)；firstStable=5905ms (2026-04-13T03:15:16.889Z)；firstL0=9905ms (2026-04-13T03:15:20.889Z) |
| 恢复诊断 | raw=loss<3%=405ms (2026-04-13T03:15:11.389Z), rtt<120ms=405ms (2026-04-13T03:15:11.389Z), jitter<28ms=405ms (2026-04-13T03:15:11.389Z), jitter<18ms=905ms (2026-04-13T03:15:11.889Z)；target=target>=120kbps=405ms (2026-04-13T03:15:11.389Z), target>=300kbps=5905ms (2026-04-13T03:15:16.889Z), target>=500kbps=8405ms (2026-04-13T03:15:19.389Z), target>=700kbps=11920ms (2026-04-13T03:15:22.904Z), target>=900kbps=25405ms (2026-04-13T03:15:36.389Z)；send=send>=300kbps=5905ms (2026-04-13T03:15:16.889Z), send>=500kbps=5905ms (2026-04-13T03:15:16.889Z), send>=700kbps=7405ms (2026-04-13T03:15:18.389Z), send>=900kbps=11405ms (2026-04-13T03:15:22.389Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1345ms, recovering=9845ms, stable=345ms, congested=1845ms, firstAction=1345ms, L0=14345ms, L1=1345ms, L2=1845ms, L3=2345ms, L4=2845ms, audioOnly=-；recovery: warning=12405ms, recovering=5405ms, stable=5905ms, congested=405ms, firstAction=405ms, L0=9905ms, L1=8405ms, L2=6905ms, L3=5405ms, L4=405ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:16:20.460Z；firstRecovering=16030ms (2026-04-13T03:16:36.490Z)；firstStable=18530ms (2026-04-13T03:16:38.990Z)；firstL0=20530ms (2026-04-13T03:16:40.990Z) |
| 恢复诊断 | raw=loss<3%=30ms (2026-04-13T03:16:20.490Z), rtt<120ms=30ms (2026-04-13T03:16:20.490Z), jitter<28ms=15530ms (2026-04-13T03:16:35.990Z), jitter<18ms=23030ms (2026-04-13T03:16:43.490Z)；target=target>=120kbps=30ms (2026-04-13T03:16:20.490Z), target>=300kbps=16530ms (2026-04-13T03:16:36.990Z), target>=500kbps=18030ms (2026-04-13T03:16:38.490Z), target>=700kbps=19530ms (2026-04-13T03:16:39.990Z), target>=900kbps=21030ms (2026-04-13T03:16:41.490Z)；send=send>=300kbps=16530ms (2026-04-13T03:16:36.990Z), send>=500kbps=18030ms (2026-04-13T03:16:38.490Z), send>=700kbps=18030ms (2026-04-13T03:16:38.490Z), send>=900kbps=18530ms (2026-04-13T03:16:38.990Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2363ms, recovering=-, stable=363ms, congested=2863ms, firstAction=2363ms, L0=-, L1=2363ms, L2=2863ms, L3=3363ms, L4=3863ms, audioOnly=-；recovery: warning=23030ms, recovering=16030ms, stable=18530ms, congested=30ms, firstAction=30ms, L0=20530ms, L1=19030ms, L2=17530ms, L3=16030ms, L4=30ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:17:29.478Z；firstRecovering=-；firstStable=2974ms (2026-04-13T03:17:32.452Z)；firstL0=2974ms (2026-04-13T03:17:32.452Z) |
| 恢复诊断 | raw=loss<3%=974ms (2026-04-13T03:17:30.452Z), rtt<120ms=474ms (2026-04-13T03:17:29.952Z), jitter<28ms=474ms (2026-04-13T03:17:29.952Z), jitter<18ms=974ms (2026-04-13T03:17:30.452Z)；target=target>=120kbps=474ms (2026-04-13T03:17:29.952Z), target>=300kbps=474ms (2026-04-13T03:17:29.952Z), target>=500kbps=474ms (2026-04-13T03:17:29.952Z), target>=700kbps=974ms (2026-04-13T03:17:30.452Z), target>=900kbps=12974ms (2026-04-13T03:17:42.452Z)；send=send>=300kbps=474ms (2026-04-13T03:17:29.952Z), send>=500kbps=474ms (2026-04-13T03:17:29.952Z), send>=700kbps=474ms (2026-04-13T03:17:29.952Z), send>=900kbps=474ms (2026-04-13T03:17:29.952Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1361ms, recovering=-, stable=361ms, congested=-, firstAction=1361ms, L0=-, L1=1361ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=474ms, recovering=-, stable=2974ms, congested=-, firstAction=2974ms, L0=2974ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 66 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:18:38.285Z；firstRecovering=14663ms (2026-04-13T03:18:52.948Z)；firstStable=17163ms (2026-04-13T03:18:55.448Z)；firstL0=18163ms (2026-04-13T03:18:56.448Z) |
| 恢复诊断 | raw=loss<3%=163ms (2026-04-13T03:18:38.448Z), rtt<120ms=163ms (2026-04-13T03:18:38.448Z), jitter<28ms=14163ms (2026-04-13T03:18:52.448Z), jitter<18ms=14663ms (2026-04-13T03:18:52.948Z)；target=target>=120kbps=2163ms (2026-04-13T03:18:40.448Z), target>=300kbps=15163ms (2026-04-13T03:18:53.448Z), target>=500kbps=16663ms (2026-04-13T03:18:54.948Z), target>=700kbps=17663ms (2026-04-13T03:18:55.948Z), target>=900kbps=18663ms (2026-04-13T03:18:56.948Z)；send=send>=300kbps=7663ms (2026-04-13T03:18:45.948Z), send>=500kbps=16663ms (2026-04-13T03:18:54.948Z), send>=700kbps=16663ms (2026-04-13T03:18:54.948Z), send>=900kbps=17663ms (2026-04-13T03:18:55.948Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1819ms, recovering=-, stable=319ms, congested=4319ms, firstAction=1819ms, L0=-, L1=1819ms, L2=4319ms, L3=4819ms, L4=5319ms, audioOnly=-；recovery: warning=-, recovering=14663ms, stable=17163ms, congested=163ms, firstAction=163ms, L0=18163ms, L1=17163ms, L2=16163ms, L3=14663ms, L4=163ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:19:47.368Z；firstRecovering=-；firstStable=3407ms (2026-04-13T03:19:50.775Z)；firstL0=3407ms (2026-04-13T03:19:50.775Z) |
| 恢复诊断 | raw=loss<3%=408ms (2026-04-13T03:19:47.776Z), rtt<120ms=1408ms (2026-04-13T03:19:48.776Z), jitter<28ms=408ms (2026-04-13T03:19:47.776Z), jitter<18ms=408ms (2026-04-13T03:19:47.776Z)；target=target>=120kbps=408ms (2026-04-13T03:19:47.776Z), target>=300kbps=408ms (2026-04-13T03:19:47.776Z), target>=500kbps=408ms (2026-04-13T03:19:47.776Z), target>=700kbps=408ms (2026-04-13T03:19:47.776Z), target>=900kbps=5414ms (2026-04-13T03:19:52.782Z)；send=send>=300kbps=408ms (2026-04-13T03:19:47.776Z), send>=500kbps=408ms (2026-04-13T03:19:47.776Z), send>=700kbps=408ms (2026-04-13T03:19:47.776Z), send>=900kbps=2408ms (2026-04-13T03:19:49.776Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1785ms, recovering=-, stable=284ms, congested=-, firstAction=1785ms, L0=-, L1=1785ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=408ms, recovering=-, stable=3407ms, congested=-, firstAction=3407ms, L0=3407ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:20:56.220Z；firstRecovering=-；firstStable=7116ms (2026-04-13T03:21:03.336Z)；firstL0=7116ms (2026-04-13T03:21:03.336Z) |
| 恢复诊断 | raw=loss<3%=116ms (2026-04-13T03:20:56.336Z), rtt<120ms=116ms (2026-04-13T03:20:56.336Z), jitter<28ms=1616ms (2026-04-13T03:20:57.836Z), jitter<18ms=1616ms (2026-04-13T03:20:57.836Z)；target=target>=120kbps=116ms (2026-04-13T03:20:56.336Z), target>=300kbps=116ms (2026-04-13T03:20:56.336Z), target>=500kbps=116ms (2026-04-13T03:20:56.336Z), target>=700kbps=116ms (2026-04-13T03:20:56.336Z), target>=900kbps=7616ms (2026-04-13T03:21:03.836Z)；send=send>=300kbps=116ms (2026-04-13T03:20:56.336Z), send>=500kbps=116ms (2026-04-13T03:20:56.336Z), send>=700kbps=116ms (2026-04-13T03:20:56.336Z), send>=900kbps=1116ms (2026-04-13T03:20:57.336Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3841ms, recovering=-, stable=341ms, congested=-, firstAction=3841ms, L0=-, L1=3841ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=116ms, recovering=-, stable=7116ms, congested=-, firstAction=7116ms, L0=7116ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 38 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-13T03:22:05.974Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=124ms, recovering=-, stable=-, congested=2124ms, firstAction=2124ms, L0=-, L1=-, L2=2124ms, L3=2624ms, L4=3124ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 239 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:24:32.300Z；firstRecovering=19921ms (2026-04-13T03:24:52.221Z)；firstStable=22397ms (2026-04-13T03:24:54.697Z)；firstL0=23397ms (2026-04-13T03:24:55.697Z) |
| 恢复诊断 | raw=loss<3%=397ms (2026-04-13T03:24:32.697Z), rtt<120ms=1397ms (2026-04-13T03:24:33.697Z), jitter<28ms=19397ms (2026-04-13T03:24:51.697Z), jitter<18ms=19397ms (2026-04-13T03:24:51.697Z)；target=target>=120kbps=15397ms (2026-04-13T03:24:47.697Z), target>=300kbps=20397ms (2026-04-13T03:24:52.697Z), target>=500kbps=21897ms (2026-04-13T03:24:54.197Z), target>=700kbps=22897ms (2026-04-13T03:24:55.197Z), target>=900kbps=23897ms (2026-04-13T03:24:56.197Z)；send=send>=300kbps=20397ms (2026-04-13T03:24:52.697Z), send>=500kbps=21897ms (2026-04-13T03:24:54.197Z), send>=700kbps=22897ms (2026-04-13T03:24:55.197Z), send>=900kbps=22897ms (2026-04-13T03:24:55.197Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=29666ms, firstAction=29666ms, L0=-, L1=-, L2=-, L3=-, L4=29666ms, audioOnly=-；recovery: warning=-, recovering=19921ms, stable=22397ms, congested=397ms, firstAction=397ms, L0=23397ms, L1=22397ms, L2=21397ms, L3=19921ms, L4=397ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 246 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-13T03:27:25.004Z；firstRecovering=22785ms (2026-04-13T03:27:47.789Z)；firstStable=25285ms (2026-04-13T03:27:50.289Z)；firstL0=27285ms (2026-04-13T03:27:52.289Z) |
| 恢复诊断 | raw=loss<3%=285ms (2026-04-13T03:27:25.289Z), rtt<120ms=785ms (2026-04-13T03:27:25.789Z), jitter<28ms=22285ms (2026-04-13T03:27:47.289Z), jitter<18ms=23285ms (2026-04-13T03:27:48.289Z)；target=target>=120kbps=16785ms (2026-04-13T03:27:41.789Z), target>=300kbps=23285ms (2026-04-13T03:27:48.289Z), target>=500kbps=24785ms (2026-04-13T03:27:49.789Z), target>=700kbps=26785ms (2026-04-13T03:27:51.789Z), target>=900kbps=-；send=send>=300kbps=23285ms (2026-04-13T03:27:48.289Z), send>=500kbps=24785ms (2026-04-13T03:27:49.789Z), send>=700kbps=26285ms (2026-04-13T03:27:51.289Z), send>=900kbps=29285ms (2026-04-13T03:27:54.289Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=26142ms, firstAction=26142ms, L0=-, L1=-, L2=-, L3=-, L4=26142ms, audioOnly=-；recovery: warning=29785ms, recovering=22785ms, stable=25285ms, congested=285ms, firstAction=285ms, L0=27285ms, L1=25785ms, L2=24285ms, L3=22785ms, L4=285ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 246 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-13T03:30:12.009Z；firstRecovering=18907ms (2026-04-13T03:30:30.916Z)；firstStable=21408ms (2026-04-13T03:30:33.417Z)；firstL0=23407ms (2026-04-13T03:30:35.416Z) |
| 恢复诊断 | raw=loss<3%=907ms (2026-04-13T03:30:12.916Z), rtt<120ms=2408ms (2026-04-13T03:30:14.417Z), jitter<28ms=18408ms (2026-04-13T03:30:30.417Z), jitter<18ms=18907ms (2026-04-13T03:30:30.916Z)；target=target>=120kbps=22407ms (2026-04-13T03:30:34.416Z), target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=-, send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=20641ms, firstAction=20641ms, L0=-, L1=-, L2=-, L3=-, L4=20641ms, audioOnly=-；recovery: warning=25907ms, recovering=18907ms, stable=21408ms, congested=407ms, firstAction=407ms, L0=23407ms, L1=21408ms, L2=20408ms, L3=18907ms, L4=407ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:31:05.647Z；firstRecovering=8734ms (2026-04-13T03:31:14.381Z)；firstStable=9234ms (2026-04-13T03:31:14.881Z)；firstL0=13234ms (2026-04-13T03:31:18.881Z) |
| 恢复诊断 | raw=loss<3%=734ms (2026-04-13T03:31:06.381Z), rtt<120ms=235ms (2026-04-13T03:31:05.882Z), jitter<28ms=235ms (2026-04-13T03:31:05.882Z), jitter<18ms=3234ms (2026-04-13T03:31:08.881Z)；target=target>=120kbps=235ms (2026-04-13T03:31:05.882Z), target>=300kbps=235ms (2026-04-13T03:31:05.882Z), target>=500kbps=235ms (2026-04-13T03:31:05.882Z), target>=700kbps=12734ms (2026-04-13T03:31:18.381Z), target>=900kbps=13735ms (2026-04-13T03:31:19.382Z)；send=send>=300kbps=235ms (2026-04-13T03:31:05.882Z), send>=500kbps=235ms (2026-04-13T03:31:05.882Z), send>=700kbps=235ms (2026-04-13T03:31:05.882Z), send>=900kbps=235ms (2026-04-13T03:31:05.882Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2351ms, recovering=-, stable=353ms, congested=-, firstAction=2351ms, L0=-, L1=2351ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=8734ms, stable=9234ms, congested=235ms, firstAction=235ms, L0=13234ms, L1=11734ms, L2=235ms, L3=734ms, L4=1234ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 82 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:32:19.733Z；firstRecovering=15899ms (2026-04-13T03:32:35.632Z)；firstStable=18399ms (2026-04-13T03:32:38.132Z)；firstL0=20399ms (2026-04-13T03:32:40.132Z) |
| 恢复诊断 | raw=loss<3%=899ms (2026-04-13T03:32:20.632Z), rtt<120ms=399ms (2026-04-13T03:32:20.132Z), jitter<28ms=15399ms (2026-04-13T03:32:35.132Z), jitter<18ms=22399ms (2026-04-13T03:32:42.132Z)；target=target>=120kbps=2899ms (2026-04-13T03:32:22.632Z), target>=300kbps=16399ms (2026-04-13T03:32:36.132Z), target>=500kbps=17899ms (2026-04-13T03:32:37.632Z), target>=700kbps=19399ms (2026-04-13T03:32:39.132Z), target>=900kbps=21399ms (2026-04-13T03:32:41.132Z)；send=send>=300kbps=16399ms (2026-04-13T03:32:36.132Z), send>=500kbps=17899ms (2026-04-13T03:32:37.632Z), send>=700kbps=18399ms (2026-04-13T03:32:38.132Z), send>=900kbps=18399ms (2026-04-13T03:32:38.132Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3360ms, recovering=-, stable=360ms, congested=3860ms, firstAction=3360ms, L0=-, L1=3360ms, L2=3860ms, L3=4360ms, L4=4860ms, audioOnly=-；recovery: warning=22899ms, recovering=15899ms, stable=18399ms, congested=399ms, firstAction=399ms, L0=20399ms, L1=18899ms, L2=17399ms, L3=15899ms, L4=399ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T03:33:13.897Z；firstRecovering=-；firstStable=16359ms (2026-04-13T03:33:30.256Z)；firstL0=16359ms (2026-04-13T03:33:30.256Z) |
| 恢复诊断 | raw=loss<3%=860ms (2026-04-13T03:33:14.757Z), rtt<120ms=1360ms (2026-04-13T03:33:15.257Z), jitter<28ms=9860ms (2026-04-13T03:33:23.757Z), jitter<18ms=12360ms (2026-04-13T03:33:26.257Z)；target=target>=120kbps=360ms (2026-04-13T03:33:14.257Z), target>=300kbps=360ms (2026-04-13T03:33:14.257Z), target>=500kbps=2360ms (2026-04-13T03:33:16.257Z), target>=700kbps=11859ms (2026-04-13T03:33:25.756Z), target>=900kbps=16859ms (2026-04-13T03:33:30.756Z)；send=send>=300kbps=360ms (2026-04-13T03:33:14.257Z), send>=500kbps=360ms (2026-04-13T03:33:14.257Z), send>=700kbps=360ms (2026-04-13T03:33:14.257Z), send>=900kbps=860ms (2026-04-13T03:33:14.757Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1749ms, recovering=-, stable=250ms, congested=-, firstAction=1749ms, L0=-, L1=1749ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=360ms, recovering=-, stable=16359ms, congested=-, firstAction=16359ms, L0=16359ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 43 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T03:34:07.828Z；firstRecovering=8560ms (2026-04-13T03:34:16.388Z)；firstStable=11060ms (2026-04-13T03:34:18.888Z)；firstL0=12059ms (2026-04-13T03:34:19.887Z) |
| 恢复诊断 | raw=loss<3%=58ms (2026-04-13T03:34:07.886Z), rtt<120ms=560ms (2026-04-13T03:34:08.388Z), jitter<28ms=3560ms (2026-04-13T03:34:11.388Z), jitter<18ms=6059ms (2026-04-13T03:34:13.887Z)；target=target>=120kbps=58ms (2026-04-13T03:34:07.886Z), target>=300kbps=58ms (2026-04-13T03:34:07.886Z), target>=500kbps=58ms (2026-04-13T03:34:07.886Z), target>=700kbps=13059ms (2026-04-13T03:34:20.887Z), target>=900kbps=14560ms (2026-04-13T03:34:22.388Z)；send=send>=300kbps=58ms (2026-04-13T03:34:07.886Z), send>=500kbps=58ms (2026-04-13T03:34:07.886Z), send>=700kbps=58ms (2026-04-13T03:34:07.886Z), send>=900kbps=560ms (2026-04-13T03:34:08.388Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2863ms, recovering=-, stable=364ms, congested=-, firstAction=2863ms, L0=-, L1=2863ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=58ms, recovering=8560ms, stable=11060ms, congested=560ms, firstAction=560ms, L0=12059ms, L1=11060ms, L2=560ms, L3=1060ms, L4=1560ms, audioOnly=- |
