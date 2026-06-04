# 客户端信令与媒体恢复接入

本文给客户端接入信令与媒体恢复使用。目标是把端上处理收敛成一个顺序：

1. 先区分信令是否可用
2. 再看媒体 transport 状态
3. 最后根据 `joinMode` 决定复用、`restartIce`，还是重建

## 1. 时序图

```text
正常状态

客户端                         信令服务                         worker/media
  |                              |                                  |
  | websocket 信令请求            |                                  |
  |----------------------------->|                                  |
  | 正常响应                      |                                  |
  |<-----------------------------|                                  |
  |                              |                                  |
  | WebRTC UDP 媒体收发           |                                  |
  |<--------------------------------------------------------------->|


情况一：媒体断，信令还在

客户端                         信令服务                         worker/media
  |                              |                                  |
  | transport disconnected/failed|                                  |
  |<---------------------------------------------------------------|
  | disconnected 后等待 10s       |                                  |
  |                              |                                  |
  | restartIce(transportId)       |                                  |
  |----------------------------->|                                  |
  |                              | transport.restartIce()           |
  |                              |--------------------------------->|
  |                              | 新 iceParameters                 |
  |                              |<---------------------------------|
  | iceParameters                 |                                  |
  |<-----------------------------|                                  |
  | transport.restartIce()        |                                  |
  |                              |                                  |
  | transport connected           |                                  |
  | RTP stats 增长                |                                  |
  |<--------------------------------------------------------------->|


情况二：信令断，媒体还在

客户端                         信令服务                         worker/media
  |                              |                                  |
  | websocket close/error         |                                  |
  | 或 wsRequest timeout          |                                  |
  |x-----------------------------x                                  |
  |                              |                                  |
  | 媒体仍 connected              |                                  |
  |<--------------------------------------------------------------->|
  |                              |                                  |
  | 重建 websocket                |                                  |
  | join(roomId, peerId)          |                                  |
  |----------------------------->|                                  |
  | joinMode=replaced-session     |                                  |
  |<-----------------------------|                                  |
  | 同步房间状态                  |                                  |
  | 沿用旧 transport              |                                  |


情况三：信令断，服务端 peer 已清理

客户端                         信令服务                         worker/media
  |                              |                                  |
  | 重建 websocket                |                                  |
  | join(roomId, peerId)          |                                  |
  |----------------------------->|                                  |
  | joinMode=new-peer             |                                  |
  |<-----------------------------|                                  |
  | 重新创建 transport            |                                  |
  | 重新 produce / consume        |                                  |
  |----------------------------->| 创建新的媒体资源                 |
```

## 2. 需要维护的状态

- websocket 状态：`connected / disconnected`
- send / recv transport 状态：`connected / disconnected / failed`
- `roomId`、`peerId`
- send / recv `transportId`
- 当前 producer / consumer 映射
- ICE 恢复定时器：`disconnected` 后等待 `10s`，避免短暂抖动立刻 `restartIce`
- ICE 恢复中标记：避免同一个 transport 并发或重复 `restartIce`

## 3. 媒体异常处理

媒体恢复入口是 transport 的 `connectionstatechange`，不要用 RTP stats 不增长直接触发恢复。

```js
transport.on('connectionstatechange', state => {
  if (state === 'connected') {
    clearIceRecoveryTimer(transport.id);
    return;
  }

  if (state === 'disconnected') {
    startIceRecoveryTimer(transport.id, 10_000);
    return;
  }

  if (state === 'failed') {
    restartIceForTransport(transport);
  }
});
```

`disconnected` 后先等 `10s`。如果 `10s` 内回到 `connected`，认为是短暂抖动；如果仍未恢复，或直接进入 `failed`，对对应 transport 做 `restartIce`。

## 4. 信令异常处理

websocket close/error 或 `wsRequest` 超时后，先标记信令不可用，不要继续发 `restartIce`：

```js
state.signalingState = 'disconnected';
failPendingRequests();
```

然后重建 websocket，并用同一个 `roomId + peerId` 重新 `join`：

```js
const ws = await connectWs();
const joinResp = await wsRequest('join', {
  roomId,
  peerId,
  displayName,
  rtpCapabilities
});
```

## 5. joinMode 分支

```js
if (joinResp.data.joinMode === 'replaced-session') {
  if (transportsConnected() && mediaLooksHealthy()) {
    await resyncRoomState(joinResp.data);
  } else {
    await restartIceForTransport(sendTransport);
    await restartIceForTransport(recvTransport);
  }
}

if (joinResp.data.joinMode === 'new-peer') {
  await rebuildMediaSession(joinResp.data);
}
```

- `replaced-session`
  服务端旧 peer 和媒体资源还在。媒体仍通时只同步房间状态；媒体不通时，对旧 transportId 做 `restartIce`。

- `new-peer`
  服务端已经没有可复用旧资源。端上要重新创建 send / recv transport，重新 connect、produce、consume，并重建本地映射。

如果旧 transportId 的 `restartIce` 返回 `room not found` / `peer not found` / `transport not found`，也按 `new-peer` 处理。

`mediaLooksHealthy()` 不应只看 stats。推荐口径是：transport 仍是 `connected`，并且在有活跃媒体预期时 RTP stats 继续增长；如果当前本来没有活跃音视频，stats 不增长不能判定媒体坏。

## 6. restartIce 调用

客户端请求：

```json
{
  "request": true,
  "id": 5,
  "method": "restartIce",
  "data": {
    "transportId": "send-transport-1"
  }
}
```

服务端返回：

```json
{
  "response": true,
  "id": 5,
  "ok": true,
  "data": {
    "iceParameters": {
      "usernameFragment": "new-ufrag",
      "password": "new-password",
      "iceLite": true
    }
  }
}
```

端上应用：

```js
const iceParameters = await requestServerRestartIce(transport.id);
await transport.restartIce({ iceParameters });
```

send transport 和 recv transport 是独立的。哪一路异常，就处理哪一路；两路都异常，就分别处理。

## 7. 恢复成功判定

不要只看：

- 服务端 `restartIce` 返回成功
- `transport.restartIce()` Promise resolve

恢复成功至少看：

1. 新 ICE 参数已返回，`usernameFragment/password` 变化
2. transport 回到 `connected`
3. RTP stats 继续增长

最小 stats 字段：

- `packetsSent`
- `bytesSent`
- `packetsReceived`
- `bytesReceived`

stats 增长是恢复成功判定，不是故障判定。`connected` 但 stats 不增长，不一定说明连接坏了，可能是用户关闭摄像头、producer/consumer pause、track ended/muted、页面后台或编码器无输出。

当前集成测试使用持续变化的 canvas 视频源，producer/consumer 未 pause，所以断言更严格：最多等 `15s`，每 `500ms` 采样一次；如果 stats 不增长，就认为恢复没有被证明成功。

## 8. 已验证

真实浏览器测试：

- [browser_ice_restart.mjs](../tests/qos_harness/browser_ice_restart.mjs)
- [ice-restart-entry.js](../tests/qos_harness/browser/ice-restart-entry.js)

覆盖：

- UDP 断开后 `disconnected/failed -> restartIce -> connected`
- TCP/WSS 信令断开时，媒体仍可继续增长，`restartIce` 请求会超时
- 信令恢复返回 `replaced-session` 后，旧 transport 再遇到 UDP 故障仍可通过旧 transportId `restartIce` 恢复
- websocket close 清理 peer 后，重新 join 返回 `new-peer`，旧 transport 不再可用
