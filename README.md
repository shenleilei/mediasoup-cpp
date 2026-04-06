# mediasoup-cpp SFU 项目文档

## 项目概述

用 C++17 完整替换 mediasoup 的 Node.js 控制层，实现纯 C++ 的 SFU（Selective Forwarding Unit）服务。mediasoup-worker C++ 进程保持不变，通过 Unix pipe + FlatBuffers 协议通信。

## 架构

```
┌─────────────────────────────────────┐
│  C++17 SFU Server (mediasoup-sfu)   │
│                                     │
│  ┌───────────┐  ┌────────────────┐  │
│  │ uWebSockets│  │  RoomService   │  │
│  │ HTTP + WS  │  │  (业务逻辑)    │  │
│  └─────┬─────┘  └───────┬────────┘  │
│        │                │           │
│  ┌─────▼──┐  ┌──────────▼────────┐  │
│  │Signaling│  │  RoomManager      │  │
│  │ Server  │  │  Room / Peer      │  │
│  └────────┘  └──────────┬────────┘  │
│                         │           │
│  ┌──────────────────────▼────────┐  │
│  │       WorkerManager           │  │
│  │  (负载均衡: least routers)     │  │
│  │  ┌─────────────────────────┐  │  │
│  │  │  Worker (fork+exec)     │  │  │
│  │  │  ┌───────────────────┐  │  │  │
│  │  │  │ Channel (pipe I/O)│  │  │  │
│  │  │  └────────┬──────────┘  │  │  │
│  │  │  Router → Transport     │  │  │
│  │  │    → Producer/Consumer  │  │  │
│  │  └─────────────────────────┘  │  │
│  └───────────────────────────────┘  │
│                                     │
│  ┌───────────────────────────────┐  │
│  │  RoomRegistry (Redis, 可选)    │  │
│  │  多节点路由 + 死节点接管        │  │
│  └───────────────────────────────┘  │
└──────────────────┬──────────────────┘
                   │ Unix Pipe (FlatBuffers)
          ┌────────▼────────┐
          │ mediasoup-worker │ (预编译二进制 v3.14.16)
          │ (C++ 媒体处理)   │
          └─────────────────┘
```

## 文件结构

```
mediasoup-cpp/
├── CMakeLists.txt                 # 构建配置
├── mediasoup-worker               # 预编译 worker 二进制 (v3.14.16)
├── fbs/                           # FlatBuffers schema (v3.14.16)
│   └── *.fbs                      # 29 个 schema 文件
├── generated/                     # flatc 生成的 C++ 头文件
│   └── *_generated.h              # 29 个生成文件
├── public/                        # Web 前端
│   ├── index.html                 # 实时通话页面 (含 QoS Monitor 面板)
│   ├── playback.html              # 录制回放页面 (视频 + QoS 时间线)
│   └── mediasoup-client.bundle.js # mediasoup-client 3.7.17 浏览器 bundle
├── src/                           # 核心源码 (~3800 行)
│   ├── main.cpp                   # 入口，配置解析，启动流程
│   ├── Constants.h                # 全局常量 (kXx 命名规范)
│   ├── Channel.h/cpp              # Worker pipe 通信层
│   ├── Worker.h/cpp               # Worker 进程管理 (fork/exec/崩溃重启)
│   ├── WorkerManager.h            # 多 Worker 管理，负载均衡，自动 respawn
│   ├── Router.h/cpp               # Router 管理，创建 Transport
│   ├── Transport.h/cpp            # Transport 基类，produce/consume
│   ├── WebRtcTransport.h/cpp      # WebRTC Transport (ICE/DTLS)
│   ├── Producer.h/cpp             # 媒体生产者
│   ├── Consumer.h/cpp             # 媒体消费者
│   ├── ortc.h                     # ORTC 逻辑 (codec 协商/映射)
│   ├── RtpTypes.h                 # RTP 数据结构 + JSON 序列化
│   ├── supportedRtpCapabilities.h # mediasoup 支持的 codec 列表
│   ├── Peer.h                     # Peer 抽象 (transport/producer/consumer)
│   ├── RoomManager.h              # Room/RoomManager 封装，空闲回收，dead room 检测
│   ├── RoomService.h/cpp          # 业务逻辑层 (join/leave/produce/auto-subscribe/QoS/recording)
│   ├── RoomRegistry.h             # Redis 多节点路由 + 死节点接管 + 自动重连
│   ├── Recorder.h                 # 录制 (RTP→WebM) + QoS 时间线记录
│   ├── PipeTransport.h/cpp        # Pipe Transport
│   ├── PlainTransport.h           # Plain Transport (录制用)
│   ├── SignalingServer.h/cpp      # WebSocket 信令 + HTTP 静态文件 + 录制回放 API
│   ├── EventEmitter.h             # 轻量级事件系统
│   ├── Logger.h                   # spdlog 封装
│   └── Utils.h                    # UUID 生成
└── third_party/                   # 第三方依赖
    ├── flatbuffers/               # FlatBuffers (序列化)
    ├── uWebSockets/               # uWebSockets (WebSocket + HTTP)
    ├── nlohmann_json/             # nlohmann/json (JSON 处理)
    └── spdlog/                    # spdlog (日志)
```

