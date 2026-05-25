# WebRTC QoS SDK 推拉流客户端线程模型升级设计方案

> 文档状态：review draft
>
> 范围：P2 之后的推拉流客户端 runtime 架构升级。
> 本文定义设计、验收标准和当前 P3 落地口径。

## 1. 结论

P2 已经完成 native push/play、QoS SDK 边界、实时 x264 输入源、V4L2 入口、QoE、
弱网报告和验收门禁。但当前客户端 runtime 仍是偏同步的单 loop 模型，适合单路
文件或短链路验证，不适合作为多摄像头、多 track、实时编码的长期模型。

下一期需要做线程模型升级，核心结论如下：

- push 侧是主战场：多摄像头、多路编码、多 track 推流的压力主要发生在发送端。
- 默认通信方式是消息投递：通过 owner event loop 串行处理状态变化，尽量不要用锁保护
  跨线程业务状态。
- SDK 对象必须单线程拥有：`VideoPushClient` / `VideoPlayClient` 只允许在各自
  SDK/transport 线程调用，避免 facade 被跨线程并发调用。
- 采集和编码在 V4L2/多摄像头目标形态下默认拆开：采集有外部时钟和设备阻塞风险，
  编码有 CPU 峰值和码率重配成本，合在一起会让背压位置不可控。
- 输入源不能一刀切：MP4 copy 是已编码 AU 输入，不进入 raw/encode 链路；V4L2 是
  真实实时输入，必须按 capture -> raw queue -> encode -> encoded queue -> SDK 设计；
  synthetic 和 MP4 decode-loop 可以先 fused，但验收指标要能对齐到同一套队列/延迟口径。
- play 侧也要拆：UDP/SDK 线程不能被文件写入、FFmpeg decode 或 QoE 计算阻塞。
- 多接收端 fanout 不进入本次目标；play 侧只为验证多 track 接收和 QoE 隔离服务。

## 2. 当前模型

### 2.1 Push 当前执行链路

当前 `WebRtcQosPushRuntime::Run()` 在一个循环里串行执行：

```text
DispatchNotifications()
  -> DrainUdpFeedback()
  -> VideoPushClient::GetEncoderAdaptation()
  -> source/decode/capture
  -> x264 encode 或 copy AU
  -> VideoPushClient::PushAnnexBAccessUnit()
  -> VideoPushClient::Process()
  -> metrics log
  -> sleep(processTickMs)
```

这个模型的优点是简单，P2 验证成本低；问题是任何一步变慢都会影响其他步骤：

- V4L2 read 卡住，会延迟 RTCP feedback 处理。
- x264 编码 spike，会延迟 `Process()`，影响 pacer、GoogCC、RTCP。
- 多 track 时，一个 track 的采集/编码会阻塞其他 track。
- 文件日志、metrics、source open/close 异常路径也可能放大 loop gap。

### 2.2 Play 当前执行链路

当前 `WebRtcQosPlayRuntime::Run()` 同样在一个循环里串行执行：

```text
DispatchNotifications()
  -> UDP recv RTP/RTCP
  -> VideoPlayClient::OnRtpPacket()/OnRtcpPacket()
  -> VideoPlayClient::Process()
  -> decoded_access_unit_output callback
  -> file sink / FFmpeg decode QoE
  -> metrics log
  -> sleep(processTickMs)
```

问题是 decode sink 和文件写入属于重活，不能阻塞 SDK 接收、NACK/PLI 定时器和
RTCP feedback 输出。

## 3. 目标

### 3.1 可实施目标

- 支持一个 push 进程内推多路 H264 video track。
- 支持多路输入源：synthetic、MP4 copy、MP4 decode-loop、V4L2 camera。
- 支持每个实时 track 独立采集、独立编码、独立 adaptation 应用。
- 支持一个 SDK/transport 线程统一拥有 `VideoPushClient` 和 UDP socket。
- 支持 play 侧 SDK 接收线程与 sink/decode/QoE worker 解耦。
- 保留当前 `plainPublish` / `plainSubscribe` 信令路线。
- 继续只消费 `webrtc_qos_sdk` public facade，不回退自研 RTP/RTCP/QoS。

### 3.2 可验证目标

- 2 路 synthetic x264 推流在普通 CPU 环境必跑。
- 2 路 MP4 decode-loop 推流在普通 CPU 环境必跑。
- 真实 V4L2 多摄像头在有 `/dev/video*` 的机器上 PASS，无设备时只允许 SKIP。
- 慢编码、慢 sink、弱网、恢复场景都能产生明确报告。
- SDK `Process()` gap、RTCP feedback gap、队列积压和丢帧都有门禁。

### 3.3 可观测目标

- 每个线程有 heartbeat、loop gap、stop reason。
- 每个 track 有 capture、raw queue、encode、encoded queue、SDK push、QoE 指标。
- 每类背压都有日志、metrics 和 alert。
- 报告能定位问题发生在 signaling、capture、encode、SDK、UDP、play sink 或 QoE。

## 4. 非目标

- 不做多接收端 fanout 产品化。
- 不做完整 PeerConnection。
- 不做 ICE / DTLS / SRTP / SDP。
- 不做 GPU/NVENC 必需路径；GPU 只能作为后续可选优化。
- 不改变 `webrtc_qos_sdk` 的 QoS 算法。
- 不把 SDK 内部对象暴露给多个业务线程并发调用。
- 不把 V4L2 无设备环境伪造成 PASS。
- 不新增自研 RTP packetizer、RTCP、pacer、GoogCC、jitter buffer。

## 5. Push 目标线程模型

### 5.1 拓扑

```text
SignalingActor
  -> join / plainPublish / notifications
  -> ControlMailbox

CaptureWorker[track 0] -> RawFrameQueue[track 0] -> EncodeWorker[track 0]
CaptureWorker[track 1] -> RawFrameQueue[track 1] -> EncodeWorker[track 1]
...

AuSourceWorker[copy track]
  -> EncodedAuQueue[copy track]

EncodeWorker[N]
  -> EncodedAuQueue[N]
  <- EncoderControlSnapshot[N]

SdkTransportThread
  -> owns VideoPushClient
  -> owns UDP socket
  -> drains EncodedAuQueue[*]
  -> PushAnnexBAccessUnit()
  -> OnTransportFeedback()
  -> Process()
  -> publishes EncoderControlSnapshot[*]

MetricsThread
  -> reads thread/track counters
  -> writes log / metrics / alerts files
```

### 5.2 线程职责

| 线程 | 数量 | 拥有对象 | 允许阻塞点 | 禁止事项 |
|---|---:|---|---|---|
| `SignalingActor` | 1 个逻辑 owner | `WsClient`、push signaling state | WebSocket request/response | 不调用 SDK，不做编码，不做 UDP media I/O；媒体线程不能直接调用它 |
| `SdkTransportThread` | 1 | `VideoPushClient`、UDP socket、SDK session | `epoll_wait` / timer wait | 不做 V4L2 read，不做 FFmpeg decode，不做 x264 encode，不写重文件 |
| `CaptureWorker` | 每实时 track 1 个 | V4L2 fd 或 source reader | V4L2 poll/read、文件读 | 不调用 SDK，不编码重活 |
| `EncodeWorker` | 每实时 track 1 个 | x264 encoder context | raw queue wait、x264 encode | 不调用 SDK，不做 UDP I/O |
| `AuSourceWorker` | 每 copy track 0 或 1 个 | MP4/H264 AU reader | 文件读 | 不调用 SDK，不伪装 encoder adaptation |
| `MetricsThread` | 0 或 1 | metrics writer | file write | 不影响媒体线程进度 |

### 5.3 为什么采集和编码默认分开

单路 synthetic 或短测可以把采集和编码合在一个 worker 里，但 V4L2/多摄像头默认
不这样做。

原因：

- 摄像头有外部时钟。编码慢时如果不及时 drain device buffer，后续拿到的是旧帧。
- V4L2 read/poll 可能因为设备、驱动、USB 抖动而阻塞，不能拖住 encoder adaptation。
- x264 重配码率、force IDR、CPU 抢占会产生 spike，不能反过来阻塞摄像头采集。
- 分开后，背压位置清晰：raw queue 满就丢旧 raw frame，encoded queue 满就丢旧 AU
  并请求 keyframe，SDK thread 永远不被源和编码拖慢。

### 5.4 输入源线程策略

不同 source 的线程策略不能强行统一：

| source | 线程策略 | 说明 |
|---|---|---|
| MP4 H264 copy | `AuSourceWorker -> EncodedAuQueue -> SDK` | 输入已经是 H264 AU，不经过 raw frame 和 x264，不能用来证明 encoder adaptation。 |
| synthetic x264 | 第一版可 fused，目标形态可 `GenerateRaw -> EncodeWorker` | 没有真实设备阻塞，主要用于可重复多 track 和慢编码注入测试。 |
| MP4 decode-loop | 第一版可 fused，压力测试时拆 `DecodeWorker -> RawFrameQueue -> EncodeWorker` | 文件源没有外部实时采集压力，但有 FFmpeg decode 和 x264 CPU 压力。 |
| V4L2 camera | 默认 split：`CaptureWorker -> RawFrameQueue -> EncodeWorker` | 真实设备有外部时钟和阻塞风险，必须让 SDK thread 与 encoder 解耦。 |

实现上可以保留优化开关：

- `fused`：synthetic、MP4 decode-loop、单路开发调试可用，capture/decode/encode 在同一 worker。
- `split`：V4L2、多路实时输入默认模式，capture 和 encode 分开。
- `copy`：MP4 H264 copy 专用模式，只生成 encoded AU，不产生 raw queue 和 encoder metrics。

外部观测和验收按 `split` 模型设计，即使内部某些 source 临时 fused，也要输出同样的
queue、latency、drop 指标；copy 模式的 raw/encoder 指标必须显式标记为 `N/A` 或
`SKIP`，不能伪造成 PASS。

## 6. Play 目标线程模型

