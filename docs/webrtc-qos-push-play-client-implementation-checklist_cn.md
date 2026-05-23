# WebRTC QoS SDK Plain 推拉流客户端实现验收清单

> 状态：已完成本地短链路验收；浏览器画面和弱网 smoke 仍需单独环境验证。

## 1. 已实现项

| 文档要求 | 当前实现 | 证据 |
|---|---|---|
| 新建同一目录下的 push/play 客户端 | `client/webrtc_qos_plain_client/{common,push,play}` | `find client/webrtc_qos_plain_client -type f` |
| 两个可执行 target | `webrtc-qos-plain-push-client`、`webrtc-qos-plain-play-client` | `cmake --build build-webrtc-qos-plain --target ...` 通过 |
| 复用现有 WebSocket 信令 | 复用 `client/WsClient.cpp` | CMake target source 包含 `client/WsClient.cpp` |
| push 复用 `join` + `plainPublish` | `PushSignalingSession` | `plainPublish` 请求含 `videoCodec=h264`、`videoSsrc`、`audioSsrc` |
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
| play 兼容 dummy audio producer | receive capabilities 声明 Opus，但 runtime 只选择 video | SFU smoke 不再出现 audio auto-subscribe error |
| consumer 无 TWCC ext 降级 | `consumer_without_twcc_ext` 日志，`twccExtId=0` 仍启动 `VideoPlayClient` | 本地 smoke 输出 120/150 AU |

## 2. 当前验证命令

```bash
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/root/webrtc_qos_sdk/dist/linux-x86_64 \
  -DBUILD_TESTS=OFF

cmake --build build-webrtc-qos-plain \
  --target webrtc-qos-plain-push-client webrtc-qos-plain-play-client \
  -j"$(nproc)"

rg -n "client/qos|sendsidebwe|ccutils|RtcpHandler|NetworkThread|SenderTransportController|H264Packetizer|Vp8Packetizer|PublisherQosController|PacketizeAnnexB" \
  client/webrtc_qos_plain_client CMakeLists.txt
```

当前结果：

- CMake configure 通过。
- push/play 两个 target 编译通过。
- 旧实现依赖门禁没有命中代码；只命中 `client/webrtc_qos_plain_client/README.md` 的说明文本。

## 3. 动态 smoke 结果

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
| play 选择 video consumer | PASS | `selected_consumer ... pt=127 twccExtId=0` |
| consumer 无 TWCC ext 降级 | PASS | `consumer_without_twcc_ext ... headerExtensions=[]` |
| keyframe 请求 | PASS | `request_consumer_keyframe_ok` |
| play RTP/RTCP/AU | PASS | 8 秒 smoke：`rtpPackets=212 rtcpPackets=6 outputAu=120` |
| push 停止统计 | PASS | 8 秒 smoke：`push_runtime_stopped pushedAu=120` |
| SFU audio auto-subscribe error | PASS | 加 Opus 兼容声明后未再出现 `auto-subscribe FAILED` |

更长一次 10 秒 smoke 结果：

- push：`push_runtime_stopped pushedAu=150`
- play：`play_runtime_stopped rtpPackets=252 rtcpPackets=9 outputAu=150`
- play 每秒 metrics 持续输出，未出现 `play_packet_input_failed`。

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

以下未在本地短 smoke 中覆盖：

- browser receiver 可看到 push 画面。
- netem 弱网 smoke：延迟、丢包、恢复。
- 当前 mediasoup consumer 未返回 TWCC header extension，因此本地 smoke 只验证 RTP/RTCP/AU 闭环；下行 TWCC 主链路需要服务端 consumer 携带 TWCC ext 后再验。
- 当前 SDK dist 包未暴露 runtime 文件配置字段，因此 SDK 内部 metrics/alerts 文件输出未覆盖；adapter spdlog 文件已覆盖。
