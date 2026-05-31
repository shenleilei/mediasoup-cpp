# 单 Worker 压测报告 2026-05-28

## 1. 摘要

本轮压测重新校准了测试方法，并修复了手写 WebSocket 客户端没有响应 ping 导致房间被服务端关闭的问题。修复后，压测脚本能够在达到目标房间数后继续保持已有房间收发媒体，不再因为 WebSocket idle close 造成“房间还在脚本里但 worker 已经没有流量”的误判。

当前结论：

- `380` 房可以稳定保持流量，接收率正常。
- `450` 房可以稳定保持流量，`recvRatio` 稳定在 `0.98 ~ 1.00`。
- `450` 房时 worker 单进程基本打满一个 CPU core，worker CPU 平均约 `94.77%`。
- 本轮未观察到 NACK 风暴，`nack/lost/reTx/subscriber loss` 均为 `0`。
- perf 热点主要集中在 UDP 发送/内核网络栈和 worker RTP 转发统计路径，不在信令、房间创建、日志或 NACK。

## 2. 测试环境

- 测试机：`root@172.31.4.40`
- 测试容器：`mediasoup-9000`
- SFU 监听：`9000/TCP`
- worker UDP 端口：`9000/UDP`
- worker 进程：容器内 PID `11`，host PID `8110`
- worker 参数：`./mediasoup-worker --logLevel=warn --rtcPort=9000`
- worker 数量：`1`
- worker thread 数量：`1`
- 压测目录：`/root/mediasoup-cpp`

本轮只管理 `mediasoup-9000` 这个测试实例，未停止测试机上其他同名或相似 mediasoup 容器。

## 3. 压测拓扑

每个房间固定为：

- `1` 个 publisher
- `2` 个 subscriber
- peer 均走完整 WebSocket `join`
- publisher 使用 `plainPublish`
- subscriber 使用 `plainSubscribe`
- RTP 包大小：`1200 bytes`
- 发送速率：`300 pps / room`
- 单房间 publisher 输入码率约 `2.88 Mbps`

450 房时理论媒体量级：

- publisher 输入约 `450 * 2.88 Mbps = 1296 Mbps`
- 2 个 subscriber 转发输出约 `2592 Mbps`
- worker 总 UDP 发包压力由每个 publisher 包 fan-out 到两个 subscriber 放大。

## 4. 测试方法修正

### 4.1 WebSocket ping/pong

前一轮出现过“房间数量看似存在，但 worker CPU 降到接近 0，媒体流量消失”的现象。根因是压测里的手写 WebSocket 客户端没有响应服务端 ping，服务端按连接关闭处理并执行 `leave`，导致房间和 transport 被回收。

已修正脚本：

- `tests/qos_harness/single_worker_pressure.mjs`
- `tests/qos_harness/multi_process_pressure.mjs`
- `tests/qos_harness/plain_room_churn.mjs`
- `tests/qos_harness/run.mjs`
- `tests/qos_harness/ws_json_client.mjs`

修正后客户端会响应 opcode `0x9` ping 为 opcode `0xA` pong，并正确处理 close frame。

### 4.2 到达目标后保持流量

压测流程调整为：

1. 按阶梯逐步加房。
2. 达到目标房间数后不再新增房间。
3. 已有房间继续保持 RTP 收发。
4. 在保持流量期间持续采集 perf、top、node-load、接收率、NACK/loss/retransmission。

这样避免把“创建房间过程”误当成稳态，也避免一出现问题就停掉导致无法观察高压下的真实瓶颈。

### 4.3 perf 连续采样

`multi_process_pressure.mjs` 增加了连续 perf 支持：

- `--perf`
- `--perf-interval-ms=20000`
- `--perf-output-dir=/tmp/perf-450-flow`
- `--perf-frequency=99`
- `--perf-percent-limit=1`

采样目标通过 `docker top mediasoup-9000 -eo pid,comm,args` 精确定位 `comm == mediasoup-worke` 的 host PID，确认本轮采样对象为 host PID `8110`，对应容器内 worker PID `11`。

perf 文件按时间命名：

```text
YYYYMMDD-HH-MM-SS.data
YYYYMMDD-HH-MM-SS.report
YYYYMMDD-HH-MM-SS.meta
```

`.meta` 记录采样时的 phase、rooms、node-load、worker PID、data/report 路径，方便把动作和结果关联。

### 4.4 本轮实际测试方法

本轮实际执行时，统一按下面的方法跑，不把“加到某个房间数就退出”当成压测完成：

