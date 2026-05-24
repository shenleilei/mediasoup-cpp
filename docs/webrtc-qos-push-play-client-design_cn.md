# WebRTC QoS SDK Plain 推拉流客户端设计方案

> 文档状态：review draft
>
> 本文只定义新的推拉流客户端实现方案，不包含代码实现。
> 后续开发都在 `mediasoup-cpp` 仓库内进行。

## 1. 背景

已删除的早期 PlainTransport C++ 推流客户端曾具备可复用的 mediasoup
信令路径，但媒体面和 QoS 面逐步膨胀成一套自研 RTC stack：

- 自研 RTP packetizer。
- 自研 RTCP SR/RR/NACK/PLI/TWCC 处理。
- 自研重传缓存。
- 自研 pacing / transport controller。
- 自研 send-side BWE。
- 自研 publisher QoS controller。
- 自研弱网矩阵和 stats 闭环。

这条路线的问题是复杂度不可控，并且和 WebRTC 已经成熟的 QoS 能力重复。

新的方向是：保留 mediasoup-cpp 里已经跑通的信令和 `PlainTransport`
接入能力，在同一套 Plain 推拉流客户端方案里同时实现 push 和 play。
客户端只做 mediasoup 信令和 UDP adapter，媒体 QoS 严格按照
`webrtc_qos_sdk` 的推拉流 facade 规范接入：

- 推流端只使用 `webrtc_qos::VideoPushClient`。
- 播放端只使用 `webrtc_qos::VideoPlayClient`。

## 2. 总目标

新增一个 Plain 推拉流客户端目录，目录内包含 push/play 两个入口：

```text
client/webrtc_qos_plain_client/
  push
  play
```

推流客户端职责：

- 使用 mediasoup-cpp 现有 WebSocket 信令加入房间。
- 调用现有 `plainPublish` 创建 mediasoup `PlainTransport` producer。
- 解析服务端返回的 `ssrc / payloadType / transportCcExtId / producerId`。
- 将 FFmpeg 输出的 H264 Annex-B access unit 喂给
  `webrtc_qos::VideoPushClient`。
- 将 SDK 输出的 RTP/RTCP bytes 直接通过 UDP 发给 mediasoup
  `PlainTransport`。
- 将 UDP 收到的 RTCP feedback 投递给
  `VideoPushClient::OnTransportFeedback()`。

播放客户端职责：

- 使用 mediasoup-cpp 现有 WebSocket 信令加入房间。
- 调用现有 `plainSubscribe` 创建 mediasoup `PlainTransport` consumer。
- 解析返回的 `consumers[].rtpParameters`，提取 video 下行 SSRC/PT/TWCC ext id。
  如果 consumer 未返回 TWCC header extension，play 端降级为无 TWCC 接收并打日志。
- 将 UDP 收到的 RTP/RTCP bytes 投递给 `webrtc_qos::VideoPlayClient`。
- 将 `VideoPlayClient` 输出的 RTCP feedback 直接通过 UDP 发回 mediasoup
  `PlainTransport`。
- 将 `VideoPlayClient` 输出的 Annex-B AU 写文件或交给后续 decoder/QoE
  harness。

目标不是修补已删除的旧实现，而是建立一条新的、边界清晰的 SDK 推拉流闭环。

## 3. 核心原则

- mediasoup-cpp 负责房间、信令、PlainTransport、producer/consumer 生命周期。
- 新客户端负责命令行、信令调用、UDP socket、FFmpeg source/sink 和日志 glue。
- SDK 负责 RTP/RTCP、H264 packetization/depacketization、NACK、PLI、TWCC、
  SR/RR、pacing、GoogCC、jitter 和 packet recovery。
- 新客户端不生成 RTP header。
- 新客户端不解析或生成 NACK/PLI/TWCC。
- 新客户端不实现 jitter buffer。
- 新客户端不写自研带宽估计。
- 新客户端不复用已删除的自研 QoS/BWE/RTCP/packetizer 实现。

## 4. 非目标

第一期不做：

- 不继续增强已删除的旧 C++ 推流客户端。
- 不把已删除的自研 QoS/BWE/RTCP/packetizer 迁移到新客户端。
- 不做 VP8。
- 不做 audio QoS。
- 不做多接收端 fanout 产品化。
- 不做 RTX。
- 不做 FEC / ULPFEC / FlexFEC / RED。
- 不做完整 PeerConnection。
- 不做 ICE / DTLS / SRTP / SDP。
- 不把 `webrtc_qos_sdk` 源码复制进 mediasoup-cpp 仓库。

