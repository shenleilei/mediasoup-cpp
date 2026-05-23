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
- 旧 SDK dist 包未暴露 runtime logs/metrics/alerts 配置字段；当前 dist 已更新，但需要 smoke gate 固化防回归。
- MP4 copy path 不能根据 SDK adaptation 改 bitrate/fps，也不能响应 PLI 强制 IDR；当前已补 synthetic+x264 最小实时编码路径，并已在 delay/loss/bandwidth/recovery netem 下验证 QoS/RTCP/QoE 不崩溃；MP4 decode-loop baseline 已补充验证，V4L2 仍需继续验收。
- `plainPublish` 当前强制创建 dummy audio producer，服务端语义不干净。
- 弱网 smoke harness 已支持 `--enable-netem` 并完成 baseline、delay、loss、bandwidth、recovery 短测；浏览器接收尚未自动化。

当前实施进展：

- P2-M1a 已修复 ORTC consumable RTP parameters 不携带 header extension 的问题。
- native smoke 已验证 video consumer `twccExtId=5`，不再走 `consumer_without_twcc_ext` 降级。
- push/play adapter 已补 RTCP 边界计数日志。
- P2-M4 已支持 `plainPublish enableAudio=false`，新 push 默认 video-only，旧请求保持 audio 默认兼容。
- 当前 SDK play facade 已生成周期性 RR/TWCC feedback；本地 baseline smoke 已验证 `qosMainline=PASS` 和 `sdkRuntimeObservability=PASS`。
- P2-M5/M6 已落地最小 synthetic+x264 路径：`RealtimeH264Source` 生成 raw frame，libx264 输出 Annex-B AU，push runtime 应用 SDK encoder adaptation；本地 synthetic baseline smoke 已验证 `encoderRuntime=PASS`，SDK keyframe request 到 IDR 输出最大延迟 `0us`。
- P2-M6 已补充 MP4 decode-loop baseline：`Mp4DecodeH264Source` 解码 MP4 video frame 后重新 x264 编码，报告 `encoderRuntime=PASS`、`nativeDecodeQoe=PASS`，baseline `pushedAu=357`、`decodedFrames=357`、`decodeErrors=0`。
- P2-M3/P2-M8 已完成真实 netem 短测：`baseline`、`delay_100ms`、`loss_2pct`、`bandwidth_600k`、`drop_recover` 全部 PASS；100ms delay RTT avg/max `109.58/242ms`，bandwidth targetBps min/max `300000/1693914`，recovery targetBps min/max `300000/2023706`，所有 case decodeErrors=0，报告 `weakNetworkCoverage=PASS`。

第二期目标是把第一期从“最小可跑”推进到“可验证、可观测、可调优、可接真实输入”的状态。

## 2. 第二期总目标

第二期对外只有一个阶段，内部拆成多个里程碑。

核心目标：

1. 补齐 QoS 主链路：TWCC、RTCP feedback、target bitrate 下探/恢复都可验证。
2. 建立弱网自动化：baseline / delay / loss / bandwidth drop / recovery 短测可重复运行。
3. 补齐观测体系：日志、metrics、alerts 写文件，排障字段完整。
4. 清理服务端 Plain 信令语义：支持 video-only publish，不再依赖 dummy audio。
5. 接入实时编码器：SDK adaptation 能驱动 encoder bitrate/fps/keyframe。
6. 扩展输入和验证：支持 MP4 decode-loop baseline、V4L2/camera 输入、浏览器 receiver 和 native decode/QoE 验证。

第二期验收口径：

- 不是只要求 play 有 AU 输出。
- 必须证明 QoS 控制信号在弱网下生效，并且日志能解释发生了什么。

第二期每个任务都必须同时满足三类门禁：

- 可实施性：明确要改哪些文件、引入哪些模块、依赖哪些前置条件。
- 可验证性：明确用什么命令或自动化 case 验收，PASS/FAIL/SKIP 口径固定。
- 可观测性：明确日志、metrics、alerts 和报告字段，失败时能定位到具体链路段。

