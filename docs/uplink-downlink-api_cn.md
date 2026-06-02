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
- `consume`
- `existingProducers`
- `newConsumer`

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

服务端返回的关键字段有：

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
      "source": "camera"
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
  - 业务语义，常见值：
    - `audio`
    - `camera`
    - `screenShare`

### 服务端成功后会做什么

1. 创建新的 `Producer`
2. 返回 `producerId`
3. 对房间里其他可订阅 peer 做 auto-subscribe

### 一个常见误区

producer 自己不应该期待收到自己这条流的 `newConsumer`。  
`newConsumer` 是发给其他订阅端的。

## 9. consume

请求：

```json
{
  "request": true,
  "id": 6,
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

服务端返回：

- `id`（consumerId）
- `producerId`
- `kind`
- `rtpParameters`

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
    "peerId": "peer-b",
    "producerId": "producer-1",
    "id": "consumer-1",
    "kind": "video",
    "rtpParameters": { }
  }
}
```

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

## 13. requestConsumerKeyFrame

请求：

```json
{
  "request": true,
  "id": 7,
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
