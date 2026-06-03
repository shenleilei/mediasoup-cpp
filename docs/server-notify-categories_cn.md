# 服务端通知分类

这份文档只总结当前服务端**主动 notify 给客户端**的消息。

范围只包含：

- WebSocket `notification=true`
- 服务端主动推送

不包含：

- 客户端主动请求的 `response`
- WebRTC 媒体包本身

## 1. 总览

当前服务端会主动推送的通知主要分 5 类：

1. 房间与成员
2. 订阅与媒体
3. QoS 策略与覆盖
4. 统计与观测
5. 异常恢复

## 2. 房间与成员通知

### `peerJoined`

含义：

- 房间里有新 peer 加入

典型数据：

```json
{
  "notification": true,
  "method": "peerJoined",
  "data": {
    "peerId": "peer-b",
    "displayName": "peer-b",
    "reconnect": false
  }
}
```

客户端应该做什么：

- 更新房间成员列表
- 更新 UI 上的在线成员

说明：

- 这是成员变化通知
- 不是“新流可播放”的通知

### `peerLeft`

含义：

- 房间里某个 peer 离开

典型数据：

```json
{
  "notification": true,
  "method": "peerLeft",
  "data": {
    "peerId": "peer-b"
  }
}
```

客户端应该做什么：

- 从成员列表移除这个 peer
- 清理它对应的远端渲染卡片

## 3. 订阅与媒体通知

### `newConsumer`

含义：

- 服务端已经为当前客户端创建好了一个新的 consumer
- 当前客户端可以直接接收并渲染这条远端流

这条通知的本质是：

- 服务端已经完成了 auto-subscribe
- 当前客户端不需要再主动发 `consume`
- 只需要把这条 consumer 接起来并渲染

典型触发时机：

- 当前 peer 已经在房间里
- 当前 peer 已经有可用的 `recvTransport`
- 房间里其他 peer 后续又新发了一条流
- 服务端成功为当前 peer 创建了新的 consumer

它不是在所有场景下都会出现。

最容易误解的地方是：

- **后加入者**不一定靠它拿到加入前已有的流
- **producer 自己**也不会收到自己那条流对应的 `newConsumer`

典型数据：

```json
{
  "notification": true,
  "method": "newConsumer",
  "data": {
    "peerId": "peer-b",
    "producerId": "producer-1",
    "id": "consumer-1",
    "kind": "video",
    "rtpParameters": {}
  }
}
```

关键字段说明：

- `peerId`
  - 这条远端流属于哪个远端 peer
- `producerId`
  - 对应远端 producer
- `id`
  - 当前客户端本地要创建的 consumer id
- `kind`
  - `audio` 或 `video`
- `rtpParameters`
  - 当前客户端真正接这条流时要使用的协商结果

其中最关键的是：

- `producerId`
- `id`
- `kind`
- `rtpParameters`

因为客户端真正 `recvTransport.consume(...)` 时就是用这些字段。

客户端应该做什么：

1. 调 `recvTransport.consume(...)`
2. 拿到 `consumer.track`
3. 绑定到 `MediaStream`
4. 渲染到 `video/audio`

最小伪代码：

```js
if (msg.notification === true && msg.method === 'newConsumer') {
  const c = msg.data;

  const consumer = await recvTransport.consume({
    id: c.id,
    producerId: c.producerId,
    kind: c.kind,
    rtpParameters: c.rtpParameters
  });

  const stream = new MediaStream([consumer.track]);
  renderRemote(stream, c.peerId, c.kind);
}
```

如果是视频，通常还会顺手做两件事：

- 绑定 `track.onunmute`
- 请求一次关键帧

也就是：

- 先让 `consumer.track` 接到 UI
- 再通过关键帧尽快出图

### 什么时候不该等 `newConsumer`

这条是排障时最重要的边界。

不要在下面两种场景里死盯 `newConsumer`：

1. 后加入者刚进房
- 如果房间里已经有现成流
- 它主要应该处理的是 `existingProducers`

2. producer 自己
- 一个 peer 自己发出的流
- 不应该再期待自己收到这条流对应的 `newConsumer`

说明：

- 这是“后续新流”的通知
- 更适用于**已经在房间里的订阅端**
- 如果一个 peer 是**后加入者**，它对加入前已有的流，不一定靠 `newConsumer`

### `newConsumer` 失败时意味着什么

如果你排查“为什么没有收到 `newConsumer`”，优先考虑这些前置条件：

- 当前 peer 是否已经在房间里
- 当前 peer 是否已经有 `recvTransport`
- 当前 peer 是否具备可用的消费能力
- 这条流是不是在它加入之前就已经存在

所以“没有 `newConsumer`”本身不是结论，只是一个现象。

### `producerLeft`

含义：

- 某个远端 producer 已经被发布端关闭
- 服务端已经关闭了当前订阅端上由该 producer 派生出的 consumers
- 当前客户端应该移除对应远端媒体

典型数据：

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

客户端应该做什么：

