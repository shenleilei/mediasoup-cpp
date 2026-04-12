# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T06:05:12.487Z`

## 1. 汇总

- 总 Case：`41`
- 已执行：`41`
- 通过：`41`
- 失败：`0`
- 错误：`0`

## 2. 逐 Case 结果

### B1

| 字段 | 内容 |
|---|---|
| Case ID | `B1` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### B2

| 字段 | 内容 |
|---|---|
| Case ID | `B2` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P1` |
| 上行带宽 | 8000 kbps |
| RTT | 20 ms |
| 丢包率 | 0.1% |
| Jitter | 3 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L0 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### B3

| 字段 | 内容 |
|---|---|
| Case ID | `B3` |
| 前置 Case | - |
| 类型 | `baseline` / priority `P0` |
| 上行带宽 | 2000 kbps |
| RTT | 55 ms |
| 丢包率 | 0.5% |
| Jitter | 12 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 37 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=219ms, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=351ms, congested=4851ms, firstAction=4851ms, L1=21851ms, L2=4851ms, L3=5374ms, L4=5851ms, audioOnly=- |

### BW1

| 字段 | 内容 |
|---|---|
| Case ID | `BW1` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 3000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### BW2

| 字段 | 内容 |
|---|---|
| Case ID | `BW2` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 2000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2323ms, congested=-, firstAction=2323ms, L1=2323ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=39ms, congested=-, firstAction=14539ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### BW3

| 字段 | 内容 |
|---|---|
| Case ID | `BW3` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P0` |
| 上行带宽 | 1000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L2)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 74 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1340ms, congested=2340ms, firstAction=1340ms, L1=1340ms, L2=2340ms, L3=2840ms, L4=3340ms, audioOnly=-；recovery: warning=-, congested=881ms, firstAction=381ms, L1=17381ms, L2=16881ms, L3=16381ms, L4=881ms, audioOnly=- |

### BW4

| 字段 | 内容 |
|---|---|
| Case ID | `BW4` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 800 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 67 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2849ms, congested=3349ms, firstAction=2849ms, L1=2849ms, L2=3349ms, L3=3849ms, L4=4349ms, audioOnly=-；recovery: warning=2367ms, congested=2867ms, firstAction=367ms, L1=867ms, L2=367ms, L3=3367ms, L4=3867ms, audioOnly=- |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P0` |
| 上行带宽 | 500 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 93 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3868ms, congested=4368ms, firstAction=3868ms, L1=3868ms, L2=4368ms, L3=4868ms, L4=5368ms, audioOnly=-；recovery: warning=23123ms, congested=123ms, firstAction=123ms, L1=21623ms, L2=21123ms, L3=20623ms, L4=123ms, audioOnly=- |

### BW6

| 字段 | 内容 |
|---|---|
| Case ID | `BW6` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 300 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=recovering/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3833ms, congested=4333ms, firstAction=3833ms, L1=3833ms, L2=4333ms, L3=4833ms, L4=5333ms, audioOnly=-；recovery: warning=1349ms, congested=1849ms, firstAction=1349ms, L1=1349ms, L2=1849ms, L3=2349ms, L4=2849ms, audioOnly=- |

### BW7

| 字段 | 内容 |
|---|---|
| Case ID | `BW7` |
| 前置 Case | B1 |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 200 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 154 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=4314ms, congested=4814ms, firstAction=4314ms, L1=4314ms, L2=4814ms, L3=5314ms, L4=5814ms, audioOnly=-；recovery: warning=22893ms, congested=393ms, firstAction=393ms, L1=20392ms, L2=19892ms, L3=19392ms, L4=393ms, audioOnly=- |

### L1

| 字段 | 内容 |
|---|---|
| Case ID | `L1` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.5% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L2

| 字段 | 内容 |
|---|---|
| Case ID | `L2` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L3

| 字段 | 内容 |
|---|---|
| Case ID | `L3` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 2% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L4

| 字段 | 内容 |
|---|---|
| Case ID | `L4` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 5% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=6823ms, congested=-, firstAction=6823ms, L1=6823ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=166ms, congested=-, firstAction=2666ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 10% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 68 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2352ms, congested=3352ms, firstAction=2352ms, L1=2352ms, L2=3352ms, L3=3857ms, L4=4352ms, audioOnly=-；recovery: warning=7198ms, congested=198ms, firstAction=198ms, L1=5198ms, L2=4698ms, L3=4198ms, L4=198ms, audioOnly=- |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 20% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 60 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1883ms, congested=3383ms, firstAction=1883ms, L1=1883ms, L2=3383ms, L3=3884ms, L4=4383ms, audioOnly=-；recovery: warning=-, congested=229ms, firstAction=229ms, L1=11728ms, L2=11228ms, L3=10728ms, L4=229ms, audioOnly=- |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 40% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 80 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1371ms, congested=3371ms, firstAction=1371ms, L1=1371ms, L2=3371ms, L3=3871ms, L4=4371ms, audioOnly=-；recovery: warning=-, congested=226ms, firstAction=226ms, L1=21726ms, L2=21226ms, L3=20726ms, L4=226ms, audioOnly=- |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 前置 Case | B1 |
| 类型 | `loss_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 60% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 81 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3840ms, congested=4340ms, firstAction=3840ms, L1=3840ms, L2=4340ms, L3=4840ms, L4=5340ms, audioOnly=-；recovery: warning=-, congested=497ms, firstAction=497ms, L1=22997ms, L2=22497ms, L3=21997ms, L4=497ms, audioOnly=- |

### R1

| 字段 | 内容 |
|---|---|
| Case ID | `R1` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 50 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R2

