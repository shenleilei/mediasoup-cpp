# 上下行接口总文档

这份文档把当前端上接 mediasoup-cpp 所需的**上行 / 下行接口**统一整理到一处。

目标是回答 6 个问题：

1. 客户端要主动发什么
2. 服务端会回什么
3. 服务端会主动推什么
4. 每个字段从哪里来
5. 正确顺序是什么
6. 后加入者和已在房间里的订阅端分别怎么处理

范围只覆盖浏览器 / WebSocket / WebRTC 主路径，不展开 plain transport 和复杂 QoS 控制细节。

## 1. 总览

整个链路可以拆成 4 层：

1. 房间层
2. 能力层
3. 传输层
4. 媒体层

### 房间层

- `join`
- `peerJoined`
- `peerLeft`

### 能力层

- `routerRtpCapabilities`
- `device.rtpCapabilities`

### 传输层

- `createWebRtcTransport`
- `connectWebRtcTransport`

### 媒体层

- `produce`
- `closeProducer`
- `consume`
- `existingProducers`
- `newConsumer`
- `producerLeft`
- `consumerClosed`

## 2. 两条主路径

当前端上要区分两条完全不同的路径：

### 路径 A：后加入者拿已有流

特点：

- 你加入房间时，别人已经在发流

关键入口：

- `join-ok.data.existingProducers`
- 或 `createWebRtcTransport(consuming=true)` 响应里的 `consumers`

### 路径 B：已在房间里的人收到后续新流

特点：

- 你已经在房间里
- 别人后面又新发了一条流

关键入口：

- `newConsumer`

这个区分非常重要。  
很多现场误判，本质上都是把路径 A 和路径 B 混成一条了。

## 3. 完整顺序

最小推荐顺序如下：

1. `join`
2. 从 `join` 响应拿 `routerRtpCapabilities`
3. `device.load(routerRtpCapabilities)`
4. 得到 `device.rtpCapabilities`
5. `createWebRtcTransport(consuming=true, rtpCapabilities=device.rtpCapabilities)`
6. `connectWebRtcTransport`
7. 处理 `consumers`
8. 兼容处理 `existingProducers`
9. 后续处理 `newConsumer`
10. 如果要发流，再创建发送 transport
11. `connectWebRtcTransport`
12. `produce`
13. 如果要停止某路本地流，发 `closeProducer`
14. 订阅端持续处理 `producerLeft` / `consumerClosed`

如果顺序没对，后面即使日志里看起来“加入成功”，也不代表一定能看到流。

## 4. join

### 请求

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

可选：

```json
{
  "rtpCapabilities": { }
}
```

### 字段来源

- `roomId`
  - 业务房间号
- `peerId`
  - 当前客户端身份
- `displayName`
  - 展示名，没有特殊要求通常等于 `peerId`
- `rtpCapabilities`
  - 如果此时已经有本端真实能力，可以带
  - 否则可以先不带

### 响应

服务端返回：

```json
{
  "response": true,
  "id": 1,
  "ok": true,
  "data": {
    "routerRtpCapabilities": {
      "codecs": [
        {
          "kind": "audio",
          "mimeType": "audio/opus",
          "clockRate": 48000,
          "channels": 2
        },
        {
          "kind": "video",
          "mimeType": "video/H264",
          "clockRate": 90000
        }
      ],
      "headerExtensions": [
        { "kind": "video", "uri": "urn:ietf:params:rtp-hdrext:sdes:mid" }
      ]
    },
    "existingProducers": [
      {
        "producerId": "producer-video-1",
        "producerPeerId": "vehicle_ZL15812",
        "kind": "video",
        "appData": {
          "source": "vehicle-left-door"
        }
      }
    ],
    "participants": [
      {
        "peerId": "vehicle_ZL15812",
        "displayName": "vehicle_ZL15812",
        "producers": [
          {
            "producerId": "producer-video-1",
            "kind": "video"
          }
        ]
      }
    ],
    "qosPolicy": {
      "schema": "mediasoup.qos.policy.v1"
    }
  }
}
```

关键字段：

- `routerRtpCapabilities`
- `existingProducers`
- `participants`
- `qosPolicy`

### 含义

`join` 的职责是：

- 进房
- 拿到房间级基础信息

`join` 不负责：

- 建 transport
- 自动把远端媒体接通
- 返回最终消费参数

### 最重要的两个字段

#### `routerRtpCapabilities`

作用：

- 给端上 `device.load(...)`
- 告诉端上当前房间支持什么 codec / extension

