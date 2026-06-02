# 最小订阅接入

这份文档只说明当前协议下，端上如何以最小成本接入“进房后看别人流”。

目标只覆盖这条最短链：

1. `join`
2. `device.load(routerRtpCapabilities)`
3. `createWebRtcTransport(consuming=true, rtpCapabilities=...)`
4. 处理 `existingProducers`
5. 处理后续 `newConsumer`

不包含：

- QoS
- 上行自适应
- downlink 控制
- 复杂 UI

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

这里最关键的是：

- `routerRtpCapabilities`
- `existingProducers`

说明：

- `existingProducers` 表示这个 peer 加入房间时，房间里已经存在的流
- 后加入者主要靠它来消费已有流

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

这里的 `consumers` 表示：

- 在创建接收 transport 时，服务端已经把当前已有流直接转成了可消费结果

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
  renderRemote(stream, c.peerId, c.kind);
}
```

### 4.2 如果拿到的是 `existingProducers`

需要再主动发一次 `consume`：

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

## 5. 处理后续新流

如果已经在房间里，后面别人再发新流，服务端会推：

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

端上收到后直接：

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

## 6. 两条路径的边界

要分清：

- `existingProducers`
  - 用于“后加入者”
  - 处理加入前已经存在的流

- `newConsumer`
  - 用于“已在房间里的订阅端”
  - 处理后续新出现的流

不要把两条路径混成一个判断。

如果一个 peer 是后加入者，它不一定会为加入前已有的流收到 `newConsumer`。

## 7. 最小前端伪代码

```js
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
  const consumer = await recvTransport.consume({
    id: c.id,
    producerId: c.producerId,
    kind: c.kind,
    rtpParameters: c.rtpParameters
  });
  renderRemote(new MediaStream([consumer.track]), c.peerId, c.kind);
}

for (const p of joinResp.data.existingProducers || []) {
  const resp = await wsRequest('consume', {
    transportId: recvResp.data.id,
    producerId: p.producerId,
    rtpCapabilities: device.rtpCapabilities
  });
  const consumer = await recvTransport.consume({
    id: resp.data.id,
    producerId: resp.data.producerId,
    kind: resp.data.kind,
    rtpParameters: resp.data.rtpParameters
  });
  renderRemote(new MediaStream([consumer.track]), p.producerPeerId, p.kind);
}

ws.onmessage = async (event) => {
  const msg = JSON.parse(event.data);
  if (msg.notification === true && msg.method === 'newConsumer') {
    const c = msg.data;
    const consumer = await recvTransport.consume({
      id: c.id,
      producerId: c.producerId,
      kind: c.kind,
      rtpParameters: c.rtpParameters
    });
    renderRemote(new MediaStream([consumer.track]), c.peerId, c.kind);
  }
};
```

## 8. 当前服务端代码位置

- [src/RoomServiceLifecycle.cpp](../src/RoomServiceLifecycle.cpp)
- [src/RoomServiceMedia.cpp](../src/RoomServiceMedia.cpp)
- [src/RoomMediaHelpers.h](../src/RoomMediaHelpers.h)

端上参考实现：

- [public/qos-demo.js](../public/qos-demo.js)
