# mediasoup-cpp Redis Key 规范

## 目标

这份文档定义 `mediasoup-cpp` 项目内 Redis key 的命名、编码和使用规则，目标是解决以下问题：

- 用户输入直接进入 Redis key，导致 key 冲突、分隔符歧义和安全风险
- 多 key 批量命令依赖字符串拼接，遇到特殊字符会出问题
- 不同类型的 Redis key 语义不清晰，不利于排障和运维
- 本地内存复合 key 与 Redis key 风格不一致，容易引入碰撞

## 总原则

### 1. 不要把用户输入直接拼进 Redis key

`roomId`、`peerId`、`displayName` 这类来自客户端的数据，默认都视为不可信输入。

禁止写法：

```text
room:<roomId>
node:<nodeId>
room:<roomId>:peer:<peerId>
```

因为这些输入可能包含：

- `/`
- `:`
- 空格
- 换行
- 通配符
- 非 ASCII 字符

这些字符会影响：

- Redis key 结构
- SCAN / MATCH 模式
- 批量命令拼接
- 日志可读性
- 文件路径拼接

### 2. key 命名必须带命名空间

推荐统一格式：

```text
<app>:<env>:<domain>:<entity>:<id>
```

例如：

```text
mscpp:prod:room:owner:<room_id_safe>
mscpp:prod:node:meta:<node_id_safe>
mscpp:prod:room:lease:<room_id_safe>
```

如果暂时不区分环境，也至少要保证：

```text
sfu:room:owner:<room_id_safe>
sfu:node:meta:<node_id_safe>
```

### 3. key 要表达“这是什么”，不要只表达“这是谁”

例如：

- `sfu:room:owner:<room_id_safe>` 明确表示“房间 owner”
- `sfu:node:meta:<node_id_safe>` 明确表示“节点元数据”

不要只写：

- `room:<id>`
- `node:<id>`

因为后续一旦需要增加别的 room/node 相关 key，就会命名混乱。

### 4. 多 key 命令不要手工拼接命令字符串

禁止：

```cpp
std::string mget = "MGET " + key1 + " " + key2;
redisCommand(ctx, mget.c_str());
```

推荐：

- `redisCommandArgv`
- pipeline + 参数化
- Lua 脚本参数传递

即使 key 已经做过编码，也不要依赖“拼接字符串刚好没问题”。

## 推荐的 ID 处理方式

## 方案 A：严格白名单字符集

如果协议层允许约束 `roomId` / `peerId`，推荐直接限制为：

```text
[A-Za-z0-9_-]{1,128}
```

优点：

- 实现最简单
- 日志和 key 可读性最好

缺点：

- 对外部接入系统约束较强

## 方案 B：保留原始输入，但在 key 中使用安全编码

推荐优先级：

1. `hex(sha256(raw_id))`
2. base64url(raw_id)
3. percent-encoding

其中最稳妥的是：

```text
room_id_safe = hex(sha256(roomId))
peer_id_safe = hex(sha256(peerId))
```

优点：

- 不受原始字符影响
- key 长度固定
- 不会泄露原始业务 ID

缺点：

- key 不可直接读懂

因此推荐把原始值放入 value，而不是放入 key。

例如：

```text
sfu:room:owner:9f3ab1...
```

value:

```json
{
  "roomId": "原始 roomId",
  "ownerNodeId": "hz-01",
  "ownerAddress": "ws://10.0.0.5:3000/ws"
}
```

## 当前项目推荐方案

结合当前 `RoomRegistry`、`roomId`、`peerId` 用法，推荐：

### 节点元数据

```text
sfu:node:meta:<node_id_safe>
```

value:

```text
<address>|<rooms>|<maxRooms>|<lat>|<lng>|<isp>|<country>
```

如果未来允许改 value 格式，建议直接切 JSON。

### 房间 owner

```text
sfu:room:owner:<room_id_safe>
```

value:

```text
<node_id_safe>
```

### Pub/Sub channel

```text
sfu:pubsub:nodes
sfu:pubsub:rooms
```

### 可选：房间调试映射

如果需要排障，可增加：

```text
sfu:room:debug:<room_id_safe>
```

value:

```json
{
  "roomId": "原始 roomId",
  "ownerNodeId": "...",
  "updatedAt": 1710000000
}
```

用于人工排查，不参与核心读写路径。

## 本地内存 key 也要一起规范

当前代码里本地复合 key 常见形式是：

```text
roomId + "/" + peerId
```

这和 Redis key 问题本质相同，也会有分隔符碰撞。

推荐改成以下任一方案：

### 方案 1：二级 map

```cpp
unordered_map<string, unordered_map<string, T>>
```

### 方案 2：结构体 key

```cpp
struct RoomPeerKey {
  std::string roomId;
  std::string peerId;
};
```

配套 `hash<RoomPeerKey>`。

### 方案 3：若必须拼接，至少先编码

```text
safe(roomId) + ":" + safe(peerId)
```

但整体上不推荐继续走拼接字符串方案。

## 针对当前项目的风险点

以下位置特别需要统一治理：

### 1. Redis key

当前 `RoomRegistry` 中仍然使用：

- `room:<roomId>`
- `node:<nodeId>`

这类 key 应逐步迁移为：

- `sfu:room:owner:<room_id_safe>`
- `sfu:node:meta:<node_id_safe>`

### 2. 批量命令

当前 `SCAN + MGET` 流程中，key 会被直接拼进 `MGET ...` 字符串。

这里应优先改为：

- `redisCommandArgv`

否则即使 key 部分受控，后续扩展时仍容易出错。

### 3. 录制路径

`recordDir_ + "/" + roomId`
`dir + "/" + peerId + "_" + ts + ".webm"`

这里说明：

- `roomId` / `peerId` 不只是 Redis key 问题
- 也必须做路径安全约束

### 4. WebSocket / Recorder 本地 map key

凡是使用：

```text
roomId + "/" + peerId
```

的地方，都应该纳入同一轮治理。

## 推荐的最小落地步骤

如果希望低风险分阶段推进，建议按下面顺序：

### 第一步：入口校验

对外部输入的 `roomId` / `peerId` 加最小白名单约束，例如：

```text
[A-Za-z0-9_-]{1,128}
```

这是最快速止血方案。

### 第二步：Redis key 统一命名

把：

- `room:<id>`
- `node:<id>`

迁移为：

- `sfu:room:owner:<id>`
- `sfu:node:meta:<id>`

### 第三步：替换批量命令拼接

所有 `MGET` / 多 key 命令改为 `redisCommandArgv`。

### 第四步：本地复合 key 去字符串拼接

把：

```text
roomId + "/" + peerId
```

改成结构化 key。

### 第五步：如需放宽输入范围，再引入 key 编码 / 哈希

如果业务上确实需要任意房间名或任意 peerId，再从“白名单约束”升级到“原值保留 + 安全编码”。

## 推荐结论

对 `mediasoup-cpp` 这个项目，当前最现实的方案是：

1. 入口先限制 `roomId` / `peerId` 为安全字符集
2. Redis key 统一加命名空间和语义前缀
3. 批量 Redis 操作改用参数化接口
4. 本地复合 key 不再使用字符串拼接

如果只做第 1 步，能止住一大半问题。
如果做到第 4 步，Redis key 这一块就会比较稳。
