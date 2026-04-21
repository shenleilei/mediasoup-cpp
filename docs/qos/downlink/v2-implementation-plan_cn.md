# 下行 QoS v2 分阶段实施方案

## 1. 文档目的

这份文档是 [downlink-qos-v2-design_cn.md](v2-design_cn.md) 的工程落地版本。

目标不是再解释一遍为什么要做 `v2`，而是明确：

- 先改哪一层
- 先加哪些类
- 具体要改哪些函数
- 哪些现有函数保持不动
- 每一阶段如何验收

本文尽量写到函数级别。

## 2. 当前代码基线

当前仓库已经有这些可直接复用的能力：

- 服务端接收 `downlinkClientStats`
  - [SignalingServerWs.cpp](../../../src/SignalingServerWs.cpp)
  - [RoomServiceStats.cpp](../../../src/RoomServiceStats.cpp)
- 服务端下行注册表
  - [DownlinkQosRegistry.h](../../../src/qos/DownlinkQosRegistry.h)
- 服务端下行健康状态机
  - [DownlinkHealthMonitor.h](../../../src/qos/DownlinkHealthMonitor.h)
- 服务端 `Consumer` 动作执行
  - [SubscriberQosController.h](../../../src/qos/SubscriberQosController.h)
- 服务端 `priority / preferredLayers / pause` 的 `v1` allocator
  - [DownlinkAllocator.h](../../../src/qos/DownlinkAllocator.h)
- 客户端接收 `track-scoped qosOverride`
  - [protocol.js](../../../src/client/lib/qos/protocol.js)
  - [controller.js](../../../src/client/lib/qos/controller.js)
- publisher 侧 `localTrackId + producerId` 已经在 `clientStats` 里上报
  - [types.d.ts](../../../src/client/lib/qos/types.d.ts)
  - [controller.js](../../../src/client/lib/qos/controller.js)

这意味着 `v2` 第一阶段不需要先发明新协议。

## 3. 实施原则

建议固定下面 4 条，不然后面很容易把范围做炸。

1. 先做 `server control plane`，先不改 `mediasoup-worker`
2. 先做 `track-scoped maxLevelClamp`，后做 `zero-demand pause`
3. 先把 room-scoped planner 跑通，再把显式 bitrate allocator 接进去
4. 先复用 `qosRegistry_` 里的 `producerId -> localTrackId` 映射，不先加一套新的 publisher registry

## 4. 目标调用链

`v2` 最终要形成这条链：

1. subscriber 上报 `downlinkClientStats`
2. `RoomService` 只做校验、入库、标记 room dirty
3. room-scoped planner 聚合所有 subscriber 的最新状态
4. planner 为每个 subscriber 算出 receive allocation
5. planner 再按 `producerId` 聚合成 room demand
6. planner 为 publisher 生成 `track-scoped qosOverride(maxLevelClamp)`
7. publisher controller 应用 clamp

也就是：

- subscriber side demand
- SFU side room planning
- publisher side supply clamp

## 5. 阶段 1：把当前逐条 snapshot 执行改成 room-scoped planner

### 5.1 目标

先把现在 `setDownlinkClientStats()` 里“收到一条就直接算动作”的路径，改成“标脏 + 合并调度”。

这是 `v2` 的总入口，不先做这层，后面的 producer 聚合没有地方挂。

### 5.2 需要修改的文件

- [RoomService.h](../../../src/RoomService.h)
- [RoomServiceStats.cpp](../../../src/RoomServiceStats.cpp)
- [RoomServiceDownlink.cpp](../../../src/RoomServiceDownlink.cpp)

### 5.3 `RoomService` 新增字段

建议新增：

```cpp
std::deque<std::string> pendingDownlinkRooms_;
std::unordered_set<std::string> dirtyDownlinkRooms_;
bool downlinkPlanningActive_{ false };
```

语义：

