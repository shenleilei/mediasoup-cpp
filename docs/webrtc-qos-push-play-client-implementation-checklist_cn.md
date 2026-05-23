# WebRTC QoS SDK Plain 推拉流客户端实现验收清单

> 状态：P1 已完成本地短链路验收；P2 已完成 consumer TWCC ext 修复、adapter RTCP 边界计数、SDK play RR/TWCC 输出、SDK runtime 文件观测、video-only publish、P2 smoke 报告脚本、synthetic+x264 最小实时编码路径、native decode/QoE baseline，以及真实 baseline/delay/loss/bandwidth/recovery netem 短测。浏览器画面、MP4 decode loop 和 V4L2 仍需后续验证。

## 1. 已实现项

| 文档要求 | 当前实现 | 证据 |
|---|---|---|
| 新建同一目录下的 push/play 客户端 | `client/webrtc_qos_plain_client/{common,push,play}` | `find client/webrtc_qos_plain_client -type f` |
| 两个可执行 target | `webrtc-qos-plain-push-client`、`webrtc-qos-plain-play-client` | `cmake --build build-webrtc-qos-plain --target ...` 通过 |
| 复用现有 WebSocket 信令 | 复用 `client/WsClient.cpp` | CMake target source 包含 `client/WsClient.cpp` |
| push 复用 `join` + `plainPublish` | `PushSignalingSession` | 新客户端请求含 `videoCodec=h264`、`videoSsrc`、`enableAudio=false` |
| play 复用 `join` + `plainSubscribe` | `PlaySignalingSession` | `join` 带最小 Plain receive capabilities，`plainSubscribe(recvIp, recvPort)` |
| play 借鉴原 Play 的 `newConsumer` | `PlaySignalingSession` 缓存 `newConsumer` 并单路选择 | `pendingConsumers_` + `TakeSelectedConsumer()` |
| play 借鉴 `requestConsumerKeyFrame` | 选中 consumer 后立即请求，1s 后再请求一次 | `play/main.cpp` |
| push 只用 `VideoPushClient` | `WebRtcQosPushRuntime` 调 `CreateVideoPushClient` | 无旧 QoS/packetizer 依赖 |
| play 只用 `VideoPlayClient` | `WebRtcQosPlayRuntime` 调 `CreateVideoPlayClient` | 无自研 receiver/jitter/RTCP |
| push H264 MP4 copy path | `H264AnnexBSource` 使用 `h264_mp4toannexb` | 输入非 H264 直接失败 |
| play AU 输出 | `AnnexBSink` 支持 `--output-au` / `--output-null` | `decoded_access_unit_output` 写 sink |
| UDP 地址规则 | push/play 默认 `--server-ip + response.port`，可用 `--media-remote-ip` 覆盖 | `PlainUdpTransport` + CLI |
| play bind 先于 subscribe | `play/main.cpp` 先 `udp.Bind()` 再 `ConnectJoinAndSubscribe()` | 符合文档时序 |
| RTP/RTCP 最小分类 | `RtpRtcpClassifier` 只按 version/PT 分类 | 不解析 NACK/PLI/TWCC 内容 |
| runtime loop 持续 `Process()` | push/play 每 tick 调用 SDK `Process()` | `WebRtcQosPushRuntime` / `WebRtcQosPlayRuntime` |
| runtime loop 持续处理 WS notification | push/play loop 调 `DispatchNotifications()` | 支持 `qosPolicy`/`newConsumer` 等通知观测 |
| 不复用旧自研 QoS/RTCP/packetizer | 源码和 target 不引用旧模块 | `rg` 门禁仅命中 README 的说明文本 |
| CLI 兼容 `--key value` 和 `--key=value` | `ClientArgs` | smoke 使用 `--server-ip=127.0.0.1` 等参数通过 |
| play 兼容历史 dummy audio producer | receive capabilities 暂保留 Opus，runtime 只选择 video | P1 历史 smoke 不再出现 audio auto-subscribe error |
| consumer TWCC ext 透传 | `ortc::getConsumableRtpParameters()` 保留 producer/router 均支持的 header extension | P2 smoke：`selected_consumer ... twccExtId=5` |
| consumer 无 TWCC ext 降级 | 保留 `consumer_without_twcc_ext` 日志，兼容异常环境继续启动 `VideoPlayClient` | 降级可播放，但不允许作为 QoS 主链路 PASS |
| push RTCP feedback 输入计数 | `WebRtcQosPushRuntime` 记录 `rtcpFeedbackPacketsIn/BytesIn/Failures` | P2 smoke：`rtcpFeedbackPacketsIn=66 ... Failures=0` |
| play RTCP 输出计数 | `WebRtcQosPlayRuntime` 记录 `rtcpPacketsOut/BytesOut/SendFailures` | P2 smoke：`rtcpPacketsOut=98 ... SendFailures=0` |
| SDK RR/TWCC counter | `webrtc_qos_sdk` play 生成 RR/TWCC，push 统计收到的 RR/TWCC | P2 smoke：play `transportFeedbackCountMax=92`、`receiverReportCountMax=6`；push `transportFeedbackCountMax=50`、`receiverReportCountMax=4` |
| SDK runtime 文件观测 | dist 暴露 `logging/metrics/alerts`，adapter 自动启用 SDK runtime 文件 | P2 smoke：push/play 均 `sdk_runtime_files enabled=true`，各生成 log/metrics/alerts 文件 |
| video-only plain publish | 服务端支持 `enableAudio=false`；新 push 默认不发 `audioSsrc`；旧请求默认仍启用 audio | P2-M4 smoke：`audioEnabled=false` 且 play 只收到 video consumer |
| P2 smoke 报告脚本 | `scripts/run_webrtc_qos_plain_p2_smoke.sh` 统一启动 SFU/push/play/netem 并生成报告 | `docs/generated/webrtc-qos-plain-p2-smoke-report.{json,md}` |
| synthetic x264 实时编码 | `RealtimeH264Source` 生成 synthetic YUV420 frame，经 libx264 baseline/zerolatency 编码成 Annex-B AU | `WebRtcQosRealtimeSourceTest.*` 通过 |
| encoder adaptation 接入 | push runtime 每 tick 调 `GetEncoderAdaptation()` 并应用到 x264 bitrate/fps/keyframe | synthetic smoke：`encoderRuntime=PASS`，encoder `accessUnits=332`、`keyframes=12`、`currentFps=30` |
| encoder runtime 观测 | push log 输出 `encoder_metrics`，报告解析 encoder AU/keyframe/fps/bitrate/recreate/change/forced-IDR counters | `docs/generated/webrtc-qos-plain-p2-smoke-report.md` 的 `encoderRuntime=PASS`，`forcedKeyframeRequests=1`、`forcedKeyframes=1`、`maxForcedKeyframeDelayUs=0` |
| native decode/QoE baseline | play 端 `--decode-qoe` 启用 `FfmpegDecodeSink`，对 SDK 输出的 Annex-B AU 做 H264 解码 | `WebRtcQosDecodeSinkTest.*` 通过；smoke `nativeDecodeQoe=PASS` |
| QoE runtime 观测 | play log 输出 `qoe_metrics` / `qoe_runtime_stopped`，报告解析 decoded frames、decode errors、first frame、freeze、output fps | 当前报告：baseline decodedFrames=358、decodeErrors=0、freezeCount=0、outputFps=30.08；所有已跑 case decodeErrors=0 |
| delay/loss/bandwidth/recovery netem 弱网短测 | `scripts/run_webrtc_qos_plain_p2_smoke.sh --enable-netem --cases baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover` | 当前报告：`5 / 5 PASS`，`weakNetworkCoverage=PASS`，delay RTT avg/max `109.58/242ms`，bandwidth targetBps min/max `300000/1693914`，recovery targetBps min/max `300000/2023706`，decodeErrors=0 |

