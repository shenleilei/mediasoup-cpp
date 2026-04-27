# mediasoup-cpp

这是一个围绕上游 `mediasoup-worker` 构建的 C++17 SFU 控制平面。

本项目使用原生的 C++ 服务器取代了 mediasoup 通常的 Node.js 控制层，同时仍然依赖经过实战检验的 `mediasoup-worker` 进程进行媒体处理。最终呈现的是一个原生的 C++ 信令技术栈，具备房间/会话管理、多节点房间路由、录制以及 QoS 聚合等功能。

## 在线体验 (Live Demo)

立即体验: **http://47.99.237.234:3000**

在两个浏览器标签页或两台设备中打开，即可开始带有实时 QoS 监控的 1v1 通话。

录制回放: **http://47.99.237.234:3000/playback.html**

## 为什么这么做 (Why)

mediasoup 得到了广泛应用，但其默认的控制平面是 Node.js。本项目保留了上游的 C++ 媒体 worker，并将控制平面替换为 C++17，以实现：

- 更低的信令开销
- 单二进制文件（single-binary）的控制平面部署
- 对线程、故障恢复以及与原生系统集成的更精细控制

媒体平面仍然由 `mediasoup-worker` 处理，通过 Unix 管道（Unix pipes）和 FlatBuffers IPC 进行连接。

## 本项目的定位 (What This Project Is)

本项目包含：

- 一个 WebSocket/HTTP 信令服务器
- 一个房间 / 节点 (peer) / 会话 (session) 管理层
- 一个 Transport / Producer / Consumer 编排层
- 一个基于 Redis 的多节点房间路由层
- 一个录制器 / QoS 集成层

本项目不是：

- 对 mediasoup 媒体内部机制的重写
- `mediasoup-worker` 的替代品

## 核心优势与业界对比 (Core Advantages)

相比于原版 mediasoup (Node.js 控制面)、LiveKit (Go)、SRS 或 Licode 等业界主流开源 SFU 方案，本项目在架构设计与工程交付上做出了具有针对性的降维打击级改进：

### 1. 极致的性能与无锁并发模型
- **纯 C++17 单文件部署 (Single-Binary)：** 彻底摒弃 Node.js / V8 引擎，采用 `uWebSockets` 作为信令和 HTTP 层，消除了系统垃圾回收 (GC) 的停顿尖峰，大幅降低高并发 Join/Leave 时的信令延迟与内存开销。
- **无锁状态机 (Thread-per-Room)：** 独创的 WorkerThread 线程池机制，将单个房间的完整生命周期绑定在唯一的串行事件循环中，**彻底避免了多线程并发读写房间状态时的细粒度锁冲突**。
- **现代化零拷贝 IPC：** 信令面与底层 `mediasoup-worker` 进程间采用基于 Unix 管道的 **FlatBuffers 通信**，将海量 RTP 统计快照和控制指令的传递序列化开销压缩到了极致。

### 2. 业界天花板级的端到端 QoS 体系
目前绝大多数开源 SFU 的 QoS 仅停留在算法拼凑的“功能实现”层面，而本项目不仅实现了算法，还提供了一套**有严格 SLA 数据背书的、高度自动化的毫秒级 QoS 矩阵测试台 (Matrix Harness)**：
- **系统级量化的弱网对抗：** 通过脚本驱动无头浏览器或 C++ 客户端，精准注入极端弱网（如盲区：`200kbps带宽 / 20%丢包 / 500ms RTT`），不但测试能否恢复，更能够**精确量化退避和恢复生命周期的每一个关键帧 (精确到毫秒)**。
- **极其细腻的上行状态机 (Uplink State Machine)：** 实现了从 `stable` -> `early_warning` -> `congested` -> `recovering` 的多级阶梯降级 (Ladder Control)。独创的两阶段快速恢复 (Fast-Path Recovery) 机制，在不引起网络震荡 (overshoot) 的前提下，将极端弱网后的视频恢复时间硬生生提前了 5~6 秒，超越了标准的 LiveKit 分配器与探测策略。
- **纯 C++ 客户端对齐 (Plain-Client Parity)：** 仓库内包含一个硬核的 Linux C++ 纯客户端，完美复刻了浏览器的发送端带宽评估 (Send-Side BWE)、探测流隔离 (Probe Isolation) 以及平滑发送 (Pacing) 逻辑，并在数十个严苛的独立矩阵测试中跑通。
- **铁血的防回归护栏 (Nightly Full Regression)：** 所有的 QoS 指标结果被强绑定到 Nightly CI 和质量关卡中。任何可能导致弱网恢复变慢（即使慢了2秒）的代码修改都会被立即阻断报警，确保拥塞控制算法的演进极其稳健。