## 模块说明

### Channel (Channel.h/cpp)
与 mediasoup-worker 的通信核心。通过 Unix pipe (fd 3/4) 收发 FlatBuffers 消息。

- **发送**: FlatBuffers 序列化 → 4 字节 size prefix (little-endian) → write to pipe
- **接收**: 独立读线程 → 按 size prefix 分帧 → 反序列化 → 分发
- **request()**: 发送 Request，返回 `future<OwnedResponse>`，通过 request id 匹配
- **notify()**: 发送 Notification（无需响应）
- **OwnedResponse**: 拥有 FlatBuffers 数据的所有权，避免 use-after-free

### Worker (Worker.h/cpp)
管理 mediasoup-worker 子进程的生命周期。

- `fork()` + `execvp()` 启动 worker 二进制
- 创建 2 对 pipe，通过 `dup2()` 映射到子进程的 fd 3/4
- 传入命令行参数: logLevel, rtcMinPort, rtcMaxPort, dtls 证书等
- 监听 WORKER_RUNNING notification 确认启动成功
- 子进程退出时触发 workerDied 事件
- 崩溃自动重启: WorkerManager 检测到 worker 死亡后在独立线程中 respawn，带速率限制
- 崩溃通知: checkRoomHealth 每 2 秒检测 dead router，向受影响房间广播 serverRestart

### 稳定性保障
- **Worker 崩溃恢复**: 自动 respawn + serverRestart 通知客户端重连，最多 2 秒感知
- **Redis 自动重连**: 每次操作前 ensureConnected()，heartbeat 线程定期重试
- **磁盘空间保护**: 录制目录超过 10GB 自动清理最老录制（跳过活跃录制）
- **Channel 安全**: builderMutex 只保护序列化，sendBytes 在锁外执行；readThread 用 join 不用 detach
- **EventEmitter 清理**: Transport/Producer/Consumer close 时 off 掉 channel listener，防止内存泄漏
- **getStats 超时**: collectPeerStats 中每个 IPC 调用 500ms 超时，防止阻塞事件循环
- **优雅关闭**: signal handler 只设 atomic flag，uWS timer 轮询后关闭 listen socket
- **常量管理**: Constants.h 统一管理所有 timer/超时/限制值，kXx 命名规范

### Router (Router.h/cpp)
媒体路由管理，对应 mediasoup 的 Router 概念。

- `createWebRtcTransport()`: 创建 WebRTC Transport，解析 ICE/DTLS 参数
- 维护 transports、producers 映射

### ORTC (ortc.h)
RTP 能力协商逻辑，从 mediasoup 的 ortc.ts 移植。

- `generateRouterRtpCapabilities()`: 根据 media codecs 生成 Router RTP 能力
- `getProducerRtpParametersMapping()`: 计算 Producer RTP 参数映射
- `getConsumableRtpParameters()`: 生成可消费的 RTP 参数
- `getConsumerRtpParameters()`: 生成 Consumer RTP 参数
- `getPipeConsumerRtpParameters()`: Pipe Consumer 专用

### SignalingServer (SignalingServer.h/cpp)
基于 uWebSockets 的信令服务。

- WebSocket 路径: `/ws`
- HTTP 静态文件: `/*` → `public/` 目录
- GC 定时器: 每 30 秒清理空闲房间
- JSON 信令协议:
  - `join`: 加入房间，返回 routerRtpCapabilities + existingProducers
  - `createWebRtcTransport`: 创建 Transport，返回 ICE/DTLS 参数
  - `connectWebRtcTransport`: DTLS 握手
  - `produce`: 发布媒体流
  - `consume`: 订阅媒体流
  - `pauseProducer/resumeProducer`: 暂停/恢复
  - `restartIce`: ICE 重启
  - `getStats`: 获取指定 peer 的全链路 QoS 统计
  - `clientStats`: 接收浏览器端 stats（BWE、RTT 等），合并到 QoS 数据