第一期只做：单路 H264 video push + 单路 H264 video play 闭环。

## 5. 目录规划

push 和 play 放在同一个目录下，避免后续形成两套重复实现：

```text
client/webrtc_qos_plain_client/
  README.md
  common/
    ClientArgs.h/.cpp
    ClientIds.h/.cpp
    PlainUdpTransport.h/.cpp
    RtpRtcpClassifier.h/.cpp
    RuntimeLogHelpers.h/.cpp
  push/
    main.cpp
    PushSignalingSession.h/.cpp
    H264AnnexBSource.h/.cpp
    WebRtcQosPushRuntime.h/.cpp
  play/
    main.cpp
    PlaySignalingSession.h/.cpp
    AnnexBSink.h/.cpp
    WebRtcQosPlayRuntime.h/.cpp
```

第一期允许 push/play runtime 各自保持简单，但 UDP、参数解析、ID 派生和日志
helper 放在 `common/`，避免同一类 PlainTransport glue 复制两份。

## 6. 新旧边界

### 6.1 保留

| 模块 | 保留原因 |
|---|---|
| `client/WsClient.h/.cpp` | 已经能完成 WebSocket request/response/notification。 |
| `join` 信令 | 房间和 peer 生命周期继续沿用现有服务端协议。 |
| `plainPublish` 信令 | 已经能创建 PlainTransport producer 并返回 RTP 参数。 |
| `plainSubscribe` 信令 | 已经能创建 PlainTransport consumer 并返回 consumer RTP 参数。 |
| `RoomService::plainPublish()` | 服务端已有 H264 Baseline、PT、SSRC、TWCC ext id 和 producer 创建逻辑。 |
| `RoomService::plainSubscribe()` | 服务端已有 PlainTransport consumer 创建和 connect 逻辑。 |
| `common/ffmpeg/*` | FFmpeg RAII 封装可继续复用。 |
| UDP socket | PlainTransport 当前就是 RTP/RTCP datagram。 |

### 6.2 不复用

| 模块 | 不复用原因 |
|---|---|
| 自研 QoS 状态机 | 和 SDK GoogCC/Pacer/NACK 路线冲突。 |
| 自研 send-side BWE | 和 SDK WebRTC GoogCC 重复。 |
| 自研 probe/trend/regulator | 和 WebRTC probing/pacing 重复。 |
| 自研 RTCP/NACK/PLI/重传缓存 | 应该由 SDK 接管。 |
| 自研 packetizer/TWCC rewrite/pacing | 应该由 SDK pacer 和 RTP/RTCP 模块接管。 |
| `common/media/rtp/H264Packetizer.*` | 新客户端 H264 RTP packetization 由 SDK 内部完成。 |
| 自研 VP8 packetizer | 新客户端第一期不支持 VP8。 |

### 6.3 借鉴原 Play 的部分

原浏览器 Play 链路里可以借鉴的是信令编排，不是 QoS 和媒体处理：

| 可借鉴点 | 新 native play 的处理 |
|---|---|
| `join` 后再建立接收侧能力 | native play 在 `join` 时直接提交最小 H264 receive capabilities。 |
| `newConsumer` notification | 保留 pending queue 思路；如果 consumer 早于 runtime ready 到达，先缓存再选择。 |
| `existingProducers` / 预创建 consumer | `plainSubscribe` 返回的 `consumers[]` 等价于预创建 consumer，优先从这里选 video。 |
| `requestConsumerKeyFrame` | 选中 video consumer 后调用一次，并延迟约 1s 再补一次，加快首帧。 |
| consumer `rtpParameters` | 完全复用，用来构造 `VideoPlayClient` 的 `SessionConfig`。 |

明确不借鉴：

- 不使用 `mediasoup-client Device` / `createRecvTransport` / WebRTCTransport。
- 不使用浏览器 downlink QoS bundle、hints、sampler、reporter。
- 不把浏览器 receiver stats 逻辑搬到 native play。
- 不因为支持 `newConsumer` 就做多接收端 fanout；第一期仍只选择一个 video consumer。

## 7. 端到端架构

```text
webrtc-qos-plain-push-client
  -> join
  -> plainPublish
  -> VideoPushClient
  -> UDP RTP/RTCP
  -> mediasoup PlainTransport producer
  -> mediasoup Router / Consumer
  -> mediasoup PlainTransport consumer
  -> UDP RTP/RTCP
  -> VideoPlayClient
  -> Annex-B AU output
```

