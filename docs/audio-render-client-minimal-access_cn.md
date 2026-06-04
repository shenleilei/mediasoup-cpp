# 音频受限端客户端最小接入

本文给业务客户端接入音频受限能力使用。第一版只考虑 `WebRtcTransport`，不做 audio consumer `pause/resume`，只做打开和关闭。

示例里的 `id` 是客户端数字流水号，只用于匹配 request/response，不是业务 id。示例 JSON 只展示业务接入需要关注的字段，ICE/DTLS/RTP 参数用 `{}` 省略。

## 1. 接入目标

业务要实现的是：某些端入会时标记为 `audio-restricted`，默认收不到任何普通端的 audio。普通端必须先选择某个受限端并 claim 成功，然后再发布 audio；服务端只会把这路 audio 转发给被 claim 的受限端。其他普通端不受影响，仍按原有逻辑接收 audio。

最少需要接这几件事：

- 受限端入会时带 `audioRole="audio-restricted"`；普通端不带 `audioRole` 或带 `audioRole="normal"`。
- 普通端从 `join.data.participants` 和 `peerJoined` 维护受限端列表。
- 下拉框显示 `displayName (peerId)`，选中值使用 `peerId`。
- 普通端点击“打开”时，先发 `claimAudioRestrictedSlot(targetPeerId)`，成功后再采集麦克风并 `produce` audio。
- `claimAudioRestrictedSlot` 失败时不要采集麦克风、不要 produce。
- claim 成功但麦克风采集或 produce 失败时，立即发 `releaseAudioRestrictedSlot(targetPeerId)` 清占位。
- 当前 demo 的“关闭 Audio”语义是不再发布本地音频：先发 `closeProducer`，再发 `releaseAudioRestrictedSlot`。
- 受限端不主动 claim，只处理服务端下发的 `newConsumer`、`createWebRtcTransport.data.consumers[]`、`producerLeft`、`consumerClosed`。

业务本地最少保存这些状态：

- `selfPeerId`：当前端自己的 peerId。
- `audioRestrictedPeers`：可选受限端列表，来源是 `join.data.participants` 和 `peerJoined`。
- `openedAudioTargetPeerId`：当前已打开的受限端 peerId。
- `audioProducer`：当前本地 audio producer。当前 demo 只考虑一个 audio producer。
- `consumers`：受限端本地 `consumerId -> { consumer, producerId, element }` 映射，用于关闭通知清理。

### 1.1 接入步骤速查

| 顺序 | 业务动作 | 客户端发什么/收什么 | 客户端必须做什么 |
| --- | --- | --- | --- |
| 1 | 受限端入会 | 发 `join(audioRole="audio-restricted")` | 正常创建 recv transport；没有 audio consumer 是正常状态 |
| 2 | 普通端入会 | 收 `join.data.participants` 和 `peerJoined` | 维护受限端下拉框，显示 `displayName (peerId)`，值用 `peerId` |
| 3 | 普通端打开 | 发 `claimAudioRestrictedSlot(targetPeerId)` | 成功后才采集麦克风并 `produce`；失败不要 produce |
| 4 | 普通端发布 audio | 发 `produce(kind="audio", appData.source="audio")` | 保存 `audioProducer`；produce 失败要 `releaseAudioRestrictedSlot` |
| 5 | 受限端收到 audio | 收 `newConsumer` 或 `consumers[]` | 调 `recvTransport.consume()`，渲染 audio，保存 `consumerId` 映射 |
| 6 | 当前 demo 关闭 | 普通端发 `closeProducer`，再发 `releaseAudioRestrictedSlot` | 普通端关本地 producer 和 track；受限端处理 `producerLeft` |
| 7 | 只释放受限端 | 普通端只发 `releaseAudioRestrictedSlot` | producer 可继续存在；受限端处理 `consumerClosed` |
| 8 | 受限端重进 | 普通端收同 peerId 的 `peerJoined` | 如果本地 audio producer 还在，重新 claim；不需要重新 produce |

## 2. 完整时序

