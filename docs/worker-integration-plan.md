# mediasoup-worker 源码集成计划

> worker 版本：mediasoup v3.14.6
> 源码位置：`src/worker/`
> 日期：2026-04-10

---

## 1. 总体规划（三步走）

### 第一步：从源码编译 worker，替换预编译二进制

- 方案：保留独立进程架构（方案A），worker 代码完整保留不精简
- worker 源码已放置在 `src/worker/`
- 目标：从源码编译出 `mediasoup-worker`，替换项目根目录的预编译二进制
- 验证：启动 mediasoup-sfu 确认功能正常

### 第二步：吃透 worker QoS 核心机制

- 深入理解流媒体 QoS 弱网对抗的实现细节
- 重点：带宽估计（GoogCC）、NACK 重传、Simulcast 层切换、质量评分
- 在理解基础上评估可优化的点

### 第三步：与客户端配合，制定上下行联动 QoS 策略

- 当前 mediasoup 的 QoS 是被动式的（依赖 RTCP 反馈）
- 目标：制定更主动的上下行配合策略
- 方向：上行主动上报、下行差异化、上下行联动

---

## 2. Worker 源码架构概览

### 2.1 入口与启动流程

```
main.cpp
  ├── 检查 MEDIASOUP_VERSION 环境变量
  └── 调用 mediasoup_worker_run()

lib.cpp (mediasoup_worker_run)
  ├── DepLibUV::ClassInit()
  ├── new Channel::ChannelSocket()   // IPC 通道（fd 3/4 或回调函数）
  ├── Logger::ClassInit()
  ├── Settings::SetConfiguration()
  ├── 初始化静态依赖:
  │   ├── DepOpenSSL, DepLibSRTP, DepUsrSCTP
  │   ├── DepLibUring (Linux kernel >= 6)
  │   ├── DepLibWebRTC, Utils::Crypto
  │   └── RTC::DtlsTransport, RTC::SrtpSession
  └── new Worker(channel)
      ├── 设置 SignalHandle (SIGINT/SIGTERM)
      ├── 创建 RTC::Shared (ChannelMessageRegistrator + ChannelNotifier)
      ├── 发送 WORKER_RUNNING 通知
      └── DepLibUV::RunLoop()  // 进入事件循环
```

### 2.2 IPC 通道

两种模式：

**进程模式（当前使用）**
- 父进程 fork 子进程，通过 pipe 通信
- fd 3：子进程读（ConsumerSocket），父进程写（producerFd）
- fd 4：子进程写（ProducerSocket），父进程读（consumerFd）
- 消息格式：4字节长度前缀 + FlatBuffers 序列化的 `FBS::Message`
- 消息类型：Request（请求/响应）、Notification（通知）、Log（日志）

**库模式（备选，未来可用）**
- 通过 `channelReadFn`/`channelWriteFn` 回调 + `uv_async_t` 通知
- worker 跑在调用方进程的一个线程里，无需 fork/pipe
- 官方 Rust binding 使用此模式，稳定性有保障

### 2.3 模块分类

| 分类 | 文件 | 说明 |
|------|------|------|
| 入口/框架 | `main.cpp`, `lib.cpp`, `Worker.cpp`, `Settings.cpp` | 启动、配置、顶层管理 |
| IPC | `Channel/*`, `ChannelMessageRegistrator.cpp` | 请求/响应/通知路由 |
| 路由 | `RTC/Router.cpp` | Producer→Consumer 路由 |
| Transport 基类 | `RTC/Transport.cpp` | 带宽分配、RTCP 处理、QoS 核心 |
| Transport 子类 | `WebRtcTransport`, `PlainTransport`, `PipeTransport`, `DirectTransport` | 各类传输 |
| Producer | `RTC/Producer.cpp` | 接收端（上行） |
| Consumer | `Consumer`, `SimulcastConsumer`, `SvcConsumer`, `SimpleConsumer`, `PipeConsumer` | 发送端（下行） |
| RTP 流 | `RtpStream`, `RtpStreamRecv`, `RtpStreamSend`, `RtxStream` | 流状态管理 |
| QoS - BWE | `TransportCongestionControlClient/Server` | 带宽估计 |
| QoS - NACK | `NackGenerator`, `RtpRetransmissionBuffer` | 丢包重传 |
| QoS - 辅助 | `RateCalculator`, `TrendCalculator`, `SenderBandwidthEstimator`, `SeqManager` | 速率/趋势/序列号 |
| ICE/DTLS/SRTP | `IceServer`, `StunPacket`, `DtlsTransport`, `SrtpSession` | WebRTC 安全层 |
| RTCP | `RTC/RTCP/*` (27个文件) | RTCP 包解析/构建 |
| Codecs | `RTC/Codecs/*` (H264, VP8, VP9, Opus, H264_SVC) | 编解码器头解析 |
| 网络句柄 | `handles/*` | libuv 封装（UDP/TCP/Timer/Signal/UnixStream） |
| 依赖初始化 | `Dep*.cpp` | OpenSSL/SRTP/SCTP/UV/WebRTC/Uring |
| 工具 | `Utils/*` | Crypto/IP/String/File |

