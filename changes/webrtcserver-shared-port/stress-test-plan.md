# 单 Worker 高强度压测方案

## 1. 目标

验证当前 `mediasoup-sfu` 在 **单 worker** 配置下的稳定性、吞吐上限和资源耗尽边界。

本次压测重点不是功能正确性本身，而是：

- 房间持续加入/退出是否稳定
- WebSocket `join`、`plainPublish`、`plainSubscribe` 是否在高压下保持成功
- `mediasoup-worker` 与 `mediasoup-sfu` 的 CPU / RSS / 房间数 / 队列负载是否随压力可观测
- 系统在不断加压时，最终是先出现性能耗尽，还是先出现端口、房间、信令或 worker 异常

## 2. 环境前提

- 目标机器：`root@172.31.4.40`
- 运行实例：单个 `mediasoup-sfu` 容器
- worker 数量：`1`
- worker thread 数量：`1`
- WebRTC 路径：启用 `WebRtcServer`
- PlainTransport：保留，仅用于后端媒体压力，不做浏览器 plain client 验证

## 3. 测试拓扑

每个房间固定为：

- `1` 个 publisher
- `2` 个 subscriber
- 全部 peer 都走完整 WebSocket `join`
- 后续媒体接入使用后端 `plainPublish` / `plainSubscribe`

单房间信令链路：

```text
pub    -> join -> plainPublish
sub1   -> join -> plainSubscribe
sub2   -> join -> plainSubscribe
```

这条拓扑适合持续 churn，且能稳定模拟“多房间、稳定收发”的服务端负载。

## 4. 压测节奏

采用“**每 10 秒增加 10 个房间**”的阶梯式加压方式。

建议默认参数：

- 初始房间数：`10`
- 每轮增量：`10`
- 采样窗口：`10s`
- 保持收发速率：固定不变
- churn 周期：持续到性能耗尽或达到预设上限

可执行的节奏示例：

```text
t=0s      : 10 rooms
t=10s     : 20 rooms
t=20s     : 30 rooms
t=30s     : 40 rooms
...
```

如果某一档已经出现明显退化，就不要继续提高该档位，先保留现场数据。

## 5. 采样指标

每 10 秒同步采样一次，分成三层：

### 5.1 Worker 层

采样对象：

- `mediasoup-worker` CPU
- `mediasoup-worker` RSS
- UDP drops / socket drops
- RTP 发包 / 收包速率

建议采样来源：

- `tests/bench_worker_load.cpp` 里的 `/proc` 采样逻辑
- 或直接从 `/proc/<workerPid>/stat`、`/proc/<workerPid>/status`、`/proc/net/udp` 取值

### 5.2 SFU 层

采样对象：

- `mediasoup-sfu` CPU
- `mediasoup-sfu` RSS
- `GET /healthz`
- `GET /readyz`
- `GET /api/node-load`
- `GET /metrics`

重点字段：

- `rooms`
- `workers`
- `workerThreads`
- `availableWorkerThreads`
- `dispatchRooms`
- `workerQueueStats`
- `roomOwnership`

### 5.3 业务层

采样对象：

- 每个房间的 send pps
- 每个 subscriber 的 recv pps
- `recvRatio`
- join / publish / subscribe 是否失败
- churn 后房间是否还能恢复到健康状态

## 6. `recvRatio` 的定义

当前压测脚本里的 `recvRatio` 不是协议指标，而是采样窗口内的粗略接收比例：

```text
recvRatio = (recv1 + recv2) / (send * 2)
```

因为每个房间有两个 subscriber，所以：

- `1.00` 表示接收完全对齐
- `0.93 ~ 0.95` 表示在当前采样窗口里，整体收包大约在 93% 到 95%

它是稳定性粗指标，不是精确网络丢包率。

建议判定：

- 稳态：`recvRatio >= 0.93`
- churn 期间：`recvRatio >= 0.90`
- 若低于阈值但 join / transport / room 生命周期都正常，先判断是否是窗口边界或 churn 过渡导致，而不是直接判失败

## 7. 加压停止条件

满足任一条件就停止继续加压：

- `join`、`plainPublish`、`plainSubscribe` 出现失败
- `GET /healthz` 或 `GET /readyz` 变为异常
- `room` 数无法继续按预期增长
- `availableWorkerThreads` 下降到 0 且无法恢复
- `workerQueueStats` 持续恶化，且排队时延显著上升
- `recvRatio` 连续 3 个采样窗口低于阈值
- `mediasoup-worker` 进程退出、重启或报错
- 出现端口耗尽、transport 创建失败、socket 错误
- CPU / RSS 到达目标机器可接受上限并开始持续退化

## 8. 建议执行矩阵

### 8.1 Smoke

- 10 rooms
- 3 个采样窗口
- 仅确认流程打通和采样链路正常

### 8.2 Baseline

- 10 -> 20 -> 30 rooms
- 每档保持 10 秒采样
- 观察房间数、worker CPU、SFU CPU、RSS 是否线性增长

### 8.3 Stress

- 从 10 rooms 继续每 10 秒加 10 个房间
- 一直跑到出现明确性能耗尽或功能失败

### 8.4 Churn

在达到某个稳定档位后，继续做批量进出：

- 每轮删除 10 个房间
- 立即再补 10 个房间
- 重复多轮

这个阶段用于验证：

- 房间回收是否干净
- 资源释放是否及时
- churn 后能否回到原始稳定档位

## 9. 推荐执行顺序

1. 先跑 Smoke，确认测试脚本和监控采样都正常。
2. 跑 Baseline，确认 `10 -> 20 -> 30` 的增长趋势。
3. 跑 Stress，直到性能耗尽。
4. 在峰值附近做 Churn，验证反复进出是否会把系统拖坏。

## 10. 预期产出

每轮测试至少记录：

- 时间戳
- 房间数
- worker CPU / RSS
- sfu CPU / RSS
- `healthz` / `readyz`
- `api/node-load`
- `metrics`
- send / recv / recvRatio
- 失败原因或停止原因

## 11. 结论判定

如果在单 worker 条件下：

- 10 秒一档持续加压
- join / publish / subscribe 保持成功
- `healthz` 保持正常
- `workerQueueStats` 没有持续失控
- `recvRatio` 仍在可接受范围

则可以认为当前这版实现的单 worker 承载能力达到了可接受水平。

如果先出现端口耗尽、room 回收异常、worker 崩溃或队列失控，就说明当前实现的瓶颈已经暴露，后续需要针对那一层做优化。

## 12. 结果解读约束

不要把 `tests/qos_harness/single_worker_pressure.mjs` 的单次失败阈值直接当成 worker 的真实容量上限。

原因是：

- 这个脚本把信令、发包、收包和采样都放在一个 Node 进程里
- 当房间数上来后，脚本自身会先变重，进而干扰 `recvRatio` 和采样窗口
- 它适合做“业务链路压力”观察，不适合单独作为 worker 极限判定

判定 worker 本身的容量上限时，优先级应当是：

1. `tests/bench_worker_load.cpp` 这类低开销基准
2. 浏览器路径的 `browser_capacity_rooms` 真实 E2E 压测
3. 最后才参考 Node 压测脚本的退化点
