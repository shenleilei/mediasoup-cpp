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

## 已修复：黑屏问题
- 根因：recvTransport 在 publish 时才创建，导致只 join 不 publish 的 peer 无法接收媒体
- 修复：前端 join 后立即创建 recvTransport 并消费已有 producer，不再依赖 publish 触发
- 服务端 `createTransport()` 创建 recvTransport 时补一轮 auto-subscribe，返回 `consumers` 数组

## 已完成：公网 IP 自动探测
- `announcedIp` 为空时，启动时自动通过 ifconfig.me / api.ipify.org / icanhazip.com 探测公网 IP
- 探测成功后自动设置 `announcedIp`，写入 ICE candidate，浏览器可直接连接
- 用户手动配置 `--announcedIp` 时跳过探测
- 探测失败打 warn 日志提醒

## 任务完成状态
- P0: ✅ Peer 抽象 + 服务端自动订阅
- P0: ✅ 信令与业务解耦
- P0: ✅ Redis 多节点路由 + 删除 PipeTransport
- P1: ✅ 客户端自动重连（doReconnect 已与新 join 流程对齐：rejoin → recvTransport → 消费已有 → 重建 sendTransport）
- P1: ✅ Room 空闲回收 + 节点健康检查（GC timer 已接入 cleanIdleRooms；RoomRegistry.claimRoom 检测死节点并接管 room）
- P1: ✅ QoS 实时监控（全链路 stats 采集 + 2 秒广播 + 前端面板）
- P1: ✅ 录制 + QoS 时间线 + 回放页面
- P2: ✅ Worker 负载均衡（getLeastLoadedWorker 按 routerCount 选最少的 worker）
- P2: 待做 Dynacast
- P2: 待做 信令协议标准化

## 单元测试
- 框架: GoogleTest，`BUILD_TESTS` 默认 ON
- 构建: `cmake --build build --target mediasoup_tests`
- 运行: `./build/mediasoup_tests` 或 `cd build && ctest --output-on-failure`
- 测试文件在 `tests/` 目录
- 共 51 个单元测试用例，覆盖 ORTC 协商、RTP 类型、Room/Peer 管理、QoS 数据结构、Recorder QoS 文件写入
- 集成测试: mediasoup_integration_tests (12) + mediasoup_qos_integration_tests (15) + mediasoup_e2e_tests (3) + mediasoup_topology_tests (6)
- 总计 87 个测试用例

## 关键规则
- 每次新增功能都必须同步补充 UT 和集成测试覆盖

## 关键文件
- `src/main.cpp` - 入口，参数解析，组装各组件
- `src/RoomService.h` - 核心业务逻辑（join/leave/produce/QoS采集/录制）
- `src/SignalingServer.cpp` - WebSocket 信令层 + 录制回放 HTTP API
- `src/RoomManager.h` - Room/RoomManager
- `src/RoomRegistry.h` - Redis 多节点注册
- `src/Peer.h` - Peer 抽象
- `src/Router.cpp` - Router 封装（创建 transport、管理 producer）
- `src/Transport.cpp` - produce/consume/getStats 的 FBS 协议实现
- `src/Channel.h/cpp` - Worker pipe 通信（OwnedResponse/OwnedNotification）
- `src/Producer.h/cpp` - Producer（score 追踪、getStats）
- `src/Consumer.h/cpp` - Consumer（score 追踪、getStats）
- `src/Recorder.h` - 录制（RTP→WebM + QoS 时间线）
- `src/ortc.h` - RTP 能力协商
- `public/index.html` - 实时通话页面（含 QoS Monitor）
- `public/playback.html` - 录制回放页面（视频 + QoS 时间线）
- `CMakeLists.txt` - 构建配置
- `tests/test_qos_unit.cpp` - QoS 单元测试
- `tests/test_qos_integration.cpp` - QoS + 录制集成测试