- 优先按 `consumerIds` 移除本地 consumer 和对应 audio/video DOM。
- 再按 `producerId` 做兜底清理，避免 UI 残留。
- 不要把它当成 `peerLeft`；发布端 peer 可能还在线，只是关闭了一路 producer。

### `consumerClosed`

含义：

- 服务端关闭了当前客户端上的某条 consumer
- producer 不一定关闭，发布端 peer 也不一定离开

典型数据：

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

客户端应该做什么：

- 按 `consumerId` 移除本地 consumer 和对应 audio/video DOM。
- 如果业务维护了 producerId 到 consumerId 的索引，同步删除该索引。
- 不要自动重试 consume，除非业务上确认仍有授权和需要。

## 4. join 响应里的已有流

虽然它不是 notify，但端上必须和 notify 区分开。

### `join-ok.data.existingProducers`

含义：

- 这个 peer 加入房间时，房间里已经存在的流列表

典型数据：

```json
{
  "existingProducers": [
    {
      "producerId": "producer-audio-1",
      "producerPeerId": "peer-b",
      "kind": "audio"
    },
    {
      "producerId": "producer-video-1",
      "producerPeerId": "peer-b",
      "kind": "video"
    }
  ]
}
```

关键字段说明：

- `producerId`
  - 这条已有流的远端 producer id
- `producerPeerId`
  - 这条流属于哪个远端 peer
- `kind`
  - `audio` 或 `video`

这份列表不是“最终消费参数”，而是“当前房间里已有流的候选列表”。

也就是说：

- 它告诉客户端“现在有哪些流存在”
- 但不会直接给你最终 `consumer.rtpParameters`
- 真正消费时仍然要走 `consume`

客户端应该做什么：

1. 建好 `recvTransport`
2. 遍历 `existingProducers`
3. 对每个 producer 主动发 `consume`
4. 再 `recvTransport.consume(...)`

最小伪代码：

```js
for (const p of joinResp.data.existingProducers || []) {
  const resp = await wsRequest('consume', {
    transportId: recvTransportId,
    producerId: p.producerId,
    rtpCapabilities: device.rtpCapabilities
  });

  const consumer = await recvTransport.consume({
    id: resp.data.id,
    producerId: resp.data.producerId,
    kind: resp.data.kind,
    rtpParameters: resp.data.rtpParameters
  });

  const stream = new MediaStream([consumer.track]);
  renderRemote(stream, p.producerPeerId, p.kind);
}
```

### 为什么这里不是直接 `newConsumer`

因为这里对应的是“你加入房间时，房间里已经存在的流”。

时序上是：

1. 远端 peer 先发流
2. 你后加入
3. 服务端在 `join-ok` 里把当前房间的已有流列表返回给你

所以这条路径本来就不是：

- “先在房间里，再等后续通知”

而是：

- “加入时一次性同步已有流，再由客户端补 consume”

### 为什么不能只靠 `existingProducers`

因为它不包含完整消费结果。

当前字段只够做这些事：

- 展示房间里有哪些已有流
- 知道要对哪个 `producerId` 发 `consume`

它不直接包含：

- `consumerId`
- 最终消费态的 `rtpParameters`

所以如果端上只拿 `existingProducers` 做展示，不继续 `consume`，那就不会真正接到流。

### 和 `createWebRtcTransport(consuming=true)` 返回 `consumers` 的关系

当前服务端还支持另一种更顺手的方式：

- 在创建接收 transport 时
- 直接把当前已有流转成 `consumers` 放进响应

这比 `existingProducers` 更接近“可直接播放”的结果。

但即使如此，端上仍然要理解：

- `existingProducers` 是已有流列表
- `newConsumer` 是后续新流通知

不要把两者混成同一种事件。

说明：

- 这是“后加入者”最关键的已有流入口
- 不能只等 `newConsumer`

## 5. QoS 策略与覆盖通知

### `qosPolicy`

含义：

- 服务端下发或更新 QoS 策略

典型数据：

```json
{
  "notification": true,
  "method": "qosPolicy",
  "data": {
    "schema": "mediasoup.qos.policy.v1",
    "allowAudioOnly": true,
    "allowVideoPause": true
  }
}
```

客户端应该做什么：

- 更新本地 QoS 策略状态
- 如果启用了 QoS 控制器，则按策略调整上行和订阅行为

### `qosOverride`

含义：

- 服务端对某个 peer 下发临时 QoS 覆盖

客户端应该做什么：

- 覆盖当前本地 QoS 行为
- 直到 TTL 过期或收到清理覆盖

### `qosConnectionQuality`

含义：

- 服务端根据 QoS 快照判断出的连接质量

客户端应该做什么：

- 更新质量展示
- 可选用于端上弱网提示

### `qosRoomState`

含义：

- 服务端汇总后的房间级 QoS 状态

客户端应该做什么：

- 展示房间当前整体状态
- 一般不是基础收流必需

## 6. 统计与观测通知

### `statsReport`

含义：

- 服务端按房间周期广播的统计快照

它的定位不是控制信令，而是**观测面数据**。

也就是说：

- 它用来给页面展示统计、QoS 卡片、调试信息
- 不应该作为“是否能接收媒体”的唯一判断依据

