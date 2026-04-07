# mediasoup-cpp 工作上下文

## 项目概述
基于 mediasoup C++ worker 的 SFU 服务端，参考 livekit 的 Room-first 设计思路进行改造。

## 已完成的 P0 改造（3项）

### 1. Peer 抽象 + 服务端自动订阅
- `Peer.h`: 封装了 sendTransport/recvTransport/producers/consumers
- `RoomService.h` 的 `produce()`: 当 peer produce 时，自动为房间内其他有 recvTransport 的 peer 创建 consumer，并通过 `newConsumer` 通知推送

### 2. 信令与业务解耦
- `SignalingServer.cpp`: 只负责 WebSocket 收发和路由分发
- `RoomService.h`: 独立的业务层，包含 join/leave/createTransport/connectTransport/produce/consume 等方法
- 未来可以从 HTTP/gRPC 等其他入口调用同一套业务逻辑

### 3. Redis 多节点路由
- `RoomRegistry.h`: 基于 Redis 的房间到节点映射（claimRoom/refreshRoom/unregisterRoom）
- join 时如果发现房间在别的节点会返回 redirect
- `main.cpp`: 支持 --redisHost/--redisPort/--nodeId/--nodeAddress 参数

## 其他已完成
- `RoomManager.h`: Room/RoomManager 封装，支持空闲回收，getPeerIds/getRoomIds
- `RoomService.h` 的 `cleanIdleRooms()`: GC 空闲房间
- 前端 `public/index.html`: 自动重连、redirect 处理、auto-subscribe 支持、QoS Monitor 面板

## 已完成：QoS 实时监控
- Channel: OwnedNotification 透传完整 FlatBuffers 通知数据（解决之前传 nullptr 的问题）
- Producer: handleNotification 解析 PRODUCER_SCORE，getStats 获取 bitrate/jitter/RTT
- Consumer: 解析 CONSUMER_SCORE body，getStats 获取发送端统计
- Transport: getStats 支持 WebRtcTransport 和 PlainTransport
- RoomService: collectPeerStats 全链路采集，broadcastStats 每 2 秒广播
- SignalingServer: getStats 信令方法 + statsReport 推送定时器
- 前端 QoS Monitor 面板: 颜色编码 score，展示上行/下行比特率、丢包、ICE/DTLS 状态

