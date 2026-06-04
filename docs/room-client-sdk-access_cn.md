# 房间客户端 SDK 接入

本文只面向新的浏览器端 SDK：

- `MediasoupRoomClient`
- `TalkbackClient`

目标是让业务直接用 SDK 拿流、收事件、处理对讲，而不再自己拼 `join / createWebRtcTransport / consume / restartIce` 细节。

## 1. 最小接入

创建房间客户端：

```js
const room = new MediasoupRoomClient({
  wssUrl: 'wss://example.com/ws',
  roomId: 'room-1',
  peerId: 'peer-a',
  displayName: 'peer-a'
});
```

监听事件：

```js
const unsubs = [
  room.on('track', info => {
    renderRemote(info.stream, info);
  }),
  room.on('trackClosed', info => {
    removeRemote(info.consumerId);
  }),
  room.on('networkState', event => {
    renderNetworkState(event.state, event.data);
  }),
  room.on('peerJoined', peer => {
    renderPeer(peer);
  }),
  room.on('peerLeft', peer => {
    removePeer(peer.peerId);
  }),
  room.on('error', error => {
    console.error(error.message, error.context);
  }),
];
```

加入和离开房间：

```js
await room.join();
await room.leave();

for (const off of unsubs) {
  off();
}
```

## 2. 远端流事件

`track` 事件返回：

```js
{
  peerId,
  producerId,
  consumerId,
  kind,
  source,
  appData,
  stream,
  track
}
```

字段含义：

- `peerId`：发布端 peer
- `producerId`：远端 producer
- `consumerId`：当前订阅端上的 consumer
- `kind`：`audio` 或 `video`
- `source`：媒体来源，例如 `audio`、`camera`、`screenShare`、`talkback`
- `appData`：发布这路媒体时带上的业务字段
- `stream`：直接可播放的 `MediaStream`
- `track`：底层 `MediaStreamTrack`

`trackClosed` 事件表示这路远端媒体已经不可用。业务应移除对应 DOM、停止播放并清理本地映射。

## 3. 本地发布

发布普通音频：

```js
const [audioProducer] = await room.publish({
  audio: {
    appData: {
      source: 'audio',
      label: 'front-desk'
    }
  }
});
```

发布摄像头视频：

```js
const [videoProducer] = await room.publish({
  video: {
    appData: {
      source: 'camera',
      cameraId: 'cam-1'
    }
  }
});
```

发布屏幕共享：

```js
const [screenProducer] = await room.publish({
  screen: {
    appData: {
      source: 'screenShare',
      screenId: 'screen-main'
    }
  }
});
```

关闭某一路：

```js
await room.unpublish({ producerId: audioProducer.id });
```

关闭全部本地发布：

```js
await room.unpublish();
```

## 4. 对讲接入

对讲使用 `TalkbackClient`：

```js
const talkback = new TalkbackClient(room, {
  onTargetsChanged: targets => {
    renderTargets(targets);
  },
  onStateChange: event => {
    renderTalkbackState(event.state, event.targetPeerId);
  }
});
```

打开对讲：

```js
await talkback.claim(targetPeerId);
await talkback.openMic(targetPeerId);
```

关闭对讲：

```js
await talkback.closeMic();
await talkback.release();
```

SDK 会把对讲音频作为普通 audio producer 发布，但媒体级 `appData.source` 固定为 `talkback`，并带上 `targetPeerId`。

受限端收到的远端流仍然通过 `track` 事件交给业务：

```js
room.on('track', info => {
  if (info.source === 'talkback') {
    renderTalkbackStream(info.stream, info.peerId);
  }
});
```

## 5. 恢复行为

业务不需要自己处理：

- `join`
- `createWebRtcTransport`
- `consume`
- `restartIce`
- websocket 重连

SDK 内部会处理：

- `request timeout` 后的信令恢复
- `joinMode=replaced-session`
- `joinMode=new-peer`
- transport `disconnected/failed` 后的媒体恢复

业务只需要看 `networkState` 事件：

```js
room.on('networkState', event => {
  console.log(event.state, event.data);
});
```

当前已经验证过的关键状态：

- `joined`
- `connected`
- `reconnecting`
- `transport-state`
- `session-restored`
- `closed`

其中：

- `joined.data.joinMode` 用于区分 `new-peer` 和 `replaced-session`
- `transport-state.data.connectionState` 用于观察 `connected / disconnected / failed`

## 6. 最小实践建议

- 业务保存 `consumerId -> DOM` 映射，用 `track` 创建、`trackClosed` 清理。
- 业务保存 `producerId -> 本地按钮状态` 映射，用于本地 publish/unpublish 管理。
- 不要自己直接发 `consume`、`restartIce`、`connectWebRtcTransport`。
- 不要只停本地 track 而不调用 `room.unpublish()`。
- 对讲目标列表直接使用 `TalkbackClient` 的 `onTargetsChanged`。
