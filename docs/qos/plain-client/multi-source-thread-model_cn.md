# Linux PlainTransport C++ Client 多视频源推流迁移设计

> 2026-04-22 更新
>
> 主 `plain-client` 二进制已经收敛为 threaded-only runtime。
> 文中涉及 `copyMode`、legacy 单线程入口和阶段性双路径灰度的内容，保留为演进历史背景，不再代表当前主实现仍支持这些入口。

## 1. 文档目的

这份文档描述的是 Linux `PlainTransport C++ client` 从当前单主线程媒体循环演进到多线程推流架构的目标设计与迁移约束。

它不只是线程模型说明，还回答下面这些实现问题：

- 当前代码到底哪些地方仍然是单线程推进，哪些地方已经有异步成分
- 为什么 `main.cpp` 现状适合验证，但不适合作为多视频源推流的长期模型
- 多视频源、共享音频、共享 transport 时，线程和所有权该怎么拆
- `QoS` 从同步调用链迁到异步消息模型后，哪些语义必须保住
- 分阶段迁移时，哪些实现细节如果不先写清，后面一定会踩坑

如果要看当前 Linux client 已经落地的架构说明，请继续看：

- [linux-client-architecture_cn.md](architecture_cn.md)
- [plain-client-qos-status.md](README.md)
- [plain-client-qos-parity-checklist.md](parity-checklist.md)

如果要看当前轮次的执行任务清单，请继续看：

- [linux-client-threaded-implementation-checklist_cn.md](threaded-implementation-checklist_cn.md)

> 当前签收范围说明
>
> 本文保留长期目标架构，但截至 `2026-04-17` 当前代码签收范围不包含独立 `demux thread`，也不包含
> `SharedDecodeMultiTrackMode` 的线程化实现。
>
> 当前已落地的主线是：
>
> - 共享 `network thread`
> - 共享 `audio worker`
> - 每源一个 `video source worker`
> - `QoS` 的 async stats + command/ack 骨架

## 2. 适用场景与范围

本文档针对的是仓库内 Linux `PlainTransport C++ client` 的目标推流场景：

- 一个 `peer / publish session`
- `0..1` 条共享音频 track
- `1..N` 条视频 track
- 视频 track 可以来自多个独立视频源
- 所有 track 共享同一个 `PlainTransport` 和同一个 UDP socket
- 典型规格是 `2..4` 路 `720p`

这里的“同一个流”更准确地说是：

- 同一个 `plainPublish`
- 服务端为每个 audio/video track 分配独立的 `producerId / ssrc / pt`
- 客户端在一个 publish session 下并行发送多条 track

本文档不试图覆盖：

- 通用媒体图框架
- 浏览器 WebRTC sender 那种完整栈能力
- 十几路以上高密度多源的通用调度

## 3. 当前实现评审

当前仓库里的 Linux client 仍然以主线程推进媒体循环，见：

- [client/main.cpp](../../../client/main.cpp)
- [client/RtcpHandler.h](../../../client/RtcpHandler.h)
- [client/qos/QosController.h](../../../client/qos/QosController.h)
- [client/qos/QosExecutor.h](../../../client/qos/QosExecutor.h)

### 3.1 当前执行单元

| 执行单元 | 数量 | 当前职责 |
|---|---:|---|
| `main thread` | 1 | `av_read_frame`、视频 decode/scale/encode、音频 AAC->Opus、RTP 发送、RTCP 处理、QoS 决策、notification 分发 |
| `WsClient` reader thread | 1 | 接收 WS response / notification，匹配 pending request，触发 async completion |
| test helper thread | `0..2` | matrix/harness 注入 `clientStats` 和测试 WS 请求 |

### 3.2 现状里的异步成分

当前实现并不是“所有事都在 main thread 同步完成”。

已经存在的异步点包括：

- `getStats` 通过 `ws.requestAsync()` 异步发起
- `WsClient` reader thread 在回调里写入 `CachedServerStatsResponse`
- 主线程随后在 QoS 采样窗口内读取这份带 mutex 保护的缓存

因此更准确的表述是：

- 媒体循环、QoS 状态机和 `actionSink` 仍然主要由主线程推进
- server stats 拉取和 WS 收包已经是跨线程异步路径

