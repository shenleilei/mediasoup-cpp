# Plain Client 架构与线程模型

> 文档性质
>
> 这是一份当前有效的 Linux `PlainTransport C++ client` 架构与线程模型说明。
> 它描述的是现在主分支里的真实实现，不是迁移中的中间态，也不是历史设计草案。
>
> 如果只想快速看结果入口，先看：
> - [plain-client-qos-status.md](README.md)
>
> 如果想看更短的架构摘要，继续看：
> - [linux-client-architecture_cn.md](architecture_cn.md)

## 1. 一句话结论

当前 `plain-client` 已经收敛成一条 threaded-only 运行时主路径：

- `PlainClientApp` 负责启动参数、输入发现、会话建立和外层 shell
- `PlainClientThreaded.cpp` 只是一个薄入口
- `ThreadedQosRuntime` 是真正的运行时 owner
- 每个 video track 各有一个 `SourceWorker`
- 全局只有一个 `NetworkThread` 持有 transport 发送权

所以它不是“每路视频都各自发 RTP”的模型，而是：

- 每路视频一个独立生产线程
- 全局一个统一发送线程
- 全局一个控制/协调线程

## 2. 为什么要这样拆

如果 plain-client 只有单线程，多个 video track 会把下面这些事情全部串在一起：

- 读输入
- decode / scale / encode
- RTP packetization
- UDP send
- RTCP / TWCC feedback
- QoS 采样与动作执行

这样的问题很直接：

- 一路视频编码变慢，会连带拖慢别的 track
- transport-wide sequence、pacing、重传、TWCC 这类发送侧状态很难保证只有一个 owner
- 多路 QoS 协调没有统一观察点

当前拆法的目标不是“线程越多越好”，而是把 ownership 拆对：

- `SourceWorker` 负责“把输入变成 encoded access unit”
- `NetworkThread` 负责“什么时候真正发包”
- control thread 负责“QoS 怎么决策、各 track 怎么协调”

## 3. 当前组件图

```text
main.cpp
  -> PlainClientApp
       -> ParseArguments / InitializeMediaBootstrap / InitializeSession
       -> RunThreadedMode()
            -> ThreadedQosRuntime
                 -> NetworkThread
                 -> audio worker (optional)
                 -> SourceWorker x N
                 -> PublisherQosController x N
```

可以把它理解成三层：

```text
Session / Shell
  -> PlainClientApp

Control / QoS Runtime
  -> ThreadedQosRuntime
  -> PublisherQosController x N

Media / Transport Execution
  -> SourceWorker x N
  -> audio worker (optional)
  -> NetworkThread
```

## 4. 线程清单

### 4.1 当前线程表

| 线程 | 数量 | 当前 owner | 主要职责 |
|---|---:|---|---|
| main / control thread | 1 | `PlainClientApp` + `ThreadedQosRuntime` | session、WS 通知分发、QoS controller、peer coordination、`clientStats` 上传 |
| `WsClient` reader thread | 1 | `WsClient` | 异步接收 WS response / notification，写入本地 pending 队列 |
| `NetworkThread` | 1 | `ThreadedQosRuntime` | UDP socket owner、RTP/RTCP、pacing、TWCC、send-side BWE、probe |
| audio worker | `0..1` | `ThreadedQosRuntime` | 从输入文件读音频、decode、Opus encode、产出 audio AU |
| `SourceWorker` | 每 video track 1 个 | `ThreadedQosRuntime` | 视频输入、decode/scale/encode、关键帧命令执行、产出 video AU |

### 4.2 关键判断

最重要的不是线程数，而是这两个不变量：

1. `NetworkThread` 是唯一 transport owner
2. 每个 video track 都有自己的 source/encode owner

这就是“每路视频一个 worker，但全局只有一个发送线程”的本质。

## 5. 每个线程到底干什么

### 5.1 `main.cpp`

作用很薄：

```text
main()
  -> PlainClientApp::RunPlainClientApp()
```

它不参与 runtime 逻辑。

### 5.2 `PlainClientApp`

`PlainClientApp` 只负责外层 shell，不负责长期运行时细节。

当前职责：

- 解析 CLI 参数和环境变量
- 打开输入文件并发现 `video/audio` stream
- 建立 WebSocket 会话
- 执行 `join` / `plainPublish`
- 建 UDP socket
- 保存每个 track 的静态 bootstrap 信息
  - `trackId`
  - `producerId`
  - `ssrc`
  - `payloadType`
  - `transportCcExtensionId`
  - 初始编码目标
- 把执行权交给 `ThreadedQosRuntime`

它不再负责：

- direct RTP send
- legacy copy mode
- sender QoS runtime loop

### 5.3 `PlainClientThreaded.cpp`

这个文件现在只是一个薄入口：

