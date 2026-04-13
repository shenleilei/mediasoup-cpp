# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-13T00:57:28.724Z`

## 1. 汇总

- 总 Case：`5`
- 已执行：`5`
- 通过：`5`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
- jitter_sweep：[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T10](#t10)、[T11](#t11)

## 3. 逐 Case 结果

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
| 恢复里程碑 | start=2026-04-13T00:48:59.639Z；firstRecovering=-；firstStable=6151ms (2026-04-13T00:49:05.790Z)；firstL0=6151ms (2026-04-13T00:49:05.790Z) |
| 恢复诊断 | raw=loss<3%=151ms (2026-04-13T00:48:59.790Z), rtt<120ms=151ms (2026-04-13T00:48:59.790Z), jitter<28ms=651ms (2026-04-13T00:49:00.290Z), jitter<18ms=2151ms (2026-04-13T00:49:01.790Z)；target=target>=120kbps=151ms (2026-04-13T00:48:59.790Z), target>=300kbps=151ms (2026-04-13T00:48:59.790Z), target>=500kbps=151ms (2026-04-13T00:48:59.790Z), target>=700kbps=651ms (2026-04-13T00:49:00.290Z), target>=900kbps=7151ms (2026-04-13T00:49:06.790Z)；send=send>=300kbps=151ms (2026-04-13T00:48:59.790Z), send>=500kbps=151ms (2026-04-13T00:48:59.790Z), send>=700kbps=151ms (2026-04-13T00:48:59.790Z), send>=900kbps=6651ms (2026-04-13T00:49:06.290Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2367ms, recovering=-, stable=367ms, congested=-, firstAction=2367ms, L0=-, L1=2367ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=151ms, recovering=-, stable=6151ms, congested=-, firstAction=6151ms, L0=6151ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:50:08.608Z；firstRecovering=-；firstStable=14597ms (2026-04-13T00:50:23.205Z)；firstL0=14597ms (2026-04-13T00:50:23.205Z) |
| 恢复诊断 | raw=loss<3%=97ms (2026-04-13T00:50:08.705Z), rtt<120ms=97ms (2026-04-13T00:50:08.705Z), jitter<28ms=1097ms (2026-04-13T00:50:09.705Z), jitter<18ms=1097ms (2026-04-13T00:50:09.705Z)；target=target>=120kbps=97ms (2026-04-13T00:50:08.705Z), target>=300kbps=97ms (2026-04-13T00:50:08.705Z), target>=500kbps=97ms (2026-04-13T00:50:08.705Z), target>=700kbps=97ms (2026-04-13T00:50:08.705Z), target>=900kbps=15097ms (2026-04-13T00:50:23.705Z)；send=send>=300kbps=97ms (2026-04-13T00:50:08.705Z), send>=500kbps=97ms (2026-04-13T00:50:08.705Z), send>=700kbps=597ms (2026-04-13T00:50:09.205Z), send>=900kbps=1597ms (2026-04-13T00:50:10.205Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=2365ms, recovering=-, stable=365ms, congested=-, firstAction=2365ms, L0=-, L1=2365ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=97ms, recovering=-, stable=14597ms, congested=-, firstAction=14597ms, L0=14597ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 72 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:51:17.906Z；firstRecovering=14680ms (2026-04-13T00:51:32.586Z)；firstStable=17180ms (2026-04-13T00:51:35.086Z)；firstL0=19180ms (2026-04-13T00:51:37.086Z) |
| 恢复诊断 | raw=loss<3%=180ms (2026-04-13T00:51:18.086Z), rtt<120ms=1180ms (2026-04-13T00:51:19.086Z), jitter<28ms=14180ms (2026-04-13T00:51:32.086Z), jitter<18ms=19680ms (2026-04-13T00:51:37.586Z)；target=target>=120kbps=180ms (2026-04-13T00:51:18.086Z), target>=300kbps=15180ms (2026-04-13T00:51:33.086Z), target>=500kbps=16680ms (2026-04-13T00:51:34.586Z), target>=700kbps=18180ms (2026-04-13T00:51:36.086Z), target>=900kbps=19680ms (2026-04-13T00:51:37.586Z)；send=send>=300kbps=15180ms (2026-04-13T00:51:33.086Z), send>=500kbps=16680ms (2026-04-13T00:51:34.586Z), send>=700kbps=16680ms (2026-04-13T00:51:34.586Z), send>=900kbps=18180ms (2026-04-13T00:51:36.086Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1362ms, recovering=-, stable=362ms, congested=1862ms, firstAction=1362ms, L0=-, L1=1362ms, L2=1862ms, L3=2362ms, L4=2862ms, audioOnly=-；recovery: warning=-, recovering=14680ms, stable=17180ms, congested=180ms, firstAction=180ms, L0=19180ms, L1=17680ms, L2=16180ms, L3=14680ms, L4=180ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 241 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:54:12.289Z；firstRecovering=20601ms (2026-04-13T00:54:32.890Z)；firstStable=23101ms (2026-04-13T00:54:35.390Z)；firstL0=24601ms (2026-04-13T00:54:36.890Z) |
| 恢复诊断 | raw=loss<3%=101ms (2026-04-13T00:54:12.390Z), rtt<120ms=1101ms (2026-04-13T00:54:13.390Z), jitter<28ms=20101ms (2026-04-13T00:54:32.390Z), jitter<18ms=20101ms (2026-04-13T00:54:32.390Z)；target=target>=120kbps=13601ms (2026-04-13T00:54:25.890Z), target>=300kbps=21101ms (2026-04-13T00:54:33.390Z), target>=500kbps=22601ms (2026-04-13T00:54:34.890Z), target>=700kbps=23601ms (2026-04-13T00:54:35.890Z), target>=900kbps=25101ms (2026-04-13T00:54:37.390Z)；send=send>=300kbps=21101ms (2026-04-13T00:54:33.390Z), send>=500kbps=22601ms (2026-04-13T00:54:34.890Z), send>=700kbps=23601ms (2026-04-13T00:54:35.890Z), send>=900kbps=23601ms (2026-04-13T00:54:35.890Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=27531ms, firstAction=27531ms, L0=-, L1=-, L2=-, L3=-, L4=27531ms, audioOnly=-；recovery: warning=-, recovering=20601ms, stable=23101ms, congested=101ms, firstAction=101ms, L0=24601ms, L1=23101ms, L2=22101ms, L3=20601ms, L4=101ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 238 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:56:58.570Z；firstRecovering=18155ms (2026-04-13T00:57:16.725Z)；firstStable=20655ms (2026-04-13T00:57:19.225Z)；firstL0=21655ms (2026-04-13T00:57:20.225Z) |
| 恢复诊断 | raw=loss<3%=155ms (2026-04-13T00:56:58.725Z), rtt<120ms=2155ms (2026-04-13T00:57:00.725Z), jitter<28ms=17655ms (2026-04-13T00:57:16.225Z), jitter<18ms=17655ms (2026-04-13T00:57:16.225Z)；target=target>=120kbps=11655ms (2026-04-13T00:57:10.225Z), target>=300kbps=19155ms (2026-04-13T00:57:17.725Z), target>=500kbps=20155ms (2026-04-13T00:57:18.725Z), target>=700kbps=23155ms (2026-04-13T00:57:21.725Z), target>=900kbps=29155ms (2026-04-13T00:57:27.725Z)；send=send>=300kbps=19155ms (2026-04-13T00:57:17.725Z), send>=500kbps=19155ms (2026-04-13T00:57:17.725Z), send>=700kbps=21655ms (2026-04-13T00:57:20.225Z), send>=900kbps=29655ms (2026-04-13T00:57:28.225Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=19664ms, firstAction=19664ms, L0=-, L1=-, L2=-, L3=-, L4=19664ms, audioOnly=-；recovery: warning=24155ms, recovering=18155ms, stable=20655ms, congested=155ms, firstAction=155ms, L0=21655ms, L1=20655ms, L2=19655ms, L3=18155ms, L4=155ms, audioOnly=- |