### 3.3 当前实现的核心耦合点

当前代码里最关键的同步假设有四个：

1. `av_read_frame -> decode -> encode -> RTP send -> RTCP process -> QoS sample` 在同一条主线程调用链里推进。
2. `QosController::onSample()` 会同步触发 `actionSink`，而 `actionSink` 直接修改 `VideoTrackRuntime` 和 encoder。
3. `RtcpContext` 同时承载：
   - per-track retransmission store
   - per-track last keyframe
   - per-track RR 统计
   - SR 发送时间映射
4. `VideoTrackRuntime::seq` 同时被正常发包路径和 `PLI/FIR` 响应路径共享，当前通过 `RtcpContext::seqPtr` 间接访问。

### 3.4 为什么当前实现还能工作

这个实现适合仓库当前的验证和 matrix 回归，原因很简单：

- 状态共享几乎都发生在同一线程
- `QoS`、encoder、RTP state 不需要复杂同步
- 对单文件推流和有限路数的验证足够直接

### 3.5 为什么它不是长期架构

一旦进入“多视频源、共享 transport、要求更稳的 720p 推流”场景，当前模型会暴露几个硬问题：

- `decode/encode` 会阻塞 `RTCP` 和 pacing
- 某一路视频源变慢会拖住其他 track
- `QoS` 采样时钟挂在媒体循环上，不是独立时钟
- `actionSink` 的同步语义无法直接迁到跨线程 encoder
- `g_rtcp` / `seqPtr` / direct send 一旦半迁移就会形成 data race

## 4. 设计结论

目标架构的核心结论是：

1. 共享一个 `network thread`，它是唯一 transport owner。
2. 长期上可以引入独立 `demux thread`，但它不属于当前轮次签收范围。
3. 视频侧采用 `video source worker` 模型。
4. 音频采用共享 `audio worker`，归属于 `publish session / peer`，不绑定任何单一视频 track。
5. `QoS` 继续集中在 `main / control thread`，但必须改成异步 stats + command/ack 模型。

换句话说，默认模型不是：

- 一个主线程跑完所有事情
- 也不是每个 track 自己带一套网络线程

而是：

- 当前先不拆 `demux`
- 数据面网络集中
- 编解码按源隔离
- 控制面 QoS 集中汇总

## 5. 长期目标运行时拓扑

```text
                                 +----------------------------+
                                 |      main / control        |
                                 |----------------------------|
                                 | lifecycle / state machine  |
                                 | PublisherQosController     |
                                 | peer budget coordination   |
                                 | WsClient business owner    |
                                 | command routing            |
                                 +-----+---------------+------+
                                       ^               |
                                       | stats/events  | WS requests /
                                       | acks/errors   | transport config
                                       |               v
                          +------------+----+   +------+-------------+
                          |  ws reader      |   |   network thread   |
                          |-----------------|   |---------------------|
                          | recv WS frames  |   | UDP socket owner    |
                          | pending match   |   | RTP packetize/send  |
                          | queue notify    |   | RTCP SR/RR/NACK/PLI |
                          +-----------------+   | pacing / keyframe   |
                                                | retransmission store|
                                                | SenderStatsSnapshot |
                                                +-----+-----------+---+
                                                      ^           ^
                                                      | AU        | AU
                                                      |           |
                      +-------------------------------+           +------------------------------+
                      |                                                                  |
              +-------+--------+                                                 +-------+--------+
              |  audio worker  |                                                 | video worker #1 |
              |----------------|                                                 |-----------------|
              | audio decode   |                                                 | decode/scale/enc|
              | audio encode   |                                                 | apply commands  |
              | audio clock    |                                                 | keyframe merge  |
              +-------^--------+                                                 +-----------------+
                      | packet/frame                                                       ^
                      |                                                                     |
                +-----+------------------------------------------------------+              |
                |                    demux thread                            |--------------+
                |------------------------------------------------------------|
                | AVFormatContext owner / av_read_frame                      |
                | file-clock mapping / stream_index dispatch                 |
                +------------------------------------------------------------+
```

上图是长期目标拓扑。