## 高层架构 (High-Level Architecture)

```text
Browser
  │
  │ WebSocket / HTTP
  ▼
uWS Main Thread
  │
  ├─ SignalingServer 主线程粘合层
  ├─ SignalingServerWs 请求/会话处理
  ├─ SignalingServerHttp 路由处理
  ├─ 绑定 roomId -> WorkerThread
  └─ 发送响应 / 通知
  │
  ▼
WorkerThread Pool (N)
  │
  ├─ 每个 WorkerThread 拥有一个串行事件循环
  ├─ 拥有 mediasoup worker 进程的子集
  ├─ 拥有已分配房间的 RoomManager + RoomService
  ├─ 通过 epoll 驱动 Channel IPC
  └─ 单线程运行房间业务逻辑
  │
  ▼
RoomService Facade
  │
  ├─ 生命周期切面 (lifecycle slice)
  ├─ 媒体切面 (media slice)
  ├─ 统计 / QoS 切面
  └─ 下行调度规划切面 (downlink planning slice)
  │
  ▼
Router / Transport / Producer / Consumer
  │
  ▼
Channel (基于 Unix pipe 的 FlatBuffers)
  │
  ▼
mediasoup-worker 进程
  │
  ▼
RTP / SRTP / ICE / DTLS
```

## Linux 纯传输客户端 (Linux PlainTransport Client)

除了浏览器路径，本仓库还提供了一个 Linux `PlainTransport C++ 客户端`，用于端到端的发布 / QoS 验证以及矩阵回归测试。

```text
MP4
  -> FFmpeg 解封装 / 解码
  -> 每轨 x264 重新编码 (或 H264 拷贝路径)
  -> RTP 打包
  -> UDP
  -> mediasoup PlainTransport

WebSocket 控制平面
  -> join / plainPublish / clientStats / getStats
  -> qosPolicy / qosOverride 通知

RTCP 侧边路径
  -> SR / RR / RTT / jitter
  -> NACK / PLI / FIR
  -> 每轨重传 + 关键帧缓存
```

当前的架构文档：

- [docs/dependencies_cn.md](./docs/dependencies_cn.md)
- [docs/linux-client-architecture_cn.md](./docs/linux-client-architecture_cn.md)
- [docs/linux-client-multi-source-thread-model_cn.md](./docs/linux-client-multi-source-thread-model_cn.md)
- [docs/linux-client-threaded-implementation-checklist_cn.md](./docs/linux-client-threaded-implementation-checklist_cn.md)
- [docs/architecture_cn.md](./docs/architecture_cn.md)
- [docs/plain-client-qos-parity-checklist.md](./docs/plain-client-qos-parity-checklist.md)

## 多节点路由架构 (Multi-Node Routing Architecture)

```text
SignalingServer
  │
  ├─ WorkerThread Pool
  │
  └─ Registry Worker Thread
       │
       └─ RoomRegistry
            ├─ Redis 命令连接
            ├─ 本地房间缓存
            ├─ 本地节点缓存
            ├─ 房间所有权声明 / 刷新 / 注销
            └─ resolveRoom(roomId, remoteIp)
                 │
                 └─ GeoRouter (可选)
                      ├─ ip2region 查找
                      ├─ 国家隔离
                      └─ 基于 ISP / 距离的打分
```

此外，还有一个专用的 Redis 订阅者线程（subscriber thread），负责监听节点和房间更新，并刷新本地缓存状态。

## 特性 (Features)

- 以房间为中心的设计：当一个 peer 开始发布媒体流时，房间内的其他 peer 会被自动订阅。
- 基于 WorkerThread 的信令：房间控制逻辑在 uWS 主循环之外运行。
- 会话身份标识：重连会替换旧连接，过时的请求会被 `sessionId` 拒绝。
- 基于 Redis 的房间路由：房间所有权、缓存、pub/sub 同步、降级为本地模式。
- 感知地理位置的路由：基于 ip2region 的国家 / ISP / 距离打分。
- H264 / VP8 录制：通过 FFmpeg 进行 RTP 解包和 WebM 封装。
- QoS 监控：服务器统计数据 + 客户端统计数据的聚合及定期推送。
- Worker 崩溃恢复：带有速率限制的子进程重生（respawn）。
- 守护进程模式 (daemon mode)：支持 fork、PID 文件、结构化日志。