### 2.4 当前 mediasoup-cpp 控制面与 worker 的关系

```
mediasoup-cpp (控制面)                    mediasoup-worker (媒体面)
┌─────────────────────┐                  ┌─────────────────────┐
│ SignalingServer      │                  │ Worker              │
│ RoomService          │                  │   ├── Router(s)     │
│ WorkerThread         │   pipe + FBS     │   │   ├── Transport │
│   ├── Worker.cpp ────┼──────────────────┼───│   ├── Producer  │
│   ├── Channel.cpp    │   fd 3 (write→)  │   │   ├── Consumer  │
│   ├── Router.cpp     │   fd 4 (←read)   │   │   └── ...      │
│   └── Transport.cpp  │                  │   └── WebRtcServer  │
└─────────────────────┘                  └─────────────────────┘
```

控制面通过 Channel 发送 FlatBuffers 请求（创建 Router/Transport/Producer/Consumer 等），
worker 处理请求并通过 Channel 返回响应或推送通知（score 变化、ICE 状态等）。

---

## 3. QoS 核心机制详解

### 3.1 带宽估计（BWE）

支持两种模式（`RTC::BweType`）：

#### 3.1.1 Transport-CC + GoogCC（主要模式）

**发送端：`TransportCongestionControlClient`**

核心：包装了 libwebrtc 的 `GoogCcNetworkController`。

流程：
1. 每个发出的 RTP 包打上 `transport-wide-cc` 序列号（`transportWideCcSeq`）
2. 包信息注册到 `rtpTransportControllerSend->OnAddPacket()`
3. 发送完成后回调 `tccClient->PacketSent()` 记录发送时间
4. 收到对端的 Transport-CC RTCP Feedback 后，喂给 GoogCC：
   `rtpTransportControllerSend->OnTransportFeedback(*feedback)`
5. GoogCC 通过 `OnTargetTransferRate()` 回调输出可用带宽估计值
6. 内置 pacer（`RtpTransportControllerSend`）控制发送节奏
7. 支持 padding/probing 探测带宽上限

关键参数：
- `initialAvailableBitrate`: 600000 bps（默认起始带宽）
- `TransportCongestionControlMinOutgoingBitrate`: 30000 bps（最低带宽）
- `MaxBitrateIncrementFactor`: 1.35（最大带宽 = desiredBitrate × 1.35）
- `MaxPaddingBitrateFactor`: 0.85（padding 带宽 = maxBitrate × 0.85）
- `MaxBitrateMarginFactor`: 0.1（带宽变化 < 10% 时不调整，避免抖动）
- `AvailableBitrateEventInterval`: 1000ms（正常通知间隔）
- 带宽骤降 > 25% 或骤升 > 50% 时立即通知

**接收端：`TransportCongestionControlServer`**

职责：收集入站 RTP 包的到达时间，生成 Transport-CC Feedback 发回给发送端。

流程：
1. `IncomingPacket()` 记录每个包的 wideSeqNumber → arrivalTimestamp
2. 定时器每 100ms 触发 `FillAndSendTransportCcFeedback()`
3. 构建 `FeedbackRtpTransportPacket`，包含包序列号和到达时间差
4. 通过 RTCP 发回给发送端

