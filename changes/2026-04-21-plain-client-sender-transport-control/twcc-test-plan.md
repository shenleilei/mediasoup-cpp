# TWCC / Send-Transport 测试设计

## 目标

用可重复、可度量的测试覆盖以下风险：

1. transport-cc feedback 到发送预算的闭环是否真实生效
2. 不同码率下音频/重传/新鲜视频的调度分配是否符合策略
3. 持续回压与暂停场景下是否仍保持队列有界且无视频泄漏

## 范围

包含：

- `TransportCcHelpers` 解析与基础合法性
- `NetworkThread` 的 feedback 接收与估算更新
- `SenderTransportController` 的预算计算与队列调度
- threaded plain-client 的回归链路

不包含：

- 完整 GCC（delay-based + acknowledged-bitrate + probe controller）对齐
- worker 侧 congestion controller 行为重写

## 分层策略

### 1) 单元测试（确定性）

用于固定算法和策略约束，避免时间/网络抖动导致误判。

| ID | 组件 | 场景 | 关键断言 |
|---|---|---|---|
| UT-TWCC-001 | `TransportCcHelpers` | run-length + vector + mixed delta 解析 | `packetStatusCount/received/lost` 与预期一致 |
| UT-TWCC-002 | `TransportCcHelpers` | malformed/short/invalid length | 返回 false，且不会污染统计 |
| UT-STC-001 | `SenderTransportController` | `effective=min(target,estimate,appCap)` | `EffectivePacingBitrateBps()` 精确匹配 |
| UT-STC-002 | `SenderTransportController` | estimate 下降后预算变化 | 同 tick 条件下 `MediaBudgetBytes` 显著下降 |
| UT-STC-003 | `SenderTransportController` | estimate 增长分支 | budget 随 estimate 上升，且不超 cap |
| UT-STC-004 | `SenderTransportController` | queue bound under WouldBlock | fresh/retrans queue 不超过上限 |
| UT-STC-005 | `SenderTransportController` | PauseTrack 后清理 | fresh + retrans 同时被清空，后续不再发该轨视频 |
| UT-STC-006 | `SenderTransportController` | 优先级契约 | `Control > Audio > Retrans > Fresh` 恒成立 |

### 2) 组件集成测试（NetworkThread）

用于验证 feedback 到发送控制的接线正确。

| ID | 组件 | 场景 | 关键断言 |
|---|---|---|---|
| IT-NT-001 | `NetworkThread` | 注入 10%+ loss feedback | `transportEstimatedBitrateBps` 下调为预期倍率 |
| IT-NT-002 | `NetworkThread` | 注入 2%~10% loss feedback | 走温和下降分支 |
| IT-NT-003 | `NetworkThread` | 注入 <2% loss feedback | 走 additive increase 分支 |
| IT-NT-004 | `NetworkThread` | 连续 feedback + clamp | 估算值不低于 min、不高于 max |
| IT-NT-005 | `NetworkThread` | malformed feedback | `transportCcMalformedFeedbackCount` 递增，estimate 不变 |
| IT-NT-006 | `NetworkThread` | estimate 变化后发送吞吐 | 同负载下发送字节速率随 estimate 变化 |

### 3) 策略分配矩阵测试（重点）

用于回答“不同码率下各类型发送数量是否正确”。

固定输入模型（确定性）：

- pacing tick：20ms
- audio：每 20ms 入队 120B（等效 48kbps）
- retrans：持续入队 1000B 包
- fresh video：持续入队 1000B 包
- 观察窗口：10s

码率矩阵：

- 64kbps（极低）
- 128kbps（低）
- 256kbps（中）
- 512kbps（高）
- 1000kbps（较高）

每档断言：

| ID | 断言 |
|---|---|
| MX-ALLOC-001 | `Control` 不受 backlog 饥饿影响（注入控制包后快速发送） |
| MX-ALLOC-002 | `Audio` 发送数优先于 fresh video；低码率可出现 deadline drop 但需可观测 |
| MX-ALLOC-003 | 在 retrans backlog 非空阶段，fresh video 发送为 0 |
| MX-ALLOC-004 | 码率升高时 fresh video 发送计数单调不减 |
| MX-ALLOC-005 | 总发送字节近似受 `effective pacing bitrate` 约束（容差窗口内） |

### 4) 端到端链路测试（可选增强）

用于验证真实 SFU/TWCC 反馈路径，而非仅“测试注入 RTCP”。

