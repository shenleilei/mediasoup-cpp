# mediasoup-cpp 开发文档

## 架构概述

基于 mediasoup C++ worker 的 SFU 服务端，Room-first 设计。控制面用 C++17 实现，媒体面复用 mediasoup-worker 预编译二进制（v3.14.6）。

仓库当前保留浏览器 / WebRTC demo 路径。根目录 `client/` 下的 WebRTC QoS plain push/play 原生客户端已经移除；不要把它和仍在使用的浏览器端 `src/client/lib/` 混淆。

## 线程模型

### 线程清单

| 线程 | 数量 | 阻塞方式 | 职责 |
|------|------|----------|------|
| uWS 主线程 | 1 | epoll_wait（非阻塞事件循环） | WebSocket 收发、HTTP、定时器、defer 回调、room dispatch |
| WorkerThread | ×N | epoll_wait（task queue + Channel IPC + timer） | 串行执行 RoomService 方法、Channel IPC、health/GC |
| Worker waiter | ×N worker | waitpid() | 等待子进程退出，触发 respawn |

典型部署：1 WorkerThread + 1 worker = **2-4 线程**（取决于辅助子线程和系统运行时）。

### 关键不变量

1. **同房间单线程**：room → WorkerThread 绑定在 uWS 主线程原子完成（dispatch 前），不在 defer 回调里。并发首 join 同房间保证落到同一线程。
2. **uWS 主线程不阻塞**：所有 Channel IPC 在 WorkerThread 执行，主线程只做 JSON 解析 + dispatch。
3. **旧连接不可写状态**：reconnect 时旧 socket 立即 `alive=false`；WorkerThread 执行前校验 `peer->sessionId`，不匹配返回 `"remote login"` 错误。

### Channel IPC 模式

Worker 以 `threaded=false` 创建。Channel 的 consumer fd 注册到 WorkerThread 的 epoll。IPC 调用使用 `requestWait()`（内部 poll consumer fd），**禁止使用 `request().get()`**（会死锁，因为没有独立 readThread pump fd）。

## 启动序列

```
1. WorkerThread::start() → createWorkers() (fork+exec worker)
2. waitReady() 阻塞直到所有 WorkerThread 初始化完成
3. uWS::App().listen() — 此时才开始接受连接
```

所有模块必须在 listen 之前就绪。

## 关闭序列

```
1. g_shutdown = true → uWS timer 检测到 → 关闭 listen socket → uWS loop 退出
2. server.run() 返回
3. WorkerThread::stop() — 关闭房间与 worker 资源
4. 主进程退出
```

## Session Identity

```
join 请求 → 主线程预分配 newSessionId = g_nextSessionId++
         → WorkerThread: rs->join() 成功后 peer->sessionId = newSessionId
         → defer: sd->sessionId = newSessionId, 旧 socket alive=false

非 join 请求 → 主线程捕获 sessionId
            → WorkerThread: 校验 peer->sessionId == sessionId
            → 不匹配 → 返回 {"error": "remote login"}
```

`g_nextSessionId` 从随机大数开始，避免重启后 ID 碰撞。

## Worker 管理

### Respawn

WorkerThread 检测到 worker 死亡（channel fd EOF）→ `onWorkerDied()` → 从 WorkerManager 移除旧 worker → 创建新 worker → 注册到 epoll。

**速率限制**：10 秒内最多 3 次 respawn。超限后放弃，日志报错。

### 负载均衡

`pickLeastLoadedWorkerThread()`：按 `roomCount()` 选最少的线程。跳过 `workerCount() == 0` 的死线程。

## SIGPIPE 防护

`main()` 启动时 `signal(SIGPIPE, SIG_IGN)`。`Channel::sendBytes()` 写失败后调用 `close()` 标记 channel 死亡，`requestWait()` 立即失败而不是等 5 秒超时。

## 公网 IP 探测

启动时自动探测公网 IP：

1. 先尝试 curl 探测（2 个 URL × 2 秒超时 = 最坏 4 秒）
2. 再尝试 UDP connect trick（`connect(8.8.8.8:53)` + `getsockname`，零网络开销）
3. 仍然失败则直接退出

当前版本不再接受手工传入的 `announcedIp` / `listenIp`。

## 服务端 PlainTransport

服务端仍保留 `plainPublish` / `plainSubscribe` 协议能力，它们属于 `RoomService` 的媒体接入路径，不依赖已移除的根目录原生客户端。

主路径：

```text
join
  -> plainPublish / plainSubscribe
  -> UDP PlainTransport RTP/RTCP
  -> RoomService / Router / Transport
  -> Producer / Consumer / QoS aggregation
```

## 数据流

### 信令流（produce 请求）

