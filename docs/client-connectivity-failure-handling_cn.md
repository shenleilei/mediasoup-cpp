# 客户端连通性故障处理

本文只覆盖当前已经验证过的三种情况：

1. 媒体断，信令还在
2. 信令断，媒体还在
3. 信令断，服务端 peer 已清理

不讨论 TURN、TCP 回落、跨节点迁移。

## 1. 关键时间

| 时间 | 含义 | 处理口径 |
| --- | --- | --- |
| `10s` | 客户端 `wsRequest` 超时 | 只说明这次信令请求失败，不说明服务端资源已清理 |
| `10s` | 媒体恢复 grace window | transport 进入 `disconnected` 后先等 `10s`，仍未恢复再 `restartIce` |
| `30s` | worker `ICE consent timeout` | worker 进入 `disconnected` 并清 selected tuple，不会自动 `restartIce` |
| `120s` | 服务端 websocket `idleTimeout` | `.close` 触发后才会走 `leave -> peer.close()` 清理资源 |
| `500ms` | stats 单次查询预算 | 只影响统计采集，不用于连接恢复状态机 |

因此会存在一个窗口：端上信令请求已超时，但服务端 websocket 还没 close，peer/transport 仍在，媒体仍可能继续。

## 2. 三种情况

| 情况 | 检测信号 | 端上处理 |
| --- | --- | --- |
| 媒体断，信令还在 | websocket 正常；transport 进入 `disconnected/failed` | `disconnected` 后等 `10s`；仍未恢复或进入 `failed` 时，对旧 transport 做 `restartIce` |
| 信令断，媒体还在 | websocket close/error 或请求超时；transport 仍 `connected`；重连后 `joinMode=replaced-session` | 先重连 websocket，用同 `roomId + peerId` 重新 `join`；媒体仍通则沿用旧 transport，不做 `restartIce` |
| 信令断，服务端 peer 已清理 | 重连后 `joinMode=new-peer`；或旧资源操作返回 `room/peer/transport not found` | 不再复用旧 transport，重建 send/recv transport，重新 produce/consume |

## 3. joinMode

`join` response 返回：

```json
{
  "joinMode": "replaced-session"
}
```

- `replaced-session`
  同一 `roomId + peerId` 的旧 websocket session 仍存在，新 websocket 顶替旧 session。服务端保留原 peer 上的 transport / producer / consumer。

- `new-peer`
  服务端没有旧 session 可以顶替。这次是新 peer，端上必须重建 transport 和媒体发布/订阅。

`replaced-session` 只说明服务端资源还在，不保证媒体路径一定可用。端上仍要看 transport 状态、RTP stats，以及旧 `transportId` 操作是否成功。

## 4. 端上处理分支

媒体状态入口：

```js
transport.on('connectionstatechange', state => {
  if (state === 'connected') {
    clearIceRecoveryTimer(transport.id);
    return;
  }

  if (state === 'disconnected') {
    startIceRecoveryTimer(transport.id, 10_000);
    return;
  }

  if (state === 'failed') {
    restartIceForTransport(transport);
  }
});
```

信令异常后先恢复 websocket：

```js
const ws = await connectWs();
const joinResp = await wsRequest('join', {
  roomId,
  peerId,
  displayName,
  rtpCapabilities
});

if (joinResp.data.joinMode === 'replaced-session') {
  if (transportsConnected() && mediaLooksHealthy()) {
    await resyncRoomState(joinResp.data);
  } else {
    await restartIceForTransport(sendTransport);
    await restartIceForTransport(recvTransport);
  }
}

if (joinResp.data.joinMode === 'new-peer') {
  await rebuildMediaSession(joinResp.data);
}
```

`restartIceForTransport()` 失败时，如果错误是 `room not found` / `peer not found` / `transport not found`，说明旧资源不可用，直接走 `rebuildMediaSession()`。

`mediaLooksHealthy()` 不应只看 stats。推荐口径是：transport 仍是 `connected`，并且在有活跃媒体预期时 RTP stats 继续增长；如果当前本来没有活跃音视频，stats 不增长不能判定媒体坏。

## 5. RTP stats 口径

RTP stats 增长用于证明恢复成功，不用于单独判断连接断开。

- `disconnected/failed` 是媒体恢复流程的主要入口
- `connected` 但 stats 不增长，不一定是连接问题
- stats 不增长可能来自摄像头关闭、producer/consumer pause、track ended/muted、页面后台、编码器无输出、该路本来没有活跃媒体

恢复成功至少看：

1. transport 回到 `connected`
2. 状态经历过 `disconnected/failed -> connected`
3. `packetsSent/bytesSent/packetsReceived/bytesReceived` 继续增长

当前测试里 stats 断言有意严格：测试使用持续变化的 canvas 视频源，producer/consumer 未 pause，正常情况下 RTP 必须增长。测试最多等 `15s`，每 `500ms` 采样一次。

## 6. 测试覆盖

| 情况 | 覆盖 |
| --- | --- |
| 媒体断，信令还在 | [browser_ice_restart.mjs](../tests/qos_harness/browser_ice_restart.mjs) UDP case：断 UDP，`10s` grace，`restartIce` 后媒体恢复 |
| 信令断，媒体还在 | [browser_ice_restart.mjs](../tests/qos_harness/browser_ice_restart.mjs) TCP/WSS case：信令超时，媒体继续增长，恢复后 `joinMode=replaced-session` |
| `replaced-session` 后媒体又坏 | 同一浏览器测试：恢复信令后再断 UDP，用旧 transportId `restartIce` 恢复 |
| 信令断且 peer 已清理 | [test_review_fixes_integration.cpp](../tests/test_review_fixes_integration.cpp)：重新 join 返回 `new-peer`，旧 transport `restartIce` 失败 |

多轮串联测试：

```bash
ICE_RESTART_REPEAT=2 node tests/qos_harness/browser_ice_restart.mjs
```

每轮都会顺序覆盖 UDP 媒体断恢复、TCP/WSS 信令断恢复、`replaced-session` 后再次 UDP 媒体断并通过旧 transportId `restartIce` 恢复。测试结束会清理 `tc` 规则。

## 7. 推荐日志

端上最少记录：

- websocket `open/close/error`
- 请求超时的方法名
- `joinMode`
- transport `connectionstatechange`
- `restartIce` 请求和返回时间
- 恢复前后 `packets/bytes` 差值