```mermaid
sequenceDiagram
  participant R as 受限接收端 B
  participant S as 服务端
  participant A as 普通发送端 A

  R->>S: join(audioRole="audio-restricted", displayName)
  S-->>R: join response(participants, routerRtpCapabilities)
  S-->>A: peerJoined(B, audioRole="audio-restricted", displayName)
  R->>S: createWebRtcTransport(consuming=true)
  S-->>R: recv transport params, consumers=[]

  A->>S: join(audioRole="normal", displayName)
  S-->>A: join response(participants includes B)
  A->>A: 下拉框显示 displayName(peerId)

  A->>S: claimAudioRestrictedSlot(targetPeerId=B)
  alt B 未被占用
    S->>S: 记录 B 的 owner=A
    S-->>A: ok(required=true, claimed=true)
    A->>S: produce(kind="audio", appData.source="audio")
    S->>S: 只给 B 创建来自 A 的 audio consumer
    S-->>A: ok(producerId)
    S-->>R: newConsumer(audio, producerPeerId=A, consumerId)
    R->>R: recvTransport.consume()
    R->>R: 渲染 audio，记录 consumerId -> Consumer/audio DOM
  else B 已被其他发送端占用
    S-->>A: ok=false, reason="occupied", ownerPeerId
    A->>A: 不采集麦克风，不 produce，提示被占用
  end

  alt 当前 demo 关闭 Audio：不再发布本地音频
    A->>S: closeProducer(producerId 或 source="audio")
    S->>S: 关闭 producer 和相关 consumers
    S-->>R: producerLeft(producerId, consumerIds)
    R->>R: consumer.close(); 清理 audio DOM/音量/缓存
    S-->>A: ok(closedConsumers, notifiedPeers)
    A->>A: producer.close(); stop local audio track
    A->>S: releaseAudioRestrictedSlot(targetPeerId=B)
    S->>S: 释放 B 的 owner
    S-->>A: ok(released=true, closedConsumers=0)
    A->>A: 清空本地打开状态
  else 只释放受限端：producer 继续存在
    A->>S: releaseAudioRestrictedSlot(targetPeerId=B)
    S->>S: 释放 B 的 owner，关闭 B 上来自 A 的 audio consumer
    S-->>R: consumerClosed(consumerId, reason="audio-slot-release")
    R->>R: consumer.close(); 清理 audio DOM/音量/缓存
    S-->>A: ok(released=true, closedConsumers=1)
    A->>A: 清空本地打开状态，audio producer 可继续保留
  end
```

## 3. 入会和受限端列表

### 3.1 受限端入会

受限端 URL 示例：

```text
https://volcvideo3.zelostech.com.cn:1770/?audioRole=audio-restricted&displayName=受限端A
```

受限端发 `join`：

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

受限端处理：

- 正常 `device.load(routerRtpCapabilities)`。
- 正常创建 recv `WebRtcTransport`。
- 正常处理 `existingProducers`、`createWebRtcTransport.data.consumers[]`、后续 `newConsumer`。
- 刚入会没有 audio consumer 是正常状态，不是连接失败。

服务端语义：

- 保存该 peer 的 `audioRole="audio-restricted"`。
- 在没有普通端成功 claim 之前，不给该受限端下发 audio consumer。
- video consumer 和普通媒体流程不受这个角色影响。

### 3.2 普通端入会并获得 peer 信息

普通端 URL 示例：

```text
https://volcvideo3.zelostech.com.cn:1770/?displayName=普通端A
```

普通端发 `join`：

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

普通端处理：

- 原有收发流流程不变。
- 从 `participants` 里筛出 `audioRole="audio-restricted"` 且 `peerId != selfPeerId` 的 peer。
- 下拉框显示 `displayName (peerId)`，下拉值使用 `peerId`。
- 不从 producer `source` 获取目标端；`source=audio` 只表示媒体来源。

后续有新 peer 加入时，普通端收到 `peerJoined`：

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

普通端处理：

- 如果 `audioRole="audio-restricted"` 且不是自己，就加入下拉列表。
- 如果该 `peerId` 是当前已打开目标，且本端仍有 `audioProducer`，重新发 `claimAudioRestrictedSlot`。
- 不要只依赖 `reconnect=true` 判断是否重 claim；受限端先退出再重新加入时，`reconnect` 可能是 `false`。

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

