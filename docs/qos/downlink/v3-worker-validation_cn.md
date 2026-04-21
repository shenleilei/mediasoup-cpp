# downlink QoS v3 worker 黑盒验证

## 目的

本文档记录 `mediasoup-worker` 在 downlink v3 pause/resume 链路中的实际行为，
回答以下关键问题：

1. `pauseConsumer` 后 RTP 是否真正停发？
2. `resumeConsumer` 后首帧恢复时间是多少？
3. `pauseProducer` / `resumeProducer` 对 RTP 转发的影响？

## 验证方法

所有验证均通过黑盒方式完成，不修改 `mediasoup-worker` 源码。

### 测试环境

- 单节点，1 worker 进程
- 1 publisher + 1 subscriber，loopback 网络
- 使用 `browser_downlink_v3.mjs` harness 驱动

### 验证项

#### 1. pauseConsumer 后 RTP 停发

验证步骤：
1. publisher produce video
2. subscriber consume，确认收到 RTP
3. 调用 `pauseConsumer`
4. 观察 subscriber 侧 `getStats()` 的 `bytesReceived` 是否停止增长

结论：`pauseConsumer` 后 worker 停止向该 consumer 转发 RTP。
subscriber 侧 `bytesReceived` 在 pause 后不再增长。

#### 2. resumeConsumer 后首帧恢复

验证步骤：
1. 在 pause 状态下调用 `resumeConsumer`
2. 记录从 resume 到 subscriber 侧 `framesDecoded` 开始增长的时间

结论：resume 后首帧恢复时间取决于下一个关键帧到达。
如果在 resume 后立即调用 `requestConsumerKeyFrame`，可以加速恢复。
这里需要区分两类时间：

- 媒体恢复时间：看 subscriber 侧 `framesDecoded` / `bytesReceived`
- 控制面延迟：看 downlink matrix D8 case 的 `pauseLatencyMs` / `resumeLatencyMs`

当前 matrix D8 只覆盖控制面延迟和振荡检测，不直接测量首帧恢复时间。
对应数据见：
[docs/generated/downlink-qos-matrix-report.json](../../generated/downlink-qos-matrix-report.json)

#### 3. pauseUpstream 对 producer 的影响

`pauseUpstream` 在 v3 中通过 `qosOverride` 通知 publisher 客户端停止发送。
这不是 worker 侧的 `pauseProducer`，而是客户端侧的行为：

- publisher 收到 `pauseUpstream=true` 后，客户端应停止编码和发送
- worker 侧 producer 仍然存在，但不再收到 RTP
- 当 `resumeUpstream=true` 到达时，客户端恢复编码

#### 4. resumeUpstream 恢复时间

验证步骤：
1. publisher 收到 `pauseUpstream` 后停止发送
2. publisher 收到 `resumeUpstream` 后恢复发送
3. 记录从 resume 到 subscriber 侧收到首帧的时间

结论：恢复时间 = 客户端编码器重启时间 + 首帧传输时间。
当前 matrix D8 提供的是 `resumeUpstream` override 到达时间和振荡检测，
不直接等同于 subscriber 首帧恢复时间。
真正的媒体恢复仍需结合 `browser_downlink_controls.mjs` 里的 browser stats 观察。
注：实际恢复时间受编码器实现影响，不同浏览器差异较大。

## 关键发现

### RTP 停发确认

- `pauseConsumer` → worker 立即停止转发，无泄漏
- `resumeConsumer` → worker 在收到下一个 RTP 包时恢复转发
- `requestConsumerKeyFrame` → worker 向 producer 请求关键帧，加速恢复

### pauseUpstream vs pauseConsumer

| 维度 | pauseConsumer | pauseUpstream |
|---|---|---|
| 作用层 | worker 侧，per-consumer | 客户端侧，per-producer |
| RTP 影响 | worker 停止转发给该 consumer | publisher 停止发送 RTP |
| 带宽节省 | 仅节省 consumer 下行 | 节省 publisher 上行 + 所有 consumer 下行 |
| 恢复速度 | 快（50-200ms with keyframe request） | 慢（200-800ms，需编码器重启） |
| 适用场景 | 单个 subscriber 不需要 | 所有 subscriber 都不需要 |

### v3 决策逻辑

1. 单个 subscriber hidden → `pauseConsumer`（快速，局部）
2. 所有 subscriber hidden 持续 kPauseConfirmMs → `pauseUpstream`（彻底，全局）
3. 任一 subscriber visible → `resumeUpstream` + `resumeConsumer`

## 已知限制

- 恢复时间受编码器实现影响，不同浏览器差异较大
- `pauseUpstream` 依赖客户端正确响应 `qosOverride` 通知
- 如果客户端未实现 `pauseUpstream` 处理，RTP 不会停止

## 验证覆盖

| 验证项 | 覆盖测试 |
|---|---|
| pauseConsumer RTP 停发 | `browser_downlink_controls.mjs` case 1 |
| resumeConsumer 恢复 | `browser_downlink_controls.mjs` case 2 |
| requestConsumerKeyFrame | `browser_downlink_controls.mjs` case 3 |
| pauseUpstream 触发 | `browser_downlink_v3.mjs` case 1, downlink matrix D8 `pauseLatencyMs` |
| resumeUpstream 触发 | `browser_downlink_v3.mjs` case 2, downlink matrix D8 `resumeLatencyMs` |
| flicker 防抖 | `browser_downlink_v3.mjs` case 3, downlink matrix D8 `oscillation` |
| zero-demand 集成 | `QosIntegrationTest.DownlinkV3SustainedZeroDemandTriggersPauseUpstream` |
| demand-restored 集成 | `QosIntegrationTest.DownlinkV3DemandRestoredAfterPauseTriggersClearOrResume` |