不满足这三类门禁的功能不进入第二期 scope。

落地格式固定为：

- 实施：写清楚修改文件、入口参数、依赖包、兼容策略和失败返回。
- 验证：给出可直接执行的命令，报告中每个 case 只能是 `PASS`、`FAIL` 或 `SKIP`。
- 观测：写清楚 adapter log、SDK runtime metrics/alerts、smoke report 字段和定位链路。

每个里程碑 review 时必须填满下面四项，不允许只写方向：

| 字段 | 必填内容 | 不合格示例 |
|---|---|---|
| 实施边界 | 具体文件、模块、CLI、配置、依赖和兼容策略 | “补一下 QoS” |
| 验证入口 | 可复制执行的命令、case 名、PASS/FAIL/SKIP 口径 | “本地看起来正常” |
| 观测入口 | log 文件、metrics 字段、alerts 字段、报告字段 | “stdout 里能看到” |
| 退出条件 | 该里程碑什么时候算完成，什么时候必须停下修前置 | “差不多可用” |

方案 review 的最低标准：

- 不能落到文件和接口的任务，不进入开发。
- 没有自动化验证或明确 SKIP 规则的任务，不进入签收。
- 不能从日志、metrics、alerts 或报告定位失败链路的任务，不进入 P2 完成范围。

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

### 4.3 文件级交付边界

第二期每类改动的默认落点如下。review 时如果偏离这些路径，需要在 PR 或提交说明中解释原因。

| 交付项 | mediasoup-cpp 落点 | SDK 落点 | 不允许 |
|---|---|---|---|
| Plain 信令 | `src/RoomService*.cpp`、`src/SignalingServer*.cpp`、相关集成测试 | 无 | 在客户端绕过服务端信令语义。 |
| push adapter | `client/webrtc_qos_plain_client/push/*` | 只消费 public facade | include SDK internal header。 |
| play adapter | `client/webrtc_qos_plain_client/play/*` | 只消费 public facade | 自研 jitter buffer / NACK / TWCC。 |
| 公共 CLI/UDP/log glue | `client/webrtc_qos_plain_client/common/*` | 无 | 把业务状态藏在全局变量或 stdout。 |
| 弱网 harness | `scripts/run_webrtc_qos_plain_p2_smoke.sh`，后续可拆到 `client/webrtc_qos_plain_client/harness/*` | 无 | 手工跑 case 后口头签收。 |
| 报告 | `docs/generated/webrtc-qos-plain-p2-smoke-report.{json,md}` | 无 | 只保留临时目录日志。 |
| SDK public API 缺口 | mediasoup-cpp 先记录缺口，不在 adapter 绕实现 | `include/`、`src/`、`dist/` | 在 mediasoup-cpp 复制 RTCP/TWCC 算法。 |

每次交付至少包含：

- 一条构建或单测命令。
- 一条动态 smoke 或明确说明为什么该阶段只能静态验证。
- 一处可追溯观测输出：adapter log、SDK jsonl、alerts 或 generated report。

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

当前实施口径：

- `scripts/run_webrtc_qos_plain_p2_smoke.sh` 是当前 P2 harness 入口。
- 默认不加 `--enable-netem` 时只签收 baseline 主链路；弱网 coverage 明确为 `SKIP`。
- 加 `--source synthetic` 时使用 realtime x264 source，报告增加 `encoderRuntime` gate 和 encoder AU/keyframe 指标。
- 加 `--enable-netem` 时才允许把 delay/loss/bandwidth/recovery 计入弱网 PASS；权限或网卡不满足时只能记 `SKIP`。

## 7. P2-C：观测、日志、告警

### 7.0 统一落盘规范

第二期不再把 `std::cout` / `printf` 当作正式观测面。允许测试脚本采集 stdout/stderr
作为附件，但正式排障入口必须是文件日志、metrics jsonl、alerts jsonl 和 smoke report。

