# mediasoup-cpp 静态代码审查报告

日期：2026-04-08

> **文档性质**
>
> 这是 `2026-04-08` 的历史静态 code review 记录。
> 它反映的是 review 当时的发现，不等于当前仓库状态。
> 如需看当前结论，请优先参考 [DEVELOPMENT.md](../platform/README.md) 和相关专项最终报告。

## 范围与方法

- 范围：`src/`、`tests/`、`CMakeLists.txt`、`generated/`
- 方法：仅做静态代码检查与设计审查，不做动态运行结论

## 总结

整体架构方向是对的：控制面把 `RoomService` 串行化到单独 worker 线程，`Worker` / `Channel` / `Router` / `Transport` 的分层也比较清晰，便于把 mediasoup worker 进程封装在 C++ 控制面之后。

但当前实现里有几处实质性缺陷，已经超出“代码风格”层面，会直接影响在线会话、集群主权和故障恢复。下面按严重级别列出。

## Findings

### 1. 高风险：相同 `peerId` 的重连会覆盖在线会话，旧连接关闭时还会把新连接踢下线

位置：

- `src/RoomService.cpp:17`
- `src/RoomManager.h:25`
- `src/SignalingServer.cpp:223`
- `src/SignalingServer.cpp:236`

问题说明：

- `RoomService::join()` 创建新 `Peer` 后，直接调用 `room->addPeer(peer)`。
- `Room::addPeer()` 直接执行 `peers_[peer->id] = peer`，不会拒绝重复 `peerId`，也不会先关闭旧 `Peer`。
- `SignalingServer` 在 join 成功后用 `wsMap->peers[jPeerId] = ws` 覆盖旧 websocket 指针。
- 旧连接之后进入 `.close` 回调时，会按同一个 `peerId` 执行 `wsMap->peers.erase(peerId)` 和 `roomService_.leave(roomId, peerId)`。

实际后果：

- 新连接刚加入成功，旧连接一旦关闭，就会把新连接的 `wsMap` 映射删掉。
- 同一个 `peerId` 的 `leave()` 会把新 `Peer` 一并移出房间，形成“重连成功后又被旧连接踢掉”的行为。
- 被覆盖掉的旧 `Peer` 没有显式 `close()`，它持有的 transport / producer / consumer 也不会被正常清理，容易在 worker 侧留下泄漏对象。

建议：

- 把“同一 `peerId` 再次 join”定义为显式协议行为：拒绝、踢旧连接、或以 session token 替换。
- `close` 路径不要只按 `peerId` 清理，至少要带上连接级唯一标识，确认当前关闭的是不是仍然绑定到该 `peerId` 的那个 socket。
- 覆盖旧 `Peer` 前必须先做资源回收，而不是直接覆盖 `shared_ptr`。

### 2. 高风险：Redis 死节点接管不是原子操作，存在房间双主风险

位置：

- `src/RoomRegistry.h:93`

问题说明：

- `claimRoom()` 先用 `SET key value NX EX ttl` 抢新房间，这一步是对的。
- 但如果发现房间已有 owner，且 owner 对应节点已经死掉，代码会直接执行普通 `SET key nodeId EX ttl` 覆盖。
- 这个“判定 owner 已死”到“写入新 owner”的过程不是原子操作，也没有 compare-and-set。

实际后果：

- 两个节点同时发现旧 owner 已死时，都可能走到 takeover 分支。
- 两边都可能返回“我接管成功了”，然后分别在本地创建同一个房间，形成 split-brain。
- 这会破坏多节点路由的一致性，出现同房间双主、用户被分流到不同 SFU 的问题。

建议：

- takeover 需要原子化。常见做法是：
- 用 Lua 脚本把“读取旧 owner、确认 owner 死亡、替换 owner”放进一个 Redis 原子操作。
- 或者使用 `WATCH/MULTI/EXEC` 做带版本校验的 CAS。
- 进一步建议为房间 owner 引入 fencing token，避免延迟节点继续对旧房间写入。

### 3. 高风险：`restartIce()` 忽略 worker 返回的新 ICE 参数，返回给客户端的仍是旧 credential

位置：

- `src/WebRtcTransport.cpp:44`
- `generated/transport_generated.h:624`

问题说明：

- `TRANSPORT_RESTART_ICE` 的返回体里明确包含 `username_fragment`、`password`、`ice_lite`。
- 但 `WebRtcTransport::restartIce()` 只发请求，不解析响应，随后直接把缓存里的 `iceParameters_` 原样返回。

实际后果：

- 服务端表面上完成了 ICE restart。
- 客户端却拿不到新的 `ufrag` / `pwd`，继续沿用旧 ICE credential。
- 在网络切换、ICE failover、重协商场景下，这会让 restartIce 失效，表现为“信令成功但媒体恢复失败”。