普通端处理：

- 从下拉列表删除该 `peerId`。
- 如果该 `peerId` 是当前已打开目标，业务可以保留 `openedAudioTargetPeerId`，用于同 `peerId` 重进后自动重 claim。
- 如果业务不需要重连保持，也可以在 `peerLeft` 时清空本地打开状态。

服务端语义：

- 普通端不需要占位就可以接收其他普通端的 audio。
- 普通端是否 claim 某个受限端，只影响该受限端能不能收到这个普通端的 audio。
- 受限端退出或重连替换旧 peer 时，服务端会清理旧占位和旧 consumer；如果业务要恢复，需要发送端重新 claim。

## 4. 打开受限端音频

用户在普通端下拉框选择 `受限端A (peer-b)` 后，客户端用选中项的 `peerId` 发 claim。

### 4.1 先 claim

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

claim 成功时，普通端收到：

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

普通端处理：

- 记录 `openedAudioTargetPeerId = "peer-b"`。
- 然后才采集麦克风并发布 audio producer。
- `alreadyOwned=true` 表示重复 claim，同样按成功处理。
- `consumersCreated` 只用于观测，不需要客户端据此播放；受限端真正播放以 `newConsumer` 或 `consumers[]` 为准。

服务端语义：

- 如果目标是受限端且未被占用，记录 `targetPeerId -> ownerPeerId`。
- 如果同一个 owner 重复 claim，返回 `alreadyOwned=true`。
- 如果目标已被其他 owner 占用，返回 `occupied`，不会抢占。
- 如果 owner 已经有 audio producer，服务端会尝试给目标受限端补建 audio consumer；否则等待后续 `produce`。

claim 失败时，普通端不要采集麦克风、不要 produce。

已被其他发送端占用：

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

目标不存在：

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

目标是普通端：

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

`required=false` 表示目标不是受限端，不需要占位。普通端是否继续按普通发流逻辑处理，由业务自己决定。

### 4.2 claim 成功后 produce audio

普通端随后走原有发布流程：创建或复用 send `WebRtcTransport`，在 `transport.produce()` 回调里发 `produce`。这里不是新增 transport 协议，只要求 audio producer 的 `appData.source` 是 `"audio"`。

普通端发：

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

普通端处理：

- 保存本地 `audioProducer`。
- 如果麦克风采集失败或 `produce` 失败，立即对刚才的 `targetPeerId` 发 `releaseAudioRestrictedSlot`。
- 不要把 `appData.source` 当成目标端；目标端只来自下拉框选中的 `targetPeerId`。

服务端语义：

- 对普通端接收者，audio producer 仍按原有房间转发逻辑处理。
- 对 `audioRole="audio-restricted"` 的接收者，只有当该受限端的 owner 是当前 producer 所属 peer 时，才创建 audio consumer。
- 如果发送端没有先 claim，服务端不会向受限端下发这路 audio consumer。

## 5. 受限端接收音频

普通端 claim 成功并 produce audio 后，服务端才会给受限端创建 audio consumer。受限端不需要主动判断“谁 claim 了我”，也不需要再发额外授权请求。

### 5.1 受限端已创建 recv transport

如果受限端已经创建好 recv transport，普通端后来打开音频，受限端会收到 `newConsumer`：

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

### 5.2 受限端后创建 recv transport

如果普通端已经 claim 并 produce audio，受限端之后才创建 recv transport，audio consumer 可能直接出现在 `createWebRtcTransport` 响应的 `consumers[]` 里：

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

受限端处理：

- `newConsumer.data` 和 `createWebRtcTransport.data.consumers[]` 是同一种 consumer 参数，处理函数复用。
- 调用 `recvTransport.consume({ id, producerId, kind, rtpParameters, appData })`。
- 用 `consumer.track` 创建 `MediaStream`，挂到 audio DOM 上。
- 保存 `consumerId -> { consumer, producerId, element }`，后续 `producerLeft` 或 `consumerClosed` 按 consumerId 清理。
- `data.peerId` 是发流端，也就是普通端 A，不是受限端自己的 peerId。
- `data.appData.source="audio"` 只表示这条 producer 是音频来源，不表示目标端。
- 受限端收到 audio consumer 后才应该出现远端音频卡片或音量变化；claim 成功但还没 produce 时，不会有 audio consumer。