推流端媒体输出：

```text
H264 Annex-B AU
  -> VideoPushClient::PushAnnexBAccessUnit()
  -> WebRTC H264 packetizer
  -> WebRTC RTP/RTCP
  -> WebRTC PacingController
  -> WebRTC GoogCC
  -> TransportOutput callback
  -> UDP send
  -> mediasoup PlainTransport producer
```

播放端媒体输入：

```text
mediasoup PlainTransport consumer
  -> UDP RTP/RTCP
  -> VideoPlayClient::OnRtpPacket()
  -> VideoPlayClient::OnRtcpPacket()
  -> WebRTC NackRequester / PLI
  -> WebRTC H264 depacketization / jitter
  -> decoded_access_unit_output(Annex-B AU)
```

播放端反馈输出：

```text
VideoPlayClient::transport_output
  -> UDP send RTCP feedback
  -> mediasoup PlainTransport consumer
  -> mediasoup worker forwards feedback to producer path
  -> push client receives RTCP
  -> VideoPushClient::OnTransportFeedback()
```

## 8. 信令设计

### 8.1 Push 信令

推流端复用：

```text
connect /ws
  -> join
  -> plainPublish
```

`join` 请求：

```json
{
  "roomId": "...",
  "peerId": "...",
  "displayName": "...",
  "rtpCapabilities": {}
}
```

`plainPublish` 请求：

```json
{
  "videoCodec": "h264",
  "videoSsrc": 11111111,
  "audioSsrc": 22222222
}
```

当前服务端 `plainPublish` 要求 `audioSsrc` 非 0，并且会创建 audio producer。
第一期 push 客户端可以先传一个稳定 audio SSRC，但不发送音频 RTP。这个行为必须
在 README 和日志里明确标注为 v1 限制。

后续可单独改服务端支持：

```json
{
  "videoCodec": "h264",
  "videoSsrc": 11111111,
  "enableAudio": false
}
```

### 8.2 Push 响应

push 客户端解析：

```json
{
  "transportId": "...",
  "ip": "127.0.0.1",
  "port": 40000,
  "videoPt": 102,
  "videoSsrc": 11111111,
  "videoProdId": "...",
  "videoCodec": "h264",
  "videoTracks": [
    {
      "index": 0,
      "pt": 102,
      "ssrc": 11111111,
      "producerId": "...",
      "transportCcExtId": 5
    }
  ],
  "videoTransportCcExtId": 5
}
```

第一期只接受：

- `videoCodec == "h264"`
- `videoTracks.size() == 1`
- `videoTracks[0].ssrc != 0`
- `videoTracks[0].pt != 0`
- `videoTracks[0].transportCcExtId != 0`

### 8.3 Play 信令

播放端复用：

```text
connect /ws
  -> join(with minimal H264 receive capabilities)
  -> bind local UDP socket
  -> plainSubscribe(recvIp, recvPort)
  -> use consumers[] or wait for newConsumer
```

`plainSubscribe` 内部 `consume()` 使用的是 `join` 时写入 peer 的
`rtpCapabilities`。因此 play 客户端不能先空 capabilities join，再依赖
join response 里的 `routerRtpCapabilities` 自动补齐。

第一期 play 客户端必须在 `join` 请求里带最小 Plain receive capabilities：

- H264 Baseline。
- `packetization-mode=1`。
- `profile-level-id=42e01f`。
- `transport-wide-cc-01` header extension。
- `nack` / `nack pli` feedback。
- Opus 兼容声明。

Opus 兼容声明不是接入 audio QoS。当前服务端 `plainPublish` 会创建 dummy audio
producer；play 如果只声明 H264，服务端在自动订阅 audio producer 时会产生
`no compatible codecs` 错误日志。play runtime 仍只选择 video consumer。

如果后续希望完全按 server router capabilities 自动生成 play capabilities，需要先新增
preflight 能力查询接口，或者让 `plainSubscribe` 接受并更新 `rtpCapabilities`。
这不是第一期必需项。

原浏览器 Play 可以通过 `mediasoup-client Device.load(routerRtpCapabilities)`
生成完整 receive capabilities；native Plain play 不引入 `mediasoup-client`，所以第一期用
固定的 Plain 最小 capabilities，避免空 capabilities 导致服务端 `consume()` 失败。