## QoS 状态 (QoS Status)

本仓库目前包含：

- 完整的上行 (Uplink) QoS 链路，覆盖：
  - 客户端 Publisher QoS 状态机和阶梯控制
  - 服务端对 `clientStats` 的接收、校验、聚合以及自动 Override 生成
  - 针对推流、陈旧序列号 (stale-seq)、策略更新、自动 Override、手动清除的浏览器/Node 测试套件
  - 浏览器回环弱网矩阵 (weak-network matrix) 执行及逐 Case 的结果报告
- 完整的下行 (Downlink) QoS 链路，覆盖：
  - 订阅者侧对 `downlinkClientStats` 的接收、校验、存储以及控制器执行
  - 服务端基于隐藏 (hidden) / 驻留 (pinned) / 尺寸的带宽分配、基于健康度的降级/恢复，以及优先级处理
  - 针对持续全隐藏 (all-hidden) 场景，协调 Producer 侧的零需求 `pauseUpstream` / `resumeUpstream` 控制
  - 针对消费者控制、下行自动暂停/恢复，以及在受限下行网络下优先级竞争的浏览器测试套件

当前的下行范围主要是：接收端控制加上对零需求的发布端自动暂停/恢复协调。
`dynacast` 以及房间级别的全局比特率预算划分仍然是后续的工作内容。

### 当前检验状态 (Current checked status)

当前仓库文档和生成的制品显示：

- 浏览器上行矩阵主闸门 (main gate)：`43 / 43 PASS` (`2026-04-13`)
- PlainTransport C++ 客户端矩阵：`48 / 48 PASS` (`2026-04-26`)
- PlainTransport C++ 客户端信令测试套件：`PASS`
- C++ 客户端 QoS 单元测试对齐 (parity)：`PASS`

当前范围提示：

- 上行 QoS 主链路在浏览器和 PlainTransport C++ 客户端上均已闭环
- 下行目前覆盖接收端控制以及零需求发布端暂停/恢复协调
- `dynacast` 和房间级全局比特率预算划分是后续工作

事实来源链接 (Source-of-truth links)：

- QoS 整体状态：[docs/qos-status.md](./docs/qos-status.md)
- 最终总结：[docs/uplink-qos-final-report.md](./docs/uplink-qos-final-report.md)
- 结果总结：[docs/uplink-qos-test-results-summary.md](./docs/uplink-qos-test-results-summary.md)
- 逐 Case 最终结果：[docs/uplink-qos-case-results.md](./docs/uplink-qos-case-results.md)
- 纯客户端当前状态：[docs/plain-client-qos-status.md](./docs/plain-client-qos-status.md)
- 纯客户端对齐检查单：[docs/plain-client-qos-parity-checklist.md](./docs/plain-client-qos-parity-checklist.md)
- 纯客户端矩阵结果：[docs/plain-client-qos-case-results.md](./docs/plain-client-qos-case-results.md)
- 下行当前状态：[docs/downlink-qos-status.md](./docs/downlink-qos-status.md)
- Linux 客户端架构：[docs/linux-client-architecture_cn.md](./docs/linux-client-architecture_cn.md)
- 测试覆盖地图：[docs/qos-test-coverage_cn.md](./docs/qos-test-coverage_cn.md)
- 生成的矩阵制品：[docs/generated/uplink-qos-matrix-report.json](./docs/generated/uplink-qos-matrix-report.json)

## 核心运行时模型 (Core Runtime Model)

### uWS 主线程 (uWS Main Thread)

主线程负责：

- WebSocket 的接收 / 关闭 / 消息处理
- HTTP 接口端点
- 房间到线程的分配 (room-to-thread dispatch)
- Socket/Session 的所有权
- 向客户端事件循环的延迟发送 (deferred sends)

重要不变量 (Important invariant)：

- **同一个房间 -> 同一个 WorkerThread**

在执行业务任务之前，第一次成功的加入（join）就会在主线程中将 `roomId` 绑定到一个特定的 `WorkerThread`。

### 工作线程 (WorkerThread)

每个 `WorkerThread` 是一个串行事件循环，拥有：