- Notification 推送:
  - `newConsumer`: 新的 consumer 创建（auto-subscribe）
  - `peerJoined/peerLeft`: peer 加入/离开
  - `statsReport`: 每 2 秒推送房间内所有 peer 的 QoS 数据
- HTTP API:
  - `GET /api/recordings`: 录制文件列表（按房间分组）
  - `GET /recordings/{room}/{file}`: 下载录制文件（.webm / .qos.json）

### Peer (Peer.h)
Peer 抽象，封装单个参与者的所有媒体资源。

- sendTransport / recvTransport: 发送和接收 Transport
- producers / consumers: 媒体生产者和消费者映射
- rtpCapabilities: 客户端 RTP 能力

### RoomService (RoomService.h)
独立的业务逻辑层，与信令层解耦。

- `join()` / `leave()`: 房间管理，支持 Redis redirect
- `createTransport()`: 创建 Transport，recvTransport 创建时自动补订阅已有 producer
- `produce()`: 发布媒体流，服务端自动为其他 peer 创建 consumer 并推送 `newConsumer` 通知
- `cleanIdleRooms()`: GC 空闲房间

### RoomRegistry (RoomRegistry.h)
基于 Redis 的多节点房间路由（可选）。

- `claimRoom()`: 房间归属判定，支持死节点检测和自动接管
- 节点心跳: 每 10 秒刷新，30 秒 TTL
- 房间 TTL: 300 秒自动过期

### QoS 实时监控
全链路 QoS 数据采集和推送，从 mediasoup-worker 到前端可视化。

**数据流**: `mediasoup-worker` → `Channel(OwnedNotification)` → `Producer/Consumer(解析 score)` → `RoomService.broadcastStats()` → WebSocket `statsReport` 通知 → 前端 QoS Monitor 面板

- **Channel**: `OwnedNotification` 结构持有通知的原始 FlatBuffers 数据副本，确保下游可安全解析 notification body
- **Producer**: `handleNotification()` 解析 `PRODUCER_SCORE`（每个编码层的 ssrc/rid/score）；`getStats()` 获取 bitrate/jitter/RTT/丢包等
- **Consumer**: 解析 `CONSUMER_SCORE`（score/producerScore）；`getStats()` 获取发送端统计
- **Transport**: `getStats()` 支持 WebRtcTransport（ICE/DTLS 状态、收发比特率、可用带宽）和 PlainTransport
- **RoomService**: `collectPeerStats()` 全链路采集单个 peer 的 stats（含浏览器上报的 clientStats）；`broadcastStats()` 每 2 秒广播所有房间的 stats
- **SignalingServer**: `getStats` 信令方法（主动拉取）；`clientStats` 接收浏览器端 BWE/RTT；2 秒定时器触发 `statsReport` 推送
- **前端 QoS Monitor**: 颜色编码（绿≥7/橙≥4/红<4），展示上行/下行比特率、浏览器 BWE、丢包率、ICE/DTLS 状态、Producer score/jitter/RTT/NACK/PLI/FIR、Consumer score

### 录制与回放 (Recorder.h)
Per-peer 录制，RTP 通过 PlainTransport 转发到本地 UDP，用 libav 封装为 WebM。

- **PeerRecorder**: 接收 RTP → 解析 RTP header → 按 PT 分流 audio/video → `av_interleaved_write_frame()` 写入 WebM
- **QoS 时间线**: 录制期间同步写入 `.qos.json` 文件（与 `.webm` 同名配对），时间基准与视频 PTS 对齐（以第一个 RTP 包到达时刻为零点）
- **自动录制**: `RoomService.autoRecord()` 在 produce 时自动创建 PlainTransport + pipe consumer → PeerRecorder
- **回放 API**: `GET /api/recordings` 列出所有录制；`GET /recordings/{room}/{file}` 下载文件（含路径穿越防护）
- **回放页面**: `playback.html` — 视频播放器 + QoS 面板随进度同步 + score 时间线条形图（绿/橙/红）

## 开发过程中遇到的问题和修复

### 1. FlatBuffers Schema 版本不匹配
**问题**: 最初使用最新 v3 分支的 FBS schema，但 worker 二进制是 v3.14.16，枚举值完全对不上，导致 worker SIGSEGV (status 139)。

