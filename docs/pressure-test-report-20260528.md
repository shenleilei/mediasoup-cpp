# mediasoup-9000 单 Worker 压测报告

生成时间：`2026-05-29 CST`

## 1. 目标

验证当前 `mediasoup-cpp` 新版本在测试机 `mediasoup-9000` 容器上的单 worker 承载能力，并确认：

- 容器更新后服务是否正常启动
- `local-only / noredis` 路径在高并发房间压力下是否稳定
- `multi_process_pressure.mjs` 全流程压测和 perf 采样是否正常工作
- 在 `450 rooms` 目标下是否出现业务退化拐点

## 2. 测试环境

- 测试机：`root@172.31.4.40`
- 容器：`mediasoup-9000`
- 容器网络：`host`
- 信令端口：`9000/TCP`
- Worker UDP 端口：`9000/UDP`
- 压测脚本：
  - `tests/qos_harness/multi_process_pressure.mjs`
  - `tests/qos_harness/single_worker_pressure.mjs`
- 测试协议：
  - `wss://127.0.0.1:9000/ws`
  - `https://127.0.0.1:9000`

容器更新后确认项：

- `docker ps` 显示 `mediasoup-9000` 正常运行
- `docker logs mediasoup-9000` 显示：
  - `Starting in local-only mode`
  - `SignalingServer listening with HTTPS/WSS on port 9000`
- `curl -sk https://127.0.0.1:9000/healthz` 返回：
  - `ok=true`
  - `ready=true`

## 3. 压测拓扑

每个房间固定为：

- `1 publisher`
- `2 subscribers`
- publisher 使用 `plainPublish`
- subscribers 使用 `plainSubscribe`

发送参数：

- RTP payload size：`1200 bytes`
- publisher 发送速率：`300 pps / room`
- 近似输入带宽：`2.88 Mbps / room`

理论上在 `450 rooms` 时：

- publisher 输入总量约 `1296 Mbps`
- fanout 后 subscriber 输出总量约 `2592 Mbps`

## 4. 执行命令

在测试机本机执行：

```bash
cd /root/workspace/mediasoup-cpp
stdbuf -oL -eL node tests/qos_harness/multi_process_pressure.mjs \
  --ws-url=wss://127.0.0.1:9000/ws \
  --http-url=https://127.0.0.1:9000 \
  --container=mediasoup-9000 \
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
  --perf-output-dir=/tmp/perf-450-flow \
  2>&1 | tee /tmp/pressure-450-perf.log
```

说明：

- `5` 个 shard
- 每个 shard 目标 `90 rooms`
- 总目标 `450 rooms`
- 达到目标房间数后继续 steady 压测，不自动停止

## 5. 关键修正

本轮测试确认并修正了两个关键问题：

1. 压测必须走测试机本机内网
   - 公网 `14.103.165.183:9000` 未开放，不适合压测
   - 正确入口是 `127.0.0.1:9000`

2. `sampleHost=local` 必须按本机执行处理
   - 旧逻辑会错误执行 `ssh local`
   - 已修正为本机直接 `docker exec`
   - 进程采样与 perf 采样恢复正常

## 6. 结果摘要

### 6.1 最终房间数

最终成功推进到：

- `rooms=450`
- `dispatchRooms=450`

### 6.2 服务健康状态

在 steady 阶段持续保持：

- `healthy=true`
- `ready=true`
- `health=200`

### 6.3 业务健康度

steady 阶段多个 shard 的结果稳定为：

- `healthy=90/90`
- `recvRatio ≈ 0.98 ~ 0.99`

未观察到明显媒体异常：

- `nack=0`
- `lost=0`
- `reTx=0`
- `sub1Loss=0`
- `sub2Loss=0`

### 6.4 资源使用

在 `450 rooms` steady 阶段：

- `workerCpu ≈ 95.9% ~ 97.4%`
- `workerRss ≈ 41.4MB`
- `sfuRss ≈ 80.8MB`
- `selfRss ≈ 264MB ~ 283MB`
- `queueDepth ≈ 0 ~ 1`
- `queuePeakDepth = 50`
- `avgTaskWaitUs ≈ 69 ~ 88 us`

### 6.5 内核/网络侧观察

观察到：

- `softnetDrop` 持续增长
- `softnetSqueeze=1`
- `load1 ≈ 2.1 ~ 3.3`

这表明：

- 单 worker 已经逼近高负载区
- 主要瓶颈开始转向 worker CPU 和内核网络软中断路径
- 但业务质量尚未先行退化

## 7. 典型采样

### steady#5

- `rooms=450`
- `healthy=90/90`
- `recvRatio=0.98`
- `workerCpu=96.6`
- `workerRss=41.4MB`

### steady#8

- `rooms=450`
- `healthy=90/90`
- `recvRatio=0.98 ~ 0.99`
- `workerCpu=96.6 ~ 97.3`
- `workerRss=41.4MB`

### steady#10

- `rooms=450`
- `healthy=90/90`
- `recvRatio=0.98 ~ 0.99`
- `workerCpu=96.6 ~ 97.4`
- `workerRss=41.4MB`

## 8. perf 产物

压测期间持续生成了 perf 采样：

- 目录：`/tmp/perf-450-flow`
- 产物类型：
  - `*.data`
  - `*.report`
  - `*.meta`

本轮已生成 `21` 份 `*.report`

## 9. 结论

本轮压测可以得出明确结论：

- 更新后的 `mediasoup-9000` 容器在单 worker 条件下成功支撑了 `450 rooms`
- 在 `450 rooms` steady 多轮采样期间，服务持续 `healthy/ready`
- 业务侧未出现丢包、重传、收发比异常或房间健康退化
- 当前极限拐点尚未出现
- 单 worker 已明显接近上限区，瓶颈首先体现在 CPU/softirq，而非业务链路失真

因此当前版本可以认为：

- **`450 rooms` 单 worker 稳态运行通过**
- **但已经处于接近上限的高负载区，后续再扩房间数需要谨慎推进**

## 10. 建议

1. 如需继续探索极限，下一轮可以在同一方法下继续提高目标房间数，观察首次 `recvRatio < 0.85` 或 `healthy=false` 的拐点。
2. 若要进入上线前容量结论，建议结合：
   - `/tmp/perf-450-flow/*.report`
   - `docker exec mediasoup-9000 top`
   - `softnetDrop / softnetSqueeze`
   做一次更正式的瓶颈归因。
3. 当前测试方法与经验应以如下知识库条目为准：
   - `/root/.codex/memories/mediasoup_single_worker_pressure_20260528.md`
   - `/root/.codex/memories/mediasoup_test_flow.md`
   - `/root/.codex/skills/mediasoup-pressure-runbook/SKILL.md`
