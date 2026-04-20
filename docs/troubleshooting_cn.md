# mediasoup-cpp 排障手册

本文档面向两类场景：

- 服务已经跑起来，但出现“join 失败、publish 慢、worker 崩溃、多节点错乱、QoS 异常、录制异常”
- 你需要快速判断问题卡在主线程、`WorkerThread`、Redis、还是 `mediasoup-worker`

建议配合 [architecture_cn.md](./architecture_cn.md) 一起读：

- 架构文档回答“系统怎么设计的”
- 本文回答“出问题时先看哪里、怎么缩小范围”

## 1. 先看哪些入口

### 1.1 HTTP 诊断入口

本项目当前最直接的在线诊断入口有四个：

1. `GET /healthz`
2. `GET /api/node-load`
3. `GET /metrics`
4. `GET /api/resolve?roomId=...&clientIp=...`

建议观察顺序：

```text
先看 /healthz
  -> 再看 /api/node-load
     -> 再看 /metrics
        -> 最后结合日志定位
```

### 1.2 `curl` 速查

```bash
curl -s http://127.0.0.1:3000/healthz | jq
curl -s http://127.0.0.1:3000/api/node-load | jq
curl -s http://127.0.0.1:3000/metrics
curl -s "http://127.0.0.1:3000/api/resolve?roomId=test-room&clientIp=1.2.3.4" | jq
```

### 1.3 最有用的几个观察字段

`/healthz`：

- `ok`
- `startupSucceeded`
- `shutdownRequested`
- `workers`
- `availableWorkerThreads`

`/api/node-load`：

- `rooms`
- `maxRooms`
- `dispatchRooms`
- `workerQueueStats[].queueDepth`
- `workerQueueStats[].queuePeakDepth`
- `workerQueueStats[].avgTaskWaitUs`
- `roomOwnership`
- `staleRequestDrops`
- `rejectedClientStats`

`/metrics`：

- `mediasoup_sfu_healthy`
- `mediasoup_sfu_workers`
- `mediasoup_sfu_worker_threads`
- `mediasoup_sfu_available_worker_threads`
- `mediasoup_sfu_rooms`
- `mediasoup_sfu_max_rooms`
- `mediasoup_sfu_dispatch_rooms`
- `mediasoup_sfu_stale_request_drops_total`
- `mediasoup_sfu_rejected_client_stats_total`

## 2. 一分钟决策树

```text
用户报 join 失败
  -> 看 /healthz 是否 200
  -> 看 /api/node-load 是否 workers=0 或 availableWorkerThreads=0
  -> 看日志是 redirect / room not assigned / no available worker thread / remote login 哪一种

用户报 createTransport / produce 慢
  -> 看 /api/node-load 的 queueDepth / avgTaskWaitUs
  -> 看是否只有单个 WorkerThread 堆积
  -> 再区分是控制面排队慢，还是 mediasoup IPC 慢

用户报多节点串房 / 跳错节点
  -> 看 /api/resolve
  -> 看 Redis 是否可用
  -> 看 subscriber 是否正常同步 nodeCache_ / roomCache_

用户报通话中断 / 收到 serverRestart
  -> 先看 worker crash 相关日志
  -> 再看 room 是否被健康检查清理

用户报 QoS 不生效
  -> 看 clientStats 是否被拒
  -> 看 rejectedClientStats / staleRequestDrops
  -> 再看 RoomService QoS 聚合链
```

## 3. 启动失败

### 3.1 现象

- 进程启动后立刻退出
- `/healthz` 不可达
- daemon 模式父进程返回失败
- 日志出现：
  - `No WorkerThreads created, exiting`
  - `No mediasoup workers available, refusing to listen`
  - `Redis connect failed`

### 3.2 排查顺序

1. 看 `mediasoup-worker` 二进制路径是否存在、可执行
2. 看 fork+exec 后 worker 是否立刻退出
3. 看 `workerThreads` / `workers` 配置是否非法
4. 看端口是否被占用
5. 看 Redis 是否只是降级失败，还是主进程真的起不来

### 3.3 判断标准

- Redis 起不来时，系统应退化成单节点模式，而不是直接退出
- 真正会阻止 listen 的，是“没有任何可用 mediasoup worker”

### 3.4 典型原因