#### `existingProducers`

作用：

- 告诉端上“你加入时，房间里已经有哪些流”

说明：

- 这是已有流候选列表
- 不是最终可播放 consumer
- 真正播放还要再经过 `consume`
- 每一项会带 `producerId`、`producerPeerId`、`kind`、`appData`
- `appData` 原样来自发布端 `produce.appData`
- 如果发布端声明了业务来源，端上从 `appData.source` 读取
- `existingProducers` 只是候选 producer 列表，没有 `consumerId`，也没有 `rtpParameters`

## 5. device.load

浏览器端典型写法：

```js
const device = new mediasoupClient.Device();
await device.load({
  routerRtpCapabilities: joinResp.data.routerRtpCapabilities
});
```

完成后得到：

```js
device.rtpCapabilities
```

这份能力是后续最关键的输入之一。

### 它的来源是什么

- 来自本端运行环境真实支持的接收能力
- 不是服务端直接替你生成

### 它会用在哪里

主要用在：

- `createWebRtcTransport(consuming=true, rtpCapabilities=...)`
- `consume(producerId, rtpCapabilities=...)`

## 6. createWebRtcTransport

### 创建接收 transport

请求：

```json
{
  "request": true,
  "id": 2,
  "method": "createWebRtcTransport",
  "data": {
    "producing": false,
    "consuming": true,
    "rtpCapabilities": { "...device.rtpCapabilities..." }
  }
}
```

### 为什么这里必须带 `rtpCapabilities`

因为这一步是当前协议下最重要的“消费能力声明点”。

如果 `join` 没带能力，这一步最好一定带。

这能解决：

- 服务端后续不知道你能消费什么
- auto-subscribe / `consume` 因能力缺失而失败

### 响应

关键字段：

- `id`
- `iceParameters`
- `iceCandidates`
- `dtlsParameters`
- `consumers`

### `consumers` 是什么

这是一个非常容易被忽略的字段。

如果房间里此时已经有流，服务端会在创建接收 transport 时，直接把这些已有流转成 `consumers` 返回。

也就是说：

- 后加入者不一定非要等 `newConsumer`
- 很多已有流会直接在这一步返回

`consumers[]` 每一项是已经协商好的消费参数，结构和 `newConsumer.data` 保持一致：

```json
{
  "peerId": "vehicle_ZL15812",
  "producerId": "producer-1",
  "id": "consumer-1",
  "kind": "video",
  "appData": {
    "source": "vehicle-left-door"
  },
  "rtpParameters": { },
  "producerPaused": false
}
```

这里的 `appData` 来自该 producer 的 `produce.appData`，方便端上通过 `appData.source` 把 consumer 对应回业务来源。

字段说明：

- `peerId`
  - producer 所属 peer
- `producerId`
  - 被消费的 producer
- `id`
  - 当前订阅端拿到的 consumerId
- `kind`
  - `audio` 或 `video`
- `appData`
  - 发布端 `produce.appData` 的服务端保存值
- `rtpParameters`
  - 已经协商好的消费参数，端上直接传给 `recvTransport.consume(...)`
- `producerPaused`
  - 可选状态字段，表示 producer 当前是否暂停
  - 最小接入不依赖这个字段，端上调用 `recvTransport.consume(...)` 时不需要传它

### 创建发送 transport

请求：

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

## 7. connectWebRtcTransport

请求：

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

### 含义

- 把本地 transport 的 DTLS 参数发给服务端

### 它不负责什么

不要把它和能力协商搞混：

- 它不负责上报消费能力
- 它不直接触发 `newConsumer`
- 它不等于“已经能看到远端流”

## 8. produce

请求：