服务端语义：

- 如果受限端已经有 recv transport，服务端通过 `newConsumer` 通知新增 audio consumer。
- 如果受限端后创建 recv transport，服务端把已有可消费 consumer 放在 `createWebRtcTransport` 响应的 `consumers[]` 里。
- 这两种时序下，受限端都不需要额外发“同意接收 audio”的信令。

## 6. 关闭音频

关闭有两种语义，业务必须区分。

### 6.1 当前 demo：关闭 producer，再释放占位

当前 public demo 普通端页面里的“关闭 Audio”按钮走这条路径。语义是：普通端不再发布本地音频。

普通端先发 `closeProducer`：

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

也可以按 `source` 关闭：

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

服务端会给受影响的受限端下发 `producerLeft`：

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

受限端收到 `producerLeft` 后：

- 按 `consumerIds` 找本地 `mediasoup-client Consumer`。
- 调用 `consumer.close()`。
- 清理 audio DOM：`audio.pause()`、`audio.srcObject = null`、移除 DOM。
- 清理音量检测、QoS hint、consumer 状态缓存。
- 如果 `consumerIds` 为空或没匹配到，按 `producerId` 兜底清理本地保存的 consumer。
- 不要当成 `peerLeft`，也不需要给服务端发 ack。

普通端本地处理：

- `producer.close()`。
- 停止本地 audio track。
- 清空本地 `audioProducer`。
- 继续发 `releaseAudioRestrictedSlot` 清占位。

普通端再发 `releaseAudioRestrictedSlot`：

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

普通端收到：

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

这里 `closedConsumers` 常见是 0，因为 producer 已经先关闭，相关 consumer 通常已经随 `closeProducer` 被关闭。普通端收到 release 返回后清空 `openedAudioTargetPeerId`。

### 6.2 可选语义：只释放受限端，producer 继续存在

这条路径适用于以后业务想“只取消某个受限端收听，但本地 audio producer 继续存在”。当前 public demo 没有单独按钮。

普通端只发 `releaseAudioRestrictedSlot`，不发 `closeProducer`：

```json
{
  "request": true,
  "id": 6,
  "method": "releaseAudioRestrictedSlot",
  "data": {
    "targetPeerId": "peer-b"
  }
}
```

如果 producer 仍存在，服务端会关闭受限端上来自 owner 的 audio consumer，并给受限端发 `consumerClosed`：

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

受限端收到 `consumerClosed` 后：

- 按 `consumerId` 找本地 `mediasoup-client Consumer`。
- 调用 `consumer.close()`。
- 清理 audio DOM：`audio.pause()`、`audio.srcObject = null`、移除 DOM。
- 清理音量检测、QoS hint、consumer 状态缓存。
- 不需要调用 `consumer.pause()` 或 `consumer.resume()`。
- 不需要给服务端发额外 ack。
- 不要自动重试 consume，除非业务重新 claim 成功。

普通端收到 release 返回后：

- 清空 `openedAudioTargetPeerId`。
- 如果本地 `audioProducer` 仍然存在，可以继续保留；不要因为 release 成功就默认 `producer.close()`。

服务端语义：

- `consumerClosed` 由服务端触发，不是客户端主动 close consumer 触发。
- 触发入口是 `releaseAudioRestrictedSlot` 释放占位，并且服务端发现目标受限端上还有来自 owner 的 audio consumer。
- 受限端退出或重连替换旧 peer 时，服务端也会清理旧占位和旧 consumer；如果旧 consumer 仍存在，也可能走同样的关闭清理逻辑。

### 6.3 release 常见返回

```json
{
  "response": true,
  "id": 5,
  "ok": true,
  "data": {
    "released": false,
    "reason": "not-claimed",
    "targetPeerId": "peer-b"
  }
}
```

常见结果：