关键参数：
- `TransportCcFeedbackSendInterval`: 100ms
- `PacketArrivalTimestampWindow`: 500ms（超过此窗口的旧记录被清理）

#### 3.1.2 REMB（备选模式）

- 接收端用 `RemoteBitrateEstimatorAbsSendTime` 基于 abs-send-time 扩展头估计入站带宽
- 通过 RTCP REMB 包告知发送端
- `LimitationRembInterval`: 1500ms

### 3.2 带宽分配：`Transport::DistributeAvailableOutgoingBitrate()`

当 BWE 输出新的可用带宽时触发，按 Consumer 优先级分配带宽。

算法：
```
1. 收集所有 Consumer 及其 priority（priority > 0 的参与分配）
2. availableBitrate = tccClient->GetAvailableBitrate()
3. 第一轮（base allocation）：
   - 从高优先级到低优先级遍历
   - 每个 Consumer 调用 IncreaseLayer()，每次只升一层
   - 扣减 usedBitrate
4. 后续轮：
   - 高优先级 Consumer 可以多升几层（priority 次）
   - 继续扣减
5. 循环直到带宽用完或没有 Consumer 需要更多
6. 所有 Consumer 调用 ApplyLayers() 应用计算结果
```

### 3.3 Simulcast 层切换：`SimulcastConsumer::IncreaseLayer()`

逐层尝试升级 spatial/temporal 层：

```
1. 如果已在 preferredLayer，返回 0（不需要更多带宽）
2. 根据丢包率调整虚拟可用带宽：
   - 丢包 < 2%：virtualBitrate = bitrate × 1.08（放大 8%）
   - 丢包 > 10%：virtualBitrate = bitrate × (1 - 0.5 × lossPercentage/100)
   - 其他：virtualBitrate = bitrate
3. 从低到高遍历 spatial layer：
   - BWE 降级保守期检查（lastBweDowngradeAtMs + BweDowngradeConservativeMs）
   - 流活跃时间检查（StreamMinActiveMs）
   - 逐个 temporal layer 检查 requiredBitrate
   - simulcast 模式下扣减当前层的带宽
4. requiredBitrate <= virtualBitrate 则设置 provisionalTarget
5. ApplyLayers() 中如果 target 变化则 UpdateTargetLayers()
   - 如果是降级且当前层 <= preferredLayer，记录 lastBweDowngradeAtMs
```

### 3.4 NACK 丢包恢复：`NackGenerator`

检测丢包并请求重传：

```
1. ReceivePacket() 检测序列号间隙
2. 间隙中的包加入 nackList（seq → NackInfo）
3. 两种触发方式：
   - SEQ filter：收到新包时立即发送（sendNackDelayMs = 0 时）
   - TIME filter：定时器触发，基于 RTT 控制重传间隔
4. 重传次数超限后请求关键帧（OnNackGeneratorKeyFrameRequired）
5. RTT 通过 UpdateRtt() 更新，影响重传间隔
```

### 3.5 质量评分：`RtpStreamRecv::UpdateScore()`

0-10 分制，每个 RtpStream 独立计算：

```
公式：
  lost = expected - received（本区间）
  repairedRatio = repaired / received
  repairedWeight = (1 / (repairedRatio + 1))^4 × (repaired / retransmitted)
  effectiveLost = lost - repaired × repairedWeight
  deliveredRatio = (received - effectiveLost) / received
  score = round(deliveredRatio^4 × 10)

含义：
  - 10 分：完美，无丢包
  - 7-9 分：轻微丢包，NACK 修复效果好
  - 4-6 分：中等丢包
  - 0-3 分：严重丢包
  - 0 分：流不活跃（inactivityCheckPeriodicTimer 触发）

特点：
  - deliveredRatio^4 使得评分对丢包非常敏感（5% 丢包 ≈ 8 分，10% ≈ 6.5 分）
  - NACK 修复的包会提升评分，但有衰减权重
  - RTX 修复效率（repaired/retransmitted）影响权重
```

### 3.6 Jitter 计算

标准 RFC 3550 算法：

```
transit = (nowMs × clockRate / 1000) - rtpTimestamp
d = transit - prev_transit
jitter += (1/16) × (|d| - jitter)
```