```text
RunThreadedMode()
  -> ThreadedQosRuntime runtime(*this)
  -> runtime.Run()
```

它存在的意义主要是把“进入 threaded runtime”这个入口从 app shell 里单独隔出来。

### 5.4 `ThreadedQosRuntime`

这是当前 plain-client 最重要的 runtime owner。

它负责：

- 创建 per-track queue
- 创建 `NetworkThread`
- 启动/停止 audio worker
- 启动/停止 `SourceWorker`
- 创建每路 `PublisherQosController`
- 消费 stats / ack
- 定时触发 sender QoS 采样
- 多 track peer coordination
- 触发 `clientStats` 上传

它内部持有的状态基本就是“当前 threaded runtime 真正还活着的状态”：

- `queues_`
- `statsQueue_`
- `networkControlQueue_`
- `networkAckQueue_`
- `audioAuQueue_`
- `netThread_`
- `audioThread_`
- `workers_`
- `pendingCommands_`
- `trackControlStates_`
- `trackStats_`
- `nextCommandId_`
- `lastPeerQosSampleMs_`
- `peerSnapshotSeq_`

这些状态都不再挂在 `PlainClientApp` 上。

### 5.5 `WsClient` reader thread

`WsClient` 自己有一个 reader thread。

它的职责很简单：

- 持续从 socket 读 WebSocket frame
- 把 response 放进 pending request map
- 把 notification 放进 pending notification queue

真正的 notification 消费不是在 reader thread 里做，而是由 control thread 调：

```text
ws_.dispatchNotifications()
```

这样可以保证 `qosPolicy` / `qosOverride` 最终还是在 control/runtime 线程里落地。

### 5.6 `NetworkThread`

`NetworkThread` 是整个 plain-client 里最关键的单点 owner。

它负责：

- 持有唯一的 UDP socket
- 注册所有 video track 的 transport 状态
- 发送 comedia probe
- 真正执行 RTP send
- packet class 区分
- pacing
- retransmission
- RTCP RR / SR / NACK / PLI / FIR
- transport-cc header extension rewrite
- TWCC feedback 解析
- send-side estimate 维护
- probe cluster 生命周期
- 输出 sender stats snapshot

换句话说：

- 别的线程只能“产出要发的内容”
- 只有 `NetworkThread` 能决定“什么时候真的发出去”

### 5.7 audio worker

audio worker 不是必然存在的。

只有在：

- 输入文件有 audio stream
- 音频 decoder / encoder 初始化成功

时才会启动。

它负责：

- 重新打开输入文件
- 找到 audio stream
- decode 音频帧
- Opus encode
- 把结果写入 `audioAuQueue`

它不直接 send UDP。

### 5.8 `SourceWorker`

每路 video track 一个 `SourceWorker`。

它负责：

- 读取输入源
  - 文件
  - `/dev/video*` camera
- decode 视频
- scale
- x264 encode
- 执行 track control command
  - `SetEncodingParameters`
  - `PauseTrack`
  - `ResumeTrack`
  - `ForceKeyframe`
- 生成 `EncodedAccessUnit`
- 把 ack 写回 control side

所以它是“视频生产者”，不是“网络发送者”。

## 6. 为什么每个 video track 一个线程

这是最容易被问到的问题。

### 6.1 原因

每路视频都有自己的：

- 输入节奏
- decode / scale / encode 开销
- 关键帧请求
- 码率/FPS/分辨率调整

如果把多路视频都放进一个 producer thread：

- 任一路的编码阻塞都会拖慢其它路
- command / ack 也更难按 track 独立处理
- 在多路场景下很难保持“每路都是独立 source pipeline”

所以当前模型选择：

- 每路视频独立生产
- 生产完成后统一交给 `NetworkThread`

### 6.2 这不等于每路都有自己的发送栈

这里要特别区分：

- `SourceWorker` 线程是“生产线程”
- `NetworkThread` 才是“发送线程”

所以它不是：

```text
track1 -> own RTP sender
track2 -> own RTP sender
track3 -> own RTP sender
```

而是：

```text
track1 SourceWorker \
track2 SourceWorker  -> NetworkThread -> UDP
track3 SourceWorker /
```

### 6.3 代价

这个模型的代价也很明确：

- 多路重编码时 CPU 压力高
- 线程数上升
- 低配机器上不一定划算

所以它是“为了 ownership 正确和多路 QoS/transport 正确”做的设计，不是为了炫多线程。

## 7. 为什么全局只有一个 `NetworkThread`

发送线程必须单点收口，否则这些状态很容易乱：

- RTP seq
- transport-wide sequence
- pacing budget
- retransmission queue
- RTCP state
- TWCC sent-packet history
- send-side BWE estimator
- probe state

