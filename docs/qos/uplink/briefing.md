# 上行 QoS 一页汇报稿

日期：`2026-04-13`

> **文档性质**
>
> 这是对外/对上汇报的一页摘要版，不包含完整证据链。
> 需要完整测试范围、结果和边界时，请看 [uplink-qos-final-report.md](README.md)；
> 需要内部技术边界说明时，请看 [uplink-qos-boundaries.md](boundaries.md)。

## 一句话结论

`截至 2026-04-13，当前仓库内已落地的 uplink QoS 自动化主测试集已通过；默认 browser loopback matrix 主 gate 通过，blind-spot recovery fast-path 已完成首轮验证，BW2 继续作为非阻断的扩展边界哨兵保留。`

## 本轮完成内容

### 1. 自动化验证通过面

- 服务端 QoS 单元测试：`18 / 18 PASS`
- 服务端 QoS 集成测试：`12 / 12 PASS`
- client QoS JS 单测：`27 / 27 PASS`
- Node harness：`6 / 6 PASS`
- browser harness：`2 / 2 PASS`
- browser matrix 主 gate：`43 / 43 PASS`

### 2. browser matrix 主 gate 覆盖范围

- baseline：`B1-B3`
- bandwidth sweep：`BW1`、`BW3-BW7`
- loss sweep：`L1-L8`
- RTT sweep：`R1-R6`
- jitter sweep：`J1-J5`
- transition：`T1-T11`
- burst：`S1-S4`

### 3. 扩展边界哨兵

- `BW2`：
  `4000 -> 2000 -> 4000` 的 loopback boundary transition。
- 当前状态：
  latest 组合 targeted regression 已通过，但历史 dedicated strict rerun 曾失败。
- 当前策略：
  继续保留 `extended`，不计入默认 blocking gate。

### 4. blind-spot recovery

- `T9/T10/T11`：
  当前 recovery fast-path 已完成首轮验证。
- latest 组合 targeted regression：
  `T9,T10,T11,J3,J4,J5,BW2,T1,S4 = 9 / 9 PASS`
- 当前未完全关闭的问题：
  `T10 / T11` 的 recovery tail 曾在组合 targeted regression 中复现；
  latest `T10,T11,J3,J4,J5` targeted rerun 中已回到 `current=stable/L0`。
  当前更合理的口径是“tail oscillation 间歇性存在，继续观察”。

## 本轮关闭的关键问题

### 1. 观测与判定问题

- 修正了 stale RTT / jitter sample 对快环的误放大。
- 修正了 `bw_sweep` 被 recovery 规则误判失败的问题。
- 保持了 impairment 看 `peak`、recovery 看 `best` 的判定口径。

### 2. loopback 边界问题

- `T1` 在不同 full run 中可从 `stable/L0` 波动到 `congested/L4`。
- `S4` 在不同 full run 中可从 `early_warning/L1` 波动到 `congested/L2`。
- 对这两个已证明存在 loopback 波动窗口的 case，已新增最小 `expectByRunner.loopback`。

## 当前质量判断

### 对 uplink QoS 主测试集的综合评分

`8 / 10`

### 评分理由

- 正向项：
  主测试集已经形成稳定签收口径。
- 正向项：
  服务端、client、browser harness 到 loopback matrix 的主链路都已打通。
- 正向项：
  `BW2` 已被明确隔离为扩展边界哨兵，不再污染默认 gate。
- 正向项：
  blind-spot recovery fast-path 已在 `T9` isolated rerun 和 `9-case` 组合 targeted regression 中得到实证支持。

- 扣分项：
  `BW2` 这类 loopback boundary case 仍未完全收敛到单一 deterministic expectation。
- 扣分项：
  `browser -> real SFU + netem` 仍没有单独的新 runner 作为独立证据层。
- 扣分项：
  `T10 / T11` 的 recovery tail 目前更像是“间歇性 oscillation 风险”，说明“恢复开始得足够快”并不自动等于“长期稳定收尾”。

## 风险边界

- 本轮签收的是“当前仓库内已落地的自动化主测试集”。
- `BW2` 继续单独观察，但不影响当前主 gate 通过的结论。
- 如果需要解释“哪些能力由 WebRTC/mediasoup 底层自动提供、哪些能力属于当前仓库 uplink QoS 主线”，请参考 [uplink-qos-boundaries.md](boundaries.md)。

## 建议对上口径

`uplink QoS 当前已经具备可签收的自动化主验证集。后续工作重点不再是恢复默认 gate 的可用性，而是继续收窄 blind-spot recovery 的尾部波动，同时持续治理 BW2 这类 loopback boundary 哨兵，并补更接近真实链路的独立证据层。`
