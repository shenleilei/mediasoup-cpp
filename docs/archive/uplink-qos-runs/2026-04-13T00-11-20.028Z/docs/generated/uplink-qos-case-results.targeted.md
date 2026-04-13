# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-13T00:11:20.028Z`

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
| 实际动作 | setEncodingParameters（共 235 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。 |
| 恢复里程碑 | start=2026-04-13T00:10:49.772Z；firstRecovering=17848ms (2026-04-13T00:11:07.620Z)；firstStable=20349ms (2026-04-13T00:11:10.121Z)；firstL0=22349ms (2026-04-13T00:11:12.121Z) |
| 恢复诊断 | raw=loss<3%=348ms (2026-04-13T00:10:50.120Z), rtt<120ms=1348ms (2026-04-13T00:10:51.120Z), jitter<28ms=17348ms (2026-04-13T00:11:07.120Z), jitter<18ms=18348ms (2026-04-13T00:11:08.120Z)；target=target>=120kbps=12348ms (2026-04-13T00:11:02.120Z), target>=300kbps=18348ms (2026-04-13T00:11:08.120Z), target>=500kbps=19848ms (2026-04-13T00:11:09.620Z), target>=700kbps=21348ms (2026-04-13T00:11:11.120Z), target>=900kbps=22848ms (2026-04-13T00:11:12.620Z)；send=send>=300kbps=18348ms (2026-04-13T00:11:08.120Z), send>=500kbps=19848ms (2026-04-13T00:11:09.620Z), send>=700kbps=20349ms (2026-04-13T00:11:10.121Z), send>=900kbps=21348ms (2026-04-13T00:11:11.120Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=-, recovering=-, stable=-, congested=14682ms, firstAction=14682ms, L0=-, L1=-, L2=-, L3=-, L4=14682ms, audioOnly=-；recovery: warning=-, recovering=17848ms, stable=20349ms, congested=348ms, firstAction=348ms, L0=22349ms, L1=20848ms, L2=19348ms, L3=17848ms, L4=348ms, audioOnly=- |

