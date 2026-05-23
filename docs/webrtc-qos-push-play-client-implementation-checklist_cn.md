# WebRTC QoS SDK Plain 推拉流客户端实现验收清单

> 状态：P1 已完成本地短链路验收；P2 已完成 consumer TWCC ext 修复、adapter RTCP 边界计数、SDK play RR/TWCC 输出、SDK runtime 文件观测、video-only publish 和 P2 smoke 报告脚本。浏览器画面、真实弱网 smoke、实时编码器和 native QoE 仍需后续验证。

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

## 2. 当前验证命令

```bash
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/root/webrtc_qos_sdk/dist/linux-x86_64 \
  -DBUILD_TESTS=OFF

cmake --build build-webrtc-qos-plain \
  --target webrtc-qos-plain-push-client webrtc-qos-plain-play-client \
  -j"$(nproc)"

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
  --cases baseline,delay_100ms \
  --duration-seconds 6 \
  --artifact-root /tmp/webrtc-qos-plain-p2-sdk-smoke-v2 \
  --report-dir docs/generated
```

当前结果：

- CMake configure 通过。
- push/play 两个 target 编译通过。
- ORTC standalone targeted test 通过。
- P2-M4 targeted integration test 通过；非默认 build 目录下通过 `MEDIASOUP_TEST_SFU_BIN` / `MEDIASOUP_TEST_WORKER_BIN` 指定测试 SFU 和 worker。
- P2 smoke 脚本短测通过：baseline 传输链路 PASS，`qosMainline=PASS`，`sdkRuntimeObservability=PASS`；未启用 netem 的 weak case 正确标记 SKIP。
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
  --cases baseline,delay_100ms \
  --duration-seconds 6 \
  --artifact-root /tmp/webrtc-qos-plain-p2-sdk-smoke-v2 \
  --report-dir docs/generated
```

当前结果：

| 验收点 | 结果 | 证据 |
|---|---:|---|
| baseline 传输链路 | PASS | `pushedAu=90`、`outputAu=90`、`selectedTwccExtId=5` |
| video-only | PASS | 报告中无 audio consumer，push `audioEnabled=false` |
| push RTCP feedback 输入 | PASS | `pushRtcpFeedbackPacketsIn=66` |
| play RTCP 输出 | PASS | `playRtcpPacketsOut=98`、`playRtcpSendFailures=0` |
| SDK runtime 文件 | PASS | push/play 均 `sdk_runtime_files enabled=true`，各有 log/metrics/alerts 文件 |
| SDK RR/TWCC counter | PASS | play `transportFeedbackCountMax=92`、`receiverReportCountMax=6`；push `transportFeedbackCountMax=50`、`receiverReportCountMax=4` |
| 弱网 coverage | SKIP | 未传 `--enable-netem`，`delay_100ms` 不计 PASS |

结论：

- P2-M1d、P2-M2、P2-M3 的本地 baseline 主链路已闭环：TWCC 协商、play 反馈输出、push 反馈输入、SDK runtime 文件和 SDK counter 都可观测。
- 当前结果仍不能签收完整 P2：真实弱网 case 需要带 `--enable-netem` 在可控环境运行，浏览器、实时编码器和 native QoE 仍未覆盖。

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
- netem 弱网 smoke：脚本已支持；当前机器未启用 `--enable-netem`，延迟、丢包、恢复 case 仍未实际签收。
- realtime x264 encoder adaptation。
- native decode/QoE 指标。
