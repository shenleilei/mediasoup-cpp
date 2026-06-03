# 音频受限端客户端最小接入

本文按人的实际使用顺序写：先打开受限端，再打开普通端，普通端选择受限端并打开音频，最后关闭音频。第一版只考虑 `WebRtcTransport`。

示例里的 `id` 是客户端数字流水号，只用于匹配 request/response，不是业务 id。示例 JSON 只展示业务接入需要关注的字段，ICE/DTLS/RTP 参数用 `{}` 省略。

## 0. 业务最小接入清单

业务只需要按下面几条接：

- 受限端入会时带 `audioRole="audio-restricted"`；普通端不带 `audioRole` 或带 `audioRole="normal"`。
- 普通端从 `join.data.participants` 和 `peerJoined` 维护“可选择的受限端列表”，显示 `displayName (peerId)`，选中值用 `peerId`。
- 普通端点击“打开”时，先发 `claimAudioRestrictedSlot(targetPeerId)`；claim 成功后再创建或复用本地 audio producer。
- `claimAudioRestrictedSlot` 失败时不要采集麦克风、不要 produce。
- claim 成功但采集麦克风或 produce 失败时，要发 `releaseAudioRestrictedSlot(targetPeerId)` 清掉占位。
- 普通端点击“关闭”时，当前 demo 的语义是“不再发本地音频”：先 `closeProducer`，再 `releaseAudioRestrictedSlot`。
- 受限端不需要发 claim，也不需要额外鉴权；只处理服务端下发的 `newConsumer` / `consumers`，以及后续清理通知。
- 这个限制只影响受限端接收 audio；普通端接收 audio、受限端接收 video 都按原流程走。
- 业务本地最少记三类信息：自己的 `peerId`、可选受限端列表、当前已打开的目标 `peerId` 和本地 audio producer。
- 如果当前已打开的受限端退出又用同一个 `peerId` 重进，普通端在收到该 peer 的 `peerJoined` 后要重新发一次 `claimAudioRestrictedSlot`。

## 1. 打开受限端页面并加入房间

受限端 URL：

```text
https://volcvideo3.zelostech.com.cn:1770/?audioRole=audio-restricted&displayName=受限端A
```

受限端点击加入房间时发 `join`：

```json
{
  "request": true,
  "id": 1,
  "method": "join",
  "data": {
    "roomId": "room-1",
    "peerId": "peer-b",
    "displayName": "受限端A",
    "audioRole": "audio-restricted"
  }
}
```

受限端收到：

```json
{
  "response": true,
  "id": 1,
  "ok": true,
  "data": {
    "audioRole": "audio-restricted",
    "routerRtpCapabilities": {},
    "existingProducers": [],
    "participants": [
      {
        "peerId": "peer-b",
        "displayName": "受限端A",
        "audioRole": "audio-restricted",
        "producers": []
      }
    ]
  }
}
```

受限端行为：

- 正常 `device.load(routerRtpCapabilities)`。
- 正常创建 recv `WebRtcTransport`。
- 正常处理 `existingProducers`、transport 响应里的 `consumers`、后续 `newConsumer`。
- 没收到 audio consumer 是正常状态，不是连接失败。

## 2. 打开普通端页面并加入同一房间

普通端 URL：

```text
https://volcvideo3.zelostech.com.cn:1770/?displayName=普通端A
```

普通端点击加入房间时发 `join`：

```json
{
  "request": true,
  "id": 1,
  "method": "join",
  "data": {
    "roomId": "room-1",
    "peerId": "peer-a",
    "displayName": "普通端A",
    "audioRole": "normal"
  }
}
```

`audioRole` 不传时默认为 `normal`。

普通端收到：

```json
{
  "response": true,
  "id": 1,
  "ok": true,
  "data": {
    "audioRole": "normal",
    "routerRtpCapabilities": {},
    "existingProducers": [],
    "participants": [
      {
        "peerId": "peer-b",
        "displayName": "受限端A",
        "audioRole": "audio-restricted",
        "producers": []
      },
      {
        "peerId": "peer-a",
        "displayName": "普通端A",
        "audioRole": "normal",
        "producers": []
      }
    ]
  }
}
```