`plainSubscribe` 请求：

```json
{
  "recvIp": "127.0.0.1",
  "recvPort": 50000
}
```

其中 `recvPort` 是 play 客户端本地 UDP socket 绑定端口。服务端
`PlainTransport` 使用 `rtcpMux=true`，RTP/RTCP 都发到同一端口。

### 8.4 Play 响应

play 客户端解析：

```json
{
  "transportId": "...",
  "ip": "127.0.0.1",
  "port": 40002,
  "consumers": [
    {
      "peerId": "linux-pusher-1",
      "producerId": "...",
      "id": "...",
      "kind": "video",
      "rtpParameters": {
        "codecs": [
          {
            "mimeType": "video/H264",
            "payloadType": 102,
            "clockRate": 90000,
            "parameters": {
              "packetization-mode": 1,
              "profile-level-id": "42e01f"
            }
          }
        ],
        "encodings": [
          {
            "ssrc": 33333333
          }
        ],
        "headerExtensions": [
          {
            "uri": "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01",
            "id": 5
          }
        ]
      }
    }
  ]
}
```

第一期 play 客户端只接受一个 video consumer：

- `kind == "video"`
- codec `mimeType == "video/H264"`
- payload type 非 0
- encoding SSRC 非 0
- transport-wide-cc extension id 可为 0；为 0 时记录 `consumer_without_twcc_ext`
  并降级为无 TWCC 接收

如果 room 中已有多个 video producer，第一期通过参数 `--producer-id` 或
`--producer-peer-id` 显式选择一个；默认选择第一个 video consumer。

### 8.5 Play notification

play 客户端需要复用原 Play 的 `newConsumer` 思路，但只服务于单路播放：

```text
on notification(newConsumer):
  if runtime not ready:
    queue consumer data
  else if no selected video consumer:
    try select matching video consumer
    build SessionConfig from consumer.rtpParameters
    create VideoPlayClient
    requestConsumerKeyFrame(consumerId)
    schedule requestConsumerKeyFrame(consumerId) after ~1s
  else:
    log ignored consumer
```

规则：

- 如果 `plainSubscribe` 已经返回目标 video consumer，立即创建 `VideoPlayClient`。
- 如果 `plainSubscribe` 返回空 `consumers[]`，保持 WebSocket 和 UDP socket，等待
  `newConsumer`，直到 `--wait-consumer-timeout-ms` 超时。
- `newConsumer` 只用于补齐启动顺序，不表示第一期支持多画面或多接收端。
- `requestConsumerKeyFrame` 是服务端现有信令，用于加快首帧；NACK/PLI/TWCC 仍由
  `VideoPlayClient` 生成，不在 adapter 层自研。

## 9. SessionConfig 映射

推端和播端必须按同一套规则构造 `SessionConfig`。注意这里的一致性指
业务身份和字段语义一致，不表示 push 上行 SSRC 一定等于 play 下行 SSRC。
mediasoup consumer 可能会改写 RTP 参数，play 端必须以 consumer
`rtpParameters` 为准。

| 语义 | Push 来源 | Play 来源 | SDK 字段 |
|---|---|---|---|
| session | roomId hash | roomId hash | `session.ids.session_id` |
| transport | plainPublish transportId hash | plainSubscribe transportId hash | `session.ids.transport_id` |
| source | push peer hash | producer peer hash | `session.ids.source_id` |
| receiver | 0 或固定值 | play peer hash | `session.ids.receiver_id` |
| track | 本地固定 `1` | 本地固定 `1` | `video_tracks[0].ids.track_id` |
| sender SSRC | producer 上行 `videoTracks[0].ssrc` | consumer 下行 `rtpParameters.encodings[0].ssrc` | `video_tracks[0].ids.sender_ssrc` |
| payload type | `videoTracks[0].pt` | consumer codec `payloadType` | `video_tracks[0].h264.payload_type` |
| TWCC ext id | `transportCcExtId` | consumer header extension id；缺省为 0 | `session.twcc.extension_id` |

注意：mediasoup consumer 下行 SSRC 可能不等于 producer 上行 SSRC。play 客户端必须以
consumer `rtpParameters.encodings[].ssrc` 为准，不能假设和 push 端相同。

推荐第一期配置：

```text
session.start_bitrate_bps = 1200000
session.min_bitrate_bps   = 300000
session.max_bitrate_bps   = 2500000
session.h264.profile_level_id = 0x42e01f
session.h264.packetization_mode_1 = true
session.h264.max_rtp_payload_bytes = 1200
session.rtcp.sr_rr_interval_ms = 1000
```

