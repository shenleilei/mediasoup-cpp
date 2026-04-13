# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T12:27:38.870Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`0`
- 失败：`1`
- 错误：`0`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [T9](#t9) | `FAIL` | stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢 |

## 2. 快速跳转

- 失败 / 错误：[T9](#t9)
- transition：[T9](#t9)

## 3. 逐 Case 结果

### T9

| 字段 | 内容 |
|---|---|
| Case ID | `T9` |
| 前置 Case | - |
| 类型 | `transition` / priority `P0` |
| 上行带宽 | 200 kbps |
| RTT | 500 ms |
| 丢包率 | 20% |
| Jitter | 50 ms |
| 持续时间 | baseline 15000ms / impairment 100000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | FAIL（stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=congested/L4, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 256 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=恢复过慢。预期={"state":"congested","maxLevel":4}；实际 impairment 评估值=congested/L4，recovery 评估值=congested/L4；失败原因=stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢 |
| 关键时间指标 | impairment: warning=-, congested=21574ms, firstAction=21574ms, L1=-, L2=-, L3=-, L4=21574ms, audioOnly=-；recovery: warning=-, congested=9ms, firstAction=9ms, L1=-, L2=-, L3=-, L4=9ms, audioOnly=- |

