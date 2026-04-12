# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T02:03:54.534Z`

## 1. 汇总

- 总 Case：`41`
- 已执行：`41`
- 通过：`40`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| `B3` | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |

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
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=congested/L4)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 123 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["stable","early_warning"],"maxLevel":1}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 关键时间指标 | impairment: warning=45233ms, congested=233ms, firstAction=233ms, L1=42233ms, L2=41733ms, L3=41233ms, L4=233ms, audioOnly=-；recovery: warning=24886ms, congested=386ms, firstAction=386ms, L1=21886ms, L2=21386ms, L3=20886ms, L4=386ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=stable/L0, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=22321ms, congested=-, firstAction=22321ms, L1=22321ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=2049ms, congested=-, firstAction=2049ms, L1=2049ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 71 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1852ms, congested=2352ms, firstAction=1852ms, L1=1852ms, L2=2352ms, L3=2852ms, L4=3352ms, audioOnly=-；recovery: warning=-, congested=413ms, firstAction=413ms, L1=15913ms, L2=15415ms, L3=14913ms, L4=413ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 78 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2848ms, congested=3348ms, firstAction=2848ms, L1=2848ms, L2=3348ms, L3=3849ms, L4=4348ms, audioOnly=-；recovery: warning=-, congested=343ms, firstAction=343ms, L1=20343ms, L2=19843ms, L3=19343ms, L4=343ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 80 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1861ms, congested=2361ms, firstAction=1861ms, L1=1861ms, L2=2361ms, L3=2861ms, L4=3361ms, audioOnly=-；recovery: warning=21608ms, congested=108ms, firstAction=108ms, L1=19108ms, L2=18608ms, L3=18108ms, L4=108ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L3)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 92 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2353ms, congested=4353ms, firstAction=2353ms, L1=2353ms, L2=4353ms, L3=4853ms, L4=5353ms, audioOnly=-；recovery: warning=25761ms, congested=261ms, firstAction=261ms, L1=21761ms, L2=21261ms, L3=20761ms, L4=261ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 120 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2815ms, congested=3315ms, firstAction=2815ms, L1=2815ms, L2=3315ms, L3=3815ms, L4=4315ms, audioOnly=-；recovery: warning=23155ms, congested=156ms, firstAction=156ms, L1=20656ms, L2=20156ms, L3=19656ms, L4=156ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=2380ms, congested=-, firstAction=2380ms, L1=2380ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=235ms, congested=-, firstAction=12735ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 66 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2369ms, congested=4869ms, firstAction=2369ms, L1=2369ms, L2=4869ms, L3=5369ms, L4=5869ms, audioOnly=-；recovery: warning=7221ms, congested=221ms, firstAction=221ms, L1=5721ms, L2=5221ms, L3=4721ms, L4=221ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 73 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3366ms, congested=3867ms, firstAction=3366ms, L1=3366ms, L2=3867ms, L3=4369ms, L4=4866ms, audioOnly=-；recovery: warning=-, congested=190ms, firstAction=190ms, L1=18690ms, L2=18190ms, L3=17690ms, L4=190ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 81 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1119ms, congested=5119ms, firstAction=1119ms, L1=1119ms, L2=5119ms, L3=5619ms, L4=6119ms, audioOnly=-；recovery: warning=-, congested=319ms, firstAction=319ms, L1=22319ms, L2=21842ms, L3=21319ms, L4=319ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 94 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1139ms, congested=5139ms, firstAction=1139ms, L1=1139ms, L2=5139ms, L3=5639ms, L4=6139ms, audioOnly=-；recovery: warning=-, congested=107ms, firstAction=107ms, L1=23607ms, L2=23107ms, L3=22607ms, L4=107ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=3268ms, congested=-, firstAction=3268ms, L1=3268ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=382ms, congested=-, firstAction=14882ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 93 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1733ms, congested=3733ms, firstAction=1733ms, L1=1733ms, L2=3733ms, L3=4233ms, L4=4733ms, audioOnly=-；recovery: warning=6246ms, congested=246ms, firstAction=246ms, L1=5246ms, L2=4746ms, L3=4246ms, L4=246ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=1711ms, congested=2211ms, firstAction=1711ms, L1=1711ms, L2=2211ms, L3=2718ms, L4=3211ms, audioOnly=-；recovery: warning=6551ms, congested=52ms, firstAction=52ms, L1=5052ms, L2=4551ms, L3=4051ms, L4=52ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=2346ms, congested=-, firstAction=2346ms, L1=2346ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=481ms, congested=-, firstAction=15481ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 69 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1804ms, congested=7804ms, firstAction=1804ms, L1=1804ms, L2=7804ms, L3=8304ms, L4=8804ms, audioOnly=-；recovery: warning=8012ms, congested=12ms, firstAction=12ms, L1=7012ms, L2=6512ms, L3=6012ms, L4=12ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 50 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2172ms, congested=5171ms, firstAction=2172ms, L1=2172ms, L2=5171ms, L3=5671ms, L4=6172ms, audioOnly=-；recovery: warning=-, congested=285ms, firstAction=285ms, L1=8285ms, L2=7785ms, L3=7287ms, L4=285ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=21337ms, congested=-, firstAction=21337ms, L1=21337ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=1039ms, congested=-, firstAction=1039ms, L1=1039ms, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 82 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1875ms, congested=3375ms, firstAction=1875ms, L1=1875ms, L2=3375ms, L3=3875ms, L4=4375ms, audioOnly=-；recovery: warning=7492ms, congested=492ms, firstAction=492ms, L1=5992ms, L2=5492ms, L3=4992ms, L4=492ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 79 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2859ms, congested=3358ms, firstAction=2859ms, L1=2859ms, L2=3358ms, L3=3859ms, L4=4359ms, audioOnly=-；recovery: warning=21542ms, congested=42ms, firstAction=42ms, L1=19541ms, L2=19041ms, L3=18541ms, L4=42ms, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=5841ms, congested=-, firstAction=5841ms, L1=5841ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=184ms, congested=-, firstAction=2684ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际动作 | setEncodingParameters（共 62 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1335ms, congested=2334ms, firstAction=1335ms, L1=1335ms, L2=2334ms, L3=2834ms, L4=3334ms, audioOnly=-；recovery: warning=12178ms, congested=178ms, firstAction=178ms, L1=10678ms, L2=10179ms, L3=9678ms, L4=178ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=3293ms, congested=-, firstAction=3293ms, L1=3293ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=416ms, congested=-, firstAction=16916ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 关键时间指标 | impairment: warning=3777ms, congested=-, firstAction=3777ms, L1=3777ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=78ms, congested=-, firstAction=5578ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=congested/L4)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 67 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 关键时间指标 | impairment: warning=-, congested=216ms, firstAction=216ms, L1=-, L2=-, L3=-, L4=216ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L3, current=congested/L3)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 23 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L3，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2370ms, congested=4370ms, firstAction=2370ms, L1=2370ms, L2=4370ms, L3=4870ms, L4=5370ms, audioOnly=-；recovery: warning=10024ms, congested=24ms, firstAction=24ms, L1=8024ms, L2=7524ms, L3=7024ms, L4=24ms, audioOnly=- |

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 74 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=8354ms, congested=8854ms, firstAction=8354ms, L1=8354ms, L2=8854ms, L3=9354ms, L4=9854ms, audioOnly=-；recovery: warning=-, congested=253ms, firstAction=253ms, L1=21755ms, L2=21255ms, L3=20755ms, L4=253ms, audioOnly=- |

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
| 关键时间指标 | impairment: warning=2267ms, congested=-, firstAction=2267ms, L1=2267ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=386ms, congested=-, firstAction=17886ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

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
| 关键时间指标 | impairment: warning=2383ms, congested=-, firstAction=2383ms, L1=2383ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=67ms, congested=-, firstAction=14567ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