指数移动平均，1/16 的平滑因子。

### 3.7 丢包率统计

Client 和 Server 都维护 `packetLossHistory`（滑动窗口，24 个样本）：

```
加权平均算法：
  weight = 1, 2, 3, ..., N（越新权重越大）
  packetLoss = Σ(weight_i × loss_i) / Σ(weight_i)
```

### 3.8 关键常量汇总

| 常量 | 值 | 位置 | 说明 |
|------|-----|------|------|
| `initialAvailableOutgoingBitrate` | 600000 bps | Transport.hpp | 起始带宽 |
| `TransportCongestionControlMinOutgoingBitrate` | 30000 bps | TCC Client | 最低带宽 |
| `MaxBitrateIncrementFactor` | 1.35 | TCC Client | 最大带宽倍数 |
| `MaxPaddingBitrateFactor` | 0.85 | TCC Client | padding 带宽比例 |
| `MaxBitrateMarginFactor` | 0.1 | TCC Client | 带宽变化忽略阈值 |
| `AvailableBitrateEventInterval` | 1000ms | TCC Client | BWE 通知间隔 |
| `TransportCcFeedbackSendInterval` | 100ms | TCC Server | Feedback 发送间隔 |
| `PacketArrivalTimestampWindow` | 500ms | TCC Server | 到达时间窗口 |
| `LimitationRembInterval` | 1500ms | TCC Server | REMB 限制间隔 |
| `PacketLossHistogramLength` | 24 | TCC Client/Server | 丢包历史窗口 |
| `RateCalculator::DefaultWindowSize` | 1000ms | RateCalculator | 速率计算窗口 |
| `RtpDataCounter windowSize` | 2500ms | RateCalculator | RTP 数据计数窗口 |
| `TrendCalculator::DecreaseFactor` | 0.05/s | TrendCalculator | 趋势衰减因子 |

---

## 4. 第一步详细计划：从源码编译 worker

### 4.1 构建系统

- 构建工具：**meson + ninja**
- 入口：`src/worker/Makefile` → `tasks.py`（pip invoke）→ meson
- 所有依赖通过 meson wrap 系统自动下载源码编译，不依赖系统库

### 4.2 依赖列表

| 依赖 | 版本 | wrap 文件 | 说明 |
|------|------|-----------|------|
| openssl | 3.0.8 | `subprojects/openssl.wrap` | TLS/DTLS/加密 |
| libuv | 1.48.0 | `subprojects/libuv.wrap` | 事件循环 |
| libsrtp2 | - | `subprojects/libsrtp2.wrap` | SRTP 加密 |
| usrsctp | - | `subprojects/usrsctp.wrap` | SCTP（DataChannel） |
| abseil-cpp | 20230802.1 | `subprojects/abseil-cpp.wrap` | flat_hash_map 等容器 |
| flatbuffers | - | `subprojects/flatbuffers.wrap` | FBS 序列化 |
| catch2 | - | `subprojects/catch2.wrap` | 测试框架 |
| liburing | - | `subprojects/liburing.wrap` | io_uring（kernel >= 6） |
| libwebrtc | 内置 | `deps/libwebrtc/` | GoogCC/BWE 算法 |

### 4.3 编译步骤

```bash
# 1. 安装构建工具
pip3 install meson ninja

# 2. 进入 worker 目录
cd src/worker

# 3. 编译（自动下载依赖 + 编译）
make mediasoup-worker

# 编译产物在: src/worker/out/Release/build/mediasoup-worker
```

如果 `make` 方式有问题，可以手动执行 meson：

```bash
cd src/worker

# 手动 setup
meson setup --buildtype release out/Release

# 编译
ninja -C out/Release mediasoup-worker

# 产物: out/Release/mediasoup-worker
```

### 4.4 替换和验证

```bash
# 备份旧的
cp mediasoup-worker mediasoup-worker.bak

# 替换
cp src/worker/out/Release/mediasoup-worker ./mediasoup-worker
chmod +x mediasoup-worker

# 验证版本（应该输出 "you don't seem to be my real father!" 因为没设环境变量）
./mediasoup-worker

# 启动 mediasoup-sfu 验证功能
./build/mediasoup-sfu \
  --workers=2 \
  --workerBin=./mediasoup-worker \
  --port=3003 \
  --announcedIp=<YOUR_IP>
```

