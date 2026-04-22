# PlainTransport C++ Client 对齐 LiveKit 的整体规划

> 2026-04-22 更新
>
> sender QoS 重写的前四个 wave 已经完成基础 cutover：
> - sender QoS 控制输入已收敛为本地 `CanonicalTransportSnapshot`
> - server `getStats` 已退出 sender QoS 主控制链
> - sender QoS runtime 已收敛到 threaded 主路径
>
> 下文保留的是继续向 LiveKit 最终态演进时的规划背景，不表示当前代码仍保留这些旧边界。

## 1. 文档目的

这份文档回答的是：

- 如果 plain-client sender QoS 继续往 LiveKit 方向对齐，下一步整体该怎么做。

它不重复历史实现过程，也不以“某批 case 是否通过”作为主要依据，而是基于当前代码结构和已确认的边界问题，给出一个可执行的整体规划与实施方案。

## 2. 核心判断

如果要按最高标准对齐 LiveKit，这件事应该按**替换式重构**来做，而不是按兼容式演进来做。

也就是说：

- 不保旧 sender QoS 边界逻辑
- 不保 legacy / threaded 双路径长期共存
- 不继续让多来源 stats 混合喂给一个控制链

最终只保留一套新的 sender-side architecture。

一句话概括目标架构：

- **传输内环**负责“现在能发多少”
- **策略外环**负责“发什么质量”
- **观测层**只负责观测和对账，不参与 sender 控制决策

## 3. 当前架构和 LiveKit 的关键差距

### 3.1 LiveKit 的做法

从当前本地可见的 LiveKit 源码看：

- 发送侧 BWE 主闭环依赖本地 packet send history 和 TWCC feedback
- `SendSideBWE` 通过 `RecordPacketSendAndGetSequenceNumber()`、`HandleTWCCFeedback()`、`UpdateRTT()` 驱动内环
- 它不会把“client 本地 QoS snapshot + server getStats”混成一个 transport control 输入

这意味着 LiveKit 在 sender BWE 主路径上更接近：

- 单一内部真相源

### 3.2 规划起点时 plain-client 的差距

当前 plain-client 虽然已经具备：

- `NetworkThread`
- `SenderTransportController`
- `SendSideBwe`
- transport-cc / TWCC 主路径

但 sender QoS 输入层仍存在这些混合源：

- local RTCP
- server `getStats`
- matrix/synthetic 注入

这些来源最终会进入：

- 旧 sender snapshot contract（现已收敛为 `CanonicalTransportSnapshot`）
- `deriveSignals()`
- QoS 状态机 / planner / executor

这就是当前和 LiveKit 最本质的差距。

## 4. 最终态架构

### 4.1 传输内环：唯一真相源

最终态里，sender transport state 只来自本地传输路径：

- `client/NetworkThread.h`
- `client/RtcpHandler.h`
- `client/sendsidebwe/SendSideBwe.h`
- `client/SenderTransportController.h`

这层负责：

- send history
- transport-wide sequence
- TWCC feedback 对账
- RTT
- jitter
- loss 的 transport 视角
- transport estimate
- effective pacing
- retransmission scheduling
- probe lifecycle
- queue pressure

### 4.2 策略外环：只做质量决策

QoS 外环只负责：

- level
- audio-only
- pause / resume
- encoding parameters
- source adaptation
- cooldown / hysteresis

它不再负责：

- 推断 transport truth
- 混合多来源 sender stats
- 用 server stats 直接驱动 sender 控制

### 4.3 观测层：不进控制链

下面这些数据保留，但退出 sender QoS 主控制链：

- server `getStats`
- trace
- report
- matrix/synthetic profile

这里还要区分两类：

- `server getStats`：运行时观测 / 对账 / debug
- matrix/synthetic profile：测试注入

它们都不再是 sender control truth。

其中：

- `server getStats` 必须退出生产控制路径
- `matrix/synthetic profile` 必须作为测试注入机制存在，不得继续留在生产 sender QoS 运行时路径中充当普通输入源

### 4.4 最终态里内环/外环的数据从哪里来

#### 内环数据来源

只来自：

- 本地发送事件
- 本地 RTCP feedback
- 本地 TWCC feedback
- 本地队列/调度状态

不来自：

- server `getStats`
- report 聚合
- harness 注入数据

#### 外环数据来源

只来自：

- 内环输出的 canonical transport snapshot
- policy / thresholds / ladder
- manual override / server override /业务输入

#### 外环里的延迟、丢包、抖动从哪里来

最终态里这三项都不该由外环自己采集，而应由内环统一提供：

- 延迟：来自本地 RTCP feedback 解析出的 RTT
- 丢包：来自本地反馈链路或 TWCC/发送对账后统一归一的 loss 信号
- 抖动：来自本地 RTCP RR / 相关 feedback 解析并统一单位后的 jitter

外环绝不直接使用：

- server `getStats` 覆盖值

### 4.5 最终态唯一允许的主控制链

```text
本地发送/反馈
  -> 内环 canonical transport snapshot
  -> 外环 derive policy signals
  -> 编码/档位动作
```

以下旧模式应被彻底淘汰：

```text
local RTCP + server getStats + synthetic profile
  -> 混合 snapshot
  -> deriveSignals
```

## 5. 实施原则

### 5.1 行为对齐，不追求代码逐行模仿

目标不是复制 LiveKit 的每个内部文件和类型，而是：

- 对齐架构职责
- 对齐控制闭环
- 对齐信号边界

### 5.2 不做兼容设计

这里的前提已经明确：

- 原来的 sender QoS 代码不要了

所以实现阶段不应该围绕这些目标展开：

