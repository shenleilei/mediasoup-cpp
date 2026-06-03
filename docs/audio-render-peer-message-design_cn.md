# 音频受限端最小接入方案

## 结论

该功能第一版做成纯服务端授权方案，端上只需要在 join 时声明角色，发送端在需要给某个音频受限端下发音频前先申请占位。

第一版只考虑 `WebRtcTransport`，不使用 audio consumer pause/resume，不做运行中切换角色，不做排队，不做抢占。

核心规则：

- 普通端不受影响，所有音频照常转发。
- 音频受限端默认不接收任何发送端的 audio consumer。
- 发送端必须先对某个音频受限端 `claimAudioRestrictedSlot` 成功，服务端才允许把该发送端的 audio producer 转发给该受限端。
- 某个音频受限端同一时间只能被一个发送端占位。
- 已经被其他发送端占位后，不能再被抢占。
- 同一个发送端可以同时占位多个音频受限端，限制只作用在每个目标端自己的 slot 上。
- 取消占位后，服务端关闭旧 owner 到该受限端的 audio consumer，其他发送端才可以重新申请。

## 术语

- `normal peer`：普通端，默认角色，音频转发不受占位影响。
- `audio-restricted peer`：音频受限端，只允许当前 owner 的音频进入。
- `slot`：某个音频受限端的音频入口占位，业务状态，不是 mediasoup 对象。
- `owner`：当前占住某个音频受限端 slot 的发送端。

slot 是按目标端维护的：

```text
audioRestrictedSlots[targetPeerId] = ownerPeerId
```

不是 sender 的全局权限。A 占住 B，只表示 A 的音频可以下发给 B，不影响 A 给普通端下发，也不影响其他发送端给普通端下发。

## 客户端接入

客户端最小接入单独见 [audio-render-client-minimal-access_cn.md](./audio-render-client-minimal-access_cn.md)。

本设计文档只保留服务端授权语义和信令定义。

## 信令设计

信令 `id` 使用当前 WebSocket 协议里的数字请求流水号，由客户端生成，服务端响应时原样带回。它只用于匹配 request 和 response，不是业务 id，不参与占位判断。

### join

新增字段：

```text
audioRole: normal | audio-restricted
```

规则：

- 默认值是 `normal`。
- 非法值 join 失败。
- 第一版只在 join 时确定角色，不支持运行中修改。

### claimAudioRestrictedSlot

请求：

```json
{
  "request": true,
  "id": 1001,
  "method": "claimAudioRestrictedSlot",
  "data": {
    "targetPeerId": "peer-b"
  }
}
```

成功，占位到音频受限端：

```json
{
  "response": true,
  "id": 1001,
  "ok": true,
  "data": {
    "required": true,
    "claimed": true,
    "targetPeerId": "peer-b",
    "ownerPeerId": "peer-a"
  }
}
```

成功，目标端是普通端，不需要占位：

```json
{
  "response": true,
  "id": 1001,
  "ok": true,
  "data": {
    "required": false,
    "claimed": false,
    "reason": "not-required",
    "targetPeerId": "peer-normal"
  }
}
```

失败，已经被其他发送端占位：

```json
{
  "response": true,
  "id": 1001,
  "ok": false,
  "error": "audio slot occupied",
  "data": {
    "reason": "occupied",
    "targetPeerId": "peer-b",
    "ownerPeerId": "peer-a"
  }
}
```

失败，目标端不存在：

```json
{
  "response": true,
  "id": 1001,
  "ok": false,
  "error": "target peer not found",
  "data": {
    "reason": "target-not-found",
    "targetPeerId": "peer-b"
  }
}
```

同一个 owner 重复 claim 同一个 target，按幂等成功处理。

### releaseAudioRestrictedSlot

请求：

```json
{
  "request": true,
  "id": 1002,
  "method": "releaseAudioRestrictedSlot",
  "data": {
    "targetPeerId": "peer-b"
  }
}
```

成功释放：

```json
{
  "response": true,
  "id": 1002,
  "ok": true,
  "data": {
    "released": true,
    "targetPeerId": "peer-b"
  }
}
```

没有占位，按幂等成功处理：

```json
{
  "response": true,
  "id": 1002,
  "ok": true,
  "data": {
    "released": false,
    "reason": "not-claimed",
    "targetPeerId": "peer-b"
  }
}
```

非 owner 释放失败：

```json
{
  "response": true,
  "id": 1002,
  "ok": false,
  "error": "not audio slot owner",
  "data": {
    "reason": "not-owner",
    "targetPeerId": "peer-b",
    "ownerPeerId": "peer-a"
  }
}
```