### 4.5 潜在问题和注意事项

1. **网络问题**：subproject 源码需要从 GitHub/wrapdb 下载，机器可能需要代理
   - 可以手动下载 wrap 文件指定的 tar.gz 放到 `subprojects/packagecache/`
2. **编译时间**：首次编译较长（openssl、abseil-cpp 等从源码编译），预计 10-30 分钟
3. **kernel 版本**：Linux kernel >= 6 会自动启用 liburing，需要额外下载 liburing 源码
4. **meson 版本**：需要 >= 1.1.0
5. **编译后的二进制是动态链接的**：依赖系统 glibc，确保部署环境兼容

---

## 5. 第二步初步思路：深入 QoS

### 5.1 重点研究模块（按优先级）

1. **TransportCongestionControlClient** — GoogCC 带宽估计 + pacer
   - 文件：`src/RTC/TransportCongestionControlClient.cpp`
   - 重点：`OnTargetTransferRate()` 回调、`MayEmitAvailableBitrateEvent()` 通知策略
   - 依赖：`deps/libwebrtc/` 中的 GoogCC 实现

2. **SimulcastConsumer** — 下行层选择
   - 文件：`src/RTC/SimulcastConsumer.cpp`
   - 重点：`IncreaseLayer()`、`ApplyLayers()`、BWE 降级保守期

3. **Transport::DistributeAvailableOutgoingBitrate** — 多 Consumer 带宽分配
   - 文件：`src/RTC/Transport.cpp` (L2215-L2300)
   - 重点：优先级机制、layer-by-layer 迭代

4. **NackGenerator** — 丢包重传策略
   - 文件：`src/RTC/NackGenerator.cpp`
   - 重点：SEQ/TIME filter、RTT 自适应、关键帧请求阈值

5. **RtpStreamRecv::UpdateScore** — 质量评分
   - 文件：`src/RTC/RtpStreamRecv.cpp` (L729-L845)
   - 重点：评分公式、NACK 修复权重

6. **RtpRetransmissionBuffer** — 重传缓冲区
   - 文件：`src/RTC/RtpRetransmissionBuffer.cpp`
   - 重点：缓冲区大小策略、包淘汰机制

### 5.2 关键代码路径

**下行 RTP 包发送路径（QoS 生效的完整链路）：**
```
Producer::ReceiveRtpPacket()
  → Router 路由到对应 Consumer
  → SimulcastConsumer::SendRtpPacket()
    → 层过滤（spatial/temporal layer 检查）
    → Consumer::Listener::OnConsumerSendRtpPacket()
      → Transport::OnConsumerSendRtpPacket()
        → 打 transport-wide-cc 序列号
        → tccClient->InsertPacket() (注册到 pacer)
        → SendRtpPacket() (实际发送)
        → 回调中 tccClient->PacketSent() (记录发送时间)
```

**BWE 反馈处理路径：**
```
Transport::ReceiveRtcpPacket()
  → HandleRtcpPacket()
    → RTCP Transport-CC Feedback:
      → tccClient->ReceiveRtcpTransportFeedback()
        → rtpTransportControllerSend->OnTransportFeedback()
          → GoogCC 算法更新
          → OnTargetTransferRate() 回调
            → MayEmitAvailableBitrateEvent()
              → Transport::OnTransportCongestionControlClientBitrates()
                → DistributeAvailableOutgoingBitrate()
                → ComputeOutgoingDesiredBitrate()
    → RTCP Receiver Report:
      → tccClient->ReceiveRtcpReceiverReport()
      → 更新 RTT、fractionLost
```

**NACK 处理路径：**
```
上行（收到 NACK）：
  Transport::HandleRtcpPacket()
    → Producer::ReceiveNack()
      → RtpStreamSend 查找重传缓冲区
      → 重传 RTP 包

下行（生成 NACK）：
  RtpStreamRecv::ReceivePacket()
    → NackGenerator::ReceivePacket()
      → 检测序列号间隙
      → OnNackGeneratorNackRequired()
        → Producer::SendRtcpPacket() (发送 NACK)
```