这些状态都要求：

- 同一个 owner
- 同一个时间基准
- 同一个发送顺序

所以当前设计里：

- `NetworkThread` 是唯一 transport owner
- control thread 只能发 command / hint
- `SourceWorker` 只能发 AU

## 8. 队列与命令拓扑

当前运行时最核心的是几类 SPSC queue。

### 8.1 每 track 一组 queue

每个 video track 都有：

- `auQueue`
  - `SourceWorker -> NetworkThread`
  - 传 `EncodedAccessUnit`
- `ctrlQueue`
  - `control thread -> SourceWorker`
  - 传 `TrackControlCommand`
- `netCmdQueue`
  - `NetworkThread -> SourceWorker`
  - 传网络触发的 keyframe / pause / resume
- `ackQueue`
  - `SourceWorker -> control thread`
  - 传 `CommandAck`

### 8.2 全局 queue

`ThreadedQosRuntime` 还持有：

- `statsQueue`
  - `NetworkThread -> control thread`
  - 传 `SenderStatsSnapshot`
- `networkControlQueue`
  - `control thread -> NetworkThread`
  - 传 `NetworkControlCommand`
- `networkAckQueue`
  - `NetworkThread -> control thread`
  - 传 network-side `CommandAck`
- `audioAuQueue`
  - `audio worker -> NetworkThread`
  - 传音频 AU

### 8.3 一张总图

```text
SourceWorker(track i)
  -> auQueue[i]
  -> NetworkThread

control thread
  -> ctrlQueue[i]
  -> SourceWorker(track i)

NetworkThread
  -> netCmdQueue[i]
  -> SourceWorker(track i)

SourceWorker(track i)
  -> ackQueue[i]
  -> control thread

NetworkThread
  -> statsQueue
  -> control thread

control thread
  -> networkControlQueue
  -> NetworkThread

audio worker
  -> audioAuQueue
  -> NetworkThread
```

## 9. 命令 / ack 模型

当前 plain-client 的 QoS 动作不是 direct call 改编码器，而是 command/ack。

### 9.1 control thread 发什么

QoS controller 产出的动作会被翻译成两类：

- track-local command
  - 发到 `ctrlQueue`
  - 让 `SourceWorker` 改编码参数 / pause / resume
- network transport command
  - 发到 `networkControlQueue`
  - 让 `NetworkThread` 更新 transport hint / pause state

### 9.2 为什么要 ack

因为 control thread 不能假设动作已经生效。

需要等 `CommandAck` 回来之后，才能确认：

- 实际 bitrate/fps/scale
- configGeneration 是否已经切换
- pause/resume 是否真正生效

这也是当前 runtime 比早期 direct actionSink 更稳的原因。

## 10. 启动流程

当前 threaded 启动主路径可以概括成：

```text
main()
  -> PlainClientApp::RunPlainClientApp()
  -> ParseArguments()
  -> InitializeMediaBootstrap()
  -> InitializeSession()
       -> ws connect
       -> join
       -> plainPublish
       -> create UDP socket
       -> store producer/track bootstrap info
  -> RunThreadedMode()
       -> ThreadedQosRuntime::Run()
            -> setupQueues()
            -> setupNetworkThread()
            -> startAudioThreadIfNeeded()
            -> netThread.start()
            -> resolveAudioAvailability()
            -> startSourceWorkers()
            -> createQosControllers()
            -> enter control loop
```

### 10.1 MP4 bootstrap 为什么有时会跳过

如果：

- `PLAIN_CLIENT_VIDEO_SOURCES` 已经为所有 track 提供了显式视频源

那么可以跳过 MP4 video bootstrap。

但如果还需要 audio worker：

- audio 仍然可能从 `mp4Path_` 重新打开文件读取

所以“跳过 MP4 bootstrap”不等于整个进程完全不依赖 `mp4Path_`。

## 11. steady-state 主循环

当前真正长期运行的控制循环在 `ThreadedQosRuntime::Run()` 里。

每轮大致做这些事：

```text
dispatchNotifications()
processStatsQueue()
processNetworkAckQueue()
processWorkerAckQueues()
maybeRunSampling()
sleep(50ms)
```

### 11.1 `dispatchNotifications()`

消费 server 下发的：

- `qosPolicy`
- `qosOverride`

并转给对应 track 的 `PublisherQosController`。

### 11.2 `processStatsQueue()`

消费 `NetworkThread` 推回来的 `SenderStatsSnapshot`，更新每 track 的最新 transport 观察值。

### 11.3 `process*AckQueue()`

消费 network/source 两侧 ack，确认命令是否真正落地。

### 11.4 `maybeRunSampling()`

如果到达最小 sample interval：

