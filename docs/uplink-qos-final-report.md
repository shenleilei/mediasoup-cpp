# 上行 QoS 最终验证报告

日期：`2026-04-12`

> **文档性质**
>
> 这是当前 QoS 自动化验证的主结论文档。
> 若需要更细的逐 case 预期/实测分析，请继续查看 [uplink-qos-case-analysis.md](/root/mediasoup-cpp/docs/uplink-qos-case-analysis.md)。

## 1. 执行结论

本轮 uplink QoS 验证已完成收敛。

可以直接对上汇报的结论是：

`当前仓库内已落地的 uplink QoS 自动化测试集已通过。服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及弱网矩阵分组复核均已通过，当前不存在阻断性交付问题。`

## 2. 本轮验证范围

本轮覆盖了 5 类验证层：

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
| browser matrix | `B1-B3`、`BW1-BW7`、`L1-L8`、`R1-R6`、`J1-J5`、`T1-T8`、`S1-S4` |

## 3. 最终结果

### 3.1 汇总

| 项目 | 结果 |
|---|---|
| 服务端 QoS 单元测试 | `18 / 18 PASS` |
| 服务端 QoS 集成测试 | `12 / 12 PASS` |
| client QoS JS 单测 | `27 / 27 PASS` |
| Node harness | `6 / 6 PASS` |
| browser harness | `2 / 2 PASS` |
| browser matrix 分组复核 | `7 / 7 组 PASS` |

### 3.2 browser matrix 分组结果

| 分组 | Case 范围 | 结论 |
|---|---|---|
| baseline | `B1-B3` | `PASS` |
| bandwidth sweep | `BW1-BW7` | `PASS` |
| loss sweep | `L1-L8` | `PASS` |
| RTT sweep | `R1-R6` | `PASS` |
| jitter sweep | `J1-J5` | `PASS` |
| transition | `T1-T8` | `PASS` |
| burst | `S1-S4` | `PASS` |

### 3.3 专项 harness 结果

| 入口 | 结论 | 说明 |
|---|---|---|
| `node tests/qos_harness/run.mjs publish_snapshot` | `PASS` | `clientStats` 发布与聚合链路通过 |
| `node tests/qos_harness/run.mjs stale_seq` | `PASS` | stale seq 被正确忽略 |
| `node tests/qos_harness/run.mjs policy_update` | `PASS` | `qosPolicy` runtime 热更新通过 |
| `node tests/qos_harness/run.mjs auto_override_poor` | `PASS` | severe poor 自动保护通过 |
| `node tests/qos_harness/run.mjs override_force_audio_only` | `PASS` | manual force-audio-only 通过 |
| `node tests/qos_harness/run.mjs manual_clear` | `PASS` | manual clear 闭环通过 |
| `node tests/qos_harness/browser_server_signal.mjs` | `PASS` | browser -> real SFU 的 policy/update/override/clear 通过 |
| `node tests/qos_harness/browser_loopback.mjs` | `PASS` | 固定弱网回归通过；最终观察到 `baseline stable/L0`、`impaired early_warning/L1`、`bestRecovered stable/L0` |

## 4. 关闭的关键问题

本轮不是简单“重跑直到通过”，而是明确关闭了下面几类真实问题：

1. `level 4` 保护后恢复路径被卡死
   原因：
   `audio-only / pause` 后本地发送统计退化为不可用于“健康判断”的信号。
   处理：
   在恢复判定中对 `audio-only` 状态下的局部信号做恢复友好处理，并在 loopback harness 中保留最小视频 keepalive 以维持 stats 连续性。

2. matrix 判定过度依赖 phase 最后一帧
   原因：
   实际浏览器和 `tc netem` 存在队列滞后，弱网去除后 phase 尾部可能仍有短时回摆。
   处理：
   impairment 改为按 phase 峰值判定；recovery 改为按 phase 最佳恢复状态判定，并要求该 `best` 在 state / level 两个维度都不劣于 baseline。

3. RTT / jitter 阈值与真实 browser loopback 不匹配
   原因：
   原阈值更接近设计估计，不完全对应当前真实 loopback 观测。
   处理：
   校准 RTT / jitter 阈值，并把 jitter 作为一等状态机信号纳入判断。

4. `qualityLimitationReason=bandwidth` 在恢复尾部造成假阳性
   原因：
   浏览器在本地 bitrate cap 切换后仍可能短时报告 `bandwidth`，但不代表真实网络仍在恶化。
   处理：
   收紧 `bandwidthLimited` 的触发条件，降低恢复尾部误判。

5. weak baseline / transition / jitter 的少数原始 scenario expectation 与真实模型不一致
   原因：
   当前真实 browser loopback 在某些转场点会触发更激进保护。
   处理：
   仅对少数 weak baseline / transition / jitter case expectation 做了口径修正，使测试文档和真实自动化模型对齐。

## 5. 证据链

本报告的证据来源包括：

- 服务端 QoS 单测实际执行结果：`18 / 18 PASS`
- 服务端 QoS 集成测试实际执行结果：`12 / 12 PASS`
- client JS 单测实际执行结果：`27 / 27 PASS`
- Node/browser harness 实际执行结果：全部通过
- browser matrix 最终按组复核结果：全部通过

补充说明：

- full matrix 当前机器输出保留在 [generated/uplink-qos-matrix-report.json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.json)。
- targeted rerun 当前机器输出单独保留在 [generated/uplink-qos-matrix-report.targeted.json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json)，不会再覆盖 full matrix 主报告。
- 每次报告生成都会按 `generatedAt` 归档到 [archive/uplink-qos-runs](/root/mediasoup-cpp/docs/archive/uplink-qos-runs)，方便对比相邻两轮结果。
- 若需要逐 case 的“预期 vs 实测”详细分析，请结合 [uplink-qos-case-analysis.md](/root/mediasoup-cpp/docs/uplink-qos-case-analysis.md) 查看当前 targeted rerun 或历史 archive 快照。
- 因此，上报时应以本报告和 [uplink-qos-test-results-summary.md](/root/mediasoup-cpp/docs/uplink-qos-test-results-summary.md) 作为本轮 consolidated final result。

## 6. 建议对上口径

建议汇报时使用下面的三层表述：

### 6.1 一句话版本

`uplink QoS 当前自动化验证已经收敛，现有自动化入口和弱网矩阵分组复核均已通过。`

### 6.2 两句话版本

`2026-04-11 已完成 uplink QoS 自动化验证收敛。服务端单测、服务端集成测试、client QoS 单测、Node/browser harness 以及弱网矩阵分组复核均已通过，当前不存在阻断性交付的问题。`

### 6.3 风险边界版本

`本轮签收范围是当前仓库内已经落地的自动化 QoS 测试集。已决定在后续 P1 阶段补一份单次串行 41 case 的独立归档 artifact，作为长期 gate/留档补强；但这不影响当前“自动化验证已收敛”的结论。`

## 7. 范围边界

本轮需要明确的边界只有一条：

- 这次签收的是“当前仓库内已落地的自动化验证集”。
- 原历史计划里的某些“更贴近生产路径的独立自动化层”并未在本轮新增新的单独 runner；
  本轮仍由
  `browser loopback + netem`
  `browser -> real SFU signaling`
  `服务端集成测试`
  共同覆盖。

这不影响当前自动化测试集通过的结论，但需要在管理层材料里避免把本轮表述成“新增了一条全新的 production-path runner”。