## 服务端必须实现的过滤点

授权函数只需要一个：

```text
CanConsumeProducer(targetPeerId, producerId):
  producer = producers[producerId]
  producerPeerId = producer.ownerPeerId

  if producer.kind != "audio":
    return true

  target = peers[targetPeerId]
  if target.audioRole == "normal":
    return true

  ownerPeerId = audioRestrictedSlots[targetPeerId]
  return ownerPeerId == producerPeerId
```

WebRtcTransport 相关的创建 consumer 入口都必须先调用它：

- recv `WebRtcTransport` 创建时，如果服务端会预创建已有 producer 的 consumer，必须过滤。
- 新 audio producer 出现时，自动订阅其他 WebRTC peer 前必须过滤。
- WebRTC 端主动 `consume(producerId)` 时必须过滤。
- join 返回 `existingProducers` 时也要过滤，避免受限端拿到未授权 audio producerId 后再主动 consume。

未授权 audio 的处理方式：

- 不创建 consumer。
- 不写入 peer consumer 列表。
- 不发送 `newConsumer`。
- 主动 `consume` 返回 `audio consumer not authorized`。

## 关键时序

### claim 先于 produce

```text
B join(audioRole=audio-restricted)
A claimAudioRestrictedSlot(B) -> ok
A produce audio
server 判断 A 是 B 的 owner
server 创建 A -> B audio consumer
server 给 B 发送 newConsumer
```

### produce 先于 claim

```text
B join(audioRole=audio-restricted)
A produce audio
server 不给 B 创建 audio consumer
A claimAudioRestrictedSlot(B) -> ok
server 如果 B 已有 recv WebRtcTransport，则补创建 A -> B audio consumer
server 给 B 发送 newConsumer
```

### B 晚创建 recv transport

```text
B join(audioRole=audio-restricted)
A claimAudioRestrictedSlot(B) -> ok
A produce audio
B create recv WebRtcTransport
server 在预创建 consumers 时允许 A audio
create transport response 返回 A audio consumer
```

### 被其他发送端占位

```text
B join(audioRole=audio-restricted)
A claimAudioRestrictedSlot(B) -> ok
C claimAudioRestrictedSlot(B) -> occupied
slot owner 仍然是 A
C 的 audio 不会下发给 B
```

### 释放后重新申请

```text
A claimAudioRestrictedSlot(B) -> ok
A audio 已经下发给 B
A releaseAudioRestrictedSlot(B) -> ok
server 关闭 A -> B audio consumer
C claimAudioRestrictedSlot(B) -> ok
C audio 可以下发给 B
```

## 清理规则

- owner 主动 release：删除 slot，关闭 owner 到 target 的 audio consumer。
- owner 离开房间：删除该 owner 持有的所有 slot，关闭对应 audio consumer。
- owner 重连替换旧 peer：按旧 owner 离开处理，新 peer 需要重新 claim。
- target 离开房间：删除 target 对应 slot，target 的 transport 和 consumers 按现有 peer 清理流程关闭。
- producer close：第一版不自动释放 slot，只关闭该 producer 对应 consumer；owner 后续重建 audio producer 时仍可继续下发给已占位 target。

### Producer close 与 release 的区别

`closeProducer` 是媒体对象生命周期，`releaseAudioRestrictedSlot` 是业务授权生命周期。两者不要混成一个动作。

`closeProducer` 请求可以按 producerId 关闭：

```json
{
  "request": true,
  "id": 1003,
  "method": "closeProducer",
  "data": {
    "producerId": "producer-audio-1"
  }
}
```

也可以按业务 source 关闭：

```json
{
  "request": true,
  "id": 1004,
  "method": "closeProducer",
  "data": {
    "source": "audio"
  }
}
```

成功响应：

```json
{
  "response": true,
  "id": 1004,
  "ok": true,
  "data": {
    "producerId": "producer-audio-1",
    "closedConsumers": 1,
    "notifiedPeers": ["peer-b"]
  }
}
```

`closeProducer` 做这些事：

- 关闭 owner 的指定 producer。
- 关闭该 producer 派生到其他 peer 的 consumers。
- 给受影响订阅端发送 `producerLeft`。
- 清理 producer 相关 QoS owner / demand 缓存。
- 从 router 移除 producer。
- 通过 producer close hook 清掉 peer producer map。
- 不删除 `audioRestrictedSlots[targetPeerId]`。

`producerLeft` 通知示例：

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

订阅端处理要求：

