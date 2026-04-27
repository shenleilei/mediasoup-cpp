# 纯 C++ 客户端发送端带宽估计 (Plain-Client Send-Side BWE)

## 适用范围 (Scope)

本规范描述了在以下条件成立时，纯 C++ 客户端 (plain-client) 发送端带宽估计（send-side bandwidth-estimation）机制已被认可的运行时行为：

- `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`
- `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1`

本规范不涉及浏览器端（browser-side）的 QoS 行为，也不包含 LiveKit 的全量下行分配器上下文（downlink allocator context）。

## 认可的行为规范 (Accepted Behavior)

### 传输范围的序列号稳定性 (Transport-Wide Sequence Stability)

- 在本地发生 `WouldBlock` 重试后，排队中的 RTP 数据包**必须**保持传输范围内的序列号（transport-wide sequence number）稳定不变。
- 失败的本地发送尝试**不能**为同一个排队中的数据包分配新的传输范围序列号。
- 只有成功发送的数据包，才能进入发送端 BWE (send-side BWE) 的已发送数据包追踪器（sent-packet tracker）。

### TWCC 反馈处理 (TWCC Feedback Handling)

- 解析 transport-cc 反馈时，**必须**展开参考时间周期（reference-time cycles），并且在支持的主处理路径上能够容忍乱序的报告。
- 对外发布的传输估计值 (transport estimate) **必须**来自于 send-side BWE 路径，而不是本地备用的回退评估器（fallback estimator）。

### 探测流隔离 (Probe Isolation)

- 探测（Probe）RTP 包**严禁**进入常规视频的重传数据包存储库。
- 探测 RTP 包**严禁**被常规媒体的 NACK 重传路径选中。
- 探测 RTP 包**严禁**导致常规视频的字节计数虚高（inflate octet accounting）。

### 探测流生命周期 (Probe Lifecycle)

- 网络线程 (network-thread) 的平滑发送（pacing）路径**必须**支持：
  - 启动探测集群 (probe cluster start)
  - 附带 transport-cc 扩展的探测 RTP 发送
  - 目标达成时提前停止 (goal-reached early stop)
  - 探测结束化 (probe finalization)
- 探测触发目标（Probe-trigger goal）的构造逻辑，**必须**遵循 LiveKit 默认的填充探测（padding-probe）策略在发送端的等效实现：
  - `availableBandwidthBps` 来自已发布的传输估计值
  - `expectedUsageBps` 来自当前发送端的实际使用量
  - `desiredIncreaseBps = max(transitionDeltaBps * ProbeOveragePct / 100, ProbeMinBps)`
  - `desiredBps = expectedUsageBps + desiredIncreaseBps`

### 观测性 (Observability)

- 线程化的纯客户端（plain-client）追踪路径**必须**暴露传输估计值（transport-estimate）以及用于进行针对性效果审计所需的白盒字段，包括：
  - 发送端实际带宽消耗 (sender-usage bitrate)
  - 传输预估带宽 (transport-estimate bitrate)
  - 实际发包的速率限制 (effective pacing bitrate)
  - transport feedback 计数
  - 探测活动状态及探测包计数
  - 队列积压与重传计数

## 明确排除的非目标 (Explicit Non-Goals)

- 本规范**不**主张与 LiveKit 下行链路中的 `streamallocator` 对象模型达到完全对等。
- 上行链路中用于承载探测 RTP 的载体轨道选择（Carrier-track selection），仅是发送端特定上下文中的等效映射，而**并非**字面意义上对 LiveKit 缺陷轨道选择逻辑的直接照搬。
- 本规范**未**定义在全局传输带宽上限下、基于权重的多轨道媒体调度器（multi-track media scheduler）；现有的多轨道平滑发送（pacing）和公平性行为仍然在此认可范围之外。