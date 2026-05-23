# WebRTC QoS SDK Plain 推拉流客户端实现验收清单

> 状态：P1 已完成本地短链路验收；P2 已完成 consumer TWCC ext 修复、adapter RTCP 边界计数和 video-only publish。浏览器画面、弱网 smoke、SDK play 周期性 RR/TWCC 输出仍需后续验证。

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
| push RTCP feedback 输入计数 | `WebRtcQosPushRuntime` 记录 `rtcpFeedbackPacketsIn/BytesIn/Failures` | P2 smoke：`rtcpFeedbackPacketsIn=86 ... Failures=0` |
| play RTCP 输出计数 | `WebRtcQosPlayRuntime` 记录 `rtcpPacketsOut/BytesOut/SendFailures` | P2 smoke 字段存在；当前 SDK baseline 输出为 0，列为 P2 后续项 |
| video-only plain publish | 服务端支持 `enableAudio=false`；新 push 默认不发 `audioSsrc`；旧请求默认仍启用 audio | P2-M4 smoke：`audioEnabled=false` 且 play 只收到 video consumer |

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

./build-webrtc-qos-p2/mediasoup_qos_integration_tests \
  --gtest_filter='QosIntegrationTest.PlainPublishSupportsVideoOnlyAndKeepsLegacyAudioDefault:QosIntegrationTest.PlainPublishReplacesOldTransportAndUsesBaselineCodec:QosIntegrationTest.PlainPublishRejectsDuplicateVideoSsrcs'
```

当前结果：

- CMake configure 通过。
- push/play 两个 target 编译通过。
- ORTC standalone targeted test 通过。
- P2-M4 targeted integration test 通过。
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
| play RTCP 输出 | FAIL | `rtcpPacketsOut=0`；SDK play facade 当前只在 NACK/PLI 事件时输出 RTCP，没有周期性 RR/TWCC |

结论：

- P2-M1a 已完成：consumer 不再丢 TWCC header extension。
- P2-M1c adapter 边界计数已完成：push/play 日志包含 RTCP I/O counters。
- P2-M1d 未完成：需要 `webrtc_qos_sdk` play facade 补周期性 RR/TWCC 输出和 SDK counter 后，再在 mediasoup-cpp 集成验证。

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

## 4. 版本差异

当前 `/root/webrtc_qos_sdk/dist/linux-x86_64` 发布包头文件没有暴露
`RuntimeLogConfig` / `RuntimeMetricsConfig` / `RuntimeAlertConfig` 字段；SDK 源码头文件有。

客户端已加兼容层：

- 如果 SDK config 暴露 `logging/metrics/alerts` 字段，则启用 SDK runtime 文件输出。
- 如果当前 dist 包不暴露，则日志里输出 `sdk_runtime_files enabled=false`，并使用 adapter 层
  spdlog 文件日志作为当前观测来源。

这意味着文档中“SDK runtime 文件输出是主要观测来源”在当前 dist 包下只能部分满足。
要完全满足，需要重新发布包含 runtime config 字段的 SDK dist 包，或改文档明确当前包降级策略。

## 5. 未覆盖项

以下未在本地短 smoke 中覆盖或未通过：

- browser receiver 可看到 push 画面。
- netem 弱网 smoke：延迟、丢包、恢复。
- SDK play 周期性 RR/TWCC 输出：当前 P2 smoke 中 `rtcpPacketsOut=0`，QoS 主链路不能签收。
- 当前 SDK dist 包未暴露 runtime 文件配置字段，因此 SDK 内部 metrics/alerts 文件输出未覆盖；adapter spdlog 文件已覆盖。