**修复**: 下载 v3.14.16 对应的 FBS schema 文件，重新生成 C++ 头文件。

**涉及的差异**:
- `WORKER_CLOSE` 在 3.14.x 是 Request Method，在最新版是 Notification Event
- `ListenInfo` 在 3.14.x 没有 `expose_internal_ip` 字段
- `RtpHeaderExtensionUri` 枚举名不同 (`SsrcAudioLevel` → `AudioLevel`)
- `CreateRtpParameters` 参数数量不同（3.14.x 少一个 msid 参数）
- `CreateProduceRequest` 参数顺序不同

### 2. FlatBuffers Response 生命周期 (Use-After-Free)
**问题**: Channel 读线程解析 Response 后，将 FlatBuffers 指针通过 `promise.set_value()` 传给主线程。但读线程继续处理下一条消息时，底层 buffer 被覆盖，主线程拿到的指针指向已释放的内存。

**修复**: 引入 `OwnedResponse` 结构，在读线程中将 Response 的原始字节数据拷贝到 `std::vector<uint8_t>` 中，主线程通过 `response()` 方法从拥有的数据中重新解析。

### 3. Notification Handler 空指针解引用
**问题**: Router 注册 transport notification handler 时，EventEmitter 只传递了 event 枚举值，notification body 传的是 `nullptr`。当 worker 发送 ICE_STATE_CHANGE 通知时，`handleNotification()` 对 nullptr 调用 FlatBuffers 方法导致 SIGSEGV。

**修复**: 在 `WebRtcTransport::handleNotification()` 中对 notification 指针做 null 检查。

### 4. DTLS Fingerprint 格式不匹配
**问题**: Worker 返回的 fingerprint algorithm 是 FBS 枚举名（如 `SHA1`），但 WebRTC SDP 需要的格式是 `sha-256`。浏览器报 "Failed to create fingerprint from the digest"。

**修复**: 在 Router::createWebRtcTransport() 解析响应时，用 switch 将 FBS 枚举转换为 WebRTC 标准格式。

### 5. Payload Type 分配冲突
**问题**: `generateRouterRtpCapabilities()` 中，opus 分配 PT=100，opus RTX 分配 PT=101。然后 VP8 从 supported capabilities 拿到 preferredPayloadType=101，和 opus RTX 冲突。导致 codec mapping 错误，consumable parameters 里全是 RTX codec。

**修复**: 分配 media codec PT 时，检查 preferredPayloadType 是否还在可用池中。如果已被占用，从池中取新的 PT。

### 6. Consumer 缺少 MID
**问题**: mediasoup-client 的 `recvTransport.consume()` 要求 rtpParameters 有唯一的 `mid` 字段，否则 SDP 解析失败 "A BUNDLE group contains a MID='' matching no m= section"。

**修复**: 在 Transport::consume() 中为每个 consumer 分配递增的 mid 值 (`nextMid_++`)。

### 7. ICE Candidate 地址为 0.0.0.0
**问题**: 服务器监听 `0.0.0.0`，worker 返回的 ICE candidate 地址也是 `0.0.0.0`，浏览器无法连接。

**修复**: 启动时通过 `--announcedIp` 参数指定公网 IP，传入 ListenInfo 的 `announced_address` 字段。

### 8. CDN 不可访问
**问题**: mediasoup-client 的 CDN (jsdelivr.net) 在服务器网络环境下无法访问。

**修复**: 用 npm + esbuild 本地打包 mediasoup-client 为 browser bundle，放在 `public/` 目录下。

### 9. 非 HTTPS 无法访问摄像头
**问题**: 浏览器在非 HTTPS 页面禁止 `getUserMedia()`。

**修复**: 使用 Canvas + `captureStream()` 生成假视频流作为测试替代。

### 10. H264 录制 — matroska 容器 segfault
**问题**: H264 设为默认编码后，Recorder 在 `avformat_write_header` 时 segfault。matroska 容器要求 H264 视频流必须有 SPS/PPS extradata，否则内部空指针崩溃。

**修复**: 延迟写 header（`headerDeferred_`）。`initMuxer()` 只分配 context 和打开文件，不写 header。等收到第一个 IDR 帧后，从缓存的 SPS/PPS 构建 AVCC extradata，再调用 `avformat_write_header`。延迟期间 audio 包缓存在 `pendingAudio_`。

