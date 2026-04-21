# 上行 QoS 逐 Case 最终结果

生成时间：`2026-04-14T11:40:39.212Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`1`
- 失败：`0`
- 错误：`0`

## 2. 快速跳转

- 失败 / 错误：无
- burst：[S2](#s2)

## 3. 逐 Case 结果

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 前置 Case | - |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 持续时间 | baseline 15000ms / impairment 8000ms / recovery 32000ms |
| 预期 QoS 状态 | congested |
| 预期动作 | 应触发本地降级动作（以 setEncodingParameters 为主），允许进入 congested，最高不超过 L4 |
| 预期服务端动作 | 无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。 |
| 实际结果 | PASS（符合） |
| 实际 QoS 状态 | baseline(current=stable/L0)；impairment(评估取 peak=congested/L4, current=congested/L4)；recovery(评估取 best=stable/L0, current=congested/L4) |
| 实际动作 | setEncodingParameters（共 61 次非 noop） |
| 实际服务端动作 | 无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。 |
| 重点分析 | 判定=符合。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=congested/L4，recovered=stable/L0。注意：recovery window 内最佳状态已恢复，但 case 结束时 current=congested/L4，说明收尾阶段仍有波动。 |
| 恢复里程碑 | start=2026-04-14T11:40:07.108Z；firstRecovering=16917ms (2026-04-14T11:40:24.025Z)；firstStable=19417ms (2026-04-14T11:40:26.525Z)；firstL0=21417ms (2026-04-14T11:40:28.525Z) |
| 恢复诊断 | raw=loss<3%=417ms (2026-04-14T11:40:07.525Z), rtt<120ms=417ms (2026-04-14T11:40:07.525Z), jitter<28ms=417ms (2026-04-14T11:40:07.525Z), jitter<18ms=16917ms (2026-04-14T11:40:24.025Z)；target=target>=120kbps=12417ms (2026-04-14T11:40:19.525Z), target>=300kbps=17417ms (2026-04-14T11:40:24.525Z), target>=500kbps=18917ms (2026-04-14T11:40:26.025Z), target>=700kbps=20417ms (2026-04-14T11:40:27.525Z), target>=900kbps=-；send=send>=300kbps=1417ms (2026-04-14T11:40:08.525Z), send>=500kbps=1917ms (2026-04-14T11:40:09.025Z), send>=700kbps=20417ms (2026-04-14T11:40:27.525Z), send>=900kbps=22917ms (2026-04-14T11:40:30.025Z)；注：诊断基于 recovery trace 中的 raw per-sample signals，不是状态机内部 EWMA。 |
| 关键时间指标 | impairment: warning=4937ms, recovering=-, stable=437ms, congested=5437ms, firstAction=4937ms, L0=-, L1=4937ms, L2=5437ms, L3=5937ms, L4=6437ms, audioOnly=-；recovery: warning=23917ms, recovering=16917ms, stable=19417ms, congested=417ms, firstAction=417ms, L0=21417ms, L1=19917ms, L2=18417ms, L3=16917ms, L4=417ms, audioOnly=- |