`producerId` / `consumerId` 是字符串，不塞进 SDK `TransportIds`。客户端维护：

```text
track_id -> producerId
track_id -> consumerId
sender_ssrc -> producerId / consumerId
```

用于日志、metrics 补充和排障。

## 10. UDP 设计

### 10.1 Push UDP

push 端是 connected UDP socket。远端端口来自 `plainPublish` 响应的 `port`；
远端 IP 默认使用 `--server-ip`，如媒体地址和信令地址不同则用
`--media-remote-ip` 覆盖。`plainPublish` 响应里的 `ip` 只记录到日志，不盲目作为
可达地址使用。

```text
VideoPushClient::transport_output(packet)
  -> send(udp_fd, packet.bytes, packet.size)
```

push 端收包：

```text
recv(udp_fd)
  -> if RTCP: VideoPushClient::OnTransportFeedback()
  -> if RTP: log unexpected inbound RTP and drop
```

### 10.2 Play UDP

play 端是 bound UDP socket。`plainSubscribe(recvIp, recvPort)` 后，mediasoup
会把 RTP/RTCP 发到该端口。

`recvIp` 是服务端可达的客户端媒体地址，不一定等于 UDP bind 地址：

- `--listen-ip` 只控制本地 bind。
- `--advertise-ip` 写入 `plainSubscribe.recvIp`。
- 如果 `--listen-ip` 不是 `0.0.0.0`，`--advertise-ip` 可默认等于 `--listen-ip`。
- 如果 `--listen-ip=0.0.0.0`，必须显式传 `--advertise-ip`。

play 端收包：

```text
recv(udp_fd)
  -> if RTP: VideoPlayClient::OnRtpPacket()
  -> if RTCP: VideoPlayClient::OnRtcpPacket()
```

play 端发送：

```text
VideoPlayClient::transport_output(packet)
  -> sendto(media_remote_ip, plain_subscribe_port, packet.bytes, packet.size)
```

`plain_subscribe_port` 来自 `plainSubscribe` 响应的 `port`。`media_remote_ip`
默认使用 `--server-ip`，如 mediasoup 媒体监听地址和信令地址不同则用
`--media-remote-ip` 覆盖。`plainSubscribe` 响应里的 `ip` 只记录到日志，不盲目作为
可达地址使用。

### 10.3 RTP/RTCP 分类

客户端只做最小分类，不解析具体 RTCP feedback：

- RTP/RTCP version 都必须为 2。
- RTCP packet type 通常是 `192..223`。
- 关注 SR 200、RR 201、RTPFB 205、PSFB 206。
- 非 RTCP 且 version=2 的包按 RTP 投给 `VideoPlayClient`。

不允许在客户端自研 NACK/PLI/TWCC 解析逻辑。

## 11. Push H264 输入

第一期支持 H264 MP4 copy path：

```text
InputFormat::Open(path)
  -> FindFirstStreamIndex(video)
  -> require codec_id == AV_CODEC_ID_H264
  -> h264_mp4toannexb bitstream filter
  -> output complete Annex-B AU
  -> VideoPushClient::PushAnnexBAccessUnit()
```

约束：

- 输入必须是 H264。
- 输出必须是完整 Annex-B access unit。
- 不能把单个裸 NALU 直接喂给 SDK。
- PTS/DTS 必须转换为单调 `capture_time_us`。
- keyframe 标记来自 `AV_PKT_FLAG_KEY`。

如果输入不是 H264，第一期直接失败，不自动转码。

## 12. Play AU 输出

`VideoPlayClientConfig::decoded_access_unit_output` 输出 Annex-B AU。

第一期 play 客户端支持两种 sink：

```text
--output-au output.annexb
--output-null
```

`--output-au` 用于确认 SDK play path 已经完成 depacketize/jitter/recovery。
`--output-null` 用于弱网和 metrics smoke，不写大文件。

第一期不内置真实 renderer，不计算 PSNR/SSIM。QoE 由后续 FFmpeg/QoE harness
接在 Annex-B AU 后面。

## 13. Runtime Loop

### 13.1 Push Loop