1. 先确认目标实例是压测专用的 `9000` 端口实例，而不是常规测试/对外服务用的 `8000` 端口实例。
2. 启动前先检查 `mediasoup-9000` 是否已经存在，并确认只有它占用了本轮压测的 `9000/TCP + 9000/UDP`。
3. 用多进程脚本按“每 10 秒增加 10 个房间”的节奏逐步加房。
4. 达到目标房间数后停止继续加房，但保持已有房间持续收发 RTP。
5. 在整个测试过程中持续采集：
   - `top` 中 worker 进程瞬时 CPU
   - `/api/node-load`
   - 压测脚本输出的 `send/recv/recvRatio`
   - `nack/lost/reTx/subscriber loss`
   - 每 20 秒一份 perf `data/report/meta`
6. 如果接收率开始退化，不要立刻停测；先停止继续加房，保留现场流量和房间，再继续观察/采样瓶颈点。
7. 收到足够多的稳态 perf 样本后，再停止压测脚本；停测时只清理本轮压测进程，不停止 `mediasoup-9000` 容器本身。

具体执行约束：

- `ps` 不用来判断 worker 瞬时 CPU，worker CPU 以容器内 `top` 结果为准。
- perf 必须采样实际 worker host PID，不能采样到 SFU 主进程。
- 停测后的 `[ws-close]` 清理日志不算运行中异常。
- 结果判读以“达到目标房间后还能否持续稳定收发”优先，而不是只看加房阶段是否成功。

本轮用于日常检查的命令主要是：

```bash
curl -sS http://127.0.0.1:9000/api/node-load
docker exec mediasoup-9000 sh -lc "top -b -n 1 | head -n 14"
tail -n 80 /tmp/pressure-450-perf.log
find /tmp/perf-450-flow -maxdepth 1 -name "*.report" | wc -l
docker logs --since 2m mediasoup-9000 2>&1 | grep -E "\[ws-close\]|disconnected"
```

### 4.5 多进程压测模型

本轮不是用单个 Node 进程独自承担全部房间，而是使用：

- 一个父进程：`tests/qos_harness/multi_process_pressure.mjs`
- 多个子进程：`tests/qos_harness/single_worker_pressure.mjs`

职责划分：

- 父进程负责：
  - 按节奏拉起子进程
  - 维护全局目标房间数增长
  - 采集 `/api/node-load`
  - 持续执行 perf 采样
  - 汇总各子进程输出
- 每个子进程负责：
  - 自己那一批房间的 `join`
  - 自己那一批房间的 `plainPublish/plainSubscribe`
  - 自己那一批房间的 RTP send/recv
  - 输出本进程维度的 `send/recv/recvRatio/workerCpu`

这样做的原因是：

- 单个 Node 进程同时承担太多房间时，脚本自身会先变成瓶颈。
- 多进程可以把“压测脚本自身 CPU”与“worker 真正的媒体转发瓶颈”分开一些。
- 也更接近用户之前约定的测试模型：每个客户端进程只负责有限数量的房间。

本轮采用的关键约束：

- 每个子进程最多负责 `90` 个房间。
- 父进程按 `--spawn-interval-ms=10000` 每 `10` 秒拉起一个新的子进程。
- 每个子进程内部按 `--step=10 --round-ms=10000` 每 `10` 秒增加 `10` 个房间。
- 达到该子进程自己的 `--max-rooms=90` 后，不再继续加房，但继续保持现有房间收发流量。

450 房这一轮之所以是 `5` 个子进程，是因为：

- 每进程上限 `90` 房
- 总目标 `450` 房
- 所以需要 `5` 个进程

对应关系是：

```text
p1 -> 90 rooms
p2 -> 90 rooms
p3 -> 90 rooms
p4 -> 90 rooms
p5 -> 90 rooms
total = 450 rooms
```

380 房验证时采用的是：

- `4` 个子进程
- 每进程 `95` 房
- 总计 `380` 房

日志中的 `p1/p2/p3/...` 就是这些子进程的 shard 标识。看到例如：

```text
[p4] steady#10 [rooms=90] ...
```

含义是：

- `p4`：第 4 个子进程
- `steady#10`：该子进程已经进入稳态第 10 轮
- `[rooms=90]`：该子进程当前持有 `90` 个房间

判读时要区分：

- 单个子进程的 `[rooms=90]` 只是 shard 局部房间数
- 全局总房间数要看 `/api/node-load` 里的 `rooms` / `dispatchRooms`

例如 450 房稳态时典型状态应当同时满足：

- 日志中多个 shard 都显示 `[rooms=90]`
- `/api/node-load` 显示 `rooms=450`
- `lastShardRooms` 类似：

```json
{"p1":90,"p2":90,"p3":90,"p4":90,"p5":90}
```

这说明 5 个子进程都已经顶满，且 worker 内部真实存在 450 个房间。

## 5. 380 房验证

命令参数要点：

- `4` 个 shard
- 每 shard `95` 房
- 总计 `380` 房
- 达到目标后保持流量

观测结果：

