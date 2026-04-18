# 下行 QoS v1 设计方案

## 1. 文档目的

这份文档给出当前仓库 `subscriber/downlink QoS v1` 的设计方案。

当前仓库的 QoS 主线仍然是上行 / publisher 侧：

- 浏览器侧 publisher QoS controller
- 服务端 `qosPolicy / qosOverride / automatic override`
- 面向 uplink 的 browser matrix / targeted harness / 集成测试

下一阶段最值得投入的能力，不是继续只调 uplink 阈值，而是补齐 subscriber/downlink QoS。

本文只覆盖 `v1`。

如果要看 `v2` 的房间级 demand 聚合、publisher 供给回收和显式 bitrate allocator，请看：

- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)

## 2. 为什么现在该做下行 QoS

继续只打磨 uplink，边际收益会开始下降。

真实通话里，用户最直接感受到的是：

- 远端视频是不是清楚
- 主讲人是不是稳定
- 宫格视图是不是能自动合理降档
- 隐藏视频是不是还在白白消耗带宽
- 差网恢复后是不是会来回抖动

这些问题主要是 subscriber/downlink 侧问题，不是继续调 publisher uplink 就能解决的。

## 3. 当前仓库已经具备的基础能力

### 3.1 已有下行观测

项目已经能采到或暴露这些下行信号：

- 浏览器 `recvTransport` stats
- 浏览器 `inboundVideo / inboundAudio`
- 浏览器 `videoFreeze`
- 服务端 `recvTransport` stats
- 服务端 consumer score / producer score

相关文件：

- [RoomServiceStats.cpp](../src/RoomServiceStats.cpp)
- [qos-demo.js](../public/qos-demo.js)

### 3.2 已有下行动作

服务端对单个 consumer 已经具备这些控制能力：

- `pause()`
- `resume()`
- `setPreferredLayers(spatialLayer, temporalLayer)`
- `setPriority(priority)`
- `requestKeyFrame()`

相关文件：

- [Consumer.h](../src/Consumer.h)
- [Consumer.cpp](../src/Consumer.cpp)

### 3.3 当前限制

当前客户端 QoS controller 是 publisher-only。

相关文件：

- [controller.js](../src/client/lib/qos/controller.js)
- [types.d.ts](../src/client/lib/qos/types.d.ts)

现在的动作模型仍然是上行发送控制：

- `setEncodingParameters`
- `setMaxSpatialLayer`
- `enterAudioOnly`
- `pauseUpstream`
- `resumeUpstream`

所以要做下行 QoS，最好单独起一套 subscriber/downlink 控制模型，而不是继续往 publisher controller 里硬塞逻辑。

## 4. V1 范围

`v1` 建议只做这些：

- 按 subscriber 粒度做视频 consumer 分配
- 根据可见性决定是否暂停远端视频
- 根据视图尺寸选择 preferred layers
- 根据真实下行健康度做降档和恢复
- 在恢复升档时请求关键帧

`v1` 暂时不做更高一层的房间级供需闭环。

这部分统一见：

- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)

## 5. 总体模型

对每个 subscriber peer：

1. 客户端上报下行状态和 UI hint
2. 服务端保存该 subscriber 的最新快照
3. 服务端为这个 subscriber 的每个 remote consumer 计算目标接收层
4. 服务端执行 consumer 侧动作

这会让 QoS 从“只有 uplink”变成“双向”：

- uplink QoS：控制 publisher 发送策略
- downlink QoS：控制 subscriber 接收策略

## 6. 建议的下行快照 Schema

不要复用 uplink schema，建议新增：

- `mediasoup.qos.downlink.client.v1`

建议结构：

```json
{
  "schema": "mediasoup.qos.downlink.client.v1",
  "seq": 12,
  "tsMs": 1710000000000,
  "subscriberPeerId": "alice",
  "transport": {
    "availableIncomingBitrate": 1200000,
    "currentRoundTripTime": 0.09
  },
  "subscriptions": [
    {
      "consumerId": "consumer-1",
      "producerId": "producer-1",
      "publisherPeerId": "bob",
      "kind": "video",
      "source": "camera",
      "visible": true,
      "pinned": false,
      "activeSpeaker": true,
      "isScreenShare": false,
      "targetWidth": 640,
      "targetHeight": 360,
      "packetsLost": 3,
      "jitter": 0.02,
      "framesPerSecond": 25,
      "frameWidth": 640,
      "frameHeight": 360,
      "freezeRate": 0.01,
      "recentFreezeRate": 0.00
    }
  ]
}
```

关键点：

- uplink 快照说的是“发送者当前怎么样”
- downlink 快照说的是“接收者当前实际收到了什么、页面需要什么”

## 7. 新增服务端组件建议

### 7.1 `DownlinkQosRegistry`

职责：

- 保存每个 subscriber 的最新 downlink snapshot
- 丢弃过期 / 非法 / 乱序快照
- 提供当前 subscriber 状态给 controller 使用

建议文件：

- `src/DownlinkQosRegistry.h`
- `src/DownlinkQosRegistry.cpp`

### 7.2 `SubscriberQosController`

