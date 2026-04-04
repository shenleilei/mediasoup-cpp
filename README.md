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
│   ├── index.html                 # 测试页面
│   └── mediasoup-client.bundle.js # mediasoup-client 3.7.17 浏览器 bundle
├── src/                           # 核心源码 (~3400 行)
│   ├── main.cpp                   # 入口，配置解析，启动流程
│   ├── Channel.h/cpp              # Worker pipe 通信层
│   ├── Worker.h/cpp               # Worker 进程管理 (fork/exec)
│   ├── WorkerManager.h            # 多 Worker 管理，负载均衡 (least routers)
│   ├── Router.h/cpp               # Router 管理，创建 Transport
│   ├── Transport.h/cpp            # Transport 基类，produce/consume
│   ├── WebRtcTransport.h/cpp      # WebRTC Transport (ICE/DTLS)
│   ├── Producer.h/cpp             # 媒体生产者
│   ├── Consumer.h/cpp             # 媒体消费者
│   ├── ortc.h                     # ORTC 逻辑 (codec 协商/映射)
│   ├── RtpTypes.h                 # RTP 数据结构 + JSON 序列化
│   ├── supportedRtpCapabilities.h # mediasoup 支持的 codec 列表
│   ├── Peer.h                     # Peer 抽象 (transport/producer/consumer)
│   ├── RoomManager.h              # Room/RoomManager 封装，空闲回收
│   ├── RoomService.h              # 业务逻辑层 (join/leave/produce/auto-subscribe)
│   ├── RoomRegistry.h             # Redis 多节点路由 + 死节点接管
│   ├── SignalingServer.h/cpp      # WebSocket 信令 + HTTP 静态文件
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

## 已知限制和待改进

### 当前限制
1. **单线程信令**: uWebSockets 事件循环是单线程的，Channel 的 FlatBufferBuilder 在信令线程使用但没有和读线程完全隔离
2. **Notification 数据未传递**: Channel 的 notification handler 只传递 event 枚举，不传递 notification body 数据（已做 null 检查避免 crash，但丢失了详细信息）
3. **无 HTTPS 支持**: 当前只有 HTTP，生产环境需要 TLS（uWebSockets 支持 `uWS::SSLApp`）
4. **Worker 二进制版本锁定**: 当前绑定 mediasoup-worker v3.14.16，升级需要同步更新 FBS schema

### 待实现
1. **Dynacast**: 按需发送视频层，减少带宽消耗
2. **信令协议标准化**: 定义统一的 request/response/notification schema

### 建议改进
1. 将 Notification body 数据拷贝后传递给 handler，而不是传 nullptr
2. 添加 HTTPS/WSS 支持
3. 实现 graceful shutdown（当前用 SIGTERM + detach）
4. 添加 Worker 健康检查和自动重启
5. 升级到最新 mediasoup-worker 版本（需要更新系统 glibc 或从源码编译）

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
| `--announcedIp` | (空) | 公网 IP（NAT 环境必须设置） |
| `--redisHost` | 127.0.0.1 | Redis 地址（多节点模式） |
| `--redisPort` | 6379 | Redis 端口 |
| `--nodeId` | hostname:port | 当前节点 ID |
| `--nodeAddress` | (自动生成) | 当前节点 WebSocket 地址 |

### 浏览器测试
1. 访问 `http://<服务器IP>:<端口>/`
2. 点击 "Join Room" 加入房间
3. 点击 "Publish A/V" 发布音视频
4. 在另一个标签页重复以上步骤，两端可互相看到视频

## 技术栈

| 组件 | 技术 | 用途 |
|------|------|------|
| 序列化 | FlatBuffers | Worker 通信协议 |
| WebSocket/HTTP | uWebSockets + uSockets | 信令和静态文件服务 |
| JSON | nlohmann/json | 信令消息和配置 |
| 日志 | spdlog | 结构化日志 |
| 媒体处理 | mediasoup-worker (C++) | RTP/RTCP/ICE/DTLS |