### 6.1 拓扑

```text
SignalingActor
  -> join / plainSubscribe / newConsumer
  -> ControlMailbox

PlaySdkTransportThread
  -> owns VideoPlayClient
  -> owns UDP socket
  -> OnRtpPacket()/OnRtcpPacket()
  -> Process()
  -> transport_output RTCP
  -> decoded_access_unit_output pushes AU to queue

DecodeSinkWorker[track 0]
DecodeSinkWorker[track 1]
...
  -> file output / null output / FFmpeg decode QoE

MetricsThread
  -> logs / metrics / alerts
```

### 6.2 线程职责

| 线程 | 数量 | 拥有对象 | 允许阻塞点 | 禁止事项 |
|---|---:|---|---|---|
| `SignalingActor` | 1 个逻辑 owner | `WsClient`、consumer selection | WebSocket request/response | 不调用 SDK，不做 UDP media I/O；媒体线程不能直接调用它 |
| `PlaySdkTransportThread` | 1 | `VideoPlayClient`、UDP socket | `epoll_wait` / timer wait | 不做 FFmpeg decode，不同步写大文件 |
| `DecodeSinkWorker` | 最小实现 1 个聚合 worker；完整实现每输出 track 1 个 | Annex-B sink、FFmpeg decoder/QoE | queue wait、decode、file write | 不调用 SDK，不影响 RTCP feedback |
| `MetricsThread` | 0 或 1 | metrics writer | file write | 不影响媒体线程进度 |

### 6.3 Play 侧升级重点

play 侧不是为了做多接收端，而是为了保证接收、反馈和 QoE 互不阻塞：

- `decoded_access_unit_output` callback 必须快速返回，只能入队或计数。
- sink/decode 慢时只影响该 track 的输出和 QoE，不影响 `OnRtpPacket()`、NACK/PLI、RR/TWCC。
- 多 track play 只选择同一 push peer 的多个 video consumer，用于验证多 track push；
  不订阅多个发布端，不扩展成多用户 fanout。
- P3 合入实现要求每个 selected video track 一个 sink/decode worker；`AnnexBAccessUnitView.ids.track_id`
  必须用于路由，不能把多路 AU 混入同一个 QoE 状态机。

## 7. SDK 调用边界

### 7.1 Push SDK 边界

下面这些调用只能发生在 `SdkTransportThread`：

- `CreateVideoPushClient()`
- `VideoPushClient::Start()`
- `VideoPushClient::Stop()`
- `VideoPushClient::Process()`
- `VideoPushClient::PushAnnexBAccessUnit()`
- `VideoPushClient::OnTransportFeedback()`
- `VideoPushClient::OnNetworkRouteChange()`
- `VideoPushClient::OnSenderRateCap()`
- `VideoPushClient::GetEncoderAdaptation()`
- `VideoPushClient::GetTrackEncoderAdaptation()`
- `VideoPushClient::GetQosSnapshot()`
- `VideoPushClient::GetTrackQosSnapshot()`

编码线程不能直接调用 `GetTrackEncoderAdaptation()`。SDK thread 每 tick 拉取每个 track
的 adaptation，然后发布不可变的 `EncoderControlSnapshot` 给对应 encoder worker。

### 7.2 Play SDK 边界

下面这些调用只能发生在 `PlaySdkTransportThread`：

- `CreateVideoPlayClient()`
- `VideoPlayClient::Start()`
- `VideoPlayClient::Stop()`
- `VideoPlayClient::Process()`
- `VideoPlayClient::OnRtpPacket()`
- `VideoPlayClient::OnRtcpPacket()`
- `VideoPlayClient::GetQosSnapshot()`
- `VideoPlayClient::GetTrackQosSnapshot()`

`decoded_access_unit_output` callback 也视为 SDK thread 上下文，不能做重活。

### 7.3 Transport callback 边界

SDK 的 `transport_output` callback 在 SDK thread 中执行：

- push: RTP/RTCP output 直接通过 SDK thread 拥有的 UDP socket send。
- play: RTCP feedback output 直接通过 play SDK thread 拥有的 UDP socket send。
- callback 内不能等待其他 worker 完成，不能做同步文件写大块数据。

## 8. 线程间通信设计

### 8.1 从 mediasoup-cpp 服务端线程模型借鉴的原则

服务端现有线程模型里，最有价值的不是线程数量，而是通信边界：

- `uWS` 主线程拥有 WebSocket、HTTP、timer 和 `roomDispatch_`。
- `WorkerThread` 拥有 `RoomService`、`RoomManager`、`WorkerManager`、task queue、
  timerfd、eventfd，以及 mediasoup worker 的 Channel fd。
- 主线程不直接执行业务，而是 `wt->post(lambda)` 投递到房间所属 `WorkerThread`。
- `post()` 只把任务放入队列并写 `eventfd`，`WorkerThread` 的 `epoll_wait` 被唤醒后
  在 owner 线程串行执行 `processTaskQueue()`。
- `WorkerThread` 不能直接操作 uWS socket，回包和广播必须 `loop->defer(lambda)` 回
  uWS owner 线程。
- non-threaded `Channel` 的 consumer fd 注册在 `WorkerThread` 的 epoll 里，所有
  `requestWait()` 必须在 `WorkerThread` 上发生，避免另起读线程和 `.get()` 死锁。
- Redis 这类慢路径不塞进房间控制链路，而是投递到 registry worker 串行执行。

这个模式映射到推拉流客户端时要注意一个差异：当前 `client/WsClient` 已经内置
reader thread、pending request map、notification queue 和短临界区锁。它不是 uWS
那种“完全由外部 owner loop 持有 socket”的模型。因此这里借鉴的是 owner 边界和消息
投递原则，而不是照搬 uWS 的实现形态。

- `SignalingActor` 对应 uWS 主线程：逻辑上拥有 `WsClient`，只做信令收发和通知解析。
  第一版可以复用 `WsClient` 内部 reader thread 和锁；媒体线程仍然不能直接使用
  `WsClient`。如果后续要进一步收敛锁，可以把 `WsClient` 改成真正的单 owner event loop。
- `PushSdkTransportThread` / `PlaySdkTransportThread` 对应 `WorkerThread`：拥有 SDK
  facade、UDP socket、timer 和媒体状态，所有 SDK 调用都在这个 owner 线程串行执行。
- capture、encode、sink 对应 side worker：它们拥有会阻塞或 CPU 重的资源，不能反向
  调用 SDK owner，只能投递数据或事件。
- `ControlMailbox.post(event)` 对应 `wt->post(lambda)`：跨线程控制事件只投递到 owner
  event loop，不直接修改 owner 状态。
- `StatsSnapshot` / `ClientStatsSnapshot` 对应 `loop->defer` 的反向边界：SDK thread
  不直接发 WebSocket，只发布快照，由 SignalingActor 在自己的边界内发送。
- UDP fd、eventfd、timerfd 都注册进 SDK owner 的 epoll；SDK owner 在同一个 loop 内
  处理 UDP feedback、control event、SDK `Process()` 和 encoded AU。

因此客户端线程通信也要坚持同一条不变量：

```text
跨线程只投递任务/事件/数据所有权
owner event loop 串行修改 owner 状态
跨回原 owner 线程才能访问该 owner 的对象
慢路径和重活不进入实时 owner loop
锁只能保护短临界区容器，不能保护业务状态机
```

### 8.2 总原则

线程之间不共享可变业务对象，不靠“大家拿锁访问同一个 runtime state”推进媒体链路。

通信规则：

- 默认用消息投递解决线程通信，不把锁作为主设计手段。
- SDK facade 对象、UDP socket、encoder context、V4L2 fd、FFmpeg decoder/sink 都有唯一拥有线程。
- 跨线程只传不可变事件、所有权转移的数据包、最新快照和原子 metrics。
- 媒体热路径上的 push 默认不阻塞；满队列时按策略丢弃并计数。
- SDK/transport thread 不能等待 capture、encode、sink、metrics 或 signaling 完成。
- control event 可以 fail fast，但不能静默丢。
- 数据 item 不能保存指向其他线程栈、SDK callback 临时 view、x264 packet 临时内存的裸指针。
- 允许锁的地方只限短临界区：队列 push/pop、快照 swap、metrics map 更新。
- 禁止用锁包住 SDK 调用、WebSocket 调用、FFmpeg/x264 调用、V4L2 read、文件写入。

### 8.3 通信原语

| 原语 | 用途 | 实现建议 | 关键要求 |
|---|---|---|---|
| `BoundedQueue<T>` | raw frame、encoded AU、decoded AU、metrics event | C++17 `std::mutex` + `std::condition_variable` | 有容量、有关闭语义、有 drop 计数，不做无界队列 |
| `ControlMailbox<T>` | signaling/control -> SDK thread | small `std::deque` + `std::mutex` + `eventfd` | control 满时 fail fast，触发 fatal，不静默覆盖 |
| `LatestValue<T>` | SDK adaptation -> encoder worker | 小锁或 double-buffer + atomic epoch | 只保留最新快照，读到旧 epoch 也能继续跑 |
| `AtomicCounters` | 高频 metrics | `std::atomic<uint64_t>` / `std::atomic<int64_t>` | 只做观测，不作为复杂控制状态 |
| `EventFdNotifier` | 唤醒 SDK event loop | Linux `eventfd`，必要时 pipe fallback | encoded/control 入队后唤醒 SDK thread |
| `TimerFd` | 固定 tick | Linux `timerfd` 或 steady_clock wait | 保证 SDK `Process()` 周期，不依赖有包才跑 |

为什么不用全局锁：

- 全局锁会把 V4L2 read、x264 encode、SDK pacing、UDP feedback 串成隐式临界区。
- 一旦某个 worker 在持锁时阻塞，SDK thread 就会错过 feedback 和 `Process()` tick。
- 多 track 下锁竞争会让问题表现成随机卡顿，排障难度高。
- 锁保护的共享业务状态会让 owner 不清晰，后续加 track、加 sink、加重试逻辑时容易形成
  死锁和锁顺序问题。

