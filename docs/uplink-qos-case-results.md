# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T08:12:22.359Z`

## 1. 汇总

- 总 Case：`41`
- 已执行：`41`
- 通过：`39`
- 失败：`2`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [B3](#b3) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| [J3](#j3) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[B3](#b3)、[J3](#j3)
- baseline：[B1](#b1)、[B2](#b2)、[B3](#b3)
- bw_sweep：[BW1](#bw1)、[BW2](#bw2)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)
- loss_sweep：[L1](#l1)、[L2](#l2)、[L3](#l3)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)
- rtt_sweep：[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)
- jitter_sweep：[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)
- burst：[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)

## 3. 逐 Case 结果

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
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱） |
| 实际 QoS 状态 | baseline(current=congested/L4)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 123 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过弱。预期={"states":["early_warning","congested"],"minLevel":1,"maxLevel":4}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| 关键时间指标 | impairment: warning=-, congested=159ms, firstAction=159ms, L1=18659ms, L2=18159ms, L3=17660ms, L4=159ms, audioOnly=-；recovery: warning=756ms, congested=1256ms, firstAction=756ms, L1=756ms, L2=1256ms, L3=1756ms, L4=2255ms, audioOnly=- |

### BW1

| 字段 | 内容 |
|---|---|
| Case ID | `BW1` |
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=2070ms, congested=-, firstAction=2070ms, L1=2070ms, L2=-, L3=-, L4=-, audioOnly=- |

### BW3

| 字段 | 内容 |
|---|---|
| Case ID | `BW3` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1855ms, congested=2355ms, firstAction=1855ms, L1=1855ms, L2=2355ms, L3=2855ms, L4=3355ms, audioOnly=-；recovery: warning=1872ms, congested=3372ms, firstAction=372ms, L1=1872ms, L2=3372ms, L3=3872ms, L4=4372ms, audioOnly=- |

### BW4

| 字段 | 内容 |
|---|---|
| Case ID | `BW4` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 70 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1384ms, congested=2884ms, firstAction=1384ms, L1=1384ms, L2=2884ms, L3=3384ms, L4=3884ms, audioOnly=-；recovery: warning=1944ms, congested=2944ms, firstAction=444ms, L1=1444ms, L2=944ms, L3=444ms, L4=3944ms, audioOnly=- |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 78 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3860ms, congested=4360ms, firstAction=3860ms, L1=3860ms, L2=4360ms, L3=4860ms, L4=5360ms, audioOnly=-；recovery: warning=22572ms, congested=72ms, firstAction=72ms, L1=20072ms, L2=19572ms, L3=19072ms, L4=72ms, audioOnly=- |

### BW6

| 字段 | 内容 |
|---|---|
| Case ID | `BW6` |
| 前置 Case | [B1](#b1) |
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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 108 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=5339ms, congested=5839ms, firstAction=5339ms, L1=5339ms, L2=5839ms, L3=6339ms, L4=6839ms, audioOnly=-；recovery: warning=21928ms, congested=1446ms, firstAction=428ms, L1=928ms, L2=428ms, L3=19928ms, L4=1446ms, audioOnly=- |

### BW7

| 字段 | 内容 |
|---|---|
| Case ID | `BW7` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 164 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=4339ms, congested=4840ms, firstAction=4339ms, L1=4339ms, L2=4840ms, L3=5340ms, L4=5839ms, audioOnly=-；recovery: warning=24054ms, congested=54ms, firstAction=54ms, L1=21555ms, L2=21055ms, L3=20554ms, L4=54ms, audioOnly=- |

### L1

| 字段 | 内容 |
|---|---|
| Case ID | `L1` |
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 4 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1847ms, congested=-, firstAction=1847ms, L1=1847ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=462ms, congested=-, firstAction=2962ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 42 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2860ms, congested=15860ms, firstAction=2860ms, L1=2860ms, L2=15860ms, L3=16360ms, L4=16860ms, audioOnly=-；recovery: warning=6709ms, congested=209ms, firstAction=209ms, L1=4709ms, L2=4209ms, L3=3709ms, L4=209ms, audioOnly=- |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 79 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=862ms, congested=2361ms, firstAction=862ms, L1=862ms, L2=2361ms, L3=2861ms, L4=3361ms, audioOnly=-；recovery: warning=10203ms, congested=202ms, firstAction=202ms, L1=8702ms, L2=8202ms, L3=7702ms, L4=202ms, audioOnly=- |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3343ms, congested=3843ms, firstAction=3343ms, L1=3343ms, L2=3843ms, L3=4343ms, L4=4843ms, audioOnly=-；recovery: warning=24207ms, congested=207ms, firstAction=207ms, L1=22707ms, L2=22207ms, L3=21707ms, L4=207ms, audioOnly=- |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 83 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2887ms, congested=3388ms, firstAction=2887ms, L1=2887ms, L2=3388ms, L3=3887ms, L4=4388ms, audioOnly=-；recovery: warning=-, congested=361ms, firstAction=361ms, L1=22360ms, L2=21861ms, L3=21361ms, L4=361ms, audioOnly=- |

### R1

| 字段 | 内容 |
|---|---|
| Case ID | `R1` |
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=3616ms, congested=-, firstAction=3616ms, L1=3616ms, L2=-, L3=-, L4=-, audioOnly=- |

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=3285ms, congested=-, firstAction=3285ms, L1=3285ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=420ms, congested=-, firstAction=2420ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 74 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1737ms, congested=4237ms, firstAction=1737ms, L1=1737ms, L2=4237ms, L3=4737ms, L4=5237ms, audioOnly=-；recovery: warning=7258ms, congested=258ms, firstAction=258ms, L1=5758ms, L2=5258ms, L3=4758ms, L4=258ms, audioOnly=- |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 84 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1696ms, congested=2196ms, firstAction=1696ms, L1=1696ms, L2=2196ms, L3=2696ms, L4=3196ms, audioOnly=-；recovery: warning=6548ms, congested=48ms, firstAction=48ms, L1=5548ms, L2=5048ms, L3=4549ms, L4=48ms, audioOnly=- |

### J1

| 字段 | 内容 |
|---|---|
| Case ID | `J1` |
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
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
| 前置 Case | [B1](#b1) |
| 类型 | `jitter_sweep` / priority `P0` |
| 上行带宽 | 4000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 40 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L2 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 76 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"state":"early_warning","minLevel":1,"maxLevel":2}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 关键时间指标 | impairment: warning=2370ms, congested=5370ms, firstAction=2370ms, L1=2370ms, L2=5370ms, L3=5870ms, L4=6370ms, audioOnly=-；recovery: warning=8152ms, congested=150ms, firstAction=150ms, L1=6650ms, L2=6151ms, L3=5650ms, L4=150ms, audioOnly=- |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 前置 Case | [B1](#b1) |
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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=early_warning/L1, current=early_warning/L1)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 2 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3782ms, congested=-, firstAction=3782ms, L1=3782ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=359ms, congested=-, firstAction=15359ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 前置 Case | [B1](#b1) |
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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 82 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1900ms, congested=4399ms, firstAction=1900ms, L1=1900ms, L2=4399ms, L3=4900ms, L4=5399ms, audioOnly=-；recovery: warning=14297ms, congested=297ms, firstAction=297ms, L1=8296ms, L2=7797ms, L3=7297ms, L4=297ms, audioOnly=- |

### T1

| 字段 | 内容 |
|---|---|
| Case ID | `T1` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | 无非 noop 动作 |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=stable/L0，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 前置 Case | [B1](#b1) |
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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=stable/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2376ms, congested=2876ms, firstAction=2376ms, L1=2376ms, L2=2876ms, L3=3379ms, L4=3879ms, audioOnly=-；recovery: warning=1331ms, congested=1831ms, firstAction=1331ms, L1=1331ms, L2=1831ms, L3=2331ms, L4=2831ms, audioOnly=- |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=3354ms, congested=3854ms, firstAction=3354ms, L1=3354ms, L2=3854ms, L3=4354ms, L4=4854ms, audioOnly=-；recovery: warning=20067ms, congested=64ms, firstAction=64ms, L1=18066ms, L2=17567ms, L3=17066ms, L4=64ms, audioOnly=- |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=4868ms, congested=-, firstAction=4868ms, L1=4868ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=192ms, congested=-, firstAction=13692ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 75 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1142ms, congested=2142ms, firstAction=1142ms, L1=1142ms, L2=2142ms, L3=2642ms, L4=3142ms, audioOnly=-；recovery: warning=7054ms, congested=54ms, firstAction=54ms, L1=5554ms, L2=5058ms, L3=4554ms, L4=54ms, audioOnly=- |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=3284ms, congested=-, firstAction=3284ms, L1=3284ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=392ms, congested=-, firstAction=15895ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=3383ms, congested=-, firstAction=3383ms, L1=3383ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=206ms, congested=-, firstAction=15706ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 前置 Case | [B3](#b3) |
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
| 实际动作 | setEncodingParameters（共 37 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=congested/L4。 |
| 关键时间指标 | impairment: warning=144ms, congested=2644ms, firstAction=2644ms, L1=-, L2=2644ms, L3=3144ms, L4=3644ms, audioOnly=-；recovery: warning=-, congested=-, firstAction=-, L1=-, L2=-, L3=-, L4=-, audioOnly=- |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 45 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1333ms, congested=-, firstAction=1333ms, L1=1333ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=10990ms, congested=490ms, firstAction=490ms, L1=9490ms, L2=490ms, L3=990ms, L4=1490ms, audioOnly=- |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 95 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2834ms, congested=3334ms, firstAction=2834ms, L1=2834ms, L2=3334ms, L3=3834ms, L4=4334ms, audioOnly=-；recovery: warning=23382ms, congested=382ms, firstAction=382ms, L1=21382ms, L2=20882ms, L3=20382ms, L4=382ms, audioOnly=- |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 前置 Case | [B1](#b1) |
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
| 实际动作 | setEncodingParameters（共 21 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=early_warning/L1，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=1745ms, congested=-, firstAction=1745ms, L1=1745ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=379ms, congested=2881ms, firstAction=2881ms, L1=11881ms, L2=2881ms, L3=3381ms, L4=3882ms, audioOnly=- |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 前置 Case | [B1](#b1) |
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
| 关键时间指标 | impairment: warning=2843ms, congested=-, firstAction=2843ms, L1=2843ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=90ms, congested=-, firstAction=14590ms, L1=-, L2=-, L3=-, L4=-, audioOnly=- |