- `workerBin` 路径错误
- worker 二进制无执行权限
- `mediasoup-worker` 与当前系统库/内核不兼容
- 所有 worker 创建都失败

## 4. `join` 失败或行为异常

### 4.1 `no available worker thread`

含义：

- 主线程在分发 `join` 时找不到任何 `workerCount() > 0` 的 `WorkerThread`

优先看：

- `/api/node-load` 的 `availableWorkerThreads`
- 日志里是否有 worker crash / respawn rate exceeded

通常不是“房间满了”，而是“线程里已经没有活的 worker 进程”。

### 4.2 `room not assigned`

含义：

- 非 `join` 请求到达时，主线程在 `roomDispatch_` 里找不到这个房间

常见原因：

- 客户端没先成功 `join`
- 房间已被清理
- worker crash 后房间被 `checkRoomHealth()` 清掉
- 多节点重定向后客户端还在打旧节点

先看：

- `/api/node-load` 的 `roomOwnership`
- 这个 `roomId` 是否还在 `dispatchRooms`

### 4.3 `remote login`

含义：

- 这是设计内的保护，不一定是 bug
- 旧连接的请求已经排队到 `WorkerThread`，但在执行前发现 `peer->sessionId` 已被新会话替换

如果该错误偶发出现且伴随重连，是正常的；如果大量出现，通常说明：

- 客户端在一个 peerId 上频繁重复连接
- 业务层有重复 join / 并发 reconnect

### 4.4 被错误重定向

先区分是哪种错：

- `claimRoom()` 已经把房间判到别的节点
- `resolveRoom()` 选点错了
- 客户端解析 `redirect` 后行为错了

排查顺序：

1. 直接请求 `/api/resolve?roomId=...&clientIp=...`
2. 看返回的 `wsUrl` 和 `isNew`
3. 看目标节点的 `/api/node-load`
4. 看 Redis 中 node / room key 是否合理
5. 看本机是否只有 self node cache，没有及时同步其他节点

### 4.5 房间容量相关错误

如果日志出现：

- `local node at capacity`
- `no available capacity`

说明是控制面的 Router 容量保护触发了，不是 uWS 层问题。

要看：

- `/api/node-load.rooms`
- `/api/node-load.maxRooms`
- `maxRoutersPerWorker`

## 5. `createTransport` / `produce` / `consume` 慢

### 5.1 先判断是“排队慢”还是“IPC 慢”

看 `/api/node-load.workerQueueStats`：

- `queueDepth` 高、`avgTaskWaitUs` 高
  - 说明请求在进入 `WorkerThread` 前就排队了
- `queueDepth` 不高，但用户体感慢
  - 更像是 worker 线程里执行耗时，常见是 mediasoup IPC 或 auto-subscribe 多次触发

### 5.2 `createTransport` 慢

别只盯一个 IPC。

接收 transport 的完整动作是：

1. `ROUTER_CREATE_WEBRTCTRANSPORT`
2. 遍历房间已有 producer
3. 对每个 producer 做一次 `TRANSPORT_CONSUME`

所以如果一个房间已有很多 producer，`createTransport` 变慢是预期现象。

排查时先看：

- 房间里已有多少 peer / producer
- 是 send transport 还是 recv transport
- 慢在第一下 transport create，还是慢在后面的批量 consume

### 5.3 `produce` 慢

`produce` 可能慢在三段：

1. `TRANSPORT_PRODUCE`
2. auto-subscribe: 对其他 peer 的多个 `TRANSPORT_CONSUME`
3. auto-record: PlainTransport + pipe consumer

如果只有大房间慢，优先怀疑第二段；如果开录制时明显慢，优先怀疑第三段。

### 5.4 `consume` 慢

显式 `consume` 的问题通常落在：

- ORTC 参数不兼容
- `TRANSPORT_CONSUME` 慢
- producer 本身已经被清理或不可见

如果是“有时成功有时失败”，要同时检查：

- producer 是否在同一个 room/router
- reconnect 是否把旧 producer 或旧 transport 替换掉了

### 5.5 浏览器房间里看不到 plain-client 的多 track 视频

这是本仓库已经实际遇到过的一类联调现象：

- plain-client 日志里已经出现：
  - `WS connected`
  - `Joined room=...`
  - `Publish -> ... videoTracks=3`