```
浏览器A ws.send({method:"produce"})
  → uWS 主线程: JSON 解析, alive 检查, assignRoom, wt->post()
  → WorkerThread: sessionId 校验 → RoomService.produce()
    → ORTC 协商 → Channel.requestWait() → pipe write → poll+read
    → auto-subscribe: 为其他 peer 创建 Consumer
  → loop->defer() → uWS 主线程 → ws.send(response)
  → loop->defer() → ws.send(newConsumer) → 其他浏览器
```

### 媒体流（不经过控制层）

```
浏览器A ──SRTP/UDP──→ WebRtcTransport → Producer
                                          ├──→ Consumer(SIMPLE) → WebRtcTransport → 浏览器B
                                          └──→ Consumer(PIPE) → PlainTransport
                                                  │ UDP 127.0.0.1
                                                  ▼
```

## 测试

### 测试矩阵

| 套件 | 内容 |
|------|------|
| mediasoup_tests | ORTC、RTP 类型、Room/Peer、geo、request timeout、review fixes、基础稳定性单测 |
| mediasoup_qos_unit_tests | uplink/downlink QoS 单测、downlink planner / demand / supply 单测 |
| mediasoup_integration_tests | 黑盒：真实 SFU + WebSocket 客户端 |
| mediasoup_review_fix_tests | 重连、geo 路由、country isolation、session identity |
| mediasoup_stability_integration_tests | close/disconnect 各种时序 |
| mediasoup_qos_integration_tests | stats 采集、广播、policy / override、downlink / publisher supply、plainPublish 约束 |
| mediasoup_e2e_tests | 多 peer 发布/订阅压力 |
| mediasoup_qos_accuracy_tests | QoS 精度（真实 UDP 收发） |
| mediasoup_bench | Worker 并发压测 |

### 运行

```bash
# 必须从项目根目录运行（worker binary 用相对路径）
cd /path/to/mediasoup-cpp

# 单元测试
./build/mediasoup_tests
./build/mediasoup_qos_unit_tests

# 集成测试
./build/mediasoup_integration_tests
./build/mediasoup_review_fix_tests
./build/mediasoup_stability_integration_tests
./build/mediasoup_qos_integration_tests
./build/mediasoup_e2e_tests

# run_all_tests.sh / run_qos_tests.sh 遇到单个测试失败后会继续跑剩余选中项，
# 最终统一输出失败汇总并返回非 0。
# run_all_tests.sh 同时会刷新 docs/full-regression-test-results.md，
# 记录本次仓库全量回归的选择项和逐任务结果。
./scripts/run_all_tests.sh unit integration
./scripts/run_all_tests.sh non-qos

# nightly 全量回归包装器：
#  - 仍然委托 scripts/run_all_tests.sh 做真实执行
#  - 生成 artifacts/nightly-full-regression/<timestamp>/ 运行目录
#  - 默认刷新 /var/log/run_all_tests.log
#  - 可通过 scripts/install_nightly_full_regression_cron.sh 安装 03:00 cron
#  - 具体配置见 docs/nightly-full-regression.md

# 快速验证：核心 unit + QoS unit
./build/mediasoup_tests
./build/mediasoup_qos_unit_tests
```

## 关键规则

1. **Channel IPC 只用 `requestWait()`**，禁止 `request().get()`（非线程模式死锁）
2. **主线程和 WorkerThread 的状态变更必须保持单线程所有权**
3. **Room dispatch 在 dispatch 前绑定**，不在 defer 里
4. **每次新增功能必须补测试**
5. **`signal(SIGPIPE, SIG_IGN)` 必须在 main 开头**

## 关键文件