- `dirtyDownlinkRooms_`
  防止同一个 room 被重复入队
- `pendingDownlinkRooms_`
  复用现有 `continueBroadcastStats()` 的风格，逐房间处理
- `downlinkPlanningActive_`
  防止重入

### 5.4 `RoomService` 新增函数

建议新增：

```cpp
void markDownlinkRoomDirty(const std::string& roomId);
void continueDownlinkPlanning();
void runDownlinkPlanningForRoom(const std::string& roomId);
```

职责划分：

- `markDownlinkRoomDirty()`
  去重入队，必要时启动 planning loop
- `continueDownlinkPlanning()`
  模仿 `continueBroadcastStats()`，一次处理一个房间
- `runDownlinkPlanningForRoom()`
  负责本房间的真正 `v2` 规划

### 5.5 需要修改的现有函数

#### `RoomService::setDownlinkClientStats()`

当前逻辑：

- parse
- upsert
- 直接 `DownlinkAllocator::ComputeDiff(...)`
- 直接 `ctrl.applyActions(...)`

`v2` 第一阶段改成：

1. parse
2. upsert
3. `markDownlinkRoomDirty(roomId)`
4. 直接返回

也就是把“立即执行”改成“延迟到 room planner 执行”。

#### `RoomService::leave()` / `cleanupRoomResources()`

需要补清理：

- `dirtyDownlinkRooms_`
- `pendingDownlinkRooms_`
- 对应 `subscriberControllers_`
- 后续 `track override` 记录

否则房间销毁后会残留 planner 状态。

### 5.6 验收标准

- `downlinkClientStats` 仍能正常入库
- 多个 subscriber 高频上报时，不会对同一个 room 重复重算
- 房间销毁后不会继续跑 planner

## 6. 阶段 2：补 publisher track 解析链路

### 6.1 目标

让服务端能把：

- `producerId`

稳定解析成：

- `publisherPeerId`
- `localTrackId`

这样后面才能生成：

- `scope = track`
- `trackId = ...`
- `maxLevelClamp = ...`

### 6.2 先不加新协议

这一阶段先不改 `produce()` 请求格式。

直接复用现有 `clientStats`：

- `tracks[].producerId`
- `tracks[].localTrackId`

对应来源：

- [controller.js](../../../src/client/lib/qos/controller.js)
- [types.d.ts](../../../src/client/lib/qos/types.d.ts)

### 6.3 `RoomService` 新增函数

建议新增：

```cpp
std::optional<std::string> resolvePublisherTrackId(
	const std::string& roomId,
	const std::string& publisherPeerId,
	const std::string& producerId) const;

std::optional<std::string> resolveProducerOwnerPeerId(
	const std::string& roomId,
	const std::string& producerId) const;
```

推荐实现：

- `resolveProducerOwnerPeerId()`
  扫描 room 里所有 peer 的 `peer->producers`
- `resolvePublisherTrackId()`
  从 `qosRegistry_.Get(roomId, publisherPeerId)` 读取最新 `clientStats`
  然后在 `snapshot.tracks[]` 中按 `producerId` 找 `localTrackId`

### 6.4 不命中时的处理

`resolvePublisherTrackId()` 查不到时，不要硬发错误 override。

建议：

- 记录 debug log
- 本轮跳过这个 producer 的 supply clamp
- 等下一轮 planner 再试

因为最常见原因只是 publisher 尚未发出第一条 `clientStats`。

### 6.5 需要补的测试

建议新增：

- `QosIntegrationTest.ResolveTrackIdFromClientStats`
- `QosIntegrationTest.SkipTrackClampWhenPublisherTrackMissing`

## 7. 阶段 3：引入 subscriber 显式 bitrate allocator

### 7.1 目标

把当前 `DownlinkAllocator` 的“规则驱动”升级成“预算驱动”。

当前 `DownlinkAllocator` 仍保留，作为：

- `v1 fallback`
- 极端情况下的保底逻辑

