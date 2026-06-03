# 音频受限端客户端最小接入

## 适用范围

本文只说明客户端如何最小接入“音频受限端”能力。

第一版只考虑 `WebRtcTransport`：

- 接收端 join 时声明本端是否是音频受限端。
- 发送端在需要把自己的音频送到某个音频受限端前，先申请占位。
- 发送端在业务结束时释放占位。

客户端不需要做这些事：

- 不需要自己判断哪些 audio producer 可以消费。
- 不需要修改 mediasoup-client 的 `consume()` / `newConsumer` 基础处理。
- 不需要等待目标端回 ACK 后再认为占位成功。
- 不需要通过 audio consumer pause/resume 控制开关。

当前 QoS demo 最小接入参数：

- `audioRole=audio-restricted`：本端作为音频受限接收端 join。
- `displayName=<name>`：用于在普通端下拉框和远端 QoS 卡片上显示可读名称；URL 参数名必须是 `displayName`。
- `audioTargetPeerId=<peerId>`：本端发布 audio 前先 claim 指定目标端；当前 public demo 只取第一个目标端。

不传这些参数时，demo 保持原行为，`audioRole` 默认为 `normal`，发布音频前不会自动 claim。

相关文档：

- 服务端详细设计见 [audio-render-peer-message-design_cn.md](./audio-render-peer-message-design_cn.md)。

## 接收端：join 声明角色

普通端可以不传 `audioRole`。如果显式传，使用：

```json
{
  "method": "join",
  "data": {
    "roomId": "room-1",
    "peerId": "peer-normal",
    "audioRole": "normal"
  }
}
```

音频受限端必须在 join 时传：

```json
{
  "method": "join",
  "data": {
    "roomId": "room-1",
    "peerId": "peer-b",
    "audioRole": "audio-restricted"
  }
}
```

字段规则：

```text
audioRole = normal | audio-restricted
```

- 默认值是 `normal`。
- `audio-restricted` 表示本端默认不接收任何发送端的 audio consumer。
- 角色只在 join 时声明，第一版不支持运行中修改。

## 接收端：订阅流程保持不变

- 仍然正常创建 recv `WebRtcTransport`。
- 仍然正常处理服务端返回的 `consumers`。
- 仍然正常处理 `newConsumer` 通知。
- 仍然正常兼容 `existingProducers` 主动 consume 逻辑。
- 受限端不会收到未授权 audio consumer，因为服务端不会创建和下发。
- 受限端没收到 audio 时，不应该当成 WebRTC 连接失败。

推荐流程：

```text
join(audioRole)
device.load(routerRtpCapabilities)
createWebRtcTransport(producing=false, consuming=true, rtpCapabilities)
处理 createWebRtcTransport 响应里的 consumers
处理 join 响应里的 existingProducers 中尚未消费的 producer
持续处理 newConsumer
```

最小伪代码：

```js
const joinResp = await wsRequest('join', {
  roomId,
  peerId,
  audioRole: isAudioRestrictedDevice ? 'audio-restricted' : 'normal'
});

const device = new mediasoupClient.Device();
await device.load({
  routerRtpCapabilities: joinResp.routerRtpCapabilities
});

const recvData = await wsRequest('createWebRtcTransport', {
  producing: false,
  consuming: true,
  rtpCapabilities: device.rtpCapabilities
});

const recvTransport = device.createRecvTransport(recvData);

for (const consumerData of recvData.consumers || []) {
  await handleNewConsumer(consumerData);
}

const consumedProducerIds = new Set(
  (recvData.consumers || []).map(item => item.producerId)
);

for (const producer of joinResp.existingProducers || []) {
  if (!consumedProducerIds.has(producer.producerId)) {
    await consumeProducer(producer.producerId);
  }
}

onNotification('newConsumer', handleNewConsumer);
```

`handleNewConsumer()` 不需要为音频受限端单独加授权判断。如果服务端下发了 audio consumer，端上按现有音频渲染逻辑处理；如果没下发，就不渲染。

## 发送端：申请占位

发送端在业务上想让自己的音频进入某个目标端前，调用 `claimAudioRestrictedSlot`。
同一个发送端可以对多个目标端分别调用 `claimAudioRestrictedSlot` / `releaseAudioRestrictedSlot`。

public QoS demo 为了验证简单，只支持一次打开一个受限端：

- 下拉框只展示同房间内 `audioRole=audio-restricted` 的其他 peer。
- 普通端不会显示在下拉框里，因为普通端音频不需要 claim。
- 受限端页面会排除自己，所以通常显示“暂无音频受限端”。
- demo 只维护一个 `claimedAudioTargetPeerId` 和一个 audio producer。
- 如果要切换目标端，必须先关闭当前目标端。

