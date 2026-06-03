# 音频受限端客户端最小接入

本文只写客户端接入动作：发什么、收什么、收到后做什么。第一版只考虑 `WebRtcTransport`。

## 1. 受限端入会

客户端发：

- `join`
- `data.audioRole = "audio-restricted"`
- `data.displayName = "<可读名称>"`

客户端收：

- `join` 成功响应
- `data.audioRole = "audio-restricted"`
- `data.routerRtpCapabilities`
- `data.existingProducers`
- `data.participants`

客户端行为：

- 正常 `device.load(routerRtpCapabilities)`。
- 正常创建 recv `WebRtcTransport`。
- 正常处理 `consumers`、`existingProducers`、`newConsumer`。
- 没收到 audio consumer 是正常状态，不是连接失败。

## 2. 普通端入会

客户端发：

- `join`
- 不传 `audioRole`，或传 `data.audioRole = "normal"`
- 可传 `data.displayName = "<可读名称>"`

客户端收：

- `join` 成功响应
- `data.audioRole = "normal"`
- `data.participants`

客户端行为：

- 原有收发流流程不变。
- 普通端音频转发不需要 claim。

## 3. 打开某个受限端音频

发 claim 前，客户端先维护可选受限端列表：

- 初始来源：`join` 成功响应里的 `data.participants`。
- 增量来源：后续 `peerJoined` 通知。
- 删除来源：后续 `peerLeft` 通知。
- `participants` 包含当前自己，所以必须排除 `selfPeerId`。
- 每个 peer 至少使用 `peerId`、`displayName`、`audioRole`。
- 只把 `audioRole = "audio-restricted"` 且 `peerId != selfPeerId` 的 peer 放进可选列表。
- 下拉框显示 `displayName (peerId)`，下拉值使用 `peerId`。
- 不从 producer `source` 获取目标端；`source=audio` 只表示媒体来源。

`join` 响应里的 `participants` 示例：

```json
{
  "response": true,
  "id": 1,
  "ok": true,
  "data": {
    "audioRole": "normal",
    "participants": [
      {
        "peerId": "peer-b",
        "displayName": "受限端A",
        "audioRole": "audio-restricted"
      },
      {
        "peerId": "peer-c",
        "displayName": "普通端C",
        "audioRole": "normal"
      }
    ]
  }
}
```

客户端从上面只保留 `peer-b`，下拉显示 `受限端A (peer-b)`，value 使用 `peer-b`。

后续有新 peer 加入时会收到：

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

客户端行为：如果 `audioRole="audio-restricted"` 且不是自己，就加入可选列表。

peer 离开时会收到：

```json
{
  "notification": true,
  "method": "peerLeft",
  "data": {
    "peerId": "peer-d"
  }
}
```

客户端行为：从可选列表删除 `peer-d`。当前 demo 会保留已打开目标状态；如果同一个 `peerId` 后续 `peerJoined` 且本端仍在发布 audio，会重新 claim。业务如果不需要重连保持，也可以在 `peerLeft` 时清空本地打开状态。

客户端选择某个受限端后，把该项的 `peerId` 作为 `targetPeerId`。

客户端发：

- `claimAudioRestrictedSlot`
- `data.targetPeerId = "<受限端 peerId>"`

客户端收：

- 成功占位：`ok=true, data.required=true, data.claimed=true`
- 重复占位：`ok=true, data.alreadyOwned=true`
- 补发 consumer 数：`data.consumersCreated`
- 目标是普通端：`ok=true, data.required=false, data.reason="not-required"`
- 被占用：`ok=false, data.reason="occupied"`
- 目标不存在：`ok=false, data.reason="target-not-found"`

客户端行为：

- 成功占位后记录本地打开状态，例如 `claimedAudioTargetPeerId = targetPeerId`。
- 成功占位后再发布 audio producer，`produce.appData.source = "audio"`。
- `required=false` 时不记录占位，按普通发流继续。
- `occupied` 时不抢占、不重试，提示业务目标已被占用。
- `target-not-found` 时不记录占位，按业务决定是否等待目标重新加入。

## 4. 关闭某个受限端音频

客户端先发：

- `closeProducer`
- `data.source = "audio"`，或 `data.producerId = "<audio producerId>"`

客户端收：

- 成功：`ok=true, data.producerId, data.closedConsumers, data.notifiedPeers`
- 失败：`missing producerId or source` / `missing source` / `permission denied` / `producer not found` / `ambiguous producer source`

客户端行为：

- 成功后本地 `producer.close()`。
- 停止本地 audio track。
- 删除本地 producer 记录。

客户端再发：

- `releaseAudioRestrictedSlot`
- `data.targetPeerId = "<受限端 peerId>"`

客户端收：

- 成功释放：`ok=true, data.released=true`
- 已无占位：`ok=true, data.released=false, data.reason="not-claimed"`
- 目标是普通端：`ok=true, data.released=false, data.reason="not-required"`
- 不是 owner：`ok=false, data.reason="not-owner"`
- 目标不存在：`ok=false, data.reason="target-not-found"`

客户端行为：

- release 返回后清空本地打开状态。
- `not-claimed`、`not-required` 或 `not-owner` 也清空本地状态。

## 5. 接收端通知

客户端收到：

- `producerLeft`

客户端行为：

- 按 `data.consumerIds` 移除本地 consumer 和对应 DOM。
- 再按 `data.producerId` 做兜底清理。
- 不要当成 peer 离开。

客户端收到：

- `consumerClosed`

客户端行为：

- 按 `data.consumerId` 移除本地 consumer 和对应 DOM。
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

## 8. 信令 JSON 示例

### join

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

### claimAudioRestrictedSlot

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

占位成功：

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

已被占用：

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

### closeProducer

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

### releaseAudioRestrictedSlot

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

### producerLeft

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

客户端行为：按 `consumerIds` 删除 consumer 和 DOM，再按 `producerId` 兜底清理。

### consumerClosed

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

客户端行为：按 `consumerId` 删除 consumer 和 DOM，不要自动重试 consume。
