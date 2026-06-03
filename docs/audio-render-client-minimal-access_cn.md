# 音频受限端客户端最小接入

本文按人的实际使用顺序写：先打开受限端，再打开普通端，普通端选择受限端并打开音频，最后关闭音频。第一版只考虑 `WebRtcTransport`。

示例里的 `id` 是客户端数字流水号，只用于匹配 request/response，不是业务 id。

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
    "consumersCreated": 1
  }
}
```

普通端行为：

- 记录本地打开状态，例如 `claimedAudioTargetPeerId = "peer-b"`。
- 然后发布 audio producer，`produce.appData.source = "audio"`。
- `alreadyOwned=true` 表示重复 claim，同样按成功处理。
- `consumersCreated` 只用于观测，不需要客户端据此播放。

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

普通端 claim 成功并 produce audio 后，受限端会按现有订阅流程收到 audio consumer。可能出现在两个地方：

- 如果受限端 recv transport 已创建，收到 `newConsumer`。
- 如果受限端晚创建 recv transport，audio consumer 可能直接在 `createWebRtcTransport` 响应的 `consumers` 里返回。

受限端行为：

- 按现有 `newConsumer` / `consumers` 逻辑创建 consumer 和 audio DOM。
- 不需要额外判断授权；服务端只会下发已授权 audio consumer。

## 6. 普通端关闭受限端音频

当前 demo 只维护一个 audio producer。关闭时先关 producer，再 release slot。

### 6.1 closeProducer

普通端发：

```json
{
  "request": true,
  "id": 3,
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
  "id": 3,
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
  "id": 3,
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
  "id": 4,
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
  "id": 4,
  "ok": true,
  "data": {
    "released": true,
    "targetPeerId": "peer-b",
    "closedConsumers": 1
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
- `closedConsumers` 是本次 release 实际关闭的 consumer 数，可能是 0 或更多。

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
