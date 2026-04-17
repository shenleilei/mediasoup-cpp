# Linux PlainTransport C++ Client 架构

> 文档性质
>
> 这是一份当前有效的 Linux `PlainTransport C++ client` 架构说明。
> 它回答的是“客户端本身怎么工作”，不是历史 code review 结论。
>
> 当前状态与签收口径请继续看：
> - [plain-client-qos-status.md](./plain-client-qos-status.md)
> - [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)
> - [plain-client-qos-case-results.md](./plain-client-qos-case-results.md)
> - [architecture_cn.md](./architecture_cn.md)
>
> 如果目标是评审 Linux client 在“多视频源、同一发布会话、多 track 推流”场景下的目标线程模型，请继续看：
> - [linux-client-multi-source-thread-model_cn.md](./linux-client-multi-source-thread-model_cn.md)

## 1. 角色与边界

仓库里的 Linux client 指的是：

- `client/main.cpp`
- `client/RtcpHandler.h`
- `client/qos/*`

它不是浏览器 client 的替代品，而是仓库内一条独立的 `PlainTransport` 发布端路径，主要用于：

- `PlainTransport` 端到端验证
- Linux 侧 QoS 策略闭环验证
- `cpp-client-harness` / `cpp-client-matrix` 自动化回归

它和浏览器 client 的最大差异是：

- 信令仍走 WebSocket JSON
- 媒体不走 WebRTC sender，而是本地 FFmpeg + RTP/UDP
- QoS stats 由 `RTCP RR + 本地计数 + server getStats` 共同构成

## 2. 进程与线程模型

Linux client 不是一个多线程媒体框架；它的主执行模型很克制：

| 执行单元 | 数量 | 职责 |
|---|---:|---|
| main thread | 1 | MP4 解封装、解码/编码、RTP 发送、RTCP 处理、QoS 采样、通知分发 |
| `WsClient` reader thread | 1 | 异步接收 WebSocket response / notification |
| test helper thread | 0..2 | 仅测试注入路径使用，发送 mock `clientStats` / 自触发 WS request |

关键点：

- 真正的媒体循环在主线程里推进。
- `WsClient` 的 reader thread 只负责收包和把 response / notification 放进本地队列。
- 业务处理仍回到主线程执行，避免 QoS controller、编码器状态和 RTP 发送路径跨线程共享。

## 3. 模块分层

```text
WsClient
  ├─ join / plainPublish / clientStats / getStats
  ├─ async response matching
  └─ qosPolicy / qosOverride notification queue

FFmpeg Pipeline
  ├─ MP4 demux
  ├─ AAC -> Opus transcode
  ├─ H264 copy path
  └─ decode -> x264 re-encode path

RTP / RTCP
  ├─ sendH264 / sendOpus
  ├─ per-track RTP state
  ├─ SR / RR / RTT / jitter
  ├─ NACK retransmission
  └─ PLI / FIR keyframe response

QoS Runtime
  ├─ PublisherQosController (per video track)
  ├─ peer-level budget coordination
  ├─ clientStats snapshot publish
  └─ server getStats assisted sampling
```

对应代码入口：

- 控制面：`client/main.cpp`
- RTCP：`client/RtcpHandler.h`
- QoS：`client/qos/QosController.h` 及周边头文件

## 4. 启动与发布链路

Linux client 的主路径可以概括成：

```text
open mp4
  -> connect WebSocket
  -> join(roomId, peerId)
  -> plainPublish(videoSsrcs, audioSsrc)
  -> 建立 UDP socket
  -> 发送 comedia 探测 RTP
  -> 注册每个 video track 的 RTCP state
  -> 进入主发送循环
```

`plainPublish` 成功后，服务端会返回：

- PlainTransport 的目标 `ip/port`
- 默认 `videoPt/audioPt`
- 每个视频 track 的：
  - `ssrc`
  - `pt`
  - `producerId`

客户端据此把本地 `trackId -> ssrc -> producerId` 对齐起来。

## 5. 媒体与 RTCP 链路

### 5.1 视频路径

有两种模式：

- `copy` 模式：直接把 MP4 里的 H264 转 Annex-B 后 RTP 打包发送
- `re-encode` 模式：`decode -> x264`，由 QoS 动态控制码率、帧率和缩放

`re-encode` 模式下，每个视频 track 都有独立的 `VideoTrackRuntime`，持有：

- `trackId`
- `producerId`
- `ssrc`
- `payloadType`
- `seq`
- `encBitrate`
- `configuredVideoFps`
- `scaleResolutionDownBy`
- `videoSuppressed`
- `forceNextVideoKeyframe`
- `encoder / scaledFrame / swsCtx`
- `qosCtrl`

### 5.2 音频路径

音频走：

```text
AAC -> decode -> Opus encode -> RTP packetize -> UDP
```

目前音频没有独立的复杂 QoS ladder；它更多是作为：

- `audio-only` 模式下保留的媒体流
- `peerHasAudioTrack` 语义的一部分

