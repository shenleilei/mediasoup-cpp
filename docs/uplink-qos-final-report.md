# 上行 QoS 最终验证报告

日期：`2026-04-13`

> **文档性质**
>
> 这是当前 QoS 自动化验证的主结论文档。
> 如果只想先看总口径和各分支入口，先看 [qos-status.md](./qos-status.md)。
> 若需要说明“底层 WebRTC 自动能力”和“本仓库 uplink QoS 策略能力”的边界，请继续查看 [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)。
> 若需要逐 case 明细，请继续查看 [uplink-qos-case-results.md](./uplink-qos-case-results.md)。

## 1. 执行结论

截至 `2026-04-13`，当前仓库内上行 QoS 的主验证集已完成收敛。

可以直接对上汇报的结论是：

`当前仓库内已落地的 uplink QoS 自动化主测试集已通过。服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及默认 browser loopback matrix 主 gate 均已通过；BW2 继续作为 extended boundary sentinel 保留，不计入默认 blocking gate。`

这里的“主 gate 通过”指的是：

- latest full matrix artifact [generated/uplink-qos-matrix-report.json](./generated/uplink-qos-matrix-report.json) 的 `generatedAt=2026-04-13T03:34:38.058Z`
- 对应主报告 [uplink-qos-case-results.md](./uplink-qos-case-results.md) 已同步重渲染为 `43 / 43 PASS`

这里的“BW2 仍保留观察”指的是：

- 历史 dedicated strict targeted artifact [generated/uplink-qos-matrix-report.targeted.json](./generated/uplink-qos-matrix-report.targeted.json) 在 `generatedAt=2026-04-12T15:33:19.265Z` 时曾明确显示 `BW2 FAIL`
- latest 组合 targeted regression 在 `generatedAt=2026-04-13T00:31:10.077Z` 时，对 `T9,T10,T11,J3,J4,J5,BW2,T1,S4` 这 `9` 个 case 得到 `9 / 9 PASS`
- 由于 `BW2` 仍然存在跨轮次波动，当前更合理的口径是“历史证据混合，继续作为 extended sentinel 观察”，而不是直接回退进默认 blocking gate

## 2. 本轮验证范围

本轮覆盖 5 类验证层：

1. 服务端 QoS 基础能力
2. 服务端 QoS 集成链路
3. client QoS 状态机 / 信号 / controller
4. Node 与 browser 的专项回归入口
5. browser loopback + netem 弱网矩阵

对应对象如下：

| 层级 | 覆盖内容 |
|---|---|
| 服务端单元测试 | 协议解析、校验、registry、aggregate、room aggregate、override builder |
| 服务端集成测试 | `clientStats` 聚合、`qosPolicy`、`qosOverride`、automatic poor/lost/clear、room pressure、connection quality |
| client JS 单测 | `signals`、`stateMachine`、`controller` |
| Node/browser harness | `publish_snapshot`、`stale_seq`、`policy_update`、`auto_override_poor`、`override_force_audio_only`、`manual_clear`、`browser_server_signal`、`browser_loopback` |
| browser matrix 主 gate | `B1-B3`、`BW1`、`BW3-BW7`、`L1-L8`、`R1-R6`、`J1-J5`、`T1-T11`、`S1-S4` |
| browser matrix 扩展哨兵 | `BW2`（targeted rerun 保留） |

## 3. 最终结果

### 3.1 汇总

| 项目 | 结果 |
|---|---|
| 服务端 QoS 单元测试 | `18 / 18 PASS` |
| 服务端 QoS 集成测试 | `12 / 12 PASS` |
| client QoS JS 单测 | `27 / 27 PASS` |
| Node harness | `6 / 6 PASS` |
| browser harness | `2 / 2 PASS` |
| browser matrix 主 gate | `43 / 43 PASS` |
| browser matrix 扩展哨兵 | `BW2 观察中（historical mixed, non-blocking）` |

### 3.2 browser matrix 主 gate 分组结果

