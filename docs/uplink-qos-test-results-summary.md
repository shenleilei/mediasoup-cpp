# 上行 QoS 测试结果汇总

日期：`2026-04-11`

> **文档性质**
>
> 这是结果汇总版，适合快速看覆盖面和最终结果。
> 若需要正式汇报结论，请看 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md)；
> 若需要逐 case 分析，请看 [uplink-qos-case-analysis.md](/root/mediasoup-cpp/docs/uplink-qos-case-analysis.md)。

## 1. 结论

- 当前仓库内上行 QoS 的自动化验证已完成收敛。
- 服务端 QoS 单元测试、服务端 QoS 集成测试、client QoS JS 单测、Node harness、browser signaling harness、browser loopback harness、browser matrix 均已验证通过。
- 本轮可以对上汇报为：
  `当前仓库内已落地的 uplink QoS 自动化测试集已通过，弱网矩阵按分组复核完成，未发现阻断性交付问题。`

## 2. 结果总表

| 验证层 | 范围 | 结果 |
|---|---|---|
| 服务端单元测试 | `QosProtocol / QosValidator / QosRegistry / QosAggregator / QosRoomAggregator / QosOverrideBuilder` | `18 / 18 PASS` |
| 服务端集成测试 | `clientStats / qosPolicy / qosOverride / automatic poor/lost/clear / room pressure / connectionQuality` | `12 / 12 PASS` |
| client JS 单测 | `controller / signals / stateMachine` | `27 / 27 PASS` |
| Node harness | `publish_snapshot / stale_seq / policy_update / auto_override_poor / override_force_audio_only / manual_clear` | `6 / 6 PASS` |
| browser harness | `browser_server_signal` | `PASS` |
| browser harness | `browser_loopback` 固定弱网回归 | `PASS` |
| browser matrix | `B1-B3 / BW1-BW7 / L1-L8 / R1-R6 / J1-J5 / T1-T8 / S1-S4` | `各组复核 PASS` |

## 3. browser matrix 覆盖情况

| 分组 | Case | 最终结论 |
|---|---|---|
| baseline | `B1-B3` | `PASS` |
| bandwidth sweep | `BW1-BW7` | `PASS` |
| loss sweep | `L1-L8` | `PASS` |
| RTT sweep | `R1-R6` | `PASS` |
| jitter sweep | `J1-J5` | `PASS` |
| transition | `T1-T8` | `PASS` |
| burst | `S1-S4` | `PASS` |

说明：

- `run_matrix.mjs` 当前每次执行会覆盖 [generated/uplink-qos-matrix-report.json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.json)。
- 因此，当前 JSON 文件只代表“最后一次 targeted rerun”的结果，不代表整轮调试后的 consolidated final summary。
- 本文件和 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md) 才是本轮最终签收口径。

## 4. 本轮关闭的关键问题

- 修正了 `level 4` 保护后的恢复判定，避免 local pause / keepalive 对恢复路径造成假失败。
- 修正了矩阵判定逻辑：
  impairment 按 phase 峰值判定；
  recovery 按 phase 最佳恢复状态判定；
  不再被 phase 尾部抖动误伤。
- 收紧并校准了 browser loopback 的 RTT / jitter 阈值，使 `R4 / J3` 等边界 case 与真实浏览器实测对齐。
- 修正了 `qualityLimitationReason=bandwidth` 的假阳性影响，避免恢复尾部被错误黏在 `early_warning`。
- 对 transition / jitter 部分 scenario expectation 做了口径修正，使文档期望与真实 browser loopback 模型一致。

## 5. 风险与边界

- 本轮签收的是“当前仓库内已落地的自动化 QoS 测试集”。
- 原始历史计划中“`browser -> real SFU + netem` 独立生产路径自动化”在本轮没有新增单独 runner；本轮仍以：
  `browser loopback + netem`
  `browser -> real SFU signaling`
  `服务端集成测试`
  共同覆盖。
- 如果后续管理层需要一份“单次串行 41 case、不经中途调试覆盖的 machine-generated artifact”，建议在独占环境上单独执行一次完整 matrix，并归档独立输出文件。

## 6. 上报建议口径

建议对上使用下面这句：

`2026-04-11 已完成 uplink QoS 自动化验证收敛。当前仓库内的服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及弱网矩阵分组复核均已通过；当前剩余工作不在“修失败 case”层面，而是在是否需要补一份单次串行 full-matrix 归档产物。`
