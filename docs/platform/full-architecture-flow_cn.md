# 全链路架构流程图

> 从客户端 WebSocket 连接到 Worker 媒体转发的完整路径。

## 1. 进程 / 线程模型总览

```
┌──────────────────────────────────────────────────────────────┐
│                         main()                               │
│  解析配置 → 可选创建 RoomRegistry(Redis) → 创建 N 个 WorkerThread │
└───────────────┬───────────────────────────────┬──────────────┘
                │                               │
                │                               ├─ registry worker thread
                │                               │    串行执行 Redis command / heartbeat / refreshRoom
                │                               │
                │                               └─ Redis subscriber thread
                │                                    poll + redisGetReply → 更新 nodeCache_/roomCache_
                │
     ┌──────────┼────────────────┐
     ▼          ▼                ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│WorkerThread 0│ │WorkerThread 1│ │WorkerThread N│
│  epoll loop  │ │  epoll loop  │ │  epoll loop  │
│  ┌────────┐  │ │              │ │              │
│  │Worker 0 │  │ │   ...        │ │   ...        │
│  │Worker 1 │  │ │              │ │              │
│  └────────┘  │ │              │ │              │
│  RoomManager │ │              │ │              │
│  RoomService │ │              │ │              │
└──────┬──────┘ └──────────────┘ └──────────────┘
       │
       │ fork+exec
       ▼
┌────────────────┐
│mediasoup-worker│  ← 单线程 libuv 事件循环
│  (C++ 进程)    │
│  Router        │
│  Transport     │
│  Producer      │
│  Consumer      │
│  BWE           │
└────────────────┘
```

关键点：
- **主线程 (uWS)**: 运行 SignalingServer，处理 WebSocket / HTTP，维护 room→WorkerThread 分派表
- **Registry worker / Redis subscriber**: 只有启用 Redis 多节点时存在；前者串行执行 Redis 命令，后者订阅 `sfu:ch:nodes` / `sfu:ch:rooms`
- **WorkerThread**: 每个是独立 std::thread + epoll 事件循环，拥有若干 Worker 子进程。内部有 4 个 epoll fd:
  - eventFd: 任务队列唤醒（主线程 post 过来的任务）
  - healthTimerFd: 定期检查房间健康（dead router 检测）
  - gcTimerFd: 定期清理空闲房间
  - delayedTimerFd: 延迟任务（如 downlink QoS coalesced planning）
- **Worker**: fork+exec 的 mediasoup-worker 进程，通过 pipe (fd 3/4) 与 WorkerThread 通信。Worker 死亡时自动 respawn（限速: 10s 内最多 3 次）
- **Channel**: FlatBuffers 序列化的 IPC 协议，Request/Response/Notification/Log 四种消息

## 2. 客户端加入频道完整时序

```
Client                SignalingServer(uWS)        WorkerThread           RoomService              Worker/Router
  │                        │                         │                      │                        │
  │── WS connect ─────────▶│                         │                      │                        │
  │                        │ ws.open: 分配 SessionData│                      │                        │
  │                        │                         │                      │                        │
  │── join ───────────────▶│                         │                      │                        │
  │   {roomId,peerId,      │                         │                      │                        │
  │    rtpCapabilities}    │                         │                      │                        │
  │                        │                         │                      │                        │
  │                        │ 1. pickLeastLoadedWT()   │                      │                        │
  │                        │ 2. assignRoom(roomId,wt) │                      │                        │
  │                        │ 3. wt->post(task) ──────▶│                      │                        │
  │                        │                         │ rs->join() ─────────▶│                        │
  │                        │                         │                      │                        │
  │                        │                         │          ┌───────────┴──────────────┐         │
  │                        │                         │          │ a. registry->claimRoom() │         │
  │                        │                         │          │    (启用 Redis 时 Lua 抢占)│        │
  │                        │                         │          │ b. 如果 redirect → 返回   │         │
  │                        │                         │          │ c. roomManager->          │         │
  │                        │                         │          │    createRoom(roomId)     │         │
  │                        │                         │          └───────────┬──────────────┘         │
  │                        │                         │                      │                        │
  │                        │                         │                      │ getLeastLoadedWorker()  │
  │                        │                         │                      │ worker->createRouter()─▶│
  │                        │                         │                      │                        │
  │                        │                         │                      │   Channel: WORKER_CREATE_ROUTER
  │                        │                         │                      │◀── response ───────────│
  │                        │                         │                      │                        │
  │                        │                         │                      │ room->replacePeer(peer) │
  │                        │                         │                      │ return bootstrap payload│
  │                        │                         │◀─────────────────────│                        │
  │                        │◀── loop->defer(resp) ───│                      │                        │
  │◀── join response ──────│                         │                      │                        │
  │   {routerRtpCapabilities,│ 绑定 sd->roomId/peerId │                      │                        │
  │    existingProducers,  │ 可能追加 qosPolicy notify│                     │                        │
  │    participants,qosPolicy}│                      │                        │
```