| 字段 | 内容 |
|---|---|
| Case ID | `R2` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 80 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 120 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 180 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2789ms, congested=-, firstAction=2789ms, L1=2789ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=391ms, congested=-, firstAction=13891ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 250 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2246ms, congested=5246ms, firstAction=2246ms, L1=2246ms, L2=5246ms, L3=5746ms, L4=6246ms, audioOnly=-；recovery: warning=7756ms, congested=256ms, firstAction=256ms, L1=5756ms, L2=5256ms, L3=4756ms, L4=256ms, audioOnly=- |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 前置 Case | B1 |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 350 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 81 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2213ms, congested=2713ms, firstAction=2213ms, L1=2213ms, L2=2713ms, L3=3213ms, L4=3713ms, audioOnly=-；recovery: warning=7554ms, congested=54ms, firstAction=54ms, L1=6054ms, L2=5554ms, L3=5054ms, L4=54ms, audioOnly=- |

### J1

| 字段 | 内容 |
|---|---|
| Case ID | `J1` |
| 前置 Case | B1 |
| 类型 | `jitter_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 10 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable |
| 预期动作 | 应保持稳定，动作为 noop 或极轻微保护，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J2

| 字段 | 内容 |
|---|---|
| Case ID | `J2` |
| 前置 Case | B1 |
| 类型 | `jitter_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 20 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 前置 Case | B1 |
| 类型 | `jitter_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 40 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3360ms, congested=-, firstAction=3360ms, L1=3360ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=113ms, congested=-, firstAction=12113ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 前置 Case | B1 |
| 类型 | `jitter_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 60 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 70 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2367ms, congested=5367ms, firstAction=2367ms, L1=2367ms, L2=5367ms, L3=5867ms, L4=6367ms, audioOnly=-；recovery: warning=9594ms, congested=79ms, firstAction=79ms, L1=8079ms, L2=7579ms, L3=7079ms, L4=79ms, audioOnly=- |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 前置 Case | B1 |
| 类型 | `jitter_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 100 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 84 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1256ms, congested=1756ms, firstAction=1256ms, L1=1256ms, L2=1756ms, L3=2256ms, L4=2756ms, audioOnly=-；recovery: warning=11116ms, congested=116ms, firstAction=116ms, L1=9616ms, L2=9116ms, L3=8616ms, L4=116ms, audioOnly=- |

### T1

| 字段 | 内容 |
|---|---|
| Case ID | `T1` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 2000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=1572ms, congested=-, firstAction=1572ms, L1=1572ms, L2=-, L3=-, L4=-, audioOnly=- |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P0` |
| 上行带宽 | 1000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 57 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1862ms, congested=15862ms, firstAction=1862ms, L1=1862ms, L2=15862ms, L3=16362ms, L4=16862ms, audioOnly=-；recovery: warning=7935ms, congested=433ms, firstAction=433ms, L1=6932ms, L2=6433ms, L3=5933ms, L4=433ms, audioOnly=- |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 500 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 80 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2339ms, congested=2838ms, firstAction=2339ms, L1=2339ms, L2=2838ms, L3=3338ms, L4=3839ms, audioOnly=-；recovery: warning=-, congested=124ms, firstAction=124ms, L1=20623ms, L2=20123ms, L3=19623ms, L4=124ms, audioOnly=- |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 5% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=5140ms, congested=-, firstAction=5140ms, L1=5140ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=472ms, congested=-, firstAction=4472ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 20% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 59 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2855ms, congested=4355ms, firstAction=2855ms, L1=2855ms, L2=4355ms, L3=4855ms, L4=5355ms, audioOnly=-；recovery: warning=12713ms, congested=213ms, firstAction=213ms, L1=11213ms, L2=10713ms, L3=10213ms, L4=213ms, audioOnly=- |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 180 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3276ms, congested=-, firstAction=3276ms, L1=3276ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=370ms, congested=-, firstAction=14870ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 前置 Case | B1 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 40 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=5357ms, congested=-, firstAction=5357ms, L1=5357ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=205ms, congested=-, firstAction=6705ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 前置 Case | B3 |
| 类型 | `transition` / priority `P1` |
| 上行带宽 | 800 kbps |
| RTT | 120 ms |
| 丢包率 | 3% |
| Jitter | 30 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 允许持续降级，不要求恢复；最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=early_warning/L1)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 39 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 关键时间指标 | impairment: warning=138ms, congested=1638ms, firstAction=1638ms, L1=-, L2=1638ms, L3=2138ms, L4=2638ms, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 前置 Case | B1 |
| 类型 | `burst` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 10% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning / congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 early_warning / congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1865ms, congested=-, firstAction=1865ms, L1=1865ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=242ms, congested=-, firstAction=14742ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | B1 |
| 类型 | `burst` / priority `P1` |
| 上行带宽 | 300 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 93 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2834ms, congested=3334ms, firstAction=2834ms, L1=2834ms, L2=3334ms, L3=3834ms, L4=4334ms, audioOnly=-；recovery: warning=24791ms, congested=285ms, firstAction=285ms, L1=21788ms, L2=21288ms, L3=20788ms, L4=285ms, audioOnly=- |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 前置 Case | B1 |
| 类型 | `burst` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 200 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2272ms, congested=-, firstAction=2272ms, L1=2272ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=390ms, congested=-, firstAction=16390ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 前置 Case | B1 |
| 类型 | `burst` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 60 ms |
| 持续时间 | baseline 15000ms / impairment 5000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3358ms, congested=-, firstAction=3358ms, L1=3358ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=168ms, congested=-, firstAction=15668ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