普通端行为：

- 原有收发流流程不变。
- 从 `participants` 里维护“可选择的受限端列表”。
- `participants` 包含自己，必须排除 `selfPeerId`。
- 只保留 `audioRole="audio-restricted"` 且 `peerId != selfPeerId` 的 peer。
- 下拉框显示 `displayName (peerId)`，下拉值使用 `peerId`。
- 不从 producer `source` 获取目标端；`source=audio` 只表示媒体来源。

## 3. 房间成员变化时更新下拉框

上面的顺序是推荐验证顺序。实际使用中如果普通端先加入房间，下拉框一开始可以为空；等受限端加入后，再通过 `peerJoined` 把它加入可选列表。

有新 peer 加入时，普通端收到 `peerJoined`：

```json
{
  "notification": true,
  "method": "peerJoined",
  "data": {
    "peerId": "peer-d",
    "displayName": "受限端D",
    "audioRole": "audio-restricted",
    "reconnect": false
  }
}
```

普通端行为：

- 如果 `audioRole="audio-restricted"` 且不是自己，就加入可选列表。
- 如果该 `peerId` 是当前已打开目标，且本端仍在发布 audio，可以重新 claim。
- 不要只依赖 `reconnect=true` 判断是否重 claim；受限端先退出再重新加入时，`peerJoined.reconnect` 可能是 `false`，关键是 `peerId` 是否等于当前已打开目标。

peer 离开时，普通端收到 `peerLeft`：

```json
{
  "notification": true,
  "method": "peerLeft",
  "data": {
    "peerId": "peer-d"
  }
}
```

普通端行为：

- 从可选列表删除该 `peerId`。
- 当前 demo 会保留已打开目标状态，用于同 `peerId` 重连后重新 claim。
- 业务如果不需要重连保持，也可以在 `peerLeft` 时清空本地打开状态。

### 3.1 受限端退出又重进

服务端在受限端离开时会清掉这个受限端对应的占位，并关闭该受限端上已有的 audio consumer。普通端如果还在发布本地 audio，并且业务希望同一个受限端恢复后继续听，就不能只把 `peerLeft` 当成永久关闭。

推荐业务行为：

- `peerLeft(peer-b)` 时，从下拉框删除 `peer-b`，但如果 `peer-b` 是当前已打开目标，可以保留本地打开状态。
- `peerJoined(peer-b, audioRole="audio-restricted")` 时，把 `peer-b` 加回下拉框。
- 如果 `peer-b` 等于当前已打开目标，且本端本地 audio producer 还存在，立即重新发 `claimAudioRestrictedSlot({ "targetPeerId": "peer-b" })`。
- 重 claim 成功后，不需要重新 produce；原来的 audio producer 还在，服务端会重新给重进的受限端创建 audio consumer。
- 如果重 claim 失败，例如目标已被其他普通端占用，清空本地打开状态并提示业务重新选择或重新打开。

当前 public demo 已按这个逻辑验证：受限端关闭页面后用同一个 `peerId` 重新加入，普通端会自动再发一次 `claimAudioRestrictedSlot`，重进后的受限端会重新收到 audio consumer。

## 4. 普通端选择受限端并打开音频

用户在普通端下拉框里选择 `受限端A (peer-b)` 后，客户端用选中项的 `peerId` 发 claim。

普通端发：

```json
{
  "request": true,
  "id": 2,
  "method": "claimAudioRestrictedSlot",
  "data": {
    "targetPeerId": "peer-b"
  }
}
```

占位成功时，普通端收到：

```json
{
  "response": true,
  "id": 2,
  "ok": true,
  "data": {
    "required": true,
    "claimed": true,
    "targetPeerId": "peer-b",
    "ownerPeerId": "peer-a",
    "alreadyOwned": false,
    "consumersCreated": 0
  }
}
```

普通端行为：

- 记录本地打开状态，例如 `claimedAudioTargetPeerId = "peer-b"`。
- 然后发布 audio producer，`produce.appData.source = "audio"`。当前 demo 是先 claim 后 produce，所以这里的 `consumersCreated` 通常是 0。
- `alreadyOwned=true` 表示重复 claim，同样按成功处理。
- `consumersCreated` 只用于观测，不需要客户端据此播放；受限端真正播放以 `newConsumer` / `consumers` 为准。

