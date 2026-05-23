# WebRTC QoS SDK Plain 推拉流客户端第二期设计方案

> 文档状态：review draft
> 范围：把原计划中的第二期和第三期合并为一个“大第二期”。
> 前置：第一期已完成 native Plain push/play 本地短链路 smoke，详见
> [webrtc-qos-push-play-client-implementation-checklist_cn.md](./webrtc-qos-push-play-client-implementation-checklist_cn.md)。

## 1. 背景

第一期已经验证了最小链路：

- mediasoup-cpp SFU 能启动。
- `webrtc-qos-plain-push-client` 能通过 `join/plainPublish` 推 H264。
- `webrtc-qos-plain-play-client` 能通过 `join/plainSubscribe/newConsumer` 拉 H264。
- push/play 都通过 `webrtc_qos_sdk` facade 处理 RTP/RTCP/QoS。
- native play 能输出 Annex-B AU。

第一期 smoke 结果证明“链路能跑通”，但还不能证明“QoS 主链路完整、弱网有效、现场可排障”。

第一期后发现的缺口：

- mediasoup consumer 下行 `rtpParameters.headerExtensions=[]`，play 只能降级为无 TWCC 接收。
- 本地 smoke 只验证 RTP/RTCP/AU 闭环，未完整验证下行 TWCC 主链路。
- 当前 SDK dist 包未暴露 runtime logs/metrics/alerts 配置字段。
- push 仍用 H264 MP4 copy path，不能根据 SDK adaptation 改 bitrate/fps，也不能响应 PLI 强制 IDR。
- `plainPublish` 当前强制创建 dummy audio producer，服务端语义不干净。
- 弱网 smoke、浏览器接收、native decode/QoE 尚未自动化。

当前实施进展：

- P2-M1a 已修复 ORTC consumable RTP parameters 不携带 header extension 的问题。
- native smoke 已验证 video consumer `twccExtId=5`，不再走 `consumer_without_twcc_ext` 降级。
- push/play adapter 已补 RTCP 边界计数日志。
- 当前 SDK play facade 只在 NACK/PLI 事件时通过 `transport_output` 发 RTCP，
  尚未生成周期性 RR/TWCC feedback；因此 P2-M1 尚不能签收为 QoS 主链路完整闭环。

第二期目标是把第一期从“最小可跑”推进到“可验证、可观测、可调优、可接真实输入”的状态。

## 2. 第二期总目标

第二期对外只有一个阶段，内部拆成多个里程碑。

核心目标：

1. 补齐 QoS 主链路：TWCC、RTCP feedback、target bitrate 下探/恢复都可验证。
2. 建立弱网自动化：baseline / delay / loss / bandwidth drop / recovery 短测可重复运行。
3. 补齐观测体系：日志、metrics、alerts 写文件，排障字段完整。
4. 清理服务端 Plain 信令语义：支持 video-only publish，不再依赖 dummy audio。
5. 接入实时编码器：SDK adaptation 能驱动 encoder bitrate/fps/keyframe。
6. 扩展输入和验证：支持 V4L2/camera 输入，浏览器 receiver 和 native decode/QoE 验证。

第二期验收口径：

- 不是只要求 play 有 AU 输出。
- 必须证明 QoS 控制信号在弱网下生效，并且日志能解释发生了什么。

第二期每个任务都必须同时满足三类门禁：

- 可实施性：明确要改哪些文件、引入哪些模块、依赖哪些前置条件。
- 可验证性：明确用什么命令或自动化 case 验收，PASS/FAIL/SKIP 口径固定。
- 可观测性：明确日志、metrics、alerts 和报告字段，失败时能定位到具体链路段。

不满足这三类门禁的功能不进入第二期 scope。

## 3. 非目标

第二期不做：

- 多接收端 fanout 压测。
- 多房间 / 多节点 / 集群级容量测试。
- 生产长时间 soak。
- GPU 编码作为必需项。
- 恢复旧 `plain-client` 自研 QoS。
- 在客户端自研 NACK/PLI/TWCC 解析逻辑。

说明：

- 多接收端放到后续 P5 之前不做。
- GPU/NVENC 可以作为可选优化，但第二期基线必须在普通 CPU 环境可跑。
- audio QoS 不作为第二期主线；第二期只清理 `video-only` 信令，避免 dummy audio 干扰。

## 4. 架构边界

### 4.0 实现仓库边界

第二期所有业务交付都在 `mediasoup-cpp` 中实现和验收：