## 3. Transport 创建与 DTLS 握手

```
Client                SignalingServer        WorkerThread/RoomService          Worker
  │                        │                         │                          │
  │── createWebRtcTransport│                         │                          │
  │   {producing:true}    ─▶│── wt->post() ─────────▶│                          │
  │                        │                         │ router->createWebRtcTransport()
  │                        │                         │   Channel: ROUTER_CREATE_WEBRTCTRANSPORT ──▶│
  │                        │                         │◀── response: {iceParams, iceCandidates,     │
  │                        │                         │              dtlsParams}                    │
  │◀── {id, iceParameters, │◀────────────────────────│                          │
  │     iceCandidates,     │                         │                          │
  │     dtlsParameters}   │                         │                          │
  │                        │                         │                          │
  │── connectWebRtcTransport                         │                          │
  │   {transportId,        │                         │                          │
  │    dtlsParameters} ───▶│── wt->post() ─────────▶│                          │
  │                        │                         │ transport->connect(dtlsParams)              │
  │                        │                         │   Channel: WEBRTCTRANSPORT_CONNECT ────────▶│
  │                        │                         │◀── response (accepted) ──────────────────── │
  │◀── {dtlsLocalRole:     │◀────────────────────────│                          │
  │    "server"} ─────────│                         │                          │
  │                        │                         │                          │
  │                        │                         │          (异步，后续到达)  │
  │                        │                         │◀── Notification:         │
  │                        │                         │    ICE_STATE_CHANGE      │
  │                        │                         │    (connected)           │
  │                        │                         │◀── Notification:         │
  │                        │                         │    DTLS_STATE_CHANGE     │
  │                        │                         │    (connected)           │
```

说明：
- 客户端需要创建两个 Transport: **sendTransport** (producing=true) 和 **recvTransport** (consuming=true)
- ICE candidate 由 Worker 在创建 Transport 时生成，通过 response 一次性返回（非 trickle ICE）
- `dtlsParameters` 由客户端完整传入；控制面当前固定返回 `{"dtlsLocalRole":"server"}`
- connect 是同步返回的，ICE/DTLS 状态变化通过异步 Notification 到达
- 如果创建的是 recvTransport，`createWebRtcTransport` 的响应还会额外带上自动补建的 `consumers[]`

## 4. Produce 与 Auto-Subscribe

```
Client A               RoomService                    Worker              Client B
  │                        │                            │                    │
  │── produce ────────────▶│                            │                    │
  │  {kind:"video",        │                            │                    │
  │   rtpParameters}       │                            │                    │
  │                        │ transport->produce()       │                    │
  │                        │  Channel: TRANSPORT_PRODUCE▶│                    │
  │                        │◀── {producerId} ───────────│                    │
  │                        │                            │                    │
  │◀── {producerId} ───────│                            │                    │
  │                        │                            │                    │
  │                        │ ── Server-driven Auto-Subscribe ──              │
  │                        │                            │                    │
  │                        │ for each otherPeer (B):    │                    │
  │                        │   if B.recvTransport exists:│                    │
  │                        │     transport->consume()   │                    │
  │                        │      Channel: TRANSPORT_CONSUME ──▶│            │
  │                        │◀──── {consumerId, rtpParams}──────│            │
  │                        │                            │                    │
  │                        │── notify B: "newConsumer" ──────────────────────▶│
  │                        │   {peerId, producerId, id, │                    │
  │                        │    kind, rtpParameters,    │                    │
  │                        │    producerPaused}         │                    │
```