```text
while running:
  now_us = monotonic_now_us()

  ws.dispatchNotifications()

  drain_udp_rtcp()
    -> push.OnTransportFeedback()

  if time_to_send_next_au:
    au = source.NextAccessUnit()
    push.PushAnnexBAccessUnit(au)

  push.Process(now_us)

  maybe_read_push_snapshot()

  sleep_until_next_tick()
```

### 13.2 Play Loop

```text
while running:
  now_us = monotonic_now_us()

  ws.dispatchNotifications()

  drain_udp_packets()
    -> play.OnRtpPacket()
    -> play.OnRtcpPacket()

  play.Process(now_us)

  maybe_read_play_snapshot()

  sleep_until_next_tick()
```

tick 建议：

```text
process tick: 5ms 到 20ms
```

关键规则：

- push/play 的 `Process()` 都必须持续调用。
- 即使没有新帧或没有新 RTP，也要调用 `Process()`。
- push 端重传由 SDK pacer 后续出队。
- play 端 NACK/PLI retry timer 由 `VideoPlayClient::Process()` 推进。

## 14. Encoder Adaptation

第一期 H264 copy path 没有实时 encoder，因此 adaptation 只做观测，不实际改码率。

push 客户端仍然周期性读取：

```text
VideoPushClient::GetTrackEncoderAdaptation()
VideoPushClient::GetTrackQosSnapshot()
```

第二期如果加入实时编码器，再把 adaptation 接到：

- encoder bitrate
- encoder max fps
- force IDR

不要恢复旧 `PublisherQosController`。

## 15. 日志、Metrics、Alerts

SDK runtime 文件输出是主要观测来源。

推荐目录：

```text
logs/webrtc_qos_plain_client/push/
  push.*.log
  push_metrics.*.jsonl
  push_alerts.*.jsonl

logs/webrtc_qos_plain_client/play/
  play.*.log
  play_metrics.*.jsonl
  play_alerts.*.jsonl
```

SDK 配置：

```text
RuntimeLogConfig.file.enabled = true
RuntimeMetricsConfig.file.enabled = true
RuntimeAlertConfig.file.enabled = true
```

adapter 层也要输出结构化日志，至少包含：

- `roomId`
- `peerId`
- `transportId`
- `producerId`
- `consumerId`
- `videoSsrc`
- `videoPt`
- `transportCcExtId`
- `udpLocalIp`
- `udpLocalPort`
- `udpRemoteIp`
- `udpRemotePort`
- SDK status code

关键 metrics：

| 类别 | 指标 |
|---|---|
| 活性 | push/play process tick gap、RTP output/input gap、AU input/output gap |
| 网络 | RTT、fraction lost、TWCC feedback count |
| 恢复 | NACK、PLI、retransmission、dropped retransmission |
| 控制 | GoogCC target、final target、pacing bitrate |
| UDP | send errors、would block、hard error |
| 数据 | malformed RTP/RTCP、unexpected packet、payload mismatch |

关键 alerts：

- 连续 UDP send hard error。
- push 超过 5s 无 RTCP feedback。
- play 超过 5s 无 RTP input。
- PLI 持续增长但没有关键帧输入。
- NACK 增长但 retransmission 不增长。
- `Process()` tick gap 超过 2s。
- malformed RTP/RTCP。

## 16. CMake 方案

新增可执行 target：

```text
webrtc-qos-plain-push-client
webrtc-qos-plain-play-client
```

push 依赖：

- `client/WsClient.cpp`
- `common/ffmpeg/*`
- `nlohmann_json`
- `spdlog`
- FFmpeg
- OpenSSL crypto/ssl
- pthread
- `WebRtcQosSdk::role_push_bundle` 或 `WebRtcQosSdk::role_push`

play 依赖：

- `client/WsClient.cpp`
- `nlohmann_json`
- `spdlog`
- OpenSSL crypto/ssl
- pthread
- `WebRtcQosSdk::role_play_bundle` 或 `WebRtcQosSdk::role_play`

推荐构建方式：

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/path/to/webrtc_qos_sdk/install
cmake --build build --target webrtc-qos-plain-push-client -j"$(nproc)"
cmake --build build --target webrtc-qos-plain-play-client -j"$(nproc)"
```

选择 SDK target：

```text
if WebRtcQosSdk::role_push_bundle exists:
  link role_push_bundle
else:
  link role_push

if WebRtcQosSdk::role_play_bundle exists:
  link role_play_bundle
else:
  link role_play