- SFU 侧 PlainTransport 信令和 consumer 参数修正。
- `client/webrtc_qos_plain_client` 下的 push/play 客户端增强。
- 实时编码器、V4L2/camera 输入、native decode/QoE sink。
- 弱网 smoke harness、浏览器 receiver smoke、报告生成。
- 文档、验收清单和 `docs/generated` 下的测试报告。

`webrtc_qos_sdk` 只作为外部依赖使用：

- mediasoup-cpp 通过 `CMAKE_PREFIX_PATH` 引入 SDK install/dist。
- 不在 mediasoup-cpp 中直接编译 SDK `src`。
- 不 include SDK internal header。
- 如果发现 SDK public API 或 dist 包缺字段，需要在 SDK 仓库补 API/重新发布 dist；
  mediasoup-cpp 只消费新的 public header 和 CMake package。

因此，第二期的默认开发目录是：

```text
/root/mediasoup-cpp
```

不是：

```text
/root/webrtc_qos_sdk
```

除非明确是在做 SDK public API 或发布包修复。

### 4.1 保持不变的边界

mediasoup-cpp 负责：

- WebSocket 信令。
- room / peer / transport / producer / consumer 生命周期。
- PlainTransport RTP/RTCP 转发。
- 浏览器兼容性。

`webrtc_qos_sdk` 负责：

- H264 RTP packetization / depacketization。
- RTP pacing / retransmission。
- RTCP NACK / PLI / RR / TWCC feedback。
- GoogCC / target bitrate。
- play 端 jitter / reorder / recovery。
- runtime logs / metrics / alerts。

Plain push/play adapter 负责：

- CLI/config。
- WebSocket 请求和通知。
- UDP socket 收发。
- H264 input / encoder / sink glue。
- 把 SDK status 和 metrics 写到可排障日志。

禁止：

- 新客户端不 include `client/qos`、`sendsidebwe`、`ccutils`。
- 新客户端不链接旧 `NetworkThread`、`RtcpHandler`、`SenderTransportController`。
- 新客户端不重新实现 RTP packetizer、NACK、PLI、TWCC。

### 4.2 第二期新增模块

```text
client/webrtc_qos_plain_client/
  common/
    RuntimeMetricsBridge
    RuntimeAlertBridge
    NetemProbeHelpers
  push/
    RealtimeVideoSource
    H264EncoderAdapter
    EncoderAdaptationApplier
    V4L2VideoSource
  play/
    FfmpegDecodeSink
    QoeProbe
  harness/
    WeakNetworkScenarioRunner
    SmokeReportWriter
```

模块职责：

| 模块 | 职责 |
|---|---|
| `RuntimeMetricsBridge` | 统一 SDK metrics / adapter metrics 输出格式。 |
| `RuntimeAlertBridge` | 统一 SDK alerts / adapter alerts 输出格式。 |
| `RealtimeVideoSource` | 抽象 MP4 loop、V4L2、synthetic frame source。 |
| `H264EncoderAdapter` | CPU x264 baseline encoder，输出 Annex-B AU 给 `VideoPushClient`。 |
| `EncoderAdaptationApplier` | 应用 SDK `GetTrackEncoderAdaptation()` 到 encoder。 |
| `V4L2VideoSource` | Linux camera 输入，输出 raw frames。 |
| `FfmpegDecodeSink` | play 端 AU 解码验证，不负责 UI。 |
| `QoeProbe` | 首帧、冻结、解码失败、输出帧率等基础 QoE 指标。 |
| `WeakNetworkScenarioRunner` | 统一启动 SFU/push/play/netem，并生成报告。 |
| `SmokeReportWriter` | 输出 markdown/json 结果，用于 review 和 CI 附件。 |

## 5. P2-A：QoS 主链路补齐

### 5.0 关键技术判断

重新审查现有服务端代码后，TWCC 缺失的优先排查点是 ORTC 参数生成，不是
PlainTransport 本身：

- `RoomService::plainPublish()` 给 producer RTP 参数写入了
  `transport-wide-cc` header extension。
- `Transport::produce()` 会调用 `ortc::getConsumableRtpParameters()` 生成
  producer 的 consumable RTP 参数。
- 当前 `ortc::getConsumableRtpParameters()` 只映射 codecs / encodings / rtcp，
  没有把 producer RTP 参数中的 `headerExtensions` 映射到 consumable 参数。
- 后续 `ortc::getConsumerRtpParameters()` 是从 consumable 参数和 remote
  capabilities 求交集；如果 consumable 里本来就没有 header extension，
  consumer 下行自然得到 `headerExtensions=[]`。

