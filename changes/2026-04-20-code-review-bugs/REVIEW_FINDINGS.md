# mediasoup-cpp 代码审查 — 问题清单

> **审查时间**: 2026-04-20
> **审查范围**: `src/RoomRegistry.cpp`, `src/SignalingServerWs.cpp`, `src/RoomServiceStats.cpp`,
> `src/RoomServiceLifecycle.cpp`, `src/RoomStatsQosHelpers.h`, `src/qos/QosValidator.cpp`,
> `src/qos/DownlinkQosRegistry.cpp`, `src/SignalingSocketState.h`, `src/WorkerThread.cpp`

---

## Bug 1 — `syncNodesUnlocked` / `syncAllUnlocked` 缩进陷阱（高风险维护隐患）

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟡 中 — 当前行为恰好正确，但极易在日后维护时引入 bug |
| **文件** | `src/RoomRegistry.cpp` 第 659-684 行、686-738 行 |
| **类型** | 危险缩进 / 缺少大括号 |

### 问题描述

`for` 循环没有使用大括号，但其后续代码按照深缩进排列，看起来像循环体的一部分：

```cpp
// 第 666-668 行
for (auto& key : nodeKeys)
    nids.push_back(key.substr(9));      // ← for 循环体在这一行结束
    auto* mr = mgetArgv(nodeKeys);      // ← 看起来在循环内，实际不是！
    if (mr && mr->type == REDIS_REPLY_ARRAY) {
        ...
    }
    if (mr) freeReplyObject(mr);
}
```

C++ 编译器将 `for` 循环体解析为仅 `nids.push_back(key.substr(9));` 一行（因为没有大括号），
后续的 `mgetArgv`、`freeReplyObject` 等调用只执行一次——这恰好是**正确**行为。

但任何开发者阅读此代码都会误认为 `mgetArgv` 在循环内被反复调用，维护修改时极易出错。

**同一问题在 `syncAllUnlocked` 中出现了三次**（nodes 同步 + rooms 同步）。

### 建议修复

为 `for` 循环添加大括号，并将 `mgetArgv` 调用移到循环之后、纠正缩进：

```cpp
for (auto& key : nodeKeys) {
    nids.push_back(key.substr(9));
}
auto* mr = mgetArgv(nodeKeys);
// ...
```

---

## Bug 2 — `downlinkClientStats` 双重解析（性能浪费 + 一致性风险）

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟡 中 — 浪费 CPU 且架构不对等 |
| **文件** | `src/SignalingServerWs.cpp` 第 203 行；`src/RoomServiceStats.cpp` 第 127 行 |
| **类型** | 冗余操作 / 设计不一致 |

### 问题描述

**Signaling 线程**（第一次解析）：

```cpp
// SignalingServerWs.cpp:203
auto dlParse = qos::QosValidator::ParseDownlinkSnapshot(data);  // 解析 #1
// ... 用 dlParse.value.seq 做 rate limiting ...
// 然后丢弃解析结果，把原始 json 传给 worker:
wt->post([wt, roomId, peerId, data] {
    wt->roomService()->setDownlinkClientStats(roomId, peerId, data);
});
```

**Worker 线程**（第二次解析）：

```cpp
// RoomServiceStats.cpp:127
auto parsed = qos::QosValidator::ParseDownlinkSnapshot(stats);  // 解析 #2
```

同一份 downlink snapshot 被完整解析了两次。`ParseDownlinkSnapshot` 内部调用了
`payload.dump()` 来检查负载大小（`QosValidator.cpp:423`），对大 payload 是显著的 CPU 浪费。

**对比 `clientStats` 的正确处理方式**——只解析一次，将解析结果 move 到 worker：

```cpp
// clientStats（正确做法）
auto qosParse = qos::QosValidator::ParseClientSnapshot(data);
wt->post([..., snapshot = std::move(qosParse.value)]() mutable {
    wt->roomService()->setClientStats(roomId, peerId, std::move(snapshot));
});
```

### 建议修复

1. 在 signaling 线程将 `dlParse.value` move 到 worker 线程的 lambda 中
2. 将 `setDownlinkClientStats` 的签名改为接收 `qos::DownlinkSnapshot` 而非 `const json&`
3. 在 worker 线程内跳过重复解析，直接使用已解析的 snapshot

