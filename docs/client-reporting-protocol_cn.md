# 端上上报协议

这份文档只总结当前客户端**主动发给服务端**的消息。

范围只包含：

- WebSocket `request=true`
- 客户端主动请求 / 上报

不包含：

- 服务端主动 `notification`
- WebRTC 媒体包本身

## 1. 总览

当前端上上报大致分 4 类：

1. 入房与能力声明
2. 传输与媒体控制
3. 订阅与消费
4. QoS / 统计 / 观测

## 2. 入房与能力声明

### `join`

含义：

- 客户端加入房间

最小请求：

```json
{
  "request": true,
  "id": 1,
  "method": "join",
  "data": {
    "roomId": "ZL15812",
    "peerId": "peer-a",
    "displayName": "peer-a"
  }
}
```

可选能力字段：

```json
{
  "rtpCapabilities": { }
}
```

说明：

- 当前协议允许 `join` 不带 `rtpCapabilities`
- 但如果端上此时已经拿到了真实能力，也可以带上

服务端响应会返回：

- `routerRtpCapabilities`
- `existingProducers`
- `participants`
- `qosPolicy`

`existingProducers[]` 是加入时房间里已经存在的 producer 候选列表。每一项包含：

- `producerId`
- `producerPeerId`
- `kind`
- `appData`

如果发布端在 `produce.appData.source` 里带了业务来源，订阅端从 `existingProducers[].appData.source` 读取。它不是裸 `source` 字段。

客户端收到后，通常马上做两件事：

1. `device.load(routerRtpCapabilities)`
2. 创建接收 transport

## 3. 传输与媒体控制

### `createWebRtcTransport`

含义：

- 创建 WebRTC transport

这是当前最重要的一条上报之一。

#### 创建接收 transport

```json
{
  "request": true,
  "id": 2,
  "method": "createWebRtcTransport",
  "data": {
    "producing": false,
    "consuming": true,
    "rtpCapabilities": { }
  }
}
```

这里最关键的是：

- `consuming=true`
- `rtpCapabilities=device.rtpCapabilities`

说明：

- 当前协议下，如果 `join` 没带能力
- 这一步就是最关键的补偿点
- 服务端会把这份能力写到当前 peer 上

如果这一步也不带能力，服务端就只能依赖兜底或历史行为。

#### 创建发送 transport

```json
{
  "request": true,
  "id": 3,
  "method": "createWebRtcTransport",
  "data": {
    "producing": true,
    "consuming": false
  }
}
```

说明：

- 发送 transport 不需要带 `rtpCapabilities`
- 因为它不是消费能力声明点

### `connectWebRtcTransport`

含义：

- 把客户端本地 transport 的 DTLS 参数回传给服务端

最小请求：

```json
{
  "request": true,
  "id": 4,
  "method": "connectWebRtcTransport",
  "data": {
    "transportId": "transport-1",
    "dtlsParameters": { }
  }
}
```

说明：

- 发送 transport 和接收 transport 都需要走这一步

### `restartIce`

含义：

- 对已有 transport 触发 ICE 重协商

最小请求：

```json
{
  "request": true,
  "id": 5,
  "method": "restartIce",
  "data": {
    "transportId": "transport-1"
  }
}
```

## 4. 发送媒体

### `produce`

含义：

- 客户端把本地音视频轨道发布到服务端

最小请求：

```json
{
  "request": true,
  "id": 6,
  "method": "produce",
  "data": {
    "transportId": "send-transport-1",
    "kind": "video",
    "rtpParameters": { },
    "appData": {
      "source": "vehicle-left-door"
    }
  }
}
```

关键字段：

- `transportId`
- `kind`
- `rtpParameters`
- `appData.source`

`appData.source` 是业务自定义的 producer 来源标识。
服务端只校验它是字符串，不把它限定成 `audio` / `camera` / `screenShare` 这类固定枚举。
媒体类型由 `kind` 表达，`source` 用来让业务把某个 producer 绑定到自己的摄像头、车载位置、设备通道或其它业务来源。

如果端上希望订阅端识别这路流，应该在 `produce` 时稳定传入 `appData.source`。服务端会把这份 `appData` 保存到该 producer 上，并在后续 `newConsumer`、`createWebRtcTransport(consuming=true)` 返回的 `consumers`、以及显式 `consume` 响应里一起返回。

