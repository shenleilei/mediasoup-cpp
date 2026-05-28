# Worker 性能优化方向确认

## 1. 当前目标

本轮目标是优化单 `mediasoup-worker` 在后端 `PlainTransport` 压测场景下的媒体数据面性能。

沿用之前的压测拓扑和判定口径：

- 单个 `mediasoup-sfu`
- 单 worker
- 每个房间 `1 publisher + 2 subscriber`
- 所有 peer 都走完整 WebSocket `join`
- 媒体链路走 `plainPublish / plainSubscribe`
- RTP 包大小 `1200 bytes`
- 发送频率 `300 pps / room`
- 单房间 publisher 输入码率约 `2.88 Mbps`
- 多进程 Node 压测，每 10 秒增加 10 个房间

测试机内网验证时，不依赖公网端口：

- TCP 主端口：`9000`
- UDP 监听端口：`9000`
- 测试连接优先使用内网地址和本机/内网回环链路
- 测试机上可能同时存在多个 `mediasoup` 服务，本轮只允许管理占用 TCP/UDP `9000` 的测试实例

## 2. 已确认的瓶颈范围

高压 `perf / flamegraph` 已经把热点收敛到 `mediasoup-worker` 的 `PlainTransport` 数据面。

主要路径是：

```text
uv__udp_recvmmsg
RTC::PlainTransport::OnUdpSocketPacketReceived
RTC::PlainTransport::OnRtpDataReceived
RTC::Transport::ReceiveRtpPacket
RTC::Producer::ReceiveRtpPacket
RTC::Router::OnTransportProducerRtpPacketReceived
RTC::SimpleConsumer::SendRtpPacket
RTC::Transport::OnConsumerSendRtpPacket
RTC::PlainTransport::SendRtpPacket
UdpSocketHandle::Send
uv__udp_try_send
sendmsg / udp_sendmsg
```

当前不把下面这些作为优先优化目标：

- WebSocket `join` 信令
- HTTP health / metrics
- `mediasoup-sfu` 主进程控制面
- 日志写入和 Docker 日志映射
- WebRTC 浏览器入口

## 3. 本轮只做三类优化

### 3.1 发送侧 batch + `sendmmsg`

当前发送侧 flamegraph 显示是逐包发送：

```text
UdpSocketHandle::Send -> uv__udp_try_send -> sendmsg / udp_sendmsg
```

优化方向：

- 在 worker 发送侧引入短窗口 batch
- 同一 UDP socket 上的多个待发送 RTP 包按顺序排队
- flush 时使用 `sendmmsg` 一次发出多个包
- 用少量额外延迟换取 syscall 次数下降和吞吐提升
- 通过环境变量控制 batch 开关与窗口，便于 A/B：
  - `MEDIASOUP_UDP_SEND_BATCH_TIMEOUT_MS`
  - `MEDIASOUP_UDP_SEND_BATCH_MAX_DATAGRAMS`

初始参数建议：

- batch 时间窗口：`5ms ~ 10ms`
- 默认先按 `10ms` 做实验
- 同时设置最大 batch 包数，避免低流量场景被时间窗口拖住
- 满足“达到包数上限”或“到达时间窗口”任一条件即 flush

实现约束：

- 不能跨 socket 混批
- 同一 socket 内必须保持 RTP 包顺序
- 必须处理 `sendmmsg` 部分发送
- 失败路径要保留明确日志和计数
- 需要保留开关，便于 A/B 测试
- 该改动需要修改并重新编译 `src/mediasoup-worker-src`

预期验证点：

- `sendmsg / udp_sendmsg` 在 flamegraph 中占比下降
- worker 热线程 CPU 下降
- 同等 CPU 下可承载房间数上升
- `recvRatio` 不低于基线
- 端到端额外延迟符合 `5ms ~ 10ms` 预期

### 3.2 `PlainTransport` 显式 `connect()`

