# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T12:16:10.821Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`1`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
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
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 241 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=18117ms, firstAction=18117ms, L1=-, L2=-, L3=-, L4=18117ms, audioOnly=-；recovery: warning=23139ms, congested=139ms, firstAction=139ms, L1=21139ms, L2=20639ms, L3=19639ms, L4=139ms, audioOnly=- |