关键设计：
- **服务端驱动订阅 (两个触发点)**:
  1. A produce 后，RoomService 自动为房间内所有已有 recvTransport 的 Peer 创建 Consumer
  2. B 创建 recvTransport (consuming=true) 时，自动订阅房间内所有已有的 Producer，Consumer 列表随 createTransport 响应一起返回
- **Reconnect 处理**: join 时如果 peerId 已存在，执行 replacePeer — 关闭旧 Peer 的所有 Transport/Producer/Consumer，清理 QoS 缓存和录制状态，新 Peer 从零开始建连

## 5. Worker 内部媒体转发 (BWE / Simulcast)

```
┌─────────────────── mediasoup-worker 进程 ──────────────────────┐
│                                                                │
│  ┌──────────┐    RTP     ┌──────────┐                          │
│  │ Producer │◀──────────│WebRtcTransport│◀── DTLS/SRTP ── Client A
│  │ (video)  │           │  (send)       │                      │
│  └────┬─────┘           └───────────────┘                      │
│       │                                                        │
│       │ RTP 路由 (Router::OnTransportProducerRtpPacketReceived) │
│       │                                                        │
│  ┌────▼──────────────┐                                         │
│  │SimulcastConsumer B │──▶ WebRtcTransport (recv) ──▶ Client B │
│  │  ┌──────────────┐ │                                         │
│  │  │ 层选择逻辑:   │ │                                         │
│  │  │ preferredLayer│ │                                         │
│  │  │ + BWE 可用带宽│ │                                         │
│  │  └──────────────┘ │                                         │
│  └───────────────────┘                                         │
│                                                                │
│  ┌─────────────────────────────────────────┐                   │
│  │ TransportCongestionControlClient (下行)  │                   │
│  │  - libwebrtc SendSideBwe               │                   │
│  │  - 根据 RTCP TWCC 反馈估算下行可用带宽    │                   │
│  │  - 触发 Consumer 层切换                  │                   │
│  └─────────────────────────────────────────┘                   │
│                                                                │
│  ┌─────────────────────────────────────────┐                   │
│  │ TransportCongestionControlServer (上行)  │                   │
│  │  - 接收端估算上行带宽                     │                   │
│  │  - 生成 REMB / TWCC 反馈给发送端          │                   │
│  └─────────────────────────────────────────┘                   │
└────────────────────────────────────────────────────────────────┘
```

## 6. IPC Channel 协议

```
WorkerThread                    Channel (pipe fd 3/4)              mediasoup-worker
    │                                │                                  │
    │── Request (FlatBuffers) ──────▶│── write(fd3) ───────────────────▶│
    │   {id, method, handlerId,      │   [4-byte len][payload]         │
    │    body}                       │                                  │
    │                                │                                  │
    │◀── Response (FlatBuffers) ─────│◀── write(fd4) ──────────────────│
    │   {id, accepted, body/reason}  │                                  │
    │                                │                                  │
    │◀── Notification ───────────────│◀── (异步) ──────────────────────│
    │   {handlerId, event, body}     │   ICE/DTLS 状态变化              │
    │                                │   Producer/Consumer score        │
    │◀── Log ────────────────────────│◀── worker 日志转发 ──────────────│
```

消息类型:
- **Request → Response**: 同步请求，Channel 内部用 `promise/future` 匹配 id，超时默认 5s
- **Notification**: Worker 主动推送（ICE/DTLS 状态、score 变化等），通过 handlerId 路由到对应 Transport/Producer/Consumer
- **Log**: Worker 日志转发到控制面（D=debug, W=warn, E=error）

帧格式: `[4-byte little-endian length][FlatBuffers payload]`（size-prefixed FlatBuffers）

非线程模式: WorkerThread 的 epoll 监听 Channel consumer fd，调用 `processAvailableData()` 非阻塞读取；`requestBuildWait()` 内部用 `poll()` 泵数据直到 response 到达或超时

## 7. Redis 多节点协调