当前 `plainPublish` publisher 侧如果长期走 comedia，会让 worker 在收包路径上保留自动探测和远端地址学习成本。

优化方向：

- 压测客户端创建 UDP sender 后，先绑定本地地址和端口
- `plainPublish` 请求携带 `senderIp` / `senderPort`
- 服务端创建 publisher `PlainTransport` 后立即调用 `transport.connect(senderIp, senderPort)`
- 对显式连接成功的 publisher 关闭 comedia

实现约束：

- 这个优化主要改 `mediasoup-cpp` 控制面和压测脚本
- 不新增 worker 协议
- 不要求修改 worker 源码
- 如果 `senderIp/senderPort` 缺失，保留原 comedia 行为作为 fallback

预期验证点：

- `plainPublish` 显式 connect 成功
- 原有 plain client / plain pressure 脚本仍可工作
- 高压下 `PlainTransport::OnUdpSocketPacketReceived` 路径开销不升高
- 与 `sendmmsg` batch 可以独立 A/B

### 3.3 NACK / 重传异常排查

`RTC::NackGenerator::ReceivePacket` 出现在 flamegraph 中。它不是最大热点，但在同一测试机/同一内网链路下，理论上不应该出现大量真实丢包。

本轮把这件事按潜在 bug 处理，而不是只当作正常伴随开销。

需要确认：

- 是否真的有大量 RTCP NACK
- NACK 是否和房间退化、worker CPU 上升同步
- 是否存在 RTP sequence gap、乱序、timestamp/marker 生成异常
- 是否是 Node sender 调度补发导致 burst 太大，引发接收端乱序或队列抖动
- 是否是 `PlainTransport` subscriber 侧 socket 或接收统计造成误判

需要补充的观测：

- 每轮采样输出 NACK 计数
- producer / consumer RTP packet loss 统计
- retransmission / nack packet 统计
- 每个房间的 send sequence 连续性
- 高压退化房间的单独样本

验证方式：

- 同一压力下对比 NACK 计数和 worker CPU
- 对比关闭或弱化 NACK 后的 worker CPU 与 `recvRatio`
- 如果关闭 NACK 后性能明显提升，需要继续定位真实 gap 来源
- 如果 NACK 计数很低，则把它降级为次级优化点

## 4. 测试配置确认

后续测试固定使用这一组基线，除非文档明确记录变更：

```text
server tcp port      = 9000
worker udp port      = 9000
workers              = 1
workerThreads        = 1
room topology        = 1P + 2C
rtp packet size      = 1200 bytes
pps per room         = 300
publisher bitrate    = 2.88 Mbps / room
pressure model       = multi-process Node
step                 = +10 rooms / 10s
transport path       = plainPublish / plainSubscribe
```

部署约束：

- 镜像优先在本地构建完成
- 构建完成后直接拷贝镜像到测试机运行，避免在测试机重新下载依赖
- 测试机只更新本轮 `9000/TCP + 9000/UDP` 测试实例
- 停止旧实例时必须按端口或容器名精确定位，不能批量 `kill mediasoup`
- 不影响测试机上其他 `mediasoup` / `mediasoup-cpp` 服务
- 宿主机日志目录仍使用 `/var/log/mediasoup-cpp`，但测试实例日志建议带独立容器名或 tag，便于和其他服务区分

采样必须同时记录：

- worker 进程 CPU
- worker 热线程 CPU
- worker RSS
- SFU 进程 CPU / RSS
- `/healthz`
- `/readyz`
- `/api/node-load`
- send pps / recv pps / recvRatio
- NACK / loss / retransmission 相关计数
- `perf` 或 flamegraph 样本

## 5. 对比实验矩阵

至少需要跑下面几组，避免把单项收益混在一起：