### 7.2 建议新增文件

- `src/qos/SubscriberBudgetAllocator.h`
- `src/qos/SubscriberBudgetAllocator.cpp`

### 7.3 建议新增类型

```cpp
struct SubscriptionLayerOption {
	std::string consumerId;
	uint8_t spatialLayer;
	uint8_t temporalLayer;
	double estimatedBitrateBps;
	double utility;
};

struct SubscriberBudgetPlan {
	double budgetBps{ 0.0 };
	std::vector<DownlinkAction> actions;
};
```

### 7.4 `SubscriberBudgetAllocator` 建议函数

```cpp
class SubscriberBudgetAllocator {
public:
	SubscriberBudgetPlan Allocate(
		const DownlinkSnapshot& snapshot,
		const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers,
		const std::unordered_map<std::string, std::shared_ptr<mediasoup::Producer>>& producers,
		const std::vector<bool>& currentlyPaused,
		int degradeLevel) const;

private:
	double computeSubscriberBudgetBps(const DownlinkSnapshot& snapshot) const;
	double estimateLayerBitrateBps(const DownlinkSubscription& sub,
		uint8_t spatialLayer, uint8_t temporalLayer) const;
	double computeSubscriptionUtility(const DownlinkSubscription& sub) const;
	std::vector<SubscriptionLayerOption> buildLayerOptions(
		const DownlinkSubscription& sub, int degradeLevel) const;
};
```

### 7.5 关键实现约束

#### `computeSubscriberBudgetBps()`

先用保守估计：

- `availableIncomingBitrate * alpha`
- 如果没有值，回退到固定安全预算

先不要在第一版里把 estimator 写太复杂。

#### `estimateLayerBitrateBps()`

第一版允许粗估：

- 用当前 `frameWidth / frameHeight / fps`
- 配合 `spatialLayer` 做缩放
- 屏幕共享给更高基础成本

不要等真正的“精确 layer bitrate 观测”做完才启动 v2。

#### `computeSubscriptionUtility()`

直接编码优先级：

- `isScreenShare`
- `pinned`
- `activeSpeaker`
- `visible`

然后再乘：

- `targetWidth / targetHeight`
- `freezeRate` 惩罚

### 7.6 与现有 `DownlinkAllocator` 的关系

建议不要把所有逻辑直接塞进 `DownlinkAllocator::ComputeDiff()`。

更合理的方式是：

- `SubscriberBudgetAllocator` 负责“预算内选层”
- `DownlinkAllocator` 保留“动作去重 / diff / v1 fallback”

推荐新增一个桥接函数：

```cpp
std::vector<DownlinkAction> DownlinkAllocator::ComputeBudgetDiff(
	const SubscriberBudgetPlan& plan,
	std::unordered_map<std::string, ConsumerLastState>& lastState);
```

如果不想改现有类，也可以把这个函数放到新文件里，但不要把预算算法和 diff 状态混在一个函数里。

### 7.7 需要补的测试

建议新增：

- `SubscriberBudgetAllocatorTest.AllVisibleReceiveBaseLayerFirst`
- `SubscriberBudgetAllocatorTest.PinnedGetsUpgradeBeforeGrid`
- `SubscriberBudgetAllocatorTest.ScreenShareDominatesGridUnderBudget`
- `SubscriberBudgetAllocatorTest.DegradeLevelStillCapsBudgetAllocator`

## 8. 阶段 4：把 room planner 跑通 subscriber 侧 allocation

### 8.1 目标

把阶段 1 的 planner 骨架和阶段 3 的 allocator 接起来，真正让 `runDownlinkPlanningForRoom()` 对整房间 subscriber 生效。

### 8.2 建议新增文件

- `src/qos/RoomDownlinkPlanner.h`
- `src/qos/RoomDownlinkPlanner.cpp`

### 8.3 `RoomDownlinkPlanner` 建议函数