---

## Bug 3 — rate limiter 提前更新 `lastAcceptedSeq`（逻辑错误）

| 项目 | 值 |
|------|-----|
| **严重程度** | 🔴 高 — 可导致合法数据被错误丢弃 |
| **文件** | `src/SignalingServerWs.cpp` 第 226-232 行 |
| **类型** | 状态一致性 / 时序错误 |

### 问题描述

```cpp
// SignalingServerWs.cpp:226-232
if (advancingSeq) {
    state.pending = true;
    state.pendingSinceMs = nowMs;
    state.pendingSeq = seq;
    state.lastAcceptedSeq = seq;   // ← 在 worker 尚未处理前就标记为 "accepted"
    state.hasAcceptedSeq = true;
}
```

`lastAcceptedSeq` 在 **worker 线程处理之前**就被更新为当前 seq。但 worker 线程可能拒绝
这个 snapshot（`DownlinkQosRegistry::Upsert` 返回 false — `stale-seq` 或 `stale-ts`）。

**后果**：
- 被 worker 拒绝的 seq 仍被 signaling 线程视为 "accepted"
- 后续真正合法的 snapshot 如果 seq 值介于 "假 accepted" 和实际需要的值之间，
  会被 `IsAdvancingDownlinkSeq` 错误判定为非 advancing，从而被静默丢弃

**对比**：`downlinkSnapshotApplied` 回调（第 66-75 行）正确地在 worker 处理完成后才清除
`pending` 状态——但 `lastAcceptedSeq` 的更新与之不一致。

### 建议修复

1. signaling 线程仅更新 `pending` / `pendingSeq` / `pendingSinceMs`
2. 将 `lastAcceptedSeq` 和 `hasAcceptedSeq` 的更新移到 `downlinkSnapshotApplied` 回调中，
   仅在 worker 确认处理成功后更新

---

## Bug 4 — `leave()` / `join(reconnect)` 未清除 room-pressure override records

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟡 中 — 内存泄漏 + 可能发送 override 到已不存在的 peer |
| **文件** | `src/RoomServiceLifecycle.cpp` 第 118 行、第 70 行 |
| **类型** | 状态清理遗漏 |

### 问题描述

`autoQosOverrideRecords_` 使用两种 key 格式：

| key 类型 | 格式 | 创建位置 |
|----------|------|----------|
| peer-level | `roomId/peerId` | `maybeSendAutomaticQosOverride` (第 296 行) |
| room-pressure | `roomId/peerId#room` | `maybeSendRoomPressureOverrides` (第 405 行) |

在 `leave()` 中：

```cpp
autoQosOverrideRecords_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
// ← 只清除了 "roomId/peerId"，未清除 "roomId/peerId#room"
```

在 `join(reconnect)` 中同样只清除 peer-level key（第 70 行）。

**后果**：在多人房间中，离开或重连的 peer 对应的 room-pressure record 会一直残留，
直到 TTL 过期或房间销毁。在此期间，`maybeSendRoomPressureOverrides` 可能发送
重复的 clear/override 通知到无效 peer。

### 建议修复

在 `leave()` 和 `join(reconnect)` 中追加清除 room-pressure key：

```cpp
autoQosOverrideRecords_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
autoQosOverrideRecords_.erase(roomstatsqos::MakeRoomPressureKey(roomId, peerId));
```

---

## Bug 5 — `claimRoom` 缓存 TOCTOU 竞争

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟡 中 — 极端场景下客户端被导向已死节点 |
| **文件** | `src/RoomRegistry.cpp` 第 247-253 行 |
| **类型** | 竞争条件 (TOCTOU) |

### 问题描述

```cpp
{
    std::lock_guard<std::mutex> cl(cacheMutex_);
    auto it = roomCache_.find(roomId);
    if (it != roomCache_.end() && !it->second.empty()) {
        if (it->second == nodeAddress_) return "";
        return it->second;   // ← 基于可能已过期的缓存做 redirect
    }
}
// 缓存未命中时才走 Redis EVAL
```

