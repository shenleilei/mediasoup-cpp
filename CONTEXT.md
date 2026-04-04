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
- `RoomManager.h`: Room/RoomManager 封装，支持空闲回收
- `RoomService.h` 的 `cleanIdleRooms()`: GC 空闲房间
- 前端 `public/index.html`: 自动重连、redirect 处理、auto-subscribe 支持

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
- P2: ✅ Worker 负载均衡（getLeastLoadedWorker 按 routerCount 选最少的 worker）
- P2: 待做 Dynacast
- P2: 待做 信令协议标准化

## 单元测试
- 框架: GoogleTest，`BUILD_TESTS` 默认 ON
- 构建: `cmake --build build --target mediasoup_tests`
- 运行: `./build/mediasoup_tests` 或 `cd build && ctest --output-on-failure`
- 测试文件在 `tests/` 目录: test_ortc / test_rtp_types / test_room / test_supported_capabilities
- 共 35 个用例，覆盖 ORTC 协商、RTP 类型序列化、Room/Peer 管理、codec 能力校验

## 关键文件
- `src/main.cpp` - 入口，参数解析，组装各组件
- `src/RoomService.h` - 核心业务逻辑
- `src/SignalingServer.cpp` - WebSocket 信令层
- `src/RoomManager.h` - Room/RoomManager
- `src/RoomRegistry.h` - Redis 多节点注册
- `src/Peer.h` - Peer 抽象
- `src/Router.cpp` - Router 封装（创建 transport、管理 producer）
- `src/Transport.cpp` - produce/consume 的 FBS 协议实现
- `src/ortc.h` - RTP 能力协商
- `public/index.html` - 测试前端页面
- `CMakeLists.txt` - 构建配置
