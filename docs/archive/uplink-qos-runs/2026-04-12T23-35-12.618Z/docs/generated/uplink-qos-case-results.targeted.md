# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-12T23:35:12.618Z`

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
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| 持续时间 | baseline 15000ms / impairment 100000ms / recovery 30000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=stable/L0) |
| 实际动作 | setEncodingParameters（共 247 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-12T23:34:42.380Z；firstRecovering=24152ms (2026-04-12T23:35:06.532Z)；firstStable=25652ms (2026-04-12T23:35:08.032Z)；firstL0=28652ms (2026-04-12T23:35:11.032Z) |
| 恢复诊断 | raw=loss<3%=152ms (2026-04-12T23:34:42.532Z), rtt<120ms=652ms (2026-04-12T23:34:43.032Z), jitter<28ms=18152ms (2026-04-12T23:35:00.532Z), jitter<18ms=19152ms (2026-04-12T23:35:01.532Z)；target=target>=120kbps=13652ms (2026-04-12T23:34:56.032Z), target>=300kbps=24652ms (2026-04-12T23:35:07.032Z), target>=500kbps=26152ms (2026-04-12T23:35:08.532Z), target>=700kbps=27652ms (2026-04-12T23:35:10.032Z), target>=900kbps=29152ms (2026-04-12T23:35:11.532Z)；send=send>=300kbps=25152ms (2026-04-12T23:35:07.532Z), send>=500kbps=26152ms (2026-04-12T23:35:08.532Z), send>=700kbps=27652ms (2026-04-12T23:35:10.032Z), send>=900kbps=27652ms (2026-04-12T23:35:10.032Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=19237ms, stable=-, congested=17237ms, firstAction=17237ms, L0=-, L1=-, L2=-, L3=19237ms, L4=17237ms, audioOnly=-；recovery: warning=-, recovering=24152ms, stable=25652ms, congested=152ms, firstAction=152ms, L0=28652ms, L1=27152ms, L2=25652ms, L3=24152ms, L4=152ms, audioOnly=- |