如果缓存中某远程节点地址尚未被 `evictDeadNodes` 清除（只在 heartbeat 定期执行，
间隔约 10 秒），客户端会被错误 redirect 到一个已经宕机的节点。

### 建议修复

`claimRoom` 在缓存命中远程节点时，增加一个 lightweight 的 Redis EXISTS 检查来验证该节点
是否仍然存活，或者在缓存条目中增加 TTL / 过期时间检查。

---

## Bug 6 — `setDownlinkClientStats` 静默忽略 room/peer 不存在（可调试性差）

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟢 低 — 不影响正确性，但增加调试难度 |
| **文件** | `src/RoomServiceStats.cpp` 第 122-125 行 |
| **类型** | 缺少日志 |

### 问题描述

```cpp
void RoomService::setDownlinkClientStats(...) {
    auto room = roomManager_.getRoom(roomId);
    if (!room) return;       // 静默返回，没有日志
    auto peer = room->getPeer(peerId);
    if (!peer) return;       // 静默返回，没有日志
```

当 peer 在 `leave` 和 `post` 之间的竞争窗口中被移除时，这些 silent return
会导致调试困难——客户端认为发送成功（已收到 ok 响应），但数据被静默丢弃。

### 建议修复

添加 `MS_DEBUG` 日志：

```cpp
if (!room) {
    MS_DEBUG(logger_, "[{} {}] setDownlinkClientStats: room gone", roomId, peerId);
    return;
}
if (!peer) {
    MS_DEBUG(logger_, "[{} {}] setDownlinkClientStats: peer gone", roomId, peerId);
    return;
}
```

---

## Bug 7 — `heartbeatRegistry()` 定义但未使用（死代码）

| 项目 | 值 |
|------|-----|
| **严重程度** | 🟢 低 — 无运行时影响，仅代码卫生 |
| **文件** | `src/RoomServiceStats.cpp` 第 205-212 行；`src/RoomService.h` 第 138 行 |
| **类型** | 死代码 |

### 问题描述

`RoomService::heartbeatRegistry()` 已定义并声明，但全局搜索显示**没有任何调用点**。
实际的 Redis heartbeat 由 `SignalingServerHttp.cpp:277` 直接调用 `registry_->heartbeat()`
在 signaling 主线程执行。

`heartbeatRegistry()` 方法在设计上如果从 worker 线程调用，会导致 blocking Redis I/O
阻塞整个 worker event loop（`registry_->heartbeat()` 持有 `cmdMutex_` 并做多次 Redis 命令），
但当前不会触发此问题因为该方法从未被调用。

### 建议修复

删除 `heartbeatRegistry()` 的声明和定义，或者如果未来需要，标注 `[[maybe_unused]]`
并添加注释说明其线程安全约束。

---

## 汇总表

| # | 严重程度 | 问题 | 文件 | 类型 |
|---|---------|------|------|------|
| 1 | 🟡 中 | `syncNodesUnlocked` / `syncAllUnlocked` 缩进陷阱 | `RoomRegistry.cpp` | 危险缩进 |
| 2 | 🟡 中 | `downlinkClientStats` 双重解析 | `SignalingServerWs.cpp` + `RoomServiceStats.cpp` | 冗余操作 |
| 3 | 🔴 高 | rate limiter 提前更新 `lastAcceptedSeq` | `SignalingServerWs.cpp` | 时序错误 |
| 4 | 🟡 中 | `leave`/`join` 未清 room-pressure records | `RoomServiceLifecycle.cpp` | 状态清理遗漏 |
| 5 | 🟡 中 | `claimRoom` 缓存 TOCTOU | `RoomRegistry.cpp` | 竞争条件 |
| 6 | 🟢 低 | `setDownlinkClientStats` 静默 return | `RoomServiceStats.cpp` | 缺少日志 |
| 7 | 🟢 低 | `heartbeatRegistry()` 死代码 | `RoomServiceStats.cpp` | 死代码 |

### 优先修复建议

1. **首先修复 Bug 3**（rate limiter seq 逻辑错误）— 这是唯一可能导致运行时行为不正确的 bug
2. **然后修复 Bug 2 和 4** — 性能影响 + 状态清理问题
3. **最后处理 Bug 1, 5, 6, 7** — 代码卫生和可维护性改善