典型数据结构大致会包含：

- `roomId`
- `peerCount`
- `peers`

其中 `peers` 里的每一项通常会带：

- `peerId`
- `sendTransport`
- `recvTransport`
- `producers`
- `clientStats`
- `downlinkClientStats`

不同 peer 上有哪些字段，取决于这个 peer 当前是否：

- 建了 send transport
- 建了 recv transport
- 发了 producer
- 上报了 `clientStats`
- 上报了 `downlinkClientStats`

### 典型用途

`statsReport` 最常见的用途有 4 类：

1. 渲染房间整体统计
- 房间里有多少个 peer
- 每个 peer 是否有上行
- 每个 peer 是否有下行

2. 渲染 QoS / 观测卡片
- producer bitrate
- producer score
- consumer 相关下行状态
- transport RTT / bitrate

3. 辅助排障
- 判断某个 peer 是否已经上报 `clientStats`
- 判断某个 peer 是否已经上报 `downlinkClientStats`
- 判断某个 producer 是否仍然存在

4. 辅助对账
- 和浏览器本地 stats 对照
- 和 `newConsumer` / `existingProducers` / `consume` 这条链对照

### 不该怎么用

不要把 `statsReport` 当成这几类事情的唯一依据：

- 不要只看 `statsReport` 就判断“流一定已经渲染成功”
- 不要只看 `statsReport` 就判断“端上一定已经收到了 `newConsumer`”
- 不要用它替代 `join-ok.existingProducers`
- 不要用它替代 `newConsumer`

原因很简单：

- `statsReport` 是周期广播
- 它有天然延迟
- 它是观测视图，不是控制面真相源

真正的控制面真相源仍然是：

- `join-ok`
- `existingProducers`
- `newConsumer`
- `consume` 响应

### 客户端应该怎么处理

客户端对 `statsReport` 的推荐处理方式是：

1. 收到后更新本地观测状态
2. 用它刷新 UI 卡片
3. 用它做调试输出
4. 不要拿它直接驱动基础订阅流程

也就是说：

- `statsReport` 适合“展示”
- 不适合“控制”

### 和 `clientStats` / `downlinkClientStats` 的关系

`statsReport` 往往会把服务端已知的两类客户端上报结果一起带出来：

- `clientStats`
- `downlinkClientStats`

可以这样理解：

- `clientStats`
  - 上行视角
  - 由发流端自己周期上报

- `downlinkClientStats`
  - 下行视角
  - 由订阅端自己周期上报

- `statsReport`
  - 房间级汇总视角
  - 由服务端广播给房间里的客户端

所以 `statsReport` 是一种“聚合快照”，不是原始上报本身。

### 和 `newConsumer` 的关系

很多人容易把这两者混淆。

要明确：

- `newConsumer`
  - 解决“你现在多了一条新流，去接它”
  - 是控制面通知

- `statsReport`
  - 解决“服务端当前看到的房间状态是什么”
  - 是观测面通知

如果你在排查“为什么端上没看到流”，顺序应该是：

1. 先看 `join-ok` / `existingProducers`
2. 再看 `newConsumer`
3. 再看 `consume` 响应
4. 最后再看 `statsReport`

不要反过来。

客户端应该做什么：

- 作为观测面板或调试页面数据源

说明：

- 它不是媒体接收所必需
- 但对观测和调试很重要

## 7. 异常恢复通知

### `serverRestart`

含义：

- 服务端检测到房间关联 worker 死亡或房间需要重连恢复

典型数据：

```json
{
  "notification": true,
  "method": "serverRestart",
  "data": {
    "roomId": "ZL15812",
    "reason": "worker crashed"
  }
}
```

客户端应该做什么：

- 触发重连或重进房恢复流程

## 8. 端上必须处理的最小集合

如果只做最小可用接入，客户端至少要处理：

1. `join` 响应里的 `routerRtpCapabilities`
2. `join` 响应里的 `existingProducers`
3. `newConsumer`
4. `producerLeft`
5. `peerLeft`

其中：

- `existingProducers` 解决“后加入者”
- `newConsumer` 解决“已在房间里的人收到后续新流”
- `producerLeft` 解决“peer 没离开，但某一路流被关闭”

## 9. 推荐分类

### 必须处理

- `existingProducers`
- `newConsumer`
- `producerLeft`
- `peerLeft`
- `serverRestart`

### 建议处理

- `peerJoined`
- `consumerClosed`
- `qosPolicy`
- `qosOverride`

### 可选处理

- `qosConnectionQuality`
- `qosRoomState`
- `statsReport`

## 10. 当前服务端代码位置

- [src/RoomServiceLifecycle.cpp](../src/RoomServiceLifecycle.cpp)
- [src/RoomMediaHelpers.h](../src/RoomMediaHelpers.h)
- [src/RoomServiceQos.cpp](../src/RoomServiceQos.cpp)
- [src/RoomServiceStats.cpp](../src/RoomServiceStats.cpp)
- [src/SignalingServerWs.cpp](../src/SignalingServerWs.cpp)
