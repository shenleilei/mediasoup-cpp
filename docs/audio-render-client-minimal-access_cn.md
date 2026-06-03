# 音频受限端客户端最小接入

本文只写客户端接入动作：发什么、收什么、收到后做什么。字段示例见 [audio-render-peer-message-design_cn.md](./audio-render-peer-message-design_cn.md)。

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

客户端行为：

- 原有收发流流程不变。
- 普通端音频转发不需要 claim。

## 3. 打开某个受限端音频

客户端发：

- `claimAudioRestrictedSlot`
- `data.targetPeerId = "<受限端 peerId>"`

客户端收：

- 成功占位：`ok=true, data.required=true, data.claimed=true`
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
- 失败：`permission denied` / `producer not found` / `ambiguous producer source`

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
- 不是 owner：`ok=false, data.reason="not-owner"`

客户端行为：

- release 返回后清空本地打开状态。
- `not-claimed` 或 `not-owner` 也清空本地状态。

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