---

## 6. 第三步初步思路：上下行联动 QoS

### 6.1 当前 mediasoup QoS 的局限性

1. **被动式**：server 只根据 RTCP 反馈做反应，延迟至少一个 RTT
2. **上下行独立**：上行质量差时，下行不知道；下行拥塞时，上行不调整
3. **层切换滞后**：BWE 降级后有保守期，升级也需要多轮迭代
4. **无客户端协作**：客户端的网络状态（WiFi/4G 切换、信号强度等）server 无法感知
5. **评分是结果不是预测**：score 反映的是过去的质量，不能预测未来

### 6.2 可增强方向

#### 上行优化
- 客户端主动上报网络状态（网络类型、信号强度、RTT 趋势）
- server 根据上报提前调整：建议客户端降低编码码率/分辨率
- 不等 RTCP 反馈，通过 DataChannel 或自定义信令快速通知

#### 下行优化
- 每个 consumer 独立的网络画像（带宽、丢包率、RTT、jitter 的时间序列）
- 基于网络画像做预测性层切换，而不是等 BWE 事件
- 差异化策略：弱网用户优先保音频、降帧率而非降分辨率

#### 上下行联动
- 上行弱网时：server 主动通知其他下行 consumer 该 producer 可能质量下降
- 下行拥塞时：server 通知上行 producer 降低码率（减少 server 转发压力）
- 全局视角：server 汇总所有 transport 的网络状况，做全局最优分配

#### 实现路径
1. 先在 worker 中增加更细粒度的 QoS 数据采集和上报
2. 在控制面（mediasoup-cpp）中实现策略引擎
3. 通过信令通道与客户端协作
4. 逐步迭代，每个优化点独立验证效果

---

## 7. 客户端（mediasoup-client）QoS 机制分析

> 源码位置：`src/client/`（mediasoup-client v3.7.17，TypeScript）

### 7.1 客户端架构概览

```
Device
  └── Transport (send / recv)
        ├── Producer (上行：track → RTCRtpSender → server)
        ├── Consumer (下行：server → RTCRtpReceiver → track)
        └── Handler (Chrome111 / Firefox120 / Safari12 / ...)
              └── RTCPeerConnection
                    ├── RTCRtpSender (上行)
                    └── RTCRtpReceiver (下行)
```

### 7.2 客户端上行 QoS 控制点

客户端的 QoS 主要依赖浏览器 WebRTC 引擎内置能力，mediasoup-client 本身是一个薄封装层。

#### 7.2.1 Simulcast 编码配置

`Transport.produce()` 时通过 `encodings` 参数配置：

```typescript
// 典型 simulcast 配置
encodings: [
  { rid: 'r0', maxBitrate: 100000, scalabilityMode: 'L1T3' },  // 低
  { rid: 'r1', maxBitrate: 300000, scalabilityMode: 'L1T3' },  // 中
  { rid: 'r2', maxBitrate: 900000, scalabilityMode: 'L1T3' },  // 高
]
```

每个 encoding 可配置：
- `maxBitrate` — 最大码率
- `maxFramerate` — 最大帧率
- `scaleResolutionDownBy` — 分辨率缩放
- `scalabilityMode` — SVC 模式（如 L1T3 = 1 spatial + 3 temporal）
- `active` — 是否启用
- `priority` / `networkPriority` — 优先级
- `adaptivePtime` — 自适应打包时间（音频）
- `dtx` — 不连续传输（音频静音检测）

#### 7.2.2 Codec 选项

`Transport.produce()` 时通过 `codecOptions` 配置：

```typescript
codecOptions: {
  // 音频 Opus
  opusStereo: true,           // 立体声
  opusFec: true,              // 前向纠错（FEC）
  opusDtx: true,              // 不连续传输
  opusMaxPlaybackRate: 48000, // 最大播放采样率
  opusMaxAverageBitrate: 64000, // 最大平均码率
  opusPtime: 20,              // 打包时间（ms）
  opusNack: true,             // NACK 重传

  // 视频
  videoGoogleStartBitrate: 1000,  // 起始码率（kbps）
  videoGoogleMaxBitrate: 5000,    // 最大码率（kbps）
  videoGoogleMinBitrate: 100,     // 最小码率（kbps）
}
```