- `nodeRooms=380`
- `recvRatio` 约 `1.00 ~ 1.04`
- worker CPU 约 `73% ~ 80%`
- `nack=0`
- `lost=0`
- `reTx=0`
- 无异常 `ws-close`

主要产物：

- `/tmp/pressure-380.log`
- `/tmp/perf-worker-380.data`
- `/tmp/perf-worker-380.report`
- `/tmp/metrics-worker-380-before.txt`
- `/tmp/metrics-worker-380-after.txt`

380 房阶段可以认为已经稳定通过。

## 6. 450 房压测

执行命令：

```bash
cd /root/mediasoup-cpp
stdbuf -oL -eL node tests/qos_harness/multi_process_pressure.mjs \
  --ws-url=ws://127.0.0.1:9000/ws \
  --http-url=http://127.0.0.1:9000 \
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

测试规模：

- `5` 个 shard
- 每 shard `90` 房
- 总计 `450` 房
- 到达 `450` 后持续稳态收发到 `steady#20`

主要产物：

- 压测日志：`/tmp/pressure-450-perf.log`
- perf 目录：`/tmp/perf-450-flow`
- 完整 perf report：`34` 个
- 其中 `450` 稳态高压样本：`28` 个
- 完整稳态样本范围：`20260528-11-02-06.report` 到 `20260528-11-10-58.report`

## 7. 450 房结果

只统计 `nodeRooms=450` 的稳态日志：

| 指标 | 结果 |
| --- | --- |
| 稳态日志行数 | `92` |
| `recvRatio` | min `0.98`, max `1.00`, avg `0.987` |
| worker CPU | min `93.9%`, max `95.7%`, avg `94.771%` |
| 压测 shard self CPU | min `38.5%`, max `52.6%`, avg `47.79%` |
| `queueDepth` | min `0`, max `3`, avg `0.098` |
| `avgTaskWaitUs` | min `1854.56us`, max `2356.39us`, avg `2087.61us` |
| `nack` | max `0`, sum `0` |
| `lost` | max `0`, sum `0` |
| `reTx` | max `0`, sum `0` |
| `sub1Loss` | max `0`, sum `0` |
| `sub2Loss` | max `0`, sum `0` |
| `softnetDrop` 增量 | `3330194` |

代表性日志：

```text
first450:
[p4] steady#1 [rooms=90] healthy=90/90 send=27325 recv1=26672 recv2=26671 ratio=0.98 workerCpu=93.9 nodeRooms=450 queueDepth=0 rtp(pub nack=0 lost=0 reTx=0 sub1Loss=0 sub2Loss=0)

last450:
[p1] steady#20 [rooms=90] healthy=90/90 send=27329 recv1=26980 recv2=26973 ratio=0.99 workerCpu=94.7 nodeRooms=450 queueDepth=0 rtp(pub nack=0 lost=0 reTx=0 sub1Loss=0 sub2Loss=0)
```

停测后 `node-load` 已回到：

```text
rooms=0
dispatchRooms=0
healthy=true
ready=true
```

容器 `mediasoup-9000` 未停止。

## 8. 450 房 perf 热点

统计 `28` 个 `nodeRooms=450` 且 phase 为 steady 的 report，平均热点如下：

| 平均占比 | 符号 |
| --- | --- |
| `5.11%` | `_raw_spin_unlock_irqrestore [kernel.kallsyms]` |
| `4.19%` | `copy_user_enhanced_fast_string [kernel.kallsyms]` |
| `4.07%` | `0x0000000000000979 [vdso]` |
| `2.69%` | `ip_idents_reserve [kernel.kallsyms]` |
| `2.29%` | `skb_set_owner_w [kernel.kallsyms]` |
| `2.08%` | `do_syscall_64 [kernel.kallsyms]` |
| `1.91%` | `sendmsg libpthread-2.31.so` |
| `1.77%` | `RTC::RateCalculator::Update mediasoup-worker` |
| `1.76%` | `__vdso_clock_gettime [vdso]` |
| `1.53%` | `RTC::Router::OnTransportProducerRtpPacketReceived mediasoup-worker` |
| `1.42%` | `do_softirq.part.17 [kernel.kallsyms]` |
| `1.38%` | `ipt_do_table [kernel.kallsyms]` |
| `1.36%` | `__nf_conntrack_find_get [kernel.kallsyms]` |
| `1.34%` | `fib_table_lookup [kernel.kallsyms]` |
| `1.28%` | `ip_finish_output2 [kernel.kallsyms]` |
| `0.98%` | `udp_sendmsg [kernel.kallsyms]` |
| `0.69%` | `RTC::NackGenerator::ReceivePacket mediasoup-worker` |
| `0.16%` | `RTC::SimpleConsumer::SendRtpPacket mediasoup-worker` |

结论：

