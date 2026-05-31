# PlainTransport 双机压测 auto-return 方案

## 背景

当前 `tests/qos_harness/multi_process_pressure.mjs` 的 PlainTransport 压测最初按同机闭环设计：

- 信令、媒体发送端、媒体接收端、`mediasoup-9000` 都在测试机本机。
- `plainPublish` 和 `plainSubscribe` 的 UDP socket 默认绑定 `127.0.0.1`。
- `plainSubscribe` 通过信令显式传 `recvIp` / `recvPort`，服务端再 `connect()` 到这个地址。

同机闭环可以验证代码路径和单 worker 高负载状态，但不能直接代表真实双机内网容量。新的压测目标是：

- 压测机和服务机分离。
- 压测机只需要指定服务机的 `ws-url` / `http-url`。
- 不要求压测机把自身 IP 暴露给服务端。
- 服务端根据实际收到的 UDP 首包学习回流地址。

## 已发现的问题

显式 `recvIp` / `recvPort` 方案在复杂网络环境下不够稳：

- 当前执行环境看到的本地地址可能是容器地址，例如 `172.19.x.x`。
- 人工指定的宿主内网地址可能不在当前 namespace 内，不能被 UDP socket 直接 `bind()`。
- 即使信令里传了某个 `recvIp`，它也不一定是当前 socket 实际可用、服务端实际可回流的地址。

这个问题的本质是：信令参数里的地址不一定等于真实 UDP 路径里的源地址。

## 目标

新增 PlainTransport subscribe auto-return 模式，使双机压测流程变成：

1. 客户端创建一个 UDP socket。
2. 客户端通过 `plainSubscribe(autoReturn=true)` 请求服务端创建接收 PlainTransport。
3. 服务端返回该 PlainTransport 的本地 UDP `ip` / `port`。
4. 客户端用同一个 UDP socket 向服务端返回的 `ip:port` 发送一个 `connect` 首包。
5. 服务端从这个 UDP 首包学习客户端真实源 `ip:port`。
6. 服务端后续把该 consumer 的 RTP 发回这个源 `ip:port`。
7. 客户端继续用同一个 UDP socket 接收 RTP。

## 非目标

- 不改变 WebRTC/ICE/DTLS-SRTP 传输模型。
- 不替换现有 `plainSubscribe(recvIp, recvPort)` 显式模式。
- 不要求所有 PlainTransport 业务都使用 auto-return。
- 不在第一阶段设计复杂 probe 鉴权或跨 peer 路由。

## 协议语义

保留旧模式：

```json
{
  "method": "plainSubscribe",
  "data": {
    "recvIp": "127.0.0.1",
    "recvPort": 40000
  }
}
```

旧模式语义不变：

- 服务端创建 `PlainTransport(comedia=false)`。
- 服务端调用 `transport->connect(recvIp, recvPort)`。
- 服务端立即把 consumer 媒体发往显式地址。

新增 auto-return 模式：

```json
{
  "method": "plainSubscribe",
  "data": {
    "autoReturn": true
  }
}
```

auto-return 语义：

- 服务端创建 `PlainTransport(comedia=true, rtcpMux=true)`。
- 服务端不要求 `recvIp` / `recvPort`。
- 服务端不调用 `connect()`。
- 服务端返回本地 UDP `ip` / `port` 和 consumers。
- 客户端必须从后续接收 RTP 的同一个 UDP socket 发送 `connect` 首包。
- worker 从首包源地址学习 tuple 后，后续 RTP 回到同一个 socket。

## connect 首包

首包使用合法 RTCP APP 包：

```text
PT=204 (APP)
subtype=1 ("connect")
name="CNCT"
ssrc=0
payload=empty
```

原因：

- 语义直接：这是 PlainTransport auto-return 连接探测包。
- 客户端和服务端都容易记录和排查。
- 不伪造 RTP，不污染 RTP 序列号和媒体统计。
- `PlainTransport(comedia=true, rtcpMux=true)` 在收到合法 RTCP 时即可学习 tuple，不需要改 worker。

## 压测脚本行为

新增参数：

```bash
--plain-auto-return
```

启用后：