### 11. H264 STAP-A 中 SPS/PPS 混入帧缓冲
**问题**: WebRTC 发送端通常将 SPS/PPS 放在 STAP-A 包中，紧接着发送 IDR 帧的 FU-A 包。之前 STAP-A 解包时所有 NAL 都进了 `videoFrameBuf_`，FU-A IDR start 到来时 flush 了 STAP-A 数据，但因为 SPS/PPS 不是 IDR，`videoKeyframe_` 为 false，帧被丢弃，导致永远等不到 IDR。

**修复**: STAP-A 解包时，SPS(nalType=7) 和 PPS(nalType=8) 单独缓存到 `h264Sps_`/`h264Pps_`，不进帧缓冲。

### 12. H264 Annex-B vs AVCC 格式不匹配
**问题**: 帧缓冲使用 Annex-B 格式（`00 00 00 01` start code），但 matroska 容器要求 AVCC 格式（4字节 NAL length prefix）。写入后 ffmpeg 解码报 "deblocking_filter_idc out of range"。

**修复**: `flushVideoFrame()` 写入前调用 `annexBToAvcc()` 将 start code 转换为 4字节 length prefix。

### 13. QoS 面板指标单位和数据源错误
**问题**: Jitter 直接显示 RTP timestamp 单位（video 36 显示为 "36 ms"，实际 0.4ms）；Fraction Lost 显示原始 0-1 浮点数；Available BW 使用 mediasoup 的 `availableIncomingBitrate`（Transport-CC 模式下永远为 0）。

**修复**: Jitter 除以 clockRate 转毫秒；Fraction Lost 乘 100 加 %；Available BW 改为浏览器端上报的 Transport-CC BWE（`candidate-pair.availableOutgoingBitrate`），新增 `clientStats` 信令方法接收浏览器 stats。

## 线程模型

```
主线程 (uWS event loop)
├── WebSocket 信令收发 (join/produce/consume/...)
├── HTTP 静态文件 + 录制回放 API
├── GC 定时器 (30s) / Stats 广播定时器 (2s)
├── RoomService 业务逻辑
└── Channel.request() → pipe write → future.get() 阻塞等待 worker 响应

Channel readThread (× N worker)
├── 从 pipe fd 读取 worker 返回的 FlatBuffers
├── 按 request id 匹配 → promise.set_value() 唤醒主线程
└── 分发 Notification → Transport/Producer/Consumer.handleNotification()

Worker waitThread (× N worker)
└── waitpid() 等待子进程退出 → 触发 workerDied

Recorder recvLoop (× M 录制 peer)
├── poll() + recv() UDP 收 RTP
├── H264/VP8 解包 + 帧重组
└── av_interleaved_write_frame() 写 WebM/MKV

RoomRegistry heartbeat (0 或 1)
└── 每 10s 刷新 Redis 节点心跳
```

典型场景（1 worker, 2 peer 录制）= 6 线程。共享数据通过 mutex 保护（Channel.pendingRequests_、Recorder.muxMutex_/qosMutex_）。

**已知风险**: 主线程 `Channel.request().get()` 同步阻塞，worker 响应慢会卡住信令循环。

## 已知限制和待改进

### 单 Worker 性能基准

使用 `mediasoup_bench` 压测工具测量单 Worker 的 RTP 转发极限。

**测试方法**:
- 每房间 1 Producer + 2 Consumer（模拟对端观看 + 录制）
- 使用 PlainTransport + audio/opus，1200 字节包，300 pps/stream（等效 1080p 30fps 带宽）
- 逐步加压（每轮 +10 房间），稳定 5 秒后采集指标
- 停止条件：连续 3 轮丢包率 >1%
- 测试环境：Intel Xeon Platinum 2.5GHz, 2 vCPU

**测试结果**:

| 网络模式 | 峰值房间 | 峰值 CPU(单核) | 峰值 RSS | 峰值吞吐 | 丢包起点 |
|----------|---------|---------------|---------|---------|---------|
| loopback (127.0.0.1) | 240 | 82% | 180 MB | 72k→144k pps | 250 rooms |
| 真实网络栈 (eth0 IP) | 80 | 23% | 67 MB | 24k→48k pps | 90 rooms |

**关键发现**:
- Worker 进程每房间 CPU 开销约 0.33%（loopback），瓶颈在 Worker 内部 RTP 路由
- 走真实网络栈时，内核 softirq 处理 UDP 包的开销导致 90 rooms 就开始丢包，此时 Worker CPU 才 26%
- 线性扩展：CPU 和房间数严格线性关系，RSS 每房间约 0.75 MB
- SendRate 列验证发送端始终维持 100% 目标速率，排除"降载假低丢包"