因此 P2-M1 的第一步是：

1. 给 `ortc::getConsumableRtpParameters()` 增加 header extension 映射。
2. 增加 unit test：producer 带 TWCC ext，consumer remote capabilities 也带 TWCC，
   最终 consumer RTP parameters 必须包含 TWCC ext。
3. 再跑 native push/play smoke，确认 `selected_consumer ... twccExtId=5`。

这一步已经完成；后续 P2-M1 的阻塞点转移到 play 侧 RTCP/TWCC feedback 生成与观测。
这一步完成之前不应该先做实时编码器；现在虽然 consumer TWCC 已恢复，但如果 play
侧 feedback 未闭环，encoder adaptation 仍无法证明是由真实下行反馈驱动。

### 5.1 Consumer TWCC header extension

已修复的问题：

- 第一期 smoke 中 video consumer 的 `rtpParameters.headerExtensions=[]`。
- play 端因此记录 `consumer_without_twcc_ext`，并以 `twccExtId=0` 降级运行。
- 该场景可以验证 RTP/RTCP/AU，但不能验证下行 TWCC 主链路。

修复：

- 在 `ortc::getConsumableRtpParameters()` 中保留 producer RTP 参数里、且 router
  capabilities 支持的 header extension。
- 增加 ORTC 单元测试，覆盖 producer TWCC ext 到 consumer RTP parameters 的透传。
- native smoke 已验证 `selected_consumer ... twccExtId=5`。

目标：

- mediasoup Plain consumer 下行 `rtpParameters` 必须包含 transport-wide-cc extension。
- play 端解析到非 0 `transportCcExtId`。
- SDK play 能按该 extension id 解析 RTP transport sequence number。

后续注意：

1. 当前修复按 `kind + uri` 判断 router 是否支持该 extension，并保留 producer ext id；
   consumer 侧仍会和 remote capabilities 再做最终交集。
2. 如果未来要支持 producer ext id、router preferred id、consumer preferred id 三者完全不同，
   需要补完整 header extension id mapping。
3. 保持客户端降级逻辑：如果某些环境仍无 TWCC，继续可播放但验收不通过 QoS 主链路。

验收：

- play 日志出现 `selected_consumer ... twccExtId=<non-zero>`。
- 不再出现 `consumer_without_twcc_ext`。
- push metrics 中 RTCP feedback 输入计数增长。
- play 侧周期性 RR/TWCC feedback 仍需要 SDK play facade 补能力后验收。

### 5.2 RTCP feedback 闭环

目标：

- play 端生成 RR/NACK/PLI/TWCC。
- mediasoup 能收到 play RTCP。
- push 能收到 mediasoup 转发/生成的 RTCP feedback。
- `VideoPushClient::OnTransportFeedback()` 被持续调用。

Adapter 日志必须记录：

- play `rtcpPacketsOut` / `rtcpBytesOut` / `rtcpSendFailures`。
- push `rtcpFeedbackPacketsIn` / `rtcpFeedbackBytesIn` / `rtcpFeedbackFailures`。
- SDK 侧 TWCC feedback count；当前 dist snapshot 未暴露该字段，不能由 adapter 自行解析代替。
- RR count。
- PLI count。
- NACK count。
- RTT。
- fraction lost。

当前验证结果：

- push 已可观测到 mediasoup 侧返回的 RTCP feedback：
  `rtcpFeedbackPacketsIn=86 rtcpFeedbackBytesIn=2376 rtcpFeedbackFailures=0`。
- SDK push snapshot 已输出 `rttMs`、`loss`、`targetBps`、`pacingBps`、`finalTargetBps`。
- play adapter 已记录 `rtcpPacketsOut`，但 baseline smoke 中仍为 `0`。
- 审查 SDK 源码确认当前 `VideoPlayClient` 只在 NACK/PLI 事件时输出 RTCP，
  没有周期性 RR/TWCC 生成路径；这必须在 SDK public API/dist 层补齐，mediasoup-cpp
  不应自研 TWCC/RR 生成。

验收：

- baseline 下 RTT 非异常值。
- baseline 下 play `rtcpPacketsOut > 0`，push `rtcpFeedbackPacketsIn > 0`。
- baseline 下 SDK 暴露的 TWCC feedback counter 增长。
- 触发丢包时 NACK 增长。
- 请求关键帧时 PLI 增长。
- bandwidth drop 时 target bitrate 下探。
- recovery 后 target bitrate 回升。