```cpp
class RoomDownlinkPlanner {
public:
	RoomDownlinkPlan PlanRoom(const RoomDownlinkPlannerInput& input);

private:
	SubscriberBudgetPlan planSubscriber(
		const SubscriberPlanningInput& input) const;
};
```

建议 `RoomDownlinkPlan` 至少包含：

- 每个 subscriber 的 `DownlinkAction[]`
- 每个 subscriber 的最终分配结果
- 每个 producer 的 aggregate demand 初稿

### 8.4 `RoomService::runDownlinkPlanningForRoom()` 建议流程

推荐流程固定成：

1. 找 room
2. 构造 `RoomDownlinkPlannerInput`
3. 对每个 subscriber 调 `ctrl.healthMonitor().update(snapshot)`
4. 调 `planner.PlanRoom(...)`
5. 对每个 subscriber 取 `SubscriberQosController`
6. `ctrl.applyActions(...)`
7. 进入 producer demand 聚合与 supply clamp

不要把这些步骤继续散落在 `setDownlinkClientStats()`。

### 8.5 需要修改的函数

#### `RoomService::setDownlinkClientStats()`

改成只做“入库 + 标脏”。

#### `RoomService::collectPeerStats()`

建议附带输出：

- `downlinkQos.health`
- `downlinkQos.degradeLevel`
- `downlinkV2.budgetBps`
- `downlinkV2.allocations[]`

后面浏览器 harness 调试时，这些字段会很值钱。

## 9. 阶段 5：做 producer demand 聚合

### 9.1 目标

把每个 subscriber 的分配结果，按 `producerId` 聚合成一份“房间真实需求”。

### 9.2 建议新增文件

- `src/qos/ProducerDemandAggregator.h`
- `src/qos/ProducerDemandAggregator.cpp`

### 9.3 建议新增类型

```cpp
struct ProducerDemandState {
	std::string producerId;
	std::string publisherPeerId;
	uint8_t maxSpatialLayer{ 0 };
	uint8_t maxTemporalLayer{ 0 };
	size_t visibleSubscriberCount{ 0 };
	size_t highPrioritySubscriberCount{ 0 };
	int64_t zeroDemandSinceMs{ 0 };
	int64_t holdUntilMs{ 0 };
};
```

### 9.4 `ProducerDemandAggregator` 建议函数

```cpp
class ProducerDemandAggregator {
public:
	void reset();
	void addSubscriberPlan(
		const std::string& subscriberPeerId,
		const DownlinkSnapshot& snapshot,
		const SubscriberBudgetPlan& plan);
	std::vector<ProducerDemandState> finalize(int64_t nowMs);

private:
	void mergeAllocation(
		const std::string& subscriberPeerId,
		const DownlinkSubscription& sub,
		const DownlinkAction* layerAction);
};
```

### 9.5 聚合规则

第一版先做最小规则：

- `maxSpatialLayer`
  取所有 subscriber 目标层的最大值
- `maxTemporalLayer`
  同理
- `visibleSubscriberCount`
  只算 `visible=true`
- `zeroDemandSinceMs`
  当没有任何 visible demand 时开始计时

不要在第一版里就做复杂公平性。

### 9.6 需要补的测试

建议新增：

- `ProducerDemandAggregatorTest.MaxLayerComesFromHighestSubscriberDemand`
- `ProducerDemandAggregatorTest.HiddenSubscriberDoesNotCreateVisibleDemand`
- `ProducerDemandAggregatorTest.ZeroDemandStartsTimer`

## 10. 阶段 6：把 aggregate demand 变成 track-scoped override

### 10.1 目标

真正跑通：

- `producer demand -> publisher clamp`

### 10.2 建议新增文件

- `src/qos/PublisherSupplyController.h`
- `src/qos/PublisherSupplyController.cpp`

### 10.3 `QosOverride` builder 需要扩展

建议直接扩展现有：