- 优先按 `consumerIds` 移除远端 consumer。
- 同时可以按 `producerId` 做兜底清理，避免历史卡片残留。
- 不需要再主动 `consume` 这个 producer；关闭后再次 consume 会返回 `producer not found`。

`releaseAudioRestrictedSlot` 做这些事：

- 删除 `audioRestrictedSlots[targetPeerId]`。
- 关闭当前 owner 到 target 的 audio consumers。
- 给 target 发送 `consumerClosed(reason=audio-slot-release)`。
- 不关闭 owner 的 producer。

因此业务上的“关闭受限端音频”推荐组合顺序是：

```text
closeProducer(source=audio 或 producerId)
releaseAudioRestrictedSlot(targetPeerId)
```

如果先 `closeProducer`，目标端通常通过 `producerLeft` 清理远端音频卡片，后续 release 可能没有 consumer 可关。如果只 release，producer 仍存在，只是不再下发给该受限端。

`closeProducer` 异常规则：

- `permission denied`：请求方不是 producer owner，不能关闭别人的 producer。
- `producer not found`：producer 已不存在，或者 source 没匹配到 producer。
- `missing producerId or source`：请求没有携带 selector。
- `ambiguous producer source`：同一个 peer 有多路 producer 使用相同 `appData.source`，应改用 `producerId`。
- `closedConsumers=0`：producer 可以被关闭，只是当前没有订阅者。

### public demo 约束

当前 public QoS demo 是最小验证实现：

- 只考虑 `WebRtcTransport`。
- 只支持一个已打开的受限端。
- 只维护一个 audio producer，`appData.source=audio`。
- 普通端下拉框只列 `audioRole=audio-restricted` 的其他 peer。
- 受限端页面会排除自己，所以不会在下拉框里看到自己。
- 下拉框 label 使用目标 peer 身份：优先 `displayName (peerId)`，缺失时兜底 `peerId`。
- 下拉框不使用 `appData.source`；`source` 是 producer/consumer 的媒体来源标识，不是目标端身份。
- 打开动作是 `claimAudioRestrictedSlot` 后 `produce audio`。
- 关闭动作是 `closeProducer(source=audio)` 后 `releaseAudioRestrictedSlot`。

## 异常情况

- 目标端是普通端：claim 返回 `required=false`，普通音频转发不受影响。
- 目标端不存在：claim 失败，返回 `target-not-found`。
- 已被其他 owner 占位：claim 失败，返回 `occupied`，不能覆盖原 owner。
- claim 成功但 target 还没创建 recv transport：只保存 slot，target 后续创建 recv transport 时再创建 consumer。
- claim 成功但 owner 还没有 audio producer：只保存 slot，owner 后续 produce audio 时再创建 consumer。
- release 时 slot 不存在：幂等成功，返回 `released=false`。
- release 时请求方不是 owner：失败，返回 `not-owner`。
- 服务端重启：slot 丢失，端上重连后按业务重新 claim。
- 同房间服务端串行处理：claim 和 release 按消息到达顺序生效，不需要客户端等待 B 再回 ACK 给 A。

## 最小验收用例

- 普通端不传 `audioRole`，仍能收到所有 audio。
- 音频受限端不被 claim 时，收不到任何 sender 的 audio consumer。
- A claim B 成功后，B 能收到 A 的 audio consumer。
- A produce 早于 claim，claim 成功后 B 能收到补发的 `newConsumer`。
- B 创建 recv transport 晚于 claim，transport 响应里能包含 A 的 audio consumer。
- A 已占住 B 后，C claim B 返回 `occupied`，B 收不到 C audio。
- A release B 后，服务端关闭/移除 B 上 A 的 audio consumer，C 可以 claim B。
- B 主动 consume 未授权 audio producer 时，服务端拒绝且不创建 consumer。
- public demo 中，普通端选择受限端并打开后，受限端出现 audio card、consumerId/producerId 和音量文本。
- public demo 中，普通端关闭后，受限端 audio card、audio element 和 remoteAudioConsumers 都清零。
- public demo 中，受限端 URL 带 `displayName=受限端A` 后，普通端下拉框显示 `受限端A (peer-...)`，状态文案也使用同一 label。
- `closeProducer` 按 source 关闭 audio producer 后，订阅端收到 `producerLeft`，consumer 被移除，关闭后不能再 consume。
- 非 owner 调用 `closeProducer` 返回 `permission denied`。
- 同 source 多 producer 调用 `closeProducer(source)` 返回 `ambiguous producer source`，改按 producerId 关闭成功。