当前已落地实现尚未拆出独立 `demux thread`，这部分请以文末的实施状态表为准。

控制路径是双向的：

- `network -> control`: stats / events / acks / errors
- `control -> network`: transport config / shutdown / pause state

`ws reader` 不直接和 `network thread` 通信；凡是受 WS 影响的 transport 或 QoS 状态，都经由 `control thread` 中转。

## 6. 长期目标线程模型与硬件前提

| 执行单元 | 数量 | 主要职责 | 是否实时敏感 |
|---|---:|---|---:|
| `main / control` | 1 | QoS 决策、状态收敛、WS 业务 owner、命令路由 | 中 |
| `ws reader` | 1 | 只收包与 pending request 匹配 | 低 |
| `demux thread` | 长期目标 1 | `AVFormatContext`、`av_read_frame`、文件源时钟 | 中 |
| `network thread` | 1 | UDP、RTP/RTCP、pacing、重传、stats 生成 | 很高 |
| `audio worker` | 0..1 | 音频 decode/encode 和音频时钟 | 中 |
| `video source worker` | 每视频源 1 个 | 视频 decode/scale/encode | 高 |
| test helper | `0..2` | 仅测试注入 | 低 |

### 6.1 硬件前提

文档默认目标机器不少于 `4` 个可用核心。

不承诺下面这类场景能稳定工作：

- `2` 核云主机
- 多路 `720p` x264 重编码
- 同时还要求稳定的 `QoS + RTCP + pacing`

### 6.2 低核数降级策略

在低核心数场景，优先采用这些降级手段：

- 减少视频 track 数
- 降低分辨率 / 帧率
- 优先退回 `copyMode`
- 必要时合并 source worker，但只作为低配模式，不作为默认架构

## 7. 视频源模型

目标架构必须同时覆盖两类模式。

### 7.1 `IndependentSourceMode`

多个独立视频源，各自一个 `video source worker`。

适用于：

- 多摄像头
- 多采集卡
- 多独立文件源

这是本文档的主目标模式。

### 7.2 `SharedDecodeMultiTrackMode`

一个源一次 decode，多路 encode，多 track 共享一个 source pipeline。

适用于：

- 当前仓库的单文件多 track
- 一个输入源要输出多个编码版本

当前代码就属于这一类：

- 一个 `fmtCtx`
- 一次 `av_read_frame`
- 一次视频 decode
- 多个 `VideoTrackRuntime` 共享同一个 `vframe`

因此文档必须保留这一兼容模式，且明确禁止通过“每个 worker 各自打开同一个文件独立 decode”来实现当前单文件多 track。

## 8. 音频模型

音频采用共享 `audio worker`，不绑定某个视频 source worker。

### 8.1 归属关系

默认 publish 拓扑是：

```text
PublishSession
  peerId
  audioTrack: 0..1
  videoTracks: 1..N
```

音频属于整个 `publish session / peer`，不是属于某个单独视频 track。

### 8.2 为什么不挂在某个视频 worker

如果把音频挂在 `source worker #0`，会破坏两个边界：

- “每个视频源一个 worker”
- “音频是 session 级共享轨”

并且当前代码里的 `AAC -> Opus` 路径与视频 decode/encode 共享同一个 `fmtCtx`，真正的拆分根因在 demux 层，而不是“把音频塞进谁”。

### 8.3 当前代码的迁移含义

当前音频路径是：

- `adec/aenc` 在主线程
- `sendOpus()` 直接在主线程发
- `rtcp.onAudioRtpSent()` 在主线程更新音频 SR 计数

迁移后应变成：

- `demux thread` 分发 audio packet
- `audio worker` 产出 `EncodedAudioAccessUnit`
- `network thread` 独占 `audioSsrc / aSeq / audio SR`

## 9. 所有权边界

### 9.1 `main / control`

独占：

- `PublisherQosController`
- peer-level budget coordination
- `WsClient` 业务 owner
- notification 业务解释
- `SenderStatsSnapshot` 聚合结果
- 生命周期与错误收敛

禁止直接访问：

- `AVFormatContext`
- `AVCodecContext`
- UDP socket
- `RtcpContext`
- retransmission store