普通端随后走原有发布流程：创建或复用 send `WebRtcTransport`，并在 `transport.produce()` 回调里发 `produce`。这里不是新增 transport 协议，只是 audio producer 的 `appData.source` 必须带上 `audio`。示例只展示业务字段：

```json
{
  "request": true,
  "id": 3,
  "method": "produce",
  "data": {
    "transportId": "send-transport-a",
    "kind": "audio",
    "rtpParameters": {},
    "appData": {
      "source": "audio"
    }
  }
}
```

普通端收到：

```json
{
  "response": true,
  "id": 3,
  "ok": true,
  "data": {
    "id": "producer-audio-1"
  }
}
```

普通端行为：

- 保存本地 audio producer。
- 如果 produce 失败或麦克风采集失败，立即对刚才的 `targetPeerId` 发 `releaseAudioRestrictedSlot`。
- 不要把 `appData.source` 当成目标端；目标端只来自下拉框选中的 `targetPeerId`。

目标是普通端时，普通端收到：

```json
{
  "response": true,
  "id": 2,
  "ok": true,
  "data": {
    "required": false,
    "claimed": false,
    "reason": "not-required",
    "targetPeerId": "peer-normal"
  }
}
```

普通端行为：

- 不记录占位。
- 按普通发流逻辑继续。

已被其他发送端占用时，普通端收到：

```json
{
  "response": true,
  "id": 2,
  "ok": false,
  "error": "audio slot occupied",
  "data": {
    "reason": "occupied",
    "targetPeerId": "peer-b",
    "ownerPeerId": "peer-other"
  }
}
```

普通端行为：

- 不抢占。
- 不记录本地打开状态。
- UI 提示目标已被占用。

目标不存在时，普通端收到：

```json
{
  "response": true,
  "id": 2,
  "ok": false,
  "error": "target peer not found",
  "data": {
    "reason": "target-not-found",
    "targetPeerId": "peer-b"
  }
}
```

普通端行为：不记录本地打开状态，按业务决定是否等待目标重新加入。

## 5. 受限端收到音频

普通端 claim 成功并 produce audio 后，服务端才会给受限端创建 audio consumer。受限端不需要主动判断“谁 claim 了我”，也不需要再发额外授权请求；只按现有订阅流程处理服务端下发的 consumer。

第一种时序是受限端已经创建好 recv transport，普通端后来打开音频。受限端会收到 `newConsumer`：

```json
{
  "notification": true,
  "method": "newConsumer",
  "data": {
    "peerId": "peer-a",
    "producerId": "producer-audio-1",
    "id": "consumer-audio-1",
    "kind": "audio",
    "appData": {
      "source": "audio"
    },
    "producerPaused": false,
    "rtpParameters": {}
  }
}
```

第二种时序是普通端已经 claim 并 produce audio，受限端之后才创建 recv transport。此时不一定再收到额外 `newConsumer`，audio consumer 可能直接出现在 `createWebRtcTransport` 响应的 `consumers` 数组里：

```json
{
  "response": true,
  "id": 2,
  "ok": true,
  "data": {
    "id": "recv-transport-b",
    "iceParameters": {},
    "iceCandidates": [],
    "dtlsParameters": {},
    "consumers": [
      {
        "peerId": "peer-a",
        "producerId": "producer-audio-1",
        "id": "consumer-audio-1",
        "kind": "audio",
        "appData": {
          "source": "audio"
        },
        "producerPaused": false,
        "rtpParameters": {}
      }
    ]
  }
}
```

受限端行为：

- `newConsumer.data` 和 `createWebRtcTransport.data.consumers[]` 是同一种 consumer 参数，处理函数可以复用。
- 调用 `recvTransport.consume({ id, producerId, kind, rtpParameters, appData })`。
- 用 `consumer.track` 创建 `MediaStream`，挂到 audio DOM 上。
- 保存 `consumerId -> consumer/audio DOM` 映射，后续 `producerLeft` 或 `consumerClosed` 要按 consumerId 清理。
- `data.peerId` 是发流端，也就是普通端 A；不是受限端自己的 peerId。
- `data.appData.source="audio"` 只表示这条 producer 是音频来源，不表示目标端。
- 受限端收到 audio consumer 后才应该出现远端音频卡片或音量变化；claim 成功但还没 produce 时，不会有音频 consumer。