- 为每个 track 构造 `CanonicalTransportSnapshot`
- 调用 controller `onSample()`
- 收集 peer 级 signals / actions / raw snapshot
- 做多 track coordination
- 如有需要，上传 `clientStats`

## 12. QoS 的内环 / 外环边界

这个边界非常重要。

### 12.1 内环

内环是 transport truth：

- packet send history
- transport-wide sequence
- RTCP / TWCC feedback
- pacing
- send-side estimate
- probe
- retransmission

当前 owner：

- `NetworkThread`

### 12.2 外环

外环是策略决策：

- track level
- audio-only
- pause / resume
- encoding parameters
- multi-track coordination

当前 owner：

- control thread 上的 `PublisherQosController`
- `ThreadedQosRuntime` 的 peer coordination 逻辑

### 12.3 当前主链路

```text
local transport stats + RTCP RR
  -> CanonicalTransportSnapshot
  -> PublisherQosController::onSample()
  -> command / transport hint
  -> ack
  -> peer snapshot
  -> clientStats
```

server `getStats` 现在只用于：

- 观测
- 对账
- harness / report

不再进入 sender QoS 主控制链。

## 13. 多 track 协调怎么做

当前多 track 协调并不是在 `NetworkThread` 里做。

顺序是：

1. 每个 track 先各自 `onSample()`
2. control thread 收集：
   - `PeerTrackState`
   - `DerivedSignals`
   - last action
   - raw snapshot
3. 根据 track request 计算 `TrackBudgetRequest`
4. 用：
   - `allocatePeerTrackBudgets()`
   - `buildCoordinationOverrides()`
5. 再把 coordination override 回写给每个 controller

所以：

- 单 track 决策是 per-track controller
- 多 track 协调是 runtime 层统一做

## 14. 为什么 `clientStats` 上传也放在 runtime

因为它本质上属于 sender runtime 的观测出口。

上传前需要拿到同一轮的：

- `PeerTrackState`
- `DerivedSignals`
- last action
- raw transport snapshot
- action applied state

这些信息都在 `ThreadedQosRuntime` 里最完整，所以放这里最合理。

## 15. 当前不变量

实现上最值得记住的几个不变量：

1. `NetworkThread` 是唯一 UDP / RTP / RTCP / TWCC owner。
2. `SourceWorker` 不直接发送网络包。
3. `PlainClientApp` 不再拥有 sender runtime loop。
4. 每个 video track 各自维护 source/encode 生命周期。
5. sender QoS 控制输入只来自本地 transport snapshot。
6. server `getStats` 不进入 sender QoS 主控制链。
7. 主 `plain-client` 二进制只支持 threaded runtime。

## 16. 常见误解

### 16.1 “是不是每个 video track 都有一个网络线程？”

不是。

每个 video track 只有一个 `SourceWorker` 生产线程。

真正的发送线程全局只有一个：

- `NetworkThread`

### 16.2 “control thread 会不会直接改 encoder？”

不会。

control thread 只发 command，真正改 encoder 的是对应 `SourceWorker`。

### 16.3 “audio worker 是不是必须有？”

不是。

只有输入里存在 audio stream 且音频初始化成功才会启动。

### 16.4 “为什么不把所有事都塞回一个线程？”

因为那样虽然线程数少，但 ownership 会变坏：

- 多路 source 相互阻塞
- transport owner 不清晰
- TWCC/BWE/pacing 更难保证一致

## 17. 阅读代码的推荐顺序

如果你要从代码读起，推荐顺序是：

1. `client/main.cpp`
2. `client/PlainClientApp.*`
3. `client/PlainClientThreaded.cpp`
4. `client/ThreadedQosRuntime.*`
5. `client/ThreadedControlHelpers.h`
6. `client/NetworkThread.h`
7. `client/SourceWorker.h`
8. `client/qos/*`

## 18. 和现有其他文档的关系

- 这份文档：
  讲“当前 plain-client 架构和线程模型到底是什么”
- [linux-client-architecture_cn.md](architecture_cn.md)：
  更短的架构摘要
- [plain-client-qos-status.md](README.md)：
  结果和入口摘要
- [plain-client-qos-boundary-review_cn.md](plain-client-qos-boundary-review_cn.md)：
  历史边界问题复盘
- [livekit-alignment-plan_cn.md](livekit-alignment-plan_cn.md)：
  更高标准的演进规划

## 19. 结论

当前 plain-client 的线程模型可以压成一句话：

- 每路视频一个生产线程
- 全局一个发送线程
- 全局一个控制/协调线程

这么做的核心不是“多线程”，而是：

- source ownership 独立
- transport ownership 单点
- QoS 协调统一

这就是当前 plain-client 能同时承载：

- 多 track
- 真发送侧 transport control
- TWCC / send-side BWE
- peer-level QoS coordination

的基础。