- 零个或多个 mediasoup worker 子进程
- 一个 `RoomManager`
- 一个 `RoomService`
- 一个任务队列
- worker 管道文件描述符的 epoll 注册
- 健康检查 / 垃圾回收 (GC) 定时器

在 `WorkerThread` 内部，房间逻辑被刻意设计为单线程的。这样可以在没有普遍的细粒度锁的情况下保持房间状态的一致性。

### mediasoup Worker 进程 (mediasoup Worker Processes)

每个 `WorkerThread` 拥有一个 mediasoup worker 子进程的子集。

这些子进程：

- 创建路由器 (routers)
- 创建传输通道 (transports)
- 协商媒体
- 转发 RTP 数据
- 通过 IPC 暴露统计信息

### 注册中心工作线程 (Registry Worker)

Redis 的“阅后即焚” (fire-and-forget) 维护任务不会内联执行在房间控制路径上。一个专用的注册中心工作线程负责处理：

- 房间 TTL 刷新
- 房间注销
- 节点心跳 / 负载更新

这使得稳态房间控制路径对 Redis 延迟不那么敏感。

## Thread Model

| Thread | Count | Role |
|---|---:|---|
| uWS main | 1 | WebSocket, HTTP, timers, room dispatch |
| WorkerThread | N | serial room logic + epoll-driven worker IPC |
| Registry worker | 1 | async Redis maintenance tasks |
| Redis subscriber | 1 | pub/sub cache updates |
| Worker waiter | per worker | child process wait / death handling |
| Recorder | per active recorder | UDP RTP receive + muxing |

Typical small deployment:

- 1 uWS main thread
- 1 registry worker
- 1 Redis subscriber
- 1 WorkerThread
- 1 mediasoup worker process
- 0..M recorder threads

## Startup Sequence

All critical modules are initialized before the server begins accepting traffic:

1. `RoomRegistry`: Redis connect, `syncAll()`, subscriber thread
2. `WorkerThread::start()`: create worker processes
3. `waitReady()`: block until WorkerThreads report initialization complete
4. `uWS::App().listen()`: only then begin accepting WebSocket / HTTP connections

## 会话身份模型 (Session Identity Model)

本项目区分了以下两种身份：

- 业务身份：`peerId`
- 连接身份：`sessionId`

这在处理重连（reconnect）时非常重要：

- 当同一个 `peerId` 发起新的 join 请求时，会替换旧的 session
- 旧的 socket 会被立即失效
- `Peer` 对象会被打上新的 `sessionId` 标记
- 那些来自被替换连接的过时请求，会在试图修改房间状态前被拒绝

## 数据流 (Data Flow)

### 加入房间 (Join)

```text
client -> WebSocket join
      -> uWS 主线程 (main thread)
      -> 分配 roomId -> 绑定到某个 WorkerThread
      -> WorkerThread 执行 RoomService::join()
      -> 如果需要，RoomManager 创建房间
      -> 将响应结果 deferred 传回主循环
      -> socket 被绑定 roomId / peerId / sessionId
```

### 发布媒体 (Produce)

```text
client -> produce
      -> uWS 主线程
      -> 分发给该房间绑定的 WorkerThread
      -> 校验 session
      -> RoomService::produce()
      -> Transport::produce()
      -> Channel::requestWait()
      -> mediasoup-worker 创建 Producer
      -> 自动给其他 peer 订阅 (auto-subscribe)
      -> (可选) 启动录制链路
      -> 响应结果 deferred 传回客户端
```

### 统计 / 服务质量 (Stats / QoS)

```text
定时器 (timer) -> 主线程
      -> 往每个 WorkerThread 投递 stats 任务
      -> WorkerThread 遍历名下的 rooms / peers
      -> 收集 transport / producer / consumer 级别的 stats
      -> 合并 clientStats
      -> 广播 statsReport
      -> 录制器 (recorder) 可能会追加写入 QoS snapshot
```

### Linux 纯传输客户端 (PlainTransport Linux Client)

```text
linux plain-client
      -> WebSocket join / plainPublish
      -> 往 PlainTransport 发送 UDP RTP
      -> RTCP SR / RR / NACK / PLI
      -> 辅助 async getStats 进行采样
      -> 每个视频轨独立的 PublisherQosController
      -> 采集 clientStats snapshots
      -> 接收 qosPolicy / qosOverride 通知
```

### Media

The media plane does not pass through the signaling logic after setup:

```text
Browser A ──SRTP/UDP──→ WebRtcTransport → Producer
                                            ├──→ Consumer (SIMPLE) → WebRtcTransport → Browser B
                                            └──→ Consumer (PIPE) → PlainTransport
                                                    │ localhost UDP RTP
                                                    ▼
                                              PeerRecorder → .webm
```

## Room / Peer Model

### Room

A room owns:

- a `Router`
- a peer map
- room activity timestamps

### Peer

A peer owns:

- `peerId`
- `displayName`
- `sessionId`
- RTP capabilities
- send transport
- recv transport
- producers
- consumers

### Auto-Subscribe

The room model is room-first rather than explicit per-subscription signaling.

When one peer produces:

- all other peers with a recv transport are auto-subscribed
- they receive `newConsumer` notifications

## Redis / Room Ownership

`RoomRegistry` is responsible for:

- node registration
- node load publication
- room ownership claim
- room ownership refresh
- room lookup / resolution
- room and node cache maintenance

Important behavior:

- room ownership is stored in Redis
- room and node info are cached locally
- pub/sub keeps cache warm
- `resolveRoom()` can fall back to a Redis-backed node refresh if local cache is missing fresh node data
- when Redis is unavailable, the system degrades to local-only mode

## Geo Routing

If `GeoRouter` is enabled:

- client IP is mapped via `ip2region`
- same-country routing can be enforced
- candidate nodes are ranked by:
  - country isolation
  - ISP affinity
  - geographic distance
  - current load

This logic lives under `RoomRegistry`, not directly under `RoomService`.

### Country Isolation

Enabled by default.

- Chinese IP -> Chinese nodes only
- US IP -> US nodes only

Disable with `--noCountryIsolation` or `"countryIsolation": false`.

### Example

```bash
# Hangzhou node (China Telecom)
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --announcedIp=<public-ip> \
  --lat=30.27 \
  --lng=120.15 \
  --isp=电信 \
  --country=中国

# US West node
./build/mediasoup-sfu \
  --nodaemon \
  --port=3001 \
  --announcedIp=<public-ip> \
  --lat=37.39 \
  --lng=-122.08 \
  --isp=Amazon \
  --country="United States"
```

## Recording

Recording is implemented as a side path:

```text
Producer
  -> PIPE Consumer
  -> PlainTransport
  -> localhost UDP RTP
  -> PeerRecorder
  -> FFmpeg / WebM output
```

Recorder responsibilities include:

- RTP receive
- H264 / VP8 packet handling
- muxing
- QoS timeline sidecar output

## Quick Start

### Prerequisites

Dependency reference:

- [docs/dependencies_cn.md](./docs/dependencies_cn.md)

- Linux
- CMake 3.16+
- GCC 10+ or Clang 12+
- OpenSSL
- zlib
- FFmpeg
  - `libavformat`
  - `libavcodec`
  - `libavutil`
  - `libswscale`
  - `libavdevice`
- hiredis
- `curl` 与 `tar`（被 `setup.sh` 用于下载和解压 `mediasoup-worker`）

备注：

- 对于单节点、仅本地路由（local-only）的模式，Redis 在运行时是可选的，但目前的默认构建依然会链接 `hiredis`。
- 目前的 `plain-client` 线程化路径直接引入了 `libavdevice`，因此 `libavdevice-dev` / `ffmpeg-devel` 也是构建要求的一部分，而不仅仅是事后为了支持摄像头才需要的。

### 构建 (Build)

```bash
git clone --recursive https://github.com/user/mediasoup-cpp.git
cd mediasoup-cpp
./setup.sh
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### 运行 (Run)

请始终在项目根目录下运行。

最简运行：

```bash
./build/mediasoup-sfu --nodaemon --port=3000 --listenIp=0.0.0.0
```

推荐的生产环境启动参数：

```bash
./build/mediasoup-sfu \
  --nodaemon \
  --port=3000 \
  --workers=1 \
  --workerThreads=1 \
  --listenIp=0.0.0.0 \
  --announcedIp=<public-ip> \
  --workerBin=./mediasoup-worker \
  --redisHost=127.0.0.1 \
  --redisPort=6379 \
  --nodeId=<unique-node-id>