推荐时序是先 claim，再 produce audio：

```text
claimAudioRestrictedSlot(targetPeerId)
claim 成功或 required=false
produce audio
```

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

如果发送端已经在发 audio，后续业务才决定要给某个受限端开放音频，也可以先 produce 再 claim：

```text
produce audio
claimAudioRestrictedSlot(targetPeerId)
claim 成功后，服务端给目标端补发 newConsumer
```

发送端本地建议只维护一个最小集合：

```text
claimedAudioTargets: Set<targetPeerId>
```

写入规则：

- `claimAudioRestrictedSlot` 返回 `ok=true` 且 `required=true` 时加入。
- `releaseAudioRestrictedSlot` 返回后删除。
- 收到 `peerLeft(targetPeerId)` 时不自动删除；这个集合表示业务仍希望保持的目标，短暂离线后可用于重新 claim。
- 收到目标端 `peerJoined` 或 `peerJoined(reconnect=true)`，如果该 target 仍在 `claimedAudioTargets` 且本端仍在发布音频，则重新 claim。
- 重新 claim 失败时，按普通 claim 失败处理；如果已确认不再是 owner，应删除本地记录并提示业务。
- WebSocket 断开、服务端重启或重新 join 后清空，按业务重新 claim。

## 发送端：处理 claim 返回

成功，占位生效：

```text
ok=true, required=true
```

端上处理：

- 把 `targetPeerId` 加入 `claimedAudioTargets`。
- 可以开始或继续 produce audio。
- 不需要等待目标端 ACK。

成功，目标端是普通端：

```text
ok=true, required=false, reason=not-required
```

端上处理：

- 不需要记录占位。
- 按原逻辑 produce audio。
- 普通端音频转发不受占位影响。

失败，目标端已被其他发送端占位：

```text
ok=false, reason=occupied
```

端上处理：

- 不要重试抢占。
- 不要把该目标端加入 `claimedAudioTargets`。
- 可以提示业务层“目标端音频已被占用”。
- 本发送端仍可以继续给普通端发送音频。

失败，目标端不存在：

```text
ok=false, reason=target-not-found
```

端上处理：

- 不要记录占位。
- 按业务决定忽略、提示，或等目标端重新 join 后再 claim。

## 发送端：释放占位

发送端不再需要占位时，调用：

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

release 返回后，无论 `released=true` 还是 `released=false`，端上都可以删除本地 `claimedAudioTargets[targetPeerId]`。

如果返回 `reason=not-owner`，说明本端本来就不是 owner，端上也应该删除本地过期状态。

release 成功后，服务端会关闭并移除当前 owner 到 `targetPeerId` 的 audio consumer，停止继续下发这路音频。第一版不新增独立的“停止渲染”点对点信令；如果端上已有 consumer/track 关闭事件处理，按现有逻辑收口即可。

如果业务语义是“关闭当前本地音频并释放受限端”，推荐顺序是：

```text
closeProducer(producerId 或 source=audio)
本地 producer.close()
停止本地 audio track / capture stream
releaseAudioRestrictedSlot(targetPeerId)
```

原因是 `closeProducer` 只关闭媒体对象和订阅端 consumer，不会自动释放 slot。slot 是业务授权状态，发送端后续重建 audio producer 时仍可继续给已占位的受限端下发音频。

订阅端必须处理两种清理通知：

- `producerLeft`：producer 被关闭，按 `consumerIds` 或 `producerId` 移除远端媒体。
- `consumerClosed`：slot release 导致 consumer 被关闭，按 `consumerId` 移除远端媒体。

## 发送端最小伪代码

```js
const claimedAudioTargets = new Set();

async function claimAudioTarget(targetPeerId) {
  const resp = await wsRequest('claimAudioRestrictedSlot', { targetPeerId });

  if (resp.required === true) {
    claimedAudioTargets.add(targetPeerId);
  }

  return resp;
}

async function releaseAudioTarget(targetPeerId) {
  try {
    await wsRequest('releaseAudioRestrictedSlot', { targetPeerId });
  } finally {
    claimedAudioTargets.delete(targetPeerId);
  }
}

async function publishAudioForTarget(targetPeerId, audioTrack) {
  let claimResp;

  try {
    claimResp = await claimAudioTarget(targetPeerId);
  } catch (error) {
    const reason = error.data?.reason || error.reason;

    if (reason === 'occupied') {
      showAudioTargetOccupied(targetPeerId);
      return;
    }

    throw error;
  }

  await ensureAudioProducer(audioTrack);
  return claimResp;
}

onNotification('peerLeft', data => {
  // 不自动删除，保留业务意图，等 peerJoined 后重新 claim。
});

onNotification('peerJoined', data => {
  if (claimedAudioTargets.has(data.peerId) && isPublishingAudio()) {
    void claimAudioTarget(data.peerId).catch(error => {
      claimedAudioTargets.delete(data.peerId);
      showAudioClaimFailed(data.peerId, error);
    });
  }
});

onServerDisconnectedOrRestart(() => {
  claimedAudioTargets.clear();
});
```

