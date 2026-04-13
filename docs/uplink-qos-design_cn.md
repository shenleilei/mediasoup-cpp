# 上行 QoS 设计说明

## 1. 文档目的

这份文档用中文说明当前仓库里已经实现的 `uplink QoS` 方案：

- 它在做什么
- 控制链路怎么流转
- 浏览器侧和服务端各自负责什么
- 它和底层 WebRTC / mediasoup-worker 的边界是什么

如果要看当前测试结论和签收口径，请继续参考：

- [uplink-qos-final-report.md](./uplink-qos-final-report.md)
- [uplink-qos-test-results-summary.md](./uplink-qos-test-results-summary.md)
- [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)

## 2. 方案定位

当前仓库里的 uplink QoS，不是在重写底层 WebRTC 传输修复机制。

它的定位更准确地说是：

- 浏览器侧：
  根据 sender stats 判断当前发送质量状态
- 服务端：
  聚合 client snapshot、生成默认策略、下发 override、做自动保护
- 控制目标：
  调整发送层级、码率、分辨率、音频优先模式、上行暂停/恢复

也就是说，这是一套“更上层的发送策略 QoS”。

底层这些能力主要仍然由浏览器 WebRTC 栈和 mediasoup-worker 提供：

- NACK
- RTX
- PLI / FIR
- 基础带宽估计
- RTP 统计与 RTCP 反馈

## 3. 总体架构

当前 uplink QoS 可以分成 4 层：

1. 浏览器 sender stats 采集层
2. 浏览器 QoS state machine / planner / executor
3. 服务端 snapshot 聚合与 policy / override 控制层
4. 服务端自动 override 和房间级质量广播

## 4. 浏览器侧链路

### 4.1 采样

浏览器侧会周期性从 sender / transport / peer connection 中取样，形成发送侧快照。

核心关注的信号包括：

- send bitrate
- target bitrate
- bitrate utilization
- loss rate
- RTT
- jitter
- quality limitation reason
- frame width / height / fps

当前客户端 QoS 相关代码位于：

- [controller.js](../src/client/lib/qos/controller.js)
- [sampler.js](../src/client/lib/qos/sampler.js)
- [signals.js](../src/client/lib/qos/signals.js)
- [stateMachine.js](../src/client/lib/qos/stateMachine.js)
- [planner.js](../src/client/lib/qos/planner.js)
- [executor.js](../src/client/lib/qos/executor.js)
- [protocol.js](../src/client/lib/qos/protocol.js)
- [types.d.ts](../src/client/lib/qos/types.d.ts)

### 4.2 状态机

浏览器侧 controller 会把采样信号映射成 QoS 状态：

- `stable`
- `early_warning`
- `congested`
- `recovering`

这些状态不是直接等于“网络好/坏”，而是面向策略控制的工作状态。

状态机还会维护一些恢复与 debounce 相关信息，例如：

- 连续健康 sample 数
- 连续拥塞 sample 数
- recovery streak
- probe 过程中的成功 / 失败样本

### 4.3 规划动作

状态机输出状态后，planner 会把状态翻译成动作计划。

当前动作模型主要是：

- `setEncodingParameters`
- `setMaxSpatialLayer`
- `enterAudioOnly`
- `exitAudioOnly`
- `pauseUpstream`
- `resumeUpstream`
- `noop`

这些动作定义见：

- [types.d.ts](../src/client/lib/qos/types.d.ts)

### 4.4 执行动作

executor / adapter 层负责把这些抽象动作真正落到 producer / transport 上。

例如：

- 限制 spatial layer
- 暂停上行视频
- 恢复上行视频
- 进入 audio-only 模式

相关文件：

- [producerAdapter.js](../src/client/lib/qos/adapters/producerAdapter.js)
- [factory.js](../src/client/lib/qos/factory.js)
- [executor.js](../src/client/lib/qos/executor.js)

## 5. 浏览器到服务端：client snapshot 上报

浏览器侧会把当前 uplink QoS 状态序列化成：

- `mediasoup.qos.client.v1`

这份 snapshot 通过信令方法 `clientStats` 发送到服务端。

相关文件：

- [protocol.js](../src/client/lib/qos/protocol.js)
- [signalChannel.js](../src/client/lib/qos/adapters/signalChannel.js)
- [SignalingServer.cpp](../src/SignalingServer.cpp)

## 6. 服务端链路