```

使用配置文件：

```bash
cat > config.json <<'EOF'
{
  "port": 3000,
  "workers": 1,
  "workerThreads": 1,
  "workerBin": "./mediasoup-worker",
  "listenIp": "0.0.0.0",
  "announcedIp": "<public-ip>",
  "redisHost": "127.0.0.1",
  "redisPort": 6379,
  "recordDir": "./recordings",
  "logDir": "/var/log/mediasoup",
  "logPrefix": "mediasoup-sfu",
  "logRotateHours": 3
}
EOF

./build/mediasoup-sfu --nodaemon --config=config.json
```

打开浏览器访问 `http://<server-ip>:3000`。

## 命令行选项 (Command-Line Options)

| 选项 (Option) | 默认值 (Default) | 描述 (Description) |
|---|---|---|
| `--port` | `3000` | 信令 + HTTP 端口 |
| `--workers` | 基于 CPU 自动计算 | mediasoup worker 子进程数量 |
| `--workerThreads` | 自动计算 | WorkerThread 事件循环数量 |
| `--listenIp` | `0.0.0.0` | 传输层 (transport) 监听 IP |
| `--announcedIp` | 自动检测 | 用于 ICE 候选者 (candidates) 的公网 IP |
| `--workerBin` | `./mediasoup-worker` | worker 可执行文件路径 |
| `--recordDir` | `./recordings` | 录制文件输出目录 |
| `--logDir` | `/var/log/mediasoup` | 守护进程日志目录 |
| `--logPrefix` | `mediasoup-sfu` | 守护进程日志文件前缀 |
| `--logLevel` | `info` | 日志详细级别 |
| `--logRotateHours` | `3` | 每 N 小时轮转守护进程日志，生成如 `mediasoup-sfu_2026041306_<pid>.log` 的文件 (`0` 为禁用轮转) |
| `--nodaemon` | flag | 在前台运行 (不作为守护进程) |
| `--redisHost` | `127.0.0.1` | Redis 主机地址 |
| `--redisPort` | `6379` | Redis 端口 |
| `--nodeId` | 自动生成 | 节点唯一标识 |
| `--nodeAddress` | 自动生成 | 对外公布的 WS 地址 |
| `--lat` | 自动检测 | 节点纬度 |
| `--lng` | 自动检测 | 节点经度 |
| `--isp` | 自动检测 | 节点所属运营商 (ISP) |
| `--country` | 自动检测 | 节点所在国家 |
| `--countryIsolation` | on | 仅限同国家路由 |
| `--noCountryIsolation` | flag | 禁用国家隔离 |
| `--geoDb` | `./ip2region.xdb` | ip2region 数据库路径 (回退路径为 `./third_party/ip2region/ip2region.xdb`) |

如果当前目录下不存在 `./ip2region.xdb`，服务器还会检查打包的源码副本路径 `./third_party/ip2region/ip2region.xdb` 以及可执行文件构建目录下的副本。

## 重要的部署注意事项 (Important Deployment Notes)

### 1. 生产环境中明确设置 `announcedIp`

代码具备尽力而为的自动检测能力，但生产环境部署不应依赖于此。

请明确设置：

- `--announcedIp`
- 可选设置 `--nodeId`
- 如果你的环境具有特殊的路由或代理拓扑，可选设置 `--nodeAddress`

### 2. 在仓库根目录下运行测试

集成测试依赖于在 `./build` 目录下使用相对路径来生成并启动二进制文件。

### 3. 多节点模式需要可达的节点地址

如果启用了基于 Redis 的多节点路由，每个节点都必须发布一个能够被客户端或上游代理层访问到的 `ws://` 地址。

## 测试 (Testing)

所有的测试必须在项目的根目录执行。

```bash
# 全仓库回归测试
./scripts/run_all_tests.sh

# QoS JS / 测试套件 / 矩阵回归
./scripts/run_qos_tests.sh

# 单独的二进制测试文件
./build/mediasoup_tests
./build/mediasoup_qos_unit_tests
./build/mediasoup_review_fix_tests
./build/mediasoup_stability_integration_tests
./build/mediasoup_multinode_tests
./build/mediasoup_topology_tests
./build/mediasoup_integration_tests
./build/mediasoup_qos_integration_tests
./build/mediasoup_e2e_tests
./build/mediasoup_bench
```

`mediasoup_review_fix_tests`、`mediasoup_multinode_tests` 以及 `mediasoup_topology_tests` 会自动启动一个隔离的 `redis-server`。这需要环境变量 `PATH` 中有 `redis-server` 可执行文件，但它们并不依赖 `127.0.0.1:6379` 上共享的 Redis。