默认文件布局：

```text
<artifact-root>/<run-id>/<case>/
  sfu.log
  push.stdout.log
  play.stdout.log
  push/
    push.log
    push_metrics.*.jsonl
    push_alerts.*.jsonl
    push_runtime.*.log
  play/
    play.log
    play_metrics.*.jsonl
    play_alerts.*.jsonl
    play_runtime.*.log
  netem.log
  case-summary.json
```

要求：

- adapter 使用 spdlog 文件日志；stdout 只作为人工调试和 harness 附件。
- SDK runtime logs/metrics/alerts 必须通过 public config 启用，不能在 adapter 里猜 SDK 内部状态。
- 每条结构化日志至少包含 `ts`、`level`、`component`、`event`、`roomId`、`peerId`、`trackId/ssrc`、`status`。
- metrics 用 jsonl，字段名稳定；新增字段只能追加，不能改旧字段语义。
- alerts 用 jsonl，字段至少包含 `severity`、`code`、`message`、`component`、`roomId`、`peerId`、`recoverable`。
- smoke report 汇总关键字段，不替代原始日志；失败时必须保留原始 artifact 目录。

### 7.1 SDK dist 更新

已更新的 dist 包要求：

- include 中暴露 `RuntimeLogConfig` / `RuntimeMetricsConfig` / `RuntimeAlertConfig`。
- play facade 输出周期性 TWCC feedback 和 RR。
- push facade 统计收到的 TWCC feedback 和 RR。
- `QosSnapshot` / metrics jsonl 输出 RR/TWCC counters。
- CMake package target 不变。
- role bundle target 不变。
- mediasoup-cpp 仍通过 `CMAKE_PREFIX_PATH` 引入，不直接引用 SDK source。

验收：

- push 日志出现 `sdk_runtime_files role=push enabled=true`。
- play 日志出现 `sdk_runtime_files role=play enabled=true`。
- `push_metrics.*.jsonl` 存在且持续写入。
- `play_metrics.*.jsonl` 存在且持续写入。
- alerts 文件存在；无 alert 时也写启动元信息。
- smoke gate `sdkRuntimeObservability=PASS`。

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

### 7.4 监控告警映射

第二期先实现“本地可观测 + 报告门禁”，不强依赖生产 Prometheus/Grafana。字段设计要能平滑映射到后续监控系统。

| 监控项 | 来源 | report 字段 | 告警规则 |
|---|---|---|---|
| 信令连接状态 | adapter log | `signalingConnected` | join/publish/subscribe 失败直接 case FAIL。 |
| UDP hard error | adapter log / metrics | `udpSendErrors` | 任意 hard error FAIL。 |
| RTCP feedback 输入 | push metrics | `pushRtcpFeedbackPacketsIn` | baseline 为 0 时 QoS 主链路 FAIL。 |
| play RTCP 输出 | play metrics | `playRtcpPacketsOut` | baseline 为 0 时 QoS 主链路 FAIL。 |
| SDK TWCC/RR counter | SDK metrics jsonl | `transportFeedbackCountMax`、`receiverReportCountMax` | 缺失或不增长时 SDK runtime observability FAIL。 |
| target bitrate | SDK snapshot / push metrics | `targetBitrateMin/Avg/Max/Last` | bandwidth 不下探或 recovery 不回升时 FAIL。 |
| pacer backpressure | SDK snapshot / push metrics | `droppedFrames` | bandwidth/recovery 下可非 0，但必须无 fatal alert 且 QoE 继续输出。 |
| encoder 状态 | adapter encoder metrics | `encoderAccessUnits`、`encoderKeyframes`、`currentBitrateBps`、`currentFps` | AU/keyframe 为 0 或 adaptation 无法落到 encoder 时 FAIL。 |
| RTP/AU 连续性 | play metrics | `rtpPackets`、`outputAu` | 长时间无 RTP/AU FAIL。 |
| QoE 解码 | qoe metrics | `decodedFrames`、`decodeErrors`、`freezeCount`、`outputFps` | baseline decode error 非 0 FAIL；弱网恢复后 decoded frames 不增长 FAIL。 |