锁使用边界：

- 可以用锁实现 `BoundedQueue` 的内部容器。
- 可以用锁做 `LatestValue` 的短时间 swap。
- 可以用锁保护低频 metrics registry。
- 不允许一个线程拿锁后再调用另一个线程 owner 的对象。
- 不允许持锁等待其他线程回调或 ack。

为什么不先上复杂 lock-free：

- 当前目标是可实施、可验证。`mutex + condition_variable + eventfd` 足够支撑 2 到 4
  路视频的第一版。
- 只有在 metrics 证明队列锁成为瓶颈后，再把单个热点队列替换成 SPSC ring buffer。

### 8.4 通信通道清单

| 通道 | 生产者 | 消费者 | 数据所有权 | 唤醒方式 | 满时行为 |
|---|---|---|---|---|---|
| `PushControlMailbox` | SignalingActor / supervisor | SdkTransportThread | value event | eventfd | fail fast + fatal |
| `PlayControlMailbox` | SignalingActor / supervisor | PlaySdkTransportThread | value event | eventfd | fail fast + fatal |
| `RawFrameQueue[track]` | CaptureWorker | EncodeWorker | `unique_ptr<RawFrame>` | condition_variable | drop oldest raw frame |
| `EncodedAuQueue[track]` | EncodeWorker | SdkTransportThread | `unique_ptr<EncodedAccessUnit>` | eventfd | drop stale P-frame，必要时 request IDR |
| `CopyAuQueue[track]` | AuSourceWorker | SdkTransportThread | `unique_ptr<EncodedAccessUnit>` | eventfd | 按 media time 丢过期 AU，不计 encoder runtime |
| `EncoderControl[track]` | SdkTransportThread | EncodeWorker | immutable snapshot copy | atomic epoch + optional cv notify | overwrite latest |
| `DecodedAuQueue[track]` | PlaySdkTransportThread | DecodeSinkWorker | `unique_ptr<DecodedAccessUnit>` | condition_variable | realtime drop oldest；test fail fast |
| `StatsSnapshot` | SDK / workers | MetricsThread / SignalingActor | immutable snapshot copy | periodic poll 或 metrics queue | overwrite latest |
| `FatalEvent` | any worker | supervisor/main | value event | eventfd 或 condition_variable | first fatal wins |

### 8.5 数据对象所有权

建议定义三类跨线程数据对象：

```cpp
struct RawFrame {
  uint32_t trackId;
  int64_t captureTimeUs;
  int width;
  int height;
  PixelFormat format;
  std::vector<uint8_t> planes;
};

struct EncodedAccessUnit {
  uint32_t trackId;
  int64_t captureTimeUs;
  int64_t enqueueTimeUs;
  bool keyframe;
  std::vector<uint8_t> annexB;
};

struct DecodedAccessUnit {
  uint32_t trackId;
  int64_t captureTimeUs;
  int64_t receiveTimeUs;
  bool keyframe;
  std::vector<uint8_t> annexB;
};
```

要求：

- queue item 入队后，生产线程不再修改。
- SDK callback 给出的 `AnnexBAccessUnitView` 生命周期只在 callback 内有效，play 侧入队前必须拷贝。
- x264 / FFmpeg packet 内存不能跨线程裸传，必须拷贝或转移到自有 buffer。
- metrics 可以引用 trackId，不引用媒体 buffer。

### 8.6 Push SDK event loop

`SdkTransportThread` 使用一个统一 event loop：

```text
epoll_wait(udp_fd, eventfd, timerfd)
  -> drain UDP RTCP feedback with packet/time budget
  -> drain PushControlMailbox
  -> Process(now)
  -> publish per-track EncoderControl snapshot
  -> drain EncodedAuQueue[*] round-robin with budget
  -> post-push Process(now) if pushed any AU
  -> update metrics snapshot
```

优先级：

1. RTCP feedback 优先，避免 TWCC/RR/NACK/PLI 延迟。
2. control event 第二，例如 route change、rate cap、shutdown。
3. SDK `Process()` 必须按 timer 保底执行，即使没有新 AU。
4. encoded AU 采用 round-robin drain，避免单 track 饿死其他 track。
5. metrics 最后做，且不能阻塞。

预算建议：

- 单轮 UDP feedback 最多处理 64 个包或 2 ms。
- 单轮 encoded AU 每 track 最多处理 1 到 2 个 AU，再切到下一个 track。
- 单轮总媒体处理超过 5 ms 时记录 `sdk_loop_over_budget`。
- timer tick 建议 5 到 10 ms，最终以 `sdkProcessGapMaxMs` 验收。

这个 loop 的结构刻意对齐服务端 `WorkerThread`：

- `eventfd` 对应 task queue 唤醒。
- `timerfd` 对应 health/delayed timer 唤醒。
- `udp_fd` 对应 Channel fd，由 owner loop 统一 pump。
- `EncodedAuQueue` 对应待处理业务任务，但媒体数据 drain 必须有预算，不能一次清空导致
  feedback 饿死。

### 8.7 Capture -> Encode

CaptureWorker 只负责把外部时钟转换成 raw frame：

```text
poll/read device or source
  -> allocate RawFrame
  -> fill captureTimeUs
  -> RawFrameQueue.try_push_drop_oldest(frame)
  -> update capture metrics
```

规则：

- V4L2 capture worker 允许阻塞在设备 poll/read，但不能拿任何 SDK/encoder 锁。
- raw queue 容量只保留 2 到 3 帧，满了丢旧帧，保证编码处理的是新画面。
- capture stall 只影响该 track，不影响 SDK thread 和其他 track。
- `captureTimeUs` 是端到端延迟基准，后续 encode 和 SDK 不重写。

### 8.8 Encode -> SDK

EncodeWorker 消费 raw frame 并输出 Annex-B AU：

```text
wait RawFrameQueue
  -> load latest EncoderControlSnapshot
  -> apply bitrate/fps/keyframe request
  -> maybe drop frame by fps adaptation
  -> x264 encode
  -> EncodedAuQueue.try_push(au)
  -> eventfd notify SdkTransportThread
```

规则：

- encoder worker 不调用 SDK，只读 `EncoderControlSnapshot`。
- fps adaptation 在 encoder worker 执行，避免 SDK thread 处理 raw frame。
- 如果 `requestKeyframe` epoch 更新，下一帧必须 force IDR，并记录 delay。
- encoded queue 满时不阻塞 encoder 太久；丢弃策略必须保留恢复能力。

encoded queue 满时的建议顺序：

1. 先丢过期 P-frame。
2. 如果队列最老 AU 超过 age 上限，清掉旧 AU。
3. 如果丢弃跨过 keyframe 或清空队列，设置 `needKeyframeDueToBackpressure`。
4. 通过 `EncoderControlSnapshot` 或本地 encoder flag 触发下一帧 IDR。

### 8.9 SDK -> Encoder adaptation

SDK thread 每 tick 发布每 track 最新控制快照：

```cpp
struct EncoderControlSnapshot {
  uint32_t trackId;
  uint64_t epoch;
  int64_t publishTimeUs;
  uint32_t targetBitrateBps;
  uint32_t minBitrateBps;
  uint32_t maxBitrateBps;
  int maxFps;
  bool requestKeyframe;
  bool needKeyframeDueToBackpressure;
};
```

发布规则：

- SDK thread 调 `GetTrackEncoderAdaptation(trackId, nowUs)`，把结果写入
  `LatestValue<EncoderControlSnapshot>`。
- encoder worker 在每次编码前读取最新 epoch。
- `requestKeyframe` 必须带 epoch，encoder 只对同一个 epoch 响应一次。
- snapshot 覆盖旧值，不排队；码率控制只需要最新状态。
- 如果 encoder worker 长时间没有读取新 epoch，记录 `encoder_control_lag`。

### 8.10 Play SDK -> Sink

play 侧 `decoded_access_unit_output` 不能做重活：

```text
decoded_access_unit_output(view)
  -> copy view bytes into DecodedAccessUnit
  -> DecodedAuQueue.try_push(...)
  -> return quickly
```

规则：

- callback 目标耗时 <= 2 ms。
- realtime 模式队列满时丢旧 AU，保证 sink 追新。
- test 模式不允许无限阻塞 SDK thread；队列满时可以短超时，超时后 fail case。
- DecodeSinkWorker 负责 Annex-B 文件写入、FFmpeg decode、QoE 计算。
- sink/decode 卡住时只触发 `sink_backpressure`，不能影响 `OnRtpPacket()` 和 RTCP feedback。

### 8.11 Signaling 通信

`SignalingActor` 是逻辑 owner，不要求第一版一定新增一个纯事件循环线程。当前可复用
`WsClient` 的 reader thread，但必须做到：

- 只有 `SignalingActor` 代码直接调用 `WsClient::request()`、`requestAsync()`、
  `dispatchNotifications()`、`sendText()` 和 `close()`。
- SDK/transport、capture、encode、sink、metrics 线程不能直接持有 `WsClient*`。
- 如果需要向服务端发送 `clientStats` 或 keyframe request，先投递 `SignalingCommand`，
  由 SignalingActor 串行调用 `WsClient`。
- `WsClient` 内部锁只能看作网络客户端封装细节，不能扩散成媒体 runtime 的锁设计。

从 signaling 到 SDK：

- `qosPolicy` / `qosOverride` notification -> `ControlEvent::QosPolicyUpdated`
- network route / bitrate envelope 变化 -> `ControlEvent::NetworkRouteChanged`
- selected consumer 或 track lifecycle 变化 -> `ControlEvent::TrackAdded/TrackRemoved`
- shutdown -> `ControlEvent::Shutdown`

从 SDK / metrics 到 signaling：

- SDK 和 worker 只发布 `ClientStatsSnapshot`。
- SignalingActor 定时读取最新 snapshot 并发送 `clientStats`。
- 如果发送失败，记录 signaling error，但不能阻塞 SDK thread。