**真实场景估算**（WebRtcTransport + SRTP + audio+video 双流）:
- SRTP 加解密额外 5-10% CPU
- audio+video 双流 = 负载翻倍
- 保守估计单 Worker 约 **30-40 个 1v1 房间**（走真实网络栈口径）

**运行压测**:
```bash
# loopback 模式
./build/mediasoup_bench --rooms-per-round=10 --warmup-sec=10 --round-sec=5 --max-rooms=500

# 走真实网络栈
./build/mediasoup_bench --ip=$(hostname -I | awk '{print $1}') --rooms-per-round=10 --warmup-sec=10 --round-sec=5 --max-rooms=500
```

**注意事项**:
- 使用 audio/opus 而非 video/VP8，因为 mediasoup Worker 的 video PipeConsumer 需要有效 VP8 keyframe 才开始转发，PlainTransport 场景下无法完成 keyframe 协商
- PlainTransport 不经过 SRTP 加解密，测的是 Worker 纯 RTP 路由转发能力
- `--ip=` 参数控制是否走真实网络栈，默认 127.0.0.1 走 loopback
- UdpDrops 列读取 `/proc/net/udp` drops，是系统全局口径（包含非本测试的 socket），仅供参考
- 两个 Consumer 均为 PIPE 类型（绕过 keyframe 等待），与真实场景的 SIMPLE Consumer 行为略有差异（SIMPLE 有 BWE/NACK 反馈开销）

**方法学保障**:
- 计数器使用 `std::atomic<uint64_t>` + relaxed ordering，无数据竞争
- Payload Type 从 router capabilities 动态获取，不硬编码
- 时间戳步进 1600（48kHz/30fps），匹配 opus 时钟域
- SendRate% 列校验发送端是否维持目标速率，峰值判定要求 ≥95%
- sender 超时不补偿（reset nextTick），避免"降载后低丢包"假象
- 预热阶段（默认 20 秒）排除 Worker/JIT/缓存冷启动影响
- 每轮增量统计（reset counters），不用全局累计，避免稀释后期抖动

### 当前限制
1. **单线程信令**: uWebSockets 事件循环是单线程的，Channel 的 FlatBufferBuilder 在信令线程使用但没有和读线程完全隔离
2. **无 HTTPS 支持**: 当前只有 HTTP，生产环境需要 TLS（uWebSockets 支持 `uWS::SSLApp`）
3. **Worker 二进制版本锁定**: 当前绑定 mediasoup-worker v3.14.16，升级需要同步更新 FBS schema

### 待实现
1. **Dynacast**: 按需发送视频层，减少带宽消耗
2. **信令协议标准化**: 定义统一的 request/response/notification schema
3. **多节点录制回放**: 当前每个节点只能查看本地录制文件。方案选项：
   - 共享存储（NFS/S3 FUSE）— 零代码改动，运维层面解决
   - 对象存储上传 — 录制完成后异步上传到 S3/OSS，API 从对象存储列文件
   - Redis 索引 + 跨节点 proxy — 录制开始/结束时写 Redis 索引，查询时聚合所有节点，播放时 proxy 到文件所在节点（需处理 CORS、节点下线等问题）

### 建议改进
1. 添加 HTTPS/WSS 支持
2. 实现 graceful shutdown（当前用 SIGTERM + detach）
3. 添加 Worker 健康检查和自动重启
4. 升级到最新 mediasoup-worker 版本（需要更新系统 glibc 或从源码编译）

## 构建和运行

## 快速部署（新环境）

### 一键部署
```bash
git clone <your-repo-url> mediasoup-cpp
cd mediasoup-cpp
./setup.sh
```

`setup.sh` 会自动完成：
1. 检查系统依赖（cmake, g++, openssl, zlib）
2. 克隆第三方库到 `third_party/`
3. 编译 flatc 并从 `fbs/` 生成 C++ 头文件到 `generated/`
4. 下载 mediasoup-worker v3.14.16 预编译二进制（根据内核版本自动选择 kernel5/kernel6）
5. 编译 Release 版本

### 系统依赖

不同发行版的安装命令：

```bash
# Ubuntu / Debian
apt install -y build-essential cmake pkg-config libssl-dev zlib1g-dev git curl

# CentOS / RHEL / Alibaba Cloud Linux
yum install -y gcc-c++ cmake openssl-devel zlib-devel git curl
```

要求：
- C++17 编译器（GCC 10+ 或 Clang 10+）
- CMake 3.16+
- OpenSSL 开发库
- zlib
- pthread（通常已内置）

