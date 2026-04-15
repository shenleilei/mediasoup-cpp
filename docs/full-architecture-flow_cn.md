# 全链路架构流程图

> 从客户端 WebSocket 连接到 Worker 媒体转发的完整路径。

## 1. 进程 / 线程模型总览

```
┌─────────────────────────────────────────────────────────────┐
│                        main()                               │
│  解析配置 → 创建 RoomRegistry(Redis) → 创建 N 个 WorkerThread │
└──────────────────────────┬──────────────────────────────────┘
                           │
              ┌────────────┼────────────────┐
              ▼            ▼                ▼
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
     ┌──────────────┐
     │mediasoup-worker│  ← 单线程 libuv 事件循环
     │  (C++ 进程)    │
     │  Router        │
     │  Transport     │
     │  Producer      │
     │  Consumer      │
     │  BWE           │
     └──────────────┘
```

关键点：
- **主线程 (uWS)**: 运行 SignalingServer，处理 WebSocket / HTTP
- **WorkerThread**: 每个是独立 std::thread + epoll 事件循环，拥有若干 Worker 子进程
- **Worker**: fork+exec 的 mediasoup-worker 进程，通过 pipe (fd 3/4) 与 WorkerThread 通信
- **Channel**: FlatBuffers 序列化的 IPC 协议，Request/Response/Notification 三种消息

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
  │                        │                         │          │    (Redis SET NX)        │         │
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
  │                        │                         │                      │ room->addPeer(peer)    │
  │                        │                         │                      │ return rtpCapabilities  │
  │                        │                         │◀─────────────────────│                        │
  │                        │◀── loop->defer(resp) ───│                      │                        │
  │◀── join response ──────│                         │                      │                        │
  │   {routerRtpCaps}      │ 绑定 sd->roomId/peerId  │                      │                        │
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
  │                        │                         │                          │
  │                        │                         │                          │ ICE 连通性检查
  │                        │                         │                          │ DTLS 握手
  │                        │                         │◀── Notification:         │
  │                        │                         │    ICE_STATE_CHANGE      │
  │                        │                         │    (connected)           │
  │                        │                         │◀── Notification:         │
  │                        │                         │    DTLS_STATE_CHANGE     │
  │                        │                         │    (connected)           │
  │◀── ok ─────────────────│◀────────────────────────│                          │
```

说明：
- 客户端需要创建两个 Transport: **sendTransport** (producing=true) 和 **recvTransport** (consuming=true)
- ICE candidate 由 Worker 在创建 Transport 时生成，通过 response 一次性返回（非 trickle）
- DTLS 角色: Worker 默认 `auto`，客户端提供 `client` 角色的 fingerprint

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
  │                        │   {consumerId, producerId, │                    │
  │                        │    kind, rtpParameters}    │                    │
```

关键设计：
- **服务端驱动订阅**: A produce 后，RoomService 自动为房间内所有已有 recvTransport 的 Peer 创建 Consumer
- 同样，当 B 创建 recvTransport 时，也会自动订阅房间内所有已有的 Producer

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
- **Request → Response**: 同步请求，Channel 内部用 `promise/future` 匹配 id
- **Notification**: Worker 主动推送（ICE/DTLS 状态、score 变化等）
- **Log**: Worker 日志转发到控制面

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
- `sfu:room:{roomId}` → `nodeId` (TTL, SET NX 抢占)
- Pub/Sub: `sfu:ch:nodes`, `sfu:ch:rooms` 实时同步缓存

## 8. QoS 控制链路

```
Client                RoomService                    QoS 子系统
  │                        │                            │
  │── clientStats ────────▶│                            │
  │  {producerId,          │ setClientStats()           │
  │   bitrate,rtt,loss}    │──▶ QosAggregator           │
  │                        │   .ingest(stats)           │
  │                        │                            │
  │                        │   QosRegistry              │
  │                        │   .getSnapshot(peerId)     │
  │                        │                            │
  │                        │   if 需要降级:              │
  │                        │     setQosOverride()       │
  │                        │     → Channel: PRODUCER_SEND │ (maxBitrate/maxLayers)
  │                        │                            │
  │                        │   if 需要通知客户端:        │
  │                        │     notify("connectionQuality", │
  │                        │       {quality, score})    │
  │                        │                            │
  │── downlinkStats ──────▶│                            │
  │  {consumerId,          │ setDownlinkClientStats()   │
  │   bitrate,loss}        │──▶ DownlinkQosRegistry     │
  │                        │                            │
  │                        │   RoomDownlinkPlanner      │
  │                        │   .plan(room)              │
  │                        │   → SubscriberBudgetAllocator │
  │                        │   → PublisherSupplyController  │
  │                        │   → Consumer setPreferredLayers│
  │                        │   → Producer QoS override      │
```

## 9. 完整生命周期一览

```
启动:
  main() → config → RoomRegistry.start() → N × WorkerThread.start()
    → 每个 WT: fork M 个 Worker → epoll 注册 Channel fd → 创建 RoomManager/RoomService
    → SignalingServer.listen(port)

加入:
  WS connect → join → pickWT → wt.post → claimRoom(Redis) → createRoom → createRouter(Worker IPC)
    → addPeer → return rtpCapabilities

建连:
  createWebRtcTransport(send) → Worker IPC → {ice, dtls}
  connectWebRtcTransport(send) → Worker IPC → ICE+DTLS 握手
  createWebRtcTransport(recv) → Worker IPC → {ice, dtls} → 自动订阅已有 Producer
  connectWebRtcTransport(recv) → Worker IPC → ICE+DTLS 握手

推流:
  produce → Worker IPC (TRANSPORT_PRODUCE) → 自动为房间内其他 Peer 创建 Consumer → notify newConsumer

媒体转发:
  Client A → SRTP → Worker Transport → Producer → Router 路由 → Consumer → SRTP → Client B
  Worker 内部: BWE 估算带宽 → SimulcastConsumer 选层 → RTCP 反馈

QoS:
  clientStats → QosAggregator → 自动 override (上行)
  downlinkStats → RoomDownlinkPlanner → 预算分配 (下行)

离开:
  WS close / leave → wt.post → peer.close() → Transport.close() → Worker IPC
    → room 空则标记 idle → GC timer 清理 → Redis unregister
```