告警分级：

| 级别 | 含义 | smoke 行为 |
|---|---|---|
| `info` | 状态变化或恢复事件 | 记录，不影响 PASS。 |
| `warn` | 可恢复异常，如 NACK 增长、pacer backpressure drop | 进入报告；如果命中 case allowlist 可 PASS。 |
| `error` | 链路失败、SDK fatal status、UDP hard error、长时间无 RTP/AU | case FAIL。 |

报告中必须区分“预期弱网现象”和“不可接受故障”。例如 `bandwidth_600k` 下 dropped frames 可以解释为
pacer backpressure，但 `push_au_failed`、`udp_send_hard_error`、`no_au_output_5s` 不允许被吞掉。

### 7.5 排障链路

P2 失败时按下面顺序定位，不允许直接从现象跳到改 QoS 算法：

| 失败现象 | 第一检查点 | 第二检查点 | 第三检查点 |
|---|---|---|---|
| join/publish/subscribe 失败 | adapter log 的 request/response | SFU log | 信令 schema / room state |
| play 没有 RTP | `selected_consumer` 和 PlainTransport tuple | SFU consumer/transport stats | UDP bind/connect 端口 |
| play 有 RTP 没 AU | SDK play alerts / depacketizer status | H264 PT/SSRC/TWCC ext | keyframe 请求是否发出 |
| push 无 RTCP feedback | play `rtcpPacketsOut` | SFU RTCP 转发 | push RTCP socket 和 SDK input |
| target 不下探 | TWCC/RR counter | loss/RTT metrics | GoogCC / pacer queue metrics |
| target 不恢复 | netem clear 时间 | target bitrate timeline | recovery 窗口是否足够 |
| QoE freeze | decoded frame timeline | AU output timeline | RTP/NACK/retransmission timeline |

每个失败 case 的 report 必须写出 `failedChecks`，并保留对应日志字段。

## 8. P2-D：服务端 Plain 信令清理

### 8.1 Video-only publish

已修复的问题：

- 当前 `plainPublish` 要求 `audioSsrc` 非 0。
- 服务端会创建 dummy audio producer。
- play 为避免 audio auto-subscribe error，被迫声明 Opus capability。

修复：

- `plainPublish` 支持 `enableAudio=false`，此时允许 `audioSsrc` 缺省或为 0。
- response 明确返回 `audioEnabled=false`。
- 新 `webrtc-qos-plain-push-client` 默认发送 `enableAudio=false`，不再发送 `audioSsrc`。
- 未传 `enableAudio` 的旧请求仍默认 `true`，保持旧 plain-client 行为。

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
- targeted integration 已验证旧请求仍创建 audio/video 两个 producer。

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
  -> RealtimeH264Source::ApplyEncoderAdaptation()
  -> libx264 bitrate/fps/keyframe control
  -> VideoPushClient::PushAnnexBAccessUnit()
```

当前最小实现已经落在：

- `client/webrtc_qos_plain_client/push/RealtimeH264Source.{h,cpp}`
- `client/webrtc_qos_plain_client/push/Mp4DecodeH264Source.{h,cpp}`
- `client/webrtc_qos_plain_client/push/WebRtcQosPushRuntime.cpp`
- `client/webrtc_qos_plain_client/common/ClientArgs.{h,cpp}`
- `tests/test_webrtc_qos_realtime_source.cpp`
- `scripts/run_webrtc_qos_plain_p2_smoke.sh`

这个实现先覆盖 synthetic raw frame + CPU x264 baseline，并补齐 MP4 decode-loop baseline；V4L2 仍是后续输入源扩展，不把它算作当前完成。

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

当前已经可用的 CLI 子集：

```bash
webrtc-qos-plain-push-client \
  --input-synthetic=true \
  --encoder=x264 \
  --synthetic-width=320 \
  --synthetic-height=180 \
  --synthetic-fps=15