## 客户端时序注意事项

- 信令里的 `id` 是客户端生成的数字请求流水号，只用于匹配 request 和 response，不是 `peerId`、`producerId`、`consumerId` 或 slot id。
- `claimAudioRestrictedSlot` 是发送端到服务端的请求，不是目标端收到的一条点对点消息。
- claim response 只表示服务端占位成功，不表示目标端已经播放出声音。
- 目标端是否马上收到 audio，取决于目标端是否已经创建 recv transport，以及发送端是否已经 produce audio。
- 目标端晚创建 recv transport 时，可能直接在 `createWebRtcTransport` 响应的 `consumers` 里拿到 audio consumer，不一定额外收到 `newConsumer`。
- 目标端重连会让服务端清理旧 slot；发送端如果仍要给它开放音频，需要在目标端重新加入后重新 claim。
- 发送端 release 后，目标端 audio consumer 由服务端关闭并停止下发；本方案不要求端上额外发“停止渲染”信令。
- 普通端不需要 claim；如果端上统一 claim，收到 `required=false` 后按原逻辑继续即可。

## public demo 验证

公网 demo：

```text
https://volcvideo3.zelostech.com.cn:1770/
```

受限端浏览器：

```text
https://volcvideo3.zelostech.com.cn:1770/?audioRole=audio-restricted&displayName=受限端
```

普通端浏览器：

```text
https://volcvideo3.zelostech.com.cn:1770/?displayName=普通端1
```

第二个普通端可用于验证不能抢占：

```text
https://volcvideo3.zelostech.com.cn:1770/?displayName=普通端2
```

三个页面必须加入同一个房间号。页面缓存异常时可以加 `cb`：

```text
https://volcvideo3.zelostech.com.cn:1770/?audioRole=audio-restricted&displayName=受限端&cb=20260603
```

### 页面行为

下拉框过滤是有意的：

- 普通端页面的“受限端音频”下拉框只显示同房间里的 `audioRole=audio-restricted` peer。
- 普通端不会显示在这个下拉框里，因为普通端音频转发不需要 claim。
- 受限端页面会排除自己，所以通常显示“暂无音频受限端”。
- 下拉项使用可读 label：`displayName (peerId)`。
- 下拉项选择的是目标端 peer，不是媒体 producer，所以不会使用 `appData.source` 作为显示名；`source=audio` 只用于 producer/consumer 业务来源和 QoS 卡片。
- 如果 `displayName` 缺失或没有传到服务端，下拉项兜底显示 `peerId`。

当前 demo 为了验证简单，只支持一个已打开的受限端：

- 选择一个受限端后点“打开音频”。
- 打开成功后不能直接切换到另一个受限端，必须先点“关闭音频”。
- 当前 demo 只维护一个 `claimedAudioTargetPeerId` 和一个 audio producer。

### 人工验证步骤

1. 打开一个受限端和至少一个普通端，输入同一个房间号后点击“加入房间”。

预期：

- 受限端状态显示已加入。
- 普通端下拉框出现 `受限端 (peer-...)`。
- 受限端下拉框显示“暂无音频受限端”。

2. claim 前确认受限端没有音频。

预期：

- 受限端没有远端音频卡片。
- 受限端 debug snapshot 中 `remoteAudios=0`。
- 受限端不会把“没有音频”当成 WebRTC 连接失败。

3. 普通端打开受限端音频。

操作：

```text
普通端下拉框选择受限端
点击“打开音频”
```

实际顺序：

```text
claimAudioRestrictedSlot(targetPeerId)
createWebRtcTransport(producing=true, consuming=false)
produce(kind=audio, appData.source=audio)
```

预期：

- 普通端日志出现 `Audio target claim result required=true claimed=true`。
- 普通端日志出现 `Producing audio kind=audio`。
- 受限端收到 `newConsumer kind=audio`。
- 受限端出现远端音频卡片。
- 受限端 QoS 区域能看到 consumer id、producer id、Publisher、Subscriber、Source=audio。
- 音频卡片显示音量文本。

音量文本格式：

```text
音量 0.0% · RMS -100.0 dBFS · Peak -100.0 dBFS
```

