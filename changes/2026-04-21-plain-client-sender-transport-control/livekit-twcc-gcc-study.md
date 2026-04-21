# Plain Client 对齐 LiveKit 的 TWCC/GCC 专项研究

## 1. 研究目标

在不推翻现有 plain-client 线程模型与 QoS 策略层的前提下，使发送侧拥塞控制能力与 LiveKit/WebRTC 经典路径对齐：

- 传输内环：`transport-cc(TWCC) + send-side BWE + pacing/probing`
- 策略外环：现有 QoS 状态机继续负责编码/档位/AudioOnly/Pause

关键结论：应采用“两层协同”，不是“二选一”。  
内环决定“现在能发多少”，外环决定“发什么质量”。

## 2. 对齐基线（LiveKit / WebRTC）

### 2.1 WebRTC 基线

- `RtpTransportControllerSend` 接收 `OnTransportFeedback`，驱动 GCC 估计与 pacer 更新。
- `GoogCcNetworkControllerFactory` 提供标准 send-side 控制器与 probe/pacing 协同。

### 2.2 LiveKit 基线

- `SendSideBWE` 通过 `HandleTWCCFeedback(*rtcp.TransportLayerCC)` 消化 feedback。
- 发送时记录 packet send 信息并分配 transport sequence。
- `connectionquality` 是独立的质量评分/策略信号层，不替代 TWCC 内环。

结论：LiveKit 同样是“内环 BWE + 外环质量策略”分层，而非单一 QoS 统计控速。

## 3. 当前 plain-client 状态（截至本次提交）

### 已具备

- 已有发送执行层边界：`SenderTransportController`
- 已有 class priority、WouldBlock/HardError 语义、回滚开关、队列上限
- 已有 QoS 策略层（EWMA + 状态机 + planner/executor）

### 关键缺口

- RTP 未写 transport-wide sequence 扩展（TWCC 前提缺失）
- RTCP 未解析 `RTPFB fmt=15` transport feedback
- 发送 budget 仍由 `targetBitrateBps` 累加驱动，不是 feedback 闭环估计
- probe 仍是 QoS 层探测，不是发送内环 probe cluster

## 4. 目标架构（对齐口径）

### 4.1 分层职责

- 传输内环（新增）：
  - 记录 sent packet（sendTime/size/seq/class/isRTX）
  - 解析 TWCC feedback，计算 `bweEstimateBps`
  - 输出 pacing budget / probe plan
- 策略外环（保留）：
  - 继续做状态机与编码层动作
  - 不直接替代内环估计

### 4.2 统一预算公式

`effectiveBudgetBps = min(bweEstimateBps, qosTargetAggregateBps, appCapBps)`

- `bweEstimateBps`：内环实时估计（主约束）
- `qosTargetAggregateBps`：外环策略目标（编码期望）
- `appCapBps`：配置上限/运营限制

### 4.3 控制优先级

- Pause/AudioOnly 继续是强控制，立即生效（覆盖发送队列与重传）
- BWE 不应绕过暂停语义
- QoS 降档不应强行抬高内环预算

## 5. 实施路线（建议分四期）

## Phase 0: 协议与观测打底

- 明确并固化 extmap 中 transport-cc id 的来源与校验
- 增加 TWCC 开关与诊断字段（协商成功率、feedback 到达率、estimate 更新频率）
- 保持可回滚：内环可随时退回当前 target-based budget

验收：

- transport-cc 协商成功时可观测到启用日志/指标
- 未协商或异常时自动回退且不中断发送

## Phase 1: TWCC wire-up

- RTP 发送端写 transport-wide sequence 扩展
- 维护发送包历史窗口（按 sequence 索引）
- RTCP 增加 `RTPFB fmt=15` 解析路径并生成 feedback 事件

验收：

- 单测覆盖 sequence 回绕、乱序 feedback、重复 feedback
- 集成测试可观测 feedback 驱动的 packet ack/loss 对账正确

## Phase 2: 内环 BWE 与 pacing/probe

- 增加 send-side BWE 模块（职责对齐 LiveKit 的 sendsidebwe）
- budget 从 target-based 改为 estimate-based
- 引入基础 probe cluster 生命周期（start/done/finalize）

验收：

- 限速场景下 estimate 可收敛且不持续过冲
- 突发丢包/时延抖动下有可解释的降速与恢复轨迹

## Phase 3: 与 QoS 外环联调去振荡

- 统一 QoS 与 BWE 的采样周期/冷却窗口
- 对 `min(bwe, qos)` 策略加入抗振荡约束（hysteresis / hold-down）
- 对 audio/video/retransmission 分配做跨码率验证

验收：

- 无“内环升速、外环立刻降档”频繁打架现象
- Pause/Resume/AudioOnly 与重传优先级在闭环下持续满足

## 6. 专项测试矩阵

## 6.1 单元测试

- TWCC RTP 扩展写入/解析正确性
- feedback 解析正确性（丢包、乱序、回绕）
- BWE 状态转移与估计边界（最小/最大/恢复）

## 6.2 线程与集成测试

- 不同码率档位下，各 class 发送占比与顺序符合策略
- WouldBlock 持续压力下，队列有界且无 pause 后视频泄漏
- fallback 开关行为与新路径等价性

## 6.3 端到端网络仿真

- 维度：带宽(0.3/0.8/1.5/3/5 Mbps)、RTT(20/80/150 ms)、loss(0/2/5/10%)、jitter
- 指标：
  - 收敛时间
  - overshoot/undershoot
  - 重传占比
  - 音频时延与丢包
  - 视频关键帧恢复时间

## 7. 风险与约束

- 若直接追求算法“字节级一致”，实现成本高且引入外部耦合；建议先追求行为/分层一致
- TWCC 协商失败与反馈缺失必须有稳定回退路径
- 内环与外环并存时的振荡风险高，必须先设计抑振策略再全面放量

## 8. 里程碑交付物

1. `SenderTransportController` 接入 TWCC feedback 事件接口与估计输入面
2. `RtcpHandler/NetworkThread` 完成 fmt=15 解析与事件分发
3. 新增 `SendSideBwe`（或等价命名）模块与测试
4. 新增闭环回归脚本与报告模板（可复现实验参数）
5. 文档更新：明确“QoS 外环 + BWE 内环”架构基线

## 9. 决策建议

建议采用“行为对齐 LiveKit”的实施策略：

- 架构与职责对齐（内环/外环分层）
- 接口与验证标准对齐（TWCC feedback、estimate、probe、pacing）
- 不要求第一阶段复制 LiveKit 的每个内部算法细节

这样可以在可控风险下最快补齐 plain-client 关键闭环，并保留后续迭代到更高拟合度的空间。
