# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-17T02:01:01.157Z`

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
| 恢复里程碑 | start=2026-04-17T01:10:02.583Z；firstRecovering=-；firstStable=418ms (2026-04-17T01:10:03.001Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=418ms (2026-04-17T01:10:03.001Z), rtt<120ms=418ms (2026-04-17T01:10:03.001Z), jitter<28ms=418ms (2026-04-17T01:10:03.001Z), jitter<18ms=418ms (2026-04-17T01:10:03.001Z)；target=target>=120kbps=418ms (2026-04-17T01:10:03.001Z), target>=300kbps=418ms (2026-04-17T01:10:03.001Z), target>=500kbps=418ms (2026-04-17T01:10:03.001Z), target>=700kbps=418ms (2026-04-17T01:10:03.001Z), target>=900kbps=418ms (2026-04-17T01:10:03.001Z)；send=send>=300kbps=418ms (2026-04-17T01:10:03.001Z), send>=500kbps=418ms (2026-04-17T01:10:03.001Z), send>=700kbps=418ms (2026-04-17T01:10:03.001Z), send>=900kbps=418ms (2026-04-17T01:10:03.001Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=455ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=418ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:11:10.995Z；firstRecovering=-；firstStable=418ms (2026-04-17T01:11:11.413Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=918ms (2026-04-17T01:11:11.913Z), rtt<120ms=418ms (2026-04-17T01:11:11.413Z), jitter<28ms=418ms (2026-04-17T01:11:11.413Z), jitter<18ms=418ms (2026-04-17T01:11:11.413Z)；target=target>=120kbps=418ms (2026-04-17T01:11:11.413Z), target>=300kbps=418ms (2026-04-17T01:11:11.413Z), target>=500kbps=418ms (2026-04-17T01:11:11.413Z), target>=700kbps=418ms (2026-04-17T01:11:11.413Z), target>=900kbps=418ms (2026-04-17T01:11:11.413Z)；send=send>=300kbps=418ms (2026-04-17T01:11:11.413Z), send>=500kbps=418ms (2026-04-17T01:11:11.413Z), send>=700kbps=418ms (2026-04-17T01:11:11.413Z), send>=900kbps=418ms (2026-04-17T01:11:11.413Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=454ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=418ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:12:19.404Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=416ms (2026-04-17T01:12:19.820Z), rtt<120ms=416ms (2026-04-17T01:12:19.820Z), jitter<28ms=5416ms (2026-04-17T01:12:24.820Z), jitter<18ms=-；target=target>=120kbps=416ms (2026-04-17T01:12:19.820Z), target>=300kbps=416ms (2026-04-17T01:12:19.820Z), target>=500kbps=416ms (2026-04-17T01:12:19.820Z), target>=700kbps=416ms (2026-04-17T01:12:19.820Z), target>=900kbps=-；send=send>=300kbps=416ms (2026-04-17T01:12:19.820Z), send>=500kbps=416ms (2026-04-17T01:12:19.820Z), send>=700kbps=416ms (2026-04-17T01:12:19.820Z), send>=900kbps=1416ms (2026-04-17T01:12:20.820Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=455ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=416ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:13:27.810Z；firstRecovering=-；firstStable=416ms (2026-04-17T01:13:28.226Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=916ms (2026-04-17T01:13:28.726Z), rtt<120ms=416ms (2026-04-17T01:13:28.226Z), jitter<28ms=416ms (2026-04-17T01:13:28.226Z), jitter<18ms=416ms (2026-04-17T01:13:28.226Z)；target=target>=120kbps=416ms (2026-04-17T01:13:28.226Z), target>=300kbps=416ms (2026-04-17T01:13:28.226Z), target>=500kbps=416ms (2026-04-17T01:13:28.226Z), target>=700kbps=416ms (2026-04-17T01:13:28.226Z), target>=900kbps=416ms (2026-04-17T01:13:28.226Z)；send=send>=300kbps=416ms (2026-04-17T01:13:28.226Z), send>=500kbps=416ms (2026-04-17T01:13:28.226Z), send>=700kbps=416ms (2026-04-17T01:13:28.226Z), send>=900kbps=416ms (2026-04-17T01:13:28.226Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=454ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=416ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 44 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:14:36.222Z；firstRecovering=6419ms (2026-04-17T01:14:42.641Z)；firstStable=8919ms (2026-04-17T01:14:45.141Z)；firstL0=10419ms (2026-04-17T01:14:46.641Z) |
| 恢复诊断 | raw=loss<3%=419ms (2026-04-17T01:14:36.641Z), rtt<120ms=419ms (2026-04-17T01:14:36.641Z), jitter<28ms=5919ms (2026-04-17T01:14:42.141Z), jitter<18ms=6419ms (2026-04-17T01:14:42.641Z)；target=target>=120kbps=2919ms (2026-04-17T01:14:39.141Z), target>=300kbps=6919ms (2026-04-17T01:14:43.141Z), target>=500kbps=8439ms (2026-04-17T01:14:44.661Z), target>=700kbps=9419ms (2026-04-17T01:14:45.641Z), target>=900kbps=10919ms (2026-04-17T01:14:47.141Z)；send=send>=300kbps=6919ms (2026-04-17T01:14:43.141Z), send>=500kbps=8439ms (2026-04-17T01:14:44.661Z), send>=700kbps=9419ms (2026-04-17T01:14:45.641Z), send>=900kbps=9419ms (2026-04-17T01:14:45.641Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1454ms, recovering=9954ms, stable=455ms, congested=1954ms, firstAction=1454ms, L0=14454ms, L1=1454ms, L2=1954ms, L3=2454ms, L4=2955ms, audioOnly=-；recovery: warning=-, recovering=6419ms, stable=8919ms, congested=419ms, firstAction=419ms, L0=10419ms, L1=8919ms, L2=7919ms, L3=6419ms, L4=419ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L3)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 72 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:15:44.630Z；firstRecovering=9416ms (2026-04-17T01:15:54.046Z)；firstStable=11916ms (2026-04-17T01:15:56.546Z)；firstL0=13916ms (2026-04-17T01:15:58.546Z) |
| 恢复诊断 | raw=loss<3%=417ms (2026-04-17T01:15:45.047Z), rtt<120ms=1916ms (2026-04-17T01:15:46.546Z), jitter<28ms=8916ms (2026-04-17T01:15:53.546Z), jitter<18ms=9916ms (2026-04-17T01:15:54.546Z)；target=target>=120kbps=417ms (2026-04-17T01:15:45.047Z), target>=300kbps=417ms (2026-04-17T01:15:45.047Z), target>=500kbps=12916ms (2026-04-17T01:15:57.546Z), target>=700kbps=15417ms (2026-04-17T01:16:00.047Z), target>=900kbps=-；send=send>=300kbps=417ms (2026-04-17T01:15:45.047Z), send>=500kbps=1417ms (2026-04-17T01:15:46.047Z), send>=700kbps=14417ms (2026-04-17T01:15:59.047Z), send>=900kbps=28915ms (2026-04-17T01:16:13.545Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1457ms, recovering=11960ms, stable=457ms, congested=1957ms, firstAction=1457ms, L0=16457ms, L1=1457ms, L2=1957ms, L3=2457ms, L4=2957ms, audioOnly=-；recovery: warning=16416ms, recovering=9416ms, stable=11916ms, congested=417ms, firstAction=417ms, L0=13916ms, L1=12416ms, L2=10916ms, L3=9416ms, L4=417ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 60 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:16:53.031Z；firstRecovering=420ms (2026-04-17T01:16:53.451Z)；firstStable=2920ms (2026-04-17T01:16:55.951Z)；firstL0=3920ms (2026-04-17T01:16:56.951Z) |
| 恢复诊断 | raw=loss<3%=920ms (2026-04-17T01:16:53.951Z), rtt<120ms=420ms (2026-04-17T01:16:53.451Z), jitter<28ms=14920ms (2026-04-17T01:17:07.951Z), jitter<18ms=23420ms (2026-04-17T01:17:16.451Z)；target=target>=120kbps=420ms (2026-04-17T01:16:53.451Z), target>=300kbps=3920ms (2026-04-17T01:16:56.951Z), target>=500kbps=17421ms (2026-04-17T01:17:10.452Z), target>=700kbps=19420ms (2026-04-17T01:17:12.451Z), target>=900kbps=22420ms (2026-04-17T01:17:15.451Z)；send=send>=300kbps=420ms (2026-04-17T01:16:53.451Z), send>=500kbps=6421ms (2026-04-17T01:16:59.452Z), send>=700kbps=18920ms (2026-04-17T01:17:11.951Z), send>=900kbps=20920ms (2026-04-17T01:17:13.951Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2956ms, recovering=18956ms, stable=455ms, congested=3455ms, firstAction=2956ms, L0=-, L1=2956ms, L2=3455ms, L3=3955ms, L4=4455ms, audioOnly=-；recovery: warning=6421ms, recovering=420ms, stable=2920ms, congested=6920ms, firstAction=920ms, L0=3920ms, L1=2420ms, L2=920ms, L3=7421ms, L4=7920ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:18:01.416Z；firstRecovering=20440ms (2026-04-17T01:18:21.856Z)；firstStable=22941ms (2026-04-17T01:18:24.357Z)；firstL0=24441ms (2026-04-17T01:18:25.857Z) |
| 恢复诊断 | raw=loss<3%=440ms (2026-04-17T01:18:01.856Z), rtt<120ms=1941ms (2026-04-17T01:18:03.357Z), jitter<28ms=19940ms (2026-04-17T01:18:21.356Z), jitter<18ms=24441ms (2026-04-17T01:18:25.857Z)；target=target>=120kbps=15441ms (2026-04-17T01:18:16.857Z), target>=300kbps=20941ms (2026-04-17T01:18:22.357Z), target>=500kbps=22441ms (2026-04-17T01:18:23.857Z), target>=700kbps=23442ms (2026-04-17T01:18:24.858Z), target>=900kbps=24941ms (2026-04-17T01:18:26.357Z)；send=send>=300kbps=20941ms (2026-04-17T01:18:22.357Z), send>=500kbps=22441ms (2026-04-17T01:18:23.857Z), send>=700kbps=23442ms (2026-04-17T01:18:24.858Z), send>=900kbps=23442ms (2026-04-17T01:18:24.858Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3458ms, recovering=-, stable=458ms, congested=5458ms, firstAction=3458ms, L0=-, L1=3458ms, L2=5458ms, L3=5958ms, L4=6458ms, audioOnly=-；recovery: warning=-, recovering=20440ms, stable=22941ms, congested=440ms, firstAction=440ms, L0=24441ms, L1=22941ms, L2=21941ms, L3=20440ms, L4=440ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 69 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-17T01:19:09.814Z；firstRecovering=17939ms (2026-04-17T01:19:27.753Z)；firstStable=436ms (2026-04-17T01:19:10.250Z)；firstL0=22440ms (2026-04-17T01:19:32.254Z) |
| 恢复诊断 | raw=loss<3%=436ms (2026-04-17T01:19:10.250Z), rtt<120ms=436ms (2026-04-17T01:19:10.250Z), jitter<28ms=436ms (2026-04-17T01:19:10.250Z), jitter<18ms=436ms (2026-04-17T01:19:10.250Z)；target=target>=120kbps=12939ms (2026-04-17T01:19:22.753Z), target>=300kbps=19939ms (2026-04-17T01:19:29.753Z), target>=500kbps=19939ms (2026-04-17T01:19:29.753Z), target>=700kbps=21940ms (2026-04-17T01:19:31.754Z), target>=900kbps=23939ms (2026-04-17T01:19:33.753Z)；send=send>=300kbps=436ms (2026-04-17T01:19:10.250Z), send>=500kbps=436ms (2026-04-17T01:19:10.250Z), send>=700kbps=436ms (2026-04-17T01:19:10.250Z), send>=900kbps=936ms (2026-04-17T01:19:10.750Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4455ms, recovering=14455ms, stable=455ms, congested=6455ms, firstAction=4455ms, L0=18955ms, L1=4455ms, L2=6455ms, L3=6955ms, L4=7455ms, audioOnly=-；recovery: warning=1436ms, recovering=17939ms, stable=436ms, congested=1936ms, firstAction=1436ms, L0=22440ms, L1=1436ms, L2=1936ms, L3=2436ms, L4=2936ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:20:18.223Z；firstRecovering=-；firstStable=418ms (2026-04-17T01:20:18.641Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=418ms (2026-04-17T01:20:18.641Z), rtt<120ms=418ms (2026-04-17T01:20:18.641Z), jitter<28ms=418ms (2026-04-17T01:20:18.641Z), jitter<18ms=418ms (2026-04-17T01:20:18.641Z)；target=target>=120kbps=418ms (2026-04-17T01:20:18.641Z), target>=300kbps=418ms (2026-04-17T01:20:18.641Z), target>=500kbps=418ms (2026-04-17T01:20:18.641Z), target>=700kbps=418ms (2026-04-17T01:20:18.641Z), target>=900kbps=418ms (2026-04-17T01:20:18.641Z)；send=send>=300kbps=418ms (2026-04-17T01:20:18.641Z), send>=500kbps=418ms (2026-04-17T01:20:18.641Z), send>=700kbps=418ms (2026-04-17T01:20:18.641Z), send>=900kbps=418ms (2026-04-17T01:20:18.641Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=455ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=418ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:21:26.672Z；firstRecovering=-；firstStable=399ms (2026-04-17T01:21:27.071Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=899ms (2026-04-17T01:21:27.571Z), rtt<120ms=399ms (2026-04-17T01:21:27.071Z), jitter<28ms=399ms (2026-04-17T01:21:27.071Z), jitter<18ms=399ms (2026-04-17T01:21:27.071Z)；target=target>=120kbps=399ms (2026-04-17T01:21:27.071Z), target>=300kbps=399ms (2026-04-17T01:21:27.071Z), target>=500kbps=399ms (2026-04-17T01:21:27.071Z), target>=700kbps=399ms (2026-04-17T01:21:27.071Z), target>=900kbps=899ms (2026-04-17T01:21:27.571Z)；send=send>=300kbps=399ms (2026-04-17T01:21:27.071Z), send>=500kbps=399ms (2026-04-17T01:21:27.071Z), send>=700kbps=399ms (2026-04-17T01:21:27.071Z), send>=900kbps=399ms (2026-04-17T01:21:27.071Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=442ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=399ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:22:35.087Z；firstRecovering=-；firstStable=418ms (2026-04-17T01:22:35.505Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=918ms (2026-04-17T01:22:36.005Z), rtt<120ms=418ms (2026-04-17T01:22:35.505Z), jitter<28ms=418ms (2026-04-17T01:22:35.505Z), jitter<18ms=418ms (2026-04-17T01:22:35.505Z)；target=target>=120kbps=418ms (2026-04-17T01:22:35.505Z), target>=300kbps=418ms (2026-04-17T01:22:35.505Z), target>=500kbps=418ms (2026-04-17T01:22:35.505Z), target>=700kbps=418ms (2026-04-17T01:22:35.505Z), target>=900kbps=418ms (2026-04-17T01:22:35.505Z)；send=send>=300kbps=418ms (2026-04-17T01:22:35.505Z), send>=500kbps=418ms (2026-04-17T01:22:35.505Z), send>=700kbps=418ms (2026-04-17T01:22:35.505Z), send>=900kbps=418ms (2026-04-17T01:22:35.505Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=454ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=418ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:23:43.520Z；firstRecovering=-；firstStable=409ms (2026-04-17T01:23:43.929Z)；firstL0=409ms (2026-04-17T01:23:43.929Z) |
| 恢复诊断 | raw=loss<3%=409ms (2026-04-17T01:23:43.929Z), rtt<120ms=409ms (2026-04-17T01:23:43.929Z), jitter<28ms=409ms (2026-04-17T01:23:43.929Z), jitter<18ms=409ms (2026-04-17T01:23:43.929Z)；target=target>=120kbps=409ms (2026-04-17T01:23:43.929Z), target>=300kbps=409ms (2026-04-17T01:23:43.929Z), target>=500kbps=409ms (2026-04-17T01:23:43.929Z), target>=700kbps=409ms (2026-04-17T01:23:43.929Z), target>=900kbps=909ms (2026-04-17T01:23:44.429Z)；send=send>=300kbps=409ms (2026-04-17T01:23:43.929Z), send>=500kbps=409ms (2026-04-17T01:23:43.929Z), send>=700kbps=409ms (2026-04-17T01:23:43.929Z), send>=900kbps=409ms (2026-04-17T01:23:43.929Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=8448ms, recovering=-, stable=448ms, congested=-, firstAction=8448ms, L0=-, L1=8448ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=409ms, congested=-, firstAction=409ms, L0=409ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 47 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:24:51.979Z；firstRecovering=4422ms (2026-04-17T01:24:56.401Z)；firstStable=4922ms (2026-04-17T01:24:56.901Z)；firstL0=8922ms (2026-04-17T01:25:00.901Z) |
| 恢复诊断 | raw=loss<3%=924ms (2026-04-17T01:24:52.903Z), rtt<120ms=422ms (2026-04-17T01:24:52.401Z), jitter<28ms=422ms (2026-04-17T01:24:52.401Z), jitter<18ms=422ms (2026-04-17T01:24:52.401Z)；target=target>=120kbps=4922ms (2026-04-17T01:24:56.901Z), target>=300kbps=6422ms (2026-04-17T01:24:58.401Z), target>=500kbps=7922ms (2026-04-17T01:24:59.901Z), target>=700kbps=9422ms (2026-04-17T01:25:01.401Z), target>=900kbps=11422ms (2026-04-17T01:25:03.401Z)；send=send>=300kbps=4922ms (2026-04-17T01:24:56.901Z), send>=500kbps=4922ms (2026-04-17T01:24:56.901Z), send>=700kbps=4922ms (2026-04-17T01:24:56.901Z), send>=900kbps=5422ms (2026-04-17T01:24:57.401Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2957ms, recovering=-, stable=457ms, congested=4457ms, firstAction=2957ms, L0=-, L1=2957ms, L2=4457ms, L3=4957ms, L4=5457ms, audioOnly=-；recovery: warning=11422ms, recovering=4422ms, stable=4922ms, congested=422ms, firstAction=422ms, L0=8922ms, L1=7422ms, L2=5922ms, L3=4422ms, L4=422ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 53 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:26:00.375Z；firstRecovering=5929ms (2026-04-17T01:26:06.304Z)；firstStable=7929ms (2026-04-17T01:26:08.304Z)；firstL0=9929ms (2026-04-17T01:26:10.304Z) |
| 恢复诊断 | raw=loss<3%=429ms (2026-04-17T01:26:00.804Z), rtt<120ms=429ms (2026-04-17T01:26:00.804Z), jitter<28ms=2429ms (2026-04-17T01:26:02.804Z), jitter<18ms=3929ms (2026-04-17T01:26:04.304Z)；target=target>=120kbps=1929ms (2026-04-17T01:26:02.304Z), target>=300kbps=7929ms (2026-04-17T01:26:08.304Z), target>=500kbps=8929ms (2026-04-17T01:26:09.304Z), target>=700kbps=8929ms (2026-04-17T01:26:09.304Z), target>=900kbps=19428ms (2026-04-17T01:26:19.803Z)；send=send>=300kbps=6429ms (2026-04-17T01:26:06.804Z), send>=500kbps=6429ms (2026-04-17T01:26:06.804Z), send>=700kbps=6429ms (2026-04-17T01:26:06.804Z), send>=900kbps=7929ms (2026-04-17T01:26:08.304Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1944ms, recovering=-, stable=468ms, congested=2946ms, firstAction=1944ms, L0=-, L1=1944ms, L2=2946ms, L3=3447ms, L4=3947ms, audioOnly=-；recovery: warning=12429ms, recovering=5929ms, stable=7929ms, congested=429ms, firstAction=429ms, L0=9929ms, L1=8432ms, L2=7429ms, L3=5929ms, L4=429ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 80 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:27:08.795Z；firstRecovering=9415ms (2026-04-17T01:27:18.210Z)；firstStable=11915ms (2026-04-17T01:27:20.710Z)；firstL0=12915ms (2026-04-17T01:27:21.710Z) |
| 恢复诊断 | raw=loss<3%=415ms (2026-04-17T01:27:09.210Z), rtt<120ms=415ms (2026-04-17T01:27:09.210Z), jitter<28ms=8915ms (2026-04-17T01:27:17.710Z), jitter<18ms=15415ms (2026-04-17T01:27:24.210Z)；target=target>=120kbps=3915ms (2026-04-17T01:27:12.710Z), target>=300kbps=9915ms (2026-04-17T01:27:18.710Z), target>=500kbps=11415ms (2026-04-17T01:27:20.210Z), target>=700kbps=12915ms (2026-04-17T01:27:21.710Z), target>=900kbps=13915ms (2026-04-17T01:27:22.710Z)；send=send>=300kbps=2415ms (2026-04-17T01:27:11.210Z), send>=500kbps=9915ms (2026-04-17T01:27:18.710Z), send>=700kbps=11415ms (2026-04-17T01:27:20.210Z), send>=900kbps=11415ms (2026-04-17T01:27:20.210Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1455ms, recovering=-, stable=455ms, congested=2455ms, firstAction=1455ms, L0=-, L1=1455ms, L2=2455ms, L3=2955ms, L4=3455ms, audioOnly=-；recovery: warning=15415ms, recovering=9415ms, stable=11915ms, congested=415ms, firstAction=415ms, L0=12915ms, L1=11915ms, L2=10915ms, L3=9415ms, L4=415ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:28:17.193Z；firstRecovering=17933ms (2026-04-17T01:28:35.126Z)；firstStable=20433ms (2026-04-17T01:28:37.626Z)；firstL0=21433ms (2026-04-17T01:28:38.626Z) |
| 恢复诊断 | raw=loss<3%=933ms (2026-04-17T01:28:18.126Z), rtt<120ms=433ms (2026-04-17T01:28:17.626Z), jitter<28ms=17433ms (2026-04-17T01:28:34.626Z), jitter<18ms=17933ms (2026-04-17T01:28:35.126Z)；target=target>=120kbps=12933ms (2026-04-17T01:28:30.126Z), target>=300kbps=18433ms (2026-04-17T01:28:35.626Z), target>=500kbps=19933ms (2026-04-17T01:28:37.126Z), target>=700kbps=20933ms (2026-04-17T01:28:38.126Z), target>=900kbps=21933ms (2026-04-17T01:28:39.126Z)；send=send>=300kbps=18433ms (2026-04-17T01:28:35.626Z), send>=500kbps=19933ms (2026-04-17T01:28:37.126Z), send>=700kbps=19933ms (2026-04-17T01:28:37.126Z), send>=900kbps=20933ms (2026-04-17T01:28:38.126Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3451ms, recovering=-, stable=451ms, congested=3951ms, firstAction=3451ms, L0=-, L1=3451ms, L2=3951ms, L3=4451ms, L4=4951ms, audioOnly=-；recovery: warning=-, recovering=17933ms, stable=20433ms, congested=433ms, firstAction=433ms, L0=21433ms, L1=20433ms, L2=19433ms, L3=17933ms, L4=433ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:29:25.607Z；firstRecovering=-；firstStable=423ms (2026-04-17T01:29:26.030Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=423ms (2026-04-17T01:29:26.030Z), rtt<120ms=423ms (2026-04-17T01:29:26.030Z), jitter<28ms=423ms (2026-04-17T01:29:26.030Z), jitter<18ms=423ms (2026-04-17T01:29:26.030Z)；target=target>=120kbps=423ms (2026-04-17T01:29:26.030Z), target>=300kbps=423ms (2026-04-17T01:29:26.030Z), target>=500kbps=423ms (2026-04-17T01:29:26.030Z), target>=700kbps=423ms (2026-04-17T01:29:26.030Z), target>=900kbps=423ms (2026-04-17T01:29:26.030Z)；send=send>=300kbps=423ms (2026-04-17T01:29:26.030Z), send>=500kbps=423ms (2026-04-17T01:29:26.030Z), send>=700kbps=423ms (2026-04-17T01:29:26.030Z), send>=900kbps=423ms (2026-04-17T01:29:26.030Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=453ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=423ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:30:34.018Z；firstRecovering=-；firstStable=416ms (2026-04-17T01:30:34.434Z)；firstL0=17416ms (2026-04-17T01:30:51.434Z) |
| 恢复诊断 | raw=loss<3%=416ms (2026-04-17T01:30:34.434Z), rtt<120ms=416ms (2026-04-17T01:30:34.434Z), jitter<28ms=416ms (2026-04-17T01:30:34.434Z), jitter<18ms=416ms (2026-04-17T01:30:34.434Z)；target=target>=120kbps=416ms (2026-04-17T01:30:34.434Z), target>=300kbps=416ms (2026-04-17T01:30:34.434Z), target>=500kbps=416ms (2026-04-17T01:30:34.434Z), target>=700kbps=416ms (2026-04-17T01:30:34.434Z), target>=900kbps=416ms (2026-04-17T01:30:34.434Z)；send=send>=300kbps=416ms (2026-04-17T01:30:34.434Z), send>=500kbps=416ms (2026-04-17T01:30:34.434Z), send>=700kbps=416ms (2026-04-17T01:30:34.434Z), send>=900kbps=416ms (2026-04-17T01:30:34.434Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2454ms, recovering=-, stable=454ms, congested=-, firstAction=2454ms, L0=16454ms, L1=2454ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=3416ms, recovering=-, stable=416ms, congested=-, firstAction=3416ms, L0=17416ms, L1=3416ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:31:42.465Z；firstRecovering=-；firstStable=415ms (2026-04-17T01:31:42.880Z)；firstL0=15915ms (2026-04-17T01:31:58.380Z) |
| 恢复诊断 | raw=loss<3%=415ms (2026-04-17T01:31:42.880Z), rtt<120ms=1915ms (2026-04-17T01:31:44.380Z), jitter<28ms=415ms (2026-04-17T01:31:42.880Z), jitter<18ms=415ms (2026-04-17T01:31:42.880Z)；target=target>=120kbps=415ms (2026-04-17T01:31:42.880Z), target>=300kbps=415ms (2026-04-17T01:31:42.880Z), target>=500kbps=415ms (2026-04-17T01:31:42.880Z), target>=700kbps=415ms (2026-04-17T01:31:42.880Z), target>=900kbps=16415ms (2026-04-17T01:31:58.880Z)；send=send>=300kbps=415ms (2026-04-17T01:31:42.880Z), send>=500kbps=415ms (2026-04-17T01:31:42.880Z), send>=700kbps=415ms (2026-04-17T01:31:42.880Z), send>=900kbps=415ms (2026-04-17T01:31:42.880Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=452ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=2915ms, recovering=-, stable=415ms, congested=-, firstAction=2915ms, L0=15915ms, L1=2915ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:32:50.882Z；firstRecovering=-；firstStable=14922ms (2026-04-17T01:33:05.804Z)；firstL0=14922ms (2026-04-17T01:33:05.804Z) |
| 恢复诊断 | raw=loss<3%=922ms (2026-04-17T01:32:51.804Z), rtt<120ms=1922ms (2026-04-17T01:32:52.804Z), jitter<28ms=423ms (2026-04-17T01:32:51.305Z), jitter<18ms=12922ms (2026-04-17T01:33:03.804Z)；target=target>=120kbps=423ms (2026-04-17T01:32:51.305Z), target>=300kbps=423ms (2026-04-17T01:32:51.305Z), target>=500kbps=423ms (2026-04-17T01:32:51.305Z), target>=700kbps=6422ms (2026-04-17T01:32:57.304Z), target>=900kbps=15422ms (2026-04-17T01:33:06.304Z)；send=send>=300kbps=423ms (2026-04-17T01:32:51.305Z), send>=500kbps=423ms (2026-04-17T01:32:51.305Z), send>=700kbps=423ms (2026-04-17T01:32:51.305Z), send>=900kbps=423ms (2026-04-17T01:32:51.305Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1960ms, recovering=-, stable=460ms, congested=-, firstAction=1960ms, L0=-, L1=1960ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=423ms, recovering=-, stable=14922ms, congested=-, firstAction=14922ms, L0=14922ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:33:59.311Z；firstRecovering=4899ms (2026-04-17T01:34:04.210Z)；firstStable=5399ms (2026-04-17T01:34:04.710Z)；firstL0=9399ms (2026-04-17T01:34:08.710Z) |
| 恢复诊断 | raw=loss<3%=399ms (2026-04-17T01:33:59.710Z), rtt<120ms=1899ms (2026-04-17T01:34:01.210Z), jitter<28ms=399ms (2026-04-17T01:33:59.710Z), jitter<18ms=399ms (2026-04-17T01:33:59.710Z)；target=target>=120kbps=399ms (2026-04-17T01:33:59.710Z), target>=300kbps=5399ms (2026-04-17T01:34:04.710Z), target>=500kbps=6899ms (2026-04-17T01:34:06.210Z), target>=700kbps=8899ms (2026-04-17T01:34:08.210Z), target>=900kbps=11399ms (2026-04-17T01:34:10.710Z)；send=send>=300kbps=5399ms (2026-04-17T01:34:04.710Z), send>=500kbps=5399ms (2026-04-17T01:34:04.710Z), send>=700kbps=5399ms (2026-04-17T01:34:04.710Z), send>=900kbps=6899ms (2026-04-17T01:34:06.210Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1443ms, recovering=-, stable=443ms, congested=3943ms, firstAction=1443ms, L0=-, L1=1443ms, L2=3943ms, L3=4443ms, L4=4943ms, audioOnly=-；recovery: warning=11899ms, recovering=4899ms, stable=5399ms, congested=399ms, firstAction=399ms, L0=9399ms, L1=7899ms, L2=6399ms, L3=4899ms, L4=399ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:35:07.718Z；firstRecovering=5415ms (2026-04-17T01:35:13.133Z)；firstStable=5916ms (2026-04-17T01:35:13.634Z)；firstL0=9915ms (2026-04-17T01:35:17.633Z) |
| 恢复诊断 | raw=loss<3%=415ms (2026-04-17T01:35:08.133Z), rtt<120ms=1915ms (2026-04-17T01:35:09.633Z), jitter<28ms=415ms (2026-04-17T01:35:08.133Z), jitter<18ms=415ms (2026-04-17T01:35:08.133Z)；target=target>=120kbps=415ms (2026-04-17T01:35:08.133Z), target>=300kbps=7415ms (2026-04-17T01:35:15.133Z), target>=500kbps=8915ms (2026-04-17T01:35:16.633Z), target>=700kbps=12915ms (2026-04-17T01:35:20.633Z), target>=900kbps=25917ms (2026-04-17T01:35:33.635Z)；send=send>=300kbps=5916ms (2026-04-17T01:35:13.634Z), send>=500kbps=5916ms (2026-04-17T01:35:13.634Z), send>=700kbps=5916ms (2026-04-17T01:35:13.634Z), send>=900kbps=7415ms (2026-04-17T01:35:15.133Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1453ms, recovering=-, stable=453ms, congested=1953ms, firstAction=1453ms, L0=-, L1=1453ms, L2=1953ms, L3=2453ms, L4=2953ms, audioOnly=-；recovery: warning=12415ms, recovering=5415ms, stable=5916ms, congested=415ms, firstAction=415ms, L0=9915ms, L1=8416ms, L2=6915ms, L3=5415ms, L4=415ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:36:16.148Z；firstRecovering=-；firstStable=416ms (2026-04-17T01:36:16.564Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=416ms (2026-04-17T01:36:16.564Z), rtt<120ms=416ms (2026-04-17T01:36:16.564Z), jitter<28ms=416ms (2026-04-17T01:36:16.564Z), jitter<18ms=416ms (2026-04-17T01:36:16.564Z)；target=target>=120kbps=416ms (2026-04-17T01:36:16.564Z), target>=300kbps=416ms (2026-04-17T01:36:16.564Z), target>=500kbps=416ms (2026-04-17T01:36:16.564Z), target>=700kbps=416ms (2026-04-17T01:36:16.564Z), target>=900kbps=416ms (2026-04-17T01:36:16.564Z)；send=send>=300kbps=416ms (2026-04-17T01:36:16.564Z), send>=500kbps=416ms (2026-04-17T01:36:16.564Z), send>=700kbps=416ms (2026-04-17T01:36:16.564Z), send>=900kbps=416ms (2026-04-17T01:36:16.564Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=458ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=416ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:37:24.583Z；firstRecovering=-；firstStable=412ms (2026-04-17T01:37:24.995Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=912ms (2026-04-17T01:37:25.495Z), rtt<120ms=412ms (2026-04-17T01:37:24.995Z), jitter<28ms=412ms (2026-04-17T01:37:24.995Z), jitter<18ms=1912ms (2026-04-17T01:37:26.495Z)；target=target>=120kbps=412ms (2026-04-17T01:37:24.995Z), target>=300kbps=412ms (2026-04-17T01:37:24.995Z), target>=500kbps=412ms (2026-04-17T01:37:24.995Z), target>=700kbps=412ms (2026-04-17T01:37:24.995Z), target>=900kbps=412ms (2026-04-17T01:37:24.995Z)；send=send>=300kbps=412ms (2026-04-17T01:37:24.995Z), send>=500kbps=412ms (2026-04-17T01:37:24.995Z), send>=700kbps=412ms (2026-04-17T01:37:24.995Z), send>=900kbps=412ms (2026-04-17T01:37:24.995Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=444ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=412ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:38:32.998Z；firstRecovering=-；firstStable=13419ms (2026-04-17T01:38:46.417Z)；firstL0=13419ms (2026-04-17T01:38:46.417Z) |
| 恢复诊断 | raw=loss<3%=419ms (2026-04-17T01:38:33.417Z), rtt<120ms=1919ms (2026-04-17T01:38:34.917Z), jitter<28ms=919ms (2026-04-17T01:38:33.917Z), jitter<18ms=1919ms (2026-04-17T01:38:34.917Z)；target=target>=120kbps=419ms (2026-04-17T01:38:33.417Z), target>=300kbps=419ms (2026-04-17T01:38:33.417Z), target>=500kbps=419ms (2026-04-17T01:38:33.417Z), target>=700kbps=419ms (2026-04-17T01:38:33.417Z), target>=900kbps=13919ms (2026-04-17T01:38:46.917Z)；send=send>=300kbps=419ms (2026-04-17T01:38:33.417Z), send>=500kbps=419ms (2026-04-17T01:38:33.417Z), send>=700kbps=419ms (2026-04-17T01:38:33.417Z), send>=900kbps=4419ms (2026-04-17T01:38:37.417Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5454ms, recovering=-, stable=454ms, congested=-, firstAction=5454ms, L0=-, L1=5454ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=419ms, recovering=-, stable=13419ms, congested=-, firstAction=13419ms, L0=13419ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 56 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:39:41.418Z；firstRecovering=2914ms (2026-04-17T01:39:44.332Z)；firstStable=5414ms (2026-04-17T01:39:46.832Z)；firstL0=6913ms (2026-04-17T01:39:48.331Z) |
| 恢复诊断 | raw=loss<3%=422ms (2026-04-17T01:39:41.840Z), rtt<120ms=1914ms (2026-04-17T01:39:43.332Z), jitter<28ms=1413ms (2026-04-17T01:39:42.831Z), jitter<18ms=15413ms (2026-04-17T01:39:56.831Z)；target=target>=120kbps=422ms (2026-04-17T01:39:41.840Z), target>=300kbps=3414ms (2026-04-17T01:39:44.832Z), target>=500kbps=4913ms (2026-04-17T01:39:46.331Z), target>=700kbps=5914ms (2026-04-17T01:39:47.332Z), target>=900kbps=9413ms (2026-04-17T01:39:50.831Z)；send=send>=300kbps=3414ms (2026-04-17T01:39:44.832Z), send>=500kbps=3414ms (2026-04-17T01:39:44.832Z), send>=700kbps=3414ms (2026-04-17T01:39:44.832Z), send>=900kbps=4913ms (2026-04-17T01:39:46.331Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1956ms, recovering=-, stable=456ms, congested=7956ms, firstAction=1956ms, L0=-, L1=1956ms, L2=7956ms, L3=8456ms, L4=8957ms, audioOnly=-；recovery: warning=9413ms, recovering=2914ms, stable=5414ms, congested=422ms, firstAction=422ms, L0=6913ms, L1=5414ms, L2=4414ms, L3=2914ms, L4=422ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 68 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:40:49.842Z；firstRecovering=3915ms (2026-04-17T01:40:53.757Z)；firstStable=6415ms (2026-04-17T01:40:56.257Z)；firstL0=8415ms (2026-04-17T01:40:58.257Z) |
| 恢复诊断 | raw=loss<3%=415ms (2026-04-17T01:40:50.257Z), rtt<120ms=415ms (2026-04-17T01:40:50.257Z), jitter<28ms=2915ms (2026-04-17T01:40:52.757Z), jitter<18ms=19415ms (2026-04-17T01:41:09.257Z)；target=target>=120kbps=415ms (2026-04-17T01:40:50.257Z), target>=300kbps=4415ms (2026-04-17T01:40:54.257Z), target>=500kbps=7415ms (2026-04-17T01:40:57.257Z), target>=700kbps=22415ms (2026-04-17T01:41:12.257Z), target>=900kbps=23415ms (2026-04-17T01:41:13.257Z)；send=send>=300kbps=4415ms (2026-04-17T01:40:54.257Z), send>=500kbps=4415ms (2026-04-17T01:40:54.257Z), send>=700kbps=4415ms (2026-04-17T01:40:54.257Z), send>=900kbps=6415ms (2026-04-17T01:40:56.257Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1952ms, recovering=-, stable=452ms, congested=2952ms, firstAction=1952ms, L0=-, L1=1952ms, L2=2952ms, L3=3452ms, L4=3952ms, audioOnly=-；recovery: warning=10915ms, recovering=3915ms, stable=6415ms, congested=415ms, firstAction=415ms, L0=8415ms, L1=6915ms, L2=5415ms, L3=3915ms, L4=415ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:41:58.272Z；firstRecovering=-；firstStable=3918ms (2026-04-17T01:42:02.190Z)；firstL0=3918ms (2026-04-17T01:42:02.190Z) |
| 恢复诊断 | raw=loss<3%=418ms (2026-04-17T01:41:58.690Z), rtt<120ms=418ms (2026-04-17T01:41:58.690Z), jitter<28ms=418ms (2026-04-17T01:41:58.690Z), jitter<18ms=918ms (2026-04-17T01:41:59.190Z)；target=target>=120kbps=418ms (2026-04-17T01:41:58.690Z), target>=300kbps=418ms (2026-04-17T01:41:58.690Z), target>=500kbps=418ms (2026-04-17T01:41:58.690Z), target>=700kbps=418ms (2026-04-17T01:41:58.690Z), target>=900kbps=5418ms (2026-04-17T01:42:03.690Z)；send=send>=300kbps=418ms (2026-04-17T01:41:58.690Z), send>=500kbps=418ms (2026-04-17T01:41:58.690Z), send>=700kbps=918ms (2026-04-17T01:41:59.190Z), send>=900kbps=4418ms (2026-04-17T01:42:02.690Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=6455ms, recovering=-, stable=455ms, congested=-, firstAction=6455ms, L0=-, L1=6455ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=418ms, recovering=-, stable=3918ms, congested=-, firstAction=3918ms, L0=3918ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 42 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:43:06.681Z；firstRecovering=8428ms (2026-04-17T01:43:15.109Z)；firstStable=10916ms (2026-04-17T01:43:17.597Z)；firstL0=12918ms (2026-04-17T01:43:19.599Z) |
| 恢复诊断 | raw=loss<3%=416ms (2026-04-17T01:43:07.097Z), rtt<120ms=1916ms (2026-04-17T01:43:08.597Z), jitter<28ms=7916ms (2026-04-17T01:43:14.597Z), jitter<18ms=14419ms (2026-04-17T01:43:21.100Z)；target=target>=120kbps=416ms (2026-04-17T01:43:07.097Z), target>=300kbps=416ms (2026-04-17T01:43:07.097Z), target>=500kbps=10916ms (2026-04-17T01:43:17.597Z), target>=700kbps=11916ms (2026-04-17T01:43:18.597Z), target>=900kbps=13418ms (2026-04-17T01:43:20.099Z)；send=send>=300kbps=416ms (2026-04-17T01:43:07.097Z), send>=500kbps=416ms (2026-04-17T01:43:07.097Z), send>=700kbps=11416ms (2026-04-17T01:43:18.097Z), send>=900kbps=11916ms (2026-04-17T01:43:18.597Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2457ms, recovering=13457ms, stable=457ms, congested=4957ms, firstAction=2457ms, L0=17957ms, L1=2457ms, L2=4957ms, L3=5457ms, L4=5958ms, audioOnly=-；recovery: warning=15416ms, recovering=8428ms, stable=10916ms, congested=416ms, firstAction=2416ms, L0=12918ms, L1=2416ms, L2=2917ms, L3=3417ms, L4=3916ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 84 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-17T01:44:15.082Z；firstRecovering=17428ms (2026-04-17T01:44:32.510Z)；firstStable=19928ms (2026-04-17T01:44:35.010Z)；firstL0=21928ms (2026-04-17T01:44:37.010Z) |
| 恢复诊断 | raw=loss<3%=928ms (2026-04-17T01:44:16.010Z), rtt<120ms=428ms (2026-04-17T01:44:15.510Z), jitter<28ms=16928ms (2026-04-17T01:44:32.010Z), jitter<18ms=23928ms (2026-04-17T01:44:39.010Z)；target=target>=120kbps=428ms (2026-04-17T01:44:15.510Z), target>=300kbps=19428ms (2026-04-17T01:44:34.510Z), target>=500kbps=19428ms (2026-04-17T01:44:34.510Z), target>=700kbps=20928ms (2026-04-17T01:44:36.010Z), target>=900kbps=22428ms (2026-04-17T01:44:37.510Z)；send=send>=300kbps=17928ms (2026-04-17T01:44:33.010Z), send>=500kbps=19428ms (2026-04-17T01:44:34.510Z), send>=700kbps=19928ms (2026-04-17T01:44:35.010Z), send>=900kbps=19928ms (2026-04-17T01:44:35.010Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3456ms, recovering=-, stable=456ms, congested=3956ms, firstAction=3456ms, L0=-, L1=3456ms, L2=3956ms, L3=4456ms, L4=4957ms, audioOnly=-；recovery: warning=24428ms, recovering=17428ms, stable=19928ms, congested=428ms, firstAction=428ms, L0=21928ms, L1=20427ms, L2=18928ms, L3=17428ms, L4=428ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:45:23.503Z；firstRecovering=-；firstStable=409ms (2026-04-17T01:45:23.912Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=409ms (2026-04-17T01:45:23.912Z), rtt<120ms=409ms (2026-04-17T01:45:23.912Z), jitter<28ms=409ms (2026-04-17T01:45:23.912Z), jitter<18ms=1409ms (2026-04-17T01:45:24.912Z)；target=target>=120kbps=409ms (2026-04-17T01:45:23.912Z), target>=300kbps=409ms (2026-04-17T01:45:23.912Z), target>=500kbps=409ms (2026-04-17T01:45:23.912Z), target>=700kbps=409ms (2026-04-17T01:45:23.912Z), target>=900kbps=409ms (2026-04-17T01:45:23.912Z)；send=send>=300kbps=409ms (2026-04-17T01:45:23.912Z), send>=500kbps=409ms (2026-04-17T01:45:23.912Z), send>=700kbps=409ms (2026-04-17T01:45:23.912Z), send>=900kbps=409ms (2026-04-17T01:45:23.912Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2449ms, recovering=-, stable=450ms, congested=-, firstAction=2449ms, L0=19450ms, L1=2449ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=409ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 60 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:46:31.915Z；firstRecovering=10914ms (2026-04-17T01:46:42.829Z)；firstStable=13414ms (2026-04-17T01:46:45.329Z)；firstL0=14414ms (2026-04-17T01:46:46.329Z) |
| 恢复诊断 | raw=loss<3%=914ms (2026-04-17T01:46:32.829Z), rtt<120ms=414ms (2026-04-17T01:46:32.329Z), jitter<28ms=10414ms (2026-04-17T01:46:42.329Z), jitter<18ms=13414ms (2026-04-17T01:46:45.329Z)；target=target>=120kbps=11414ms (2026-04-17T01:46:43.329Z), target>=300kbps=11914ms (2026-04-17T01:46:43.829Z), target>=500kbps=12914ms (2026-04-17T01:46:44.829Z), target>=700kbps=13914ms (2026-04-17T01:46:45.829Z), target>=900kbps=14914ms (2026-04-17T01:46:46.829Z)；send=send>=300kbps=11414ms (2026-04-17T01:46:43.329Z), send>=500kbps=12914ms (2026-04-17T01:46:44.829Z), send>=700kbps=12914ms (2026-04-17T01:46:44.829Z), send>=900kbps=13914ms (2026-04-17T01:46:45.829Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1951ms, recovering=-, stable=451ms, congested=3451ms, firstAction=1951ms, L0=-, L1=1951ms, L2=3451ms, L3=3951ms, L4=4451ms, audioOnly=-；recovery: warning=-, recovering=10914ms, stable=13414ms, congested=414ms, firstAction=414ms, L0=14414ms, L1=13414ms, L2=12414ms, L3=10914ms, L4=414ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:47:40.358Z；firstRecovering=-；firstStable=16901ms (2026-04-17T01:47:57.259Z)；firstL0=16901ms (2026-04-17T01:47:57.259Z) |
| 恢复诊断 | raw=loss<3%=401ms (2026-04-17T01:47:40.759Z), rtt<120ms=1902ms (2026-04-17T01:47:42.260Z), jitter<28ms=401ms (2026-04-17T01:47:40.759Z), jitter<18ms=401ms (2026-04-17T01:47:40.759Z)；target=target>=120kbps=401ms (2026-04-17T01:47:40.759Z), target>=300kbps=401ms (2026-04-17T01:47:40.759Z), target>=500kbps=401ms (2026-04-17T01:47:40.759Z), target>=700kbps=401ms (2026-04-17T01:47:40.759Z), target>=900kbps=17401ms (2026-04-17T01:47:57.759Z)；send=send>=300kbps=401ms (2026-04-17T01:47:40.759Z), send>=500kbps=401ms (2026-04-17T01:47:40.759Z), send>=700kbps=401ms (2026-04-17T01:47:40.759Z), send>=900kbps=1902ms (2026-04-17T01:47:42.260Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2951ms, recovering=-, stable=451ms, congested=-, firstAction=2951ms, L0=-, L1=2951ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=401ms, recovering=-, stable=16901ms, congested=-, firstAction=16901ms, L0=16901ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:48:48.775Z；firstRecovering=-；firstStable=14913ms (2026-04-17T01:49:03.688Z)；firstL0=14913ms (2026-04-17T01:49:03.688Z) |
| 恢复诊断 | raw=loss<3%=413ms (2026-04-17T01:48:49.188Z), rtt<120ms=1913ms (2026-04-17T01:48:50.688Z), jitter<28ms=413ms (2026-04-17T01:48:49.188Z), jitter<18ms=7413ms (2026-04-17T01:48:56.188Z)；target=target>=120kbps=413ms (2026-04-17T01:48:49.188Z), target>=300kbps=413ms (2026-04-17T01:48:49.188Z), target>=500kbps=413ms (2026-04-17T01:48:49.188Z), target>=700kbps=413ms (2026-04-17T01:48:49.188Z), target>=900kbps=15413ms (2026-04-17T01:49:04.188Z)；send=send>=300kbps=413ms (2026-04-17T01:48:49.188Z), send>=500kbps=413ms (2026-04-17T01:48:49.188Z), send>=700kbps=413ms (2026-04-17T01:48:49.188Z), send>=900kbps=913ms (2026-04-17T01:48:49.688Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5453ms, recovering=-, stable=453ms, congested=-, firstAction=5453ms, L0=-, L1=5453ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=413ms, recovering=-, stable=14913ms, congested=-, firstAction=14913ms, L0=14913ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:49:57.182Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=457ms, recovering=-, stable=-, congested=1457ms, firstAction=1457ms, L0=-, L1=-, L2=1457ms, L3=1957ms, L4=2457ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 229 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:51:55.579Z；firstRecovering=18385ms (2026-04-17T01:52:13.964Z)；firstStable=20885ms (2026-04-17T01:52:16.464Z)；firstL0=22885ms (2026-04-17T01:52:18.464Z) |
| 恢复诊断 | raw=loss<3%=885ms (2026-04-17T01:51:56.464Z), rtt<120ms=1385ms (2026-04-17T01:51:56.964Z), jitter<28ms=17885ms (2026-04-17T01:52:13.464Z), jitter<18ms=17885ms (2026-04-17T01:52:13.464Z)；target=target>=120kbps=14885ms (2026-04-17T01:52:10.464Z), target>=300kbps=18885ms (2026-04-17T01:52:14.464Z), target>=500kbps=20385ms (2026-04-17T01:52:15.964Z), target>=700kbps=21885ms (2026-04-17T01:52:17.464Z), target>=900kbps=23384ms (2026-04-17T01:52:18.963Z)；send=send>=300kbps=19385ms (2026-04-17T01:52:14.964Z), send>=500kbps=20385ms (2026-04-17T01:52:15.964Z), send>=700kbps=21885ms (2026-04-17T01:52:17.464Z), send>=900kbps=21885ms (2026-04-17T01:52:17.464Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2948ms, recovering=19448ms, stable=2448ms, congested=4448ms, firstAction=2948ms, L0=-, L1=2948ms, L2=4448ms, L3=4948ms, L4=5448ms, audioOnly=-；recovery: warning=-, recovering=18385ms, stable=20885ms, congested=385ms, firstAction=385ms, L0=22885ms, L1=21385ms, L2=19885ms, L3=18385ms, L4=385ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 234 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:54:24.023Z；firstRecovering=17898ms (2026-04-17T01:54:41.921Z)；firstStable=20398ms (2026-04-17T01:54:44.421Z)；firstL0=22398ms (2026-04-17T01:54:46.421Z) |
| 恢复诊断 | raw=loss<3%=398ms (2026-04-17T01:54:24.421Z), rtt<120ms=1398ms (2026-04-17T01:54:25.421Z), jitter<28ms=17398ms (2026-04-17T01:54:41.421Z), jitter<18ms=25898ms (2026-04-17T01:54:49.921Z)；target=target>=120kbps=11898ms (2026-04-17T01:54:35.921Z), target>=300kbps=18403ms (2026-04-17T01:54:42.426Z), target>=500kbps=19898ms (2026-04-17T01:54:43.921Z), target>=700kbps=23898ms (2026-04-17T01:54:47.921Z), target>=900kbps=28900ms (2026-04-17T01:54:52.923Z)；send=send>=300kbps=18898ms (2026-04-17T01:54:42.921Z), send>=500kbps=19898ms (2026-04-17T01:54:43.921Z), send>=700kbps=21398ms (2026-04-17T01:54:45.421Z), send>=900kbps=29399ms (2026-04-17T01:54:53.422Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2460ms, recovering=-, stable=-, congested=3960ms, firstAction=3960ms, L0=-, L1=-, L2=3960ms, L3=4460ms, L4=4960ms, audioOnly=-；recovery: warning=24898ms, recovering=17898ms, stable=20398ms, congested=398ms, firstAction=398ms, L0=22398ms, L1=20898ms, L2=19398ms, L3=17898ms, L4=398ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 216 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:56:52.486Z；firstRecovering=17383ms (2026-04-17T01:57:09.869Z)；firstStable=19883ms (2026-04-17T01:57:12.369Z)；firstL0=21883ms (2026-04-17T01:57:14.369Z) |
| 恢复诊断 | raw=loss<3%=883ms (2026-04-17T01:56:53.369Z), rtt<120ms=1383ms (2026-04-17T01:56:53.869Z), jitter<28ms=16883ms (2026-04-17T01:57:09.369Z), jitter<18ms=22383ms (2026-04-17T01:57:14.869Z)；target=target>=120kbps=11883ms (2026-04-17T01:57:04.369Z), target>=300kbps=17883ms (2026-04-17T01:57:10.369Z), target>=500kbps=19383ms (2026-04-17T01:57:11.869Z), target>=700kbps=20883ms (2026-04-17T01:57:13.369Z), target>=900kbps=22383ms (2026-04-17T01:57:14.869Z)；send=send>=300kbps=18383ms (2026-04-17T01:57:10.869Z), send>=500kbps=19383ms (2026-04-17T01:57:11.869Z), send>=700kbps=20883ms (2026-04-17T01:57:13.369Z), send>=900kbps=20883ms (2026-04-17T01:57:13.369Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4449ms, recovering=20449ms, stable=2449ms, congested=12449ms, firstAction=4449ms, L0=5949ms, L1=4449ms, L2=12449ms, L3=12949ms, L4=13449ms, audioOnly=-；recovery: warning=-, recovering=17383ms, stable=19883ms, congested=383ms, firstAction=383ms, L0=21883ms, L1=20383ms, L2=18883ms, L3=17383ms, L4=383ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 21 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:57:45.882Z；firstRecovering=8434ms (2026-04-17T01:57:54.316Z)；firstStable=8934ms (2026-04-17T01:57:54.816Z)；firstL0=12934ms (2026-04-17T01:57:58.816Z) |
| 恢复诊断 | raw=loss<3%=934ms (2026-04-17T01:57:46.816Z), rtt<120ms=434ms (2026-04-17T01:57:46.316Z), jitter<28ms=434ms (2026-04-17T01:57:46.316Z), jitter<18ms=3434ms (2026-04-17T01:57:49.316Z)；target=target>=120kbps=434ms (2026-04-17T01:57:46.316Z), target>=300kbps=434ms (2026-04-17T01:57:46.316Z), target>=500kbps=434ms (2026-04-17T01:57:46.316Z), target>=700kbps=11934ms (2026-04-17T01:57:57.816Z), target>=900kbps=13434ms (2026-04-17T01:57:59.316Z)；send=send>=300kbps=434ms (2026-04-17T01:57:46.316Z), send>=500kbps=434ms (2026-04-17T01:57:46.316Z), send>=700kbps=434ms (2026-04-17T01:57:46.316Z), send>=900kbps=434ms (2026-04-17T01:57:46.316Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1953ms, recovering=-, stable=453ms, congested=-, firstAction=1953ms, L0=-, L1=1953ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=8434ms, stable=8934ms, congested=434ms, firstAction=434ms, L0=12934ms, L1=11434ms, L2=434ms, L3=934ms, L4=1434ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T01:58:42.263Z；firstRecovering=16429ms (2026-04-17T01:58:58.692Z)；firstStable=18930ms (2026-04-17T01:59:01.193Z)；firstL0=20930ms (2026-04-17T01:59:03.193Z) |
| 恢复诊断 | raw=loss<3%=433ms (2026-04-17T01:58:42.696Z), rtt<120ms=1430ms (2026-04-17T01:58:43.693Z), jitter<28ms=433ms (2026-04-17T01:58:42.696Z), jitter<18ms=15930ms (2026-04-17T01:58:58.193Z)；target=target>=120kbps=9430ms (2026-04-17T01:58:51.693Z), target>=300kbps=16930ms (2026-04-17T01:58:59.193Z), target>=500kbps=18429ms (2026-04-17T01:59:00.692Z), target>=700kbps=19930ms (2026-04-17T01:59:02.193Z), target>=900kbps=21431ms (2026-04-17T01:59:03.694Z)；send=send>=300kbps=16930ms (2026-04-17T01:58:59.193Z), send>=500kbps=18429ms (2026-04-17T01:59:00.692Z), send>=700kbps=18429ms (2026-04-17T01:59:00.692Z), send>=900kbps=19930ms (2026-04-17T01:59:02.193Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1449ms, recovering=-, stable=450ms, congested=1949ms, firstAction=1449ms, L0=-, L1=1449ms, L2=1949ms, L3=2449ms, L4=2950ms, audioOnly=-；recovery: warning=-, recovering=16429ms, stable=18930ms, congested=433ms, firstAction=433ms, L0=20930ms, L1=19429ms, L2=17930ms, L3=16429ms, L4=433ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-17T01:59:37.652Z；firstRecovering=-；firstStable=14933ms (2026-04-17T01:59:52.585Z)；firstL0=14933ms (2026-04-17T01:59:52.585Z) |
| 恢复诊断 | raw=loss<3%=433ms (2026-04-17T01:59:38.085Z), rtt<120ms=933ms (2026-04-17T01:59:38.585Z), jitter<28ms=4933ms (2026-04-17T01:59:42.585Z), jitter<18ms=11933ms (2026-04-17T01:59:49.585Z)；target=target>=120kbps=433ms (2026-04-17T01:59:38.085Z), target>=300kbps=433ms (2026-04-17T01:59:38.085Z), target>=500kbps=433ms (2026-04-17T01:59:38.085Z), target>=700kbps=5933ms (2026-04-17T01:59:43.585Z), target>=900kbps=15433ms (2026-04-17T01:59:53.085Z)；send=send>=300kbps=433ms (2026-04-17T01:59:38.085Z), send>=500kbps=433ms (2026-04-17T01:59:38.085Z), send>=700kbps=433ms (2026-04-17T01:59:38.085Z), send>=900kbps=8933ms (2026-04-17T01:59:46.585Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1956ms, recovering=-, stable=456ms, congested=-, firstAction=1956ms, L0=-, L1=1956ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=433ms, recovering=-, stable=14933ms, congested=-, firstAction=14933ms, L0=14933ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-17T02:00:31.065Z；firstRecovering=-；firstStable=12428ms (2026-04-17T02:00:43.493Z)；firstL0=12428ms (2026-04-17T02:00:43.493Z) |
| 恢复诊断 | raw=loss<3%=928ms (2026-04-17T02:00:31.993Z), rtt<120ms=428ms (2026-04-17T02:00:31.493Z), jitter<28ms=6928ms (2026-04-17T02:00:37.993Z), jitter<18ms=7928ms (2026-04-17T02:00:38.993Z)；target=target>=120kbps=428ms (2026-04-17T02:00:31.493Z), target>=300kbps=428ms (2026-04-17T02:00:31.493Z), target>=500kbps=428ms (2026-04-17T02:00:31.493Z), target>=700kbps=6928ms (2026-04-17T02:00:37.993Z), target>=900kbps=13428ms (2026-04-17T02:00:44.493Z)；send=send>=300kbps=428ms (2026-04-17T02:00:31.493Z), send>=500kbps=428ms (2026-04-17T02:00:31.493Z), send>=700kbps=1428ms (2026-04-17T02:00:32.493Z), send>=900kbps=12928ms (2026-04-17T02:00:43.993Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2451ms, recovering=-, stable=451ms, congested=-, firstAction=2451ms, L0=-, L1=2451ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=428ms, recovering=-, stable=12428ms, congested=-, firstAction=12428ms, L0=12428ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |
