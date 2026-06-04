# 房间客户端 SDK 后续规划

本文总结后续要把 `mediasoup-room-client.js` 从 demo 级客户端打磨成业务可依赖 SDK 需要做的事情。目标不是让业务理解 mediasoup 全流程，而是让业务只用少量 API 拿到媒体流和状态事件。

## 1. 目标

一年内客户端接入形态应收敛成：

- 创建客户端时业务只传 `wssUrl`、`roomId`、`peerId` 等必要参数。
- SDK 内部处理信令、WebRTC transport、producer、consumer、ICE restart、重连和资源清理。
- 业务通过事件拿到远端媒体和状态，不直接操作底层 `transport/producer/consumer`。
- 发布前通过自动化测试覆盖信令断、媒体断、资源关闭、重连替换和多轮混合场景。

## 2. 公开 API 收口

先固定最小公开 API，后续 demo 和业务都只依赖这些入口：

```js
const client = new MediasoupRoomClient({
  wssUrl,
  roomId,
  peerId,
  displayName
});

await client.join();
await client.leave();

await client.publish({
  audio: {
    appData: { source: 'audio' }
  },
  video: {
    appData: { source: 'camera' }
  }
});
await client.unpublish({ producerId });

await client.publish({
  audio: {
    appData: {
      source: 'talkback',
      targetPeerId
    }
  }
});

client.on('track', ({ peerId, producerId, consumerId, kind, source, appData, stream, track }) => {});
client.on('trackClosed', ({ peerId, producerId, consumerId, reason }) => {});
client.on('peerJoined', peer => {});
client.on('peerLeft', peer => {});
client.on('networkState', state => {});
client.on('error', error => {});
```

业务不直接调用：

- `createWebRtcTransport`
- `connectWebRtcTransport`
- `consume`
- `restartIce`
- `transport.consume()`
- `transport.produce()`

这些都由 SDK 内部完成。

## 3. 异常恢复内聚

SDK 内部统一处理三类情况。具体故障口径见 [client-connectivity-failure-handling_cn.md](client-connectivity-failure-handling_cn.md)。

| 场景 | SDK 行为 | 业务看到什么 |
| --- | --- | --- |
| 媒体断，信令还在 | transport `disconnected` 后等待 `10s`；仍未恢复或进入 `failed` 时自动 `restartIce` | `networkState` 变化；恢复成功后不需要业务重建 |
| 信令断，媒体还在 | 重建 WebSocket，用同 `roomId + peerId` 重新 `join`；`joinMode=replaced-session` 时复用旧媒体资源 | `networkState` 进入 signaling reconnecting/restored |
| 信令断，服务端 peer 已清理 | `joinMode=new-peer` 或旧资源 not found 时重建 send/recv transport，重新 publish/subscribe | 业务收到短暂重建状态；最终恢复或失败事件 |

SDK 需要维护：

- websocket 状态
- send/recv transport 状态
- producer/consumer 映射
- peer/producer/consumer 元数据
- ICE recovery timer
- restartIce in-flight 标记
- 重连中 pending request 清理

## 4. 媒体资源表达

对业务统一返回一份稳定结构。客户端创建时不要求业务传 `appData`，但每路媒体的 producer `appData` 需要透传给业务，用于布局和业务语义判断。

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

业务用 `peerId + kind + source + appData + stream` 做布局和业务判断。`source` 只表示媒体来源，例如 `camera`、`screen`、`audio`，不表示目标端。

`producerLeft` 和 `consumerClosed` 都由 SDK 转成 `trackClosed`：

- `producerLeft`：producer 已不存在，SDK 清理该 producer 相关 consumers。
- `consumerClosed`：只关闭当前 consumer，producer 可能仍存在。

## 5. Demo 与 SDK 分离

`room-client-demo.html` 只保留 UI 示例：

- 输入房间号和 WSS 地址
- 调用 SDK `join/leave/publish`
- 监听 `track/trackClosed/networkState`
- 按 peer 一行渲染全部 tracks

核心逻辑必须留在 `mediasoup-room-client.js`：

- 信令请求封装
- 自动重连
- joinMode 分支
- transport 创建和连接
- producer/consumer 生命周期
- ICE restart
- 资源清理

demo 不再自行实现恢复策略。

## 6. 对讲能力

