# PlainTransport C++ Client QoS 边界复盘

> 2026-04-22 更新
>
> 这份文档记录的是 sender QoS 重写前的边界风险清单。
> 当前代码已经完成下面三项收敛：
> - sender QoS 控制链移除了 server `getStats`
> - sender snapshot contract 已改为 `CanonicalTransportSnapshot`
> - sender QoS runtime 已收敛到 threaded 主路径
>
> 下文保留为历史问题档案，用来解释为什么这些重构是必要的。

## 1. 文档目的

这份文档不回答“plain-client 已经通过了哪些 case”，而是回答另一个更重要的问题：

- 从代码和原理上看，当前 `PlainTransport C++ client QoS` 还有哪些边界风险值得优先盯住。

它的目标是给后续修复提供一个稳定的本地问题清单，而不是依赖聊天记录重复回忆。

## 2. 范围

这里关注的是 sender-side QoS 的输入边界和信号推导边界，尤其是：

- local RTCP 统计
- server `getStats`
- matrix/synthetic profile 注入
- `deriveSignals()` 的 counter/delta 假设
- legacy / threaded 两条 plain-client 路径的一致性

这份文档不评估：

- browser downlink QoS
- worker 内部媒体算法本体
- “昨天某个 case 是否跑过”这种经验性结论

## 3. 总结判断

当前 plain-client QoS 最大的风险，不像是“某个控制器公式显然写错了”，而更像是：

- 不同来源的 sender stats 在语义上并不完全同构
- 但它们会被送进同一套 QoS baseline / delta / EWMA 逻辑

最值得优先关注的不是单个 case，而是下面这些边界契约问题：

1. `packetsLost` 在 worker/server/plain-client 三层的语义不完全一致。
2. `lossBase` 逻辑默认把 `packetsLost` 当成非负、单调递增累计计数器。
3. `deriveSignals()` 默认把所有 counter 回退都吞成 `0 delta`，这要求上游输入必须满足严格的单调语义。
4. local RTCP 和 server stats 切换时，只做了部分 baseline 对齐，不是完整的源间归一。
5. legacy 和 threaded plain-client 都复制了类似逻辑，因此边界错误可能双份存在。

## 4. 高置信度问题

### 4.1 `packetsLost` 契约不一致

#### 观察

worker/server 边界今天已经开始朝 signed loss 语义靠拢，但 plain-client 解析 server stats 时仍把 `packetsLost` 当成无符号值：

- `client/PlainClientSupport.cpp`
- `client/PlainClientSupport.h`

关键点：

- `readUint64Metric()` 会拒绝负值
- `ServerProducerStats::packetsLost` 是 `uint64_t`

这说明当前 plain-client 隐含契约是：

- server `packetsLost` 必须是非负数

#### 风险

如果 worker/server 层暴露的 `packetsLost` 语义允许下降或需要 signed 表达，那么 plain-client 会在边界层直接把这种值吞掉或改写为 `0`。

这不一定立刻导致明显故障，但它代表：

- sender QoS 输入契约已经和上游不完全一致

#### 相关代码

- `client/PlainClientSupport.cpp`
- `client/PlainClientSupport.h`

### 4.2 `lossBase` 假设 `packetsLost` 单调递增

#### 观察

legacy 与 threaded 两条路径都有相同模式：

- 第一次取 server `packetsLost` 作为 `lossBase`
- 后续如果 `parsed.packetsLost >= lossBase`，做减法得到相对值
- 否则认为它“回退”了，直接重置 base，并把当前 loss 归零

相关代码：

- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`
- `client/PlainClientApp.h`

#### 风险

这套逻辑在下面这个前提下才是完全安全的：

- server `packetsLost` 是非负、单调递增累计计数器

如果这个前提不成立，那么当前实现会把“合法下降”或“语义不一致”直接归类为：

- counter reset
- 本轮 loss = 0

这会让 QoS 状态机看到一个被清洗过的 loss 信号，而不是源统计的真实变化。

### 4.3 `deriveSignals()` 把所有 counter 回退都吞成 `0`

#### 观察

无论 JS 版还是 C++ 版，sender QoS 都在用同一条基本规则：

- `current <= previous` 时，counter delta = `0`

相关代码：

- `client/qos/QosSignals.h`
- `src/client/lib/qos/signals.js`

#### 风险

这条规则本身不一定错，但它要求一个强前提：

- 所有输入 counter 都必须是单调递增的

一旦输入来自不同来源，或者来源之间的“累计语义”不完全一样，那么 `deriveSignals()` 会把这些不一致全部静默吞掉。

这意味着：

- 算法表面稳定
- 但问题会被隐藏在输入边界，而不是暴露成显式错误

### 4.4 local RTCP 和 server stats 只做了部分归一

#### 观察

legacy 与 threaded 两条路径都先尝试用 local RTCP 填 `RawSenderSnapshot`，再在有 server `getStats` 时覆盖：

- `bytesSent`
- `packetsSent`
- `packetsLost`
- `roundTripTimeMs`
- `jitterMs`

相关代码：

- `client/PlainClientLegacy.cpp`
- `client/PlainClientThreaded.cpp`

切换来源时，代码已经意识到风险，所以做了：

- `lossCounterSource`
- `primeSnapshotBaseline(qSnap)`

#### 风险

这说明实现已经知道 source switch 是边界风险点，但目前只做了“尽量不炸”的补偿，不是完整统一契约。

更具体地说：

- `packetsLost` 有 `lossBase`
- `bytesSent` / `packetsSent` 没有独立 source-normalization 逻辑
- 最终仍然依赖 `counterDelta()` 把异常回退吞成 `0`

这更像一种工程上可运行的折中，而不是严格定义好的信号模型。

### 4.5 同类逻辑在 legacy/threaded 双份存在

#### 观察

`PlainClientLegacy.cpp` 和 `PlainClientThreaded.cpp` 里都存在同类的：

- server stats 解析
- `lossBase` 处理
- source 切换补偿

#### 风险

这意味着 plain-client QoS 的边界契约问题不是只存在于旧路径或新路径，而是：

- 两条路径都可能一起带着同样的问题演进

因此，后续如果要修，最好优先抽出统一边界层，而不是继续分别修两份。

## 5. 中等置信度审计点

下面这些问题目前更像“值得继续审计”的方向，还不适合直接下结论说已经是 defect。

### 5.1 `parseServerProducerStats()` 的“best stat”选择策略

当前解析逻辑会在同一 producer 的 `stats[]` 里挑一个“最好”的 `inbound-rtp` 条目，比较依据包括：

- `byteCount`
- `packetCount`
- `bitrateBps`

代码在：

- `client/PlainClientSupport.cpp`

如果将来 server 侧一个 producer 对应的 `stats[]` 结构更复杂，这种“择优选择”策略未必稳定。

### 5.2 `roundTripTimeMs` / `jitterMs` 单位依赖边界约定

当前 plain-client 对 server stats 的 RTT 使用了：

- `normalizeStatsTimeToMs()`

但实现现在是 identity。

这本身不一定错，前提是：

- server stats 已经保证 `roundTripTime` 是毫秒

这条需要和 server stats contract 一起维护，否则会再次出现“边界自己猜单位”的问题。

## 6. 现有测试和结果能证明什么、不能证明什么

### 能证明的

- 当前业务场景下，主路径在既有 case 上可以跑通
- 当前 QoS 控制器在已覆盖场景里没有明显破坏
- local RTCP 负 cumulative lost 至少有独立解析保护

### 不能直接证明的

- worker/server/plain-client 对同一 counter 的语义完全一致
- source switch 后所有 sender metrics 都被严格归一
- `packetsLost` contract 在未来 worker 变更后仍然成立

因此，即使 case 全部通过，也不能自动推出：

- 当前 sender QoS 边界是“原理上干净”的

## 7. 建议的后续修复顺序

如果后续继续做 plain-client QoS 深修，我建议按下面顺序处理。

### 第一优先级

明确 `packetsLost` 在 plain-client 边界层的正式契约：

- 到底要求它是非负、单调递增累计值
- 还是允许 signed / 回落语义

然后把：

- `PlainClientSupport`
- `RawSenderSnapshot`
- `deriveSignals`

三层统一到同一套约定。

### 第二优先级

把 legacy / threaded 里重复的 server-stats boundary 逻辑收敛成公共 helper，避免双份漂移。

### 第三优先级

把 source switch 的 baseline 逻辑从“补丁式修补”升级成明确的边界归一策略，至少统一：

- `bytesSent`
- `packetsSent`
- `packetsLost`
- RTT / jitter 来源优先级

## 8. 结论

当前 plain-client QoS 的主要风险，不是“控制器主逻辑写错”，而是：

- 不同 stats 来源的边界契约没有完全统一

其中最明确、最值得优先修的点是：

- plain-client 仍把 server `packetsLost` 当成无符号、单调递增累计值处理

这在 worker/server 语义进一步演进后，很容易再次变成今天同类的问题。