注意：下行仍然返回 `appData.source`，不是裸 `source` 字段。

### 发送后的影响

当 `produce` 成功后，服务端会做两件事：

1. 记录新的 producer
2. 对房间里其他可订阅 peer 触发 auto-subscribe

所以 `produce` 不是单纯“上传一条流”，它还会影响房间里其他人的订阅状态。

### `closeProducer`

含义：

- 客户端停止自己已经发布的一路 producer
- 服务端关闭该 producer 派生到订阅端的 consumers
- 服务端给受影响订阅端推 `producerLeft`

按 producerId 关闭：

```json
{
  "request": true,
  "id": 7,
  "method": "closeProducer",
  "data": {
    "producerId": "producer-audio-1"
  }
}
```

按业务 source 关闭：

```json
{
  "request": true,
  "id": 8,
  "method": "closeProducer",
  "data": {
    "source": "audio"
  }
}
```

成功响应：

```json
{
  "response": true,
  "id": 8,
  "ok": true,
  "data": {
    "producerId": "producer-audio-1",
    "closedConsumers": 1,
    "notifiedPeers": ["peer-b"]
  }
}
```

端上推荐顺序：

```text
closeProducer(producerId 或 source)
本地 producer.close()
停止对应 MediaStreamTrack
从本地 publishedProducers 删除
```

注意：

- 只能关闭本 peer 自己的 producer；关闭别人的 producer 会返回 `permission denied`。
- `source` 只适合同一个 peer 下唯一 producer 匹配；多路同 source 会返回 `ambiguous producer source`。
- `closeProducer` 不自动释放音频受限端 slot；音频受限业务关闭还需要 `releaseAudioRestrictedSlot`。

## 5. 订阅与消费

### `consume`

含义：

- 客户端请求消费某个已有 producer

最小请求：

```json
{
  "request": true,
  "id": 9,
  "method": "consume",
  "data": {
    "transportId": "recv-transport-1",
    "producerId": "producer-1",
    "rtpCapabilities": { }
  }
}
```

关键点：

- 这里的 `rtpCapabilities` 必须是本端真实消费能力
- 服务端会据此做 codec 匹配
- 成功后返回：
  - `id`（consumerId）
  - `kind`
  - `producerId`
  - `peerId`
  - `appData`
  - `rtpParameters`
  - `producerPaused`（可选状态字段，最小接入可忽略）

响应示例：

```json
{
  "response": true,
  "id": 7,
  "ok": true,
  "data": {
    "peerId": "vehicle_ZL15812",
    "producerId": "producer-video-1",
    "id": "consumer-video-1",
    "kind": "video",
    "appData": {
      "source": "vehicle-left-door"
    },
    "rtpParameters": { },
    "producerPaused": false
  }
}
```

这里的 `peerId` 是 producer 所属 peer，不是当前订阅端 peer。这里的 `appData` 来自发布端 `produce.appData`，端上读取业务来源时使用 `data.appData.source`。

`producerPaused` 只是 producer 当前暂停状态提示，不是 `recvTransport.consume(...)` 的必需入参。

### `requestConsumerKeyFrame`

含义：

- 请求远端关键帧

最小请求：

```json
{
  "request": true,
  "id": 10,
  "method": "requestConsumerKeyFrame",
  "data": {
    "consumerId": "consumer-1"
  }
}
```

适用场景：

- 新视频刚接上时尽快出图
- 订阅恢复后尽快补画面

### `pauseConsumer` / `resumeConsumer`

含义：

- 暂停 / 恢复某条下行 consumer

最小请求：

```json
{
  "request": true,
  "id": 11,
  "method": "pauseConsumer",
  "data": {
    "consumerId": "consumer-1"
  }
}
```

```json
{
  "request": true,
  "id": 12,
  "method": "resumeConsumer",
  "data": {
    "consumerId": "consumer-1"
  }
}
```

### `setConsumerPreferredLayers`

含义：

- 对 simulcast / SVC 的 consumer 设首选层

最小请求：

