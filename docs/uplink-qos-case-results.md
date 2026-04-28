# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-28T04:38:31.524Z`

## 1. 汇总

- 总 Case：`48`
- 已执行：`46`
- 通过：`44`
- 失败：`2`
- 错误：`2`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [B3](#b3) | `ERROR` | baseline contamination detected for B3: baseline entered recovering before any impairment; state=recovering/L3 qdisc=qdisc netem 1: root refcnt 2 limit 1000 delay 28ms  12ms loss 0.5% rate 1400Kbit |
| [L1](#l1) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, maxActionCountPassed=true, analysis=过强 |
| [L5](#l5) | `ERROR` | Runtime.callFunctionOn timed out. Increase the 'protocolTimeout' setting in launch/connect calls for a higher timeout if needed. |
| [M1](#m1) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, maxActionCountPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[B3](#b3)、[L1](#l1)、[L5](#l5)、[M1](#m1)
- baseline：[B1](#b1)、[B2](#b2)、[B3](#b3)
- bw_sweep：[BW1](#bw1)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)
- loss_sweep：[L1](#l1)、[L2](#l2)、[L3](#l3)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)
- rtt_sweep：[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)
- jitter_sweep：[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)
- burst：[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)
- traffic_model：[M1](#m1)、[M2](#m2)、[M3](#m3)
- oscillation：[O1](#o1)、[O2](#o2)

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
| 恢复里程碑 | start=2026-04-28T03:20:17.070Z；firstRecovering=-；firstStable=120ms (2026-04-28T03:20:17.190Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=120ms (2026-04-28T03:20:17.190Z), rtt<120ms=120ms (2026-04-28T03:20:17.190Z), jitter<28ms=120ms (2026-04-28T03:20:17.190Z), jitter<18ms=120ms (2026-04-28T03:20:17.190Z)；target=target>=120kbps=120ms (2026-04-28T03:20:17.190Z), target>=300kbps=120ms (2026-04-28T03:20:17.190Z), target>=500kbps=120ms (2026-04-28T03:20:17.190Z), target>=700kbps=120ms (2026-04-28T03:20:17.190Z), target>=900kbps=120ms (2026-04-28T03:20:17.190Z)；send=send>=300kbps=120ms (2026-04-28T03:20:17.190Z), send>=500kbps=120ms (2026-04-28T03:20:17.190Z), send>=700kbps=120ms (2026-04-28T03:20:17.190Z), send>=900kbps=120ms (2026-04-28T03:20:17.190Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=298ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=120ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T03:21:26.361Z；firstRecovering=-；firstStable=84ms (2026-04-28T03:21:26.445Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=84ms (2026-04-28T03:21:26.445Z), rtt<120ms=84ms (2026-04-28T03:21:26.445Z), jitter<28ms=84ms (2026-04-28T03:21:26.445Z), jitter<18ms=84ms (2026-04-28T03:21:26.445Z)；target=target>=120kbps=84ms (2026-04-28T03:21:26.445Z), target>=300kbps=84ms (2026-04-28T03:21:26.445Z), target>=500kbps=84ms (2026-04-28T03:21:26.445Z), target>=700kbps=84ms (2026-04-28T03:21:26.445Z), target>=900kbps=84ms (2026-04-28T03:21:26.445Z)；send=send>=300kbps=84ms (2026-04-28T03:21:26.445Z), send>=500kbps=84ms (2026-04-28T03:21:26.445Z), send>=700kbps=84ms (2026-04-28T03:21:26.445Z), send>=900kbps=84ms (2026-04-28T03:21:26.445Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=205ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=84ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | ERROR：baseline contamination detected for B3: baseline entered recovering before any impairment; state=recovering/L3 qdisc=qdisc netem 1: root refcnt 2 limit 1000 delay 28ms  12ms loss 0.5% rate 1400Kbit |
| 实际 QoS 状态 | 执行错误：baseline contamination detected for B3: baseline entered recovering before any impairment; state=recovering/L3 qdisc=qdisc netem 1: root refcnt 2 limit 1000 delay 28ms  12ms loss 0.5% rate 1400Kbit |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：baseline contamination detected for B3: baseline entered recovering before any impairment; state=recovering/L3 qdisc=qdisc netem 1: root refcnt 2 limit 1000 delay 28ms  12ms loss 0.5% rate 1400Kbit |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T03:22:55.009Z；firstRecovering=-；firstStable=438ms (2026-04-28T03:22:55.447Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=938ms (2026-04-28T03:22:55.947Z), rtt<120ms=438ms (2026-04-28T03:22:55.447Z), jitter<28ms=438ms (2026-04-28T03:22:55.447Z), jitter<18ms=1438ms (2026-04-28T03:22:56.447Z)；target=target>=120kbps=438ms (2026-04-28T03:22:55.447Z), target>=300kbps=438ms (2026-04-28T03:22:55.447Z), target>=500kbps=438ms (2026-04-28T03:22:55.447Z), target>=700kbps=438ms (2026-04-28T03:22:55.447Z), target>=900kbps=438ms (2026-04-28T03:22:55.447Z)；send=send>=300kbps=438ms (2026-04-28T03:22:55.447Z), send>=500kbps=438ms (2026-04-28T03:22:55.447Z), send>=700kbps=438ms (2026-04-28T03:22:55.447Z), send>=900kbps=438ms (2026-04-28T03:22:55.447Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=203ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=438ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 24 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:24:04.515Z；firstRecovering=5696ms (2026-04-28T03:24:10.211Z)；firstStable=8198ms (2026-04-28T03:24:12.713Z)；firstL0=10198ms (2026-04-28T03:24:14.713Z) |
| 恢复诊断 | raw=loss<3%=197ms (2026-04-28T03:24:04.712Z), rtt<120ms=197ms (2026-04-28T03:24:04.712Z), jitter<28ms=4697ms (2026-04-28T03:24:09.212Z), jitter<18ms=20704ms (2026-04-28T03:24:25.219Z)；target=target>=120kbps=197ms (2026-04-28T03:24:04.712Z), target>=300kbps=7696ms (2026-04-28T03:24:12.211Z), target>=500kbps=24196ms (2026-04-28T03:24:28.711Z), target>=700kbps=25208ms (2026-04-28T03:24:29.723Z), target>=900kbps=26197ms (2026-04-28T03:24:30.712Z)；send=send>=300kbps=6196ms (2026-04-28T03:24:10.711Z), send>=500kbps=6696ms (2026-04-28T03:24:11.211Z), send>=700kbps=10728ms (2026-04-28T03:24:15.243Z), send>=900kbps=13699ms (2026-04-28T03:24:18.214Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1699ms, recovering=10199ms, stable=204ms, congested=2199ms, firstAction=1699ms, L0=14701ms, L1=1699ms, L2=2199ms, L3=2700ms, L4=3202ms, audioOnly=-；recovery: warning=12696ms, recovering=5696ms, stable=8198ms, congested=197ms, firstAction=197ms, L0=10198ms, L1=8696ms, L2=7196ms, L3=5696ms, L4=197ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 14 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:25:14.209Z；firstRecovering=14453ms (2026-04-28T03:25:28.662Z)；firstStable=16950ms (2026-04-28T03:25:31.159Z)；firstL0=18969ms (2026-04-28T03:25:33.178Z) |
| 恢复诊断 | raw=loss<3%=949ms (2026-04-28T03:25:15.158Z), rtt<120ms=949ms (2026-04-28T03:25:15.158Z), jitter<28ms=13952ms (2026-04-28T03:25:28.161Z), jitter<18ms=19449ms (2026-04-28T03:25:33.658Z)；target=target>=120kbps=449ms (2026-04-28T03:25:14.658Z), target>=300kbps=14965ms (2026-04-28T03:25:29.174Z), target>=500kbps=16449ms (2026-04-28T03:25:30.658Z), target>=700kbps=17951ms (2026-04-28T03:25:32.160Z), target>=900kbps=19449ms (2026-04-28T03:25:33.658Z)；send=send>=300kbps=1953ms (2026-04-28T03:25:16.162Z), send>=500kbps=15450ms (2026-04-28T03:25:29.659Z), send>=700kbps=17951ms (2026-04-28T03:25:32.160Z), send>=900kbps=17951ms (2026-04-28T03:25:32.160Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2160ms, recovering=15159ms, stable=160ms, congested=3661ms, firstAction=2160ms, L0=-, L1=2160ms, L2=3661ms, L3=4159ms, L4=4659ms, audioOnly=-；recovery: warning=449ms, recovering=14453ms, stable=16950ms, congested=949ms, firstAction=949ms, L0=18969ms, L1=17450ms, L2=949ms, L3=1449ms, L4=1953ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 12 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:26:25.013Z；firstRecovering=13245ms (2026-04-28T03:26:38.258Z)；firstStable=15746ms (2026-04-28T03:26:40.759Z)；firstL0=17745ms (2026-04-28T03:26:42.758Z) |
| 恢复诊断 | raw=loss<3%=245ms (2026-04-28T03:26:25.258Z), rtt<120ms=245ms (2026-04-28T03:26:25.258Z), jitter<28ms=12745ms (2026-04-28T03:26:37.758Z), jitter<18ms=13745ms (2026-04-28T03:26:38.758Z)；target=target>=120kbps=245ms (2026-04-28T03:26:25.258Z), target>=300kbps=13745ms (2026-04-28T03:26:38.758Z), target>=500kbps=15245ms (2026-04-28T03:26:40.258Z), target>=700kbps=16745ms (2026-04-28T03:26:41.758Z), target>=900kbps=18747ms (2026-04-28T03:26:43.760Z)；send=send>=300kbps=13745ms (2026-04-28T03:26:38.758Z), send>=500kbps=15245ms (2026-04-28T03:26:40.258Z), send>=700kbps=15245ms (2026-04-28T03:26:40.258Z), send>=900kbps=16745ms (2026-04-28T03:26:41.758Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3496ms, recovering=-, stable=488ms, congested=5485ms, firstAction=3496ms, L0=-, L1=3496ms, L2=5485ms, L3=5987ms, L4=6507ms, audioOnly=-；recovery: warning=20246ms, recovering=13245ms, stable=15746ms, congested=245ms, firstAction=245ms, L0=17745ms, L1=16245ms, L2=14745ms, L3=13245ms, L4=245ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 12 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-28T03:27:35.436Z；firstRecovering=16870ms (2026-04-28T03:27:52.306Z)；firstStable=19477ms (2026-04-28T03:27:54.913Z)；firstL0=21371ms (2026-04-28T03:27:56.807Z) |
| 恢复诊断 | raw=loss<3%=870ms (2026-04-28T03:27:36.306Z), rtt<120ms=370ms (2026-04-28T03:27:35.806Z), jitter<28ms=16370ms (2026-04-28T03:27:51.806Z), jitter<18ms=29870ms (2026-04-28T03:28:05.306Z)；target=target>=120kbps=12370ms (2026-04-28T03:27:47.806Z), target>=300kbps=17370ms (2026-04-28T03:27:52.806Z), target>=500kbps=18870ms (2026-04-28T03:27:54.306Z), target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=17370ms (2026-04-28T03:27:52.806Z), send>=500kbps=18870ms (2026-04-28T03:27:54.306Z), send>=700kbps=21884ms (2026-04-28T03:27:57.320Z), send>=900kbps=24370ms (2026-04-28T03:27:59.806Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1792ms, recovering=-, stable=299ms, congested=3291ms, firstAction=1792ms, L0=-, L1=1792ms, L2=3291ms, L3=3791ms, L4=4291ms, audioOnly=-；recovery: warning=23871ms, recovering=16870ms, stable=19477ms, congested=370ms, firstAction=370ms, L0=21371ms, L1=19870ms, L2=18371ms, L3=16870ms, L4=370ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 8 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:29:06.516Z；firstRecovering=15576ms (2026-04-28T03:29:22.092Z)；firstStable=18077ms (2026-04-28T03:29:24.593Z)；firstL0=19579ms (2026-04-28T03:29:26.095Z) |
| 恢复诊断 | raw=loss<3%=580ms (2026-04-28T03:29:07.096Z), rtt<120ms=85ms (2026-04-28T03:29:06.601Z), jitter<28ms=15081ms (2026-04-28T03:29:21.597Z), jitter<18ms=16076ms (2026-04-28T03:29:22.592Z)；target=target>=120kbps=3076ms (2026-04-28T03:29:09.592Z), target>=300kbps=16076ms (2026-04-28T03:29:22.592Z), target>=500kbps=17577ms (2026-04-28T03:29:24.093Z), target>=700kbps=18584ms (2026-04-28T03:29:25.100Z), target>=900kbps=20079ms (2026-04-28T03:29:26.595Z)；send=send>=300kbps=580ms (2026-04-28T03:29:07.096Z), send>=500kbps=17577ms (2026-04-28T03:29:24.093Z), send>=700kbps=18584ms (2026-04-28T03:29:25.100Z), send>=900kbps=18584ms (2026-04-28T03:29:25.100Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1983ms, recovering=-, stable=478ms, congested=2479ms, firstAction=1983ms, L0=-, L1=1983ms, L2=2479ms, L3=2978ms, L4=3478ms, audioOnly=-；recovery: warning=-, recovering=15576ms, stable=18077ms, congested=85ms, firstAction=85ms, L0=19579ms, L1=18077ms, L2=17076ms, L3=15576ms, L4=85ms, audioOnly=- |

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
| 实际结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, maxActionCountPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"state":"stable","maxLevel":1}；实际 impairment 评估值=early_warning/L1，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=true, recoveryPassed=true, maxActionCountPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-28T03:30:16.074Z；firstRecovering=-；firstStable=272ms (2026-04-28T03:30:16.346Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=272ms (2026-04-28T03:30:16.346Z), rtt<120ms=272ms (2026-04-28T03:30:16.346Z), jitter<28ms=272ms (2026-04-28T03:30:16.346Z), jitter<18ms=272ms (2026-04-28T03:30:16.346Z)；target=target>=120kbps=272ms (2026-04-28T03:30:16.346Z), target>=300kbps=272ms (2026-04-28T03:30:16.346Z), target>=500kbps=272ms (2026-04-28T03:30:16.346Z), target>=700kbps=272ms (2026-04-28T03:30:16.346Z), target>=900kbps=272ms (2026-04-28T03:30:16.346Z)；send=send>=300kbps=272ms (2026-04-28T03:30:16.346Z), send>=500kbps=272ms (2026-04-28T03:30:16.346Z), send>=700kbps=272ms (2026-04-28T03:30:16.346Z), send>=900kbps=272ms (2026-04-28T03:30:16.346Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3387ms, recovering=-, stable=390ms, congested=-, firstAction=3387ms, L0=14876ms, L1=3387ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=272ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 8 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:31:25.646Z；firstRecovering=-；firstStable=448ms (2026-04-28T03:31:26.094Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=448ms (2026-04-28T03:31:26.094Z), rtt<120ms=448ms (2026-04-28T03:31:26.094Z), jitter<28ms=448ms (2026-04-28T03:31:26.094Z), jitter<18ms=448ms (2026-04-28T03:31:26.094Z)；target=target>=120kbps=448ms (2026-04-28T03:31:26.094Z), target>=300kbps=448ms (2026-04-28T03:31:26.094Z), target>=500kbps=448ms (2026-04-28T03:31:26.094Z), target>=700kbps=448ms (2026-04-28T03:31:26.094Z), target>=900kbps=448ms (2026-04-28T03:31:26.094Z)；send=send>=300kbps=448ms (2026-04-28T03:31:26.094Z), send>=500kbps=448ms (2026-04-28T03:31:26.094Z), send>=700kbps=448ms (2026-04-28T03:31:26.094Z), send>=900kbps=448ms (2026-04-28T03:31:26.094Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=201ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=448ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:32:37.078Z；firstRecovering=-；firstStable=24241ms (2026-04-28T03:33:01.319Z)；firstL0=24241ms (2026-04-28T03:33:01.319Z) |
| 恢复诊断 | raw=loss<3%=233ms (2026-04-28T03:32:37.311Z), rtt<120ms=233ms (2026-04-28T03:32:37.311Z), jitter<28ms=233ms (2026-04-28T03:32:37.311Z), jitter<18ms=5752ms (2026-04-28T03:32:42.830Z)；target=target>=120kbps=233ms (2026-04-28T03:32:37.311Z), target>=300kbps=233ms (2026-04-28T03:32:37.311Z), target>=500kbps=233ms (2026-04-28T03:32:37.311Z), target>=700kbps=233ms (2026-04-28T03:32:37.311Z), target>=900kbps=24722ms (2026-04-28T03:33:01.800Z)；send=send>=300kbps=233ms (2026-04-28T03:32:37.311Z), send>=500kbps=233ms (2026-04-28T03:32:37.311Z), send>=700kbps=233ms (2026-04-28T03:32:37.311Z), send>=900kbps=2745ms (2026-04-28T03:32:39.823Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=173ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=233ms, recovering=-, stable=24241ms, congested=-, firstAction=24241ms, L0=24241ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:33:51.823Z；firstRecovering=-；firstStable=14167ms (2026-04-28T03:34:05.990Z)；firstL0=14167ms (2026-04-28T03:34:05.990Z) |
| 恢复诊断 | raw=loss<3%=177ms (2026-04-28T03:33:52.000Z), rtt<120ms=177ms (2026-04-28T03:33:52.000Z), jitter<28ms=177ms (2026-04-28T03:33:52.000Z), jitter<18ms=1177ms (2026-04-28T03:33:53.000Z)；target=target>=120kbps=177ms (2026-04-28T03:33:52.000Z), target>=300kbps=177ms (2026-04-28T03:33:52.000Z), target>=500kbps=177ms (2026-04-28T03:33:52.000Z), target>=700kbps=177ms (2026-04-28T03:33:52.000Z), target>=900kbps=14681ms (2026-04-28T03:34:06.504Z)；send=send>=300kbps=177ms (2026-04-28T03:33:52.000Z), send>=500kbps=177ms (2026-04-28T03:33:52.000Z), send>=700kbps=177ms (2026-04-28T03:33:52.000Z), send>=900kbps=177ms (2026-04-28T03:33:52.000Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=71ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=177ms, recovering=-, stable=14167ms, congested=-, firstAction=14167ms, L0=14167ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | ERROR：Runtime.callFunctionOn timed out. Increase the 'protocolTimeout' setting in launch/connect calls for a higher timeout if needed. |
| 实际 QoS 状态 | 执行错误：Runtime.callFunctionOn timed out. Increase the 'protocolTimeout' setting in launch/connect calls for a higher timeout if needed. |
| 实际动作 | 未完成 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 执行失败。浏览器/runner 在该 case 中断，错误：Runtime.callFunctionOn timed out. Increase the 'protocolTimeout' setting in launch/connect calls for a higher timeout if needed. |
| 恢复里程碑 | - |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 8 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:56:25.094Z；firstRecovering=8382ms (2026-04-28T03:56:33.476Z)；firstStable=10879ms (2026-04-28T03:56:35.973Z)；firstL0=12381ms (2026-04-28T03:56:37.475Z) |
| 恢复诊断 | raw=loss<3%=378ms (2026-04-28T03:56:25.472Z), rtt<120ms=378ms (2026-04-28T03:56:25.472Z), jitter<28ms=7883ms (2026-04-28T03:56:32.977Z), jitter<18ms=14879ms (2026-04-28T03:56:39.973Z)；target=target>=120kbps=8878ms (2026-04-28T03:56:33.972Z), target>=300kbps=8878ms (2026-04-28T03:56:33.972Z), target>=500kbps=11382ms (2026-04-28T03:56:36.476Z), target>=700kbps=11382ms (2026-04-28T03:56:36.476Z), target>=900kbps=12881ms (2026-04-28T03:56:37.975Z)；send=send>=300kbps=8878ms (2026-04-28T03:56:33.972Z), send>=500kbps=8878ms (2026-04-28T03:56:33.972Z), send>=700kbps=8878ms (2026-04-28T03:56:33.972Z), send>=900kbps=10379ms (2026-04-28T03:56:35.473Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=991ms, recovering=-, stable=491ms, congested=2997ms, firstAction=991ms, L0=-, L1=991ms, L2=2997ms, L3=3491ms, L4=3992ms, audioOnly=-；recovery: warning=-, recovering=8382ms, stable=10879ms, congested=378ms, firstAction=378ms, L0=12381ms, L1=10879ms, L2=9878ms, L3=8382ms, L4=378ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 10 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T03:57:34.448Z；firstRecovering=14875ms (2026-04-28T03:57:49.323Z)；firstStable=17375ms (2026-04-28T03:57:51.823Z)；firstL0=19375ms (2026-04-28T03:57:53.823Z) |
| 恢复诊断 | raw=loss<3%=881ms (2026-04-28T03:57:35.329Z), rtt<120ms=378ms (2026-04-28T03:57:34.826Z), jitter<28ms=14375ms (2026-04-28T03:57:48.823Z), jitter<18ms=20875ms (2026-04-28T03:57:55.323Z)；target=target>=120kbps=11375ms (2026-04-28T03:57:45.823Z), target>=300kbps=15376ms (2026-04-28T03:57:49.824Z), target>=500kbps=16877ms (2026-04-28T03:57:51.325Z), target>=700kbps=18379ms (2026-04-28T03:57:52.827Z), target>=900kbps=19875ms (2026-04-28T03:57:54.323Z)；send=send>=300kbps=6875ms (2026-04-28T03:57:41.323Z), send>=500kbps=15376ms (2026-04-28T03:57:49.824Z), send>=700kbps=15376ms (2026-04-28T03:57:49.824Z), send>=900kbps=16877ms (2026-04-28T03:57:51.325Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2094ms, recovering=-, stable=99ms, congested=2594ms, firstAction=2094ms, L0=-, L1=2094ms, L2=2594ms, L3=3094ms, L4=3594ms, audioOnly=-；recovery: warning=21875ms, recovering=14875ms, stable=17375ms, congested=378ms, firstAction=378ms, L0=19375ms, L1=17887ms, L2=16377ms, L3=14875ms, L4=378ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 8 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:01:45.139Z；firstRecovering=18823ms (2026-04-28T04:02:03.962Z)；firstStable=21323ms (2026-04-28T04:02:06.462Z)；firstL0=22324ms (2026-04-28T04:02:07.463Z) |
| 恢复诊断 | raw=loss<3%=826ms (2026-04-28T04:01:45.965Z), rtt<120ms=355ms (2026-04-28T04:01:45.494Z), jitter<28ms=18325ms (2026-04-28T04:02:03.464Z), jitter<18ms=19323ms (2026-04-28T04:02:04.462Z)；target=target>=120kbps=10828ms (2026-04-28T04:01:55.967Z), target>=300kbps=19323ms (2026-04-28T04:02:04.462Z), target>=500kbps=20825ms (2026-04-28T04:02:05.964Z), target>=700kbps=21824ms (2026-04-28T04:02:06.963Z), target>=900kbps=22823ms (2026-04-28T04:02:07.962Z)；send=send>=300kbps=19323ms (2026-04-28T04:02:04.462Z), send>=500kbps=20825ms (2026-04-28T04:02:05.964Z), send>=700kbps=20825ms (2026-04-28T04:02:05.964Z), send>=900kbps=21824ms (2026-04-28T04:02:06.963Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3076ms, recovering=-, stable=65ms, congested=3566ms, firstAction=3076ms, L0=-, L1=3076ms, L2=3566ms, L3=4065ms, L4=4565ms, audioOnly=-；recovery: warning=-, recovering=18823ms, stable=21323ms, congested=355ms, firstAction=355ms, L0=22324ms, L1=21323ms, L2=20332ms, L3=18823ms, L4=355ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:02:54.447Z；firstRecovering=-；firstStable=26ms (2026-04-28T04:02:54.473Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=26ms (2026-04-28T04:02:54.473Z), rtt<120ms=26ms (2026-04-28T04:02:54.473Z), jitter<28ms=26ms (2026-04-28T04:02:54.473Z), jitter<18ms=26ms (2026-04-28T04:02:54.473Z)；target=target>=120kbps=26ms (2026-04-28T04:02:54.473Z), target>=300kbps=26ms (2026-04-28T04:02:54.473Z), target>=500kbps=26ms (2026-04-28T04:02:54.473Z), target>=700kbps=26ms (2026-04-28T04:02:54.473Z), target>=900kbps=26ms (2026-04-28T04:02:54.473Z)；send=send>=300kbps=26ms (2026-04-28T04:02:54.473Z), send>=500kbps=26ms (2026-04-28T04:02:54.473Z), send>=700kbps=26ms (2026-04-28T04:02:54.473Z), send>=900kbps=26ms (2026-04-28T04:02:54.473Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=291ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=26ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:04:03.759Z；firstRecovering=-；firstStable=492ms (2026-04-28T04:04:04.251Z)；firstL0=14983ms (2026-04-28T04:04:18.742Z) |
| 恢复诊断 | raw=loss<3%=492ms (2026-04-28T04:04:04.251Z), rtt<120ms=492ms (2026-04-28T04:04:04.251Z), jitter<28ms=492ms (2026-04-28T04:04:04.251Z), jitter<18ms=492ms (2026-04-28T04:04:04.251Z)；target=target>=120kbps=492ms (2026-04-28T04:04:04.251Z), target>=300kbps=492ms (2026-04-28T04:04:04.251Z), target>=500kbps=492ms (2026-04-28T04:04:04.251Z), target>=700kbps=492ms (2026-04-28T04:04:04.251Z), target>=900kbps=15488ms (2026-04-28T04:04:19.247Z)；send=send>=300kbps=492ms (2026-04-28T04:04:04.251Z), send>=500kbps=492ms (2026-04-28T04:04:04.251Z), send>=700kbps=492ms (2026-04-28T04:04:04.251Z), send>=900kbps=492ms (2026-04-28T04:04:04.251Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=265ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=1484ms, recovering=-, stable=492ms, congested=-, firstAction=1484ms, L0=14983ms, L1=1484ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:05:13.043Z；firstRecovering=-；firstStable=434ms (2026-04-28T04:05:13.477Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=434ms (2026-04-28T04:05:13.477Z), rtt<120ms=1439ms (2026-04-28T04:05:14.482Z), jitter<28ms=434ms (2026-04-28T04:05:13.477Z), jitter<18ms=434ms (2026-04-28T04:05:13.477Z)；target=target>=120kbps=434ms (2026-04-28T04:05:13.477Z), target>=300kbps=434ms (2026-04-28T04:05:13.477Z), target>=500kbps=434ms (2026-04-28T04:05:13.477Z), target>=700kbps=434ms (2026-04-28T04:05:13.477Z), target>=900kbps=434ms (2026-04-28T04:05:13.477Z)；send=send>=300kbps=434ms (2026-04-28T04:05:13.477Z), send>=500kbps=434ms (2026-04-28T04:05:13.477Z), send>=700kbps=434ms (2026-04-28T04:05:13.477Z), send>=900kbps=434ms (2026-04-28T04:05:13.477Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=255ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=434ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:06:22.480Z；firstRecovering=-；firstStable=3279ms (2026-04-28T04:06:25.759Z)；firstL0=3279ms (2026-04-28T04:06:25.759Z) |
| 恢复诊断 | raw=loss<3%=279ms (2026-04-28T04:06:22.759Z), rtt<120ms=1279ms (2026-04-28T04:06:23.759Z), jitter<28ms=279ms (2026-04-28T04:06:22.759Z), jitter<18ms=279ms (2026-04-28T04:06:22.759Z)；target=target>=120kbps=279ms (2026-04-28T04:06:22.759Z), target>=300kbps=279ms (2026-04-28T04:06:22.759Z), target>=500kbps=279ms (2026-04-28T04:06:22.759Z), target>=700kbps=279ms (2026-04-28T04:06:22.759Z), target>=900kbps=4778ms (2026-04-28T04:06:27.258Z)；send=send>=300kbps=279ms (2026-04-28T04:06:22.759Z), send>=500kbps=279ms (2026-04-28T04:06:22.759Z), send>=700kbps=279ms (2026-04-28T04:06:22.759Z), send>=900kbps=3824ms (2026-04-28T04:06:26.304Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1671ms, recovering=-, stable=170ms, congested=-, firstAction=1671ms, L0=-, L1=1671ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=279ms, recovering=-, stable=3279ms, congested=-, firstAction=3279ms, L0=3279ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 10 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:07:32.112Z；firstRecovering=4108ms (2026-04-28T04:07:36.220Z)；firstStable=4608ms (2026-04-28T04:07:36.720Z)；firstL0=8601ms (2026-04-28T04:07:40.713Z) |
| 恢复诊断 | raw=loss<3%=101ms (2026-04-28T04:07:32.213Z), rtt<120ms=1101ms (2026-04-28T04:07:33.213Z), jitter<28ms=101ms (2026-04-28T04:07:32.213Z), jitter<18ms=101ms (2026-04-28T04:07:32.213Z)；target=target>=120kbps=101ms (2026-04-28T04:07:32.213Z), target>=300kbps=4608ms (2026-04-28T04:07:36.720Z), target>=500kbps=6102ms (2026-04-28T04:07:38.214Z), target>=700kbps=7625ms (2026-04-28T04:07:39.737Z), target>=900kbps=10122ms (2026-04-28T04:07:42.234Z)；send=send>=300kbps=4608ms (2026-04-28T04:07:36.720Z), send>=500kbps=4608ms (2026-04-28T04:07:36.720Z), send>=700kbps=4608ms (2026-04-28T04:07:36.720Z), send>=900kbps=7625ms (2026-04-28T04:07:39.737Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1640ms, recovering=-, stable=139ms, congested=2654ms, firstAction=1640ms, L0=-, L1=1640ms, L2=2654ms, L3=3140ms, L4=3640ms, audioOnly=-；recovery: warning=11105ms, recovering=4108ms, stable=4608ms, congested=101ms, firstAction=101ms, L0=8601ms, L1=7101ms, L2=5605ms, L3=4108ms, L4=101ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 16 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:08:41.990Z；firstRecovering=4450ms (2026-04-28T04:08:46.440Z)；firstStable=4950ms (2026-04-28T04:08:46.940Z)；firstL0=8950ms (2026-04-28T04:08:50.940Z) |
| 恢复诊断 | raw=loss<3%=450ms (2026-04-28T04:08:42.440Z), rtt<120ms=951ms (2026-04-28T04:08:42.941Z), jitter<28ms=450ms (2026-04-28T04:08:42.440Z), jitter<18ms=450ms (2026-04-28T04:08:42.440Z)；target=target>=120kbps=450ms (2026-04-28T04:08:42.440Z), target>=300kbps=4950ms (2026-04-28T04:08:46.940Z), target>=500kbps=6950ms (2026-04-28T04:08:48.940Z), target>=700kbps=11450ms (2026-04-28T04:08:53.440Z), target>=900kbps=23950ms (2026-04-28T04:09:05.940Z)；send=send>=300kbps=4950ms (2026-04-28T04:08:46.940Z), send>=500kbps=4950ms (2026-04-28T04:08:46.940Z), send>=700kbps=5450ms (2026-04-28T04:08:47.440Z), send>=900kbps=6950ms (2026-04-28T04:08:48.940Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1106ms, recovering=-, stable=107ms, congested=1607ms, firstAction=1106ms, L0=-, L1=1106ms, L2=1607ms, L3=2108ms, L4=2609ms, audioOnly=-；recovery: warning=11450ms, recovering=4450ms, stable=4950ms, congested=450ms, firstAction=450ms, L0=8950ms, L1=7452ms, L2=5951ms, L3=4450ms, L4=450ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:09:51.115Z；firstRecovering=-；firstStable=57ms (2026-04-28T04:09:51.172Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=57ms (2026-04-28T04:09:51.172Z), rtt<120ms=57ms (2026-04-28T04:09:51.172Z), jitter<28ms=57ms (2026-04-28T04:09:51.172Z), jitter<18ms=57ms (2026-04-28T04:09:51.172Z)；target=target>=120kbps=57ms (2026-04-28T04:09:51.172Z), target>=300kbps=57ms (2026-04-28T04:09:51.172Z), target>=500kbps=57ms (2026-04-28T04:09:51.172Z), target>=700kbps=57ms (2026-04-28T04:09:51.172Z), target>=900kbps=57ms (2026-04-28T04:09:51.172Z)；send=send>=300kbps=57ms (2026-04-28T04:09:51.172Z), send>=500kbps=57ms (2026-04-28T04:09:51.172Z), send>=700kbps=57ms (2026-04-28T04:09:51.172Z), send>=900kbps=57ms (2026-04-28T04:09:51.172Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=275ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=57ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:11:00.200Z；firstRecovering=-；firstStable=53ms (2026-04-28T04:11:00.253Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=53ms (2026-04-28T04:11:00.253Z), rtt<120ms=53ms (2026-04-28T04:11:00.253Z), jitter<28ms=53ms (2026-04-28T04:11:00.253Z), jitter<18ms=1052ms (2026-04-28T04:11:01.252Z)；target=target>=120kbps=53ms (2026-04-28T04:11:00.253Z), target>=300kbps=53ms (2026-04-28T04:11:00.253Z), target>=500kbps=53ms (2026-04-28T04:11:00.253Z), target>=700kbps=53ms (2026-04-28T04:11:00.253Z), target>=900kbps=53ms (2026-04-28T04:11:00.253Z)；send=send>=300kbps=53ms (2026-04-28T04:11:00.253Z), send>=500kbps=53ms (2026-04-28T04:11:00.253Z), send>=700kbps=53ms (2026-04-28T04:11:00.253Z), send>=900kbps=53ms (2026-04-28T04:11:00.253Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=285ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=53ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:12:09.422Z；firstRecovering=-；firstStable=16033ms (2026-04-28T04:12:25.455Z)；firstL0=16033ms (2026-04-28T04:12:25.455Z) |
| 恢复诊断 | raw=loss<3%=32ms (2026-04-28T04:12:09.454Z), rtt<120ms=32ms (2026-04-28T04:12:09.454Z), jitter<28ms=1033ms (2026-04-28T04:12:10.455Z), jitter<18ms=5535ms (2026-04-28T04:12:14.957Z)；target=target>=120kbps=32ms (2026-04-28T04:12:09.454Z), target>=300kbps=32ms (2026-04-28T04:12:09.454Z), target>=500kbps=32ms (2026-04-28T04:12:09.454Z), target>=700kbps=32ms (2026-04-28T04:12:09.454Z), target>=900kbps=16537ms (2026-04-28T04:12:25.959Z)；send=send>=300kbps=32ms (2026-04-28T04:12:09.454Z), send>=500kbps=32ms (2026-04-28T04:12:09.454Z), send>=700kbps=530ms (2026-04-28T04:12:09.952Z), send>=900kbps=1534ms (2026-04-28T04:12:10.956Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3771ms, recovering=-, stable=271ms, congested=-, firstAction=3771ms, L0=-, L1=3771ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=32ms, recovering=-, stable=16033ms, congested=-, firstAction=16033ms, L0=16033ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:13:18.623Z；firstRecovering=-；firstStable=7946ms (2026-04-28T04:13:26.569Z)；firstL0=7946ms (2026-04-28T04:13:26.569Z) |
| 恢复诊断 | raw=loss<3%=946ms (2026-04-28T04:13:19.569Z), rtt<120ms=946ms (2026-04-28T04:13:19.569Z), jitter<28ms=1948ms (2026-04-28T04:13:20.571Z), jitter<18ms=3946ms (2026-04-28T04:13:22.569Z)；target=target>=120kbps=446ms (2026-04-28T04:13:19.069Z), target>=300kbps=446ms (2026-04-28T04:13:19.069Z), target>=500kbps=446ms (2026-04-28T04:13:19.069Z), target>=700kbps=446ms (2026-04-28T04:13:19.069Z), target>=900kbps=8445ms (2026-04-28T04:13:27.068Z)；send=send>=300kbps=446ms (2026-04-28T04:13:19.069Z), send>=500kbps=446ms (2026-04-28T04:13:19.069Z), send>=700kbps=946ms (2026-04-28T04:13:19.569Z), send>=900kbps=8445ms (2026-04-28T04:13:27.068Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3331ms, recovering=-, stable=308ms, congested=-, firstAction=3331ms, L0=-, L1=3331ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=446ms, recovering=-, stable=7946ms, congested=-, firstAction=7946ms, L0=7946ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 16 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:14:28.234Z；firstRecovering=4074ms (2026-04-28T04:14:32.308Z)；firstStable=6574ms (2026-04-28T04:14:34.808Z)；firstL0=8574ms (2026-04-28T04:14:36.808Z) |
| 恢复诊断 | raw=loss<3%=74ms (2026-04-28T04:14:28.308Z), rtt<120ms=74ms (2026-04-28T04:14:28.308Z), jitter<28ms=3575ms (2026-04-28T04:14:31.809Z), jitter<18ms=15574ms (2026-04-28T04:14:43.808Z)；target=target>=120kbps=74ms (2026-04-28T04:14:28.308Z), target>=300kbps=4574ms (2026-04-28T04:14:32.808Z), target>=500kbps=6074ms (2026-04-28T04:14:34.308Z), target>=700kbps=10076ms (2026-04-28T04:14:38.310Z), target>=900kbps=24074ms (2026-04-28T04:14:52.308Z)；send=send>=300kbps=4574ms (2026-04-28T04:14:32.808Z), send>=500kbps=4574ms (2026-04-28T04:14:32.808Z), send>=700kbps=4574ms (2026-04-28T04:14:32.808Z), send>=900kbps=6074ms (2026-04-28T04:14:34.308Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3164ms, recovering=-, stable=163ms, congested=4663ms, firstAction=3164ms, L0=-, L1=3164ms, L2=4663ms, L3=5163ms, L4=5663ms, audioOnly=-；recovery: warning=11082ms, recovering=4074ms, stable=6574ms, congested=74ms, firstAction=74ms, L0=8574ms, L1=7074ms, L2=5574ms, L3=4074ms, L4=74ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:15:37.723Z；firstRecovering=-；firstStable=16215ms (2026-04-28T04:15:53.938Z)；firstL0=16215ms (2026-04-28T04:15:53.938Z) |
| 恢复诊断 | raw=loss<3%=215ms (2026-04-28T04:15:37.938Z), rtt<120ms=215ms (2026-04-28T04:15:37.938Z), jitter<28ms=215ms (2026-04-28T04:15:37.938Z), jitter<18ms=13215ms (2026-04-28T04:15:50.938Z)；target=target>=120kbps=215ms (2026-04-28T04:15:37.938Z), target>=300kbps=215ms (2026-04-28T04:15:37.938Z), target>=500kbps=215ms (2026-04-28T04:15:37.938Z), target>=700kbps=215ms (2026-04-28T04:15:37.938Z), target>=900kbps=16719ms (2026-04-28T04:15:54.442Z)；send=send>=300kbps=215ms (2026-04-28T04:15:37.938Z), send>=500kbps=215ms (2026-04-28T04:15:37.938Z), send>=700kbps=215ms (2026-04-28T04:15:37.938Z), send>=900kbps=1215ms (2026-04-28T04:15:38.938Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3623ms, recovering=-, stable=123ms, congested=-, firstAction=3623ms, L0=-, L1=3623ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=215ms, recovering=-, stable=16215ms, congested=-, firstAction=16215ms, L0=16215ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 16 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:16:47.278Z；firstRecovering=12188ms (2026-04-28T04:16:59.466Z)；firstStable=14685ms (2026-04-28T04:17:01.963Z)；firstL0=16682ms (2026-04-28T04:17:03.960Z) |
| 恢复诊断 | raw=loss<3%=189ms (2026-04-28T04:16:47.467Z), rtt<120ms=189ms (2026-04-28T04:16:47.467Z), jitter<28ms=7182ms (2026-04-28T04:16:54.460Z), jitter<18ms=8682ms (2026-04-28T04:16:55.960Z)；target=target>=120kbps=189ms (2026-04-28T04:16:47.467Z), target>=300kbps=1184ms (2026-04-28T04:16:48.462Z), target>=500kbps=14183ms (2026-04-28T04:17:01.461Z), target>=700kbps=15682ms (2026-04-28T04:17:02.960Z), target>=900kbps=17184ms (2026-04-28T04:17:04.462Z)；send=send>=300kbps=189ms (2026-04-28T04:16:47.467Z), send>=500kbps=1688ms (2026-04-28T04:16:48.966Z), send>=700kbps=2686ms (2026-04-28T04:16:49.964Z), send>=900kbps=17683ms (2026-04-28T04:17:04.961Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1270ms, recovering=11765ms, stable=263ms, congested=1762ms, firstAction=1270ms, L0=16262ms, L1=1270ms, L2=1762ms, L3=2263ms, L4=2762ms, audioOnly=-；recovery: warning=189ms, recovering=12188ms, stable=14685ms, congested=1688ms, firstAction=1688ms, L0=16682ms, L1=15190ms, L2=1688ms, L3=2183ms, L4=2686ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 17 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-28T04:17:57.576Z；firstRecovering=450ms (2026-04-28T04:17:58.026Z)；firstStable=4961ms (2026-04-28T04:18:02.537Z)；firstL0=6950ms (2026-04-28T04:18:04.526Z) |
| 恢复诊断 | raw=loss<3%=450ms (2026-04-28T04:17:58.026Z), rtt<120ms=450ms (2026-04-28T04:17:58.026Z), jitter<28ms=450ms (2026-04-28T04:17:58.026Z), jitter<18ms=27476ms (2026-04-28T04:18:25.052Z)；target=target>=120kbps=450ms (2026-04-28T04:17:58.026Z), target>=300kbps=7450ms (2026-04-28T04:18:05.026Z), target>=500kbps=22456ms (2026-04-28T04:18:20.032Z), target>=700kbps=24450ms (2026-04-28T04:18:22.026Z), target>=900kbps=25452ms (2026-04-28T04:18:23.028Z)；send=send>=300kbps=951ms (2026-04-28T04:17:58.527Z), send>=500kbps=6950ms (2026-04-28T04:18:04.526Z), send>=700kbps=11951ms (2026-04-28T04:18:09.527Z), send>=900kbps=24450ms (2026-04-28T04:18:22.026Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1666ms, recovering=21176ms, stable=167ms, congested=2167ms, firstAction=1666ms, L0=-, L1=1666ms, L2=2167ms, L3=2670ms, L4=3168ms, audioOnly=-；recovery: warning=9449ms, recovering=450ms, stable=4961ms, congested=9949ms, firstAction=3951ms, L0=6950ms, L1=5453ms, L2=3951ms, L3=10449ms, L4=10952ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:19:06.686Z；firstRecovering=-；firstStable=78ms (2026-04-28T04:19:06.764Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=1075ms (2026-04-28T04:19:07.761Z), rtt<120ms=78ms (2026-04-28T04:19:06.764Z), jitter<28ms=78ms (2026-04-28T04:19:06.764Z), jitter<18ms=78ms (2026-04-28T04:19:06.764Z)；target=target>=120kbps=78ms (2026-04-28T04:19:06.764Z), target>=300kbps=78ms (2026-04-28T04:19:06.764Z), target>=500kbps=78ms (2026-04-28T04:19:06.764Z), target>=700kbps=78ms (2026-04-28T04:19:06.764Z), target>=900kbps=78ms (2026-04-28T04:19:06.764Z)；send=send>=300kbps=78ms (2026-04-28T04:19:06.764Z), send>=500kbps=78ms (2026-04-28T04:19:06.764Z), send>=700kbps=78ms (2026-04-28T04:19:06.764Z), send>=900kbps=78ms (2026-04-28T04:19:06.764Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=5289ms, recovering=-, stable=288ms, congested=-, firstAction=5289ms, L0=19288ms, L1=5289ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=575ms, recovering=-, stable=78ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 10 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:20:15.888Z；firstRecovering=8065ms (2026-04-28T04:20:23.953Z)；firstStable=10564ms (2026-04-28T04:20:26.452Z)；firstL0=12564ms (2026-04-28T04:20:28.452Z) |
| 恢复诊断 | raw=loss<3%=564ms (2026-04-28T04:20:16.452Z), rtt<120ms=67ms (2026-04-28T04:20:15.955Z), jitter<28ms=7566ms (2026-04-28T04:20:23.454Z), jitter<18ms=14064ms (2026-04-28T04:20:29.952Z)；target=target>=120kbps=8568ms (2026-04-28T04:20:24.456Z), target>=300kbps=10064ms (2026-04-28T04:20:25.952Z), target>=500kbps=11565ms (2026-04-28T04:20:27.453Z), target>=700kbps=11565ms (2026-04-28T04:20:27.453Z), target>=900kbps=13067ms (2026-04-28T04:20:28.955Z)；send=send>=300kbps=8568ms (2026-04-28T04:20:24.456Z), send>=500kbps=8568ms (2026-04-28T04:20:24.456Z), send>=700kbps=8568ms (2026-04-28T04:20:24.456Z), send>=900kbps=10064ms (2026-04-28T04:20:25.952Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1764ms, recovering=-, stable=259ms, congested=2259ms, firstAction=1764ms, L0=-, L1=1764ms, L2=2259ms, L3=2762ms, L4=3261ms, audioOnly=-；recovery: warning=15067ms, recovering=8065ms, stable=10564ms, congested=67ms, firstAction=67ms, L0=12564ms, L1=11064ms, L2=9566ms, L3=8065ms, L4=67ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:21:25.273Z；firstRecovering=-；firstStable=14803ms (2026-04-28T04:21:40.076Z)；firstL0=14803ms (2026-04-28T04:21:40.076Z) |
| 恢复诊断 | raw=loss<3%=303ms (2026-04-28T04:21:25.576Z), rtt<120ms=1305ms (2026-04-28T04:21:26.578Z), jitter<28ms=303ms (2026-04-28T04:21:25.576Z), jitter<18ms=8805ms (2026-04-28T04:21:34.078Z)；target=target>=120kbps=303ms (2026-04-28T04:21:25.576Z), target>=300kbps=303ms (2026-04-28T04:21:25.576Z), target>=500kbps=1305ms (2026-04-28T04:21:26.578Z), target>=700kbps=1305ms (2026-04-28T04:21:26.578Z), target>=900kbps=15304ms (2026-04-28T04:21:40.577Z)；send=send>=300kbps=303ms (2026-04-28T04:21:25.576Z), send>=500kbps=303ms (2026-04-28T04:21:25.576Z), send>=700kbps=303ms (2026-04-28T04:21:25.576Z), send>=900kbps=3804ms (2026-04-28T04:21:29.077Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1714ms, recovering=-, stable=214ms, congested=-, firstAction=1714ms, L0=-, L1=1714ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=303ms, recovering=-, stable=14803ms, congested=-, firstAction=14803ms, L0=14803ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:22:34.632Z；firstRecovering=-；firstStable=14331ms (2026-04-28T04:22:48.963Z)；firstL0=14331ms (2026-04-28T04:22:48.963Z) |
| 恢复诊断 | raw=loss<3%=332ms (2026-04-28T04:22:34.964Z), rtt<120ms=332ms (2026-04-28T04:22:34.964Z), jitter<28ms=835ms (2026-04-28T04:22:35.467Z), jitter<18ms=835ms (2026-04-28T04:22:35.467Z)；target=target>=120kbps=332ms (2026-04-28T04:22:34.964Z), target>=300kbps=332ms (2026-04-28T04:22:34.964Z), target>=500kbps=332ms (2026-04-28T04:22:34.964Z), target>=700kbps=332ms (2026-04-28T04:22:34.964Z), target>=900kbps=14831ms (2026-04-28T04:22:49.463Z)；send=send>=300kbps=332ms (2026-04-28T04:22:34.964Z), send>=500kbps=332ms (2026-04-28T04:22:34.964Z), send>=700kbps=332ms (2026-04-28T04:22:34.964Z), send>=900kbps=1332ms (2026-04-28T04:22:35.964Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3207ms, recovering=-, stable=211ms, congested=-, firstAction=3207ms, L0=-, L1=3207ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=332ms, recovering=-, stable=14331ms, congested=-, firstAction=14331ms, L0=14331ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-28T04:23:44.761Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | - |
| 关键时间指标 | impairment: warning=91ms, recovering=-, stable=-, congested=3090ms, firstAction=3090ms, L0=-, L1=-, L2=3090ms, L3=3590ms, L4=4090ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L2) |
| 实际动作 | setEncodingParameters（共 7 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L2，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-28T04:26:03.662Z；firstRecovering=22110ms (2026-04-28T04:26:25.772Z)；firstStable=24686ms (2026-04-28T04:26:28.348Z)；firstL0=26609ms (2026-04-28T04:26:30.271Z) |
| 恢复诊断 | raw=loss<3%=116ms (2026-04-28T04:26:03.778Z), rtt<120ms=116ms (2026-04-28T04:26:03.778Z), jitter<28ms=21609ms (2026-04-28T04:26:25.271Z), jitter<18ms=-；target=target>=120kbps=16112ms (2026-04-28T04:26:19.774Z), target>=300kbps=22625ms (2026-04-28T04:26:26.287Z), target>=500kbps=24109ms (2026-04-28T04:26:27.771Z), target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=23113ms (2026-04-28T04:26:26.775Z), send>=500kbps=23113ms (2026-04-28T04:26:26.775Z), send>=700kbps=26117ms (2026-04-28T04:26:29.779Z), send>=900kbps=28109ms (2026-04-28T04:26:31.771Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=20055ms, firstAction=20055ms, L0=-, L1=-, L2=-, L3=-, L4=20055ms, audioOnly=-；recovery: warning=29109ms, recovering=22110ms, stable=24686ms, congested=116ms, firstAction=116ms, L0=26609ms, L1=25109ms, L2=23609ms, L3=22110ms, L4=116ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 11 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-28T04:28:46.121Z；firstRecovering=19777ms (2026-04-28T04:29:05.898Z)；firstStable=22277ms (2026-04-28T04:29:08.398Z)；firstL0=24278ms (2026-04-28T04:29:10.399Z) |
| 恢复诊断 | raw=loss<3%=277ms (2026-04-28T04:28:46.398Z), rtt<120ms=277ms (2026-04-28T04:28:46.398Z), jitter<28ms=19283ms (2026-04-28T04:29:05.404Z), jitter<18ms=19777ms (2026-04-28T04:29:05.898Z)；target=target>=120kbps=13278ms (2026-04-28T04:28:59.399Z), target>=300kbps=20277ms (2026-04-28T04:29:06.398Z), target>=500kbps=21780ms (2026-04-28T04:29:07.901Z), target>=700kbps=23277ms (2026-04-28T04:29:09.398Z), target>=900kbps=24777ms (2026-04-28T04:29:10.898Z)；send=send>=300kbps=20777ms (2026-04-28T04:29:06.898Z), send>=500kbps=21780ms (2026-04-28T04:29:07.901Z), send>=700kbps=22277ms (2026-04-28T04:29:08.398Z), send>=900kbps=23277ms (2026-04-28T04:29:09.398Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=18610ms, stable=19110ms, congested=15610ms, firstAction=15610ms, L0=-, L1=21610ms, L2=20118ms, L3=18610ms, L4=15610ms, audioOnly=-；recovery: warning=-, recovering=19777ms, stable=22277ms, congested=277ms, firstAction=277ms, L0=24278ms, L1=22777ms, L2=21277ms, L3=19777ms, L4=277ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 6 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-28T04:31:51.267Z；firstRecovering=19054ms (2026-04-28T04:32:10.321Z)；firstStable=21551ms (2026-04-28T04:32:12.818Z)；firstL0=23051ms (2026-04-28T04:32:14.318Z) |
| 恢复诊断 | raw=loss<3%=51ms (2026-04-28T04:31:51.318Z), rtt<120ms=551ms (2026-04-28T04:31:51.818Z), jitter<28ms=18553ms (2026-04-28T04:32:09.820Z), jitter<18ms=18553ms (2026-04-28T04:32:09.820Z)；target=target>=120kbps=13051ms (2026-04-28T04:32:04.318Z), target>=300kbps=19551ms (2026-04-28T04:32:10.818Z), target>=500kbps=21051ms (2026-04-28T04:32:12.318Z), target>=700kbps=23551ms (2026-04-28T04:32:14.818Z), target>=900kbps=-；send=send>=300kbps=19551ms (2026-04-28T04:32:10.818Z), send>=500kbps=21051ms (2026-04-28T04:32:12.318Z), send>=700kbps=23551ms (2026-04-28T04:32:14.818Z), send>=900kbps=25051ms (2026-04-28T04:32:16.318Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=37771ms, firstAction=37771ms, L0=-, L1=-, L2=-, L3=-, L4=37771ms, audioOnly=-；recovery: warning=25551ms, recovering=19054ms, stable=21551ms, congested=51ms, firstAction=51ms, L0=23051ms, L1=21551ms, L2=20551ms, L3=19054ms, L4=51ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:32:45.210Z；firstRecovering=-；firstStable=14140ms (2026-04-28T04:32:59.350Z)；firstL0=14140ms (2026-04-28T04:32:59.350Z) |
| 恢复诊断 | raw=loss<3%=134ms (2026-04-28T04:32:45.344Z), rtt<120ms=134ms (2026-04-28T04:32:45.344Z), jitter<28ms=634ms (2026-04-28T04:32:45.844Z), jitter<18ms=12634ms (2026-04-28T04:32:57.844Z)；target=target>=120kbps=134ms (2026-04-28T04:32:45.344Z), target>=300kbps=134ms (2026-04-28T04:32:45.344Z), target>=500kbps=134ms (2026-04-28T04:32:45.344Z), target>=700kbps=11634ms (2026-04-28T04:32:56.844Z), target>=900kbps=14634ms (2026-04-28T04:32:59.844Z)；send=send>=300kbps=134ms (2026-04-28T04:32:45.344Z), send>=500kbps=134ms (2026-04-28T04:32:45.344Z), send>=700kbps=134ms (2026-04-28T04:32:45.344Z), send>=900kbps=134ms (2026-04-28T04:32:45.344Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3762ms, recovering=-, stable=262ms, congested=-, firstAction=3762ms, L0=-, L1=3762ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=134ms, recovering=-, stable=14140ms, congested=-, firstAction=14140ms, L0=14140ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 12 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-28T04:33:48.627Z；firstRecovering=15817ms (2026-04-28T04:34:04.444Z)；firstStable=18317ms (2026-04-28T04:34:06.944Z)；firstL0=20317ms (2026-04-28T04:34:08.944Z) |
| 恢复诊断 | raw=loss<3%=317ms (2026-04-28T04:33:48.944Z), rtt<120ms=1318ms (2026-04-28T04:33:49.945Z), jitter<28ms=317ms (2026-04-28T04:33:48.944Z), jitter<18ms=317ms (2026-04-28T04:33:48.944Z)；target=target>=120kbps=317ms (2026-04-28T04:33:48.944Z), target>=300kbps=16317ms (2026-04-28T04:34:04.944Z), target>=500kbps=17818ms (2026-04-28T04:34:06.445Z), target>=700kbps=20817ms (2026-04-28T04:34:09.444Z), target>=900kbps=-；send=send>=300kbps=16317ms (2026-04-28T04:34:04.944Z), send>=500kbps=17818ms (2026-04-28T04:34:06.445Z), send>=700kbps=18317ms (2026-04-28T04:34:06.944Z), send>=900kbps=19818ms (2026-04-28T04:34:08.445Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1577ms, recovering=-, stable=77ms, congested=2077ms, firstAction=1577ms, L0=-, L1=1577ms, L2=2077ms, L3=2578ms, L4=3077ms, audioOnly=-；recovery: warning=22825ms, recovering=15817ms, stable=18317ms, congested=317ms, firstAction=317ms, L0=20317ms, L1=18818ms, L2=17318ms, L3=15817ms, L4=317ms, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:34:44.991Z；firstRecovering=-；firstStable=14801ms (2026-04-28T04:34:59.792Z)；firstL0=14801ms (2026-04-28T04:34:59.792Z) |
| 恢复诊断 | raw=loss<3%=801ms (2026-04-28T04:34:45.792Z), rtt<120ms=1302ms (2026-04-28T04:34:46.293Z), jitter<28ms=5300ms (2026-04-28T04:34:50.291Z), jitter<18ms=13300ms (2026-04-28T04:34:58.291Z)；target=target>=120kbps=313ms (2026-04-28T04:34:45.304Z), target>=300kbps=313ms (2026-04-28T04:34:45.304Z), target>=500kbps=313ms (2026-04-28T04:34:45.304Z), target>=700kbps=4800ms (2026-04-28T04:34:49.791Z), target>=900kbps=15300ms (2026-04-28T04:35:00.291Z)；send=send>=300kbps=313ms (2026-04-28T04:34:45.304Z), send>=500kbps=313ms (2026-04-28T04:34:45.304Z), send>=700kbps=313ms (2026-04-28T04:34:45.304Z), send>=900kbps=3802ms (2026-04-28T04:34:48.793Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1766ms, recovering=-, stable=196ms, congested=-, firstAction=1766ms, L0=-, L1=1766ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=313ms, recovering=-, stable=14801ms, congested=-, firstAction=14801ms, L0=14801ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-28T04:35:39.156Z；firstRecovering=-；firstStable=15063ms (2026-04-28T04:35:54.219Z)；firstL0=15063ms (2026-04-28T04:35:54.219Z) |
| 恢复诊断 | raw=loss<3%=64ms (2026-04-28T04:35:39.220Z), rtt<120ms=562ms (2026-04-28T04:35:39.718Z), jitter<28ms=2072ms (2026-04-28T04:35:41.228Z), jitter<18ms=4572ms (2026-04-28T04:35:43.728Z)；target=target>=120kbps=64ms (2026-04-28T04:35:39.220Z), target>=300kbps=64ms (2026-04-28T04:35:39.220Z), target>=500kbps=64ms (2026-04-28T04:35:39.220Z), target>=700kbps=1562ms (2026-04-28T04:35:40.718Z), target>=900kbps=15565ms (2026-04-28T04:35:54.721Z)；send=send>=300kbps=64ms (2026-04-28T04:35:39.220Z), send>=500kbps=64ms (2026-04-28T04:35:39.220Z), send>=700kbps=1062ms (2026-04-28T04:35:40.218Z), send>=900kbps=8564ms (2026-04-28T04:35:47.720Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1815ms, recovering=-, stable=339ms, congested=-, firstAction=1815ms, L0=-, L1=1815ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=64ms, recovering=-, stable=15063ms, congested=-, firstAction=15063ms, L0=15063ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### M1

| 字段 | 内容 |
|---|---|
| Case ID | `M1` |
| 前置 Case | [B1](#b1) |
| 类型 | `traffic_model` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 3% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 0ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L0 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, maxActionCountPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=early_warning/L1, current=early_warning/L1) |
| 实际动作 | setEncodingParameters（共 1 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"state":"stable","maxLevel":0}；实际 impairment 评估值=early_warning/L1，recovery 评估值=early_warning/L1；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, maxActionCountPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-28T04:36:33.316Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=609ms (2026-04-28T04:36:33.925Z), rtt<120ms=109ms (2026-04-28T04:36:33.425Z), jitter<28ms=109ms (2026-04-28T04:36:33.425Z), jitter<18ms=109ms (2026-04-28T04:36:33.425Z)；target=target>=120kbps=109ms (2026-04-28T04:36:33.425Z), target>=300kbps=109ms (2026-04-28T04:36:33.425Z), target>=500kbps=109ms (2026-04-28T04:36:33.425Z), target>=700kbps=109ms (2026-04-28T04:36:33.425Z), target>=900kbps=-；send=send>=300kbps=109ms (2026-04-28T04:36:33.425Z), send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3807ms, recovering=-, stable=309ms, congested=-, firstAction=3807ms, L0=-, L1=3807ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=109ms, recovering=-, stable=-, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### M2

| 字段 | 内容 |
|---|---|
| Case ID | `M2` |
| 前置 Case | [B1](#b1) |
| 类型 | `traffic_model` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 0ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters, resumeUpstream, pauseUpstream（共 5 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-28T04:36:58.888Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=490ms (2026-04-28T04:36:59.378Z), rtt<120ms=490ms (2026-04-28T04:36:59.378Z), jitter<28ms=-, jitter<18ms=-；target=target>=120kbps=-, target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=-, send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3734ms, recovering=-, stable=234ms, congested=6234ms, firstAction=3734ms, L0=-, L1=3734ms, L2=6234ms, L3=6734ms, L4=7238ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=490ms, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### M3

| 字段 | 内容 |
|---|---|
| Case ID | `M3` |
| 前置 Case | [B1](#b1) |
| 类型 | `traffic_model` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 400ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 0ms |
| 预期 QoS 状态 | stable / early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 stable / early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-28T04:37:25.282Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=172ms (2026-04-28T04:37:25.454Z), rtt<120ms=-, jitter<28ms=-, jitter<18ms=-；target=target>=120kbps=-, target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=-, send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2564ms, recovering=-, stable=64ms, congested=3066ms, firstAction=2564ms, L0=-, L1=2564ms, L2=3066ms, L3=3564ms, L4=4064ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=172ms, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### O1

| 字段 | 内容 |
|---|---|
| Case ID | `O1` |
| 前置 Case | [B1](#b1) |
| 类型 | `oscillation` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 10000ms |
| 预期 QoS 状态 | stable / early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 stable / early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-28T04:37:55.806Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=96ms (2026-04-28T04:37:55.902Z), rtt<120ms=2596ms (2026-04-28T04:37:58.402Z), jitter<28ms=-, jitter<18ms=-；target=target>=120kbps=96ms (2026-04-28T04:37:55.902Z), target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=597ms (2026-04-28T04:37:56.403Z), send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3794ms, recovering=-, stable=294ms, congested=5794ms, firstAction=3794ms, L0=-, L1=3794ms, L2=5794ms, L3=6294ms, L4=6794ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=96ms, firstAction=96ms, L0=-, L1=-, L2=-, L3=-, L4=96ms, audioOnly=- |

### O2

| 字段 | 内容 |
|---|---|
| Case ID | `O2` |
| 前置 Case | [B1](#b1) |
| 类型 | `oscillation` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 150ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 5000ms / impairment 15000ms / recovery 0ms |
| 预期 QoS 状态 | stable / early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 stable / early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 恢复里程碑 | start=2026-04-28T04:38:30.327Z；firstRecovering=-；firstStable=-；firstL0=- |
| 恢复诊断 | raw=loss<3%=164ms (2026-04-28T04:38:30.491Z), rtt<120ms=-, jitter<28ms=-, jitter<18ms=-；target=target>=120kbps=164ms (2026-04-28T04:38:30.491Z), target>=300kbps=-, target>=500kbps=-, target>=700kbps=-, target>=900kbps=-；send=send>=300kbps=-, send>=500kbps=-, send>=700kbps=-, send>=900kbps=-；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2694ms, recovering=-, stable=194ms, congested=3694ms, firstAction=2694ms, L0=-, L1=2694ms, L2=3694ms, L3=4194ms, L4=4694ms, audioOnly=-；recovery: warning=-, recovering=-, stable=-, congested=164ms, firstAction=164ms, L0=-, L1=-, L2=-, L3=-, L4=164ms, audioOnly=- |