其他线程不得直接调用 `WsClient`；凡是 WS 动作，都经由 `control thread` 路由。

### 9.2 `demux thread`

独占：

- `AVFormatContext`
- `av_read_frame`
- 文件源首帧 `PTS`
- `PTS -> steady_clock` 映射

它不拥有：

- encoder / decoder
- `seq / ssrc`
- RTCP 状态

### 9.3 `video source worker`

独占：

- 视频 decode / scale / encode 状态
- `AVCodecContext`
- `SwsContext`
- `scaledFrame`
- 编码配置与配置代数
- worker 内部统一的关键帧请求位

它不拥有：

- UDP socket
- `seq / ssrc`
- RTCP 状态
- retransmission store

### 9.4 `audio worker`

独占：

- 音频 decode / encode 状态
- 音频时钟
- `EncodedAudioAccessUnit` 生产

它不拥有：

- UDP socket
- `audioSsrc / aSeq`
- RTCP 状态

### 9.5 `network thread`

独占：

- UDP socket
- `epoll / eventfd / timerfd`
- 每个 track 的 `ssrc / payloadType / seq`
- per-track retransmission store
- per-track keyframe cache
- `RtcpContext`
- `SenderStatsSnapshot` 生成

它不直接操作：

- encoder
- `PublisherQosController`
- `WsClient`

## 10. 当前结构到目标结构的拆分映射

### 10.1 `VideoTrackRuntime` 字段归属

| 当前字段/语义 | 目标 owner |
|---|---|
| `encoder` | `video source worker` |
| `swsCtx` | `video source worker` |
| `scaledFrame` | `video source worker` |
| `encBitrate` / `configuredVideoFps` / `scaleResolutionDownBy` | `video source worker` 执行，`control` 只持目标配置 |
| `forceNextVideoKeyframe` | 改为 worker 内统一关键帧请求位 |
| `seq` | `network thread` |
| `ssrc` / `payloadType` | `network thread` |
| `qosCtrl` | `control thread` |
| `videoSuppressed` | `control` 决策，worker/network 消费其派生状态 |

### 10.2 当前音频状态归属

| 当前字段/语义 | 目标 owner |
|---|---|
| `adec / aenc` | `audio worker` |
| audio clock | `audio worker` |
| `audioSsrc / aSeq` | `network thread` |
| `peerHasAudioTrack` | `control thread` |

### 10.3 `RtcpContext` 字段归属

| 当前字段/语义 | 目标 owner |
|---|---|
| `videoStreams[].store` | `network thread` |
| `videoStreams[].lastKeyframe` | `network thread` |
| `videoStreams[].rrLossFraction / rrCumulativeLost / rrJitterMs / rrRttMs` | `network thread`，通过快照暴露给 `control` |
| `videoStreams[].seqPtr` | 删除 |
| `sendH264Fn` | 删除 |
| `srNtpToSendTime / srNtpOrder` | `network thread` |

迁移后，`control thread` 不再读取 `RtcpContext` 的任何字段。

## 11. 队列与值对象模型

推荐使用小而明确的单向队列，不共享可变大对象。

### 11.1 `ws reader -> control`

消息类型：

- `WsResponse`
- `WsNotification`

约束：

- reader thread 不做业务逻辑
- completion handler 的业务收敛最终回到 `control`

### 11.2 `demux -> audio/video worker`

消息类型：

- `DemuxedAudioPacket`
- `DemuxedVideoPacket`

约束：

- 文件源只允许 `demux thread` 调用 `av_read_frame`
- 不允许 audio/video worker 共享 `fmtCtx`

### 11.3 `control -> video/audio worker`

消息类型：

- `UpdateBitrate`
- `UpdateFramerate`
- `ReconfigureEncoder`
- `PauseVideoTrack`
- `ResumeVideoTrack`
- `ForceKeyframe`
- `StopSource`
- `StopAudio`

每条命令必须带：

- `commandId`
- `trackId` 或 `sourceId`
- `issuedAtMs`
- `configGeneration`

### 11.4 `video/audio worker -> control`

消息类型：

- `CommandAck`
- `SourceError`
- `AudioError`

`CommandAck` 至少包含：