## 2. 当前验证命令

```bash
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/root/webrtc_qos_sdk/dist/linux-x86_64 \
  -DBUILD_TESTS=ON

cmake --build build-webrtc-qos-plain \
  --target webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_webrtc_qos_plain_unit_tests \
  -j1

./build-webrtc-qos-plain/mediasoup_webrtc_qos_plain_unit_tests \
  --gtest_filter='WebRtcQosRealtimeSourceTest.*:WebRtcQosDecodeSinkTest.*'

g++ -std=c++17 -Isrc -I. -Ithird_party/nlohmann_json/include \
  -isystem third_party/googletest/googletest/include \
  tests/test_ortc.cpp \
  third_party/googletest/googletest/src/gtest-all.cc \
  third_party/googletest/googletest/src/gtest_main.cc \
  -Ithird_party/googletest/googletest \
  -pthread -o /tmp/test_ortc_p2

/tmp/test_ortc_p2 --gtest_filter='*TransportCc*:*Consumable*'

rg -n "client/qos|sendsidebwe|ccutils|RtcpHandler|NetworkThread|SenderTransportController|H264Packetizer|Vp8Packetizer|PublisherQosController|PacketizeAnnexB" \
  client/webrtc_qos_plain_client CMakeLists.txt

MEDIASOUP_TEST_SFU_BIN=./build-webrtc-qos-plain/mediasoup-sfu \
MEDIASOUP_TEST_WORKER_BIN=./mediasoup-worker \
./build-webrtc-qos-p2/mediasoup_qos_integration_tests \
  --gtest_filter='QosIntegrationTest.PlainPublishSupportsVideoOnlyAndKeepsLegacyAudioDefault:QosIntegrationTest.PlainPublishReplacesOldTransportAndUsesBaselineCodec:QosIntegrationTest.PlainPublishRejectsDuplicateVideoSsrcs'

scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source synthetic \
  --decode-qoe \
  --cases baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover \
  --duration-seconds 12 \
  --enable-netem \
  --artifact-root /tmp/webrtc-qos-plain-p2-fullweak-final3 \
  --report-dir docs/generated
```