对讲/音频受限可以作为 SDK 上层能力，不建议散落在业务 demo 中。

推荐提供独立封装：

```js
const talkback = new TalkbackClient(roomClient);

await talkback.claim(targetPeerId);
await talkback.openMic();
await talkback.closeMic();
await talkback.release();
```

对讲音频仍然是一路普通 audio producer，但必须带媒体级 `appData`：

```js
await roomClient.publish({
  audio: {
    appData: {
      source: 'talkback',
      targetPeerId
    }
  }
});
```

推荐顺序：

1. `talkback.claim(targetPeerId)`：服务端确认目标端可被当前 peer 对讲。
2. `talkback.openMic()`：内部调用 `roomClient.publish()`，发布 `source='talkback'` 的 audio producer。
3. `talkback.closeMic()`：内部调用 `roomClient.unpublish()`，关闭本地对讲 producer。
4. `talkback.release()`：释放服务端对目标端的占用。

底层仍复用 `MediasoupRoomClient` 的信令、transport、producer 和恢复能力。业务只关心对讲是否打开、是否被占用、目标端是否离开。

## 7. 测试门禁

后续测试重点要从“单个协议点能跑”升级到“业务 SDK API 在异常后仍可用”。

| 测试 | 目标 |
| --- | --- |
| SDK 正常入会拉流 | `join()` 后能收到房间内全部远端 `track` |
| producer close | 远端 producer 销毁后触发 `trackClosed`，本地 stream/DOM/consumer 映射清理 |
| 信令短断 | WebSocket 恢复后 `joinMode=replaced-session`，媒体不重建也可继续 |
| 信令断且 peer 已清理 | SDK 自动走 `new-peer` 重建 transport 和订阅 |
| UDP 媒体断 | `10s` grace 后自动 `restartIce` 并验证 RTP 恢复 |
| 信令恢复但媒体异常 | `replaced-session` 后媒体坏时对旧 transport 做 `restartIce` |
| 多轮混合场景 | 信令断、UDP 断、producer close 串起来反复跑 |
| 对讲恢复 | claim/openMic 后经历重连或目标重进，能按规则恢复或释放 |

这些测试应加入发布门禁或 nightly。当前 ICE restart 门禁已经接入 `run_all_tests.sh all`，后续继续把 SDK 级测试补进去。

## 8. 可观测性

SDK 端最少上报或打印：

- websocket `open/close/error/reconnect`
- `joinMode`
- transport `connectionstatechange`
- `restartIce` 开始、成功、失败、耗时
- producer/consumer 创建和关闭
- `track` / `trackClosed`
- 恢复前后 RTP packets/bytes 差值

服务端侧继续补齐：

- `createTransport` 返回时的 `iceState/dtlsState`
- ICE state 变化日志
- producer/consumer close 日志
- ws close 到 peer 清理日志
- restartIce 请求和结果日志

目标是线上能按 `roomId + peerId + transportId + producerId + consumerId` 串起来定位。

## 9. 里程碑

### 第一阶段：SDK API 固化

- 固定 `MediasoupRoomClient` 构造参数、方法和事件。
- demo 全部改为调用 SDK API。
- 补 SDK 正常 join/拉流/producer close 测试。

### 第二阶段：异常恢复 SDK 化

- 将信令重连、joinMode 分支、ICE restart、重建 transport 全部收进 SDK。
- 补信令断、UDP 断、replaced-session 后媒体坏、多轮混合测试。
- 整理业务接入文档，只保留业务需要调用的 API 和事件。

### 第三阶段：对讲能力产品化

- 抽象 `TalkbackClient` 或同级模块。
- 覆盖 claim/release/openMic/closeMic 和目标重进。
- 明确对讲和普通收流的事件边界。

### 第四阶段：发布与运维闭环

- SDK 加版本号和兼容说明。
- 发布门禁包含 SDK 级恢复测试。
- 监控面板能区分信令断、媒体断、服务端清理、端上重建。

## 10. 当前优先级

建议近期按这个顺序做：

1. 固定 `MediasoupRoomClient` 公开 API 和事件。
2. 确认 `room-client-demo.html` 没有业务无关的恢复逻辑。
3. 补 SDK 级 producer close、信令重连、ICE restart 测试。
4. 把业务接入文档改成只面向 SDK API。
5. 再设计对讲独立封装。
