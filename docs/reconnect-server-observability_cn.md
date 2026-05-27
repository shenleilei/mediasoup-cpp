# WebSocket 重连与重复 `join` 问题的服务端排查补充

## 为什么要做这个改动

这次改动的目标不是改变业务行为，而是把服务端在“断线重连 + `join`”这条链路上的可观测性补起来。

现在我们已经知道，客户端断线后会重新连接，但服务端返回的 `already joined on this connection` 还不能直接说明到底是哪一种问题：

1. 客户端在同一条新连接上重复发了 `join`
2. 第一次 `join` 还在 `pending`，第二次 `join` 就到了

这两种情况对排查和修复的意义不一样。前者说明客户端状态机有重复触发，后者说明启动期/重连期的时序还不稳。  
如果服务端日志没有把 `sessionId`、`pendingSessionId`、`hasMappedSession` 这些信息打出来，现场很难只凭一条 `already joined` 就判断根因。

所以这次补日志的目的，是让下次再遇到同类问题时，能直接从服务端日志判断：

- 是不是同一条连接发了两次 `join`
- 第二次 `join` 到底撞在“已 join”还是“还在 pending”
- 断开、重连、替换旧连接这几步的时序是否正常

## 背景

最近排查到一类现象：客户端在 WebSocket 断开后重新连接，随后服务端返回：

```json
{"ok":false,"error":"already joined on this connection"}
```

这个错误的含义不是“房间里已经有这个人”，而是“同一条 WebSocket 连接上已经 `join` 过，或者第一次 `join` 还在处理中，又来了第二次 `join`”。

当前服务端逻辑已经能拦住这类重复请求，但日志信息还不够完整，导致难以确认到底是：

1. 客户端在同一条新连接上重复发了 `join`
2. 第一次 `join` 还处于 `pending`，第二次 `join` 就到了

## 服务端要补的事情

### 1. 在 `already joined on this connection` 的拒绝分支补全日志

建议在服务端 `join` 的早期拒绝分支输出更完整的上下文，至少包含：

- `roomId`
- `peerId`
- `request id`
- `sessionId`
- `pendingSessionId`
- `hasMappedSession`
- 当前 socket 是否已经映射到 `wsMap`
- 客户端远端地址

这样可以直接区分两种情况：

- `hasMappedSession == true`：这条连接已经 join 成功，又来了第二次 `join`
- `pendingSessionId != invalid`：第一次 `join` 还没完成，第二次 `join` 就到了

### 2. 在 `join` 成功时补一条结构化日志

成功时建议明确打出：

- `roomId`
- `peerId`
- `sessionId`
- 是否替换了旧连接

这样就能把“第一次 join 成功”和“后续又重复 join”串起来看。

### 3. 在 `close` 路径补一条最终清理日志

建议在 WebSocket 关闭时补充：

- `roomId`
- `peerId`
- `sessionId`
- `pendingSessionId`
- 最终是否执行了 `leaveIfSessionMatches`

这样可以确认：

- 是普通断开后清理
- 还是新连接已经接管，旧连接只是被正确回收

## 推荐的最小日志格式

建议统一输出成一行，方便 grep：

```text
[join-reject] room=<roomId> peer=<peerId> id=<requestId> session=<sessionId> pending=<pendingSessionId> mapped=<true|false> reason=already-joined
```

```text
[join-ok] room=<roomId> peer=<peerId> session=<sessionId> replaced_old=<true|false>
```

```text
[ws-close] room=<roomId> peer=<peerId> session=<sessionId> pending=<pendingSessionId> leave_applied=<true|false>
```

## 验证方式

1. 复现客户端断线重连。
2. 观察同一时刻服务端是否先出现 `join-ok`，再出现 `join-reject`。
3. 如果出现 `join-reject`，查看 `pendingSessionId` 是否仍然存在。
4. 如果 `hasMappedSession == true`，说明同一条连接上重复发了 `join`。
5. 如果 `pendingSessionId` 还在，说明第一次 `join` 还没完成，第二次就到了。

## 结论

这次问题要先把“服务端到底收到了几次 `join`”查实。  
最有效的手段不是加很多新逻辑，而是在 `join` 拒绝、`join` 成功、`ws close` 这三个点补全上下文日志。