### 8.12 停止和错误传播

停止使用统一 `RuntimeStopState`：

```text
atomic<bool> stopping
atomic<int> exitCode
string firstError
deadlineUs
```

错误传播规则：

- 任意 worker 发现 fatal，写入 `FatalEvent`。第一个 fatal 决定进程退出原因。
- supervisor/main 设置 `stopping=true`，关闭所有 queue，并写 eventfd 唤醒 SDK thread。
- queue `Close()` 后，阻塞的 consumer 立即返回 closed。
- worker 退出前写 final metrics，不能等待 metrics thread ack。
- join 线程时不能持有 queue mutex，避免退出死锁。

推荐关闭顺序：

```text
supervisor sets stopping
  -> close ControlMailbox
  -> close RawFrameQueue / EncodedAuQueue / DecodedAuQueue
  -> notify eventfd/cv
  -> join CaptureWorker
  -> join EncodeWorker
  -> SDK thread drain bounded time and Stop()
  -> join SinkWorker
  -> join SignalingActor / close WsClient
  -> final metrics summary
```

### 8.13 禁止的通信方式

- 禁止 encoder worker 直接调用 `VideoPushClient`。
- 禁止 sink worker 直接调用 `VideoPlayClient`。
- 禁止 SDK callback 等待 condition_variable 直到 sink 完成。
- 禁止跨线程共享 `AVFrame*`、`AVPacket*`、x264 packet 指针或 SDK view 指针。
- 禁止无界 queue。
- 禁止用 metrics 原子计数反向驱动复杂控制逻辑。
- 禁止在持有 queue lock 时调用日志、SDK、FFmpeg、x264 或 WebSocket。
- 禁止为了省拷贝把同一块 Annex-B buffer 同时交给多个线程修改。

## 9. Push 数据流

### 9.1 启动

```text
main
  -> parse CLI tracks
  -> start SignalingActor
  -> join
  -> plainPublish(videoSsrcs[])
  -> build SessionConfig(video_tracks[])
  -> start SdkTransportThread
  -> start CaptureWorker / EncodeWorker per track
```

`plainPublish` 服务端已经支持 `videoSsrcs[]` 和返回 `videoTracks[]`。当前客户端只使用
单个 `videoSsrc` 并要求 `videoTracks.size() == 1`，线程模型升级时需要改成多 track
解析，但仍使用同一个 PlainTransport。

### 9.2 采集到编码

这一节只适用于 synthetic、MP4 decode-loop 和 V4L2 这类需要生成 raw frame 再编码的
输入源。MP4 H264 copy 不经过这个链路。

```text
CaptureWorker(track i)
  -> capture raw frame
  -> timestamp captureTimeUs
  -> push RawFrameQueue(track i)

EncodeWorker(track i)
  -> read latest EncoderControlSnapshot(track i)
  -> apply bitrate/fps/keyframe request
  -> pop raw frame
  -> x264 encode
  -> push EncodedAuQueue(track i)
```

关键规则：

- capture timestamp 在采集时确定，后续不重写。
- encoder 根据 SDK adaptation 做 fps decimation，而不是让 SDK thread 丢 raw frame。
- `request_keyframe` 由 encoder worker 在下一帧 force IDR，并上报 delay。

### 9.3 编码到 SDK

实时编码源通过 encoded queue 进入 SDK：

```text
SdkTransportThread
  -> drain UDP RTCP feedback
  -> OnTransportFeedback()
  -> GetTrackEncoderAdaptation(track i)
  -> publish EncoderControlSnapshot(track i)
  -> drain EncodedAuQueue(track i)
  -> PushAnnexBAccessUnit(ids = track.ids)
  -> Process()
  -> GetQosSnapshot()
```

关键规则：

- `AnnexBAccessUnitView.ids` 必须使用对应 `video_tracks[i].ids`。
- SDK thread 可以 round-robin drain 多个 encoded queue，避免单 track 长时间占用。
- 如果 encoded AU 已经过期，SDK thread 不发送旧帧，按背压策略丢弃并触发 keyframe。

MP4 H264 copy 源进入 SDK 的方式不同：

```text
AuSourceWorker(copy track)
  -> read Annex-B AU with mediaTimeUs
  -> CopyAuQueue(track i)

SdkTransportThread
  -> schedule by mediaTimeUs
  -> PushAnnexBAccessUnit(ids = track.ids)
  -> Process()
```

copy 源规则：

- 不调用 encoder adaptation。
- 不输出 raw frame / encoder metrics。
- 可以参与 transport、RTCP/TWCC、weak-network、QoE 验证。
- `encoderRuntime` 必须为 `SKIP` 或 `N/A`，不能计入实时编码器验收。

## 10. Play 数据流

### 10.1 启动

```text
main
  -> start SignalingActor
  -> join
  -> plainSubscribe(recvIp, recvPort)
  -> collect up to --video-consumer-count video consumers
  -> build SessionConfig(video_tracks[])
  -> start PlaySdkTransportThread
  -> start DecodeSinkWorker[track i] per selected video track
```

默认 `--video-consumer-count=1`，保持 P2 单路行为。P3 多 track 验收显式传
`--video-consumer-count=2`，只选择同一 producer peer 过滤条件下的多个 video consumer，
用于验证多 track push；仍不做多接收端 fanout。

### 10.2 接收到输出

```text
PlaySdkTransportThread
  -> UDP recv RTP/RTCP
  -> OnRtpPacket()/OnRtcpPacket()
  -> Process()
  -> decoded_access_unit_output(accessUnit)
  -> DecodedAuQueue(track i)

DecodeSinkWorker(track i)
  -> pop AU
  -> write Annex-B / null sink
  -> optional FFmpeg decode QoE
```

关键规则：

- `decoded_access_unit_output` 只入队，不 decode。
- sink queue 满时不能阻塞 SDK thread。
- QoE worker 输出 decode delay、first frame、freeze、decode error、drop counters。

## 11. 队列和背压

### 11.1 队列清单

| 队列 | 方向 | 建议容量 | 满时策略 |
|---|---|---:|---|
| `ControlMailbox` | signaling -> SDK thread | 128 event | 满则 fail fast，记录 fatal |
| `RawFrameQueue[track]` | capture -> encode | 2 到 3 frames | 丢最旧 raw frame，保留最新帧 |
| `EncodedAuQueue[track]` | encode -> SDK push | 200 到 500 ms | 丢旧 P-frame，必要时清队列并请求 IDR |
| `CopyAuQueue[track]` | copy source -> SDK push | 200 到 500 ms | 丢过期 AU，不计 encoder runtime |
| `DecodedAuQueue[track]` | SDK play -> sink | 200 到 500 ms | realtime 模式丢旧 AU，test 模式短超时或 fail fast，禁止无限阻塞 SDK thread |
| `MetricsQueue` | workers -> metrics | 1024 event | 聚合计数，丢 debug 事件，不丢 alert |

### 11.2 背压原则

- SDK thread 永远优先保活，不等待 capture、encode、sink。
- capture 背压丢 raw frame，不能把旧帧送去编码。
- encode 背压优先降 fps，其次丢旧 P-frame，最后强制 IDR 恢复。
- play sink 背压只影响本地输出，不影响 RTCP feedback。
- 所有丢弃都必须计数，不允许静默丢。

### 11.3 关键阈值

| 指标 | 建议门禁 |
|---|---:|
| SDK process max gap | <= 50 ms，弱网 case 可放宽到 100 ms |
| RTCP feedback input gap | baseline 下 <= 2 s |
| encoded queue max age | <= 500 ms |
| raw queue max depth | <= queue capacity，overflow 必须有 drop counter |
| keyframe request apply delay | <= 1 s |
| play decoded callback max cost | <= 2 ms |
| sink queue max age | <= 500 ms，超限计入 sink drop |

## 12. 线程安全审查清单

这一节用于实现前 review 和实现后静态检查。只要某条不满足，就不能进入多 track 或
V4L2 签收。

### 12.1 Owner 检查

| 对象 | 唯一 owner | 允许跨线程访问方式 |
|---|---|---|
| `VideoPushClient` | `PushSdkTransportThread` | 不允许跨线程访问，只能投递 control/data 到 SDK thread |
| `VideoPlayClient` | `PlaySdkTransportThread` | 不允许跨线程访问，只能投递 control/data 到 SDK thread |
| UDP socket | 对应 SDK/transport thread | 其他线程不持有 fd，不调用 send/recv |
| `WsClient` | `SignalingActor` | 其他线程投递 `SignalingCommand`，不持有 `WsClient*` |
| x264 encoder context | 对应 `EncodeWorker` | SDK thread 只发布 `EncoderControlSnapshot` |
| V4L2 fd | 对应 `CaptureWorker` | 其他线程只发 stop，不直接 read/poll |
| FFmpeg decode/sink | 对应 worker | SDK callback 只入队，不直接 decode/write |

### 12.2 锁检查

允许：

- `BoundedQueue` 内部 push/pop 用短锁。
- `ControlMailbox` 内部 event deque 用短锁。
- `LatestValue` snapshot swap 用短锁或 double-buffer。
- `WsClient` 内部已有锁保留在 `WsClient` 封装内。
- metrics registry 低频更新用短锁。

禁止：

- 持锁调用 SDK。
- 持锁调用 `WsClient`。
- 持锁调用 FFmpeg、x264、V4L2 read/poll、文件写入。
- 持锁等待另一个线程完成回调。
- 多把业务锁嵌套保护 runtime state。
- 用一个大 mutex 保护 push/play runtime。

### 12.3 生命周期检查