```
common/
└── ffmpeg/
    ├── AvError.{h,cpp}        # FFmpeg 错误码/错误字符串 helper
    ├── AvPtr.h                # FFmpeg 句柄 unique_ptr deleter / packet/frame helper
    ├── AvTime.h               # timebase / pts helper
    ├── InputFormat.{h,cpp}    # demux input open / stream info / read packet
    ├── OutputFormat.{h,cpp}   # mux output alloc / io / header / trailer / write packet
    ├── Decoder.{h,cpp}        # decoder open / send packet / receive frame
    ├── Encoder.{h,cpp}        # encoder open / send frame / receive packet
    └── BitstreamFilter.{h,cpp} # BSF alloc / init / send / receive
└── media/
    ├── rtp/
    │   ├── RtpHeader.h            # 共享 RTP header parse helper
    │   └── H264Packetizer.{h,cpp} # 共享 H264 RTP packetizer（sink callback，无 I/O）
    └── h264/
        ├── AnnexB.{h,cpp}         # 共享 Annex-B NAL reader
        └── Avcc.{h,cpp}           # 共享 Annex-B → AVCC 转换
src/
├── main.cpp                  # 控制编排入口（依赖 common/ffmpeg + common/media）
├── MainBootstrap.{h,cpp}     # 参数解析、geo/bootstrap、WorkerThread 池创建
├── RuntimeDaemon.{h,cpp}     # daemonize、启动状态通知
├── Constants.h               # 所有常量（kXx 命名）
├── SignalingServer.h         # signaling server facade
├── SignalingServerWs.{h,cpp} # WebSocket 请求、session、stale 请求保护
├── SignalingServerHttp.{h,cpp} # HTTP 路由、metrics、静态文件
├── SignalingServerRuntime.cpp # runtime snapshot、dispatch helper
├── SignalingSocketState.h    # ws/session/rate-limit helper
├── SignalingRequestDispatcher.h # method -> RoomService dispatch
├── StaticFileResponder.h     # 静态文件 path resolve + streaming
├── WorkerThread.{h,cpp}      # epoll 事件循环：task queue + Channel IPC + worker 生命周期
├── RoomService.h             # 房间业务 facade
├── RoomServiceLifecycle.cpp  # join/leave/health/cleanup
├── RoomServiceMedia.cpp      # transport / produce / consume
├── RoomServiceStats.cpp      # peer stats collect / stats broadcast
├── RoomServiceQos.cpp        # QoS ingest / override / connection-quality / room-pressure
├── RoomServiceRegistryView.cpp # local resolve / node-load view
├── RoomServiceDownlink.cpp   # downlink ingest / planning / publisher supply
├── RoomMediaHelpers.h        # room-level media helper
├── RoomDownlinkHelpers.h     # downlink helper
├── RoomStatsQosHelpers.h     # stats / QoS helper
├── RoomCleanupHelpers.h      # room cleanup helper
├── RoomManager.h             # Room/Peer 生命周期、idle GC
├── GeoRouter.h               # IP 地理定位 + ISP 评分
├── WorkerManager.h           # Worker 池、负载均衡、respawn 限流
├── Channel.h/cpp             # Pipe IPC（FlatBuffers，request/response/notification）
├── Worker.h/cpp              # fork+exec worker 进程管理
├── Router.h/cpp              # Router：创建 transport、管理 producer
├── Transport.h/cpp           # produce/consume/getStats FlatBuffers 协议
├── WebRtcTransport.h/cpp     # ICE/DTLS transport
├── PlainTransport.h          # Plain RTP transport
├── Producer.h/cpp            # Producer（score 追踪、stats）
├── Consumer.h/cpp            # Consumer（score 追踪、stats）
├── Peer.h                    # Peer 状态（transports、producers、consumers、sessionId）
├── ortc.h                    # ORTC 协商（codec 匹配、RTP mapping）
├── EventEmitter.h            # 轻量事件系统
└── Logger.h                  # spdlog 封装
tests/
├── test_ortc.cpp                      # ORTC 协商
├── test_room.cpp                      # Room/Peer 管理
├── test_rtp_types.cpp                 # RTP 类型序列化
├── test_stability.cpp                 # EventEmitter 清理
├── test_worker_queue.cpp              # WorkerThread 任务队列
├── test_request_timeout.cpp           # Channel IPC 超时
├── test_review_fixes.cpp              # 历史 review 修复回归
├── test_common_media.cpp              # 共享 RTP/H264 helper 单测
├── test_qos_unit.cpp                  # QoS 数据结构
├── test_qos_accuracy.cpp             # QoS 精度（真实 UDP 收发）
├── test_geo_router.cpp               # 地理路由评分
├── test_integration.cpp              # 黑盒集成测试
├── test_review_fixes_integration.cpp # 重连/geo/cache/session 集成测试
├── test_stability_integration.cpp    # close/disconnect 集成测试
├── test_qos_integration.cpp          # QoS 集成测试
├── test_e2e_pubsub.cpp              # 端到端发布订阅
└── bench_worker_load.cpp              # Worker 压测
```

## 性能基准

测试环境：Intel Xeon Platinum 2.5GHz, 2 vCPU

| 指标 | Loopback | 真实网络 |
|------|----------|----------|
| 峰值房间（1P+2C） | 240 | 80 |
| Worker CPU | 82%（单核） | 23% |
| RSS | 180 MB | 67 MB |
| PPS（in→out） | 72k→144k | 24k→48k |

真实场景估算：单 Worker 约 **30-40 个 1v1 房间**。

网络瓶颈：virtio 单队列 VM 的 softirq 是瓶颈，非架构问题。多队列 NIC 或物理机可解决。

## 待做

- P1: `broadcastStats` 拆分为 per-room 任务（大房间场景防止 WorkerThread 饿死）
- P2: Dynacast
- P2: 信令协议标准化
- P0: 上线前换多队列 VM 或物理机（解决 virtio 网络瓶颈）