- `commandId`
- `Applied | Rejected`
- `appliedAtMs`
- `reason`
- `configGeneration`

### 11.5 `video/audio worker -> network`

消息类型：

- `EncodedAccessUnit`
- `EncodedAudioAccessUnit`

统一字段至少包括：

- `trackId`
- `kind`
- `pts`
- `duration`
- `isKeyframe`
- `codec`
- `configGeneration`
- `payload bytes`

### 11.6 `network -> control`

消息类型：

- `SenderStatsSnapshot`
- `RtcpFeedbackEvent`
- `TrackSendStateChanged`
- `NetworkError`

### 11.7 `control -> network`

消息类型：

- `TrackTransportConfig`
- `PauseTransportTrack`
- `ResumeTransportTrack`
- `ShutdownTransport`

## 12. 值对象内存策略

`EncodedAccessUnit` 和 `EncodedAudioAccessUnit` 必须采用 move-only 的独占缓冲区。

最低约束是：

- 生产者分配并拥有 payload
- 入队时转移所有权给 `network thread`
- `network thread` 消费后释放
- 禁止跨线程共享裸指针
- 禁止多个线程同时改写同一个 payload buffer

推荐形态可以是：

- `std::unique_ptr<uint8_t[]> + size`
- 或等价的 move-only buffer wrapper

这条规则必须写死，避免后续在性能优化时引入隐式共享和悬垂引用。

## 13. `actionSink` 到命令模型的映射

当前代码里 `actionSink` 是同步直接操作 encoder 和 `VideoTrackRuntime`。

迁移后必须变成纯命令投递语义。

| 当前 action | 目标命令 |
|---|---|
| `SetEncodingParameters(maxBitrate)` | `UpdateBitrate` |
| `SetEncodingParameters(maxFramerate)` | `UpdateFramerate` |
| `SetEncodingParameters(scaleResolutionDownBy)` | `ReconfigureEncoder` |
| `EnterAudioOnly` | `PauseVideoTrack` |
| `ExitAudioOnly` | `ResumeVideoTrack` |
| `PauseUpstream` | `PauseVideoTrack` |
| `ResumeUpstream` | `ResumeVideoTrack` |
| 关键帧请求 | `ForceKeyframe` |

### 13.1 重命令与轻命令

`ReconfigureEncoder` 是重命令：

- 允许销毁重建 encoder
- 会导致 config generation 增加
- 必须返回显式 `CommandAck`

`UpdateBitrate` / `UpdateFramerate` 是轻命令：

- 只允许在当前 encoder 配置兼容时做在线更新

### 13.2 幂等语义

当前 `QosActionExecutor` 只靠本地 `lastKey_` 去重，这在异步模型下不够。

迁移后必须改成：

- 本地 `lastKey_` 只做候选去重
- “已下发”与“已应用”分离
- 只有收到 `CommandAck` 后，control 才认定命令已生效

## 14. QoS 子系统迁移注意事项

这是迁移里最容易低估的部分。

### 14.1 `PublisherQosController` 的线程约束

`PublisherQosController` 的所有方法只能在 `control thread` 调用，包括：

- `onSample()`
- `handlePolicy()`
- `handleOverride()`
- `setCoordinationOverride()`

不允许任何其他线程直接触碰 controller。

因此像 `coordinationBitrateCapDirty_` 这样的内部状态仍然保留单线程语义，不引入跨线程访问。

### 14.2 stats 语义必须保真

当前 `QosSignals` 依赖相邻 snapshot 的计数器差值。

迁移后不能让 `control thread` 用本地采样时钟盲目去重算网络计数器差值，否则会出现：

- 连续两次采样拿到同一份 stats，delta 为 0
- 一次采样跨越多个 stats 窗口，delta 偏大

默认规则改为：

- `network thread` 负责生成带时间戳的 `SenderStatsSnapshot`
- `network thread` 预计算派生值，例如 `sendBitrateBps`
- `control thread` 使用 `statsTimestampMs` 和 snapshot 自带派生值，不再依赖本地 wall clock 去重算 `counterDelta()`

### 14.3 peer-level coordination 的原子性