- queue close 必须唤醒阻塞 consumer。
- eventfd/timerfd close 前，owner thread 必须已经停止或不会再注册 epoll。
- SDK `Stop()` 只能在 SDK owner thread 调用。
- `WsClient::close()` 只能通过 SignalingActor 边界触发。
- worker join 时不能持有 queue/mailbox mutex。
- fatal error 只能 first-wins，后续错误作为 suppressed reason 记录。
- shutdown deadline 到期后必须继续退出，不能无限等待设备或 encoder。
- V4L2 capture 必须使用 FFmpeg interrupt callback 设置 open/read deadline；`Stop()` 必须触发
  interrupt token，不能无限等待 `avformat_open_input()`、`avformat_find_stream_info()` 或
  `av_read_frame()`。

### 12.4 数据安全检查

- 跨线程媒体 item 必须自有内存，不能保存 SDK view 指针。
- 不能跨线程共享 `AVFrame*`、`AVPacket*` 或 x264 packet 指针。
- 入队后生产者不能再修改 buffer。
- `trackId`、`captureTimeUs`、`keyframe` 在入队前固定。
- copy source 不能产生 fake encoder metrics。
- drop 行为必须更新 counters 和 report artifact。

## 13. 多 track 配置

### 13.1 CLI 形态

建议新增重复 `--track` 参数，而不是无限增加单个全局参数：

```bash
webrtc-qos-plain-push-client \
  --room room1 \
  --peer camera-pusher \
  --track id=cam0,ssrc=11111111,source=v4l2,device=/dev/video0,width=1280,height=720,fps=30,encoder=x264,weight=100 \
  --track id=cam1,ssrc=22222222,source=v4l2,device=/dev/video1,width=1280,height=720,fps=30,encoder=x264,weight=100
```

单 track 的旧参数可以保留为兼容入口，但内部统一转成 `TrackConfig[]`。

### 13.2 SessionConfig

每个 video track 生成独立 `VideoTrackConfig`：

- `track_id` 唯一。
- `sender_ssrc` 唯一。
- `payload_type` 可相同。
- `transportCcExtId` 可相同。
- `weight` 来自 CLI 或默认相等。
- `base_track` 只允许一个，默认第一个 track。

rate allocation 仍交给 SDK，客户端只使用：

- `GetTrackEncoderAdaptation(track_id, nowUs)`
- `GetTrackQosSnapshot(track_id, nowUs)`

客户端不实现自己的 track allocator。

## 14. 生命周期

### 14.1 启动顺序

1. 解析 CLI，生成 `RuntimeConfig` 和 `TrackConfig[]`。
2. 初始化或启动 `SignalingActor`，建立 WebSocket 并完成 `join`。
3. push 调 `plainPublish(videoSsrcs[])`，play 调 `plainSubscribe()`。
4. 构造 SDK `SessionConfig`。
5. 初始化 queue、mailbox、eventfd、timerfd 和 metrics counters。
6. 启动 SDK/transport thread，完成 SDK `Start()`。
7. 启动 capture / encode / sink workers。
8. metrics thread 开始输出 heartbeat。

### 14.2 停止顺序

1. main 设置 `running=false`。
2. SignalingActor 停止接收 notification。
3. capture workers 停止采集并关闭设备。
4. encode workers flush 或丢弃剩余 raw frame。
5. SDK thread drain 有限时间的 encoded queue，然后 `Stop()`。
6. play sink workers drain 或按 realtime 策略丢弃。
7. metrics thread 写 final summary。

停止必须有 deadline，不能因为某个设备或 encoder 卡住导致进程无法退出。

## 15. 日志、指标和告警

### 15.1 日志事件

必须文件化输出，不使用 `std::cout` / `printf` 作为正式观测面。

| 事件 | 字段 |
|---|---|
| `thread_started` / `thread_stopped` | role、trackId、tid、reason、durationMs |
| `capture_frame` | trackId、captureTimeUs、width、height、format |
| `capture_stall` | trackId、lastFrameAgeMs、device |
| `raw_queue_overflow` | trackId、depth、droppedFrames |
| `encoder_adaptation_applied` | trackId、targetBps、maxFps、requestKeyframe、epoch |
| `encoder_over_budget` | trackId、encodeMs、frameBudgetMs |
| `encoded_queue_overflow` | trackId、ageMs、droppedAu、forceKeyframe |
| `sdk_process_gap` | role、gapMs、rtpOut、rtcpIn |
| `feedback_gap` | role、gapMs |
| `sink_queue_overflow` | trackId、ageMs、droppedAu |
| `qoe_decode_error` | trackId、error、accessUnitsIn |

### 15.2 每秒 metrics

Push 每 track：

- `captureFps`
- `captureStallMs`
- `rawQueueDepth`
- `rawDroppedFrames`
- `encodeFps`
- `encodeAvgMs`
- `encodeP95Ms`
- `currentBitrateBps`
- `currentFps`
- `encodedQueueDepth`
- `encodedQueueMaxAgeMs`
- `encodedDroppedAu`
- `keyframes`
- `forcedKeyframes`
- `maxForcedKeyframeDelayUs`

Push SDK：

- `sdkProcessGapMaxMs`
- `pushedAu`
- `rtpPacketsOut`
- `rtcpFeedbackPacketsIn`
- `rtcpFeedbackFailures`
- `targetBps`
- `pacingBps`
- `rttMs`
- `loss`
- `droppedFrames`

Play 每 track：

- `rtpPacketsIn`
- `rtcpPacketsIn`
- `decodedAuOut`
- `sinkQueueDepth`
- `sinkQueueMaxAgeMs`
- `sinkDroppedAu`
- `decodeErrors`
- `firstFrameDelayUs`
- `freezeCount`
- `outputFps`

### 15.3 Alert

| alert | 触发条件 |
|---|---|
| `sdk_thread_blocked` | SDK process gap 连续超过阈值 |
| `capture_stall` | 指定 track 长时间无新帧 |
| `encoder_over_budget` | encode p95 超过 frame budget |
| `raw_queue_overflow` | raw queue 丢帧 |
| `encoded_queue_overflow` | encoded queue 丢 AU |
| `feedback_gap` | RTCP feedback 长时间未到 |
| `keyframe_delay` | request keyframe 后 1s 内无 IDR |
| `sink_backpressure` | play sink queue 超时或丢帧 |

## 16. 验收方案

验收目标不是“跑起来”，而是证明新线程模型没有把问题藏到锁、队列或后台线程里。
所有验收必须生成 JSON + Markdown report，并且聚合脚本非 0 退出时不能更新成功报告。

验收总原则：

- 没有自动化入口，不算完成。
- 没有 report artifact，不算完成。
- 没有静态边界检查，不算完成。
- 没有 thread/queue/track 级指标，不算完成。
- 单 track P2 行为回退，直接判 FAIL。
- 环境能力不足可以 SKIP，但必须有证据；不能用 synthetic 替代 V4L2 后记 PASS。
- 多 track、慢编码、慢 sink 是本期核心验收，不允许只用单路跑通替代。

### 16.0 验收标准一页版

线程模型升级的验收分成三层，避免把“代码能合入”“自动化验收通过”和
“完整生产能力签收”混在一起。

| 验收层级 | 自动化入口 | 必须满足 | 允许 SKIP/PARTIAL | 不允许签收的情况 |
|---|---|---|---|---|
| 合入验收 | `python3 scripts/verify_webrtc_qos_plain_thread_model_boundaries.py`、plain client 单测、`scripts/run_qos_tests.sh p3-thread-model-report:two-track-synthetic` | 静态边界 PASS；单测 PASS；P2 单路回归无退化；两路 synthetic smoke PASS；线程/队列/track 指标进入 report。 | netem、V4L2、browser H264 这类环境依赖项可以 SKIP，但必须有环境证据。 | SDK 跨线程调用、无界队列、callback 做重活、正式路径 `std::cout` / `printf`、P2 单路回退。 |
| P3 自动化验收 | `scripts/run_qos_tests.sh p3-thread-model-report` | 合入验收全部满足；两路 synthetic PASS；两路 MP4 decode-loop PASS；慢编码和慢 sink 注入 PASS；弱网 two-track PASS 或合规 SKIP/PARTIAL；V4L2 capability PASS 或合规 SKIP/PARTIAL。 | 只有缺 netem 权限、无 `/dev/video*`、browser codec 不支持这类真实环境能力不足才允许 SKIP/PARTIAL。 | 任一必跑 case 缺 report；任一 gate FAIL 但 overall 仍 PASS；用 synthetic 替代 V4L2 后记 PASS；copy source 的 encoder runtime 被记 PASS。 |
| 生产签收 | `scripts/run_qos_tests.sh p3-thread-model-acceptance` | P3 自动化验收全部满足；V4L2 `capture -> raw queue -> encode -> encoded queue -> SDK` split 的静态边界和单测 PASS；真实双 camera 硬件 PASS；弱网 two-track 在启用 netem 后 PASS。 | browser receive 可按 codec 环境 SKIP；V4L2 和弱网生产签收不允许 SKIP。 | 无真实 camera 设备还签生产；V4L2 fused 模型冒充 split；弱网未启用 netem；生产报告缺失 artifact、logs、gates 或 skip reason。 |

硬性 PASS 标准：

- 所有 SDK facade 和 UDP media I/O 只在对应 SDK/transport owner thread 执行。
- capture、encode、sink、signaling 之间只通过 mailbox、bounded queue、snapshot 或 metrics 通信。
- 所有跨线程媒体 item 都拥有自己的内存，不能保存 SDK view、FFmpeg packet 或 x264 packet 临时指针。
- 所有队列都有容量、close/wakeup、drop policy、drop counter 和 final summary。
- 每个线程都有 started、stopped、stopReason、heartbeatGapMaxMs、loopGapMaxMs。
- 每个 enabled track 都有 queue depth、drop、AU/keyframe、target bitrate、QoE 或明确的 N/A/SKIP reason。
- 普通 case `sdkProcessGapMaxMs <= 50ms`，弱网或 CPU 压力 case `sdkProcessGapMaxMs <= 100ms`。
- baseline/weak-network 下 `feedbackGapMaxMs <= 2000ms`。当前自动化从
  `push_metrics.rtcpFeedbackPacketsIn` 的增长样本推导该值；如果没有 RTCP 样本增长且不是
  明确的环境 SKIP，case 必须 FAIL。