当前结果：

- CMake configure 通过。
- push/play 两个 target 编译通过。
- `mediasoup_webrtc_qos_plain_unit_tests --gtest_filter='WebRtcQosRealtimeSourceTest.*:WebRtcQosDecodeSinkTest.*'` 通过。
- ORTC standalone targeted test 通过。
- P2-M4 targeted integration test 通过；非默认 build 目录下通过 `MEDIASOUP_TEST_SFU_BIN` / `MEDIASOUP_TEST_WORKER_BIN` 指定测试 SFU 和 worker。
- P2 synthetic+QoE+netem smoke 脚本短测通过：`baseline`、`delay_100ms`、`loss_2pct`、`bandwidth_600k`、`drop_recover` 均 PASS，`qosMainline=PASS`，`sdkRuntimeObservability=PASS`，`encoderRuntime=PASS`，`nativeDecodeQoe=PASS`，`weakNetworkCoverage=PASS`。
- 旧实现依赖门禁没有命中代码；只命中 `client/webrtc_qos_plain_client/README.md` 的说明文本。

## 3. P1 动态 smoke 结果（历史）

本节记录第一期最小链路 smoke 的历史结果，用来说明当时已经跑通 RTP/RTCP/AU，
但还没有完成 consumer TWCC header extension 透传。当前 P2 结果以 `3.1` 为准。

前置：

- 按仓库 `setup.sh` 的方式生成 `generated/*_generated.h`，共 29 个。
- 下载 `mediasoup-worker` 3.14.6；该文件被 `.gitignore` 忽略，不进入提交。
- 生成 10 秒 H264 Constrained Baseline MP4：`320x180 / 15fps`。

SFU：

```bash
./build-webrtc-qos-plain/mediasoup-sfu \
  --nodaemon \
  --port=33003 \
  --workers=1 \
  --workerThreads=1 \
  --listenIp=127.0.0.1 \
  --announcedIp=127.0.0.1 \
  --workerBin=./mediasoup-worker \
  --noRedisRequired
```

push/play：

```bash
./build-webrtc-qos-plain/webrtc-qos-plain-play-client \
  --server-ip=127.0.0.1 \
  --server-port=33003 \
  --room=smoke-room \
  --peer=smoke-play \
  --listen-ip=127.0.0.1 \
  --advertise-ip=127.0.0.1 \
  --listen-port=41002 \
  --output-null=true

./build-webrtc-qos-plain/webrtc-qos-plain-push-client \
  --server-ip=127.0.0.1 \
  --server-port=33003 \
  --room=smoke-room \
  --peer=smoke-push \
  --input=/tmp/webrtc-qos-plain-smoke/input.mp4 \
  --loop-input=true \
  --video-ssrc=11111111 \
  --audio-ssrc=22222222
```

本地结果：

| 验收点 | 结果 | 证据 |
|---|---:|---|
| SFU ready | PASS | `/readyz` 返回 `ok=true` |
| push `join/plainPublish` | PASS | `plain_publish_ok ... pt=127 twccExtId=5` |
| push SDK runtime | PASS | `push_runtime_started ... udpRemotePort=51003` |
| play `join/plainSubscribe` | PASS | `join_ok ... h264-baseline+opus-compat`，`plain_subscribe_ok` |
| `newConsumer` | PASS | 收到 video consumer；也收到 dummy audio consumer 但未选择 |
| play 选择 video consumer | HISTORICAL PASS | `selected_consumer ... pt=127 twccExtId=0` |
| consumer 无 TWCC ext 降级 | HISTORICAL PASS | `consumer_without_twcc_ext ... headerExtensions=[]` |
| keyframe 请求 | PASS | `request_consumer_keyframe_ok` |
| play RTP/RTCP/AU | PASS | 8 秒 smoke：`rtpPackets=212 rtcpPackets=6 outputAu=120` |
| push 停止统计 | PASS | 8 秒 smoke：`push_runtime_stopped pushedAu=120` |
| SFU audio auto-subscribe error | PASS | 加 Opus 兼容声明后未再出现 `auto-subscribe FAILED` |