项目的其他依赖（flatbuffers、uWebSockets、nlohmann/json、spdlog）全部在 `third_party/` 目录下，由 `setup.sh` 自动克隆，不需要系统安装。

### 手动部署（逐步）

如果不想用 `setup.sh`，可以手动执行：

```bash
# 1. 克隆第三方依赖
mkdir -p third_party
git clone --depth 1 https://github.com/google/flatbuffers.git third_party/flatbuffers
git clone --depth 1 --recurse-submodules https://github.com/uNetworking/uWebSockets.git third_party/uWebSockets
git clone --depth 1 https://github.com/nlohmann/json.git third_party/nlohmann_json
git clone --depth 1 https://github.com/gabime/spdlog.git third_party/spdlog

# 2. 编译 flatc 并生成 C++ 头文件
cmake -B third_party/flatbuffers/build -S third_party/flatbuffers -DCMAKE_BUILD_TYPE=Release -DFLATBUFFERS_BUILD_TESTS=OFF
cmake --build third_party/flatbuffers/build -j$(nproc) --target flatc
mkdir -p generated
cd fbs && for f in *.fbs; do ../third_party/flatbuffers/build/flatc --cpp -o ../generated/ --gen-object-api --scoped-enums "$f"; done && cd ..

# 3. 下载 mediasoup-worker（kernel5 版本，适用于 Linux 5.x）
curl -L -o /tmp/mw.tgz https://github.com/versatica/mediasoup/releases/download/3.14.16/mediasoup-worker-3.14.16-linux-x64-kernel5.tgz
tar xzf /tmp/mw.tgz && chmod +x mediasoup-worker

# 4. 编译
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 构建 Release
```bash
cd mediasoup-cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 构建 Debug (带 AddressSanitizer)

ASAN 版本会检测内存越界、use-after-free、内存泄漏等问题，crash 时会打印完整的堆栈信息，推荐开发调试时使用。

```bash
cmake -B build_dbg -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-fsanitize=address" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address"
cmake --build build_dbg -j$(nproc)
```

`-B build_dbg` 指定构建输出目录为 `build_dbg`，和 Release 的 `build/` 互不干扰，可同时保留两个版本。

### 运行单元测试

项目使用 GoogleTest 框架，测试代码在 `tests/` 目录下。`BUILD_TESTS` 选项默认开启，正常构建即包含测试。

```bash
cd mediasoup-cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) --target mediasoup_tests
./build/mediasoup_tests
```

也可以用 `ctest`：
```bash
cd build && ctest --output-on-failure
```

当前测试覆盖：
| Test Suite | 用例数 | 说明 |
|------------|--------|------|
| `OrtcTest` | 14 | ORTC codec 匹配、H264 profile、Router RTP 能力生成 |
| `RoomTest` | 6 | Peer 增删、空闲检测、参与者列表 |
| `PeerTest` | 3 | JSON 序列化、Transport 查询、关闭幂等性 |
| `RtpTypesTest` | 8 | RTP 数据结构 JSON 序列化/反序列化 |
| `SupportedCapabilitiesTest` | 4 | 支持的 codec 列表校验 |
| `OwnedNotificationTest` | 3 | FlatBuffers 通知数据所有权、空数据/垃圾数据安全性 |
| `ProducerScoreTest` | 2 | Producer score 结构体、边界值 |
| `ConsumerScoreTest` | 2 | Consumer score 结构体、边界值 |
| `RoomQosTest` | 4 | getPeerIds 空房间/多 peer/删除/不存在 peer |
| `OwnedResponseTest` | 2 | Response 数据所有权、垃圾数据安全性 |
| `RecorderQosTest` | 4 | QoS 文件创建、未启动不写入、无 snapshot 空数组、H264 deferred header 安全性 |
| `RtpHeaderTest` | 2 | RTP marker 位解析、短包拒绝 |
| `Vp8DescriptorTest` | 3 | VP8 payload descriptor 剥离（简单/扩展/空 payload） |
| `AnnexBToAvccTest` | 3 | Annex-B→AVCC 转换（单 NAL/多 NAL/空输入） |

集成测试（需要启动 SFU + mediasoup-worker）：
```bash
./build/mediasoup_integration_tests    # 15 个: join/leave/produce/subscribe/recording(VP8+H264)
./build/mediasoup_qos_integration_tests # 15 个: getStats/statsReport/QoS录制/回放API/路径穿越
./build/mediasoup_e2e_tests            # 3 个: 完整会议生命周期/迟到者/快速进出
./build/mediasoup_topology_tests       # 6 个: 多SFU拓扑/节点故障接管/跨节点会议
```