建议：

- 解析 `Transport_RestartIceResponse`。
- 用响应中的新值覆盖 `iceParameters_`。
- 把更新后的 `iceParameters_` 返回给上层信令。

### 4. 中风险：`collectPeerStats()` 里的 timeout 设计实际上不成立，超时后仍可能卡住唯一控制线程

位置：

- `src/RoomService.cpp:520`
- `src/RoomService.cpp:552`
- `src/RoomService.cpp:579`

问题说明：

- 代码使用 `std::async(std::launch::async, ...)` 发起 `getStats()`，再用 `wait_for(kTimeout)` 做超时判断。
- 一旦超时，函数直接返回 `nullptr` 或 fallback 值，没有 `get()`。
- 对 `std::launch::async` 创建的 `std::future`，当 future 在未取结果时析构，调用线程仍然要等异步任务结束。

实际后果：

- 表面上看写了 timeout，实际上 `collectPeerStats()` 很可能还是会被慢请求拖住。
- 由于 `RoomService` 运行在单个 worker 线程上，`broadcastStats()` 卡住时，会连带影响 join / produce / consume 等控制面请求。
- 这和当前架构“控制面串行但不能被慢操作阻塞”的目标相冲突。

建议：

- 不要把 `std::async + wait_for` 当成真正可中断超时。
- 更稳妥的方案是：
- 直接在同线程里依赖 `Channel::requestWait()` 的有界超时。
- 或者引入专用 stats 线程池 / 任务队列，把统计采集从主控制线程剥离。
- 如果一定要并发采集，需要有不会在 future 析构时回阻塞调用线程的执行模型。

### 5. 中风险：`PlainTransport::connect()` 没有超时控制，录制链路可以把唯一 worker 线程长期卡住

位置：

- `src/PlainTransport.h:18`

问题说明：

- `PlainTransport::connect()` 调用 `channel_->request(...)` 后，直接 `future.get()`。
- 这里没有像其它路径那样用 `requestWait(timeout)` 做上界控制。
- `RoomService::autoRecord()` 会在控制线程里同步调用这段逻辑。

实际后果：

- 一旦 worker 在 `PLAINTRANSPORT_CONNECT` 上卡住，录制初始化会把控制线程一起卡住。
- 影响范围不只是录制，房间里的所有后续控制面请求都会排队等待。

建议：

- 把 `future.get()` 改成有超时的 `requestWait()`。
- 录制初始化最好从主控制线程拆出去，至少不要让 recorder 路径拥有无限等待能力。

## 其他观察

### 低风险：`Room::close()` 默认假设 `router_` 非空

位置：

- `src/RoomManager.h:83`

说明：

- `router_->close()` 没有空指针保护。
- 生产路径里 `RoomManager::createRoom()` 会注入有效 router，但测试代码已经显式构造过 `Room("id", nullptr)`。
- 这更像内部约束没有被类型系统表达出来，建议补保护或在类型层面约束 `Room` 不能持有空 router。

### 低风险：`findLeastLoadedNode()` 使用 `KEYS node:*`

位置：

- `src/RoomRegistry.h:213`

说明：

- 在节点数上来之后，`KEYS` 会成为阻塞式 O(N) 操作。
- 这不是当前最紧急的问题，但它会限制房间解析路径的伸缩性。
- 更稳妥的做法是维护显式节点集合，或用 `SCAN` / sorted set 表达负载。

## 设计评价

优点：

- `RoomService` 串行化后，绝大多数业务状态不需要细粒度锁，整体可读性不错。
- `Channel::OwnedResponse` / `OwnedNotification` 通过值语义持有 FlatBuffers 数据，避免了很多悬垂指针问题。
- `WorkerManager`、`RoomRegistry`、`Recorder` 这些子系统职责分离清楚，后续适合继续做模块化补强。

主要设计债务：

- 当前实现对“连接身份”和“业务身份”区分不够。`peerId` 同时承担了业务主体和 socket 绑定键，导致重连/替换场景非常脆弱。
- 多节点房间主权没有用原子协议表达，故障接管逻辑不够严格。
- 一些看似“有 timeout”的实现实际上仍然会把单线程控制面拖住，说明故障路径的线程模型还没有完全收敛。

## 建议修复顺序

1. 先修复重复 `peerId` 的 join / close 语义，避免在线用户会话被错误清理。
2. 修复 `restartIce()` 返回旧参数的问题，这是明显的功能性缺陷。
3. 把 `claimRoom()` 改成原子接管，避免多节点 split-brain。
4. 清理 stats / recorder 路径上的无限等待和伪超时，保证控制线程可恢复。