### 6.1 接收 clientStats

服务端主线程在 `SignalingServer` 中接收 `clientStats` 请求。

它先做轻量校验，然后把任务投递到房间所属的 `WorkerThread` 中执行。

相关代码：

- [SignalingServer.cpp](../src/SignalingServer.cpp)

### 6.2 `RoomService::setClientStats`

真正的 snapshot 处理发生在：

- [RoomService.cpp](../src/RoomService.cpp)

这一步大致做这些事情：

1. 解析并校验客户端 snapshot
2. 更新服务端 QoS registry
3. 聚合当前 peer 的 QoS 状态
4. 触发连接质量通知
5. 视情况发送自动 `qosOverride`
6. 视情况广播房间级 QoS 状态

### 6.3 policy 与 override

服务端当前能向客户端下发两类控制消息：

- `qosPolicy`
- `qosOverride`

其中：

- `qosPolicy`
  更像默认运行参数，例如 sample interval、snapshot interval、profile 名称
- `qosOverride`
  更像临时强制控制，例如 clamp 最大层级、强制 audio-only、禁用恢复等

相关逻辑主要在：

- [RoomService.cpp](../src/RoomService.cpp)

## 7. 自动 override

当前服务端已经具备自动 override 能力。

典型场景：

- 客户端上报质量持续恶化
- 服务端判断达到自动保护门槛
- 下发 `server_auto_poor` / `server_auto_lost` 一类 override
- 质量恢复后再自动 clear

这一层的价值在于：

- 客户端可以做本地状态判断
- 服务端还能站在房间和系统视角追加保护逻辑

## 8. 房间级视角

当前 uplink QoS 不只是“一个 sender 的本地自适应”。

服务端还会维护房间级聚合视角，例如：

- 当前房间里多少 peer 质量变差
- 某个 peer 是否 stale
- 是否需要广播房间级 QoS 状态

这使得 uplink QoS 不只是点对点的 sender optimization，而是开始具备 room-aware control plane 的特征。

## 9. 当前 uplink QoS 的边界

这套 uplink QoS 当前明确做的是：

- 发送侧 stats 采样
- 发送侧状态判断
- 发送侧动作执行
- 服务端 snapshot 聚合
- 服务端 policy / override / auto-protection

它当前没有完整覆盖的是：

- subscriber/downlink QoS
- 完整 dynacast
- 更强的 room-wide bandwidth allocator
- 更大规模生产数据下的长期校准

这也是为什么下一阶段的重点应该放在 downlink。

## 10. 当前方案的优点

当前 uplink QoS 已经有几个比较明确的优点：

- 不是纯浏览器本地逻辑，而是有 server control plane
- 有 policy / override / automatic override 的分层控制
- 有浏览器 matrix / targeted rerun / harness 作为验证基础
- 已经把状态机、planner、executor 这些职责拆开了，后续扩展较容易

## 11. 当前方案的主要不足

当前 uplink QoS 仍然存在这些限制：

- 主要覆盖 publisher 侧
- 对下行体验提升有限
- profile / policy 还偏代码内嵌
- 与更完整的 subscriber allocator / dynacast 仍未形成闭环
- 生产环境下长期校准和观测能力仍需加强

## 12. 建议的理解方式

对外描述时，建议把当前 uplink QoS 理解成：

`在浏览器 WebRTC 与 mediasoup-worker 底层传输能力之上，增加一层可观测、可下发、可自动保护的发送策略控制面。`

这个描述比“我们实现了一整套 RTC QoS”更准确。

## 13. 与下一阶段的关系

下一阶段建议不是继续只调 uplink 阈值，而是：

1. 保持当前 uplink QoS 主线稳定
2. 新增 subscriber/downlink QoS
3. 再进一步做 adaptive stream / dynacast

这样路径才会更像完整产品能力：

- uplink QoS
- downlink QoS
- room-level allocator
- dynacast

## 14. 总结

当前仓库里的 uplink QoS 已经不是玩具级实现，而是具备：

- 客户端状态机
- 动作规划与执行
- 服务端 control plane
- policy / override / auto-protection
- 自动化验证闭环

但它仍然主要是 `publisher/uplink QoS`。

所以现在最合理的演进方向不是继续只做 uplink 微调，而是以这套 uplink 方案为基础，补齐 `subscriber/downlink QoS`，最终形成双向 QoS 体系。