```json
{
  "request": true,
  "id": 5,
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

### 关键字段

- `transportId`
- `kind`
- `rtpParameters`
- `appData.source`

### 字段来源

- `transportId`
  - 来自发送 transport
- `kind`
  - `audio` 或 `video`
- `rtpParameters`
  - 来自本地 track 在 mediasoup-client 上的发送参数
- `appData.source`
  - 业务自定义的 producer 来源标识
  - 服务端只校验它是字符串，不把它限定成 `audio` / `camera` / `screenShare`
  - 媒体类型由 `kind` 表达，`source` 用于表达业务侧的设备、通道、摄像头位、车辆位置或其它来源

### `source` 和 `producerId` 的关系

`producerId` 是服务端为本次 `produce` 创建的运行时对象 ID。
`appData.source` 是端上随 `produce.appData.source` 带上来的业务标签。

正确理解是：

- `producerId` 用来消费和控制这条媒体流
- `appData.source` 用来让业务识别这条 producer 来自哪里
- 同一个业务 `source` 断线重推后，可能生成新的 `producerId`
- 同一个 peer 可以同时有多个 producer，每条 producer 可以有不同 `source`
- 不要把 `source` 当成唯一流 ID，也不要把它当成固定媒体枚举

服务端会在所有 producer / consumer 相关下行数据里，把当前 `producerId` 对应的 `appData` 一起返回：

- `join-ok.data.existingProducers[]`
- `createWebRtcTransport(consuming=true).data.consumers[]`
- `consume` 响应
- `newConsumer.data`

协议保持同一个字段名：**下行也返回 `appData.source`，不是裸 `source` 字段**。

### 服务端成功后会做什么

1. 创建新的 `Producer`
2. 返回 `producerId`
3. 对房间里其他可订阅 peer 做 auto-subscribe

### 一个常见误区

producer 自己不应该期待收到自己这条流的 `newConsumer`。  
`newConsumer` 是发给其他订阅端的。

### closeProducer

如果端上要停止自己发布的一路 producer，应该发：

```json
{
  "request": true,
  "id": 6,
  "method": "closeProducer",
  "data": {
    "producerId": "producer-audio-1"
  }
}
```

也可以按 `produce.appData.source` 关闭唯一匹配的 producer：

```json
{
  "request": true,
  "id": 7,
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
  "id": 7,
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

- 只能关闭本 peer 自己的 producer。
- 按 source 关闭时，source 必须在当前 peer 下唯一；多路同 source 会返回 `ambiguous producer source`。
- `closeProducer` 成功后，订阅端通过 `producerLeft` 清理远端媒体。

## 9. consume

请求：

```json
{
  "request": true,
  "id": 8,
  "method": "consume",
  "data": {
    "transportId": "recv-transport-1",
    "producerId": "producer-1",
    "rtpCapabilities": { "...device.rtpCapabilities..." }
  }
}
```

### 关键字段来源

- `transportId`
  - 来自接收 transport
- `producerId`
  - 来自：
    - `existingProducers`
    - 或你自己维护的候选流列表
- `rtpCapabilities`
  - 来自 `device.rtpCapabilities`

### 响应

服务端返回的 `data`：

```json
{
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
```

字段说明：

- `id`（consumerId）
- `producerId`
- `peerId`（producer 所属 peer）
- `kind`
- `appData`（来自 producer 的 `produce.appData`，例如 `appData.source`）
- `rtpParameters`
- `producerPaused`（可选状态字段，producer 当前是否暂停；最小接入可忽略）

### 这一步的真实职责

`consume` 才是真正的消费协商点：

- 服务端会用本端 `rtpCapabilities`
- 去和目标 producer 做 codec 匹配
- 匹配成功后返回最终消费参数

所以：

- `existingProducers` 只是候选列表
- `consume` 才是最终消费确认

## 10. recvTransport.consume

拿到 `consume` 或 `newConsumer` 给你的消费参数后，端上还要继续做：

```js
const consumer = await recvTransport.consume({
  id: data.id,
  producerId: data.producerId,
  kind: data.kind,
  rtpParameters: data.rtpParameters
});

const stream = new MediaStream([consumer.track]);
renderRemote(stream, data.peerId, data.kind);
```

说明：

- 服务端返回消费参数 ≠ 页面已经播放成功
- 端上还必须把 `consumer.track` 接到浏览器媒体层

## 11. 两条下行路径

### 路径 A：后加入者

如果你加入时房间里已经有流，主要处理：

- `joinResp.data.existingProducers`
- `recvResp.data.consumers`

这时不一定会额外收到 `newConsumer`。

### 路径 B：已在房间里的订阅端

如果你已经在房间里，别人后续又发新流，主要处理：

- `newConsumer`

不要把路径 A 和路径 B 混成一个判断。

## 12. newConsumer

服务端主动通知：

```json
{
  "notification": true,
  "method": "newConsumer",
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

字段说明：

- `peerId`
  - producer 所属 peer
- `producerId`
  - 被自动订阅的 producer
- `id`
  - 服务端为当前订阅端创建的 consumerId
- `kind`
  - `audio` 或 `video`
- `appData`
  - 发布端 `produce.appData` 的服务端保存值
- `rtpParameters`
  - 已经协商好的消费参数
- `producerPaused`
  - 可选状态字段
  - 如果服务端返回该字段，表示 producer 当前暂停状态
  - 最小接入不依赖它，端上调用 `recvTransport.consume(...)` 时不需要传它

### 什么时候会有

- 当前 peer 已经在房间里
- 当前 peer 已经有可用的 `recvTransport`
- 房间里别人后续新发了一条流

### 什么时候不要等它

- 如果你是后加入者
- 且对端流在你加入前就已经存在

这时你更该处理：

- `existingProducers`
- `consumers`

### producerLeft / consumerClosed

如果远端 producer 被关闭，订阅端会收到：

```json
{
  "notification": true,
  "method": "producerLeft",
  "data": {
    "peerId": "peer-a",
    "producerId": "producer-audio-1",
    "consumerIds": ["consumer-audio-1"],
    "kind": "audio",
    "appData": {
      "source": "audio"
    }
  }
}
```

订阅端应该：

- 按 `consumerIds` 移除本地 consumer 和 DOM。
- 再按 `producerId` 做兜底清理。
- 不要把它当成 `peerLeft`；peer 可能仍在线。

如果服务端只关闭 consumer，不关闭 producer，例如音频受限端 release slot，会收到：

```json
{
  "notification": true,
  "method": "consumerClosed",
  "data": {
    "consumerId": "consumer-audio-1",
    "producerId": "producer-audio-1",
    "producerPeerId": "peer-a",
    "kind": "audio",
    "reason": "audio-slot-release"
  }
}
```

订阅端应该按 `consumerId` 移除对应远端媒体。

## 13. requestConsumerKeyFrame

请求：

```json
{
  "request": true,
  "id": 9,
  "method": "requestConsumerKeyFrame",
  "data": {
    "consumerId": "consumer-1"
  }
}
```

### 适用场景

- 新视频刚接上时尽快出图
- 订阅恢复后尽快补画面

它不是基础必需，但视频场景通常很有用。

## 14. QoS 与统计上报

### `clientStats`

含义：

- 发流端上报上行 QoS 快照

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

作用：

- 服务端计算连接质量
- 服务端生成 `qosOverride`
- 服务端聚合房间 QoS 状态

### `downlinkClientStats`

含义：

- 订阅端上报下行消费快照

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

### `subscriptions` 填什么

这里填的是：

- 当前这个订阅端已经拿到的每一条 consumer 的状态快照

每一项至少要能标识：

- `consumerId`
- `producerId`
- `kind`

如果当前没有任何 consumer，才应该是：

```json
"subscriptions": []
```

它不是：

- 房间成员列表
- producer 候选列表
- 业务 peer 列表

### `getStats`

含义：

- 主动拉取服务端当前看到的 peer 统计

它偏调试，不属于最小接入必需。

## 15. 最小必须实现

如果端上只做最小“进房后看别人流”，至少要实现：

1. `join`
2. `device.load(routerRtpCapabilities)`
3. `createWebRtcTransport(consuming=true, rtpCapabilities=device.rtpCapabilities)`
4. `connectWebRtcTransport`
5. 处理 `consumers`
6. 处理 `existingProducers`
7. 处理后续 `newConsumer`

如果端上还要发流，再实现：

8. `createWebRtcTransport(producing=true)`
9. `connectWebRtcTransport`
10. `produce`
11. `closeProducer`

## 16. 排障时怎么看

如果端上“没看到流”，按这个顺序查：

1. `join` 是否成功
2. `routerRtpCapabilities` 是否拿到
3. `device.load(...)` 是否成功
4. `createWebRtcTransport(consuming=true)` 是否带了 `rtpCapabilities`
5. `connectWebRtcTransport` 是否成功
6. `join-ok.existingProducers` 里有没有已有流
7. `recvResp.data.consumers` 里有没有已有消费者
8. 后续有没有 `newConsumer`
9. `consume` 是否成功
10. 浏览器本地 `recvTransport.consume(...)` 是否成功

不要直接跳到第 8 步就下结论。

## 17. 代码位置

服务端：

- [src/SignalingRequestDispatcher.h](../src/SignalingRequestDispatcher.h)
- [src/SignalingServerWs.cpp](../src/SignalingServerWs.cpp)
- [src/RoomServiceLifecycle.cpp](../src/RoomServiceLifecycle.cpp)
- [src/RoomServiceMedia.cpp](../src/RoomServiceMedia.cpp)
- [src/RoomMediaHelpers.h](../src/RoomMediaHelpers.h)

端上参考实现：

- [public/qos-demo.js](../public/qos-demo.js)