- [QosOverride.h](../../../src/qos/QosOverride.h)
- [QosOverride.cpp](../../../src/qos/QosOverride.cpp)

新增函数：

```cpp
QosOverride BuildTrackClampOverride(
	const std::string& trackId,
	uint32_t maxLevelClamp,
	uint32_t ttlMs,
	const std::string& reason);

QosOverride BuildTrackClearOverride(
	const std::string& trackId,
	const std::string& reason);
```

理由：

- 现有 builder 已经承担“自动 override 的规范化构造”
- `v2` 没必要另起一套 JSON 拼接代码

### 10.4 `PublisherSupplyController` 建议函数

```cpp
class PublisherSupplyController {
public:
	std::vector<QosOverride> BuildOverrides(
		const std::vector<ProducerDemandState>& states,
		int64_t nowMs) const;

private:
	bool shouldClamp(const ProducerDemandState& state) const;
	bool shouldClear(const ProducerDemandState& state) const;
};
```

第一版策略建议：

- 有 demand 时：
  发 `track-scoped maxLevelClamp`
- demand 恢复升高时：
  慢恢复，不要立刻拉满
- `zero demand` 时：
  先 clear 到最低层或维持短暂 hold，不立刻 pause upstream

### 10.5 `RoomService` 新增函数

建议新增：

```cpp
void applyPublisherSupplyPlan(
	const std::string& roomId,
	const std::vector<qos::ProducerDemandState>& demandStates);

void maybeSendTrackQosOverride(
	const std::string& roomId,
	const std::string& targetPeerId,
	const qos::QosOverride& overrideData);
```

`applyPublisherSupplyPlan()` 职责：

1. 遍历 `ProducerDemandState`
2. `resolveProducerOwnerPeerId()`
3. `resolvePublisherTrackId()`
4. 构造 `track-scoped override`
5. `maybeSendTrackQosOverride()`

`maybeSendTrackQosOverride()` 职责：

- 去重
- TTL 刷新
- 通过 `notify_()` 下发给 publisher peer

### 10.6 `RoomService` 新增记录表

建议新增：

```cpp
struct TrackQosOverrideRecord {
	std::string signature;
	std::string roomId;
	std::string peerId;
	std::string trackId;
	int64_t sentAtMs{ 0 };
	uint32_t ttlMs{ 0u };
};

std::unordered_map<std::string, TrackQosOverrideRecord> trackQosOverrideRecords_;
```

建议 key 格式：

- `roomId/peerId/trackId`

不要复用当前 `autoQosOverrideRecords_`。

原因：

- 当前记录表默认是 peer-scoped
- `track-scoped clear` 和 TTL 语义会混淆现有逻辑

### 10.7 `cleanupExpiredQosOverrides()` 需要拆分

建议新增：

```cpp
void cleanupExpiredTrackQosOverrides();
```

不要把 track-scoped TTL 清理硬塞进现有 `cleanupExpiredQosOverrides()` 的 key 解析逻辑里。

### 10.8 需要补的测试

建议新增：

- `QosIntegrationTest.TrackScopedClampNotifiesPublisherPeer`
- `QosIntegrationTest.TrackScopedClampUsesResolvedTrackId`
- `QosIntegrationTest.TrackScopedClampRefreshesWithoutDuplicateNoise`
- `QosIntegrationTest.TrackScopedClampClearsWhenDemandRecovers`

## 11. 阶段 7：zero-demand hold 与恢复抖动控制

### 11.1 目标

在 supply clamp 稳定后，再做真正的“无人需要时进一步收缩”。

这一阶段先不直接做 `pauseUpstream`，先做更稳的 hold 语义。

### 11.2 `ProducerDemandAggregator` 需要扩展

补：

- `zeroDemandSinceMs`
- `holdUntilMs`

并在 `finalize()` 中实现：

- demand 刚掉到 0 时，先进入 hold
- 持续多个窗口仍为 0，才允许进入更激进的供给收缩