- 保持 legacy 路径继续承接主 QoS 演进
- 保持 legacy / threaded 双路径长期共存
- 继续维护旧的 source-mixing / `lossBase` / 覆盖逻辑

允许存在的“过渡态”只有一种：

- 开发分支内，为了完成新路径而短暂共存

但这种共存不是目标架构的一部分。

### 5.3 先定义新契约，再做新路径，再切换，再删旧代码

正确顺序必须是：

1. 冻结新的 sender contract
2. 做出新的 transport authority
3. 重写 QoS 输入层
4. 切到新路径
5. 删除旧代码

而不是：

- 一边保兼容一边打补丁

## 6. 分阶段实施方案

## Phase 0：冻结最终 contract 和删除清单

### 目标

先把最终保留什么、最终删除什么写死。

### 主要工作

- 冻结新的 sender transport truth contract
- 冻结新的 policy input contract
- 列出 cutover 后必须删除的旧逻辑：
  - server `getStats` 对控制快照的覆盖
  - `lossBase`
  - legacy / threaded 双份 boundary 逻辑
  - 任何 source-mixing sender QoS 逻辑

### 涉及模块

- `client/qos/QosTypes.h`
- `client/qos/QosSignals.h`
- `client/PlainClientSupport.*`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`

### 完成标志

- 后续实现不再需要讨论“旧逻辑要不要保”

## Phase 1：做新的 transport authority

### 目标

先把新的 sender transport authority 做出来。

### 主要工作

- 定义新的 canonical sender transport snapshot
- 让下面组件围绕它工作：
  - `NetworkThread`
  - `RtcpHandler`
  - `SendSideBwe`
  - `SenderTransportController`
- 新路径不依赖：
  - server `getStats`
  - 旧 `lossBase`
  - 旧 source-switch 补丁

### 涉及模块

- `client/ThreadTypes.h`
- `client/NetworkThread.h`
- `client/RtcpHandler.h`
- `client/sendsidebwe/*`
- `client/SenderTransportController.h`

### 完成标志

- sender transport truth 已可完全由新路径提供

## Phase 2：重写 policy 输入层

### 目标

让 QoS 外环只消费 canonical transport signals。

### 主要工作

- 重写 sender snapshot contract / `DerivedSignals` 的语义
- 去掉“多来源累计 counter 混合再做 delta”的旧思路
- 让 source switch 在边界层一次性解决，不再由状态机和补丁逻辑兜底

### 涉及模块

- `client/qos/QosTypes.h`
- `client/qos/QosSignals.h`
- `client/qos/QosController.h`
- `client/qos/QosCoordinator.h`

### 完成标志

- QoS 外环不再直接感知 local/server/source-mixing 细节

## Phase 3：切换主运行路径

### 目标

把 sender QoS 的实际运行入口切到新路径。

### 主要工作

- `PlainClientThreaded.cpp` 改接新 transport + policy 边界
- `PlainClientThreaded` 成为唯一 sender QoS 运行入口
- legacy 路径停止承接新 sender QoS 逻辑
- 旧覆盖逻辑退出主控制链

### 涉及模块

- `client/PlainClientThreaded.cpp`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientApp.cpp`

### 完成标志

- `PlainClientThreaded` 成为唯一 sender QoS 运行入口
- 主运行路径只走新架构

## Phase 4：删除旧 sender QoS 运行时代码

### 目标

在 cutover 之后真正删掉旧代码。

### 主要工作

- 删除旧的 server-stats 覆盖控制链逻辑
- 删除 `lossBase`
- 删除 legacy/threaded 双份 sender boundary 逻辑
- 删除只为旧 sender QoS 契约存在的状态字段和 helper

### 涉及模块

- `client/PlainClientSupport.*`
- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`
- `client/PlainClientApp.h`

### 完成标志

- sender QoS 运行时代码只剩一套主路径

## Phase 5：收尾与稳态化

### 目标

在删除旧路径后，补齐文档、trace、验证入口。

### 主要工作

- 更新 accepted behavior
- 更新架构文档
- 更新 matrix / harness / trace 的解释口径
- 收敛不再需要的过渡字段和说明

### 完成标志

- 新架构成为唯一受支持口径

## 7. 风险与取舍

### 风险 1：短期内会失去一部分 server-side 观测参与控制的“补偿效果”

现在 server `getStats` 混入控制链，某些 case 下可能帮忙“补齐”本地指标。

移除后，短期内可能暴露：

- 本地 transport snapshot 本身还不完整

所以必须先做新的 transport authority，再做切换。

### 风险 2：`lossBase` 拔得太早会导致 sender loss 抖动

如果在新 transport authority 尚未稳定前就删掉 `lossBase`，会让旧补丁失去兜底作用。

因此：

- `lossBase` 只能在 cutover 后和旧路径一起删除

### 风险 3：过度模仿 LiveKit 细节会把实现复杂度拉高

本仓库不需要逐文件模仿 LiveKit，只需要：

- 对齐“内环 / 外环 / 观测层”三层职责

## 8. 当前建议的实施顺序

如果现在就开始做，建议严格按下面顺序开工：

1. **冻结最终 contract 和删除清单**
2. **做新的 transport authority**
3. **重写 policy 输入层**
4. **切换主运行路径**
5. **删除旧 sender QoS 运行时代码**

## 9. 结论

如果直接对齐 LiveKit，当前最该做的不是继续调 QoS 阈值，而是：

- 按替换式重构重建 sender-side QoS
- 先做新的 transport inner loop authority
- 再切控制入口
- 最后删掉旧代码，而不是继续兼容

这件事做完以后，后面的：

- BWE 参数调优
- QoS 阈值调整
- 运行时行为收敛

才会真正站在一个干净的架构基础上。