- 弱网 case 要证明 target bitrate 下探和恢复；不能只证明链路没断。
- V4L2 case 有设备必须跑真实设备；无设备只能记录 SKIP/PARTIAL，不能 fallback 后记 PASS。

硬性 FAIL 条件：

- 任一非 SDK owner 文件直接调用 `VideoPushClient` / `VideoPlayClient`。
- 任一媒体 worker 直接持有或调用 `WsClient`。
- 任一 worker 持锁调用 SDK、WebSocket、FFmpeg、x264、V4L2 或文件写入。
- 任一后台线程停止依赖无限等待，或 queue close 不能唤醒阻塞 consumer。
- 任一必跑 case 没有 JSON/Markdown artifact。
- 任一核心指标超阈值但报告仍标记 PASS。

### 16.0.1 Definition of Done

P3 线程模型升级只有同时满足下面 checklist，才允许从“开发完成”进入 review/合入：

| 类别 | 验收标准 | 证据 |
|---|---|---|
| 线程边界 | push/play SDK facade、UDP media I/O 都只在对应 SDK/transport owner thread；source、encoder、sink 不能直接调用 SDK。 | `webrtc-qos-plain-thread-model-boundary-report` overall PASS。 |
| 通信方式 | 跨线程只用 `BoundedQueue`、`ControlMailbox`、`LatestValue` 或 metrics snapshot；没有无界队列；队列 close 必须唤醒 consumer。 | 静态边界 report + `WebRtcQosThreadModelPrimitivesTest.*` 单测。 |
| Push 多 track | 两路 synthetic 和两路 MP4 decode-loop 都 PASS；每路 track 有 AU、keyframe、target bitrate、queue/drop counters。 | `webrtc-qos-plain-p3-thread-model-smoke-report`、`decode-loop-report`。 |
| Play per-track sink/QoE | play `--video-consumer-count=2` 时每路 selected video track 都有独立 sink worker、独立 QoE final、`decodedFrames > 0`、`decodeErrors = 0`。 | report gate `perTrackPlaySinkQoe=PASS`，日志包含 `decoded_sink_track_stopped` 和 `play_track_qoe_final`。 |
| 隔离性 | 慢编码、慢 sink 注入时 SDK loop gap、RTCP feedback loop 仍在阈值内，不能因为 worker 变慢拖死 SDK owner。 | `slow-encoder-report`、`slow-sink-report` overall PASS。 |
| 弱网 QoS | 有 netem 时 two-track 弱网必须证明 target bitrate 下探和恢复；无权限/未启用 netem 只能 `PARTIAL/SKIP`。 | `weak-network-report` gate `weakNetworkTwoTrack=PASS` 或合规 skip reason。 |
| V4L2 | 有 `/dev/video*` 时跑真实 V4L2；无设备只能 `PARTIAL/SKIP`，不能 fallback 到 synthetic 伪造 PASS。 | `v4l2-report`，skip reason 或 camera PASS 证据。 |
| 日志观测 | 正式路径不使用 `std::cout` / `printf`；每个线程/track 有 start、stop、reason、loop gap、queue/drop/final summary。 | 静态边界 report + 动态 smoke logs。 |
| 回归 | P2 单路推拉流、边界和单测不能回退。 | `p2-acceptance` 或对应 P2 report。 |

P3 自动化验收允许 V4L2 在无设备环境 `PARTIAL/SKIP`，但这只代表 CI/开发机能力不足，
不代表真实 camera 链路已经签收。当前代码已经把 V4L2 拆成
`capture -> raw queue -> encode -> encoded queue -> SDK`，并由静态边界和单测覆盖。
生产签收额外要求该 split 在真实双 camera 硬件环境 PASS。当前无设备环境
只能签“合入验收”或“P3 自动化验收”，不能签“生产能力验收”。

### 16.1 验收分层

| 层级 | 是否必跑 | 目的 | 失败处理 |
|---|---|---|---|
| 静态边界 | 必跑 | 证明 SDK、WsClient、encoder、V4L2、sink owner 没有被跨线程破坏 | FAIL，阻断 |
| 单路回归 | 必跑 | 证明 P2 单 track 行为不回退 | FAIL，阻断 |
| 多 track synthetic | 必跑 | 不依赖设备验证多 track 调度、队列和 SDK thread health | FAIL，阻断 |
| 多 track MP4 decode-loop | 必跑 | 验证 decode/encode CPU 压力下 SDK thread 不被拖死 | FAIL，阻断 |
| 慢编码/慢 sink 注入 | 必跑 | 验证隔离和背压 | FAIL，阻断 |
| 弱网 two-track | 有 netem 时必跑 | 验证 QoS target 下探/恢复和两个 track 不饿死 | 无 netem 才允许 SKIP |
| V4L2 capability | 有设备时必跑 | 验证真实摄像头入口、per-track device 参数、无设备 SKIP 口径 | 无设备才允许 SKIP/PARTIAL |
| V4L2 split + 双 camera | 生产签收必跑 | 验证真实 `capture -> raw queue -> encode -> encoded queue -> SDK` split 和双 camera 并发 | 不允许 SKIP |
| browser receive | 可选 | 验证浏览器兼容性 | 环境能力不足允许 SKIP |

### 16.2 静态边界

新增或扩展边界检查：

- SDK facade 调用只出现在 SDK/transport loop 文件。
- encoder/capture/sink worker 文件不能调用 `VideoPushClient` / `VideoPlayClient`。
- `decoded_access_unit_output` callback 不能直接 decode 或写大文件。
- 跨线程媒体对象必须转移所有权，不能保存 SDK view / FFmpeg / x264 临时指针。
- 静态检查要覆盖 owner 规则：SDK、UDP、WsClient、encoder、V4L2、FFmpeg sink 不能跨 owner 访问。
- 静态检查要覆盖锁规则：禁止持锁调用 SDK、WsClient、FFmpeg、x264、V4L2 和文件写入。
- SDK thread event loop 必须有 eventfd/timerfd 或等价唤醒机制，不能靠 sleep 轮询所有队列。
- 线程模型代码不能引入旧自研 RTP/RTCP/QoS。
- 正式日志不能使用 `std::cout` / `printf`。

静态边界的 FAIL 条件：

- 任一非 SDK owner 文件直接调用 `VideoPushClient` / `VideoPlayClient` 方法。
- 任一非 SignalingActor 文件直接持有或调用 `WsClient`。
- 任一 worker 文件持有 UDP socket fd 并执行 media send/recv。
- 任一正式路径出现 `std::cout` / `printf` 日志。
- 任一跨线程队列未设置容量或没有 close/wakeup 语义。
- 任一锁保护 SDK、WsClient、FFmpeg、x264、V4L2 或文件写入调用。

### 16.3 动态 smoke

| case | 环境 | 期望 |
|---|---|---|
| `single_track_regression` | 普通 CPU | 保持 P2 单 track baseline PASS |
| `copy_single_track_regression` | 普通 CPU | copy path 仍可推拉流，`encoderRuntime=SKIP/N/A` |
| `two_track_synthetic` | 普通 CPU | 2 路 x264 推流，play `--video-consumer-count=2`，两个 track 都有 AU/keyframe、独立 sink worker 和 per-track QoE，SDK thread gap 不超阈值 |
| `two_track_decode_loop` | 普通 CPU | 2 路 MP4 decode-loop，encoder runtime PASS，decode/encode 不阻塞 SDK thread |
| `slow_encoder_injection` | 普通 CPU | encoder worker 变慢时 SDK process gap 不超阈值 |
| `slow_play_sink_injection` | 普通 CPU | sink 变慢时 RTCP feedback 不超阈值 |
| `weak_network_two_track` | 需要 netem | target 下探/恢复，两个 track 不饿死 |
| `v4l2_single_camera` | 需要 `/dev/video0` | 有设备 PASS，无设备 SKIP |
| `v4l2_two_camera` | 需要两个设备 | 两路 camera PASS，无设备 SKIP |

### 16.4 量化门禁

| 指标 | PASS 条件 | FAIL 条件 | SKIP 条件 |
|---|---|---|---|
| `sdkProcessGapMaxMs` | 普通 case <= 50ms；弱网/CPU 压力 case <= 100ms | 超阈值且不是显式注入预期 | 不允许 SKIP |
| `feedbackGapMaxMs` | baseline/weak-network <= 2000ms | 超阈值且 RTCP counters 停止增长 | 无 netem 时弱网 case SKIP |
| `encodedQueueMaxAgeMs` | <= 500ms | 超阈值且没有 drop/IDR 恢复记录 | copy EOF drain 可 N/A |
| `rawQueueOverflow` | synthetic/decode-loop baseline 为 0；慢编码注入允许 >0 但必须有 counters | overflow 静默或导致 SDK gap 超阈值 | copy source N/A |
| `encodedQueueOverflow` | baseline 为 0；慢编码/弱网允许 >0 但必须有 counters 和恢复 | overflow 静默或恢复失败 | 不允许 SKIP |
| `sinkQueueOverflow` | baseline 为 0；slow sink 注入允许 >0 且 RTCP 不受影响 | 导致 feedback gap 超阈值 | output-null 可 N/A |
| `keyframeRequestDelayMs` | <= 1000ms | request 后 1s 内无 IDR | copy source N/A |
| `trackStarvation` | 每个 enabled track 在 2s 窗口内有 AU 或明确 drop reason | 任一 track 无 AU 且无 drop/skip reason | V4L2 缺设备 SKIP |
| `playTrackCoverage` | selected video consumers 数量等于请求值；每个 track 有 `play_track_metrics` 和 `outputAu/enqueuedAu` 增长 | 任一路 consumer 未选中或 per-track output 为 0 | 不允许 SKIP |
| `perTrackPlaySinkQoe` | 每个 selected video track 有独立 `decoded_sink_track_stopped` 和 `play_track_qoe_final`，且 `decodedFrames > 0`、`decodeErrors = 0` | 任一路 sink/QoE 缺失、串路或 decode 持续失败 | 不允许 SKIP |
| `encoderRuntime` | synthetic/decode-loop/V4L2 x264 为 PASS | x264 AU/keyframe/adaptation 缺失 | copy source 必须 SKIP/N/A |
| `nativeDecodeQoe` | decodedFrames 增长，decodeErrors=0 或在阈值内 | 无首帧、持续 decode error | browser H264 不支持时 browser case SKIP |
| `threadSafetyBoundary` | 静态 owner/锁/数据安全检查全过 | 任一禁止项命中 | 不允许 SKIP |