```json
{
  "request": true,
  "id": 13,
  "method": "setConsumerPreferredLayers",
  "data": {
    "consumerId": "consumer-1",
    "spatialLayer": 1,
    "temporalLayer": 2
  }
}
```

### `setConsumerPriority`

含义：

- 设置某条 consumer 的优先级

最小请求：

```json
{
  "request": true,
  "id": 14,
  "method": "setConsumerPriority",
  "data": {
    "consumerId": "consumer-1",
    "priority": 200
  }
}
```

## 6. QoS 与统计上报

### `clientStats`

含义：

- 发流端上报自己的上行 QoS 快照

说明：

- 这是**上行视角**
- 主要服务于服务端 QoS 判断

最小结构：

```json
{
  "request": true,
  "id": 20,
  "method": "clientStats",
  "data": {
    "schema": "mediasoup.qos.client.v1",
    "seq": 1,
    "tsMs": 1780391000000,
    "peerState": {
      "mode": "audio-video",
      "quality": "excellent",
      "stale": false
    },
    "tracks": []
  }
}
```

常见用途：

- 服务端生成 `qosConnectionQuality`
- 服务端下发 `qosOverride`
- 统计与调试

### `downlinkClientStats`

含义：

- 订阅端上报自己的下行消费快照

说明：

- 这是**下行视角**
- 反映当前 consumer 是否可见、是否卡顿、尺寸/帧率等

最小结构：

```json
{
  "request": true,
  "id": 21,
  "method": "downlinkClientStats",
  "data": {
    "schema": "mediasoup.qos.downlink.client.v1",
    "seq": 1,
    "tsMs": 1780391000000,
    "subscriberPeerId": "peer-a",
    "subscriptions": []
  }
}
```

常见用途：

- 驱动服务端下行 QoS 逻辑
- 生成统计卡片
- 辅助观察 consumer 当前状态

### `getStats`

含义：

- 客户端主动请求服务端当前看到的 peer 统计

最小请求：

```json
{
  "request": true,
  "id": 22,
  "method": "getStats",
  "data": {
    "peerId": "peer-a"
  }
}
```

说明：

- 这条是拉取，不是周期上报
- 主要用于调试和人工观测

## 7. QoS 配置类请求

### `setQosPolicy`

含义：

- 主动给某个 peer 设置 QoS 策略

### `setQosOverride`

含义：

- 主动给某个 peer 设置 QoS 覆盖

这两条更偏调试 / 控制，不属于最小接入必需。

## 8. Plain 路径

### `plainPublish`

含义：

- 通过 PlainTransport 发布一路或多路流

### `plainSubscribe`

含义：

- 通过 PlainTransport 订阅已有流

这两条属于 plain 接入，不属于浏览器最小订阅链主路径。

## 9. 客户端最小必须发送的请求

如果你只做最小“进房后看别人流”，端上至少要发：

1. `join`
2. `createWebRtcTransport(consuming=true, rtpCapabilities=...)`
3. `connectWebRtcTransport`
4. 对 `existingProducers` 发 `consume`

如果你还要发流，再加：

5. `createWebRtcTransport(producing=true)`
6. `connectWebRtcTransport`
7. `produce`
8. 停止某路本地流时发 `closeProducer`

## 10. 推荐顺序

最小推荐顺序如下：

1. `join`
2. `device.load(routerRtpCapabilities)`
3. `createWebRtcTransport(consuming=true, rtpCapabilities=device.rtpCapabilities)`
4. `connectWebRtcTransport`
5. 处理 `consumers`
6. 处理 `existingProducers`
7. 后续处理 `newConsumer`
8. 如果要发流，再创建发送 transport 并 `produce`
9. 如果要停止某路本地流，先 `closeProducer`，再关闭本地 producer/track

## 11. 当前代码位置

- [src/SignalingRequestDispatcher.h](../src/SignalingRequestDispatcher.h)
- [src/SignalingServerWs.cpp](../src/SignalingServerWs.cpp)
- [src/RoomServiceMedia.cpp](../src/RoomServiceMedia.cpp)
- [src/RoomServiceLifecycle.cpp](../src/RoomServiceLifecycle.cpp)

端上参考实现：

- [public/qos-demo.js](../public/qos-demo.js)