## 6. P2-B：弱网自动化

### 6.1 场景设计

第二期只做短 smoke，不做长 soak。

推荐每个 case 60 秒：

- 15 秒 baseline。
- 30 秒施加网络条件。
- 15 秒恢复。

场景矩阵：

| Case | 条件 | 目的 |
|---|---|---|
| `baseline` | 无 netem | 验证基本链路稳定。 |
| `delay_100ms` | 100ms delay，20ms jitter | 验证 RTT 上升和 pacing 稳定性。 |
| `loss_2pct` | 2% random loss | 验证 NACK/重传和 AU 连续性。 |
| `loss_5pct` | 5% random loss | 验证弱网降级和恢复。 |
| `bandwidth_600k` | 限带宽 600kbps | 验证 target bitrate 下探。 |
| `drop_recover` | 5% loss + 600kbps 后恢复 | 验证恢复时间和 target bitrate 回升。 |

如果运行环境没有 `CAP_NET_ADMIN` 或不能使用 `tc netem`：

- case 标记为 `SKIP`。
- 报告必须写明 skip 原因。
- 不允许把 skip 记为 pass。

### 6.2 指标和阈值

基础通过条件：

- push/play 进程不崩溃。
- WebSocket 不断开。
- UDP send hard error 为 0。
- `Process()` tick gap 不超过 2 秒。
- play 持续有 RTP input。
- play 持续有 AU output。

QoS 通过条件：

| 场景 | 验收口径 |
|---|---|
| delay | RTT 指标显著上升，AU 不长期中断。 |
| loss | NACK 增长，retransmission 增长，AU 有输出。 |
| bandwidth drop | target bitrate 在 10 秒内下探。 |
| recovery | target bitrate 在恢复后 15 秒内开始回升。 |
| mixed weak network | 不崩溃、不长时间无 RTP/AU、alerts 可解释。 |

输出报告：

```text
docs/generated/webrtc-qos-plain-p2-smoke-report.json
docs/generated/webrtc-qos-plain-p2-smoke-report.md
```

报告字段：

- case name。
- network condition。
- push pushedAu。
- play outputAu。
- RTP/RTCP counts。
- RTT min/avg/max。
- fraction lost avg/max。
- target bitrate min/avg/max。
- NACK/PLI/retransmission counts。
- alert summary。
- PASS / FAIL / SKIP。

## 7. P2-C：观测、日志、告警

### 7.1 SDK dist 更新

当前 dist 包问题：

- 头文件没有暴露 `RuntimeLogConfig`。
- 头文件没有暴露 `RuntimeMetricsConfig`。
- 头文件没有暴露 `RuntimeAlertConfig`。
- 客户端只能输出 `sdk_runtime_files enabled=false`，依赖 adapter spdlog。

第二期必须重新发布 SDK dist：

- include 中暴露 runtime config 字段。
- CMake package target 不变。
- role bundle target 不变。
- mediasoup-cpp 仍通过 `CMAKE_PREFIX_PATH` 引入，不直接引用 SDK source。

验收：

- push 日志出现 `sdk_runtime_files role=push enabled=true`。
- play 日志出现 `sdk_runtime_files role=play enabled=true`。
- `push_metrics.*.jsonl` 存在且持续写入。
- `play_metrics.*.jsonl` 存在且持续写入。
- alerts 文件存在；无 alert 时也写启动元信息。

### 7.2 Adapter 结构化日志

所有关键日志必须包含：

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

关键事件：

| 事件 | 级别 |
|---|---|
| `ws_connected` | info |
| `join_ok` | info |
| `plain_publish_ok` | info |
| `plain_subscribe_ok` | info |
| `selected_consumer` | info |
| `consumer_without_twcc_ext` | warn |
| `sdk_runtime_files enabled=false` | warn |
| UDP hard error | error |
| SDK status 非 OK | warn/error |
| RTP/AU 长时间中断 | warn/error |

### 7.3 Alerts

第二期新增 adapter alerts：

- `no_rtcp_feedback_5s`
- `no_rtp_input_5s`
- `no_au_output_5s`
- `process_tick_gap`
- `udp_send_hard_error`
- `malformed_packet`
- `twcc_missing`
- `target_bitrate_not_down`
- `target_bitrate_not_recover`
- `encoder_keyframe_not_generated`

这些 alerts 后续可以接入生产监控，但第二期先落本地文件和 smoke 报告。