`./scripts/run_all_tests.sh` 和 `./scripts/run_qos_tests.sh` 都在有测试失败后继续运行剩余被选定的测试组，只有在打印最终的失败总结后才返回非零状态码。

`./scripts/run_all_tests.sh` 还会覆写最新的全量回归测试报告：
[docs/full-regression-test-results.md](./docs/full-regression-test-results.md)

对于无人值守的夜间执行，可以使用仓库本地的包装脚本：

```bash
cp .nightly-full-regression.env.example .nightly-full-regression.env
./scripts/nightly_full_regression.py run
./scripts/install_nightly_full_regression_cron.sh
```

夜间包装脚本会在 `artifacts/nightly-full-regression/` 下保存带有时间戳的运行目录，默认情况下刷新 `/var/log/run_all_tests.log`，并通过邮件发送通过率/失败用例总结以及精选的 Markdown 报告附件。
详见 [docs/nightly-full-regression.md](./docs/nightly-full-regression.md)。

当运行范围包含 `qos` 时，该切面会被委托给 `./scripts/run_qos_tests.sh`，因此该脚本所负责的特定 QoS 总结和矩阵制品也会作为运行的一部分被刷新。

目前回归重载套件涵盖：

- 重连语义 (reconnect semantics)
- 过时请求拒绝 (stale request rejection)
- restartIce 正确性
- 非阻塞统计路径 (non-blocking stats path)
- 录制路径稳定性
- 地理位置路由 (geo routing)
- 国家隔离 (country isolation)
- Redis 降级模式 (Redis degrade mode)
- 缓存传播 (cache propagation)
- full-node 重定向行为

### 快速质量关卡 (Quick Quality Gate)

供 Review 前本地验证使用：

```bash
# 配置并构建
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# 全仓库回归测试
./scripts/run_all_tests.sh

# 快速基线 (核心单元测试)
./build/mediasoup_tests

# QoS 单元测试基线
./build/mediasoup_qos_unit_tests
```

### QoS 测试入口 (QoS Test Entry)

对于 QoS 特定的验证，请使用统一脚本：

```bash
cd /root/mediasoup-cpp

# 全量 QoS 运行
./scripts/run_qos_tests.sh

# 仅从上一次失败的任务继续
./scripts/run_qos_tests.sh --resume

# 跳过依赖浏览器的部分
./scripts/run_qos_tests.sh --skip-browser

# 运行指定的组
./scripts/run_qos_tests.sh client-js cpp-unit

# 默认浏览器矩阵关卡
node tests/qos_harness/run_matrix.mjs

# 扩展的浏览器矩阵 (追加剩余的基线用例)
node tests/qos_harness/run_matrix.mjs --include-extended

# 针对性重跑盲点
node tests/qos_harness/run_matrix.mjs --cases=T9,T10,T11

# 多房间容量压力测试:
# 每个房间有刚好 2 个 peer：1 个 publisher 发送 1080p，1 个 subscriber 接收
node tests/qos_harness/browser_capacity_rooms.mjs --workers=1 --step=5 --max-rooms=50
```

行为表现：

- 默认模式会运行所有的 QoS 组，即使某个组失败也会继续执行
- 失败会记录到 `tests/qos_harness/artifacts/last-failures.txt`
- `--resume` 仅会重新运行上一次失败的精确任务
- 如果执行了 `matrix`，脚本还会重新生成逐 Case 的报告：
  [docs/uplink-qos-case-results.md](./docs/uplink-qos-case-results.md)
- 默认矩阵现在包含盲点过渡用例 `T9/T10/T11`；剩余的 `extended` 集合是目前更高带宽的基线校准用例，可以通过 `--include-extended` 追加，或者通过 `--cases=...` 明确指定运行

### 排障 (Troubleshooting)

- `Cannot find source file ... third_party/ip2region/binding/c/xdb_searcher.c`  
  确保你在最新的分支上，并且捆绑的 `third_party/ip2region` 目录存在。
- `Could NOT find ... avformat/avcodec/avutil`  
  安装 FFmpeg 开发包（比如在 Debian/Ubuntu 上安装 `libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libavdevice-dev`），或者参阅 [docs/dependencies_cn.md](./docs/dependencies_cn.md)。