更长一次 10 秒 smoke 结果：

- push：`push_runtime_stopped pushedAu=150`
- play：`play_runtime_stopped rtpPackets=252 rtcpPackets=9 outputAu=150`
- play 每秒 metrics 持续输出，未出现 `play_packet_input_failed`。

## 3.1 P2-M1a/M1c 动态 smoke 结果

运行目录：

```text
/tmp/webrtc-qos-plain-p2-m1c
```

本地结果：

| 验收点 | 结果 | 证据 |
|---|---:|---|
| SFU ready | PASS | `/readyz` 返回 `ok=true` |
| consumer TWCC ext | PASS | `new_consumer_notification ... "headerExtensions":[{"id":5,...transport-wide-cc...}]` |
| play 选择 video consumer | PASS | `selected_consumer ... pt=127 twccExtId=5` |
| play runtime TWCC ext | PASS | `play_runtime_started ... transportCcExtId=5` |
| push RTCP feedback 输入 | PASS | `push_runtime_stopped pushedAu=120 rtcpFeedbackPacketsIn=86 rtcpFeedbackBytesIn=2376 rtcpFeedbackFailures=0` |
| push RTT/loss snapshot | PASS | `push_metrics ... rttMs=8 loss=0 rtcpFeedbackPacketsIn=77 ...` |
| play RTP/AU | PASS | `play_runtime_stopped rtpPackets=236 rtcpPackets=7 ... outputAu=120` |
| play RTCP 输出 | 早期 FAIL | 当时 SDK play facade 尚未输出周期性 RR/TWCC；当前结果以 3.3 的 `playRtcpPacketsOut=98` 为准 |

结论：

- P2-M1a 已完成：consumer 不再丢 TWCC header extension。
- P2-M1c adapter 边界计数已完成：push/play 日志包含 RTCP I/O counters。
- P2-M1d 已在后续 SDK dist 更新后完成；当前签收以 3.3 报告为准。

## 3.2 P2-M4 video-only 动态 smoke 结果

运行目录：

```text
/tmp/webrtc-qos-plain-p2-m4
```

本地结果：

| 验收点 | 结果 | 证据 |
|---|---:|---|
| push video-only publish | PASS | `plain_publish_ok ... audioSsrc=0 audioEnabled=false` |
| play 只收到 video consumer | PASS | 只有 `new_consumer_notification ... "kind":"video"`，没有 audio consumer |
| video consumer TWCC ext | PASS | `selected_consumer ... twccExtId=5` |
| statsReport 无 dummy audio producer | PASS | statsReport 中 `p2-push.producers` 只有 video producer |
| RTP/AU 输出 | PASS | `play_runtime_stopped rtpPackets=233 rtcpPackets=7 ... outputAu=118` |
| push RTCP feedback 输入 | PASS | `push_runtime_stopped pushedAu=118 rtcpFeedbackPacketsIn=87 ... Failures=0` |

配套 targeted integration：

- `QosIntegrationTest.PlainPublishSupportsVideoOnlyAndKeepsLegacyAudioDefault`
- `QosIntegrationTest.PlainPublishReplacesOldTransportAndUsesBaselineCodec`
- `QosIntegrationTest.PlainPublishRejectsDuplicateVideoSsrcs`

结论：

- P2-M4 已完成：新 push 默认 `enableAudio=false`，服务端不再创建 dummy audio producer。
- 旧请求不传 `enableAudio` 时仍默认启用 audio，保持旧 plain-client 兼容。

## 3.3 P2-M3 smoke 报告脚本结果

当前报告：

- Markdown：[generated/webrtc-qos-plain-p2-smoke-report.md](./generated/webrtc-qos-plain-p2-smoke-report.md)
- JSON：[generated/webrtc-qos-plain-p2-smoke-report.json](./generated/webrtc-qos-plain-p2-smoke-report.json)

本地短测命令：