- 但浏览器页面里仍然看不到任何远端视频。

先不要默认判断为“SFU 需要重启”。

优先排查顺序：

1. 确认 plain-client 进程还活着，而不是源文件已经播完退出。
2. 确认浏览器加入的是同一个 `roomId`。
3. 如果浏览器是先 join，plain-client 后 plainPublish，先刷新页面并重新 join 一次。
4. 再去查服务端是否真的完成了 `plainPublish -> auto-subscribe -> newConsumer` 链路。

本次联调里已经确认：

- 现有 `mediasoup-sfu` 不需要为这类 plain-client 多 track 发布专门重启。
- 问题更常见地出在“浏览器订阅时机”或“plain-client 已经结束”。
- 短视频源文件非常容易制造假象：服务端已经建起 producer，但用户打开页面时发送进程已经退出。

手工联调建议：

```bash
ffmpeg -y -stream_loop 19 -i tests/fixtures/media/test_sweep.mp4 -c copy /tmp/test_sweep_x20.mp4
env PLAIN_CLIENT_VIDEO_TRACK_COUNT=3 \
  ./client/build/plain-client \
  127.0.0.1 3000 <roomId> plain_3track_demo /tmp/test_sweep_x20.mp4
```

当 plain-client 日志已经出现 `Publish -> ... videoTracks=3` 时：

- 优先看页面刷新后重新 join 是否恢复；
- 只有在 `plainPublish` 本身失败、或服务端房间状态明显异常时，再考虑重启 SFU。

## 6. `mediasoup-worker` IPC 问题

### 6.1 典型症状

- `Channel request timeout (5000ms)`
- 某些 `getStats()` 超时
- worker fd EOF
- `response does not match any request`

### 6.2 `Channel request timeout`

默认超时：

- 普通 IPC：5 秒
- 单次 stats IPC：500 毫秒

出现超时时，先判断：

1. worker 进程还活着吗
2. `WorkerThread` 事件循环是否被长任务卡住
3. 是不是某个请求类型天然很重

如果是 `getStats()` 超时增多，但媒体仍可用，常常说明：

- worker 还活着，但 stats 拉取压力大
- control thread 正在被大量 stats 请求占满

### 6.3 non-threaded `Channel` 死锁风险

当前架构里，non-threaded `Channel` 必须依赖 `WorkerThread` 持续 pump fd。

如果你把 IPC 调用放到错误线程，或者引入了 `request().get()` 这类写法，就可能出现：

- promise 永远等不到响应
- 不是 worker 真慢，而是根本没人 read consumer fd

### 6.4 `response does not match any request`

这种日志说明：

- 收到了一个 response
- 但本地 `sents_` 里已经没有对应 requestId

通常意味着：

- 请求超时后被本地提前清理
- worker 的响应晚到了

它更像“症状放大器”，真正原因往往仍是前面的慢请求或 worker 卡顿。

## 7. Worker 崩溃与房间被清理

### 7.1 症状

- 客户端收到 `serverRestart`
- 房间突然消失
- 日志出现：
  - `worker died`
  - `attempting respawn`
  - `respawn rate exceeded`

### 7.2 实际恢复链

1. `WorkerThread` 发现 channel fd EOF
2. `handleWorkerDeath()`
3. `onWorkerDied()`
4. 尝试 respawn 新 worker
5. 周期性 `checkRoomHealth()` 发现 dead router
6. 广播 `serverRestart`
7. 清理房间并注销 registry

注意：

- worker respawn 成功，不等于旧房间会自动恢复
- 当前策略是通知客户端重连，让控制面重建房间状态

### 7.3 怎么判断是“单个 worker 崩”还是“整机退化”

看 `/api/node-load`：

- `workers` 下降但不为 0：部分退化
- `availableWorkerThreads == 0`：整机已无法承载新请求

## 8. Redis / 多节点问题

### 8.1 房间归属不一致

症状：

- 一个房间在两个节点都认为自己拥有
- `/api/resolve` 返回结果和实际 join 落点不一致

先查：

- Redis `sfu:room:<roomId>`
- 各节点 `/api/node-load.roomOwnership`
- 是否有节点长时间没 heartbeat

### 8.2 某节点只看到自己，看不到其他节点

常见原因：

- subscriber 线程断连后未及时恢复
- `nodeCache_` 只剩 self
- Redis 网络抖动