### 16.5 报告必备字段

每个 report JSON 至少包含下列顶层字段。静态边界报告没有运行时线程和 track，因此
`threads[]`、`tracks[]`、`queues[]` 允许为空，但字段必须存在；动态 smoke/V4L2 runtime
报告必须按实际日志填充。

- `overall`: `PASS` / `FAIL` / `PARTIAL`
- `environment`: `hasNetem`、`hasV4L2Devices`、`browserH264Supported`、CPU 核数
- `sourceMode`: `copy` / `synthetic` / `mp4_decode_loop` / `v4l2`
- `trackCount`
- `cases[]`
- `gates{}`
- `threads[]`: role、tid、started、stopped、stopReason、heartbeatGapMaxMs、loopGapMaxMs。
  当前 runtime 日志还未输出 OS tid，报告必须保留 `tid` 字段并标明 `tidSource`；
  如果后续把 tid 作为生产硬门槛，需要在所有 worker start/stop 日志中补 `tid`。
- `tracks[]`: trackId、source、ssrc、capture、encode、queue、sdk、qoe counters
- `queues[]`: name、capacity、maxDepth、maxAgeMs、pushCount、popCount、dropCount、closeReason
- `sdk`: processGapMaxMs、feedbackGapMaxMs、rtp/rtcp counters、target bitrate min/avg/max；
  `feedbackGapMaxMs` 来源必须写入 `feedbackGapSource`。
- `threadSafety`: ownerViolations、lockViolations、lifetimeViolations、dataViolations
- `artifacts`: push/play logs、SDK metrics/alerts、smoke stdout/stderr、netem log
- `skipReasons[]`: 每个 SKIP 必须带环境证据，例如缺 `/dev/video0` 或无 netem 权限

报告判定规则：

- 非环境依赖 case 出现 SKIP，overall 必须 FAIL。
- 环境依赖 case SKIP 时，overall 最多 PARTIAL，不能 PASS。
- 任何 gate FAIL，overall 必须 FAIL。
- copy source 的 encoder gate 只能 SKIP/N/A，不能 PASS。
- V4L2 无设备只能 SKIP，不能 fallback 到 synthetic 后记 PASS。
- `scripts/run_qos_tests.sh p3-thread-model-acceptance` 是生产签收入口；它对 weak-network 和
  V4L2 使用 strict 口径，缺 netem 或缺 `/dev/video0`/`/dev/video1` 必须非 0 退出。

### 16.6 报告 gate

建议报告新增 gates：

| gate | PASS 条件 |
|---|---|
| `threadModelBoundary` | 静态边界检查通过 |
| `threadSafetyBoundary` | owner、锁、生命周期和跨线程数据安全检查通过 |
| `sdkThreadHealth` | SDK process gap、feedback gap 在阈值内 |
| `captureIsolation` | capture stall 不阻塞 SDK thread |
| `encoderIsolation` | encode over-budget 不阻塞 SDK thread |
| `queueBackpressure` | 队列 overflow 有计数、有策略、无静默丢 |
| `communicationBoundary` | SDK/encoder/sink/signaling 只通过指定 actor/mailbox/queue/snapshot 通信 |
| `multiTrackCoverage` | 至少 2 路 synthetic 或 decode-loop PASS |
| `playTrackCoverage` | play 选择 2 个 video consumer，两个 track 都有 RTP、decoded AU 入队和输出计数 |
| `perTrackPlaySinkQoe` | 每个 selected video track 都有独立 sink worker、独立 QoE final 和 decoded frame 证据 |
| `playSinkIsolation` | slow sink 不阻塞 RTCP feedback |
| `cameraRuntime` | 有设备 PASS，无设备 SKIP/PARTIAL |

### 16.7 建议命令和退出码

当前已实现的 P3 自动化验收入口：

```bash
scripts/run_qos_tests.sh p3-thread-model-report
```

当前已实现的 `p3-thread-model-report` 是 P3 自动化验收入口：它聚合静态线程边界检查和
两路 synthetic、两路 MP4 decode-loop、慢编码注入、慢 sink 注入和弱网 two-track
的动态 smoke。弱网 case 默认安全运行：未显式允许 netem 时记录 `PARTIAL/SKIP`，
设置 `WEBRTC_QOS_P3_ENABLE_NETEM=1` 后才会修改 `lo` 上的 netem，并要求 target bitrate
有下探和恢复证据。

如果只做合入前快速复核，至少执行下面三类证据：

```bash
python3 scripts/verify_webrtc_qos_plain_thread_model_boundaries.py
cmake --build build-webrtc-qos-plain --target mediasoup_webrtc_qos_plain_unit_tests -j2
./build-webrtc-qos-plain/mediasoup_webrtc_qos_plain_unit_tests \
  --gtest_filter='WebRtcQosThreadModelPrimitivesTest.*:WebRtcQosDecodeSinkTest.*'
scripts/run_qos_tests.sh p3-thread-model-report:two-track-synthetic
```

生产签收入口：

```bash
scripts/run_qos_tests.sh p3-thread-model-acceptance
```

该入口是显式生产签收入口，不包含在默认测试和 `all` 中。它复用 P3 自动化报告，
但对弱网和 V4L2 使用 strict 口径：弱网必须实际启用 netem 并 PASS，V4L2 必须在
真实双 camera 硬件上 PASS；任何 SKIP/PARTIAL 都返回非 0。

退出码语义：

- `0`: 自动化报告入口中所有非环境依赖 gate PASS，环境依赖项要么 PASS 要么合规 SKIP/PARTIAL；生产签收入口中所有 gate 必须 PASS。
- `1`: 任一必跑 case 或 gate FAIL。
- `2`: 参数错误、构建产物缺失或脚本环境错误。
- `3`: 环境依赖项缺失但未按 SKIP 规则处理；生产签收入口中环境依赖项缺失按 FAIL 处理。

### 16.8 P3 自动化验收签收包

`scripts/run_qos_tests.sh p3-thread-model-report` 至少要产出下面这些 report：

| report | 必须状态 | 说明 |
|---|---|---|
| `webrtc-qos-plain-thread-model-boundary-report` | PASS | 静态 owner、锁、数据安全、旧 QoS 禁用检查。 |
| `webrtc-qos-plain-p3-thread-model-smoke-report` | PASS | 2 路 synthetic push + 2 consumer play，验证 per-track push/play、per-track sink/QoE 和 SDK thread health。 |
| `webrtc-qos-plain-p3-thread-model-decode-loop-report` | PASS | 2 路 MP4 decode-loop，证明 decode/encode CPU 压力不阻塞 SDK thread。 |
| `webrtc-qos-plain-p3-thread-model-slow-encoder-report` | PASS | 慢编码注入，证明 source/encode worker 变慢不会拖死 push SDK owner。 |
| `webrtc-qos-plain-p3-thread-model-slow-sink-report` | PASS | 慢 sink 注入，证明 play sink/QoE 变慢不会拖死 RTCP feedback。 |
| `webrtc-qos-plain-p3-thread-model-weak-network-report` | PASS 或合规 SKIP/PARTIAL | 启用 netem 时必须 PASS；未启用或无权限只能 SKIP/PARTIAL 并写明证据。 |
| `webrtc-qos-plain-p3-thread-model-v4l2-report` | PASS 或合规 SKIP/PARTIAL | 有 `/dev/video*` 时必须跑真实设备；无设备只能 SKIP/PARTIAL。 |

当前 `scripts/run_qos_tests.sh p3-thread-model-report` 已聚合静态边界和
`two_track_synthetic`、`two_track_decode_loop`、`slow_encoder_injection`、
`slow_play_sink_injection`、`weak_network_two_track`、V4L2 capability smoke，并会刷新
`webrtc-qos-plain-thread-model-boundary-report` 与
`webrtc-qos-plain-p3-thread-model-smoke-report`、
`webrtc-qos-plain-p3-thread-model-decode-loop-report`、
`webrtc-qos-plain-p3-thread-model-slow-encoder-report`、
`webrtc-qos-plain-p3-thread-model-slow-sink-report`、
`webrtc-qos-plain-p3-thread-model-weak-network-report`、
`webrtc-qos-plain-p3-thread-model-v4l2-report`。`scripts/run_qos_tests.sh p3-thread-model-acceptance`
会运行同一组报告，但强制 weak-network 和 V4L2 不能 SKIP/PARTIAL。
生产签收仍需在真实双 camera 硬件环境复核 V4L2 PASS。
验收不能只看其中一个报告。聚合入口必须验证上述 report 都存在，并复核
`overall`、`gates`、`skipReasons` 和 artifact 路径。

当前可执行的正式刷新报告：

```bash
scripts/run_qos_tests.sh p3-thread-model-report
```

当前可执行的生产签收报告：

```bash
scripts/run_qos_tests.sh p3-thread-model-acceptance
```

只刷新动态 two-track synthetic smoke：

```bash
scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh --strict
```

只刷新动态 two-track MP4 decode-loop smoke：

```bash
scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh \
  --source mp4-decode-loop \
  --input tests/fixtures/media/test_sweep.mp4 \
  --report-name webrtc-qos-plain-p3-thread-model-decode-loop-report \
  --strict
```

只刷新慢编码注入 smoke：

