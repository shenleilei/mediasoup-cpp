# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-13T01:14:23.797Z`

## 1. 汇总

- 总 Case：`4`
- 已执行：`4`
- 通过：`3`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [BW2](#bw2) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[BW2](#bw2)
- bw_sweep：[BW2](#bw2)
- transition：[T9](#t9)、[T10](#t10)、[T11](#t11)

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 23 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["stable","early_warning"],"maxLevel":1}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 恢复里程碑 | start=2026-04-13T01:05:49.295Z；firstRecovering=-；firstStable=76ms (2026-04-13T01:05:49.371Z)；firstL0=76ms (2026-04-13T01:05:49.371Z) |
| 恢复诊断 | raw=loss<3%=76ms (2026-04-13T01:05:49.371Z), rtt<120ms=76ms (2026-04-13T01:05:49.371Z), jitter<28ms=76ms (2026-04-13T01:05:49.371Z), jitter<18ms=6576ms (2026-04-13T01:05:55.871Z)；target=target>=120kbps=76ms (2026-04-13T01:05:49.371Z), target>=300kbps=76ms (2026-04-13T01:05:49.371Z), target>=500kbps=76ms (2026-04-13T01:05:49.371Z), target>=700kbps=1576ms (2026-04-13T01:05:50.871Z), target>=900kbps=16076ms (2026-04-13T01:06:05.371Z)；send=send>=300kbps=76ms (2026-04-13T01:05:49.371Z), send>=500kbps=76ms (2026-04-13T01:05:49.371Z), send>=700kbps=76ms (2026-04-13T01:05:49.371Z), send>=900kbps=576ms (2026-04-13T01:05:49.871Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=1313ms, recovering=16813ms, stable=313ms, congested=8813ms, firstAction=1313ms, L0=-, L1=1313ms, L2=8813ms, L3=9313ms, L4=9813ms, audioOnly=-；recovery: warning=3076ms, recovering=-, stable=76ms, congested=-, firstAction=76ms, L0=76ms, L1=3076ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 233 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T01:08:36.655Z；firstRecovering=18116ms (2026-04-13T01:08:54.771Z)；firstStable=20616ms (2026-04-13T01:08:57.271Z)；firstL0=22618ms (2026-04-13T01:08:59.273Z) |
| 恢复诊断 | raw=loss<3%=116ms (2026-04-13T01:08:36.771Z), rtt<120ms=2616ms (2026-04-13T01:08:39.271Z), jitter<28ms=17616ms (2026-04-13T01:08:54.271Z), jitter<18ms=18116ms (2026-04-13T01:08:54.771Z)；target=target>=120kbps=13616ms (2026-04-13T01:08:50.271Z), target>=300kbps=18616ms (2026-04-13T01:08:55.271Z), target>=500kbps=20116ms (2026-04-13T01:08:56.771Z), target>=700kbps=21616ms (2026-04-13T01:08:58.271Z), target>=900kbps=23116ms (2026-04-13T01:08:59.771Z)；send=send>=300kbps=19116ms (2026-04-13T01:08:55.771Z), send>=500kbps=20116ms (2026-04-13T01:08:56.771Z), send>=700kbps=20616ms (2026-04-13T01:08:57.271Z), send>=900kbps=21616ms (2026-04-13T01:08:58.271Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=23543ms, stable=24043ms, congested=20543ms, firstAction=20543ms, L0=-, L1=-, L2=-, L3=23543ms, L4=20543ms, audioOnly=-；recovery: warning=-, recovering=18116ms, stable=20616ms, congested=116ms, firstAction=116ms, L0=22618ms, L1=21116ms, L2=19617ms, L3=18116ms, L4=116ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 230 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T01:11:14.754Z；firstRecovering=19871ms (2026-04-13T01:11:34.625Z)；firstStable=22371ms (2026-04-13T01:11:37.125Z)；firstL0=24371ms (2026-04-13T01:11:39.125Z) |
| 恢复诊断 | raw=loss<3%=371ms (2026-04-13T01:11:15.125Z), rtt<120ms=1372ms (2026-04-13T01:11:16.126Z), jitter<28ms=19371ms (2026-04-13T01:11:34.125Z), jitter<18ms=20371ms (2026-04-13T01:11:35.125Z)；target=target>=120kbps=12871ms (2026-04-13T01:11:27.625Z), target>=300kbps=20371ms (2026-04-13T01:11:35.125Z), target>=500kbps=21871ms (2026-04-13T01:11:36.625Z), target>=700kbps=23371ms (2026-04-13T01:11:38.125Z), target>=900kbps=24871ms (2026-04-13T01:11:39.625Z)；send=send>=300kbps=20872ms (2026-04-13T01:11:35.626Z), send>=500kbps=21871ms (2026-04-13T01:11:36.625Z), send>=700kbps=23371ms (2026-04-13T01:11:38.125Z), send>=900kbps=23371ms (2026-04-13T01:11:38.125Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=18760ms, stable=19261ms, congested=11761ms, firstAction=11761ms, L0=23260ms, L1=21761ms, L2=20261ms, L3=18760ms, L4=11761ms, audioOnly=-；recovery: warning=-, recovering=19871ms, stable=22371ms, congested=371ms, firstAction=371ms, L0=24371ms, L1=22871ms, L2=21372ms, L3=19871ms, L4=371ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 242 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T01:13:53.645Z；firstRecovering=23681ms (2026-04-13T01:14:17.326Z)；firstStable=26181ms (2026-04-13T01:14:19.826Z)；firstL0=27181ms (2026-04-13T01:14:20.826Z) |
| 恢复诊断 | raw=loss<3%=181ms (2026-04-13T01:13:53.826Z), rtt<120ms=1181ms (2026-04-13T01:13:54.826Z), jitter<28ms=23181ms (2026-04-13T01:14:16.826Z), jitter<18ms=23181ms (2026-04-13T01:14:16.826Z)；target=target>=120kbps=17181ms (2026-04-13T01:14:10.826Z), target>=300kbps=24181ms (2026-04-13T01:14:17.826Z), target>=500kbps=25681ms (2026-04-13T01:14:19.326Z), target>=700kbps=26681ms (2026-04-13T01:14:20.326Z), target>=900kbps=27682ms (2026-04-13T01:14:21.327Z)；send=send>=300kbps=24681ms (2026-04-13T01:14:18.326Z), send>=500kbps=25681ms (2026-04-13T01:14:19.326Z), send>=700kbps=26681ms (2026-04-13T01:14:20.326Z), send>=900kbps=27181ms (2026-04-13T01:14:20.826Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=18114ms, stable=18614ms, congested=12114ms, firstAction=12114ms, L0=-, L1=21114ms, L2=19614ms, L3=18114ms, L4=12114ms, audioOnly=-；recovery: warning=-, recovering=23681ms, stable=26181ms, congested=181ms, firstAction=181ms, L0=27181ms, L1=26181ms, L2=25181ms, L3=23681ms, L4=181ms, audioOnly=- |