### 11.3 `PublisherSupplyController` 需要扩展

补：

- `buildLowClampOverride(...)`
- `buildClearOrRecoverOverride(...)`

先做：

- `clamp to lowest layer`

后做：

- 若未来协议允许，再做 `pauseUpstream`

### 11.4 当前阶段先不要做

- 新建 `pauseTrack` 协议
- worker 内 forwarding 改造
- “0 demand 就立刻关编码”的硬切

## 12. 客户端侧改动范围

`v2` 第一版客户端主链路改动很少。

### 12.1 预计无需修改的运行时代码

- [protocol.js](../../../src/client/lib/qos/protocol.js)
- [controller.js](../../../src/client/lib/qos/controller.js)
- [adapters/signalChannel.js](../../../src/client/lib/qos/adapters/signalChannel.js)

原因：

- `track-scoped override`
- `maxLevelClamp`
- override merge / ttl / clear

这些都已经存在。

### 12.2 需要补的客户端测试

建议补明确的 track-scope case：

- `PublisherQosController ignores track override for other trackId`
- `PublisherQosController applies track clamp for matching trackId`
- `PublisherQosController clears matching track clamp`

对应文件：

- [test.qos.controller.js](../../../src/client/lib/test/test.qos.controller.js)

## 13. Browser harness 方案

### 13.1 建议新增文件

- `tests/qos_harness/browser/downlink-v2-entry.js`
- `tests/qos_harness/browser_downlink_v2.mjs`

### 13.2 页面需要暴露的方法

建议挂到：

- `window.__downlinkV2Harness`

至少包含：

- `initRoomWithTwoPublishersOneSubscriber()`
- `sendSubscriberSnapshots()`
- `sampleSubscriberConsumers()`
- `samplePublisherControllerState()`
- `waitForTrackClamp(trackId)`

### 13.3 第一批浏览器级 case

1. `PinnedBeatsGridUnderThrottle`
2. `ScreenShareClampPropagatesToPublisher`
3. `HiddenDemandDropsPublisherClamp`
4. `DemandRecoveryClearsTrackClamp`

### 13.4 断言重点

subscriber 侧看：

- `preferredSpatialLayer`
- `preferredTemporalLayer`
- `priority`
- `paused`

publisher 侧看：

- `controller.getActiveOverride()`
- `maxLevelClamp`
- 发送侧 `quality limitation` / `frameHeight`

不要只断言 signaling 回包成功。

## 14. 推荐提交顺序

建议按下面节奏拆 commit，不要攒一个巨型 patch：

1. `refactor: add room-scoped downlink planning loop`
2. `feat: resolve publisher track id from client stats`
3. `feat: add subscriber budget allocator`
4. `feat: add room downlink planner`
5. `feat: add producer demand aggregator`
6. `feat: add track-scoped publisher supply clamp`
7. `test: add downlink v2 browser harness`

## 15. 完成标准

`v2` 只有在下面都满足时才算完成：

1. `setDownlinkClientStats()` 已不再直接做逐条 subscriber 执行，而是进入 room planner
2. 服务端能从 publisher `clientStats` 解析出 `producerId -> localTrackId`
3. subscriber 侧分配不再只靠 `priority`，而是有显式 budget allocator
4. room demand 能稳定生成 `track-scoped maxLevelClamp`
5. 浏览器弱网竞争里，高优先级流稳定优于低优先级流
6. publisher 侧能观测到 clamp 生效，而不是只有 consumer 侧动作

## 16. 总结

`v2` 的关键不是多加几条 `setPriority()` case，而是把当前代码补成一条完整执行链：

- `downlinkClientStats -> room planner -> subscriber allocation -> producer demand -> track clamp`

只要按本文的阶段拆开做，这个目标是可以逐步验证、逐步交付的，不需要一开始就跳进 `mediasoup-worker` 深改。
