# Linux PlainTransport C++ Client 线程化实施清单

## 1. 文档定位

这份文档是当前轮次的实施清单。

它不是完整架构设计说明，而是从
[linux-client-multi-source-thread-model_cn.md](./linux-client-multi-source-thread-model_cn.md)
里抽出来的、当前确认要落地的执行任务列表。

当前轮次关注过下面这些闭环：

1. 命令/ack 协议闭环
2. `NetworkThread` 事件循环闭环
3. pacing 闭环
4. 旧消息过滤机制
5. 测试补齐
6. 文档状态同步

当前轮次**不包含**：

- 独立 `demux thread`
- `SharedDecodeMultiTrackMode`
- 单文件多 track 的共享 decode 实现
- 低配模式 / worker 合并策略

这些项仍然保留在主设计文档里，但不属于这轮实施范围。

## 2. 当前状态摘要

这份清单当前已经不再是“待做列表”，而是“本轮完成情况 + 剩余项”。

当前实现已经落下来的主线包括：

- `commandId` / `CommandAck` / `PendingCommand` 基础闭环
- `NetworkThread` 的 `epoll + eventfd + timerfd`
- 基础 pacing 队列
- 基于 `configGeneration` 的旧 AU 过滤
- 一批关键协议边界测试：
  - queue-full 不创建 pending command
  - pause/resume ack 驱动状态推进
  - stale track 采样门控
  - `commandId` 错配忽略
  - generation 过滤

当前仍未纳入本轮范围的项保持不变：

- 独立 `demux thread`
- `SharedDecodeMultiTrackMode`
- 单文件多 track 的共享 decode 实现
- 低配模式 / worker 合并策略

如果要看当前仍缺失的测试覆盖，请继续看：

- [linux-client-threaded-test-gap-checklist_cn.md](./linux-client-threaded-test-gap-checklist_cn.md)

## 3. 本轮完成情况

### P0 命令/ack 协议闭环

状态：`⚠️ 基础闭环已落地`

已落地：

- `TrackControlCommand` 已带：
  - `commandId`
  - `issuedAtMs`
  - `configGeneration`
- `CommandAck` 已带：
  - `commandId`
  - `appliedAtMs`
  - `reason`
  - `configGeneration`
- `control thread` 已维护 per-track pending command 状态
- `QosController` 已改成 ack 驱动的 `confirmAction()`

仍需后续补强：

- 更严格的失败语义
- 更完整的 pending 生命周期
- 更细的协议边界验证

### P1 `NetworkThread` 事件循环闭环

状态：`✅ 已实现`

已落地：

- `epoll_wait`
- `eventfd`
- `timerfd`
- UDP socket 可读事件统一进 network loop

### P2 pacing 闭环

状态：`✅ 已实现基础版本`

已落地：

- packet-level pacing queue
- 定时 flush
- 基础 burst 控制

仍可后续优化：

- pacing 参数调优
- 更精细的 deadline 策略

### P3 旧消息过滤机制

状态：`⚠️ 基础版本已落地`

已落地：

- `configGeneration` 进入 `TrackControlCommand` / `CommandAck` / `EncodedAccessUnit`
- network 对旧 generation 的 AU 做过滤
- generation 切换时清理 keyframe cache / retransmission store

仍可后续优化：

- 更完整的 ack 过滤策略
- 更严格的 generation 迁移校验

### P4 测试补齐

状态：`⚠️ 线程化专项测试已补一部分`

已落地：

- stale stats 过滤测试
- PLI 路由测试
- rejected ack 重试测试
- generation 过滤测试
- pacing 行为测试

仍可后续补充：

- 更复杂的乱序/失败场景
- 多 source / 多 track 更重的集成测试

### P5 文档状态同步

状态：`进行中`

已做：

- 主设计文档增加状态表
- 当前清单与主设计文档建立入口链接

仍需继续保持：

- 每次状态变化都同步更新

## 4. 剩余项

当前真正剩下的项，按重要性看主要是：

1. 把文档状态写实，不再把“基础骨架”写成“最终闭环”
2. 继续补更复杂的乱序/失败/多 source 集成测试
3. 低配模式与 worker 合并策略

`demux thread` 和 `SharedDecodeMultiTrackMode` 仍然是未来工作，不属于这轮的剩余项。