当前代码里的 peer budget coordination 是同步流程。

迁移后必须维持“同一轮 stats 集”的语义：

1. `network thread` 周期性产生 per-track stats
2. `control` 收集同一轮或足够新鲜的一组 stats
3. 仅在这一轮对齐后的 stats 集上执行：
   - `onSample()`
   - `allocatePeerTrackBudgets()`
   - `buildCoordinationOverrides()`

不允许把任意旧 stats 无差别混入当轮 coordination。

### 14.4 stale stats 规则

每条 `SenderStatsSnapshot` 必须带：

- `statsTimestampMs`
- `generation`
- `trackId`
- freshness 指示

控制线程默认容忍策略：

- 若某个 track 在当前窗口没有新 stats，可使用上一轮最近且未过期的 stats
- 过期 stats 不参与带宽受限判定
- stale track 必须显式标注，避免预算分配误用

### 14.5 command/ack 约束

QoS action 不得仅凭“发出去一次”就认为已应用。

默认规则：

- 每条命令必须显式 `ack`
- `Rejected` 也算完成态，但必须回 error reason
- control 未收到 ack 前，不得把该命令当成“已生效配置”

## 15. 关键帧请求语义

当前代码里的 `forceNextVideoKeyframe` 是主线程里的一个 bool。

迁移后会有两个来源的关键帧请求：

- `network thread` 的 `PLI/FIR`
- `control thread` 的 `ExitAudioOnly` 或其他 QoS 恢复动作

目标规则是：

- `network` 和 `control` 都只发 `ForceKeyframe` 命令
- `video source worker` 内部合并成一个统一的关键帧请求位
- 不允许维护两套互不知晓的 keyframe flag

## 16. 网络线程设计

### 16.1 事件循环模型

默认采用：

- `epoll_wait`
- `eventfd`
- `timerfd`

统一驱动：

- UDP socket 可读事件
- source/audio AU 到达通知
- pacing tick
- SR 周期
- stats 生成窗口

默认不采用 busy-poll 作为基线实现。

### 16.2 `RtcpContext` 约束

迁移后 `RtcpContext` 完全内聚在 `network thread`。

控制线程不再直接读取：

- `rrCumulativeLost`
- `rrRttMs`
- `rrJitterMs`
- `packetCount`
- `octetCount`

这些都必须通过 `SenderStatsSnapshot` 暴露。

### 16.3 PLI / FIR 处理

默认策略：

1. 如果最近缓存关键帧存在且 track 未 pause，先立即补发缓存关键帧。
2. 同时向对应 worker 下发 `ForceKeyframe`。
3. 同一 track 的 `PLI/FIR` debounce 默认为 `1500ms`。

### 16.4 NACK 处理

`NACK` 必须完全留在 `network thread` 本地处理：

- 从 per-track retransmission store 读取 RTP 包
- 直接重传

绝不回到 worker 重新编码。

### 16.5 pacing 默认策略

默认采用包级 pacing，而不是“一帧所有包一次性发出”。

默认调度规则：

- 按最早发送 deadline 选包
- deadline 相同则轻量 round-robin
- 音频优先于视频
- pacing buffer 目标窗口为 `100ms`

## 17. 背压与丢帧策略

### 17.1 视频队列

`video worker -> network` 队列按时间窗口限制，目标 `100..200ms`。

默认丢帧规则不是“随便丢旧 P 帧”，而是：

- 只保留最新关键帧及其后的访问单元
- 如果新关键帧尚未到达，可直接丢弃整段旧 GOP

禁止保留断链 P 帧。

### 17.2 retransmission store

retransmission store 必须按 track 隔离。

不允许：

- 多 track 共用同一个 store
- source worker 直接写 store

### 17.3 control 命令队列

控制命令允许 latest-wins，但只对尚未执行的同类命令生效。

一旦命令已发出并等待 ack，control 必须通过 `commandId` 跟踪完成态。

## 18. 时钟规范

### 18.1 文件源

文件源由 `demux thread` 做：

- 首帧 `PTS` 记录
- 启动 `steady_clock` 记录
- `PTS -> steady_clock` 映射

### 18.2 实时源

摄像头 / 采集卡等实时源不走文件时钟映射。