- 热点主要在 UDP sendmsg / copy_user / IP output / conntrack / softirq 等内核网络路径。
- worker 用户态稳定可见热点为 `RateCalculator::Update` 和 `Router::OnTransportProducerRtpPacketReceived`。
- `NackGenerator::ReceivePacket` 有采样但占比低，本轮没有 NACK 计数增长，不支持“NACK 风暴导致瓶颈”的判断。

## 9. 瓶颈判断

本轮 450 房已经接近单 worker 单核极限：

- worker CPU 长时间稳定在 `94% ~ 96%`
- `recvRatio` 仍然健康，说明 450 尚未越过业务失败线
- `queueDepth` 基本为 `0`，没有出现应用任务队列持续堆积
- `avgTaskWaitUs` 约 `2ms`，未出现明显队列失控
- RTP loss/NACK/retransmission 均为 `0`
- host `softnetDrop` 在高压期间持续增长，需要作为网络栈压力指标继续观察

当前瓶颈更像是：

```text
单 worker 一个 CPU core
  -> 每个 publisher RTP 包 fan-out 到 2 个 subscriber
  -> 每包触发用户态 RTP 改写、统计、转发
  -> 大量 UDP sendmsg 进入内核网络栈
  -> copy_user / ip output / conntrack / softirq 消耗明显
```

不是：

- WebSocket join 瓶颈
- 房间创建瓶颈
- mediasoup-sfu 主进程控制面瓶颈
- 日志瓶颈
- NACK 风暴

## 10. 可优化方向

### 10.1 优先：发送路径和网络栈

perf 最大头部仍在内核 UDP 发送路径。优先级最高的优化/验证项：

- 测试 `host network`，减少 Docker bridge / NAT / conntrack 开销。
- 对比关闭或绕开 conntrack/iptables 后的 CPU 与 `softnetDrop`。
- 调整 host 网络参数，例如 UDP buffer、`netdev_max_backlog`、RPS/XPS、网卡队列。
- 使用新内核验证 worker 的 `liburing` 路径。当前 worker 构建逻辑只在 Linux kernel `>= 6` 时启用 `MS_LIBURING_SUPPORTED`，测试机 4.15 内核不会走该路径。

### 10.2 中优先：Transport 发送统计降频

`RTC::RateCalculator::Update` 是 worker 用户态可见热点。当前 `Transport::OnConsumerSendRtpPacket()` 每个发送 RTP 包都会更新：

```text
sendRtpTransmission.Update(packet)
```

但这个 `Transport` 级 `sendRtpTransmission` 主要用于 transport stats：

```text
rtpBytesSent
rtpSendBitrate
```

不参与 TCC、consumer bitrate 分配、producer score 等核心媒体控制逻辑。

安全边界：

- 不应全局修改 `RateCalculator` 或 `RtpDataCounter`。
- 不应影响 `RtpStreamRecv/Send` 中用于 score、layer、bitrate 决策的计数器。
- 可以只给 `Transport` 级发送统计换轻量计数器：每包精确累加 bytes/packets，bitrate 在 stats 查询或定时器里按较低频率刷新。

预期影响：

- `rtpBytesSent` 可保持精确。
- `rtpSendBitrate` 从严格滑动窗口变成低频近似值，监控精度略降。
- 媒体转发正确性不应受影响。

### 10.3 中优先：减少无效 RTP extension 改写

当前发送路径每包可能执行：

- `UpdateAbsSendTime()`
- `UpdateTransportWideCc01()`
- `UpdateMid()`

可以继续确认压测场景和生产场景是否确实需要这些 RTP extension。如果不需要，可以从配置层关闭；如果需要，也可以优化为更快判断“该包是否存在对应 extension”，减少无效扫描。

### 10.4 低优先：NACK 继续保持观测

本轮没有 NACK 风暴证据：

- `nack=0`
- `lost=0`
- `reTx=0`
- `RTC::NackGenerator::ReceivePacket` 平均仅 `0.69%`

NACK 代码仍需要保持正确性 review，但不建议把它作为下一步性能优化主线。

## 11. 下一步建议

建议下一轮按下面顺序做 A/B：

1. 保持 worker 和业务代码不变，只切换 host network / 网络栈参数，复测 `450` 和继续冲 `500`。
2. 单独实现 `Transport` 发送统计降频，复测同样的 `450/500`，观察 worker CPU、`RateCalculator::Update` 占比、`recvRatio`、stats 精度。
3. 如果仍然是 UDP sendmsg 为主，再评估 `sendmmsg` batch 或新内核 `liburing` 方案。
4. 每轮都保留同样命名规则的 perf data/report/meta，确保能把压测阶段和 perf 结果对应起来。

目前可以把 `450` 作为当前基线容量：在 `1P + 2C`、`1200 bytes`、`300 pps / room`、单 worker 单线程条件下，`450` 房稳定，但 worker 已接近单核上限。