```

禁止：

- 不把 `webrtc_qos_sdk/src` 直接加进 mediasoup-cpp target。
- 不 include SDK internal header。
- 不链接已删除的自研 QoS/BWE/congestion-control 代码。

## 17. CLI 方案

push：

```bash
webrtc-qos-plain-push-client \
  --server-ip 127.0.0.1 \
  --server-port 3000 \
  --room room1 \
  --peer linux-pusher-1 \
  --input file.mp4 \
  --log-dir logs/webrtc_qos_plain_client/push
```

play：

```bash
webrtc-qos-plain-play-client \
  --server-ip 127.0.0.1 \
  --server-port 3000 \
  --room room1 \
  --peer linux-player-1 \
  --listen-ip 127.0.0.1 \
  --advertise-ip 127.0.0.1 \
  --listen-port 50000 \
  --output-au output.annexb \
  --log-dir logs/webrtc_qos_plain_client/play
```

push 可选参数：

```text
--video-ssrc <uint32>
--audio-ssrc <uint32>
--start-bitrate <bps>
--min-bitrate <bps>
--max-bitrate <bps>
--process-tick-ms <5..20>
--loop-input
--media-remote-ip <ip>
```

play 可选参数：

```text
--producer-id <id>
--producer-peer-id <peerId>
--receiver-id <uint32>
--advertise-ip <ip>
--process-tick-ms <5..20>
--media-remote-ip <ip>
--wait-consumer-timeout-ms <ms>
--output-null
```

## 18. 测试和验收

### 18.1 静态门禁

新增 grep/link gate：

```text
webrtc-qos-plain-push-client and webrtc-qos-plain-play-client must not depend on:
  deleted self-managed QoS / BWE / RTCP / packetizer code
  H264Packetizer
  self-managed VP8 packetizer
```

### 18.2 信令 smoke

push 验证：

- WebSocket connect 成功。
- `join` 成功。
- `plainPublish` 成功。
- 返回 `videoTracks[0]`。
- SSRC/PT/TWCC ext id 完整。
- UDP socket connect 成功。

play 验证：

- WebSocket connect 成功。
- `join` 携带最小 Plain receive capabilities 并成功。
- UDP bind 成功。
- `plainSubscribe` 成功。
- 返回 video consumer，或能通过 `newConsumer` 等到 video consumer。
- consumer RTP parameters 可映射为 `SessionConfig`。
- 选中 consumer 后 `requestConsumerKeyFrame` 信令成功或失败可观测。

### 18.3 RTP/RTCP 兼容性

抓包或日志确认：

- push 上行 RTP SSRC/PT/TWCC ext id 与 `plainPublish` 一致。
- play 下行 RTP SSRC/PT 与 consumer `rtpParameters` 一致；如果 consumer
  不带 TWCC header extension，日志必须显示无 TWCC 降级。
- play 端 RTCP feedback 能通过 UDP 发回 mediasoup。
- push 端能收到 mediasoup 转发/生成的 RTCP feedback。

### 18.4 端到端闭环

第一期推荐启动顺序：

```text
mediasoup-sfu
  -> webrtc-qos-plain-push-client
  -> webrtc-qos-plain-play-client
