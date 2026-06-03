# 音频受限端客户端最小接入

本文只写客户端要发什么、会收到什么、收到后做什么。第一版只考虑 `WebRtcTransport`。

## 1. join 声明角色

受限端发：

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

普通端发：

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

客户端收：

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
        "peerId": "peer-a",
        "displayName": "普通端A",
        "audioRole": "normal",
        "producers": []
      },
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

- 正常 `device.load(routerRtpCapabilities)`。
- 正常创建 recv `WebRtcTransport`。
- 正常处理 `existingProducers`、transport 响应里的 `consumers`、后续 `newConsumer`。
- 受限端没收到 audio consumer 是正常状态，不是连接失败。

## 2. 维护可选受限端列表

普通端打开某个受限端音频前，先从 `join` 响应里的 `data.participants` 建立可选列表。

客户端行为：

- `participants` 包含当前自己，必须排除 `selfPeerId`。
- 每个 peer 至少使用 `peerId`、`displayName`、`audioRole`。
- 只保留 `audioRole="audio-restricted"` 且 `peerId != selfPeerId` 的 peer。
- 下拉框显示 `displayName (peerId)`。
- 下拉值使用 `peerId`，后续作为 `targetPeerId`。
- 不从 producer `source` 获取目标端；`source=audio` 只表示媒体来源。

新 peer 加入时，客户端收：

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

客户端行为：

- 如果 `audioRole="audio-restricted"` 且不是自己，就加入可选列表。
- 如果该 `peerId` 是当前已打开目标，且本端仍在发布 audio，可以重新 claim。

peer 离开时，客户端收：

```json
{
  "notification": true,
  "method": "peerLeft",
  "data": {
    "peerId": "peer-d"
  }
}
```

客户端行为：

- 从可选列表删除该 `peerId`。
- 当前 demo 会保留已打开目标状态，用于同 `peerId` 重连后重新 claim。
- 业务如果不需要重连保持，也可以在 `peerLeft` 时清空本地打开状态。

## 3. 打开受限端音频

用户选择受限端后，客户端用选中项的 `peerId` 发 claim。

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

占位成功时，客户端收：

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

客户端行为：

- 记录本地打开状态，例如 `claimedAudioTargetPeerId = "peer-b"`。
- 然后发布 audio producer，`produce.appData.source = "audio"`。
- `alreadyOwned=true` 表示重复 claim，同样按成功处理。
- `consumersCreated` 只用于观测，不需要客户端据此播放。

目标是普通端时，客户端收：

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

客户端行为：

- 不记录占位。
- 按普通发流逻辑继续。

已被其他发送端占用时，客户端收：

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

- 不抢占。
- 不记录本地打开状态。
- UI 提示目标已被占用。

目标不存在时，客户端收：

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

客户端行为：不记录本地打开状态，按业务决定是否等待目标重新加入。

## 4. 关闭受限端音频

当前 demo 只维护一个 audio producer。关闭时先关 producer，再 release slot。

### 4.1 closeProducer

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
- 停止本地 audio track。
- 删除本地 producer 记录。

常见失败：

- `missing producerId or source`：请求没有带 selector。
- `missing source`：按 source 关闭但 source 为空。
- `permission denied`：不是 producer owner。
- `producer not found`：producer 不存在，或 source 未匹配。
- `ambiguous producer source`：同 source 多 producer，改用 `producerId`。

### 4.2 releaseAudioRestrictedSlot

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

释放成功时，客户端收：

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

客户端行为：

- release 返回后清空本地打开状态。
- `not-claimed`、`not-required` 或 `not-owner` 也清空本地状态。
- `closedConsumers` 是本次 release 实际关闭的 consumer 数，可能是 0 或更多。

## 5. 接收端通知

远端 producer 被关闭时，客户端收：

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

slot release 只关闭 consumer 时，客户端收：

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
- 不要自动重试 consume，除非业务重新 claim 成功。

## 6. public demo

受限端打开方式：

```text
https://volcvideo3.zelostech.com.cn:1770/?audioRole=audio-restricted&displayName=受限端A
```

普通端打开方式：

```text
https://volcvideo3.zelostech.com.cn:1770/?displayName=普通端A
```

页面行为：

- 普通端下拉框只显示同房间受限端。
- 下拉框显示 `displayName (peerId)`。
- 下拉框不显示 `source`；`source=audio` 只表示 producer/consumer 的媒体来源。
- 当前 demo 只支持一个 audio producer 和一个已打开目标端。

## 7. 请求 id

`id` 是客户端数字流水号，只用于匹配 request/response，不是业务 id。