| 组别 | 显式 connect | sendmmsg batch | NACK 调整 | 目的 |
| --- | --- | --- | --- | --- |
| A | 关闭 | 关闭 | 不调整 | 当前基线 |
| B | 开启 | 关闭 | 不调整 | 验证 connect 收益 |
| C | 开启 | 开启 | 不调整 | 验证 sendmmsg batch 收益 |
| D | 开启 | 开启 | 观测/调整 | 验证 NACK 是否是 bug 或放大器 |

每组都用同一套压测参数跑到退化点，并记录：

- 最大健康房间数
- 退化开始房间数
- worker 热线程 CPU
- `recvRatio`
- `queuePeakDepth`
- `avgTaskWaitUs`
- flamegraph 热点变化

## 6. 成功标准

本轮目标是让单 worker 承载能力明显提升，目标值是接近或达到性能翻倍。

判定方式：

- 在相同 `1200 bytes`、`300 pps / room`、`1P+2C` 条件下
- 优化后退化房间数相对基线显著后移
- worker 热线程 CPU 在同房间数下明显下降
- `recvRatio` 不因 batch 明显下降
- 额外排队延迟符合配置窗口
- NACK 相关异常有明确结论

如果 `sendmmsg` 后 `sendmsg / udp_sendmsg` 下降明显，但整体容量没有明显提升，说明瓶颈已经从 syscall 转移到 RTP 转发、NACK、统计或 socket 队列，需要继续按新的 flamegraph 再定位。

## 7. 风险和边界

- `sendmmsg` batch 会引入额外发送延迟，必须可配置、可关闭。
- 批处理开关约定：
  - `MEDIASOUP_UDP_SEND_BATCH_TIMEOUT_MS=0` 关闭 batch
  - `MEDIASOUP_UDP_SEND_BATCH_MAX_DATAGRAMS=1` 关闭 batch
  - 其余值启用 batch，默认值仍为 `10ms / 32 datagrams`
- 不能为了吞吐破坏 RTP 包顺序。
- `sendmmsg` 的部分发送和错误处理必须明确，不能静默丢包。
- NACK 调整不能直接掩盖真实丢包；必须先观测再决定是否改行为。
- 测试端口使用 `9000` 是测试机内网约定，不代表生产公网配置。

## 8. 当前方向结论

本轮优化方向确认如下：

1. 先完成 `PlainTransport` 显式 `connect()`，建立低风险 A/B 基线。
2. 再修改 worker 发送侧，做 `sendmmsg` batch 实验版。
3. 并行补齐 NACK / loss 观测，把高压下 NACK 作为潜在 bug 排查。
4. 用同一套 `9000/TCP + 9000/UDP` 内网测试配置复跑压测。
5. 以退化房间数、worker 热线程 CPU、`recvRatio` 和 flamegraph 变化作为最终判断。

## 9. 已验证结果

在测试机 `172.31.4.40` 上，使用单实例 `mediasoup-9000` 完成了以下验证：

- `plainPublish` 的显式 `connect()` 路径可用。
- `--no-explicit-connect` 的 comedia 回退路径在测试机本机执行时可用；这条对照仅在测试机本机执行，不能在外部主机直接以 `127.0.0.1` 作为 sender 地址替代。
- `MEDIASOUP_UDP_SEND_BATCH_TIMEOUT_MS=0` + `MEDIASOUP_UDP_SEND_BATCH_MAX_DATAGRAMS=1` 可稳定关闭 batch。
- batch 关闭时，`120` rooms 仍可通过，`queuePeakDepth` 约 `62`，`avgTaskWaitUs` 约 `415 us`。
- batch 开启时，`120` rooms 仍可通过，`queuePeakDepth` 下降到约 `3`，`avgTaskWaitUs` 约 `502 us`。
- 两组测试里的 `nack` / `packetsLost` / `packetsRetransmitted` 统计都为 `0`，当前没有证据表明 NACK 是主要放大项。
- 当前这轮 A/B 证明了 `sendmmsg` 批处理和显式 `connect()` 路径都已被真实压测覆盖，且测试机端口约束已收敛到 `9000/TCP + 9000/UDP`。