```
┌──────────┐         ┌──────────┐         ┌──────────┐
│  Node A  │         │  Redis   │         │  Node B  │
│  (SFU)   │         │          │         │  (SFU)   │
└────┬─────┘         └────┬─────┘         └────┬─────┘
     │                    │                    │
     │ SET sfu:node:A     │                    │
     │ {addr|rooms|max|   │                    │
     │  lat|lng|isp|cc}  ─▶                    │
     │                    │                    │
     │                    │◀─ SET sfu:node:B   │
     │                    │                    │
     │ Client join room1  │                    │
     │ ──────────────────▶│                    │
     │ SET sfu:room:room1 │                    │
     │   = "A" (NX, EX)  ─▶                    │
     │                    │                    │
     │                    │  Client join room1 │
     │                    │◀───────────────────│
     │                    │  GET sfu:room:room1│
     │                    │  → "A"             │
     │                    │  GET sfu:node:A    │
     │                    │  → addr of A       │
     │                    │── redirect to A ──▶│
     │                    │                    │
     │ PUBLISH sfu:ch:rooms                    │
     │ PUBLISH sfu:ch:nodes                    │
     │◀── SUBSCRIBE ──────│──── SUBSCRIBE ────▶│
```

Redis Key 设计:
- `sfu:node:{nodeId}` → `address|rooms|maxRooms|lat|lng|isp|country` (TTL)
- `sfu:room:{roomId}` → `nodeId` (TTL, Lua EVAL 原子抢占: SET NX + 死节点接管)
- Pub/Sub: `sfu:ch:nodes`, `sfu:ch:rooms` 实时同步本地缓存
- 每个节点维护 nodeCache_ / roomCache_ 内存缓存，resolveRoom 优先查缓存，miss 才走 Redis
- 默认运行契约下，Redis 是 readiness 强依赖：Redis 不可用时启动失败，运行中 `/readyz` 变为 `503`
- 只有显式配置 `redisRequired=false` 时才允许 `registry_ == nullptr` 的 local-only 模式；该模式不属于默认多节点路由契约

## 8. Linux PlainTransport C++ client 与 PlainTransport 发布路径

```text
PlainClientApp / plain-client
  -> WsClient.connect()
  -> join(roomId, peerId)
  -> plainPublish(videoSsrcs, audioSsrc)
  -> 服务端返回 {ip, port, videoTracks[], audioPt, transportCcExtId...}
  -> 本地 UDP connect 到 PlainTransport
  -> threaded 主路径创建 NetworkThread
  -> registerVideoTrack(ssrc, pt, transportCcExtId)
  -> 发送 comedia probe RTP
  -> 启动 SourceWorker / audio worker
  -> 进入 control-thread + network-thread 主循环

threaded 数据面:
  SourceWorker / audio worker
    -> encoded AU
    -> NetworkThread
       -> SenderTransportController
       -> transport-cc rewrite
       -> RTP / RTCP / NACK / PLI / FIR
       -> TWCC feedback -> send-side BWE -> transport estimate
       -> probe start / send / early-stop / finalize
       -> UDP send
```

QoS 侧链路：

```text
local transport counters + RTCP RR
  -> CanonicalTransportSnapshot
  -> PublisherQosController (per video track)
  -> actionSink
     - source worker encoding command
     - network transport hint
  -> serializeSnapshot()
  -> clientStats
```

server `getStats`:

- 观测 / 对账 / debug
- 不进入 sender QoS 主控制链

```text
notification:
  qosPolicy / qosOverride
    -> plain-client control thread
    -> handlePolicy / handleOverride
```

关键点：

- Linux client 不走 WebRTC sender，但仍走同一套 QoS schema 和通知协议。
- 多 video track 共享同一个 peer 级采样窗口，但每个 track 有独立的 controller / source pipeline / SSRC。
- `NetworkThread` 是当前 threaded plain-client 的唯一 transport owner。
- threaded 主路径已经把 transport-cc、TWCC feedback、send-side estimate 和 probe 生命周期接进发送链路。
- `pauseUpstream` / `audio-only` 不只停止主发送路径，也会阻止 RTCP 触发的视频重传 / keyframe 补发。

## 9. QoS 控制链路