如果浏览器采集到真实麦克风声音，百分比和 dBFS 会变化。自动化/headless 验证中可能是静音，显示 `0.0% / -100 dBFS` 也表示音频元素和分析器已经建立。

4. 第二个普通端尝试抢占。

操作：

```text
第二个普通端加入同一房间
选择同一个受限端
点击“打开音频”
```

预期：

- 服务端返回 `occupied`。
- 第二个普通端不能抢占。
- 受限端仍只接收第一个普通端的音频。

5. 普通端关闭受限端音频。

操作：

```text
已打开的普通端点击“关闭音频”
```

实际顺序：

```text
closeProducer(source=audio)
producer.close()
停止本地 audio-only capture stream
releaseAudioRestrictedSlot(targetPeerId)
```

预期：

- 普通端日志出现 `Producer stopped on server source=audio`。
- 普通端日志出现 `Audio target release result released=true`。
- 受限端收到 `producerLeft kind=audio`。
- 受限端远端音频卡片消失。
- 受限端 debug snapshot 中 `remoteAudios=0`。

如果业务直接 release 但 producer 仍存在，受限端会收到 `consumerClosed(reason=audio-slot-release)`。demo 的关闭路径因为先 close producer，通常由 `producerLeft` 完成卡片清理。

### 自动化验证结果

2026-06-03 已在公网测试环境验证音频打开/关闭链路，镜像：

```text
mediasoup-cpp:audio-volume-review-20260603_2054
```

验证房间：

```text
audio_ui_1780491400417
```

关键结果：

```json
{
  "before": {
    "audioRole": "audio-restricted",
    "remoteAudios": 0,
    "remoteAudioConsumers": 0
  },
  "restrictedAfterOpen": {
    "remoteAudios": 1,
    "remoteAudioConsumers": 1,
    "volume": "音量 0.0% · RMS -100.0 dBFS · Peak -100.0 dBFS"
  },
  "afterClose": {
    "remoteAudios": 0,
    "remoteAudioConsumers": 0,
    "audioCards": 0,
    "audios": 0
  }
}
```

同时确认：

- 普通端 claim 成功后持有 `claimedAudioTargetPeerId`。
- 受限端收到 `newConsumer kind=audio` 和 `Remote audio track unmuted`。
- 关闭后普通端本地 producer 数为 0。
- 关闭后受限端收到 `producerLeft kind=audio`。
- h1/h2/h3 均已替换到同一测试镜像。

2026-06-03 追加验证 public demo 下拉框 `displayName` 展示，最终镜像：

```text
mediasoup-cpp:audio-target-label-final-20260603_212815
```

验证房间：

```text
audio_label_1780493488898
```

关键结果：

```json
{
  "normalPeer": {
    "displayName": "普通端A",
    "audioRole": "normal",
    "status": "状态： Joined room audio_label_1780493488898"
  },
  "audioRestrictedPeers": [
    {
      "peerId": "peer-46879p",
      "displayName": "受限端A",
      "audioRole": "audio-restricted"
    }
  ],
  "audioTargetOptions": [
    {
      "value": "peer-46879p",
      "text": "受限端A (peer-46879p)"
    }
  ],
  "audioSlotStatus": "已选择 受限端A (peer-46879p)，可点击打开"
}
```

这次验证确认：

- URL 中 `displayName=受限端A` 会随 join 进入 peer profile。
- 普通端下拉框显示 `displayName (peerId)`，不会显示 `source`。
- join 完成后会重新刷新下拉框和状态文案，避免停留在中间态 `Connected`。
- h1/h2/h3 已替换到最终测试镜像。

### 常见问题

- 普通端下拉框为空：确认受限端已加入同一房间，并且 URL 带了 `audioRole=audio-restricted`。
- 普通端下拉框显示 peerId 而不是 displayName：确认 URL 参数名是 `displayName`，中文或特殊字符建议让浏览器正常 URL 编码；刷新验证时加 `cb=<timestamp>` 避免旧 JS 缓存。
- 受限端下拉框为空：这是正常的，受限端页面会排除自己。
- 普通端看不到普通端：这是正常的，下拉框只列受限端。
- 下拉框不应该显示 `source`：`source` 是 producer 的 `appData.source`，用于媒体卡片和 QoS 信息，不用于选择目标端。
- 点击打开失败 `occupied`：目标受限端已经被其他普通端占位，不能抢占。
- 音量一直是 0：如果使用 headless 或静音麦克风，这是可能的；用真实浏览器说话应能看到百分比变化。
- 看到 404：当前自动化里观测到的 404 是 favicon，不影响功能。
