# Linux PlainTransport C++ Client 架构

> 文档性质
>
> 这是一份当前有效的 Linux `PlainTransport C++ client` 架构说明。
> 它描述的是当前实现与当前主路径，不是历史 review 结论。
>
> 当前状态、TWCC 变化和结果入口请继续看：
> - [plain-client-qos-status.md](README.md)
> - [plain-client-architecture-thread-model_cn.md](architecture-thread-model_cn.md)
> - [plain-client-twcc-change-summary.md](twcc-change-summary.md)
> - [twcc-ab-test.md](twcc-ab-test.md)
> - [architecture_cn.md](../../platform/architecture_cn.md)

## 1. 角色与边界

仓库里的 Linux client 主要指：

- `client/main.cpp`
- `client/PlainClientApp.*`
- `client/PlainClientThreaded.cpp`
- `client/ThreadedQosRuntime.*`
- `client/NetworkThread.h`
- `client/SenderTransportController.h`
- `client/RtcpHandler.h`
- `client/qos/*`

它不是浏览器 client 的替代品，而是仓库内一条独立的 `PlainTransport` 发布端路径，主要用于：

- `PlainTransport` 端到端验证
- Linux 侧 QoS 策略闭环验证
- `cpp-client-harness` / `cpp-client-matrix` / `cpp-client-ab` 自动化回归

它和浏览器发送端的核心差异是：

- 信令仍走 WebSocket JSON
- 媒体不走浏览器 WebRTC sender，而是本地 FFmpeg + RTP/UDP
- plain-client 自己维护 RTP / RTCP / transport-cc / send-side estimate 主路径
- sender QoS 控制输入由 `本地 transport 指标 + RTCP RR` 构成
- server `getStats` 只用于观测、对账和测试，不再进入 sender QoS 控制链

## 2. 当前运行时结论

当前 plain-client 只有一条运行时主路径：

- threaded 主路径
  - `plain-client` 默认且唯一支持的运行时入口
  - 这是当前带 `NetworkThread` / transport control / TWCC send-side BWE 的主口径

### 2.1 执行单元

| 执行单元 | 数量 | 当前职责 |
|---|---:|---|
| control / main thread | 1 | session bootstrap、QoS controller、WS 业务分发、source/network 命令路由、`clientStats` 上报 |
| `WsClient` reader thread | 1 | 异步接收 WebSocket response / notification，写入本地队列 |
| `NetworkThread` | 1 | UDP socket owner、RTP/RTCP、pacing、重传、transport-cc rewrite、TWCC feedback、send-side estimate、probe 生命周期 |
| audio worker | `0..1` | 音频 decode / Opus encode / 音频 AU 生产 |
| video `SourceWorker` | 每视频源 1 个 | 视频输入、decode/scale/encode、关键帧命令执行、encoded AU 生产 |

### 2.2 所有权结论

当前 threaded plain-client 的关键所有权边界是：

- `PlainClientApp` 负责 bootstrap 和 session shell
- `ThreadedQosRuntime` 负责 threaded sender runtime 生命周期
- control thread 是 QoS / signaling / session owner
- `NetworkThread` 是唯一 transport owner
- `SourceWorker` 负责视频输入和编码，不直接拥有网络发送语义
- audio worker 只产出音频 AU，最终发送仍归 `NetworkThread`

这意味着当前 plain-client 已经不应再被描述成“main thread 自己推进全部媒体循环”的单路径模型。

## 3. 模块分层

```text
Session Bootstrap
  ├─ main.cpp
  └─ PlainClientApp.*

Control / QoS Runtime
  ├─ PlainClientThreaded.cpp
  ├─ ThreadedQosRuntime.*
  ├─ ThreadedControlHelpers.h
  ├─ qos/*
  └─ PlainClientSupport.*

Transport Execution
  ├─ NetworkThread.h
  ├─ SenderTransportController.h
  ├─ TransportCcHelpers.h
  ├─ UdpSendHelpers.h
  └─ RtcpHandler.h

Send-Side BWE / Probe
  ├─ sendsidebwe/*
  └─ ccutils/*

Media Source Workers
  ├─ SourceWorker.h
  └─ common/ffmpeg/* + common/media/*

Signaling
  └─ WsClient.*
```

对应关系可以概括成：

- `PlainClientApp` 负责 bootstrap / session，`PlainClientThreaded.cpp` 只负责进入 threaded runtime
- `ThreadedQosRuntime` 负责 queue / worker / controller / sampling / coordination / snapshot upload
- control thread 决定“发什么、怎么降级、什么时候采样”
- `NetworkThread` 决定“什么时候发、怎么 pacing、怎么处理 RTCP/TWCC”
- source worker 决定“如何把输入变成 encoded access unit”

## 4. 启动与发布链路

当前主路径可以概括成：

```text
open inputs
  -> connect WebSocket
  -> join(roomId, peerId)
  -> plainPublish(videoSsrcs, audioSsrc)
  -> 服务端返回 {ip, port, videoTracks[], audioPt, transportCcExtId...}
  -> 建立 UDP socket
  -> 创建 NetworkThread
  -> registerVideoTrack(...)
  -> sendComediaProbe()
  -> 启动 SourceWorker / audio worker
  -> 进入 threaded control loop
```

`plainPublish` 这一步现在不只返回传统的 `ssrc / pt / producerId`，还会返回 plain-client 发送端需要的 transport-cc header extension id。

当前 threaded runtime 的重要环境开关：

- `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER`
- `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE`
- `PLAIN_CLIENT_VIDEO_TRACK_COUNT`
- `PLAIN_CLIENT_VIDEO_SOURCES`

## 5. 数据面与 transport-control 链路

### 5.1 视频与音频数据流

