# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T12:33:22.261Z`

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
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 242 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 关键时间指标 | impairment: warning=-, congested=16619ms, firstAction=16619ms, L1=24119ms, L2=22619ms, L3=21119ms, L4=16619ms, audioOnly=-；recovery: warning=26423ms, congested=423ms, firstAction=423ms, L1=25423ms, L2=24923ms, L3=15923ms, L4=423ms, audioOnly=- |

