# mediasoup-worker 3.14.6 架构与模块分析

## 1. 文档目的

这份文档基于当前仓库本地下载的 `versatica/mediasoup` 源码：

- 路径：[src/mediasoup-worker-src](https://github.com/versatica/mediasoup/tree/3.14.6)
- tag：`3.14.6`
- commit：`c042ee7f08ca94ac903f8140f66febb720ef9f91`

目标不是泛泛介绍 mediasoup，而是回答下面这些和当前 `mediasoup-cpp` 项目直接相关的问题：

1. `mediasoup-worker` 进程内部的总体架构是什么。
2. `Worker / Router / Transport / Producer / Consumer` 这些核心模块分别做什么。
3. 媒体面上的 RTP/RTCP、BWE、层切换、score、关键帧请求分别落在哪一层。
4. 它和当前仓库控制面自定义的 QoS/allocator 逻辑边界在哪里。

这份文档尽量遵循“从整体到局部”的顺序。

## 2. 目录与代码布局

`mediasoup-worker` 在这份源码里的核心目录是：

- [worker/src](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src)
- [worker/include](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include)
- [worker/fbs](https://github.com/versatica/mediasoup/blob/3.14.6/worker/fbs)
- [doc](https://github.com/versatica/mediasoup/tree/3.14.6/doc)

按职责划分，可以粗分成 6 层：

1. 进程入口与全局生命周期
   - [lib.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/lib.cpp)
   - [Worker.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/Worker.cpp)
2. Channel/IPC
   - [ChannelSocket.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/Channel/ChannelSocket.hpp)
   - [ChannelMessageRegistrator.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/ChannelMessageRegistrator.hpp)
3. 房间与对象图
   - [Router.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Router.hpp)
   - [Transport.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Transport.hpp)
4. 媒体对象
   - [Producer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Producer.hpp)
   - [Consumer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Consumer.hpp)
   - [SimulcastConsumer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/SimulcastConsumer.hpp)
   - [SvcConsumer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/SvcConsumer.hpp)
5. 网络与传输
   - [WebRtcTransport.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/WebRtcTransport.hpp)
   - [WebRtcServer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/WebRtcServer.hpp)
   - [RtpListener.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/RtpListener.hpp)
6. 拥塞控制与带宽分配
   - [TransportCongestionControlClient.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/TransportCongestionControlClient.hpp)
   - [TransportCongestionControlServer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/TransportCongestionControlServer.hpp)

## 3. 总体架构

可以把 `mediasoup-worker` 看成一个：

- 单进程
- 单 libuv 事件循环
- 以 Channel IPC 为控制入口
- 以 `Worker -> Router -> Transport -> Producer/Consumer` 为对象树
- 以 libwebrtc 的 GCC/TCC、RTP/RTCP 能力为底层依赖

的媒体执行引擎。

### 3.1 顶层对象关系

```text
mediasoup-worker process
  -> Worker
     -> ChannelSocket
     -> RTC::Shared
        -> ChannelMessageRegistrator
        -> ChannelNotifier
     -> WebRtcServer*
     -> Router*
        -> Transport*
           -> Producer*
           -> Consumer*
           -> DataProducer*
           -> DataConsumer*
           -> TCC Client / Server
```

### 3.2 最关键的设计点

`mediasoup-worker` 的核心设计不是“一个大对象管所有事情”，而是：

- `Worker`
  只管进程级资源和顶层对象创建
- `Router`
  只管 producer/consumer/data 的拓扑关系与 fanout
- `Transport`
  管一个具体传输会话上的 Producer/Consumer/Data/BWE/RTCP
- `Producer`
  负责接收媒体、维护接收流与 score
- `Consumer`
  负责发送媒体、维护目标层/当前层/RTCP 反馈

这也是它能把“控制请求”和“媒体执行”分层做干净的原因。

## 4. 进程入口与生命周期

### 4.1 入口：`mediasoup_worker_run`

主入口在 [lib.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/lib.cpp)：

1. 初始化 `libuv`
2. 创建 `ChannelSocket`
3. 初始化 `Logger`
4. 解析 `Settings`
5. 初始化静态依赖：
   - OpenSSL
   - SRTP
   - usrsctp
   - libwebrtc 相关包装
6. 构造 `Worker`
7. 进入 `DepLibUV::RunLoop()`

也就是说，`Worker` 构造成功以后，整个进程就进入一个事件驱动的 libuv loop。

### 4.2 `Worker` 是进程根对象

[Worker.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/Worker.hpp) 和 [Worker.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/Worker.cpp) 说明：

- `Worker` 持有：
  - `ChannelSocket`
  - `SignalHandle`
  - `RTC::Shared`
  - `mapWebRtcServers`
  - `mapRouters`
- `Worker` 自己不持有具体 Producer/Consumer
- 它只管顶层创建和销毁

因此，`Worker` 更像：

- 进程 supervisor
- 顶层对象工厂
- 顶层 Channel dispatcher

而不是媒体算法本体。

### 4.3 `RTC::Shared`

[Shared.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Shared.hpp) 很小，但很重要。

它把两类跨模块共享能力抽出来：

- `ChannelMessageRegistrator`
- `ChannelNotifier`

这意味着对象之间不会互相硬编码“我要怎么回消息”，而是都通过 `Shared` 拿到统一的注册与通知能力。

## 5. Channel / IPC 架构

### 5.1 Channel 是 worker 的控制面入口

[ChannelSocket.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/Channel/ChannelSocket.hpp) 定义了 worker 内部最重要的控制抽象：

- `RequestHandler`
- `NotificationHandler`
- `Listener`

`ChannelSocket` 的作用可以概括成：

- 从父进程收 request/notification
- 反序列化 FlatBuffers
- 根据 `handlerId` 找到目标对象
- 把消息投递给对应对象

### 5.2 `Worker` 只处理顶层 request

[Worker.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/Worker.cpp) 的 `HandleRequest()` 只直接处理少数顶层方法：

- `WORKER_CLOSE`
- `WORKER_DUMP`
- `WORKER_GET_RESOURCE_USAGE`
- `WORKER_UPDATE_SETTINGS`
- `WORKER_CREATE_WEBRTCSERVER`
- `WORKER_CLOSE_WEBRTCSERVER`
- `WORKER_CREATE_ROUTER`
- `WORKER_CLOSE_ROUTER`

剩余 request 都通过：

- `shared->channelMessageRegistrator->GetChannelRequestHandler(handlerId)`

转发给具体对象。

这点很关键：

- 顶层对象创建由 `Worker` 做
- 业务对象自己的 request 由对象自己处理

### 5.3 `ChannelMessageRegistrator`

[ChannelMessageRegistrator.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/ChannelMessageRegistrator.hpp) 本质上是：

- `handlerId -> RequestHandler*`
- `handlerId -> NotificationHandler*`

的注册表。

所以 `mediasoup-worker` 内部对象都有一个非常鲜明的生命周期模式：

1. 构造对象
2. 用自己的 `id` 注册 handler
3. 通过该 `id` 接收后续 request/notification
4. 析构时注销 handler

这比集中式大 switch 更利于扩展。

## 6. 顶层对象图：Worker / WebRtcServer / Router

### 6.1 `Worker`

职责：

- 维护 libuv loop 生命周期
- 管理所有 `Router`
- 管理所有 `WebRtcServer`
- 处理信号与资源统计

不负责：

- RTP 包路由
- BWE 计算
- consumer 层切换

### 6.2 `WebRtcServer`

[WebRtcServer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/WebRtcServer.hpp) 和 [WebRtcServer.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/WebRtcServer.cpp) 说明：

`WebRtcServer` 的职责是：

- 持有一组 UDP socket / TCP server
- 为多个 `WebRtcTransport` 复用 listen socket
- 根据 STUN username fragment 或 tuple，把收到的包路由到对应 `WebRtcTransport`

这意味着它不是“一个 transport”，而是：

- WebRTC 接入层共享 listener

### 6.3 `Router`

[Router.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Router.hpp) 是 worker 内部真正的媒体拓扑中枢。

它维护：

- `mapTransports`
- `mapProducers`
- `mapProducerConsumers`
- `mapConsumerProducer`
- `mapRtpObservers`
- data producer / consumer 对应关系

一句话概括：

`Router` 负责“谁生产、谁消费、怎么 fanout”，而不是“怎么发包”。

## 7. `Router` 的功能边界

### 7.1 创建传输和观察器

`Router::HandleRequest()` 负责创建：

- `WebRtcTransport`
- `PlainTransport`
- `PipeTransport`
- `DirectTransport`
- `ActiveSpeakerObserver`
- `AudioLevelObserver`

因此 `Router` 是 transport 的拥有者。

### 7.2 维护 `Producer <-> Consumer` 拓扑

在 [Router.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/Router.cpp) 里：

- `OnTransportNewProducer()` 把 Producer 加入路由图
- `OnTransportNewConsumer()` 把 Consumer 绑定到 Producer
- `OnTransportConsumerClosed()` / `OnTransportProducerClosed()` 维护拓扑一致性

这层不做复杂算法，但做了非常关键的一件事：

- 把对象之间的引用关系和生命周期对齐

### 7.3 RTP fanout

`OnTransportProducerRtpPacketReceived()` 是 Router 最重要的媒体路径之一。

它会：

1. 找到该 Producer 对应的所有 Consumer
2. 更新 packet MID
3. 调每个 consumer 的 `SendRtpPacket()`
4. 同时把包交给相关 `RtpObserver`

也就是说：

- Producer 收到包以后，并不是 Transport 自己 fanout
- fanout 是 Router 做的

### 7.4 关键帧请求回传

`OnTransportConsumerKeyFrameRequested()` 会把 Consumer 的 keyframe 请求回传给对应 Producer。

这是一个很重要的边界：

- Consumer 决定“我需要关键帧”
- Router 负责把这个意图路由回 Producer
- Producer 再触发真正的上游请求

## 8. `Transport`：最核心的媒体执行层

如果说 `Router` 负责“关系”，那么 `Transport` 负责“执行”。

### 8.1 `Transport` 的职责

从 [Transport.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Transport.hpp) 可以看出，`Transport` 是一个超核心聚合类：

- 拥有 Producer / Consumer / DataProducer / DataConsumer
- 拥有 `RtpListener`
- 拥有 `SctpAssociation`
- 拥有 `TransportCongestionControlClient`
- 拥有 `TransportCongestionControlServer`
- 拥有 RTCP timer

所以可以把 `Transport` 理解成：

- 一个具体网络会话上的媒体执行容器

### 8.2 `Transport` 的主要职责清单

1. 创建和销毁 Producer/Consumer/Data 对象
2. 接收 RTP/RTCP/SCTP 数据
3. 把 incoming RTP demux 到正确的 Producer
4. 把 outgoing RTP/RTCP 发到网络
5. 维护 BWE、desired bitrate、available bitrate
6. 在 consumer 间分配 available bitrate

### 8.3 `Transport` 是 BWE 的承载点

很重要的一点：

- `TransportCongestionControlClient`
- `TransportCongestionControlServer`

都挂在 `Transport` 上。

这意味着 worker 内建的带宽估计与 consumer 分配粒度，主要是：

- transport 级别

不是：

- room 级别
- application UI 级别

## 9. `RtpListener`：incoming RTP demux 核心

[RtpListener.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/RtpListener.hpp) 和 [RtpListener.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/RtpListener.cpp) 负责把 incoming RTP 归到某个 Producer。

它维护 3 张表：

- `ssrcTable`
- `midTable`
- `ridTable`

查找顺序是：

1. SSRC
2. MID
3. RID

并且一旦通过 MID/RID 找到 Producer，会回填 `ssrcTable`。

这意味着 worker 的 incoming RTP demux 不是只依赖 SSRC，而是：

- 先天支持 MID/RID 辅助定位
- 并把第一次解析结果缓存成 SSRC 快速路径

## 10. `Producer`：接收侧媒体对象

### 10.1 `Producer` 的角色

[Producer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Producer.hpp) 对应的是：

- 接收进来的媒体流
- 维护 `RtpStreamRecv`
- 记录每个编码流 score
- 处理 keyframe 请求与 RTCP sender report

### 10.2 `Producer` 做的事

- `ReceiveRtpPacket()`
  处理收到的 RTP
- `CreateRtpStream()`
  为不同 encoding/ssrc 建立接收流
- `OnRtpStreamScore()`
  接收 score 更新
- `RequestKeyFrame()`
  响应下游 Consumer 的关键帧需求

### 10.3 `Producer` 不做的事

它不会：

- 自己决定哪个 Consumer 发哪一层
- 自己做 consumer 间带宽分配

这些都是 `Transport + Consumer` 的职责。

## 11. `Consumer`：发送侧媒体对象抽象

### 11.1 `Consumer` 是一个抽象基类

[Consumer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Consumer.hpp) 定义了非常关键的抽象接口：

- `GetBitratePriority()`
- `IncreaseLayer()`
- `ApplyLayers()`
- `GetDesiredBitrate()`
- `SendRtpPacket()`

这说明 worker 对 Consumer 的建模是：

- Consumer 自己知道自己能升到哪里
- Consumer 自己知道自己想要多少 bitrate
- Transport 负责统一调度这些 Consumer

### 11.2 `externallyManagedBitrate`

`Consumer` 里有一个关键开关：

- `externallyManagedBitrate`

一旦打开，就意味着：

- bitrate 分配不再完全由 consumer 自己即时决定
- Transport 会调用 `IncreaseLayer()` / `ApplyLayers()` 去统一分配

### 11.3 `priority`

`Consumer` 里也直接有：

- `priority`

这和外部 API 的 `consumer.setPriority()` 对应。  
worker 内部会真实使用这个 priority 来参与可用带宽分配，不是纯状态字段。

## 12. `Transport` 如何创建 Consumer 与启用 BWE

### 12.1 `TRANSPORT_CONSUME`

[Transport.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/Transport.cpp) 的 `TRANSPORT_CONSUME` 分支会按 `RtpParameters::Type` 创建：

- `SimpleConsumer`
- `SimulcastConsumer`
- `SvcConsumer`
- `PipeConsumer`

这说明：

- Consumer 类型不是外部随便选的“标签”
- 它决定 worker 内部的层切换策略实现

### 12.2 何时启用 `TransportCongestionControlClient`

在创建 consumer 的过程中，如果满足：

- video consumer
- 具备 transport-cc 或 goog-remb 所需 header extension + RTCP feedback

worker 会创建 `TransportCongestionControlClient`。

然后会：

- 对已有 Consumer 调 `SetExternallyManagedBitrate()`
- 新 Consumer 也调 `SetExternallyManagedBitrate()`

这一步非常关键。

它意味着：

- 只要 transport 上启用了 BWE client
- 该 transport 下的 consumer bitrate/layer 分配，就会进入 worker 内部统一管理模式

## 13. `TransportCongestionControlClient`：发送侧 GCC / pacing 封装

### 13.1 它不是手写 BWE，而是 libwebrtc 封装

[TransportCongestionControlClient.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/TransportCongestionControlClient.cpp) 里明确用了：

- `webrtc::GoogCcNetworkControllerFactory`
- `webrtc::RtpTransportControllerSend`

所以 worker 不是自己重写一套 GCC，而是：

- 用 libwebrtc 的发送侧拥塞控制
- 外面包一层 worker 自己的适配与事件桥接

### 13.2 它负责什么

1. 跟踪 `desiredBitrate / availableBitrate`
2. 对接 RTCP RR / transport feedback
3. 对接 pacer
4. 把可用 bitrate 回调给 `Transport`

### 13.3 它暴露给 `Transport` 的关键量

- `GetAvailableBitrate()`
- `SetDesiredBitrate()`
- `GetPacingInfo()`
- `OnTargetTransferRate()`

所以从 `Transport` 视角看，它更像：

- 一个可用带宽与 pacing 的服务模块

## 14. `TransportCongestionControlServer`：接收侧 feedback 生成

### 14.1 角色

[TransportCongestionControlServer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/TransportCongestionControlServer.hpp) 和 [TransportCongestionControlServer.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/TransportCongestionControlServer.cpp) 负责：

- 收集 incoming RTP 的时间信息
- 生成 `transport-cc` feedback
- 或在 REMB 模式下运行 `RemoteBitrateEstimatorAbsSendTime`

### 14.2 两种模式

- `TRANSPORT_CC`
  - 周期性发送 transport-cc feedback
- `REMB`
  - 用 abs-send-time + REMB 做远端估计

### 14.3 这层的意义

它负责的是：

- 接收侧反馈生成

而不是：

- consumer 间的可用带宽如何分配

真正做“分配”的还是 `Transport`。

## 15. `Transport` 内建的 consumer 带宽分配

这是当前问题里最关键的一层。

### 15.1 `DistributeAvailableOutgoingBitrate()`

[Transport.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/Transport.cpp) 有一个非常直接的内部算法：

- 收集所有 `priority > 0` 的 Consumer
- 从 `tccClient->GetAvailableBitrate()` 取当前可用带宽
- 按 priority 从高到低遍历
- 调 `consumer->IncreaseLayer(availableBitrate, considerLoss)`
- 最后统一 `consumer->ApplyLayers()`

这说明 `mediasoup-worker` 已经内建了：

- transport 级 consumer 间 available bitrate 分配
- 基于 priority 的分配
- layer-by-layer 的升级过程

### 15.2 `ComputeOutgoingDesiredBitrate()`

同一个 `Transport` 还会：

- 汇总所有 consumer 的 `GetDesiredBitrate()`
- 调 `tccClient->SetDesiredBitrate(totalDesiredBitrate, forceBitrate)`

所以 worker 内部是完整闭环：

`desired bitrate -> libwebrtc GCC -> available bitrate -> per-consumer allocation`

## 16. `SimulcastConsumer`：worker 内建层切换逻辑

### 16.1 它不只是执行 `preferredLayers`

[SimulcastConsumer.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/SimulcastConsumer.cpp) 里，`preferredSpatialLayer` / `preferredTemporalLayer` 只是“上界偏好”，不是最终直接下发层。

真正运行时还有：

- `provisionalTargetSpatialLayer`
- `targetSpatialLayer`
- `currentSpatialLayer`
- `lastBweDowngradeAtMs`

这说明它内部自己维护了：

- 目标层
- 当前层
- 刚刚因 BWE 降级过的时间点

### 16.2 内建的稳定性逻辑

这个文件里至少有这几条明确的稳定性机制：

- `StreamMinActiveMs = 2000`
- `BweDowngradeConservativeMs = 10000`
- `BweDowngradeMinActiveMs = 8000`
- 需要 Sender Report 才允许切换到某些 spatial layer
- spatial 切换时会主动请求 keyframe

所以 worker 在单个 consumer 层面已经有：

- dwell / conservative 窗口
- stream active time 门槛
- keyframe / SR gating

### 16.3 `IncreaseLayer()` 做什么

它会：

1. 根据 packet loss 算 `virtualBitrate`
2. 遍历更高 spatial/temporal layer
3. 结合最近 BWE downgrade 时间，避免过快回升
4. 只在 required bitrate 足够时设置 `provisionalTarget*`

### 16.4 `ApplyLayers()` 做什么

`ApplyLayers()` 不直接做搜索，它只是：

- 把 provisional target 提交成真正 target
- 如果像是 BWE 触发的 downgrade，则记录 `lastBweDowngradeAtMs`

### 16.5 `MayChangeLayers()` / `RecalculateTargetLayers()`

除了 transport 统一分配路径，`SimulcastConsumer` 自己也会因为：

- preferred layers 变化
- score 变化
- Sender Report 到达

触发 `MayChangeLayers()`。

但在 `externallyManagedBitrate` 模式下，它不会直接随意改层，而是更多通过：

- `OnConsumerNeedBitrateChange()`

把需求上推给 `Transport`。

## 17. `SvcConsumer`：同类逻辑，适配 SVC

`SvcConsumer` 和 `SimulcastConsumer` 的设计思想基本相同：

- 同样有 `preferred/target/provisional`
- 同样有 `lastBweDowngradeAtMs`
- 同样在 `IncreaseLayer()` 里根据 available bitrate 逐步尝试升级
- 同样在 `ApplyLayers()` 里记录可能由 BWE 造成的 downgrade

差别主要在：

- SVC 流的 spatial/temporal 组合逻辑
- 对 K-SVC 的 bitrate 估算和层切换语义

## 18. 准确的边界判断：worker 已做了什么

基于 `3.14.6` 源码，可以确认 `mediasoup-worker` 已经做了下面这些事情：

1. 传输级 RTP/RTCP/SRTP/ICE/DTLS/SCTP
2. Producer/Consumer 生命周期与包转发
3. score、NACK、PLI/FIR、RR、SR 等基础媒体控制
4. transport 级发送侧 GCC / pacing
5. transport 级接收侧 TCC/REMB feedback 生成
6. transport 内多个 video consumer 的可用带宽分配
7. 单个 Simulcast/SVC consumer 的层切换稳定性控制

这几点很重要，尤其是第 6、7 点。

## 19. worker 没有做什么

worker 仍然**不知道**这些业务语义：

- 页面里哪个 tile 是 `pinned`
- 哪个 consumer 在 UI 上 `visible`
- `targetWidth/targetHeight` 代表的业务价值
- 哪个 subscriber 在房间语义上更重要
- 跨 subscriber 的 producer demand 聚合
- publisher 侧 `pauseUpstream/resumeUpstream` 这样的上层供给策略

这些不是 worker 的职责边界。

因此，worker 内建的带宽分配粒度主要是：

- 单 transport 内的多个 consumer

而不是：

- room 级、多 subscriber、带业务/UI 语义的 planner

## 20. 和当前 `mediasoup-cpp` 控制面的关系

这部分是对当前项目最关键的结论。

### 20.1 当前控制面没有重写 worker 的底层媒体算法

你们当前项目的控制面：

- 通过 [Consumer.cpp](../src/Consumer.cpp) 把 `pause/resume/setPreferredLayers/setPriority` 发给 worker
- 通过 [Producer.cpp](../src/Producer.cpp) 观测 score 与 stats
- 通过 [RoomService.h](../src/RoomService.h) 和 `RoomServiceStats.cpp` / `RoomServiceDownlink.cpp` 做 room 级管理与 QoS 聚合

所以当前架构是：

- worker 负责 transport/media execution
- `mediasoup-cpp` 负责 room/session/QoS control plane

### 20.2 对 `Q8` 的直接影响

这份源码分析会带来一个很重要的修正：

当前你们在控制面实现的下行 allocator，不能再假设：

- worker 只是被动执行 `preferredLayers`

实际上 worker 已经会在 transport 级做：

- available bitrate 分配
- priority 驱动的 consumer 升降层
- consumer 自身的 anti-flap 逻辑

所以后续如果继续推进控制面上的 `Q8`，要先回答一个边界问题：

1. 我们的控制面 allocator 是不是在重复 worker 已有能力。
2. 我们的控制面 allocator 是不是在和 worker transport-level allocation 打架。
3. 控制面真正该保留的，是不是 room-level / UI semantic / producer-demand 这一层。

### 20.3 当前更合理的职责划分

结合 worker 源码，我认为更合理的职责划分是：

- `mediasoup-worker`
  - transport 级 BWE
  - consumer 间 available bitrate 分配
  - simulcast / SVC 层切换与局部抗抖
- `mediasoup-cpp` 控制面
  - room 级业务语义
  - subscriber UI hints 到 worker 控制参数的映射
  - producer demand 聚合
  - publisher supply / clamp / pauseUpstream / resumeUpstream

## 21. 推荐阅读顺序

如果后面还要继续读 worker 源码，建议按下面顺序：

1. [worker/src/lib.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/lib.cpp)
2. [worker/src/Worker.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/Worker.cpp)
3. [worker/include/Channel/ChannelSocket.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/Channel/ChannelSocket.hpp)
4. [worker/include/RTC/Router.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Router.hpp)
5. [worker/include/RTC/Transport.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Transport.hpp)
6. [worker/include/RTC/RtpListener.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/RtpListener.hpp)
7. [worker/include/RTC/Producer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Producer.hpp)
8. [worker/include/RTC/Consumer.hpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/include/RTC/Consumer.hpp)
9. [worker/src/RTC/TransportCongestionControlClient.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/TransportCongestionControlClient.cpp)
10. [worker/src/RTC/SimulcastConsumer.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/SimulcastConsumer.cpp)
11. [worker/src/RTC/SvcConsumer.cpp](https://github.com/versatica/mediasoup/blob/3.14.6/worker/src/RTC/SvcConsumer.cpp)

## 22. 结论

一句话总结：

`mediasoup-worker 3.14.6` 不是一个“只转发 RTP 的黑盒”，它已经内建了 transport 级拥塞控制、consumer 可用带宽分配、simulcast/SVC 层切换和一定程度的 anti-flap 逻辑；但它不理解 room 级业务语义，也不替控制面做 producer demand / publisher supply 这一层。`

对当前项目最重要的启发是：

- 以后讨论 downlink allocator 时，不能把 worker 当成纯执行器
- 也不能把 worker 已有的 transport-level 分配能力和控制面 room-level 业务规划混成一层

这两个边界如果不先分清，后面无论是继续做 `Q8`，还是改 downlink QoS 架构，都会反复踩到“重复实现”或“控制冲突”的问题。