- `ok=true, data.released=false, data.reason="not-claimed"`：本来就没有占位。
- `ok=true, data.released=false, data.reason="not-required"`：目标不是受限端。
- `ok=false, data.reason="not-owner"`：当前端不是 owner。
- `ok=false, data.reason="target-not-found"`：目标 peer 不存在。

普通端处理：

- release 成功后清空本地打开状态。
- `not-claimed`、`not-required`、`not-owner` 也建议清空本地打开状态，避免 UI 和服务端状态不一致。

## 7. 受限端退出又重进

服务端在受限端离开时会清掉这个受限端对应的占位，并关闭该受限端上已有的 audio consumer。普通端如果还在发布本地 audio，并且业务希望同一个受限端恢复后继续听，需要在该受限端重新加入时重新 claim。

推荐普通端处理：

- `peerLeft(peer-b)` 时，从下拉框删除 `peer-b`。
- 如果 `peer-b` 是当前已打开目标，可以保留 `openedAudioTargetPeerId`。
- `peerJoined(peer-b, audioRole="audio-restricted")` 时，把 `peer-b` 加回下拉框。
- 如果 `peer-b === openedAudioTargetPeerId` 且本端 `audioProducer` 还存在，立即重新发 `claimAudioRestrictedSlot({ "targetPeerId": "peer-b" })`。
- 重 claim 成功后，不需要重新 produce；原来的 audio producer 还在，服务端会重新给重进的受限端创建 audio consumer。
- 如果重 claim 失败，例如目标已被其他普通端占用，清空本地打开状态并提示业务重新选择或重新打开。

当前 public demo 已按这个逻辑验证：受限端关闭页面后用同一个 `peerId` 重新加入，普通端会自动再发一次 `claimAudioRestrictedSlot`，重进后的受限端会重新收到 audio consumer。

## 8. 最小接入代码参考

下面是业务端最小代码骨架，只保留和音频受限相关的必做逻辑。`createWebRtcTransport`、`connectWebRtcTransport`、`produce`、`newConsumer` 的完整 mediasoup-client 参数仍按现有客户端流程处理。

### 8.1 通用 WebSocket request

```js
let requestId = 0;
const pendingRequests = new Map();

function sendRequest(method, data = {}) {
  const id = ++requestId;
  ws.send(JSON.stringify({
    request: true,
    id,
    method,
    data,
  }));

  return new Promise((resolve, reject) => {
    const timer = setTimeout(() => {
      pendingRequests.delete(id);
      reject(new Error(`request timeout: ${method}`));
    }, 10000);

    pendingRequests.set(id, {
      resolve: responseData => {
        clearTimeout(timer);
        resolve(responseData);
      },
      reject: error => {
        clearTimeout(timer);
        reject(error);
      },
    });
  });
}

ws.onmessage = event => {
  const message = JSON.parse(event.data);

  if (message.response === true) {
    const pending = pendingRequests.get(message.id);
    if (!pending) return;
    pendingRequests.delete(message.id);

    if (message.ok) {
      pending.resolve(message.data || {});
    } else {
      const error = new Error(message.error || message.reason || 'request failed');
      error.reason = message.reason || message.data?.reason || '';
      error.data = message.data || {};
      pending.reject(error);
    }
    return;
  }

  if (message.notification === true) {
    handleNotification(message.method, message.data || {});
  }
};
```

### 8.2 入会和维护受限端列表