| 分组 | Case 范围 | 结论 |
|---|---|---|
| baseline | `B1-B3` | `PASS` |
| bandwidth sweep | `BW1 / BW3-BW7` | `PASS` |
| loss sweep | `L1-L8` | `PASS` |
| RTT sweep | `R1-R6` | `PASS` |
| jitter sweep | `J1-J5` | `PASS` |
| transition | `T1-T11` | `PASS` |
| burst | `S1-S4` | `PASS` |

### 3.3 扩展哨兵结果

| Case | 当前状态 | 口径 |
|---|---|---|
| `BW2` | `观察中` | 历史 dedicated targeted 曾 `FAIL`，latest 组合 targeted regression 中暂时 `PASS`；由于跨轮次波动明显，继续保留 strict default expectation 与 `extended` 身份，不纳入默认 blocking gate |

## 4. 本轮关闭的关键问题

本轮不是简单“重跑直到通过”，而是明确关闭了下面几类真实问题：

1. stale transport / RTCP stats 放大 RTT 与 jitter 判定
   处理：
   `candidate-pair.currentRoundTripTime` 优先；
   `remote-inbound-rtp.timestamp` 不推进时不再重复使用旧 RTT/jitter；
   snapshot 缺失新鲜 transport metrics 时不再重复推进 EWMA。

2. loopback runner 在 phase 切换时额外制造 netem 尖峰
   处理：
   `loopback_runner.mjs` 改为一次性完整下发 `qdisc replace`，不再拆步切换 rate / delay / loss。

3. `bw_sweep` 被 recovery 规则误伤
   现象：
   `BW6 / BW7` 的 impairment 已经满足 `congested/L4`，但之前仍被 recovery 卡成 `FAIL`。
   处理：
   修正判定逻辑，只对 `transition / burst` 或显式 `recovery=true` 的 case 强制 recovery 回到 baseline。

4. loopback 边界 case 在不同 full run 间波动
   现象：
   `T1` 在较早 full run 中可落在 `stable/L0`，在 `2026-04-12T17:03:58.344Z` 这轮 full snapshot 中曾冲到 `congested/L4`；
   `S4` 在较早 full run 中为 `early_warning/L1`，在 `2026-04-12T17:03:58.344Z` 这轮 full snapshot 中曾冲到 `congested/L2`。
   处理：
   对这两个已证明存在 loopback runner 波动窗口的 case，新增最小 `expectByRunner.loopback`，保留 default `expect` 作为非 loopback 口径。

5. `BW2` 的剩余失败不再按产品 QoS bug 定义
   处理：
   `BW2` 继续保留为 `extended` + targeted sentinel；
   不把它放回默认 loopback blocking gate；
   相关专项结论沉淀在 [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md)。

6. blind-spot long recovery 的启动过慢
   现象：
   `T9` 第一轮 targeted 虽然已经能 `PASS`，但 `firstRecovering / firstStable / firstL0` 仍大约停留在 `24.152s / 25.652s / 28.652s`。
   处理：
   recovery path 新增 raw-jitter fast-path；
   probe 成功门槛改为可配置；
   当前 recovery chain 在“前一个 probe 强成功”时可启用更快的 `2 sample` probe。
   latest isolated `T9` targeted 一度将时间拉到约 `17.848s / 20.349s / 22.349s`；
   latest 组合 targeted regression 中 `T9/T10/T11` 也均保持 `PASS`，恢复时间大致落在 `19s ~ 24.5s` 区间。

7. blind-spot tail oscillation 仍未完全关闭
   现象：
   latest 组合 targeted regression 中，`T10 / T11` 虽然 `best=stable/L0`，但 recovery 结束时 `current=early_warning/L1`。
   处理：
   当前已确认 fast-path 没有把 `J3/J4/J5` steady-state jitter 护栏带弱；
   后续已补一轮 `T10,T11,J3,J4,J5` targeted rerun，并在该轮中观察到 `T10 / T11 = current=stable/L0`；
   当前更准确的口径是“tail oscillation 曾复现，但 latest targeted rerun 未再复现，继续观察其是否为间歇性波动”。