webrtc-qos-plain-push-client \
  --input=file.mp4 \
  --input-decode-loop=true \
  --loop-input=true \
  --encoder=x264
```

兼容约束：

- 未启用 `--input-synthetic` 或 `--input-decode-loop` 时继续使用 MP4 H264 copy path，`--encoder` 必须为 `copy`。
- 启用 `--input-synthetic` 时 `--encoder` 必须为 `x264`。
- 启用 `--input-decode-loop` 时 `--encoder` 必须为 `x264`，输入 MP4 会先解码为 raw frame，再进入同一套 x264 adaptation 链路。
- 当前 synthetic fps 是输入起点；运行中 `currentFps` 可能被 SDK adaptation 调整，验收看 encoder metrics 里的运行时值。

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

当前最小验收：

```bash
cmake --build build-webrtc-qos-plain \
  --target webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_webrtc_qos_plain_unit_tests \
  -j1

./build-webrtc-qos-plain/mediasoup_webrtc_qos_plain_unit_tests \
  --gtest_filter='WebRtcQosRealtimeSourceTest.*'

scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source synthetic \
  --cases baseline \
  --duration-seconds 6 \
  --report-dir docs/generated

scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source mp4-decode-loop \
  --decode-qoe \
  --cases baseline \
  --duration-seconds 12 \
  --artifact-root /tmp/webrtc-qos-plain-p2-mp4-decode-loop \
  --report-dir docs/generated \
  --report-name webrtc-qos-plain-p2-mp4-decode-loop-report
```

当前 synthetic baseline 报告要求：

- `encoderRuntime=PASS`
- baseline case `PASS`
- encoder `mode=synthetic`
- encoder `name=x264`
- `accessUnits > 0`
- `keyframes > 0`
- `forcedKeyframeRequests > 0`
- `forcedKeyframes > 0`
- `maxForcedKeyframeDelayUs <= 1000000`
- `width/height` 等于 synthetic 输入配置
- `currentFps > 0`
- `currentBitrateBps > 0`

当前 MP4 decode-loop baseline 报告要求：

- `encoderRuntime=PASS`
- `nativeDecodeQoe=PASS`
- baseline case `PASS`
- encoder `mode=mp4_decode_loop`
- encoder `name=x264`
- `accessUnits > 0`
- `keyframes > 0`
- `forcedKeyframeRequests > 0`
- `forcedKeyframes > 0`
- `maxForcedKeyframeDelayUs <= 1000000`
- QoE `decodedFrames > 0`
- QoE `decodeErrors == 0`

当前已在 delay/loss/bandwidth/recovery 真实 netem 下验证反馈闭环、target bitrate 下探、恢复开始回升、PLI/SDK keyframe request 后 1 秒内 IDR，以及 native decode 稳定性；恢复时首帧时间仍需要在后续 browser/输入源扩展中继续细化。

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

当前最小实现已经落在：

- `client/webrtc_qos_plain_client/play/FfmpegDecodeSink.{h,cpp}`
- `client/webrtc_qos_plain_client/play/WebRtcQosPlayRuntime.cpp`
- `client/webrtc_qos_plain_client/common/ClientArgs.{h,cpp}`
- `tests/test_webrtc_qos_decode_sink.cpp`
- `scripts/run_webrtc_qos_plain_p2_smoke.sh`

当前已经可用的 CLI：

```bash
webrtc-qos-plain-play-client \
  --output-null=true \
  --decode-qoe=true
