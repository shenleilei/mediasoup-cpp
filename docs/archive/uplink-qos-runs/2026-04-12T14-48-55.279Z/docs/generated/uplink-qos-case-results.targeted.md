# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T14:48:55.279Z`

## 1. 汇总

- 总 Case：`2`
- 已执行：`2`
- 通过：`1`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [BW2](#bw2) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[BW2](#bw2)
- bw_sweep：[BW2](#bw2)、[BW5](#bw5)

## 3. 逐 Case 结果

### BW2

| 字段 | 内容 |
|---|---|
| Case ID | `BW2` |
| 前置 Case | - |
| 类型 | `bw_sweep` / priority `P1` |
| 上行带宽 | 2000 kbps |
| RTT | 25 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=congested/L4)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 101 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["stable","early_warning"],"maxLevel":1}；实际 impairment 评估值=congested/L4，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过强 |
| 关键时间指标 | impairment: warning=11504ms, congested=4ms, firstAction=4ms, L1=7504ms, L2=6004ms, L3=4504ms, L4=4ms, audioOnly=-；recovery: warning=12699ms, congested=199ms, firstAction=199ms, L1=8699ms, L2=7217ms, L3=5699ms, L4=199ms, audioOnly=- |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 前置 Case | - |
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
| 实际动作 | setEncodingParameters（共 110 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=2799ms, congested=3299ms, firstAction=2799ms, L1=2799ms, L2=3299ms, L3=3799ms, L4=4299ms, audioOnly=-；recovery: warning=-, congested=289ms, firstAction=289ms, L1=21289ms, L2=19789ms, L3=18289ms, L4=289ms, audioOnly=- |