## 6. 普通端关闭受限端音频

当前 demo 只维护一个 audio producer。关闭时先关 producer，再 release slot。
页面上的普通端“关闭”按钮走的就是这条路径，语义是“不再发布本地音频”。

### 6.1 closeProducer

普通端发：

```json
{
  "request": true,
  "id": 4,
  "method": "closeProducer",
  "data": {
    "source": "audio"
  }
}
```

也可以按 producerId 发：

```json
{
  "request": true,
  "id": 4,
  "method": "closeProducer",
  "data": {
    "producerId": "producer-audio-1"
  }
}
```

普通端收到：

```json
{
  "response": true,
  "id": 4,
  "ok": true,
  "data": {
    "producerId": "producer-audio-1",
    "closedConsumers": 1,
    "notifiedPeers": ["peer-b"]
  }
}
```

普通端行为：

- 本地 `producer.close()`。
- 停止本地 audio track。
- 删除本地 producer 记录。

常见失败：

- `missing producerId or source`：请求没有带 selector。
- `missing source`：按 source 关闭但 source 为空。
- `permission denied`：不是 producer owner。
- `producer not found`：producer 不存在，或 source 未匹配。
- `ambiguous producer source`：同 source 多 producer，改用 `producerId`。

### 6.2 releaseAudioRestrictedSlot

普通端发：

```json
{
  "request": true,
  "id": 5,
  "method": "releaseAudioRestrictedSlot",
  "data": {
    "targetPeerId": "peer-b"
  }
}
```

释放成功时，普通端收到：

```json
{
  "response": true,
  "id": 5,
  "ok": true,
  "data": {
    "released": true,
    "targetPeerId": "peer-b",
    "closedConsumers": 0
  }
}
```

其他返回：

- `ok=true, data.released=false, data.reason="not-claimed"`
- `ok=true, data.released=false, data.reason="not-required"`
- `ok=false, data.reason="not-owner"`
- `ok=false, data.reason="target-not-found"`

普通端行为：

- release 返回后清空本地打开状态。
- `not-claimed`、`not-required` 或 `not-owner` 也清空本地状态。
- `closedConsumers` 是本次 release 实际关闭的 consumer 数，可能是 0 或更多。当前 demo 先 `closeProducer`，consumer 通常已经被 producer 关闭，所以这里常见是 0。

## 7. 受限端清理音频

远端 producer 被关闭时，受限端收到 `producerLeft`：

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

受限端行为：

- 按 `consumerIds` 删除 consumer 和 DOM。
- 按 `producerId` 兜底清理。
- 不要当成 `peerLeft`。

slot release 只关闭 consumer 时，受限端收到 `consumerClosed`：

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

受限端行为：

- 按 `consumerId` 删除 consumer 和 DOM。
- 不要自动重试 consume，除非业务重新 claim 成功。

## 8. 两种关闭语义

第一种是关闭 producer，也就是普通端不再发本地音频。普通端先发 `closeProducer`，再发 `releaseAudioRestrictedSlot`；受限端通常收到 `producerLeft`，因为 producer 本身已经不存在。后续 release 只是释放占位，如果 consumer 已经随 producer 关闭，`closedConsumers` 可能是 0，也不会再收到 `consumerClosed`。当前 public demo 普通端页面里的“关闭”按钮就是这种。

第二种是只释放受限端，也就是普通端的 audio producer 继续存在，只是不再让某个受限端消费这路音频。普通端只发 `releaseAudioRestrictedSlot`，不发 `closeProducer`；受限端收到 `consumerClosed`，`reason="audio-slot-release"`。这个适用于以后要支持“取消某个受限端收听，但本地音频还继续给普通端或其他目标使用”的场景；当前 public demo 没有单独提供这个按钮。