视频主路径：

```text
SourceWorker
  -> encoded access unit
  -> per-track AU queue
  -> NetworkThread
  -> SenderTransportController / sendMediaPacketWithTransportCc
  -> UDP
```

音频主路径：

```text
audio worker
  -> encoded audio AU
  -> audio AU queue
  -> NetworkThread
  -> SenderTransportController
  -> UDP
```

### 5.2 `NetworkThread` 的职责

`NetworkThread` 当前负责：

- UDP socket 所有权
- RTP 发送与 packetization glue
- RTCP RR / SR / NACK / PLI / FIR 处理
- transport-cc header extension rewrite
- TWCC feedback 解析
- send-side estimate 发布
- probe cluster 启动、发包、early-stop、finalize
- sender white-box stats 输出

当前主路径的关键点是：

- 只有 `NetworkThread` 可以真正决定 RTP 什么时候写 socket
- source worker 不直接调用 `send()`
- QoS 动作要么变成 source 命令，要么变成 network-thread transport hint

### 5.3 `SenderTransportController`

`SenderTransportController` 是当前发送执行层边界，负责：

- packet class 区分
  - `Control`
  - `AudioRtp`
  - `VideoRetransmission`
  - `VideoMedia`
- `Sent / WouldBlock / HardError` 分类
- byte-budget pacing
- per-class retain / drop / metrics
- shutdown drain / discard 语义

它暴露的计数器也是当前 plain-client 白盒可观测面的重要组成部分，例如：

- `wouldBlockByClass`
- `queuedVideoRetentions`
- `audioDeadlineDrops`
- `retransmissionDrops`
- `retransmissionSent`

### 5.4 transport-cc 与 send-side BWE

当前 threaded 主路径已经把 TWCC 接入到真实发送链路：

- 根据服务端返回的 `transportCcExtId` 重写 RTP header extension
- queued packet 在本地 `WouldBlock` 重试时复用同一个 transport-wide sequence
- 只有真正发送成功的包才进入 sent-packet tracker
- RTCP transport-cc feedback 直接进入 send-side BWE 路径
- published estimate 会反向约束 `SenderTransportController` 的 effective pacing bitrate

新增模块位于：

- `client/sendsidebwe/*`
- `client/ccutils/*`

当前签收口径是：

- send-side BWE 主路径已接入
- probe 生命周期已接入
- carrier-track 选择和 probe goal 使用 sender-side equivalent mapping

accepted behavior 见：

- [specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

## 6. QoS 与反馈链路

### 6.1 每 track 一个 controller

每个视频 track 对应一个 `PublisherQosController`，但 controller 不再直接操作 RTP 发送调用栈。

当前主路径是：

```text
local transport stats + RTCP RR
  -> per-track CanonicalTransportSnapshot
  -> per-track PublisherQosController::onSample()
  -> actionSink
     - source command
     - network transport hint
  -> peer-level clientStats snapshot
  -> clientStats
```

### 6.2 server-side observation

plain-client 仍然会异步请求服务端 `getStats`，但用途已经收敛为：

- producer / peer 观测与对账
- `clientStats` 聚合后的 server-side 可见性
- harness / matrix / debug 验证

sender QoS 主控制链使用的仍然是本地 transport snapshot。该 snapshot 还会暴露 transport 白盒字段，例如：

- `senderUsageBitrateBps`
- `transportEstimatedBitrateBps`
- `effectivePacingBitrateBps`
- `transportCcFeedbackReports`
- probe / queue / retransmission counters

### 6.3 notification 与动作落地

客户端继续消费：

- `qosPolicy`
- `qosOverride`

动作落地规则保持不变，但执行载体已经分层：

| QoS action | 当前落点 |
|---|---|
| `SetEncodingParameters` | source worker 编码参数命令 |
| `EnterAudioOnly` | source/video suppression + network transport hint |
| `ExitAudioOnly` | 解除 suppression，并触发关键帧恢复 |
| `PauseUpstream` | 停止对应视频发送并同步 transport hint |
| `ResumeUpstream` | 恢复发送，并触发关键帧恢复 |

## 7. 结果与回归入口

当前 plain-client 相关的稳定入口包括：

- 当前状态：
  - [plain-client-qos-status.md](README.md)
- TWCC 汇总：
  - [plain-client-twcc-change-summary.md](twcc-change-summary.md)
- TWCC A/B 说明：
  - [twcc-ab-test.md](twcc-ab-test.md)
- 最新稳定 A/B 报告：
  - [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
- current accepted behavior：
  - [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

当前默认全量 QoS 入口还会刷新 stable TWCC A/B 报告，因此 TWCC 主路径已经进入常规回归面，而不是一次性的 change artifact。

## 8. 多 track 模型与非目标

当前多视频 track 规则保持不变：

- `PLAIN_CLIENT_VIDEO_TRACK_COUNT` 控制视频 track 数
- `PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS` 控制 peer 内预算权重
- 多个 video track 共享同一个 publish session、同一个 UDP socket、同一个 `NetworkThread`
- 每个 video track 仍然有独立 controller / source pipeline / SSRC

明确非目标：

- 不把 plain-client 描述成完整浏览器 sender 等价物
- 不宣称已经具备 LiveKit downlink `streamallocator` 的完整对象模型
- 不宣称已经具备新的 weight-aware multi-track transport fairness 模型

## 9. 推荐阅读顺序

如果目标是理解 plain-client 当前主路径，建议按这个顺序：

1. [plain-client-qos-status.md](README.md)
2. [plain-client-twcc-change-summary.md](twcc-change-summary.md)
3. [twcc-ab-test.md](twcc-ab-test.md)
4. [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)
5. [architecture_cn.md](../../platform/architecture_cn.md)