- `plainSubscribe` 请求只传 `autoReturn=true`。
- subscriber UDP socket 绑定 `0.0.0.0:0`。
- subscriber 用同一个 socket 向 `plainSubscribe` 返回的 `ip:port` 发送 3 次 RTCP APP `CNCT`，间隔 20-50 ms。
- subscriber 继续用同一个 socket 统计接收 RTP 包数。
- 不再需要 `--loadgen-ip`。
- 不再暴露 `--service-rtp-ip`；UDP 目标地址默认使用 `ws-url.hostname`，端口使用服务端返回的 `port`。

publisher 侧建议也支持 comedia：

- `plainPublish` 不传 `senderIp` / `senderPort`。
- publisher UDP socket 绑定 `0.0.0.0:0`。
- publisher 直接把 RTP 发到 `plainPublish` 返回的 `ip:port`。
- 服务端从真实 RTP 首包学习 publisher tuple。

## 命令形态

双机内网压测命令应收敛为：

```bash
node tests/qos_harness/multi_process_pressure.mjs \
  --ws-url=wss://172.31.4.40:9000/ws \
  --http-url=https://172.31.4.40:9000 \
  --sample-host=root@172.31.4.40 \
  --container=mediasoup-9000 \
  --plain-auto-return
```

扩容到 450 rooms 时沿用现有 shard 参数：

```bash
node tests/qos_harness/multi_process_pressure.mjs \
  --ws-url=wss://172.31.4.40:9000/ws \
  --http-url=https://172.31.4.40:9000 \
  --sample-host=root@172.31.4.40 \
  --container=mediasoup-9000 \
  --plain-auto-return \
  --rooms-per-process=90 \
  --step=10 \
  --round-ms=10000 \
  --steady-round-ms=20000 \
  --spawn-interval-ms=10000 \
  --steady-rounds=0 \
  --recv-ratio=0.85 \
  --max-processes=5 \
  --perf \
  --perf-interval-ms=20000 \
  --perf-output-dir=/tmp/perf-450-dualhost
```

## 服务端改造点

需要修改：

- `SignalingRequestDispatcher.h`
- `RoomService.h`
- `RoomServiceMedia.cpp`

建议接口形态：

- `RoomService::plainSubscribe(..., std::optional<std::string> recvIp, std::optional<uint16_t> recvPort, bool autoReturn)`

行为：

- `autoReturn=false`：要求 `recvIp` / `recvPort`，保持旧逻辑。
- `autoReturn=true`：忽略 `recvIp` / `recvPort`，使用 `comedia=true`，不调用 `connect()`。
- `autoReturn=true` 与显式 `recvIp` / `recvPort` 同时出现时，建议返回参数错误，避免语义混淆。

## 验证计划

1. 单元/集成检查：
   - 旧 `plainSubscribe(recvIp, recvPort)` 测试继续通过。
   - 新增 auto-return 参数解析测试。

2. 本机 smoke：
   - `--plain-auto-return`
   - `wss://127.0.0.1:9000/ws`
   - 验证 1 room `recv1/recv2 > 0`。

3. 双机 1 room：
   - 压测机运行脚本。
   - 服务机 `172.31.4.40` 运行 `mediasoup-9000`。
   - 验证服务机 `tcpdump` 能看到 RTCP APP `CNCT` 包和 RTP。
   - 验证 `recv1/recv2 > 0`。

4. 双机 10 rooms：
   - 验证 `healthy=10/10`。
   - 验证服务机 `serviceRxMbps` / `serviceTxMbps` 正常增长。

5. 双机 450 rooms：
   - 沿用 5 shard x 90 rooms。
   - 保持 steady。
   - 收集 perf、`/api/node-load`、worker CPU/RSS、softnet、网卡吞吐。

## 风险和注意事项

- auto-return 模式依赖 UDP 首包到达；脚本需要发多次 RTCP APP `CNCT` 降低首包丢失影响。
- auto-return 模式应只作为 PlainTransport 压测/工具链能力，不能混同 WebRTC ICE 行为。
- 旧显式模式必须保留，避免影响已有同机压测、plain churn 和集成测试。

## 结论

`plainSubscribe(autoReturn=true)` + subscriber socket 发送 RTCP APP `CNCT` 首包，是当前双机内网压测更合适的方向。

它把“服务端要往哪里回流”从信令地址参数改成真实 UDP 路径学习，能避开容器地址、宿主机地址、多网卡和 NAT 带来的不确定性，同时保留旧显式模式作为兼容路径。