- `hiredis not found` 或 Redis 符号的链接错误  
  安装 hiredis 开发包 (`libhiredis-dev` / `hiredis-devel`)，或者参阅 [docs/dependencies_cn.md](./docs/dependencies_cn.md)。

## 监控 (Monitoring)

生产环境的监控位于 `deploy/monitoring` 下。

### 快速开始

```bash
cd deploy/monitoring
docker compose up -d
```

### 面板 (Dashboards)

| 服务 (Service) | URL |
|---|---|
| Grafana | `http://<server-ip>:3001` |
| Prometheus | `http://<server-ip>:9090` |
| Alertmanager | `http://<server-ip>:9093` |

在线演示 Grafana: **http://47.99.237.234:3001**

## 性能 (Performance)

在 Intel Xeon Platinum 2.5GHz, 2 vCPU 上的测试结果：

| 指标 (Metric) | 本地回环 (Loopback) | 真实网络 (Real Network) |
|---|---:|---:|
| 峰值房间数 (每个含 1推 + 2拉) | 240 | 80 |
| Worker CPU | 82% | 23% |
| RSS (内存驻留) | 180 MB | 67 MB |
| PPS (包速率 in -> out) | 72k -> 144k | 24k -> 48k |

真实环境预估：对于典型的音视频 WebRTC 流量，每个 mediasoup worker 大约能承载 **30-40** 个 1v1 房间。

## 项目结构 (Project Structure)

```text
src/
├── main.cpp              # 瘦进程入口 + 信号接线
├── MainBootstrap.*       # 运行时选项、geo/初始化、worker 线程池创建
├── RuntimeDaemon.*       # 守护进程化 + 启动通知管道
├── Constants.h           # 运行时常量
├── SignalingServer.h     # 信令服务器外观 (facade)
├── SignalingServerWs.*   # WebSocket 请求/会话分发
├── SignalingServerHttp.* # HTTP 路由、指标、文件服务
├── SignalingServerRuntime.cpp # 运行时快照、注册中心 worker、房间分配助手
├── SignalingSocketState.h     # WS 会话 / 速率限制助手
├── SignalingRequestDispatcher.h # method -> RoomService 分发粘合
├── StaticFileResponder.h      # 静态文件路径解析 + 流式传输
├── WorkerThread.*        # 每个信令 worker 线程一个 epoll 事件循环
├── RoomService.h         # 房间服务外观 (facade)
├── RoomServiceLifecycle.cpp # join/leave/健康/清理
├── RoomServiceMedia.cpp  # transport / produce / consume 流程
├── RoomServiceStats.cpp  # stats / QoS / 房间状态广播
├── RoomServiceDownlink.cpp # 下行规划 + 发布端供应
├── RoomRecordingHelpers.* # 录制器创建 / 清理助手
├── RoomMediaHelpers.h    # 媒体侧辅助例程
├── RoomDownlinkHelpers.h # 下行辅助例程
├── RoomStatsQosHelpers.h # 统计/QoS 辅助例程
├── RoomManager.h         # 房间容器与生命周期
├── RoomRegistry.*        # Redis 路由、缓存、pub/sub 同步
├── GeoRouter.h           # 地理位置解析和打分
├── WorkerManager.h       # worker 选取 / 容量辅助
├── Worker.*              # mediasoup-worker 子进程包装器
├── Channel.*             # 基于 Unix 管道的 FlatBuffers IPC
├── Router.*              # 路由器包装器
├── Transport.*           # 传输通道包装器
├── WebRtcTransport.*     # ICE / DTLS 传输通道
├── PlainTransport.h      # 用于录制路径的 plain RTP 传输通道
├── Producer.*            # producer 包装器
├── Consumer.*            # consumer 包装器
├── Peer.h                # peer + session 状态
├── Recorder.h            # RTP -> WebM 录制与 QoS 时间线
├── EventEmitter.h        # 轻量级事件系统
└── Logger.h              # spdlog 包装器
```

## 许可证 (License)

MIT — 详见 [LICENSE](LICENSE).

## 致谢 (Acknowledgments)

- [mediasoup](https://mediasoup.org/)
- [uWebSockets](https://github.com/uNetworking/uWebSockets)
- [FlatBuffers](https://google.github.io/flatbuffers/)
- [nlohmann/json](https://github.com/nlohmann/json)
- [spdlog](https://github.com/gabime/spdlog)
- [FFmpeg](https://ffmpeg.org/)
- [ip2region](https://github.com/lionsoul2014/ip2region)