```

当前 smoke 报告新增 gate：

- `nativeDecodeQoe=PASS`
- `decodedFrames > 0`
- `decodeErrors == 0`
- `firstFrameDelayUs >= 0`
- `freezeCount`、`maxFrameGapUs`、`outputFps` 可观测。

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
| P2-M6 | 输入源扩展 | synthetic 和 MP4 decode-loop 已覆盖；V4L2 后续按设备条件补。 |
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

### 12.3 里程碑实施卡

#### P2-M1 QoS 主链路

- 实施：修 ORTC header extension 透传；push/play runtime 补 RTCP 边界计数；只消费 SDK RR/TWCC public counter。
- 验证：ORTC targeted test、native baseline smoke、`selected_consumer twccExtId=5`、play RTCP out > 0、push RTCP in > 0。
- 观测：`selected_consumer`、`play_runtime_started transportCcExtId`、`push_metrics rtcpFeedbackPacketsIn/rttMs/loss`、SDK RR/TWCC counters。
- 退出条件：consumer `twccExtId=0`、play `rtcpPacketsOut=0` 或 SDK TWCC counter 缺失时不得继续签收弱网 QoS。

#### P2-M2 SDK runtime dist

- 实施：升级 `/root/webrtc_qos_sdk/dist/linux-x86_64`，mediasoup-cpp 通过 `CMAKE_PREFIX_PATH` 消费；adapter 做字段探测和降级日志。
- 验证：构建通过；启动日志 `sdk_runtime_files enabled=true`；push/play metrics 和 alerts 文件存在。
- 观测：`push_metrics.*.jsonl`、`play_metrics.*.jsonl`、`push_alerts.*.jsonl`、`play_alerts.*.jsonl`。
- 退出条件：只要 runtime 文件不能启用，P2-M2 失败；不能用 stdout 代替。

#### P2-M3 弱网 harness

- 实施：脚本统一启动 SFU、push、play、netem、artifact 收集和 report 生成；所有 case 统一 PASS/FAIL/SKIP。
- 验证：`baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover` 可单独或组合执行；无 netem 权限时弱网 case SKIP。
- 观测：report 输出 RTT/loss/target bitrate/NACK/PLI/retransmission/droppedFrames/QoE/alerts。
- 退出条件：case 不能复现、不能生成 report、或 SKIP 被记成 PASS 时停止。

#### P2-M4 video-only publish

- 实施：`plainPublish enableAudio=false`，新 push 默认 video-only；旧请求不传 `enableAudio` 时保持原行为。
- 验证：targeted integration 验证新旧请求；smoke 验证无 audio producer、play 只收到 video consumer。
- 观测：`plain_publish_ok audioEnabled=false`、stats report producer 列表、无 audio auto-subscribe error。
- 退出条件：破坏旧 plain-client 兼容或仍创建 dummy audio producer 时不得合入。

#### P2-M5 实时 x264 encoder

- 实施：synthetic raw frame -> x264 baseline/zerolatency -> Annex-B AU；push runtime 每 tick 应用 SDK adaptation。
- 验证：单测覆盖 encoder 输出；baseline smoke `encoderRuntime=PASS`；bandwidth case 证明 bitrate/fps/keyframe adaptation 实际落到 encoder。
- 观测：`encoder_metrics accessUnits/keyframes/currentBitrateBps/currentFps/recreateCount/bitrateChangeCount/fpsChangeCount`。
- 退出条件：只打印 target 而不改 encoder、PLI 后不能 force IDR、或 encoder metrics 缺失时失败。

#### P2-M6 输入源扩展

- 实施：保留 MP4 H264 copy path；新增 synthetic 必跑路径；新增 MP4 decode-loop path；后续补 V4L2 source。
- 验证：synthetic 在 CI/普通 CPU 环境必跑；MP4 decode-loop baseline 必跑；无 `/dev/video*` 时 V4L2 case SKIP。
- 观测：report 记录实际 source mode；synthetic/MP4 decode-loop 输出 source frame count、fps、encoder AU/keyframe、forced-IDR；V4L2 后续补 capture errors。
- 退出条件：输入源失败不能静默降级到其他源；必须在 report 里显示实际 source。

#### P2-M7 浏览器 receiver

- 实施：复用现有 browser signaling，增加最小 receiver smoke，消费新 push 发布的 video。
- 验证：browser 收到 `newConsumer`，video frames/packets 增长，必要时保存截图或 stats。
- 观测：browser inbound-rtp stats、console error、SFU consumer stats、case artifact。
- 退出条件：native play 通过不能替代 browser receiver；浏览器未跑时只能标记未覆盖。

#### P2-M8 native decode/QoE

- 实施：play AU 输出接 FFmpeg H264 decoder，再汇总 first frame、decode errors、freeze 和 output fps。
- 验证：baseline decode error 为 0；弱网恢复后 decoded frames 继续增长；复杂 VMAF/PSNR 不进入 P2。
- 观测：`qoe_metrics decodedFrames/decodeErrors/firstFrameDelayUs/freezeCount/maxFrameGapUs/outputFps`。
- 退出条件：只验证 AU 不验证 decode 时，不能签收 QoE。

#### P2-M9 签收回归

- 实施：聚合构建、单测、静态门禁、native smoke、弱网 smoke、browser smoke 和 report。
- 验证：一条最终命令生成 `docs/generated/webrtc-qos-plain-p2-smoke-report.{json,md}`，失败返回非 0。
- 观测：report 包含 commit id、SDK dist 路径、case 状态、核心指标、alerts、failedChecks 和 artifact 路径。
- 退出条件：任何非环境依赖型 case 未跑或失败时，P2 不完成。

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
| P2-M4 video-only publish | 修改 `RoomService::plainPublish()`、signaling dispatcher、push signaling；保持旧请求兼容。 | `enableAudio=false` 时无 audio producer；旧 `audioSsrc` 请求仍通过；P2-M4 targeted integration 和 smoke 通过。 | SFU stats/report 中 `audioEnabled=false`；无 dummy audio consumer 日志。 |
| P2-M5 realtime x264 encoder | 已新增 `RealtimeH264Source` 最小 x264 encoder，并在 push runtime 调 `ApplyEncoderAdaptation()`；后续再拆 `H264EncoderAdapter` 时保持同一外部行为。 | 当前 synthetic baseline、bandwidth/recovery netem 和单测已验证 encoder AU/keyframe/adaptation、target bitrate 下探、恢复开始回升，以及 SDK keyframe request 后 1 秒内 IDR。 | 当前输出 `encoder_metrics` 的 bitrate/fps/AU/keyframe/recreate/change/forced-IDR counters；后续补 frameDrop 和 keyframe alert。 |
| P2-M6 输入源扩展 | 已新增 synthetic raw frame source 和 MP4 decode-loop source；V4L2 source 待实现。 | 当前 synthetic 必跑；MP4 decode-loop baseline 已通过；无 V4L2 设备时 V4L2 case SKIP。 | 当前 encoder/source metrics 输出 frame count、input fps、AU/keyframe/forced-IDR；report 输出 `sourceMode=synthetic` 或 `sourceMode=mp4-decode-loop`。 |
| P2-M7 浏览器兼容 | 新增 browser receiver smoke，复用现有 web/signaling。 | 浏览器收到 `newConsumer`、video frames 增长、截图或 stats 通过。 | browser stats 附到 report，失败带 console/error 摘要。 |
| P2-M8 native decode/QoE | 已新增 `FfmpegDecodeSink` 解码和 QoE 指标；复杂 `QoeProbe` 可后续独立扩展。 | 当前 baseline/delay/loss/bandwidth/recovery 已验证 decode error 为 0、decoded frames 增长；browser QoE 仍待覆盖。 | 当前 report 输出 first-frame、freeze、decode errors、output fps。 |
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
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source synthetic \
  --decode-qoe \
  --cases baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover \
  --duration-seconds 12 \
  --enable-netem \
  --report-dir docs/generated

scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source mp4-decode-loop \
  --decode-qoe \
  --cases baseline \
  --duration-seconds 12 \
  --artifact-root /tmp/webrtc-qos-plain-p2-mp4-decode-loop \
  --report-dir docs/generated \
  --report-name webrtc-qos-plain-p2-mp4-decode-loop-report
```