### 5.3 RTCP 路径

`RtcpContext` 维护每个视频 SSRC 的独立状态：

- RTP packet store
- last keyframe
- RR-derived `packetsLost / jitter / rtt`
- SR NTP send-time map

它负责：

- 定期发送 SR
- 解析来自 worker 的 RR
- 处理 NACK 重传
- 处理 PLI / FIR 的关键帧响应

当前实现里，视频被 `audio-only / pauseUpstream` 抑制时，RTCP 触发的重传和关键帧补发也会一起停止，不再绕过抑制状态。

## 6. QoS 链路

### 6.1 每 track 一个 controller

每个视频 track 对应一个 `PublisherQosController`：

- 输入：`RawSenderSnapshot`
- 输出：`PlannedAction`
- 执行：通过 `actionSink` 反映到真实编码/发送行为

主线程在每个采样窗口里做：

```text
RTCP + server getStats + local encoder state
  -> per-track RawSenderSnapshot
  -> per-track PublisherQosController::onSample()
  -> actionSink
  -> 主线程聚合全部 video track 的 state / signals / lastAction
  -> peer-level clientStats snapshot
  -> clientStats
```

### 6.2 server-assisted sampling

Linux client 不是纯本地闭环，它还会异步请求服务端 `getStats`，用来补足：

- producer stats 的 `byteCount / packetCount / packetsLost`
- `roundTripTime / jitter`

采样时优先取新鲜的 server stats；如果暂时拿不到，再回退到本地 RTCP 派生值。

### 6.3 notification 驱动

客户端会消费两类 QoS notification：

- `qosPolicy`
- `qosOverride`

其中：

- `qosPolicy` 更新 runtime 采样/快照节奏与 profile 选择
- `qosOverride` 驱动 force-audio-only、pause/resume、disableRecovery 等行为

`scope=track` 的 override 现在要求显式携带 `trackId`，避免把本来只想打到单 track 的控制误扩散到所有视频 track。

### 6.4 动作如何落到真实数据面

| QoS action | Linux client 行为 |
|---|---|
| `SetEncodingParameters` | 调整 bitrate / fps / scale；必要时重建 x264 encoder |
| `EnterAudioOnly` | `videoSuppressed=true`，停止视频 RTP |
| `ExitAudioOnly` | 解除 suppression，并强制下一帧 keyframe |
| `PauseUpstream` | 与 video suppression 语义一致 |
| `ResumeUpstream` | 恢复发送，并触发关键帧恢复 |

这条链路的设计目标是：

- 不只打印日志
- 而是真实改变 RTP 发送行为

## 7. 多视频 track 模型

Linux client 现在支持多个 video track runtime。

关键规则：

- `trackId` 默认形如：
  - `video`
  - `video-1`
  - `video-2`
- `PLAIN_CLIENT_VIDEO_TRACK_COUNT` 控制视频 track 数量
- `PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS` 控制 peer 内预算分配权重
- `plainPublish` 必须使用唯一、非零的 `videoSsrcs`

peer 级预算协调只发生在本地多个 video track 之间，不改变服务端房间模型。

### 7.1 浏览器房间联调经验

在现网 SFU 页面里手工验证 plain-client 多 track 推流时，本仓库已经确认过一条稳定路径：

- 现有 `mediasoup-sfu` 不需要为了 plain-client 多 track 推流专门重启。
- `PLAIN_CLIENT_VIDEO_TRACK_COUNT=3` 的 plain-client 可以正常对同一个房间发布 `3` 条 video track。
- 如果浏览器页面是先 `join(roomId)`，之后 plain-client 才执行 `plainPublish`，页面侧可能需要刷新并重新 `join` 一次，才能稳定看到后加入的 plain producer。

本次确认可用的手工验证方式是：

```bash
ffmpeg -y -stream_loop 19 -i test_sweep.mp4 -c copy /tmp/test_sweep_x20.mp4
env PLAIN_CLIENT_VIDEO_TRACK_COUNT=3 \
  ./client/build/plain-client \
  127.0.0.1 3000 <roomId> plain_3track_demo /tmp/test_sweep_x20.mp4
```

经验结论：

- 短文件容易在你打开页面前就结束，联调时优先用长文件或循环文件。
- 如果 `plain-client` 日志已经出现：
  - `WS connected`
  - `Joined room=...`
  - `Publish -> ... videoTracks=3`
  那么优先怀疑的是浏览器订阅时机，而不是 SFU 没有建起 plain producer。

## 8. 推荐阅读顺序

如果目标是理解 Linux client 当前状态，建议按这个顺序：

1. [plain-client-qos-status.md](./plain-client-qos-status.md)
2. [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)
3. [plain-client-qos-case-results.md](./plain-client-qos-case-results.md)
4. [architecture_cn.md](./architecture_cn.md)

如果目标是追历史 review 或 gap 分析，建议直接看 git 历史，而不是继续依赖已经失效的 plain-client 迁移 / blocker 清单。