症状通常是：

- `resolveRoom()` 老是选自己
- 明明集群里还有别的节点，但本机路由视图像单节点

### 8.3 Redis 不可用时的表现

默认运行契约下，Redis 不可用不会再隐式降级成单节点：

- 启动阶段：进程直接启动失败
- 运行阶段：`/readyz` 返回 `503`
- 新的 `/api/resolve` / `join` 等依赖 room-registry 的路径会显式失败

只有显式使用 `redisRequired=false` 时，才允许 local-only 模式继续运行。

### 8.4 subscriber 忙循环

如果看到 subscriber 相关 CPU 异常或日志异常密集，先看已有专题文档：

- [fix-subscriber-busyloop.md](./fix-subscriber-busyloop.md)

## 9. QoS 问题

### 9.1 `clientStats` 被拒

看两个计数：

- `staleRequestDrops`
- `rejectedClientStats`

常见原因：

- 客户端还没完成 join 就发了 `clientStats`
- reconnect 后旧会话还在发 stats
- schema 校验失败

### 9.2 QoS override / policy 看起来没生效

先区分是哪个阶段出了问题：

1. `clientStats` 根本没进 worker
2. 进了 worker，但聚合后没触发 override
3. override 发了，但客户端没应用

服务端排查顺序：

- `clientStats` 是否被拒
- `RoomService::setClientStats()` 是否被执行
- `maybeSendAutomaticQosOverride()` / `maybeNotifyConnectionQuality()` 是否触发
- `notify_` 是否通过 `defer` 成功发回主线程

### 9.3 QoS 与媒体问题不要混着查

当前 uplink QoS 主要是控制面策略，不是 mediasoup-worker 里的媒体算法。

所以如果问题是：

- override 没发
- qosPolicy 没收到
- connectionQuality 不更新

先看控制面，不要第一时间去翻 `Transport` / `Producer` 的 IPC。

## 10. 录制问题

### 10.1 症状分类

- 没有 `.webm`
- 有 `.webm` 但没有 `.qos.json`
- 文件存在但内容异常
- 开录制后 publish 明显变慢

### 10.2 录制链排查顺序

1. `RoomService::autoRecord()` 是否执行
2. `PeerRecorder::createSocket()` / `start()` 是否成功
3. `Router::createPlainTransport()` 是否成功
4. `PlainTransport::connect(127.0.0.1, port)` 是否成功
5. `pt->consume(pipe=true)` 是否成功

如果录制链失败，媒体主链通常仍然可能正常。

### 10.3 磁盘与清理

系统会按录制目录总大小做清理，但这不是替代监控的理由。

遇到录制异常时一起看：

- 磁盘剩余空间
- 是否触发旧目录清理
- 录制目录权限

## 11. 常用定位命令

### 11.1 HTTP / metrics

```bash
curl -s http://127.0.0.1:3000/healthz | jq
curl -s http://127.0.0.1:3000/api/node-load | jq
curl -s http://127.0.0.1:3000/metrics | less
```

### 11.2 进程

```bash
ps -ef | grep mediasoup
pgrep -af mediasoup-worker
```

### 11.3 日志

```bash
tail -f /var/log/mediasoup/*.log
grep -R \"worker died\\|serverRestart\\|Channel request timeout\\|remote login\" /var/log/mediasoup
```

### 11.4 Redis

```bash
redis-cli KEYS 'sfu:node:*'
redis-cli KEYS 'sfu:room:*'
redis-cli GET sfu:room:test-room
redis-cli SUBSCRIBE sfu:ch:nodes sfu:ch:rooms
```

## 12. 升级为长期治理的问题

下面这些问题如果出现一次，不要只停留在“现场恢复”：

- `queueDepth` 长期偏高
- `avgTaskWaitUs` 长期上升
- `Channel request timeout` 持续出现
- `serverRestart` 偶发但重复出现
- `staleRequestDrops` 或 `rejectedClientStats` 长期高
- 某些房间或大房间下 `createTransport` / `produce` 明显退化

这些都应该回到架构层看：

- 是否需要拆更细的任务
- 是否需要把慢逻辑从 `WorkerThread` 挪走
- 是否需要补指标
- 是否需要给多节点缓存和 subscriber 加更强的一致性保护