| ID | 场景 | 关键断言 |
|---|---|---|
| E2E-TWCC-001 | plain-client 推流 + worker 真实 transport-cc feedback | client 侧 feedback 计数持续增长 |
| E2E-TWCC-002 | 限速/丢包网络仿真 | estimate 与发送字节变化方向一致 |
| E2E-TWCC-003 | PauseTrack/audio-only 切换 | pause ack 后无视频 RTP 泄漏（含 retrans） |

## 通过门槛（建议）

1. 单元测试全部通过，且覆盖每个 estimator 分支与每个优先级契约。
2. 组件集成测试覆盖 6 条场景（下降/上升/clamp/异常/吞吐联动）。
3. 分配矩阵 5 档码率全部通过，且输出结果可归档（json/日志）。
4. 至少 1 条 E2E-TWCC 用例稳定通过（增强项）。

## 有效性评估（A/B）

本节用于回答两个问题：

1. 加入 TWCC 闭环后是否优于原路径。
2. “上层 QoS + transport TWCC 估算”是否优于仅依赖上层策略的行为。

### 分组

- Baseline：不包含本次 TWCC send-side 估算闭环的版本（或关闭新估算路径）。
- Candidate：当前版本（启用 transport controller + TWCC 反馈估算）。

要求：

- 两组使用同一测试机、同一网络注入配置、同一输入素材。
- 每个场景至少执行 10 次，报告均值 + P50/P90（避免单次偶然波动）。

### 场景矩阵（A/B 两组都跑）

| ID | 场景 | 目的 |
|---|---|---|
| AB-001 | 稳态高带宽低丢包 | 验证无退化（不应因新闭环明显欠发） |
| AB-002 | 带宽阶跃下降（如 2Mbps→500kbps）+ loss | 验证过冲与拥塞收敛 |
| AB-003 | 带宽阶跃回升（如 500kbps→2Mbps） | 验证恢复速度与稳定性 |
| AB-004 | 高 RTT/高 jitter | 验证抖动工况下稳定性 |
| AB-005 | 多路视频权重发送 | 验证预算分配与公平性 |

### 指标口径

黑盒（A/B 主判定）：

- `Goodput`：服务端 producer 侧有效发送码率。
- `Loss`：`packetsLost/fractionLost`。
- `Recovery Time`：降档后收敛时间、升档后回升到新稳态 80% 的时间。
- `Stability`：码率抖动幅度（窗口方差或 P90-P50）。
- `Fairness`：多路发送占比与配置权重偏差。

白盒（用于定位差异原因）：

- `transportEstimatedBitrateBps`、`effectivePacingBitrateBps`。
- `transportCcFeedbackReports`、malformed feedback 计数。
- `wouldBlockByClass`、`queuedVideoRetentions`、`audioDeadlineDrops`、`retransmissionDrops`。

### 判定标准（建议）

- 拥塞阶段（AB-002）：Candidate 的 `Loss` 与队列压力指标至少改善 20%。
- 恢复阶段（AB-003）：Candidate 恢复时间不劣于 Baseline，目标改善 15%+。
- 稳态阶段（AB-001）：Candidate `Goodput` 不应较 Baseline 下降超过 5%。
- 多路分配（AB-005）：权重偏差不放大，且高权重轨发送份额不下降。
- 回归约束：Pause/Shutdown/Generation 切换相关回归测试必须全绿。

## 执行与归档

建议命令（按场景组合）：

1. `./scripts/run_qos_tests.sh cpp-client-matrix`
2. `./scripts/run_qos_tests.sh cpp-client-harness:threaded_multi_video_budget`
3. `./scripts/run_qos_tests.sh cpp-threaded`

可选：

- 通过 `QOS_MATRIX_SPEED` 调整矩阵时长。
- 对比结果统一归档到 `docs/generated/` 与 `docs/archive/`，并输出一份 A/B 对比表（均值、P50、P90、改善百分比）。
- 报告模板：`changes/2026-04-21-plain-client-sender-transport-control/twcc-ab-result-template.md`
- 指标模板：`changes/2026-04-21-plain-client-sender-transport-control/twcc-ab-metrics-template.json`
- 报告渲染脚本：`node tests/qos_harness/render_twcc_ab_report.mjs --input=<metrics.json> --output=<report.md>`

## 实施顺序

1. 先补单元测试与集成测试（确定性、快反馈）。
2. 再补码率分配矩阵测试（核心验收）。
3. 最后补 E2E-TWCC 增强用例（较慢、依赖环境）。
4. 完成 A/B 对比评估并归档结果，作为“改动有效性”验收输出。