## 8. P2-D：服务端 Plain 信令清理

### 8.1 Video-only publish

问题：

- 当前 `plainPublish` 要求 `audioSsrc` 非 0。
- 服务端会创建 dummy audio producer。
- play 为避免 audio auto-subscribe error，被迫声明 Opus capability。

目标：

- `plainPublish` 支持 video-only。
- 新客户端默认 `enableAudio=false`。
- 不再创建 dummy audio producer。
- play receive capabilities 可以只声明 H264。

建议请求格式：

```json
{
  "videoCodec": "h264",
  "videoSsrc": 11111111,
  "enableAudio": false
}
```

兼容策略：

- 未传 `enableAudio` 时保持旧行为，避免破坏旧 plain-client。
- `enableAudio=false` 时允许 `audioSsrc` 缺省或为 0。
- response 中明确返回 `audioEnabled=false`。

验收：

- push 请求不带 `audioSsrc` 也能 `plainPublish` 成功。
- 服务端 room stats 中没有 dummy audio producer。
- play 不再收到 audio consumer。
- SFU 日志没有 audio auto-subscribe 相关错误。

### 8.2 Plain capabilities 查询

第二期可增加一个轻量能力查询接口，减少 native play 固定 capabilities 的硬编码。

候选接口：

```text
getRouterRtpCapabilities
```

用途：

- play 启动前获取 router capabilities。
- 根据 H264 Baseline / Opus / header extension 生成 receive capabilities。
- 避免 server codec 配置变化导致 native play 不兼容。

如果接口成本高，第二期可以先保留固定 Plain receive capabilities，但要在文档中明确。

## 9. P2-E：实时编码器和 adaptation

### 9.1 为什么要做实时编码器

第一期 H264 MP4 copy path 的限制：

- 不能根据 target bitrate 改编码码率。
- 不能降低 fps。
- PLI 到达时不能立即 force IDR。
- 输入文件码率固定，QoS 控制效果只能观测，不能真正作用到编码器。

第二期需要把 SDK adaptation 接到 encoder：

```text
VideoPushClient::GetTrackEncoderAdaptation()
  -> H264EncoderAdapter::SetBitrate()
  -> H264EncoderAdapter::SetMaxFps()
  -> H264EncoderAdapter::ForceKeyFrame()
```

### 9.2 Encoder 基线

第二期基线使用 CPU x264 / libx264。

理由：

- 不要求运行环境有 GPU。
- 更容易在 CI 和普通服务器复现。
- 和当前 FFmpeg 依赖一致。

编码参数建议：

| 参数 | 建议 |
|---|---|
| profile | baseline |
| pixel format | yuv420p |
| B-frames | 0 |
| tune | zerolatency |
| GOP | 1-2 秒 |
| rate control | bitrate + vbv |
| output | Annex-B AU |

GPU/NVENC：

- 只作为可选优化。
- 不作为第二期验收条件。
- 如果环境没有显卡，测试直接跳过 GPU case。

### 9.3 输入源

第二期支持三个输入源：

| 输入源 | 用途 |
|---|---|
| MP4 loop decode | 可重复测试，适合 CI。 |
| synthetic raw frames | 无外部文件依赖，适合最小 smoke。 |
| V4L2 camera | 接真实摄像头，适合本地调试和 demo。 |

CLI 建议：

```text
--input file.mp4
--input-v4l2 /dev/video0
--input-synthetic testsrc
--width 640
--height 360
--fps 30
--encoder x264
--bitrate 1200000
```

约束：

- 同一时间只启用一个输入源。
- 输入源输出 raw frames。
- encoder 输出 Annex-B AU。
- SDK 仍只接收 Annex-B AU，不关心输入来源。

### 9.4 Keyframe

当 SDK adaptation 或 RTCP feedback 要求 keyframe：

- adapter 记录 `requestKeyframe=true`。
- encoder 在下一帧 force IDR。
- 日志记录 `forced_keyframe frameIndex=... reason=...`。

验收：

- play 启动时请求关键帧，push 能在 1 秒内输出 IDR。
- PLI 增长时 push 能响应 IDR。
- 弱网恢复后首帧时间可观测。

## 10. P2-F：浏览器和 native QoE 验证

### 10.1 浏览器 receiver

目标：

- 新 push 推出的流能被普通浏览器 WebRTC receiver 播放。
- 不只和 native play 自闭环。

验收：

- 浏览器加入同一 room。
- 收到 `newConsumer`。
- video element 有画面。
- browser inbound stats 显示 packets/frames 增长。
- 弱网 case 下浏览器不断流或可恢复。

