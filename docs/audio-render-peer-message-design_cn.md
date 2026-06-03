# 音频受限端信令速查

本文只列客户端要用到的信令字段、响应字段和处理动作。

## 1. join

客户端发：

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

客户端收：

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

客户端行为：

- `audioRole=audio-restricted`：按受限端接入。
- `audioRole=normal` 或缺省：按普通端接入。
- `participants` 包含当前自己；客户端维护目标端列表时要排除自己。
- 后续 transport / consumer 处理不需要额外授权判断。

## 2. claimAudioRestrictedSlot

客户端发：

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

客户端可能收到：

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

客户端行为：

- `required=true, claimed=true`：记录 target 已打开，然后 produce audio。
- `alreadyOwned=true`：本端重复 claim，同样按成功处理。
- `consumersCreated`：claim 时服务端补发的 audio consumer 数，只用于观测，不需要客户端据此播放。
- `required=false`：目标是普通端，不记录占位，继续普通发流。
- `reason=occupied`：不抢占，提示失败。
- `reason=target-not-found`：不记录占位。

## 3. closeProducer

客户端发：

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

也可以发：

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

客户端收：

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

客户端行为：

- 本地 `producer.close()`。
- 停止对应 track。
- 删除本地 producer 记录。
- 如果是受限端关闭流程，继续 release slot。

失败处理：

- `missing producerId or source`：请求没有带 selector。
- `missing source`：按 source 关闭但 source 为空。
- `permission denied`：不是 producer owner。
- `producer not found`：producer 不存在，或 source 未匹配。
- `ambiguous producer source`：同 source 多 producer，改用 `producerId`。

## 4. releaseAudioRestrictedSlot

客户端发：

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

客户端收：

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

客户端行为：

- 清空本地 target 打开状态。
- `closedConsumers` 是本次 release 实际关闭的 consumer 数，可能是 0 或更多。
- `released=false` 也清空。
- `reason=not-owner` 也清空。

## 5. producerLeft

客户端收：

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

客户端行为：

- 按 `consumerIds` 删除 consumer 和 DOM。
- 按 `producerId` 兜底清理。
- 不要当成 `peerLeft`。

## 6. consumerClosed

客户端收：

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

客户端行为：

- 按 `consumerId` 删除 consumer 和 DOM。
- 不要自动重试 consume。

## 7. 最小关闭顺序

```text
closeProducer(source=audio 或 producerId)
本地 producer.close()
停止本地 audio track
releaseAudioRestrictedSlot(targetPeerId)
清空本地打开状态
```

## 8. 请求 id

`id` 是客户端数字流水号，只用于匹配 request/response，不是业务 id。
