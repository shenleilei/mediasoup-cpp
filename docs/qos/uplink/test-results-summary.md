# 上行 QoS 测试结果汇总

日期：`2026-04-13`

> **文档性质**
>
> 这是结果汇总版，适合快速看覆盖面和最终口径。
> 如果只想先看总口径和各分支入口，先看 [qos-status.md](../README.md)。
> 若需要正式汇报结论，请看 [uplink-qos-final-report.md](README.md)；
> 若需要逐 case 明细，请看 [uplink-qos-case-results.md](../../uplink-qos-case-results.md)。

## 1. 结论

- 当前仓库内上行 QoS 的自动化主验证集已完成收敛。
- 服务端 QoS 单元测试、服务端 QoS 集成测试、client QoS JS 单测、Node harness、browser harness、browser matrix 主 gate 均已通过。
- `BW2` 仍保留为 `extended` 的 targeted boundary sentinel；latest 组合 targeted regression 里它已通过，但历史证据仍有波动，因此暂不纳入默认 blocking gate。

建议对上统一使用下面这句：

`截至 2026-04-13，当前仓库内已落地的 uplink QoS 自动化主测试集已通过；默认 weak-network matrix 主 gate 通过，BW2 继续作为非阻断的扩展边界哨兵保留，blind-spot recovery fast-path 已完成首轮验证。`

## 2. 结果总表

| 验证层 | 范围 | 结果 |
|---|---|---|
| 服务端单元测试 | `QosProtocol / QosValidator / QosRegistry / QosAggregator / QosRoomAggregator / QosOverrideBuilder` | `18 / 18 PASS` |
| 服务端集成测试 | `clientStats / qosPolicy / qosOverride / automatic poor/lost/clear / room pressure / connectionQuality` | `12 / 12 PASS` |
| client JS 单测 | `controller / signals / stateMachine` | `27 / 27 PASS` |
| Node harness | `publish_snapshot / stale_seq / policy_update / auto_override_poor / override_force_audio_only / manual_clear` | `6 / 6 PASS` |
| browser harness | `browser_server_signal / browser_loopback` | `2 / 2 PASS` |
| browser matrix 主 gate | `B1-B3 / BW1 / BW3-BW7 / L1-L8 / R1-R6 / J1-J5 / T1-T11 / S1-S4` | `43 / 43 PASS` |
| browser matrix 扩展哨兵 | `BW2` | `观察中（historical mixed, non-blocking）` |

## 3. browser matrix 覆盖情况

| 分组 | Case | 最终结论 |
|---|---|---|
| baseline | `B1-B3` | `PASS` |
| bandwidth sweep | `BW1 / BW3-BW7` | `PASS` |
| loss sweep | `L1-L8` | `PASS` |
| RTT sweep | `R1-R6` | `PASS` |
| jitter sweep | `J1-J5` | `PASS` |
| transition | `T1-T11` | `PASS` |
| burst | `S1-S4` | `PASS` |

补充说明：

- latest full matrix `generatedAt=2026-04-13T03:34:38.058Z`，当前重渲染结果见 [uplink-qos-case-results.md](../../uplink-qos-case-results.md)。
- latest targeted 组合回归结果见 [generated/uplink-qos-case-results.targeted.md](../../generated/uplink-qos-case-results.targeted.md)。
- full / targeted 的原始机器 artifact 仍分别保留在 [generated/uplink-qos-matrix-report.json](../../generated/uplink-qos-matrix-report.json) 和 [generated/uplink-qos-matrix-report.targeted.json](../../generated/uplink-qos-matrix-report.targeted.json)。
- latest 组合 targeted regression 覆盖：
  `T9,T10,T11,J3,J4,J5,BW2,T1,S4`
  当前结果为 `9 / 9 PASS`。

## 4. 本轮新增的收口点

- 修正了 `bw_sweep` 的 recovery 判定 bug，`BW6 / BW7` 不再被 recovery 规则误判失败。
- 为已证明存在 loopback runner 波动窗口的 `T1 / S4` 新增最小 `expectByRunner.loopback`。
- `BW2` 继续保留 strict default expectation，不回退进默认主 gate；虽然 latest 组合 targeted regression 已通过，但历史 dedicated strict rerun 仍显示它存在非稳定波动。
- `R4 / J3 / T9` 在 latest full matrix 主 gate 中保持通过，无需额外 runner-specific expectation。
- recovery fast-path 已在 `T9` 上验证有效，并在 `T9/T10/T11 + J3/J4/J5 + BW2/T1/S4` 的 `9-case` 组合 targeted regression 中保持全通过。
- `T10 / T11` 的 recovery tail 曾在组合 targeted regression 中出现 `current=early_warning/L1`；
  但 latest `T10,T11,J3,J4,J5` targeted rerun 中，两者都已回到 `current=stable/L0`。
  当前更合理的口径是“tail oscillation 具备间歇性，仍需继续观察”。

## 5. 风险与边界

- 本轮签收的是“当前仓库内已落地的自动化主测试集”。
- `BW2` 一类 loopback boundary case 仍然持续观察，但已被显式隔离在 `extended` targeted 路径。
- 原始历史计划中的 `browser -> real SFU + netem` 独立生产路径自动化，本轮仍未新增单独 runner；当前仍由：
  `browser loopback + netem`
  `browser -> real SFU signaling`
  `服务端集成测试`
  共同覆盖。