默认规则：

- 采集时间戳或采集到达时刻就是时钟基准

### 18.3 网络时钟

`network thread` 的独立时钟负责：

- pacing
- SR 周期
- stats 窗口
- PLI debounce

禁止再把这些行为挂在主循环 `sleep_until()` 上。

## 19. comedia probe、rollback 与配置代数

这些属于不写清就会踩坑的实施护栏。

### 19.1 comedia probe 最小化

迁移后 probe 仍由 `network thread` 执行。

默认规则：

- probe 只负责让服务端 learned remote tuple
- 不在 probe 阶段引入完整媒体数据
- audio/video track 的 probe 包数和间隔应最小化，避免在启动阶段制造 burst

### 19.2 probe rollback

如果 publish 建链在 probe 后失败，rollback 规则要写死：

- 未进入 stable sending 前，probe 产生的临时状态不得污染 worker 的长期状态
- `network thread` 在回滚时清空与本次 publish 相关的未确认 transport state
- control 负责把 session 状态收敛到 failed/stopped

### 19.3 `encoder recreated` 与 config generation

文档中统一使用 `configGeneration` 表示 encoder 配置代数。

规则：

- 每次重建 encoder 都增加 generation
- `EncodedAccessUnit` 带 generation
- `CommandAck` 带 generation
- control 和 network 可用 generation 过滤旧配置残留数据

## 20. 日志与调试约束

所有新增日志必须至少带上这些维度：

- thread name
- `peerId`
- `trackId`
- `sourceId`
- `commandId`
- `configGeneration`

否则多线程下无法追命令、追 stats、追关键帧请求。

## 21. 编译期与运行期回滚开关

迁移过程中必须保留回滚能力。

建议至少保留这些开关语义：

- 旧单线程媒体路径开关
- 新 `network thread` 开关
- 新 `demux thread` 开关
- `copyMode` 强制开关
- 新 QoS async path 开关

目标是允许按阶段灰度，而不是一次性切全量。

## 22. 渐进式演进方案

### 阶段 1：抽值对象与边界

先抽出：

- `DemuxedAudioPacket`
- `DemuxedVideoPacket`
- `EncodedAccessUnit`
- `EncodedAudioAccessUnit`
- `SenderStatsSnapshot`
- `CommandAck`

这一阶段不要求立刻改线程。

### 阶段 2：先抽离 `network thread`

这是收益最大且必须闭环的一步。

阶段 2 必须同时完成：

- `sendH264/sendOpus` 从 direct send 改成 AU 入队
- `network thread` 成为唯一 RTP sender
- `seq` 所有权迁移到 `network thread`
- `RtcpContext` 迁移到 `network thread`
- `sendH264Fn` 删除
- `g_rtcp` 删除

不允许只挪 RTCP 不挪 RTP。

### 阶段 3：引入 `demux thread` 和 worker

这一步解决：

- 当前 `fmtCtx + av_read_frame` 交错读包问题
- audio/video worker 的源分发

并同时保留两种视频模式：

- `IndependentSourceMode`
- `SharedDecodeMultiTrackMode`

### 阶段 4：QoS 改为 stats-driven + ack-driven

这一步完成：

- `SenderStatsSnapshot` 驱动 QoS
- command/ack 模型替代 direct `actionSink`
- peer coordination 改为对齐 stats 集驱动

### 阶段 5：性能与低配模式优化

根据实际路数与 CPU 压力，再评估：

- audio worker 是否可选关闭
- 某些低配模式是否合并 worker
- 是否强制回退 `copyMode`

## 23. 测试与阶段验收

### 23.1 每阶段回归目标

- 阶段 1：现有 `cpp-client-matrix` 结果不回退
- 阶段 2：`g_rtcp` 消失，RTP/RTCP 行为等价
- 阶段 3：单文件多 track 与 `copyMode` 继续可用
- 阶段 4：QoS 注入路径和 matrix 继续通过

### 23.2 多线程专项测试

需要新增的专项测试至少包括：

- source worker 延迟 / 阻塞
- network 背压与 GOP 丢弃
- stale stats 对 peer coordination 的影响
- command ack 丢失 / 延迟
- `PLI/FIR` 与 `ExitAudioOnly` 并发关键帧请求
- probe rollback

