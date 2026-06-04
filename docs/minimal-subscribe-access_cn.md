# 最小订阅接入

这份文档只说明当前协议下，端上如何以最小成本接入“进房后看别人流”。

目标只覆盖这条最短链：

1. `join`
2. `device.load(routerRtpCapabilities)`
3. `createWebRtcTransport(consuming=true, rtpCapabilities=...)`
4. 处理 `createWebRtcTransport` 响应里的 `consumers`
5. 兼容处理 `join` 响应里的 `existingProducers`
6. 处理后续 `newConsumer`
7. 处理远端 producer 关闭后的 `producerLeft` / `consumerClosed`

不包含：

- QoS
- 上行自适应
- downlink 控制
- 复杂 UI

如果本端同时也发布本地流，关闭 producer 的最小顺序见第 9 节：先通知服务端 `closeProducer`，再关闭本地 producer 和 track。不要只停本地 track。

## 1. join

先发 `join`：

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

服务端返回：

- `routerRtpCapabilities`
- `existingProducers`
- `participants`
- `qosPolicy`

最小响应示例：

```json
{
  "response": true,
  "id": 1,
  "ok": true,
  "data": {
    "routerRtpCapabilities": { "...": "..." },
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
    "participants": [],
    "qosPolicy": { }
  }
}
```

这里最关键的是：

- `routerRtpCapabilities`
- `existingProducers`

说明：

- `existingProducers` 表示这个 peer 加入房间时，房间里已经存在的流
- 后加入者主要靠它来消费已有流
- `existingProducers` 只是 producer 候选列表，不是可直接播放的 consumer
- 每项里的 `producerPeerId` 是发布端 peer
- 每项里的 `appData` 来自发布端 `produce.appData`
- 如果发布端带了 `produce.appData.source`，订阅端读取 `existingProducers[].appData.source`
- 协议不返回裸 `source` 字段

## 2. 创建本端 device

浏览器端用 `mediasoup-client`：

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

这就是本端真实可消费能力。

## 3. 创建接收 transport

然后创建接收 transport：

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

服务端返回：

- `id`
- `iceParameters`
- `iceCandidates`
- `dtlsParameters`
- `consumers`

如果房间里已经有可消费的流，`consumers` 里会直接带最终消费参数：

```json
{
  "response": true,
  "id": 2,
  "ok": true,
  "data": {
    "id": "recv-transport-1",
    "iceParameters": { },
    "iceCandidates": [],
    "dtlsParameters": { },
    "consumers": [
      {
        "peerId": "vehicle_ZL15812",
        "producerId": "producer-video-1",
        "id": "consumer-video-1",
        "kind": "video",
        "appData": {
          "source": "vehicle-left-door"
        },
        "rtpParameters": { }
      }
    ]
  }
}
```

这里的 `consumers` 表示：

- 在创建接收 transport 时，服务端已经把当前已有流直接转成了可消费结果
- `consumers[].id` 是当前订阅端的 consumerId
- `consumers[].peerId` 是 producer 所属 peer
- `consumers[].producerId` 是被消费的 producer
- `consumers[].appData.source` 是发布端传入的业务来源
- 服务端可能额外返回 `producerPaused`，最小接入可以忽略

随后完成 `connectWebRtcTransport`。

## 4. 处理已有流

这里有两种可兼容的来源：

1. `join` 响应里的 `existingProducers`
2. `createWebRtcTransport(consuming=true)` 响应里的 `consumers`

当前最稳妥的端上处理建议是：

- 优先消费 `createWebRtcTransport` 响应里的 `consumers`
- 同时保留对 `existingProducers` 的兼容处理

### 4.1 如果拿到的是 `consumers`

直接接收并渲染：

```js
for (const c of recvResp.data.consumers || []) {
  const consumer = await recvTransport.consume({
    id: c.id,
    producerId: c.producerId,
    kind: c.kind,
    rtpParameters: c.rtpParameters
  });

  const stream = new MediaStream([consumer.track]);
  renderRemote(stream, c.peerId, c.kind, {
    producerId: c.producerId,
    source: c.appData?.source || ''
  });
}
```

### 4.2 如果拿到的是 `existingProducers`

需要再主动发一次 `consume`：

```js
for (const p of joinResp.data.existingProducers || []) {
  const resp = await wsRequest('consume', {
    transportId: recvResp.data.id,
    producerId: p.producerId,
    rtpCapabilities: device.rtpCapabilities
  });

  const c = resp.data;
  const consumer = await recvTransport.consume({
    id: c.id,
    producerId: c.producerId,
    kind: c.kind,
    rtpParameters: c.rtpParameters
  });

  const stream = new MediaStream([consumer.track]);
  renderRemote(stream, c.peerId, c.kind, {
    producerId: c.producerId,
    source: c.appData?.source || ''
  });
}
```

## 5. 处理后续新流

如果已经在房间里，后面别人再发新流，服务端会推：

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
    "rtpParameters": { }
  }
}
```

字段口径：

- `data.id` 是当前订阅端拿到的 consumerId
- `data.peerId` 是 producer 所属 peer，不是当前订阅端 peer
- `data.producerId` 是被自动订阅的 producer
- `data.appData` 来自发布端 `produce.appData`
- 业务来源从 `data.appData.source` 读取，不读取裸 `data.source`
- 服务端可能额外返回 `producerPaused`，最小接入可以忽略

端上收到后直接：

```js
const consumer = await recvTransport.consume({
  id: data.id,
  producerId: data.producerId,
  kind: data.kind,
  rtpParameters: data.rtpParameters
});

const stream = new MediaStream([consumer.track]);
renderRemote(stream, data.peerId, data.kind, {
  producerId: data.producerId,
  source: data.appData?.source || ''
});
```

## 6. 处理远端流关闭

订阅端必须保存 consumer 映射，否则收到关闭通知后无法准确清理页面。

建议最少保存：

- `consumerId -> { consumer, producerId, element }`
- `producerId -> Set<consumerId>`

这里的 `consumer` 是本地 `mediasoup-client Consumer`，`element` 是对应 audio/video DOM。

### 6.1 producer 被关闭：收到 producerLeft

触发条件：发布端调用 `closeProducer`，服务端关闭这路 producer，并关闭由它派生出的 consumers。服务端会给受影响的订阅端推 `producerLeft`：

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

订阅端最小处理：

- 按 `consumerIds` 找到本地 `mediasoup-client Consumer`。
- 对每个本地 consumer 调用 `consumer.close()`。
- 清理对应 audio/video DOM：`element.pause()`、`element.srcObject = null`、移除 DOM。
- 清理音量检测、QoS hint、consumer 状态缓存。
- 从 `consumerId -> consumer` 和 `producerId -> consumerIds` 映射里删除。
- 如果 `consumerIds` 为空或没匹配到，也要按 `producerId` 做兜底清理。
- 不要再对这个 `producerId` 发 `consume`；服务端已关闭后会返回 `producer not found`。
- 不要把它当成 `peerLeft`；发布端 peer 可能还在线，只是这一路 producer 没了。

### 6.2 只关闭某条 consumer：收到 consumerClosed

触发条件：服务端只关闭某条 consumer，不关闭 producer。例如音频受限端 release slot 时，发送端 producer 仍然存在，但服务端不再让某个受限端消费这路音频。订阅端会收到 `consumerClosed`：

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

订阅端最小处理：

- 按 `consumerId` 找到本地 `mediasoup-client Consumer`。
- 调用 `consumer.close()`。
- 清理对应 audio/video DOM：`element.pause()`、`element.srcObject = null`、移除 DOM。
- 清理音量检测、QoS hint、consumer 状态缓存。
- 从本地映射删除该 consumer。
- 不需要调用 `consumer.pause()` / `consumer.resume()`。
- 不需要给服务端发 ack。
- 不要自动重试 `consume`，除非业务重新获得服务端授权或重新触发订阅。
- 不要把它当成 peer 离开；producer 所属 peer 仍可能在线。

### 6.3 两个通知的区别

| 通知 | 服务端关闭了什么 | producer 是否还存在 | 订阅端动作 |
| --- | --- | --- | --- |
| `producerLeft` | producer 以及它派生的 consumers | 不存在 | 按 `consumerIds` 清理，按 `producerId` 兜底 |
| `consumerClosed` | 当前订阅端上的某条 consumer | 仍可能存在 | 按 `consumerId` 清理，不要影响同 producer 的其他订阅关系 |

## 7. 两条路径的边界

要分清：

- `existingProducers`
  - 用于“后加入者”
  - 处理加入前已经存在的流

- `newConsumer`
  - 用于“已在房间里的订阅端”
  - 处理后续新出现的流

不要把两条路径混成一个判断。

如果一个 peer 是后加入者，它不一定会为加入前已有的流收到 `newConsumer`。

## 8. 最小前端伪代码

```js
const remoteConsumers = new Map(); // consumerId -> { consumer, producerId, element }
const remoteConsumersByProducerId = new Map(); // producerId -> Set<consumerId>

async function consumeAndRender(c) {
  const consumer = await recvTransport.consume({
    id: c.id,
    producerId: c.producerId,
    kind: c.kind,
    rtpParameters: c.rtpParameters
  });

  const stream = new MediaStream([consumer.track]);
  const element = renderRemote(stream, c.peerId, c.kind, {
    consumerId: consumer.id,
    producerId: c.producerId,
    source: c.appData?.source || ''
  });

  remoteConsumers.set(consumer.id, {
    consumer,
    producerId: c.producerId,
    element
  });

  if (c.producerId) {
    const ids = remoteConsumersByProducerId.get(c.producerId) || new Set();
    ids.add(consumer.id);
    remoteConsumersByProducerId.set(c.producerId, ids);
  }

  consumer.on('producerclose', () => removeRemoteConsumer(consumer.id));
  consumer.on('transportclose', () => removeRemoteConsumer(consumer.id));
}

function removeRemoteConsumer(consumerId) {
  const entry = remoteConsumers.get(consumerId);
  if (!entry) return;

  remoteConsumers.delete(consumerId);
  if (entry.producerId) {
    const ids = remoteConsumersByProducerId.get(entry.producerId);
    ids?.delete(consumerId);
    if (!ids || ids.size === 0) {
      remoteConsumersByProducerId.delete(entry.producerId);
    }
  }

  entry.consumer.close();

  if (entry.element) {
    entry.element.pause?.();
    entry.element.srcObject = null;
    entry.element.remove?.();
  }
}

function removeRemoteConsumersByProducerId(producerId) {
  const ids = Array.from(remoteConsumersByProducerId.get(producerId) || []);
  for (const consumerId of ids) {
    removeRemoteConsumer(consumerId);
  }
}

const joinResp = await wsRequest('join', {
  roomId,
  peerId,
  displayName
});

const device = new mediasoupClient.Device();
await device.load({
  routerRtpCapabilities: joinResp.data.routerRtpCapabilities
});

const recvResp = await wsRequest('createWebRtcTransport', {
  producing: false,
  consuming: true,
  rtpCapabilities: device.rtpCapabilities
});

const recvTransport = device.createRecvTransport(recvResp.data);

recvTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
  try {
    await wsRequest('connectWebRtcTransport', {
      transportId: recvResp.data.id,
      dtlsParameters
    });
    callback();
  } catch (error) {
    errback(error);
  }
});

for (const c of recvResp.data.consumers || []) {
  await consumeAndRender(c);
}

for (const p of joinResp.data.existingProducers || []) {
  const resp = await wsRequest('consume', {
    transportId: recvResp.data.id,
    producerId: p.producerId,
    rtpCapabilities: device.rtpCapabilities
  });
  await consumeAndRender(resp.data);
}

ws.onmessage = async (event) => {
  const msg = JSON.parse(event.data);
  if (msg.notification === true && msg.method === 'newConsumer') {
    await consumeAndRender(msg.data);
  } else if (msg.notification === true && msg.method === 'producerLeft') {
    for (const consumerId of msg.data.consumerIds || []) {
      removeRemoteConsumer(consumerId);
    }
    removeRemoteConsumersByProducerId(msg.data.producerId);
  } else if (msg.notification === true && msg.method === 'consumerClosed') {
    removeRemoteConsumer(msg.data.consumerId);
  }
};
```

## 9. 如果本端也发流：关闭 producer

如果最小接入里还包含发布本地流，停止某一路 producer 时不要只停本地 track，也要通知服务端。否则服务端仍认为 producer 存在，其他订阅端不会收到关闭通知，页面上可能残留远端媒体。

### 9.1 发送 closeProducer

按 producerId 关闭：

```json
{
  "request": true,
  "id": 20,
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
  "id": 21,
  "method": "closeProducer",
  "data": {
    "source": "audio"
  }
}
```

成功响应示例：

```json
{
  "response": true,
  "id": 20,
  "ok": true,
  "data": {
    "producerId": "producer-audio-1",
    "closedConsumers": 2,
    "notifiedPeers": ["peer-b", "peer-c"]
  }
}
```

### 9.2 本端关闭顺序

最小顺序是：

```text
closeProducer(producerId 或 source)
本地 producer.close()
停止对应 MediaStreamTrack
从本地 publishedProducers 删除
```

示例：

```js
async function closeLocalProducer(producer) {
  if (!producer) return;

  await wsRequest('closeProducer', {
    producerId: producer.id
  });

  producer.close();
  producer.track?.stop?.();
  publishedProducers.delete(producer.id);
}
```

### 9.3 远端会收到什么

服务端关闭 producer 后，会给受影响的订阅端下发 `producerLeft`。订阅端按第 6 节处理：找到本地 consumer，调用 `consumer.close()`，再清理 audio/video DOM 和本地缓存。

注意：

- `source` 只适合同一个 peer 下唯一 producer 匹配；如果多路 producer 使用同一个 source，服务端会返回 `ambiguous producer source`。
- `closeProducer` 成功后，订阅端通过 `producerLeft` 清理远端媒体。
- `closeProducer` 不等于 `releaseAudioRestrictedSlot`；音频受限端业务关闭需要两者配合。
- 如果是音频受限端当前 demo 的关闭 Audio，普通端需要先 `closeProducer`，再 `releaseAudioRestrictedSlot`。

## 10. 当前服务端代码位置

- [src/RoomServiceLifecycle.cpp](../src/RoomServiceLifecycle.cpp)
- [src/RoomServiceMedia.cpp](../src/RoomServiceMedia.cpp)
- [src/RoomMediaHelpers.h](../src/RoomMediaHelpers.h)

端上参考实现：

- [public/qos-demo.js](../public/qos-demo.js)