## 已完成：录制 + QoS 时间线 + 回放
- PeerRecorder: 录制时同步写入 .qos.json 文件，时间与视频 PTS 对齐（第一个 RTP 包为零点）
- RoomService.broadcastStats: 广播 stats 时同步写入 recorder
- 默认录制目录: ./recordings（main.cpp 默认值）
- HTTP API: GET /api/recordings 列表 + GET /recordings/* 文件下载（含路径穿越防护）
- playback.html: 视频播放器 + QoS 面板随进度同步 + score 时间线条形图

## 已修复：录制链路问题
- PlainTransport listenIp: 录制用的 PlainTransport 改为 127.0.0.1（之前复用了 WebRTC 的公网 listenInfos，导致 RTP 包无法到达本地 Recorder）
- Video Payload Type: 从 router RTP capabilities 动态读取 VP8 的 mapped PT（之前硬编码 101，实际是 102）
- VP8 帧重组: 将多个 RTP 包的 VP8 payload 拼接为完整帧后再写入 WebM（之前每个 RTP 包单独写入导致 Invalid sync code）
- VP8 Payload Descriptor: 写入前剥离 VP8 RTP payload descriptor（RFC 7741）
- VP8 Keyframe 标记: 检测 VP8 keyframe 并设置 AV_PKT_FLAG_KEY，确保浏览器 seek 正常
- PTS 时基转换: 使用 av_rescale_q 将 RTP timestamp（90kHz/48kHz）转换为 stream time_base，修复视频时长显示异常
- QoS 文件 double-stop: Recorder::stop() 末尾清空 outputPath_ 防止析构函数二次调用时覆盖已写入的 QoS 数据
- playback.html QoS 容错: 处理未闭合的 JSON（SFU 异常退出时 .qos.json 可能缺少 ]）
- playback.html seek 同步: 添加 seeked 事件监听，确保 seek 后 QoS 面板更新

## 已修复：H264 录制支持
- H264 设为默认视频编码（main.cpp codec 顺序调整，浏览器普遍硬件加速 H264）
- H264 RTP 解包（RFC 6184）: 支持 Single NAL、FU-A 分片重组、STAP-A 聚合包
- STAP-A SPS/PPS 提取: 从 STAP-A 包中分离 SPS(7)/PPS(8) 单独缓存，不混入帧缓冲
- 延迟写 header: H264 matroska 容器需要 SPS/PPS extradata，initMuxer 时不写 header，等收到第一个 IDR 帧后从缓存的 SPS/PPS 构建 AVCC extradata 再写 header
- 音频缓冲: header 延迟期间 audio RTP 包缓存在 pendingAudio_，header 写入后一次性 flush
- Annex-B→AVCC 转换: matroska 容器要求 AVCC 格式（4字节 length prefix），flushVideoFrame 时将 Annex-B start code 转换为 AVCC
- 无 IDR 安全退出: 如果始终没收到 IDR，finalizeMuxer 跳过 av_write_trailer，不崩溃
- Codec 动态检测: RoomService 从 router RTP capabilities 读取 codec 类型和 PT，自动选择 H264 或 VP8

## 已修复：QoS 指标显示问题
- Jitter 单位: 之前直接显示 RTP timestamp 单位（如 36 显示为 "36 ms"），实际应除以 clockRate（video 90kHz / audio 48kHz）再转毫秒（36/90000*1000=0.4ms）
- Fraction Lost: 之前直接显示 0-1 浮点数，改为乘 100 加 % 显示
- Available BW ⬆: mediasoup 的 availableIncomingBitrate 在 Transport-CC 模式下永远为 0，改为浏览器上报的 candidate-pair.availableOutgoingBitrate（Transport-CC BWE）
- Available BW ⬇: 移除（mediasoup recvTransport 的 availableOutgoingBitrate 也为 null，BWE 在此方向不可用）
- 新增浏览器端 stats 上报: 前端每 2 秒采集 RTCPeerConnection.getStats() 中的 candidate-pair，通过 clientStats 信令上报，服务端合并到 QoS 数据
- 新增 RTT (browser): 显示浏览器 STUN RTT（补充 mediasoup RTCP RTT）
- playback.html 同步修复 jitter 单位和 Available BW

## 已修复：黑屏问题
- 根因：recvTransport 在 publish 时才创建，导致只 join 不 publish 的 peer 无法接收媒体
- 修复：前端 join 后立即创建 recvTransport 并消费已有 producer，不再依赖 publish 触发
- 服务端 `createTransport()` 创建 recvTransport 时补一轮 auto-subscribe，返回 `consumers` 数组

## 已完成：公网 IP 自动探测
- `announcedIp` 为空时，启动时自动通过 ifconfig.me / api.ipify.org / icanhazip.com 探测公网 IP
- 探测成功后自动设置 `announcedIp`，写入 ICE candidate，浏览器可直接连接
- 用户手动配置 `--announcedIp` 时跳过探测
- 探测失败打 warn 日志提醒

## 已完成：信令非阻塞（uWS 事件循环卸载）
- 问题: 之前所有 RoomService 方法（join/produce/consume 等）在 uWS 主线程同步执行，Channel::RequestWait() 会阻塞事件循环，worker 响应慢时所有 WS 连接的信令都会卡住
- 方案: SignalingServer 新增单工作线程 + 任务队列（生产者-消费者模式）
- uWS 主线程只做 JSON 解析和 postWork()，立即返回不阻塞
- 工作线程串行执行 RoomService 方法，Channel IPC 阻塞在此线程
- 完成后通过 uWS::Loop::defer() 回到主线程发送 WS 响应
- 单工作线程保证请求串行，无需改 RoomService/Room/Peer 的锁
- per-connection alive token (shared_ptr<atomic<bool>>) 防止 defer 回调向已关闭的 ws 发送
- notify/broadcast 回调也通过 defer() 回到 uWS 线程（线程安全）
- 定时器回调（GC/健康检测/stats广播）也卸载到工作线程
- clientStats 仍在主线程处理（只写 map，不涉及 Channel IPC）
- 关键: uWS::Loop::get() 是 thread-local 的，必须在 uWS 线程捕获 loop 指针传给工作线程

## 已完成：Daemon 模式 + 结构化日志
- 默认 daemonize 运行（fork + setsid），`--nodaemon` 前台运行
- daemon 模式日志写入文件（默认 /var/log/mediasoup-sfu.log），同时输出到 stdout
- PID 文件（默认 /var/run/mediasoup-sfu.pid）
- 所有业务日志统一 `[room:X Y]` 前缀（X=roomId, Y=peerId），方便 grep 定位问题
- 信令层每个请求记录 `[room:X Y] method id=N`
- join/disconnect 记录 info 级别日志
- Recorder 日志带 peerId 前缀

## 已完成：稳定性全面加固 (2026-04-06)

### 严重 bug 修复
- Channel::close() readThread detach→join: 修复 UAF（fd 关闭后 read 返回 0，join 安全）
- Worker waitThread 生命周期: 析构时检测是否在自身线程中，避免 std::terminate
- Worker::workerDied() 不调 channel->close(): 避免在 waitThread 中 join readThread 导致死锁
- Channel request/notify 释放 builderMutex 后再 sendBytes: 防止管道满时全局阻塞
- leave() 时从 Router 移除 peer 的 producers: 防止 stale shared_ptr 引用
- Recorder pendingAudio_ 加 kMaxPendingAudioPackets 上限: 防止 H264 无 IDR 时 OOM
- Transport/Producer/Consumer close 时 off channel listener: 修复 EventEmitter 内存泄漏
- signalHandler 只设 atomic flag: 不调非 async-signal-safe 函数，uWS timer 轮询后优雅关闭
- RTP 时间戳回绕: 用 int32 差值防止 pts 溢出

### 长时间运行稳定性
- Worker 崩溃自动重启: WorkerManager.respawnOne()，独立线程执行，带速率限制（kMaxRespawnsPerWindow/kRespawnWindowSec）
- Worker 崩溃通知客户端: checkRoomHealth() 每 2 秒检测 dead router，broadcast serverRestart 通知
- Redis 断连自动重连: ensureConnected() 守卫 + heartbeat 线程定期重试
- 录制磁盘空间保护: cleanOldRecordings() 超过 kMaxRecordingDirBytes 自动清理最老录制
- Recorder 空文件清理: finalizeMuxer 中无帧时删除空 webm
- GC 清理 recorder: cleanIdleRooms 同步清理 recorder/transport/clientStats
- collectPeerStats 超时保护: 每个 getStats IPC 调用 kStatsTimeoutMs 超时

### 代码整洁度
- Constants.h: 所有魔数提取为 kXx 命名常量（timer/超时/端口/限制值等）
- RoomService.h 拆分: .h 声明(90行) + .cpp 实现(583行)
- Recorder public/private 整理: 内部方法改为 private，测试通过 PeerRecorderTestAccess friend class
- broadcastStats 日志: 0 rooms 不打印，有 room 时打印房间名
- stats 广播改为 10s 间隔，health check 保持 2s 独立 timer
- cleanupRoomResources() 提取消除重复代码

### 测试
- 71 个单元测试: 含 Room 健康检测、EventEmitter 清理、Recorder 稳定性（pendingAudio cap/空文件清理/时间戳回绕）
- 2 个 Worker 崩溃恢复集成测试: WorkerCrashSendsServerRestart + WorkerRespawnAllowsNewRooms

## 线程模型
- 主线程(uWS): WebSocket 收发、JSON 解析、定时器触发，**不阻塞**
- 信令工作线程(1): 串行执行 RoomService 方法（join/produce/consume/leave/getStats 等），Channel::RequestWait() 阻塞在此线程，通过 Loop::defer() 回到主线程发送 WS 响应
- Channel readThread(×N worker): pipe读取、response匹配、notification分发
- Worker waitThread(×N worker): waitpid子进程
- Recorder recvLoop(×M peer): UDP收RTP、H264/VP8解包、写WebM/MKV
- Registry heartbeat(0或1): Redis心跳10s
- 典型1 worker + 2录制 = 7线程（含信令工作线程）
- 共享数据: Channel.pendingRequests_(锁)、Recorder.muxMutex_/qosMutex_(锁)
- 信令工作线程串行执行，RoomService/Room/Peer 无需额外加锁
- per-connection alive token (shared_ptr<atomic<bool>>) 防止向已关闭的 ws 发送

## 任务完成状态
- P0: ✅ Peer 抽象 + 服务端自动订阅
- P0: ✅ 信令与业务解耦
- P0: ✅ Redis 多节点路由 + 删除 PipeTransport
- P1: ✅ 客户端自动重连（doReconnect 已与新 join 流程对齐：rejoin → recvTransport → 消费已有 → 重建 sendTransport）
- P1: ✅ Room 空闲回收 + 节点健康检查（GC timer 已接入 cleanIdleRooms；RoomRegistry.claimRoom 检测死节点并接管 room）
- P1: ✅ QoS 实时监控（全链路 stats 采集 + 2 秒广播 + 前端面板）
- P1: ✅ 录制 + QoS 时间线 + 回放页面
- P2: ✅ Worker 负载均衡（getLeastLoadedWorker 按 routerCount 选最少的 worker）
- P2: ✅ 信令非阻塞（SignalingServer 工作线程卸载 Channel IPC，uWS 事件循环不再阻塞）
- P2: 待做 Dynacast
- P2: 待做 信令协议标准化
- P0: 待优化 virtio 单队列网络瓶颈（详见"网络瓶颈分析"），上线前必须换多队列 VM 或物理机

## 单元测试
- 框架: GoogleTest，`BUILD_TESTS` 默认 ON
- 构建: `cmake --build build --target mediasoup_tests`
- 运行: `./build/mediasoup_tests` 或 `cd build && ctest --output-on-failure`
- 测试文件在 `tests/` 目录
- 共 98 个单元测试用例，覆盖 ORTC 协商、RTP 类型、Room/Peer 管理、QoS 数据结构、Recorder 稳定性、EventEmitter 清理、Room 健康检测、Utils、WorkerQueue（任务队列 FIFO/并发/alive token）
- 集成测试: mediasoup_integration_tests (14) + mediasoup_qos_integration_tests (15) + mediasoup_e2e_tests (3) + mediasoup_topology_tests (6) + mediasoup_stability_integration_tests (10)
- 总计 122 个测试用例

## 关键规则
- 每次新增功能都必须同步补充 UT 和集成测试覆盖

## 单 Worker 性能基准（已测）
- 测试工具: `tests/bench_worker_load.cpp` → `mediasoup_bench`
- 场景: 每房间 1P+2C, PlainTransport, opus 1200字节 300pps（等效1080p带宽）
- 环境: Intel Xeon Platinum 2.5GHz, 2 vCPU
- loopback 结果: 240 rooms peak, 82% CPU(单核), 180MB RSS, 72k→144k pps
- 真实网络栈结果: 80 rooms peak, 23% CPU(单核), 67MB RSS, 24k→48k pps
- 真实场景估算（WebRTC+SRTP+audio+video双流）: 单Worker约30-40个1v1房间
- 关键发现: Worker每房间CPU约0.33%线性扩展; 走真实网络栈时内核softirq是瓶颈(90rooms就丢包,Worker CPU才26%)
- 方法学: std::atomic计数器(relaxed), 动态PT从router获取, 48kHz时间戳步进, SendRate%列校验发送速率≥95%才计入峰值
- 注意: video PipeConsumer需要有效keyframe才转发，PlainTransport无法完成keyframe协商，因此用opus替代
- 已知局限: UdpDrops是/proc/net/udp全局口径; 两个Consumer均为PIPE类型(无BWE/NACK); PlainTransport无SRTP开销
- 热路径优化: sendFrame预计算dest sockaddr, epoll_data.ptr直存fd+counter指针(零查表)

## 网络瓶颈分析（virtio 单队列）
- 内核参数调优无效: net.core.netdev_budget 翻倍到 600、netdev_budget_usecs 翻倍到 4000，峰值仍为 80 rooms
- softnet_stat 证据: 测试期间 time_squeeze 增加约 3 万次/核，说明 softirq 仍处理不过来
- 根因: virtio_net 单队列，所有包的硬中断→NAPI poll 路径只能在单核串行处理，RPS 只分散 backlog 不分散 poll
- 多队列可解决: 多队列让网卡有 N 个独立 ring buffer 绑不同 CPU 核，收包 softirq 并行化
- 多队列不需要同步增加 Worker: 多队列是内核层面的收包并行化，Worker 数量由 CPU 瓶颈决定
- 物理机估算: 8 核物理机 + 万兆网卡 16 队列，8 Worker 保守 240 个 1v1 房间，总 pps 约 216k，万兆网卡不到 10% 负载
- 结论: 当前网络瓶颈是 virtio 虚拟化特有问题，非架构性问题；上物理机或多队列 VM 后瓶颈回到 Worker CPU，多 Worker 水平扩展即可

## 关键文件
- `src/main.cpp` - 入口，参数解析，组装各组件
- `src/Constants.h` - 全局常量定义（kXx 命名，timer/超时/端口/限制值）
- `src/RoomService.h` - 核心业务逻辑声明
- `src/RoomService.cpp` - 核心业务逻辑实现（join/leave/produce/QoS采集/录制/健康检测）
- `src/SignalingServer.cpp` - WebSocket 信令层 + 录制回放 HTTP API
- `src/RoomManager.h` - Room/RoomManager
- `src/RoomRegistry.h` - Redis 多节点注册（含自动重连）
- `src/WorkerManager.h` - Worker 管理（负载均衡 + 崩溃自动重启）
- `src/Peer.h` - Peer 抽象
- `src/Router.cpp` - Router 封装（创建 transport、管理 producer）
- `src/Transport.cpp` - produce/consume/getStats 的 FBS 协议实现
- `src/Channel.h/cpp` - Worker pipe 通信（OwnedResponse/OwnedNotification）
- `src/Worker.h/cpp` - Worker 进程管理（fork/exec/崩溃检测）
- `src/Producer.h/cpp` - Producer（score 追踪、getStats）
- `src/Consumer.h/cpp` - Consumer（score 追踪、getStats）
- `src/Recorder.h` - 录制（RTP→WebM + QoS 时间线）
- `src/ortc.h` - RTP 能力协商
- `public/index.html` - 实时通话页面（含 QoS Monitor）
- `public/playback.html` - 录制回放页面（视频 + QoS 时间线）
- `tests/test_stability.cpp` - 稳定性单元测试
- `tests/test_qos_unit.cpp` - QoS 单元测试
- `tests/test_integration.cpp` - 集成测试（含 Worker 崩溃恢复）
- `tests/bench_worker_load.cpp` - Worker 并发压测工具（1P+2C, PlainTransport, 逐步加压）
- `tests/test_worker_queue.cpp` - 信令工作线程单元测试（FIFO/并发/alive token/阻塞隔离）