`-fsanitize=address` 的作用：
- **编译时**：GCC/Clang 在每次内存访问前后插入检查代码（检查越界、use-after-free 等）
- **链接时**：自动链接 `libasan.so`（ASAN 运行时库），替换 malloc/free，提供错误报告和堆栈回溯
- 三个 CMake 变量分别对应 C++ 编译、C 编译（uSockets 是 C 代码）、链接阶段

`-fno-omit-frame-pointer` 保留栈帧指针，确保 crash 时能打印完整的函数调用链。

确认是否为 ASAN 版本：
```bash
# 方法1：看文件大小（ASAN ~30MB，Release ~1.5MB）
ls -lh build/mediasoup-sfu build_dbg/mediasoup-sfu

# 方法2：检查是否链接了 libasan
ldd build_dbg/mediasoup-sfu | grep asan

# 方法3：查看 CMake 缓存的编译参数
grep "fsanitize" build_dbg/CMakeCache.txt
```

运行 ASAN 版本：
```bash
cd /root/mediasoup-cpp
./build_dbg/mediasoup-sfu \
  --workers=1 \
  --workerBin=/root/mediasoup-cpp/mediasoup-worker \
  --port=3003 \
  --announcedIp=<公网IP>
```

如果发生内存错误，终端会输出类似以下信息：
```
==12345==ERROR: AddressSanitizer: heap-use-after-free on address 0x...
READ of size 4 at 0x... thread T1
    #0 0x... in function_name file.cpp:123
    #1 0x... in caller_function file2.cpp:456
    ...
```

注意事项：
- ASAN 版本性能约为 Release 的 1/3，仅用于调试
- ASAN 版本二进制约 30MB（Release 约 1.5MB）
- 需要 GCC 10+ 或 Clang 10+ 的 libasan 支持
- 将 ASAN 输出重定向到文件：`./build_dbg/mediasoup-sfu ... 2>/path/to/asan.log`

### 运行
```bash
cd mediasoup-cpp
./build/mediasoup-sfu \
  --workers=1 \
  --workerBin=/path/to/mediasoup-worker \
  --port=3003 \
  --announcedIp=<公网IP> \
  --listenIp=0.0.0.0
```

### 命令行参数
| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--port` | 3000 | WebSocket/HTTP 监听端口 |
| `--workers` | CPU 核心数 | Worker 进程数量 |
| `--workerBin` | ./mediasoup-worker | Worker 二进制路径 |
| `--listenIp` | 0.0.0.0 | WebRTC 监听 IP |
| `--announcedIp` | (自动探测) | 公网 IP（空则自动探测） |
| `--recordDir` | ./recordings | 录制文件输出目录 |
| `--redisHost` | 127.0.0.1 | Redis 地址（多节点模式） |
| `--redisPort` | 6379 | Redis 端口 |
| `--nodeId` | hostname:port | 当前节点 ID |
| `--nodeAddress` | (自动生成) | 当前节点 WebSocket 地址 |
| `--nodaemon` | (默认后台运行) | 前台运行，不 daemonize |
| `--logFile` | /var/log/mediasoup-sfu.log | 日志文件路径（daemon 模式） |
| `--pidFile` | /var/run/mediasoup-sfu.pid | PID 文件路径（daemon 模式） |

### 浏览器测试
1. 访问 `http://<服务器IP>:<端口>/`
2. 点击 "Join Room" 加入房间
3. 点击 "Publish A/V" 发布音视频
4. 在另一个标签页重复以上步骤，两端可互相看到视频
5. 页面下方 QoS Monitor 面板实时展示网络质量

### 录制回放
1. 启动 SFU（默认录制到 `./recordings`）
2. 进行一次通话后，访问 `http://<服务器IP>:<端口>/playback.html`
3. 点击录制文件即可回放，视频下方同步展示当时的 QoS 数据

## 技术栈

| 组件 | 技术 | 用途 |
|------|------|------|
| 序列化 | FlatBuffers | Worker 通信协议 |
| WebSocket/HTTP | uWebSockets + uSockets | 信令和静态文件服务 |
| JSON | nlohmann/json | 信令消息和配置 |
| 日志 | spdlog | 结构化日志 |
| 媒体处理 | mediasoup-worker (C++) | RTP/RTCP/ICE/DTLS |