```
Client                RoomService                    QoS 子系统
  │                        │                            │
  │── clientStats ────────▶│                            │
  │  {tracks,...}          │ setClientStats()           │
  │                        │──▶ QosAggregator           │
  │                        │   .ingest(stats)           │
  │                        │                            │
  │                        │   QosRegistry              │
  │                        │   .getSnapshot(peerId)     │
  │                        │                            │
  │                        │   if 需要降级:              │
  │                        │     maybeSendAutomaticQosOverride()
  │                        │     → notify "qosOverride" │
  │                        │       (peer / track scope) │
  │                        │     客户端侧执行编码/暂停策略 │
  │                        │                            │
  │                        │   if 聚合变化:              │
  │                        │     notify("qosConnectionQuality",
  │                        │       {quality,stale,lost,lastUpdatedMs})
  │                        │     broadcast("qosRoomState", roomAggregate)
  │                        │                            │
  │── downlinkClientStats ▶│                            │
  │  {subscriptions,...}   │ setDownlinkClientStats()   │
  │                        │──▶ DownlinkQosRegistry     │
  │                        │                            │
  │                        │   RoomDownlinkPlanner      │
  │                        │   .plan(room)              │
  │                        │   → subscriber-side controls│
  │                        │      setPreferredLayers / priority / pause │
  │                        │   → publisher-side track qosOverride notify │
```

上行 / 发布侧 QoS 说明：
- QoS override 是**通知客户端执行**的（notify `qosOverride`），不是直接操作 Worker 内的 Producer
- override 既可以是 peer 级，也可以是 track 级；客户端收到后自行调整编码参数或暂停/恢复上行
- 有去重和 TTL 刷新机制：相同 signature 的 override 在 TTL/2 内不重复发送
- 恢复时发送 ttlMs=0 的 clear 通知

## 10. 完整生命周期一览

```
启动:
  main() → config → 可选 RoomRegistry.start() → 创建 N × WorkerThread
    → startRegistryWorker()/Redis subscriber（启用 Redis 时）
    → 每个 WT: fork M 个 Worker → epoll 注册 Channel fd → 创建 RoomManager/RoomService
    → waitReady() 且至少 1 个 worker 可用 → SignalingServer.listen(port)

加入:
  WS connect → join → pickLeastLoadedWT → assignRoom → wt.post
    → claimRoom(Redis Lua EVAL 原子抢占；无 Redis 时本地处理)
    → createRoom → createRouter(Worker IPC)
    → replacePeer/addPeer → return {routerRtpCapabilities, existingProducers, participants, qosPolicy}
    → defer 回主线程后再补发 qosPolicy notify

建连:
  createWebRtcTransport(send) → Worker IPC → {id, iceParameters, iceCandidates, dtlsParameters}
  connectWebRtcTransport(send) → Worker IPC → {dtlsLocalRole:"server"} (ICE/DTLS 异步完成)
  createWebRtcTransport(recv) → Worker IPC → {id, iceParameters, iceCandidates, dtlsParameters, consumers[]}
  connectWebRtcTransport(recv) → Worker IPC → {dtlsLocalRole:"server"}

推流:
  produce → Worker IPC (TRANSPORT_PRODUCE) → 自动为房间内其他 Peer 创建 Consumer → notify newConsumer
  → autoRecord: 创建 PlainTransport + PeerRecorder 录制到本地文件

媒体转发:
  Client A → SRTP → Worker Transport → Producer → Router 路由 → Consumer → SRTP → Client B
  Worker 内部: BWE 估算带宽 → SimulcastConsumer 选层 → RTCP 反馈

QoS:
  上行: clientStats → QosAggregator → maybeSendAutomaticQosOverride
    → notify qosOverride / qosConnectionQuality / qosRoomState
  下行: downlinkClientStats → DownlinkQosRegistry → RoomDownlinkPlanner
    → subscriber-side Consumer controls + publisher-side track qosOverride

故障恢复:
  Worker 死亡 → epoll 检测 EOF → handleWorkerDeath → respawn 新 Worker (限速 10s/3次)
  → 受影响的 Room 在 healthCheck 中被检测为 dead → 清理

离开:
  WS close / leave → wt.post → peer.close() → Transport.close() → Worker IPC
    → 清理 QoS/录制状态 → room 空则 Redis unregister → GC timer 清理
```
