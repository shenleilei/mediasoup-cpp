# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T13:34:10.926Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`0`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [R3](#r3) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[R3](#r3)
- rtt_sweep：[R3](#r3)

## 3. 逐 Case 结果

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 前置 Case | - |
| 类型 | `rtt_sweep` / priority `P1` |
| 上行带宽 | 4000 kbps |
| RTT | 120 ms |
| 丢包率 | 0.1% |
| Jitter | 5 ms |
| 持续时间 | baseline 15000ms / impairment 20000ms / recovery 30000ms |
| 预期 QoS 状态 | stable / early_warning |
| 预期动作 | 应保持 stable 或轻度降级到 stable / early_warning，动作以 noop / setEncodingParameters 为主，最高不超过 L1 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强） |
| 实际 QoS 状态 | baseline(current=stable/L2)；impairment(评估取 peak=congested/L0, current=congested/L0)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 29 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=过强。预期={"states":["stable","early_warning"],"maxLevel":1}；实际 impairment 评估值=congested/L0，recovery 评估值=stable/L0；失败原因=stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过强 |
| 关键时间指标 | impairment: warning=2200ms, congested=3700ms, firstAction=200ms, L1=200ms, L2=-, L3=-, L4=-, audioOnly=-；recovery: warning=-, congested=383ms, firstAction=1876ms, L1=1876ms, L2=2381ms, L3=2882ms, L4=3377ms, audioOnly=- |