### 23.3 现有测试注入路径的迁移

当前已有：

- `testClientStatsPayloads`
- `testWsRequests`

迁移后：

- mock stats 注入点进入 `control` 的 stats 输入队列
- test WS 请求仍由 `control thread` 发起

## 24. 非目标

这份设计当前不试图解决：

- 房间级 SFU 侧 QoS 逻辑重构
- 浏览器端 QoS 协议重写
- 通用硬件编解码抽象
- 任意多源的大规模通用调度

## 25. 结论

对“多个视频源、一个共享音频、同一 publish session、多 track 发送”这个场景，推荐的目标迁移架构是：

- `1` 个 `main / control thread`
- `1` 个 `ws reader thread`
- `1` 个 `demux thread`
- `1` 个共享 `network thread`
- `1` 个共享 `audio worker`
- `N` 个 `video source worker`

这份设计的关键不在于“线程越多越好”，而在于：

- demux 入口唯一
- 网络状态唯一 owner
- encoder 按源隔离
- 音频按 session 共享
- QoS 保住单线程控制面语义，同时改成异步 stats + ack 模型

只有把这些约束一起写死，迁移才不会在 `actionSink`、`g_rtcp`、`seq`、peer coordination 和背压策略上走样。

## 26. 实施状态（2026-04-17）

| 阶段 | 状态 | 说明 |
|---|---|---|
| 1 | ⚠️ 部分 | `ThreadTypes.h`: `EncodedAccessUnit`(含 `configGeneration`), `SenderStatsSnapshot`(含 `generation`), `TrackControlCommand`(含 `commandId/configGeneration/issuedAtMs`), `CommandAck`(含 `commandId/configGeneration/reason/appliedAtMs`), `SpscQueue`。值对象骨架已落地，但完整协议边界仍依赖后续补强。 |
| 2 | ✅ 完成 | `NetworkThread.h`: epoll + eventfd + timerfd 事件驱动。UDP socket、RTP packetization(经 pacing buffer)、RTCP SR/RR/NACK/PLI(debounce 1s)/FIR、stats push(generation 计数)、control command queue。configGeneration 过滤旧 AU，generation 切换时清除 keyframe cache 和 retransmission store。`g_rtcp` 在 threaded 路径置空。 |
| 3 | ⚠️ 部分 | `SourceWorker.h`: 支持 File 和 V4L2Camera 两种输入。`IndependentSourceMode` 已实现（每源一个 worker，encoder 重建时 `configGeneration++`）。`SharedDecodeMultiTrackMode` 未实现——多 track 单文件强制回退 legacy 路径（要求 `distinctSourceCount >= videoTrackCount`）。独立 `demux thread` 不在当前轮次范围。 |
| 4 | ⚠️ 部分 | `actionSink` 返回 false（pending），controller 不在入队时推进 level/audioOnly。per-track `PendingCommand` 表按 `commandId` 匹配 ack。`confirmAction()` 在 ack applied 时推进 `currentLevel/inAudioOnlyMode`。`resetExecutor()` 在 ack rejected 时重置。stats freshness 用 `generation` 门控。peer coordination 只包含本周期实际 sampled 的 track。当前仍是“基础闭环已落地”，不是最终协议形态。 |
| 5 | ❌ 未开始 | 低配模式、audio worker 可选关闭、worker 合并策略 |

### 已知的设计-实现差距

| 文档要求 | 当前实现 | 计划 |
|---|---|---|
| network thread 用 `epoll + eventfd + timerfd` 事件驱动 | ✅ 已实现 | — |
| RTP pacing 在 network thread 做包级均匀分散 | ✅ 已实现（2ms tick, burst limit 8） | 可调优 burst/interval |
| `commandId` + `configGeneration` 协议 | ⚠️ 基础骨架已实现 | 继续收紧协议边界与失败语义 |
| demux thread 独立，音视频分发到 worker | 不在当前轮次范围 | 后续阶段再评估 |
| 单文件多 track 共享 decode | 强制回退 legacy 路径 | 后续阶段再评估 |