职责：

- 针对某个 subscriber，决定它的每个 remote consumer 应该收什么层
- 决定 pause / resume / preferred layers / priority / keyframe
- 管理恢复状态，避免振荡

建议文件：

- `src/SubscriberQosController.h`
- `src/SubscriberQosController.cpp`

### 7.3 `DownlinkAllocator`

职责：

- 把 UI 需求和网络健康度整合成最终动作
- 对 consumer 做重要性排序
- 按“对体验伤害最小”的顺序降级

建议文件：

- `src/DownlinkAllocator.h`
- `src/DownlinkAllocator.cpp`

## 8. 动作模型

`SubscriberQosController` 在 `v1` 里只需要输出这些动作：

- `pauseConsumer`
- `resumeConsumer`
- `setPreferredLayers`
- `setPriority`
- `requestKeyFrame`
- `noop`

这些都能直接映射到当前已有的 `Consumer` 能力。

## 9. 分配策略

### 9.1 第一层：由 UI 决定目标层

先根据 subscriber 页面里的显示需求，给每个 remote consumer 设一个“想要的目标层”。

建议规则：

- hidden / offscreen：
  直接 `pause`
- 很小的 thumbnail：
  最低 spatial layer
- 普通 grid tile：
  中等 spatial layer
- pinned speaker：
  高 spatial layer
- screenshare：
  最高优先级、尽量给高层

### 9.2 第二层：由真实下行健康度修正

在 UI 目标之上，再结合实际 downlink 健康度做修正：

- freeze rate 上升：
  快速降档
- FPS 明显下降：
  降档
- loss / jitter / RTT 恶化：
  先降低低优先级 consumer
- `availableIncomingBitrate` 紧张：
  按优先级从低到高依次收缩

### 9.3 恢复策略

恢复不要一步拉满，建议：

- 每次只升一档
- 升档后请求关键帧
- 连续健康多个 sample 后才能继续升档
- “停止紧急降级”的门槛可以比“恢复到高质量”的门槛更宽松

## 10. 优先级模型

建议的默认优先级：

- screenshare: `255`
- pinned: `220`
- active speaker: `180`
- visible grid: `120`
- hidden: `0`

这个优先级足够支撑 `v1`，而且和用户感知价值基本一致。

## 11. v2 另见

本文不展开 `v2`。

`v2` 的设计统一见：

- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)

## 12. 需要新增 / 修改的文件

### 新增文件

- `src/DownlinkQosRegistry.h`
- `src/DownlinkQosRegistry.cpp`
- `src/SubscriberQosController.h`
- `src/SubscriberQosController.cpp`
- `src/DownlinkAllocator.h`
- `src/DownlinkAllocator.cpp`
- `src/client/lib/qos/downlinkProtocol.js`
- `src/client/lib/qos/downlinkTypes.d.ts`
- `src/client/lib/qos/downlinkSampler.js`
- `src/client/lib/qos/downlinkHints.js`

### 需要修改的现有文件

- [SignalingServerWs.cpp](../src/SignalingServerWs.cpp)
  增加 `downlinkClientStats` method
- [RoomService.h](../src/RoomService.h)
- [RoomServiceStats.cpp](../src/RoomServiceStats.cpp)
  存储快照并更新 downlink registry
- [RoomServiceDownlink.cpp](../src/RoomServiceDownlink.cpp)
  驱动 subscriber controller 和 room-scoped planner
- [qos-demo.js](../public/qos-demo.js)
  先作为 downlink QoS 的首个 demo 入口

## 13. 测试计划

### 13.1 单测

- hidden consumer -> `pauseConsumer`
- pinned consumer 优先级高于 grid consumer
- freeze / loss / jitter 触发降档
- 恢复时一层一层升档
- screenshare 始终最高优先级

### 13.2 集成测试

- 两个 subscriber 请求不同目标尺寸，收到不同 preferred layers
- hidden 视频被服务端暂停
- 升档时触发 keyframe
- 带宽紧张时，低优先级 consumer 先降

### 13.3 Browser harness

- 一个大窗 + 一个小窗
- 一个 pinned + 一个普通宫格
- hidden/offscreen 转场
- 下行弱网 + freeze 场景

## 14. 建议实施顺序

1. 定义 downlink snapshot schema
2. 在浏览器侧跑通上报
3. 实现 `DownlinkQosRegistry`
4. 实现简单版 `DownlinkAllocator`
5. 先接入：
   - `pause/resume`
   - `setPreferredLayers`
6. 再补：
   - recovery
   - keyframe
   - priority
7. `v1` 稳定后，再进入 [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)

## 15. V1 验收标准

`v1` 至少应满足：

- hidden 视频会被服务端暂停
- 大窗和小窗能收到不同视频层
- pinned 视频在带宽紧张时更晚降档
- freeze / loss / jitter 触发的降档是稳定可观测的
- 恢复不是振荡式来回跳

## 16. 总结

当前收益最大的方向不是继续只调 uplink，而是先把 `subscriber/downlink QoS v1` 做稳。

`v1` 稳定以后，再进入 [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) 定义的房间级下行闭环能力。
