# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-13T00:31:10.077Z`

## 1. 汇总

- 总 Case：`9`
- 已执行：`9`
- 通过：`9`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
- bw_sweep：[BW2](#bw2)
- jitter_sweep：[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T9](#t9)、[T10](#t10)、[T11](#t11)
- burst：[S4](#s4)

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:16:46.479Z；firstRecovering=-；firstStable=15579ms (2026-04-13T00:17:02.058Z)；firstL0=15579ms (2026-04-13T00:17:02.058Z) |
| 恢复诊断 | raw=loss<3%=79ms (2026-04-13T00:16:46.558Z), rtt<120ms=79ms (2026-04-13T00:16:46.558Z), jitter<28ms=79ms (2026-04-13T00:16:46.558Z), jitter<18ms=13079ms (2026-04-13T00:16:59.558Z)；target=target>=120kbps=79ms (2026-04-13T00:16:46.558Z), target>=300kbps=79ms (2026-04-13T00:16:46.558Z), target>=500kbps=79ms (2026-04-13T00:16:46.558Z), target>=700kbps=79ms (2026-04-13T00:16:46.558Z), target>=900kbps=16079ms (2026-04-13T00:17:02.558Z)；send=send>=300kbps=79ms (2026-04-13T00:16:46.558Z), send>=500kbps=79ms (2026-04-13T00:16:46.558Z), send>=700kbps=79ms (2026-04-13T00:16:46.558Z), send>=900kbps=579ms (2026-04-13T00:16:47.058Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=3342ms, recovering=-, stable=342ms, congested=-, firstAction=3342ms, L0=-, L1=3342ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=79ms, recovering=-, stable=15579ms, congested=-, firstAction=15579ms, L0=15579ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:17:55.398Z；firstRecovering=-；firstStable=13593ms (2026-04-13T00:18:08.991Z)；firstL0=13593ms (2026-04-13T00:18:08.991Z) |
| 恢复诊断 | raw=loss<3%=594ms (2026-04-13T00:17:55.992Z), rtt<120ms=95ms (2026-04-13T00:17:55.493Z), jitter<28ms=2094ms (2026-04-13T00:17:57.492Z), jitter<18ms=3094ms (2026-04-13T00:17:58.492Z)；target=target>=120kbps=95ms (2026-04-13T00:17:55.493Z), target>=300kbps=95ms (2026-04-13T00:17:55.493Z), target>=500kbps=95ms (2026-04-13T00:17:55.493Z), target>=700kbps=2594ms (2026-04-13T00:17:57.992Z), target>=900kbps=14093ms (2026-04-13T00:18:09.491Z)；send=send>=300kbps=95ms (2026-04-13T00:17:55.493Z), send>=500kbps=95ms (2026-04-13T00:17:55.493Z), send>=700kbps=95ms (2026-04-13T00:17:55.493Z), send>=900kbps=95ms (2026-04-13T00:17:55.493Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1876ms, recovering=-, stable=376ms, congested=-, firstAction=1876ms, L0=-, L1=1876ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=95ms, recovering=-, stable=13593ms, congested=-, firstAction=13593ms, L0=13593ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 63 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:19:04.324Z；firstRecovering=2056ms (2026-04-13T00:19:06.380Z)；firstStable=4556ms (2026-04-13T00:19:08.880Z)；firstL0=6556ms (2026-04-13T00:19:10.880Z) |
| 恢复诊断 | raw=loss<3%=57ms (2026-04-13T00:19:04.381Z), rtt<120ms=1556ms (2026-04-13T00:19:05.880Z), jitter<28ms=1556ms (2026-04-13T00:19:05.880Z), jitter<18ms=14556ms (2026-04-13T00:19:18.880Z)；target=target>=120kbps=57ms (2026-04-13T00:19:04.381Z), target>=300kbps=2556ms (2026-04-13T00:19:06.880Z), target>=500kbps=4556ms (2026-04-13T00:19:08.880Z), target>=700kbps=8556ms (2026-04-13T00:19:12.880Z), target>=900kbps=23056ms (2026-04-13T00:19:27.380Z)；send=send>=300kbps=2556ms (2026-04-13T00:19:06.880Z), send>=500kbps=2556ms (2026-04-13T00:19:06.880Z), send>=700kbps=2556ms (2026-04-13T00:19:06.880Z), send>=900kbps=5557ms (2026-04-13T00:19:09.881Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1832ms, recovering=-, stable=331ms, congested=5331ms, firstAction=1832ms, L0=-, L1=1832ms, L2=5331ms, L3=5831ms, L4=6331ms, audioOnly=-；recovery: warning=9056ms, recovering=2056ms, stable=4556ms, congested=57ms, firstAction=57ms, L0=6556ms, L1=5056ms, L2=3556ms, L3=2056ms, L4=57ms, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:20:13.258Z；firstRecovering=3561ms (2026-04-13T00:20:16.819Z)；firstStable=6062ms (2026-04-13T00:20:19.320Z)；firstL0=8065ms (2026-04-13T00:20:21.323Z) |
| 恢复诊断 | raw=loss<3%=62ms (2026-04-13T00:20:13.320Z), rtt<120ms=1562ms (2026-04-13T00:20:14.820Z), jitter<28ms=3062ms (2026-04-13T00:20:16.320Z), jitter<18ms=10062ms (2026-04-13T00:20:23.320Z)；target=target>=120kbps=62ms (2026-04-13T00:20:13.320Z), target>=300kbps=4062ms (2026-04-13T00:20:17.320Z), target>=500kbps=5562ms (2026-04-13T00:20:18.820Z), target>=700kbps=7062ms (2026-04-13T00:20:20.320Z), target>=900kbps=9061ms (2026-04-13T00:20:22.319Z)；send=send>=300kbps=4062ms (2026-04-13T00:20:17.320Z), send>=500kbps=4062ms (2026-04-13T00:20:17.320Z), send>=700kbps=5562ms (2026-04-13T00:20:18.820Z), send>=900kbps=5562ms (2026-04-13T00:20:18.820Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1839ms, recovering=-, stable=339ms, congested=2339ms, firstAction=1839ms, L0=-, L1=1839ms, L2=2339ms, L3=2839ms, L4=3339ms, audioOnly=-；recovery: warning=10562ms, recovering=3561ms, stable=6062ms, congested=62ms, firstAction=62ms, L0=8065ms, L1=6562ms, L2=5062ms, L3=3561ms, L4=62ms, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:21:22.186Z；firstRecovering=-；firstStable=71ms (2026-04-13T00:21:22.257Z)；firstL0=- |
| 恢复诊断 | raw=loss<3%=71ms (2026-04-13T00:21:22.257Z), rtt<120ms=71ms (2026-04-13T00:21:22.257Z), jitter<28ms=71ms (2026-04-13T00:21:22.257Z), jitter<18ms=1071ms (2026-04-13T00:21:23.257Z)；target=target>=120kbps=71ms (2026-04-13T00:21:22.257Z), target>=300kbps=71ms (2026-04-13T00:21:22.257Z), target>=500kbps=71ms (2026-04-13T00:21:22.257Z), target>=700kbps=71ms (2026-04-13T00:21:22.257Z), target>=900kbps=71ms (2026-04-13T00:21:22.257Z)；send=send>=300kbps=71ms (2026-04-13T00:21:22.257Z), send>=500kbps=71ms (2026-04-13T00:21:22.257Z), send>=700kbps=71ms (2026-04-13T00:21:22.257Z), send>=900kbps=571ms (2026-04-13T00:21:22.757Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=351ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, recovering=-, stable=71ms, congested=-, firstAction=-, L0=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 恢复里程碑 | start=2026-04-13T00:24:05.629Z；firstRecovering=20049ms (2026-04-13T00:24:25.678Z)；firstStable=22549ms (2026-04-13T00:24:28.178Z)；firstL0=24549ms (2026-04-13T00:24:30.178Z) |
| 恢复诊断 | raw=loss<3%=49ms (2026-04-13T00:24:05.678Z), rtt<120ms=1049ms (2026-04-13T00:24:06.678Z), jitter<28ms=19549ms (2026-04-13T00:24:25.178Z), jitter<18ms=19549ms (2026-04-13T00:24:25.178Z)；target=target>=120kbps=14049ms (2026-04-13T00:24:19.678Z), target>=300kbps=20549ms (2026-04-13T00:24:26.178Z), target>=500kbps=22049ms (2026-04-13T00:24:27.678Z), target>=700kbps=23549ms (2026-04-13T00:24:29.178Z), target>=900kbps=25049ms (2026-04-13T00:24:30.678Z)；send=send>=300kbps=20549ms (2026-04-13T00:24:26.178Z), send>=500kbps=22049ms (2026-04-13T00:24:27.678Z), send>=700kbps=22549ms (2026-04-13T00:24:28.178Z), send>=900kbps=23549ms (2026-04-13T00:24:29.178Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=17126ms, firstAction=17126ms, L0=-, L1=-, L2=-, L3=-, L4=17126ms, audioOnly=-；recovery: warning=-, recovering=20049ms, stable=22549ms, congested=49ms, firstAction=49ms, L0=24549ms, L1=23049ms, L2=21549ms, L3=20049ms, L4=49ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 240 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-13T00:26:48.238Z；firstRecovering=19872ms (2026-04-13T00:27:08.110Z)；firstStable=22372ms (2026-04-13T00:27:10.610Z)；firstL0=24372ms (2026-04-13T00:27:12.610Z) |
| 恢复诊断 | raw=loss<3%=372ms (2026-04-13T00:26:48.610Z), rtt<120ms=1372ms (2026-04-13T00:26:49.610Z), jitter<28ms=19372ms (2026-04-13T00:27:07.610Z), jitter<18ms=19372ms (2026-04-13T00:27:07.610Z)；target=target>=120kbps=14372ms (2026-04-13T00:27:02.610Z), target>=300kbps=20372ms (2026-04-13T00:27:08.610Z), target>=500kbps=21872ms (2026-04-13T00:27:10.110Z), target>=700kbps=23872ms (2026-04-13T00:27:12.110Z), target>=900kbps=26872ms (2026-04-13T00:27:15.110Z)；send=send>=300kbps=20872ms (2026-04-13T00:27:09.110Z), send>=500kbps=21872ms (2026-04-13T00:27:10.110Z), send>=700kbps=23372ms (2026-04-13T00:27:11.610Z), send>=900kbps=25872ms (2026-04-13T00:27:14.110Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=16180ms, firstAction=16180ms, L0=-, L1=-, L2=-, L3=-, L4=16180ms, audioOnly=-；recovery: warning=26872ms, recovering=19872ms, stable=22372ms, congested=372ms, firstAction=372ms, L0=24372ms, L1=22872ms, L2=21372ms, L3=19872ms, L4=372ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 239 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=early_warning/L1，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-13T00:29:45.603Z；firstRecovering=19061ms (2026-04-13T00:30:04.664Z)；firstStable=21561ms (2026-04-13T00:30:07.164Z)；firstL0=23561ms (2026-04-13T00:30:09.164Z) |
| 恢复诊断 | raw=loss<3%=61ms (2026-04-13T00:29:45.664Z), rtt<120ms=561ms (2026-04-13T00:29:46.164Z), jitter<28ms=18561ms (2026-04-13T00:30:04.164Z), jitter<18ms=28061ms (2026-04-13T00:30:13.664Z)；target=target>=120kbps=13061ms (2026-04-13T00:29:58.664Z), target>=300kbps=19561ms (2026-04-13T00:30:05.164Z), target>=500kbps=21061ms (2026-04-13T00:30:06.664Z), target>=700kbps=23061ms (2026-04-13T00:30:08.664Z), target>=900kbps=-；send=send>=300kbps=20061ms (2026-04-13T00:30:05.664Z), send>=500kbps=21061ms (2026-04-13T00:30:06.664Z), send>=700kbps=22561ms (2026-04-13T00:30:08.164Z), send>=900kbps=25561ms (2026-04-13T00:30:11.164Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=29439ms, firstAction=29439ms, L0=-, L1=-, L2=-, L3=-, L4=29439ms, audioOnly=-；recovery: warning=26061ms, recovering=19061ms, stable=21561ms, congested=61ms, firstAction=61ms, L0=23561ms, L1=22061ms, L2=20561ms, L3=19061ms, L4=61ms, audioOnly=- |

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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 44 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:30:39.829Z；firstRecovering=9151ms (2026-04-13T00:30:48.980Z)；firstStable=11152ms (2026-04-13T00:30:50.981Z)；firstL0=13151ms (2026-04-13T00:30:52.980Z) |
| 恢复诊断 | raw=loss<3%=151ms (2026-04-13T00:30:39.980Z), rtt<120ms=151ms (2026-04-13T00:30:39.980Z), jitter<28ms=4652ms (2026-04-13T00:30:44.481Z), jitter<18ms=5652ms (2026-04-13T00:30:45.481Z)；target=target>=120kbps=151ms (2026-04-13T00:30:39.980Z), target>=300kbps=151ms (2026-04-13T00:30:39.980Z), target>=500kbps=11152ms (2026-04-13T00:30:50.981Z), target>=700kbps=13652ms (2026-04-13T00:30:53.481Z), target>=900kbps=28652ms (2026-04-13T00:31:08.481Z)；send=send>=300kbps=151ms (2026-04-13T00:30:39.980Z), send>=500kbps=652ms (2026-04-13T00:30:40.481Z), send>=700kbps=652ms (2026-04-13T00:30:40.481Z), send>=900kbps=13151ms (2026-04-13T00:30:52.980Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2251ms, recovering=-, stable=251ms, congested=-, firstAction=2251ms, L0=-, L1=2251ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=15652ms, recovering=9151ms, stable=11152ms, congested=151ms, firstAction=151ms, L0=13151ms, L1=11652ms, L2=151ms, L3=652ms, L4=1152ms, audioOnly=- |