```

验收：

- play 客户端收到 video consumer。
- push 客户端开始输出 RTP。
- play 客户端收到 RTP。
- `VideoPlayClient` 输出 Annex-B AU。
- play 端生成 NACK/PLI/SR/RR feedback。
- push 端收到 RTCP feedback。
- push/play 两端 metrics 持续输出。

如果先启动 play，再启动 push，play 客户端保持单路等待模式：`plainSubscribe`
返回空 `consumers[]` 后继续监听 `newConsumer`，收到第一个匹配 video consumer 后创建
`VideoPlayClient`。这只解决启动顺序，不扩展成多接收端。

### 18.5 浏览器兼容性

保留浏览器 receiver 验证：

- 浏览器加入同一房间。
- push 客户端推流。
- 浏览器收到 `newConsumer`。
- 浏览器能看到画面。

浏览器验证用于确认 mediasoup 兼容性；push/play 客户端闭环用于确认 SDK
推拉流规范在 native C++ 路径打通。

### 18.6 弱网 smoke

第一期只做短时 smoke：

- baseline 30s。
- 加入延迟/丢包/限带宽 60s。
- 恢复 30s。

验收：

- push/play 客户端不断流。
- SDK metrics 持续输出。
- push target bitrate 有下探。
- play 有 AU 输出。
- RTCP feedback 不丢失。
- 没有长时间 process tick gap。

## 19. 里程碑

| 阶段 | 目标 | 输出 |
|---|---|---|
| M0 | 设计评审 | 本文档 review 通过 |
| M1 | 目录和 build target | 同一目录下 push/play 新 target，能编译空 runtime |
| M2 | 信令和 UDP | push `plainPublish`、play `plainSubscribe` / `newConsumer` / `requestConsumerKeyFrame` 跑通 |
| M3 | Push SDK | H264 Annex-B AU 能喂给 `VideoPushClient` 并输出 RTP |
| M4 | Play SDK | existing consumer 或 `newConsumer` 的 RTP 能喂给 `VideoPlayClient` 并输出 Annex-B AU |
| M5 | RTCP 闭环 | play feedback 回到 mediasoup，push 收到 feedback |
| M6 | Observability | push/play logs/metrics/alerts 可用于排障 |
| M7 | 弱网 smoke | 通过 baseline/weak/recovery 短测 |

## 20. 风险

| 风险 | 处理 |
|---|---|
| 当前 `plainPublish` 强制创建 audio producer | 第一期传 `audioSsrc` 但不发音频；后续单独改服务端支持 video-only。 |
| `plainSubscribe` consumer capabilities 不完整 | play 的 `join` 必须携带最小 Plain receive capabilities；不能空 join 后再假设自动补齐。 |
| H264 MP4 不一定是 Annex-B | push 必须使用 `h264_mp4toannexb` bitstream filter。 |
| 输入不是 H264 | 第一期直接失败，不自动转码。 |
| consumer TWCC ext id 为 0 | play 端降级接收并打 `consumer_without_twcc_ext`；该场景不能验证下行 TWCC 主链路，只能验证 RTP/RTCP/AU 闭环。 |
| consumer 下行 SSRC 与 producer 上行 SSRC 不同 | play 必须以 consumer `rtpParameters` 为准。 |
| `plainSubscribe` 响应 IP 不可达 | UDP 发送默认用 `--server-ip` + 响应 `port`，必要时用 `--media-remote-ip`；响应 `ip` 只作日志。 |
| mediasoup 不转发 play feedback | 检查 PlainTransport `rtcpMux`、UDP tuple、consumer feedback、worker RTCP 路径。 |
| play 先启动时没有 consumer | 第一期开启单路等待，复用 `newConsumer` notification 创建一次 play runtime，不做多接收端。 |
| PLI 请求关键帧但 copy path 没法立即造 IDR | 第一版记录 `request_keyframe`，后续实时编码器阶段再真正 force IDR。 |
| SDK install prefix 不一致 | CMake 通过 `CMAKE_PREFIX_PATH` 显式指定 SDK 安装目录。 |
| 旧代码被误链接 | 增加 dependency gate 和 target source review。 |

## 21. 后续扩展

原第二期和第三期已合并为一个“大第二期”，详见
[webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md)。

大第二期覆盖：

- 实时 H264 encoder。
- `GetTrackEncoderAdaptation()` 驱动 bitrate/fps/keyframe。
- `enableAudio=false` 服务端信令。
- audio RTP/RTCP 接入。
- 多 video track。
- camera/V4L2 input。
- play 端 FFmpeg decode/QoE harness。
- 更完整的 weak-network matrix。

暂不建议：

- 重新引入旧 `PublisherQosController`。
- 为了兼容旧矩阵恢复自研 RTCP。
- 在第一期加入 RTX/FEC。

## 22. Review Checklist

评审本文档时重点确认：

- 新 push/play 客户端是否必须在同一个 `webrtc_qos_plain_client` 目录下，并保持与已删除旧实现解耦。
- 第一版是否只做 H264 video push + play。
- play 端是否必须严格使用 `VideoPlayClient`，不自研 receiver/jitter/RTCP。
- 是否接受借鉴原 Play 的 `newConsumer` queue 和 `requestConsumerKeyFrame` 信令，但不复用浏览器媒体/QoS 栈。
- 是否接受第一期 push 传 `audioSsrc` 但不发 audio RTP。
- 是否接受 MP4 H264 copy path 先跑通，不做实时转码。
- 是否接受 play 端先输出 Annex-B AU，不内置 renderer。
- 是否接受 SDK 通过 installed package 链接，不 vendoring 源码。
- 是否接受 SDK runtime logs/metrics/alerts 作为主要观测方式。
- 是否需要在第一期就改服务端 `plainPublish` 支持 video-only。