```bash
scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --worker-bin ./mediasoup-worker \
  --source synthetic \
  --decode-qoe \
  --cases baseline,delay_100ms,loss_2pct,bandwidth_600k,drop_recover \
  --duration-seconds 12 \
  --enable-netem \
  --artifact-root /tmp/webrtc-qos-plain-p2-fullweak-final3 \
  --report-dir docs/generated
```

当前结果：

| 验收点 | 结果 | 证据 |
|---|---:|---|
| baseline 传输链路 | PASS | `pushedAu=359`、`outputAu=358`、`selectedTwccExtId=5` |
| video-only | PASS | 报告中无 audio consumer，push `audioEnabled=false` |
| push RTCP feedback 输入 | PASS | baseline `pushRtcpFeedbackPacketsIn=130` |
| play RTCP 输出 | PASS | baseline `playRtcpPacketsOut=244`、`playRtcpSendFailures=0` |
| SDK runtime 文件 | PASS | push/play 均 `sdk_runtime_files enabled=true`，各有 log/metrics/alerts 文件 |
| SDK RR/TWCC counter | PASS | play `transportFeedbackCountMax=232`、`receiverReportCountMax=12`；push `transportFeedbackCountMax=110`、`receiverReportCountMax=9` |
| synthetic x264 runtime | PASS | `encoderRuntime=PASS`，baseline encoder `accessUnits=332`、`keyframes=12`、`currentBitrateBps=2500000`、`currentFps=30` |
| PLI/SDK keyframe request 响应 IDR | PASS | baseline `forcedKeyframeRequests=1`、`forcedKeyframes=1`、`maxForcedKeyframeDelayUs=0`；bandwidth case `maxForcedKeyframeDelayUs=190401us`，均小于 1s |
| native decode/QoE | PASS | `nativeDecodeQoe=PASS`，baseline `decodedFrames=358`、`decodeErrors=0`、`freezeCount=0`、`outputFps=30.08` |
| 弱网 coverage | PASS | `delay_100ms`、`loss_2pct`、`bandwidth_600k`、`drop_recover` 实际启用 netem，全部 PASS |
| delay netem | PASS | RTT avg/max `109.58/242ms`，`decodedFrames=333`、`decodeErrors=0`、`NACK=249` |
| loss netem | PASS | `loss_2pct` 下 `NACK=51`、`pushRtcpFeedbackPacketsIn=157`、`decodeErrors=0` |
| bandwidth netem | PASS | `bandwidth_600k` 下 targetBps min/max `300000/1693914`、`droppedFrames=3`、`decodeErrors=0` |
| recovery netem | PASS | `drop_recover` 下 targetBps min/max `300000/2023706`，清网后 15 秒窗口内开始回升，`decodeErrors=0` |

结论：

- P2-M1d、P2-M2、P2-M3 的本地 baseline 主链路已闭环：TWCC 协商、play 反馈输出、push 反馈输入、SDK runtime 文件和 SDK counter 都可观测。
- P2-M5/P2-M6 的 synthetic+x264 最小切片已闭环：encoder AU、keyframe、bitrate/fps、forced-IDR runtime metrics 和 smoke gate 都可观测。
- P2-M8 的 native decode/QoE baseline 最小切片已闭环：decoded frames、decode errors、first frame、freeze、output fps 都写入日志和报告。
- P2-M3 已覆盖 baseline、delay、loss、bandwidth、recovery 真实 netem 短测；浏览器、MP4 decode loop 和 V4L2 仍未覆盖。

## 4. 版本差异

当前 `/root/webrtc_qos_sdk/dist/linux-x86_64` 发布包已经暴露
`RuntimeLogConfig` / `RuntimeMetricsConfig` / `RuntimeAlertConfig` 字段，且包含 play RR/TWCC 输出和 RR/TWCC counter。

客户端仍保留兼容层：

- 如果 SDK config 暴露 `logging/metrics/alerts` 字段，则启用 SDK runtime 文件输出。
- 如果当前 dist 包不暴露，则日志里输出 `sdk_runtime_files enabled=false`，并使用 adapter 层
  spdlog 文件日志作为当前观测来源。

当前 smoke 已验证正常路径为 `sdk_runtime_files enabled=true`；降级路径只作为旧 SDK 包兼容策略，不允许作为 P2-M2 PASS。

## 5. 未覆盖项

以下未在本地短 smoke 中覆盖或未通过：

- browser receiver 可看到 push 画面。
- 恢复时首帧时间的专项 browser/输入源验证。
- MP4 decode loop 和 V4L2 输入源。