输出：

- 浏览器 smoke 脚本。
- stats 摘要。
- 失败时截图或日志附件。

### 10.2 Native decode/QoE

play 端增加 FFmpeg decode sink：

```text
Annex-B AU
  -> FFmpeg H264 decoder
  -> decoded frame
  -> QoeProbe
```

QoE 指标：

- first frame time。
- decoded frame count。
- decode error count。
- output fps。
- freeze count。
- max freeze duration。
- keyframe interval。

第二期不做复杂 VMAF/PSNR。

验收：

- baseline 下 decode error 为 0。
- 弱网下可记录 freeze，但恢复后 decoded frames 继续增长。
- QoE 指标写入 smoke report。

## 11. P2-G：配置和 CLI

第二期 CLI 要避免参数爆炸，建议支持 config file。

示例：

```json
{
  "serverIp": "127.0.0.1",
  "serverPort": 3000,
  "room": "room1",
  "peer": "linux-pusher-1",
  "logDir": "logs/webrtc_qos_plain_client/push",
  "input": {
    "type": "v4l2",
    "device": "/dev/video0",
    "width": 640,
    "height": 360,
    "fps": 30
  },
  "encoder": {
    "type": "x264",
    "startBitrateBps": 1200000,
    "minBitrateBps": 300000,
    "maxBitrateBps": 2500000
  },
  "publish": {
    "videoCodec": "h264",
    "enableAudio": false
  }
}
```

CLI 优先级：

```text
defaults < config file < CLI flags
```

## 12. 实施里程碑

### 12.1 前置依赖 DAG

```text
P2-M1a ORTC consumable header extension
  -> P2-M1b native TWCC consumer smoke
  -> P2-M1c adapter RTCP boundary counters
  -> P2-M1d SDK play RR/TWCC feedback output
  -> P2-M3 weak-network harness
  -> P2-M5 realtime encoder adaptation
  -> P2-M7 browser weak-network smoke
  -> P2-M8 native decode/QoE

P2-M2 SDK runtime dist
  -> P2-M3 weak-network harness
  -> P2-M9 final report

P2-M4 video-only publish
  -> P2-M7 browser compatibility cleanup
```

关键顺序：

- P2-M1a/P2-M1b 是硬前置；不先修 consumer TWCC，就无法签收 QoS 主链路。
- P2-M1d 是 QoS 闭环硬前置；play 侧不输出 RR/TWCC 时，弱网 case 不能证明下行反馈有效。
- P2-M2 是弱网自动化的观测前置；没有 metrics/alerts 文件，弱网 case 只能靠日志猜。
- P2-M5 实时编码器依赖 P2-M1 和 P2-M3；否则 bitrate/fps/keyframe adaptation 没有可信验收。
- P2-M4 可以并行，但不应阻塞 P2-M1；它是语义清理，不是 QoS 主链路前置。

### 12.2 里程碑列表

| 里程碑 | 内容 | 输出 |
|---|---|---|
| P2-M0 | 文档 review | 本文档确认。 |
| P2-M1 | QoS 主链路 | consumer TWCC ext 非 0，play RR/TWCC 输出、push feedback 输入和 SDK counter 闭环可观测。 |
| P2-M2 | SDK runtime dist | logs/metrics/alerts 文件输出启用。 |
| P2-M3 | 弱网 harness | baseline/delay/loss/bandwidth/recovery 自动报告。 |
| P2-M4 | video-only publish | `enableAudio=false`，去掉 dummy audio。 |
| P2-M5 | 实时 x264 encoder | bitrate/fps/keyframe adaptation 生效。 |
| P2-M6 | 输入源扩展 | MP4 decode loop / synthetic / V4L2。 |
| P2-M7 | 浏览器兼容 | 浏览器 receiver 可播放并输出 stats。 |
| P2-M8 | native decode/QoE | play decode sink 和 QoE 报告。 |
| P2-M9 | 签收回归 | 全部 P2 smoke PASS，报告入 docs/generated。 |

推荐顺序：

1. 先做 P2-M1 / P2-M2 / P2-M3。
2. 再做 P2-M4。
3. 再做 P2-M5 / P2-M6。
4. 最后做 P2-M7 / P2-M8 / P2-M9。

理由：

- 没有 QoS 主链路和观测，实时编码器做出来也无法证明有效。
- 没有弱网 harness，后续每次改 encoder 都靠手工判断，风险高。
- video-only 是服务端语义清理，不应该阻塞 QoS 主链路验证。