```js
const self = {
  peerId: 'peer-a',
  displayName: '普通端A',
  audioRole: 'normal', // 受限端传 'audio-restricted'
};

const audioRestrictedPeers = new Map(); // peerId -> { peerId, displayName }
let openedAudioTargetPeerId = '';
let audioProducer = null;
let sendTransport = null;
let recvTransport = null;

async function joinRoom(roomId, rtpCapabilities) {
  const joinData = await sendRequest('join', {
    roomId,
    peerId: self.peerId,
    displayName: self.displayName,
    audioRole: self.audioRole,
    rtpCapabilities,
  });

  for (const peer of joinData.participants || []) {
    upsertAudioRestrictedPeer(peer);
  }

  return joinData;
}

function upsertAudioRestrictedPeer(peer) {
  if (!peer?.peerId || peer.peerId === self.peerId) return;

  if (peer.audioRole === 'audio-restricted') {
    audioRestrictedPeers.set(peer.peerId, {
      peerId: peer.peerId,
      displayName: peer.displayName || peer.peerId,
    });
  } else {
    audioRestrictedPeers.delete(peer.peerId);
  }

  renderAudioRestrictedPeerSelect();
}

function removeAudioRestrictedPeer(peerId) {
  audioRestrictedPeers.delete(peerId);
  renderAudioRestrictedPeerSelect();
}

function handleNotification(method, data) {
  if (method === 'peerJoined') {
    upsertAudioRestrictedPeer(data);

    if (
      data.peerId === openedAudioTargetPeerId &&
      data.audioRole === 'audio-restricted' &&
      audioProducer
    ) {
      void claimAudioRestrictedSlotAgain(data.peerId);
    }
    return;
  }

  if (method === 'peerLeft') {
    removeAudioRestrictedPeer(data.peerId);
    return;
  }

  if (method === 'newConsumer') {
    void handleConsumerParams(data);
    return;
  }

  if (method === 'producerLeft') {
    handleProducerLeft(data);
    return;
  }

  if (method === 'consumerClosed') {
    closeConsumerById(data.consumerId);
  }
}
```

### 8.3 普通端打开音频

```js
async function openAudioForRestrictedPeer(targetPeerId) {
  if (!targetPeerId) throw new Error('请选择音频受限端');
  if (openedAudioTargetPeerId && openedAudioTargetPeerId !== targetPeerId) {
    throw new Error('当前已打开其他受限端，请先关闭');
  }

  let claimed = false;
  try {
    const claimResp = await sendRequest('claimAudioRestrictedSlot', {
      targetPeerId,
    });

    if (claimResp.required === true && claimResp.claimed !== true && claimResp.alreadyOwned !== true) {
      throw new Error(claimResp.reason || 'claim failed');
    }

    claimed = claimResp.required === true;
    openedAudioTargetPeerId = targetPeerId;
    audioProducer = await ensureAudioProducer();
  } catch (error) {
    if (claimed && !audioProducer) {
      await sendRequest('releaseAudioRestrictedSlot', { targetPeerId }).catch(() => {});
    }
    openedAudioTargetPeerId = '';
    throw error;
  }
}

async function claimAudioRestrictedSlotAgain(targetPeerId) {
  try {
    const claimResp = await sendRequest('claimAudioRestrictedSlot', {
      targetPeerId,
    });
    if (claimResp.required === true && claimResp.claimed !== true && claimResp.alreadyOwned !== true) {
      openedAudioTargetPeerId = '';
    }
  } catch {
    openedAudioTargetPeerId = '';
  }
}

async function ensureAudioProducer() {
  if (audioProducer) return audioProducer;

  const stream = await navigator.mediaDevices.getUserMedia({
    audio: true,
    video: false,
  });
  const track = stream.getAudioTracks()[0];
  if (!track) {
    stream.getTracks().forEach(item => item.stop());
    throw new Error('no microphone track');
  }

  const transport = await ensureSendTransport();
  const producer = await transport.produce({
    track,
    appData: { source: 'audio' },
  });

  producer.on('trackended', () => {
    void closeAudioForRestrictedPeer();
  });
  producer.on('transportclose', () => {
    audioProducer = null;
  });

  audioProducer = producer;
  return producer;
}

async function ensureSendTransport() {
  if (sendTransport) return sendTransport;

  const params = await sendRequest('createWebRtcTransport', {
    producing: true,
    consuming: false,
  });

  sendTransport = device.createSendTransport(params);

  sendTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      await sendRequest('connectWebRtcTransport', {
        transportId: sendTransport.id,
        dtlsParameters,
      });
      callback();
    } catch (error) {
      errback(error);
    }
  });

  sendTransport.on('produce', async ({ kind, rtpParameters, appData }, callback, errback) => {
    try {
      const resp = await sendRequest('produce', {
        transportId: sendTransport.id,
        kind,
        rtpParameters,
        appData,
      });
      callback({ id: resp.id });
    } catch (error) {
      errback(error);
    }
  });

  return sendTransport;
}
```