这些参数写入 SDP answer 的 fmtp 行，影响浏览器 WebRTC 引擎的编码行为。

#### 7.2.3 动态控制 API

- `producer.setMaxSpatialLayer(layer)` — 限制最大发送层
  - 实现：设置 `RTCRtpSender.setParameters()`，将高于 layer 的 encoding 设为 `active: false`
  - 用途：弱网时客户端主动降低上行层数

- `producer.setRtpEncodingParameters(params)` — 动态修改编码参数
  - 可修改 maxBitrate、maxFramerate、scaleResolutionDownBy 等
  - 应用到所有 encoding

- `producer.pause()` / `resume()` — 暂停/恢复发送
  - `disableTrackOnPause`: 设置 `track.enabled = false`
  - `zeroRtpOnPause`: 替换 sender track 为 null（完全停止 RTP）

#### 7.2.4 浏览器内置 QoS（不在 mediasoup-client 代码中，但实际生效）

浏览器 WebRTC 引擎自动处理：
- **上行 BWE**：浏览器内置 GoogCC，根据 server 返回的 Transport-CC Feedback 自动调整编码码率
- **RTCP 处理**：自动发送 Sender Report，处理 Receiver Report
- **FEC**：Opus inband FEC、视频 FlexFEC/UlpFEC
- **重传**：响应 server 的 NACK 请求，从 RTX 流重传丢失包
- **拥塞控制**：自动降低码率、帧率

### 7.3 客户端下行（Consumer 侧）

Consumer 侧 mediasoup-client 做的事情很少：
- 接收 server 推送的 RTP 流
- 浏览器自动发送 Transport-CC Feedback 给 server（server 用来做下行 BWE）
- 浏览器自动发送 NACK 请求丢失包的重传
- `consumer.pause()` / `resume()` — 暂停/恢复接收

### 7.4 上下行 QoS 数据流全链路

```
客户端上行:
  getUserMedia → track → RTCRtpSender
    → 浏览器编码（受 encodings/codecOptions 约束）
    → RTP 包 + transport-wide-cc 扩展头
    → 网络 → server

server 处理上行:
  Transport::ReceiveRtpPacket()
    → tccServer->IncomingPacket()  // 记录到达时间
    → 定时发送 Transport-CC Feedback → 客户端
    → Producer → RtpStreamRecv
      → NackGenerator（检测丢包 → 发 NACK → 客户端重传）
      → UpdateScore()（计算质量评分）
    → Router 路由到 Consumer(s)

server 下行:
  SimulcastConsumer::SendRtpPacket()
    → 层过滤（spatial/temporal）
    → Transport::OnConsumerSendRtpPacket()
      → 打 transport-wide-cc 序列号
      → tccClient->InsertPacket()
      → 发送 RTP → 网络 → 客户端

客户端下行:
  RTCRtpReceiver → track → 渲染
    → 浏览器自动发送 Transport-CC Feedback → server
    → server tccClient->ReceiveRtcpTransportFeedback()
      → GoogCC 更新可用带宽
      → DistributeAvailableOutgoingBitrate()
      → 调整 simulcast 层
```

### 7.5 当前上下行 QoS 的断裂点

1. **客户端上行状态 server 不知道**
   - 浏览器的 BWE 估计值、编码器实际输出码率、丢包率 — server 无法直接获取
   - server 只能通过 RtpStreamRecv 的 score 间接推断

2. **server 下行决策客户端不知道**
   - server 切换了 simulcast 层、降低了码率 — 客户端不知道原因
   - 客户端无法配合调整（比如降低上行码率来减轻 server 压力）

3. **无自定义信令通道**
   - mediasoup-client 没有内置的 QoS 信令机制
   - 上下行 QoS 数据交换只能通过 RTCP（延迟大、信息有限）

4. **客户端 QoS 控制是手动的**
   - `setMaxSpatialLayer`、`setRtpEncodingParameters` 需要应用层主动调用
   - 没有自动化的弱网适应策略