## 13. 验收清单

### 13.0 实施 / 验证 / 观测矩阵

| 里程碑 | 可实施性 | 可验证性 | 可观测性 |
|---|---|---|---|
| P2-M1a ORTC header extension | 修改 `src/ortc.h`，补 `getConsumableRtpParameters()` header extension 映射；补 `tests/test_ortc.cpp`。 | standalone ORTC test 通过；native smoke 中 `selected_consumer twccExtId=5`。 | play 日志不再出现 `consumer_without_twcc_ext`。 |
| P2-M1b native TWCC consumer smoke | 复用当前 push/play 和 SFU smoke。 | baseline smoke 验证 video `newConsumer.headerExtensions` 包含 TWCC id 5，play `transportCcExtId=5`。 | `play_runtime_started ... transportCcExtId=5`。 |
| P2-M1c adapter RTCP boundary counters | 修改 push/play runtime counters。 | baseline smoke 验证 push `rtcpFeedbackPacketsIn > 0`，play 记录 `rtcpPacketsOut` 字段。 | push/play metrics 输出 `rtcpFeedbackPacketsIn`、`rtcpFeedbackBytesIn`、`rtcpFeedbackFailures`、`rtcpPacketsOut`、`rtcpBytesOut`、`rtcpSendFailures`、`rttMs`、`loss`。 |
| P2-M1d SDK play RR/TWCC feedback output | 在 `webrtc_qos_sdk` public facade/dist 补 play 周期性 RR/TWCC 输出和 snapshot counter；mediasoup-cpp 只升级 dist 并验证。 | baseline smoke 必须看到 play `rtcpPacketsOut > 0`、SDK TWCC feedback counter 增长、push feedback input 增长。 | SDK metrics 输出 TWCC/RR counters；adapter 不自研解析 TWCC。 |
| P2-M2 SDK runtime dist | 更新 SDK install/dist；mediasoup-cpp 仅更新 `CMAKE_PREFIX_PATH` 和兼容检查。 | 启动日志必须是 `sdk_runtime_files enabled=true`；metrics/alerts 文件存在。 | `push_metrics.jsonl`、`play_metrics.jsonl`、alerts jsonl 持续写入。 |
| P2-M3 弱网 harness | 新增 `client/webrtc_qos_plain_client/harness` 或 `tests/qos_harness` 脚本，统一启动 SFU/push/play/netem。 | baseline/delay/loss/bandwidth/recovery case 输出 PASS/FAIL/SKIP。 | 生成 `docs/generated/webrtc-qos-plain-p2-smoke-report.{json,md}`。 |
| P2-M4 video-only publish | 修改 `RoomService::plainPublish()`、signaling dispatcher、push signaling；保持旧请求兼容。 | `enableAudio=false` 时无 audio producer；旧 `audioSsrc` 请求仍通过。 | SFU stats/report 中 `audioEnabled=false`；无 dummy audio consumer 日志。 |
| P2-M5 realtime x264 encoder | 新增 `H264EncoderAdapter` 和 `EncoderAdaptationApplier`；push runtime 接入 raw frame source。 | bandwidth drop case 中 encoder bitrate 下探；PLI 后 1 秒内输出 IDR。 | encoder metrics 输出 bitrate/fps/keyframe/frameDrop；alerts 记录 keyframe 未生成。 |
| P2-M6 输入源扩展 | 新增 synthetic、MP4 decode loop、V4L2 source；统一 `RealtimeVideoSource` 接口。 | synthetic 和 MP4 decode loop 必跑；无 V4L2 设备时 V4L2 case SKIP。 | source metrics 输出 frame count、input fps、decode/capture errors。 |
| P2-M7 浏览器兼容 | 新增 browser receiver smoke，复用现有 web/signaling。 | 浏览器收到 `newConsumer`、video frames 增长、截图或 stats 通过。 | browser stats 附到 report，失败带 console/error 摘要。 |
| P2-M8 native decode/QoE | 新增 `FfmpegDecodeSink` 和 `QoeProbe`。 | baseline decode error 为 0；弱网恢复后 decoded frames 继续增长。 | report 输出 first-frame、freeze、decode errors、output fps。 |
| P2-M9 签收回归 | 聚合所有 case 和门禁。 | 一条命令生成最终报告；失败非零退出。 | report 可直接定位失败发生在 signaling/UDP/RTP/RTCP/SDK/encoder/sink。 |