```bash
scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh \
  --injection slow-encoder \
  --report-name webrtc-qos-plain-p3-thread-model-slow-encoder-report \
  --strict
```

只刷新慢 sink 注入 smoke：

```bash
scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh \
  --injection slow-sink \
  --report-name webrtc-qos-plain-p3-thread-model-slow-sink-report \
  --strict
```

只刷新弱网 two-track smoke：

```bash
WEBRTC_QOS_P3_ENABLE_NETEM=1 \
scripts/run_qos_tests.sh p3-thread-model-report:weak-network-two-track
```

弱网验收标准：

- `networkMode=weak`，`netemApplied=true`，弱网窗口为 `5% loss + 600kbps rate limit`，随后清网恢复。
- push/play 都必须是 2 个 track，两个 track 都有 AU/keyframe/output，不能单路饿死。
- `targetBps` 样本数至少 6 个，`min < max * 0.9`，清网后的 `postClearTargetBps.max > targetBps.min`。
- `pushRtcpFeedbackPacketsIn > 0` 且 `playRtcpPacketsOut > 0`，RTCP/packet failure 必须为 0。
- SDK loop gap、sink loop gap 仍按报告阈值通过；弱网不能把 SDK owner thread 阻塞住。
- 没有 `tc`、没有 root/CAP_NET_ADMIN 或未显式允许 netem 时，只能记录 `SKIP/PARTIAL`，不能记 PASS。

真实双 camera 硬件复核是生产签收的必跑 case，统一由
`scripts/run_qos_tests.sh p3-thread-model-acceptance` 聚合复核。

只刷新 V4L2 capability report：

```bash
scripts/run_qos_tests.sh p3-thread-model-report:v4l2
```

V4L2 验收标准：

- `v4l2_single_camera` 有 `/dev/video0` 时运行真实 V4L2，不允许 fallback 到 synthetic；无设备时必须 `SKIP/PARTIAL` 并写明设备缺失。
- `v4l2_two_camera` 需要两个真实设备；push runtime 通过 per-track
  `--track source=v4l2,device=...` 选择不同 camera，无设备时必须作为明确 SKIP/剩余项记录。
- 有设备 PASS 时，encoder mode 必须为 `v4l2`，x264 AU/keyframe、push/play track、RTCP feedback、SDK/sink gap 必须通过。
- 无设备或 camera case 未跑通时，report overall 最多 `PARTIAL` 或 `FAIL`，不能记为 PASS。

### 16.9 里程碑签收标准

每个里程碑签收时，都要同时满足“功能结果、线程边界、观测结果、回归结果”。只满足功能
结果但缺少边界或观测，不允许签收。

| 里程碑 | 必须证明 | 最低命令/报告 | PASS 标准 |
|---|---|---|---|
| M1 基础线程和队列 | bounded queue、close/wakeup、heartbeat、stop reason 可用；单路行为不回退。 | `thread-model-boundary-report`、plain client unit tests、P2 单路回归。 | 队列容量固定，overflow 有 counter；worker stop 不死锁；P2 单 track PASS；正式路径无 `std::cout` / `printf` 日志。 |
| M2 Push SDK/transport thread | `VideoPushClient` 和 UDP 只在 SDK owner thread 访问；source/encoder 只能通过队列投递 AU。 | `thread-model-single-track-report`、`slow_encoder_injection`。 | 静态检查无 SDK 跨线程调用；slow encoder 下 `sdkProcessGapMaxMs <= 100ms`；copy encoder gate 只能 SKIP/N/A。 |
| M3 Push 多 track | 两路 track 独立 source/encode/queue/adaptation，任一路不能饿死另一路。 | `thread-model-two-track-synthetic-report`、`thread-model-two-track-decode-loop-report`。 | 两个 track 都有 AU、keyframe、target bitrate、queue counters；2s 窗口内无无理由 starvation；decode/encode 压力不阻塞 SDK thread。 |
| M4 Play SDK/sink 解耦 | `decoded_access_unit_output` 只入队，文件写入、FFmpeg decode、QoE 在 per-track sink worker 执行。 | `slow_play_sink_injection`、two-track P3 smoke per-track QoE gate。 | 每路 selected video track 都有独立 sink/QoE final；callback p95 <= 2ms；slow sink 不导致 `feedbackGapMaxMs > 2000ms`；sink queue overflow 有 counter 和 drop reason。 |
| M5 V4L2 和聚合验收 | 真实 camera 环境可跑，无设备环境按证据 SKIP；V4L2 open/read 有 interrupt deadline；所有 report 可被一条命令复核。 | `thread-model-v4l2-report`、`p3-thread-model-acceptance`。 | 有 `/dev/video*` 时 camera case PASS；无设备时 overall 最多 PARTIAL；聚合脚本复核所有 report、gates、skipReasons、interrupt deadline 和 artifact。 |

### 16.10 Go / No-Go 判定

允许进入下一里程碑的条件：

- 当前里程碑所有非环境依赖 gate 为 PASS。
- 所有新增线程都有 owner、stop deadline、heartbeat 和 final summary。
- 所有跨线程队列都有容量、close/wakeup、drop policy 和 metrics。
- 单 track P2 acceptance 未回退。
- 新增风险项已经写入 report 或风险表，不能只停留在代码注释。

必须阻断的条件：

- SDK、UDP、WsClient、FFmpeg、x264 或 V4L2 出现未定义 owner 的跨线程访问。
- 任一 worker 持锁调用 SDK、WsClient、FFmpeg、x264、V4L2 或文件写入。
- 任一后台线程 stop/join 依赖无限等待。
- 任一必跑 case 没有 artifact 或 artifact 缺少 gate 结果。
- 任一指标超阈值但报告仍标记 PASS。

## 17. 实施拆分

### M1: 基础线程和队列

- 增加 `BoundedQueue`、thread heartbeat、stop token、metrics counters。
- 给 `PlainUdpTransport` 暴露可 poll 的 fd 或新增 pollable transport。
- 增加 `ControlMailbox`、`LatestValue<EncoderControlSnapshot>`、eventfd/timerfd 唤醒。
- 先不改多 track，保证单 track 行为不回退。

退出条件：

- 单 track P2 acceptance 仍 PASS。
- SDK thread gap metrics 可见。

### M2: Push SDK/transport thread

- `VideoPushClient` 和 UDP socket 迁移到 `SdkTransportThread`。
- source/encoder 先作为单 track worker，通过 encoded queue 喂 SDK thread。
- 所有 SDK 调用收敛到 SDK thread。
- copy 输入用 `AuSourceWorker` 喂 SDK thread，encoder runtime 明确 `SKIP/N/A`。

退出条件：

- 单 track synthetic、MP4 decode-loop、copy baseline PASS。
- slow encoder injection 下 SDK process gap 不超阈值。

### M3: Push 多 track capture/encode

- CLI 支持 `--track`。
- `PushSignalingSession` 支持 `videoSsrcs[]` 和多 `videoTracks[]`。
- `SessionConfig.video_tracks` 构造多 track。
- 每 track 独立 capture/encode/queue/adaptation。

退出条件：

- 2 路 synthetic PASS。
- 2 路 MP4 decode-loop PASS。
- 弱网下两个 track 都有 target/adaptation/QoE 指标。

### M4: Play SDK/sink 解耦

- `VideoPlayClient` 和 UDP socket 迁移到 `PlaySdkTransportThread`。
- `decoded_access_unit_output` 只入队。
- 每个 selected video track 启动独立 sink/decode worker，并输出 per-track `play_track_metrics` 与 `play_track_qoe_final`。

退出条件：

- slow sink injection 下 RTCP feedback gap 不超阈值。
- play QoE 指标仍可生成。
- `--video-consumer-count=2` 下两个 video consumer 都被选中，两个 track 的 `enqueuedAu/outputAu` 都增长。

### M5: V4L2 和验收报告

- 单 camera 和双 camera smoke。
- 无设备环境只允许 SKIP/PARTIAL。
- 报告新增 thread/queue/backpressure gates。

退出条件：

- 有设备环境 camera PASS。
- 无设备环境 skip reason 明确。
- 聚合 acceptance 一条命令可复跑。

## 18. 风险和处理

| 风险 | 处理 |
|---|---|
| SDK facade 实际线程安全边界不明确 | 客户端强制单线程拥有 SDK 对象，不依赖 SDK 内部锁 |
| 多线程引入隐藏延迟 | 所有 frame/AU 带 capture timestamp，报告 queue age 和 end-to-end delay |
| x264 CPU 过载 | per-track encode budget、fps adaptation、over-budget alert、最大 track 数限制 |
| 队列掩盖问题 | 队列容量按时间上限控制，overflow 必须进 metrics/alert |
| 多 track rate allocation 做重复 | 客户端不自研 allocator，只使用 SDK per-track adaptation |
| `WsClient` 内部已有锁，和无锁 actor 原则冲突 | 第一版把锁限制在 `WsClient` 封装内，媒体 runtime 只通过 `SignalingActor` 发命令；后续如有必要再改成纯 owner loop |
| V4L2 环境不可控 | synthetic/decode-loop 必跑，V4L2 按环境 PASS 或 SKIP |
| 退出卡死 | 所有 worker stop 带 deadline，超时记录 fatal 并继续进程退出 |

## 19. 需要 review 的核心问题

- V4L2/多摄像头是否默认采用 split 模式：每 track 一个 capture worker 加一个 encode worker。
- 单进程最大目标 track 数先定为 2 还是 4。
- realtime 模式下 encoded queue 满时是否允许清空旧 P-frame 并强制 IDR。
- play sink 慢时默认 drop 还是 block。建议 realtime drop，测试模式可 block。
- 是否在第一版就做 `--track` 统一配置，还是先用 `--tracks N --source synthetic`
  快速验证线程模型。
- SDK thread 单轮预算是否按 64 个 feedback 包 / 2 ms、每 track 1 到 2 个 AU 起步。
- test 模式下 sink queue 满时是 fail fast，还是允许短超时后 fail。