### 8.4 普通端关闭音频

当前 demo 关闭语义是 `closeProducer` 后再 `releaseAudioRestrictedSlot`。

```js
async function closeAudioForRestrictedPeer() {
  const targetPeerId = openedAudioTargetPeerId;
  const producer = audioProducer;

  try {
    if (producer) {
      await sendRequest('closeProducer', {
        producerId: producer.id,
      });
      producer.close();
      producer.track?.stop?.();
      audioProducer = null;
    }
  } finally {
    if (targetPeerId) {
      await sendRequest('releaseAudioRestrictedSlot', {
        targetPeerId,
      }).catch(() => {});
    }
    openedAudioTargetPeerId = '';
  }
}
```

### 8.5 受限端接收和清理音频

```js
const consumers = new Map(); // consumerId -> { consumer, producerId, element }
const consumersByProducerId = new Map(); // producerId -> Set<consumerId>

async function handleConsumerParams(data) {
  const consumer = await recvTransport.consume({
    id: data.id,
    producerId: data.producerId,
    kind: data.kind,
    rtpParameters: data.rtpParameters,
    appData: data.appData || {},
  });

  let element = null;

  if (consumer.kind === 'audio' || data.appData?.source === 'audio') {
    element = document.createElement('audio');
    element.autoplay = true;
    element.controls = true;
    element.srcObject = new MediaStream([consumer.track]);
    document.body.appendChild(element);
    await element.play().catch(() => {});
  } else {
    element = document.createElement('video');
    element.autoplay = true;
    element.playsInline = true;
    element.srcObject = new MediaStream([consumer.track]);
    document.body.appendChild(element);
    await element.play().catch(() => {});
  }

  consumers.set(consumer.id, {
    consumer,
    producerId: data.producerId,
    element,
  });

  if (data.producerId) {
    const ids = consumersByProducerId.get(data.producerId) || new Set();
    ids.add(consumer.id);
    consumersByProducerId.set(data.producerId, ids);
  }

  consumer.on('producerclose', () => closeConsumerById(consumer.id));
  consumer.on('transportclose', () => closeConsumerById(consumer.id));
}

async function createRecvTransportAndConsumeExisting() {
  const resp = await sendRequest('createWebRtcTransport', {
    producing: false,
    consuming: true,
    rtpCapabilities: device.rtpCapabilities,
  });

  recvTransport = device.createRecvTransport(resp);
  recvTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      await sendRequest('connectWebRtcTransport', {
        transportId: recvTransport.id,
        dtlsParameters,
      });
      callback();
    } catch (error) {
      errback(error);
    }
  });

  for (const consumerParams of resp.consumers || []) {
    await handleConsumerParams(consumerParams);
  }
}

function closeConsumerById(consumerId) {
  const entry = consumers.get(consumerId);
  if (!entry) return;

  consumers.delete(consumerId);
  if (entry.producerId) {
    const ids = consumersByProducerId.get(entry.producerId);
    ids?.delete(consumerId);
    if (!ids || ids.size === 0) {
      consumersByProducerId.delete(entry.producerId);
    }
  }

  entry.consumer.close();

  if (entry.element) {
    entry.element.pause();
    entry.element.srcObject = null;
    entry.element.remove();
  }
}

function closeConsumersByIds(consumerIds) {
  for (const consumerId of consumerIds) {
    closeConsumerById(consumerId);
  }
}

function closeConsumersByProducerId(producerId) {
  const ids = Array.from(consumersByProducerId.get(producerId) || []);
  for (const consumerId of ids) {
    closeConsumerById(consumerId);
  }
}

function handleProducerLeft(data) {
  closeConsumersByIds(data.consumerIds || []);
  if (data.producerId) {
    closeConsumersByProducerId(data.producerId);
  }
}
```

业务接入时按上面的顺序实现即可：`join` 维护受限端列表，打开按钮调用 `openAudioForRestrictedPeer(selectedPeerId)`，关闭按钮调用 `closeAudioForRestrictedPeer()`，受限端继续复用原来的 `newConsumer` / `consumers[]` 消费逻辑。