当前脚本实现状态：

- 默认安全模式只运行 `baseline`，弱网 case 在未传 `--enable-netem` 时写为 `SKIP`，不计 PASS。
- 需要真实弱网验证时显式传 `--enable-netem`；脚本会预检 `tc`、root/CAP_NET_ADMIN 和目标网卡。
- 默认短测报告写入 `docs/generated/webrtc-qos-plain-p2-smoke-report.{json,md}`；专项报告可用 `--report-name` 指定 basename，例如 MP4 decode-loop baseline 写入 `docs/generated/webrtc-qos-plain-p2-mp4-decode-loop-report.{json,md}`。
- 当前机器 synthetic+QoE+netem 短测结果：`baseline PASS`、`delay_100ms PASS`、`loss_2pct PASS`、`bandwidth_600k PASS`、`drop_recover PASS`，`qosMainline PASS`，`sdkRuntimeObservability PASS`，`encoderRuntime PASS`，`nativeDecodeQoe PASS`，`weakNetworkCoverage PASS`。
- 当前机器 MP4 decode-loop baseline 短测结果：`baseline PASS`，`qosMainline PASS`，`sdkRuntimeObservability PASS`，`encoderRuntime PASS`，`nativeDecodeQoe PASS`；弱网 coverage 未跑，overall 为 `PARTIAL`。

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
| play `rtcpPacketsOut=0` | QoS 主链路 gate 直接 FAIL；不在 adapter 自研 RTCP/TWCC，必须回到 SDK play facade 修复。 |
| SDK dist 版本不一致 | smoke 检查 `sdk_runtime_files enabled=true`、runtime 文件存在、SDK RR/TWCC counter 非 0；任一缺失即 P2-M2 FAIL。 |
| netem 权限不足 | case 标记 SKIP 并记录原因；不算 PASS。 |
| x264 CPU 占用高 | 基线分辨率先用 640x360/30fps，必要时降到 320x180/15fps。 |
| PLI force IDR 不稳定 | 加 encoder 级日志和 AU keyframe 标记，验收看 1 秒内 IDR。 |
| 浏览器自动化不稳定 | browser smoke 独立成可重试 case，不阻塞 native QoS harness 的基础报告。 |
| V4L2 环境缺设备 | V4L2 case 可 SKIP；synthetic 和 MP4 decode loop 是必跑。 |
| video-only 影响旧 plain-client | `enableAudio` 默认保持旧行为，新客户端显式传 `false`。 |

### 14.1 风险里程碑

| 节点 | 主要风险 | 必须观测到的证据 | 停工条件 |
|---|---|---|---|
| M1 前 | TWCC header extension 或 RTCP feedback 不闭环 | `twccExtId=5`、play RTCP out、push RTCP in、SDK TWCC/RR counter | 任一为 0 时先修主链路，不做 encoder/browser 扩展。 |
| M2 前 | SDK dist 与 adapter 编译期/运行期不一致 | runtime files enabled、metrics/alerts 文件存在 | 不能启用文件观测时不跑弱网签收。 |
| M3 前 | netem 环境不可控 | `netem.log` 记录 apply/clear，report 记录 SKIP reason | 无权限时只允许 SKIP，不允许 PASS。 |
| M5 前 | adaptation 只停留在 target 数字 | encoder bitrate/fps/keyframe metrics 变化 | encoder 未实际变化时不签收 QoS 效果。 |
| M7 前 | native 自闭环掩盖浏览器兼容问题 | browser stats 或截图 | browser 未跑时不得声明浏览器可播放。 |
| M9 前 | 结果不可复现或不可排障 | generated report + artifact path + failedChecks | 只有口头结果或临时日志缺失时不完成。 |

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