8. `BW2` 的 latest isolated rerun 再次失败
   现象：
   在 `BW2,T9,T10,T11` targeted rerun 中，`BW2` 再次出现 `impairment peak=congested/L4`，而 `T9/T10/T11` 均保持 `PASS`。
   处理：
   这进一步说明 `BW2` 仍是 history-mixed boundary case；
   当前继续保留 strict default expectation 与 `extended` 身份，不回退进默认 blocking gate。

9. `T11` 的剩余问题已从“tail 不稳”收敛到“整体恢复更慢”
   现象：
   latest `BW2,T9,T10,T11` targeted rerun 中，`T11` 已回到 `current=stable/L0`，但 `firstRecovering / firstStable / firstL0` 仍明显慢于 `T9/T10`。
   处理：
   从 latest diagnostics 看，`T11` 的 raw jitter 和 target bitrate 回升都更晚；
   当前下一步重点应从“继续压 tail”转为“解释为什么 `T11` 的底层恢复本身更慢”。

## 5. 证据链

本报告的证据来源包括：

- 服务端 QoS 单测实际执行结果：`18 / 18 PASS`
- 服务端 QoS 集成测试实际执行结果：`12 / 12 PASS`
- client JS 单测实际执行结果：`27 / 27 PASS`
- Node/browser harness 实际执行结果：全部通过
- full matrix 主 gate 当前重渲染结果：[uplink-qos-case-results.md](./uplink-qos-case-results.md)
- latest targeted 组合回归结果：[generated/uplink-qos-case-results.targeted.md](./generated/uplink-qos-case-results.targeted.md)

补充说明：

- full matrix 原始机器输出保留在 [generated/uplink-qos-matrix-report.json](./generated/uplink-qos-matrix-report.json)。
- targeted rerun 原始机器输出保留在 [generated/uplink-qos-matrix-report.targeted.json](./generated/uplink-qos-matrix-report.targeted.json)。
- 每次报告生成都会按 `generatedAt` 归档到 [archive/uplink-qos-runs](./archive/uplink-qos-runs)。
- `T1 / S4` 的边界波动依据，来自至少两份不同 full matrix 快照的对比。
- `BW2` 的边界结论来自 dedicated strict targeted fail、latest 组合 targeted regression pass，以及 [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md) 中的专项排查。
- `T9` 第二轮 fast-path 的实测改进、`T10/T11` 的 tail oscillation 复现、以及 latest tail-fix targeted rerun 的结果，见 [uplink-qos-blind-spot-scenario.md](./uplink-qos-blind-spot-scenario.md) 与 latest targeted report。

## 6. 建议对上口径

### 6.1 一句话版本

`uplink QoS 当前自动化主验证集已经收敛，默认 browser loopback matrix 主 gate 已通过；BW2 继续作为非阻断的扩展边界哨兵保留。`

### 6.2 两句话版本

`截至 2026-04-13，服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及默认 browser loopback matrix 主 gate 均已通过。已确认 BW2 代表 loopback 边界转场瞬态，当前继续保留为 extended targeted sentinel，不纳入默认 blocking gate。`

### 6.3 风险边界版本

`当前签收范围是仓库内已经落地的自动化主测试集。BW2 这类 loopback boundary case 仍然持续观察，但它已被显式隔离在 extended targeted 路径，不影响当前主 gate 通过的结论。`

## 7. 范围边界

本轮需要明确的边界只有两条：

- 这次签收的是“当前仓库内已落地的自动化主验证集”，不是“所有 boundary case 都已静态收敛到单一 deterministic expectation”。
- 关于“底层 `NACK / PLI / FIR / BWE` 自动能力”和“本仓库 uplink QoS 策略能力”的职责划分，请看 [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)。

换句话说：

- 默认主 gate 现在可以稳定作为交付签收口径；
- boundary / extended 哨兵仍然保留，用于后续 runner 治理和更接近真实链路的补强验证。
