# 上行 QoS Synthetic 值分析

日期：`2026-04-28`

> **文档性质**
>
> 本文档描述当前仓库中已吸收的 `synthetic-only regression suite` 方法，
> 用于回答：
> 1. synthetic suite 里哪些输入值是人为构造的
> 2. 这些构造值为什么存在
> 3. 哪些参数已经被采纳到当前主线
> 4. 哪些 retained override 只是暂时保留的 legacy forcing
>
> 本文档**不**将这些值解释为现实物理网络的直接等价值。

## 1. 适用范围声明

当前 synthetic suite 的定位是：

- 验证 QoS 状态机 / controller / planner 在**可重复 synthetic 输入**下的行为
- 提高回归稳定性
- 使 synthetic regression 与经验数据更一致

当前 synthetic suite **不用于**：

- 现实网络条件签收
- 物理网络参数直接映射
- 解释 “某个真实 `netem` case 在线上就一定表现为同样的 QoS 状态”

如果后续需要现实网络解释，应使用独立的 `physical-e2e` suite，而不是继续扩大本 synthetic suite 的职责。

## 2. 当前已吸收的 synthetic 参数/机制

### 2.1 Loss synthetic model

当前 synthetic JS 模型中，loss stress 归一化已从：

```js
lossPct / 10
```

调整为：

```js
lossPct / 6
```

同时 loss 权重从 `0.15` 提高到 `0.40`。

**含义**

- 不是在声明“真实 WebRTC 一定等于这个公式”
- 而是在 synthetic suite 中承认：loss 对 GCC 风格 send rate 影响远强于旧模型

### 2.2 RTT amplification

当前 synthetic suite 不再用旧的近似 2× 放大：

```js
baseRtt + min(baseRtt, 100)
```

而是改为对数模型：

```js
baseRtt + 5 + 15 * log2(1 + baseRtt / 50)
```

**含义**

- 低 RTT 场景下保留协议/统计开销
- 高 RTT 场景下逐步趋近 1.0x 放大
- 用于 synthetic 回归，不等价于真实 WebRTC RTT 真值

### 2.3 Jitter smoothing

当前 synthetic suite 使用：

```js
rawJitter * 0.75
```

模拟“reported jitter 往往比 raw network jitter 更平滑”的事实。

**含义**

- 这是 synthetic approximation
- 不是在声明 RFC 3550 或浏览器实现一定产出固定 0.75 倍

### 2.4 Convergence simulation

C++ 侧 `applyMatrixTestProfile()` 现在不再让 synthetic phase 值瞬时跳变，而是对以下字段做指数收敛：

- `sendCeilingBps`
- `roundTripTimeMs`
- `jitterMs`
- `lossRate`

当前时间常数：

- `τ_degrade = 1500ms`
- `τ_recover = 6000ms`

**含义**

- 这是 synthetic-only 的快降慢升建模
- 目的是避免 synthetic phase 切换过于生硬
- 它不是现实 transport 的真实动力学模型

## 3. 当前明确保留的 legacy synthetic forcing

以下规则仍然存在，但其性质是：

> **retained legacy synthetic forcing**

它们用于保持 synthetic suite 的稳定性，不代表现实网络真值。

### 3.1 Low-bandwidth send ceiling override

在 cpp synthetic runner 中，当：

- `phase.name === 'impairment'`
- `group ∈ {bw_sweep, transition}`
- `bandwidth <= 1000`

会额外执行：

```js
sendCeilingBps *= 0.75
qualityLimitationReason = 'bandwidth'
```

**定位**

- 允许暂时保留
- 已有区间型测试约束它不要跑出 synthetic 模型可接受范围
- 但它不是已被完全标定的现实参数

### 3.2 Burst low-bandwidth QLR forcing

当：

- `group === 'burst'`
- `bandwidth <= 300`

会强制：

```js
qualityLimitationReason = 'bandwidth'
```

**定位**

- synthetic trigger
- 不能解释成“真实网络直接产出该字段”

### 3.3 Jitter floor override

当：

- `group === 'jitter_sweep'`
- `jitter >= 40`

会将 `jitterMs` 至少抬到 `32ms`。

**定位**

- 这是 retained legacy forcing
- 仅用于 synthetic suite 的稳定触发

## 4. current synthetic suite 如何构造算法输入

在 cpp synthetic runner 中，数据链路是：

```text
scenario definition
  -> toSyntheticCondition()
  -> buildMatrixTestProfile()
  -> QOS_TEST_MATRIX_PROFILE
  -> applyMatrixTestProfile()
  -> RawSenderSnapshot
  -> deriveSignals()
  -> PublisherQosController::onSample()
```

关键点：

- `applyMatrixTestProfile()` 会改写 `RawSenderSnapshot`
- 因此算法吃到的是 **synthetic input**
- 不是纯 transport 真实观测值

这也是为什么本 suite 只能被称为：

- `synthetic-only regression suite`

而不能被称为：

- `physical network validation suite`

## 5. 质量限制原因（qualityLimitationReason）的边界

在当前 synthetic suite 中：

- `qualityLimitationReason` 是 synthetic trigger signal
- 用来驱动 controller 分支覆盖

它**不是**：

- 物理网络本身的度量
- 真实 browser WebRTC encoder flag 的直接等价物

因此：

- 在 synthetic suite 中它可以保留
- 在 future physical-e2e suite 中它不能被直接借用为现实网络解释信号

## 6. 当前主线已采纳的内容

当前主线已经采纳：

- synthetic JS model recalibration
- C++ convergence implementation
- JS synthetic validation tests
- C++ formula / pipeline / runner-level synthetic tests

当前主线**没有采纳**：

- 直接 merge `copilot/analyze-network-condition-processing` 分支
- 会回退当前主线 harness/runtime/build 修复的旧实现
- 将 synthetic suite 重新包装成现实网络修复的说法

## 7. 如何解释 synthetic suite 的结果

正确解释方式：

> “QoS 算法对这些 synthetic 刺激的响应是否正确、稳定、可重复。”

错误解释方式：

> “真实网络 `2000kbps / 55ms / 0.5% / 12ms` 下算法一定就是这个表现。”

## 8. GCC-oriented expectation 对齐

当前主线已进一步对齐一批 synthetic 用例期望，使其符合更常见的 GCC 直觉，而不是更保守的产品级预警策略。

已放宽的 case：

- `B3`
- `R4`
- `R5`
- `T6`
- `T7`
- `S3`

这些 case 的共同点是：

- 丢包仍然很低（`0.1%` 到 `0.5%`）
- synthetic `qualityLimitationReason` 仍为 `none`
- 主要压力来自稳定 RTT 或稳定 jitter，而不是强带宽不足或高丢包

因此它们现在被解释为：

- 允许保持 `stable`
- 或最多进入 `early_warning`

而不再强制要求 `warning/congested`，避免把“高 RTT 绝对值”直接等价为“拥塞严重度”。

## 8. 后续迁移方向

后续仍然需要独立的 `physical-e2e` suite 来回答：

- configured network
- applied netem
- observed transport stats
- algorithm response

那条 suite 的原则是：

- 算法输入应来自真实观测
- 不再经过 synthetic 覆写

而当前本文档描述的 synthetic suite 将继续承担：

- 回归稳定性
- 分支覆盖
- controller 行为一致性验证