### 13.0.1 建议验收命令

```bash
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/path/to/webrtc_qos_sdk/dist \
  -DBUILD_TESTS=ON

cmake --build build-webrtc-qos-plain \
  --target mediasoup-sfu webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_tests \
  -j"$(nproc)"

./build-webrtc-qos-plain/mediasoup_tests --gtest_filter='*Ortc*:*Plain*'

scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --cases baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover \
  --report-dir docs/generated
```

如果脚本名后续调整，必须在本文档和 `docs/README.md` 同步更新。

### 13.1 功能验收

- push/play target 编译通过。
- SFU + push + play 本地 smoke 通过。
- push `plainPublish enableAudio=false` 成功。
- play 只收到 video consumer。
- play selected consumer `twccExtId != 0`。
- browser receiver 能看到画面。
- native play 能 decode AU。

### 13.2 QoS 验收

- delay case RTT 上升。
- loss case NACK/retransmission 增长。
- bandwidth drop case target bitrate 下探。
- recovery case target bitrate 回升。
- PLI case push 能 force IDR。
- `Process()` tick gap 不超过 2 秒。
- 无长时间 RTP/AU 中断。

### 13.3 观测验收

- push/play adapter log 写文件。
- SDK runtime logs/metrics/alerts 写文件。
- 每条关键日志带 room/peer/transport/producer/consumer/ssrc/pt/ext id。
- smoke report 汇总 PASS/FAIL/SKIP。
- 异常能从 report 定位到信令、UDP、RTP、RTCP、SDK、encoder 或 sink。

### 13.4 静态门禁

```text
webrtc-qos-plain-push-client and webrtc-qos-plain-play-client must not depend on:
  client/qos
  client/sendsidebwe
  client/ccutils
  RtcpHandler.h
  NetworkThread.h
  SenderTransportController.h
  H264Packetizer
  Vp8Packetizer
  PublisherQosController
```

## 14. 风险和处理

| 风险 | 处理 |
|---|---|
| consumer 仍无 TWCC ext | 先定位 server consume 参数生成；保留 play 降级但 QoS 主链路验收不通过。 |
| play `rtcpPacketsOut=0` | 不在 adapter 自研 RTCP/TWCC；在 SDK play facade 补周期性 RR/TWCC 输出和 counter 后再签收 P2-M1。 |
| SDK dist 版本不一致 | P2-M2 明确重新发布 SDK dist，并在 smoke 中检查 `enabled=true`。 |
| netem 权限不足 | case 标记 SKIP 并记录原因；不算 PASS。 |
| x264 CPU 占用高 | 基线分辨率先用 640x360/30fps，必要时降到 320x180/15fps。 |
| PLI force IDR 不稳定 | 加 encoder 级日志和 AU keyframe 标记，验收看 1 秒内 IDR。 |
| 浏览器自动化不稳定 | browser smoke 独立成可重试 case，不阻塞 native QoS harness 的基础报告。 |
| V4L2 环境缺设备 | V4L2 case 可 SKIP；synthetic 和 MP4 decode loop 是必跑。 |
| video-only 影响旧 plain-client | `enableAudio` 默认保持旧行为，新客户端显式传 `false`。 |

## 15. 完成定义

第二期完成必须同时满足：

- 文档中的 P2-M1 到 P2-M9 全部完成；只有环境依赖型 case 允许 SKIP。
- 本地 native smoke PASS。
- 弱网 smoke 关键 case PASS。
- 浏览器 receiver smoke PASS。
- SDK runtime 文件输出启用。
- 实时 x264 encoder 能被 SDK adaptation 控制。
- 所有结果写入报告。
- 新客户端仍不链接旧自研 QoS。

不允许的完成方式：

- 只说“手动看起来正常”，没有自动化命令或报告。
- 弱网 case 因权限失败但记为 PASS。
- metrics/alerts 文件没落盘，只靠控制台日志。
- consumer `twccExtId=0` 仍签收 QoS 主链路。
- play `rtcpPacketsOut=0` 或 SDK TWCC counter 缺失时签收 QoS 主链路。
- encoder adaptation 只打印 target，不实际改 encoder bitrate/fps/keyframe。
- 浏览器 smoke 没跑，却把 native play 闭环当作浏览器兼容性。

最终签收报告必须包含：

- commit id。
- 构建命令和 SDK dist 路径。
- 每个 case 的 PASS/FAIL/SKIP。
- 核心指标表。
- alerts 汇总。
- 失败链路定位。
