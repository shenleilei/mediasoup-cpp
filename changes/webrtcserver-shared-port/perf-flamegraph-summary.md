# 单 Worker 高压 perf / flamegraph 结论

## 1. 结论

这次高压下的 `perf` 采样已经把热点收敛到了 `mediasoup-worker` 的媒体数据面，而不是信令面、日志面或容器外层。

最宽的调用链集中在：

- `RTC::PlainTransport::OnUdpSocketPacketReceived`
- `RTC::PlainTransport::OnRtpDataReceived`
- `RTC::Transport::ReceiveRtpPacket`
- `RTC::Producer::ReceiveRtpPacket`
- `RTC::Router::OnTransportProducerRtpPacketReceived`
- `RTC::SimpleConsumer::SendRtpPacket`
- `RTC::Transport::OnConsumerSendRtpPacket`
- `RTC::PlainTransport::SendRtpPacket`
- `UdpSocketHandle::Send`
- `uv__udp_try_send`
- `sendmsg` / `udp_sendmsg`

换句话说，当前单 worker 的瓶颈主要落在：

- UDP 收包
- RTP 解析与转发
- PlainTransport 发包
- 少量 NACK / 统计计数
- 以及对应的内核 UDP 发送/接收路径

这和前面压测里看到的现象一致：退化先发生在媒体收发，不是 join、health 或日志。

## 2. 采样背景

本次 flamegraph 使用的是测试机 `root@172.31.4.40` 上的高压样本。

采样对象：

- `mediasoup-worker` 进程

采样方式：

```text
perf record -F 99 -g --call-graph dwarf -p 21072 -o /tmp/worker_flame.perf.data -- sleep 45
```

样本结果：

- `1382 samples`
- `11.265 MB`
- 无丢样

压测拓扑仍然是：

- 单 `mediasoup-sfu`
- 单 worker
- 多进程 Node 压测
- 每房间 `1 pub + 2 sub`
- 业务媒体接入走 `plainPublish / plainSubscribe`

## 3. flamegraph 里最明显的路径

### 3.1 收包路径

最宽的底座在：

- `uv__io_poll`
- `uv__udp_io`
- `uv__udp_recvmmsg`
- `RTC::PlainTransport::OnUdpSocketPacketReceived`
- `RTC::PlainTransport::OnRtpDataReceived`

这说明 worker 主循环里很大一部分时间都花在 UDP 事件轮询和 RTP 包接收上。

### 3.2 转发路径

从接收进入媒体转发后，热点继续沿着：

- `RTC::Transport::ReceiveRtpPacket`
- `RTC::Producer::ReceiveRtpPacket`
- `RTC::Router::OnTransportProducerRtpPacketReceived`
- `RTC::SimpleConsumer::SendRtpPacket`
- `RTC::Transport::OnConsumerSendRtpPacket`
- `RTC::PlainTransport::SendRtpPacket`

这说明主要开销不是某个单独的控制点，而是整条 RTP forwarding 链路持续在吃 CPU。

### 3.3 发包路径

发包侧的宽带集中在：

- `UdpSocketHandle::Send`
- `uv__udp_try_send`
- `__sendmsg`
- `udp_sendmsg`
- `udp_send_skb`
- `ip_output`
- `ip_finish_output`

这说明当 worker 进入高压时，真正的开销是实打实的 UDP 发包，而不是上层信令空转。

### 3.4 内核网络栈

在内核侧，图里可以看到明显的：

- `udp_sendmsg`
- `udp_recvmsg`
- `net_rx_action`
- `process_backlog`
- `ip_rcv`
- `udp_queue_rcv_skb`

这说明瓶颈已经落到了用户态 RTP 处理和内核 UDP 路径的交界处。

## 4. 次级热点

除主路径外，还有一些次级热点：

- `RTC::NackGenerator::ReceivePacket`
- `RTC::RtpDataCounter::Update`
- `uv__hrtime`
- `clock_gettime`

这些都出现了，但它们不是主瓶颈。它们更像是高压下的伴随开销。

## 5. 这份 flamegraph 说明了什么

### 5.1 说明的问题不在信令

`join`、`ws-close`、`healthz`、`readyz` 这些控制面路径并不是当前的主要热区。

### 5.2 说明问题主要在 PlainTransport 数据面

这次图里最宽的路径都压在 `PlainTransport` 相关调用链上，说明当前退化更接近：

- RTP 收发吞吐到了边界
- per-packet 处理开销开始累积
- UDP socket 处理链路开始变重

### 5.3 说明单 worker 的瓶颈是“媒体转发热路径”，不是“整体服务进程”

`mediasoup-sfu` 主进程本身不是主热点；热点集中在 worker 里真正处理 packet 的路径。

## 6. 需要注意的限制

这份图不是完整的“绝对 CPU 占比”报表，它是一个基于采样栈宽度的热点示意图。

所以这里的结论应该理解成：

- 谁最宽，谁最热
- 哪条链最粗，哪条链最值得继续挖
- 它适合定位方向，不适合拿来替代精确容量基准

## 7. 后续建议

如果后面继续定位，还应该优先做两件事：

1. 针对 `PlainTransport` 数据面继续抓更长时间的高压样本，确认热点是否稳定落在同一条链上。
2. 如果要继续细分瓶颈，再结合 `perf top`、线程级 CPU、UDP socket 队列和 drop 计数一起看。

当前阶段可以先把结论记成一句话：

> 单 worker 高压下的主要瓶颈在 `PlainTransport` 的 RTP 收发与转发链路，不在信令和日志。
