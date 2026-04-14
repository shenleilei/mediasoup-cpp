# 下行 QoS v3 分阶段实施方案

## 1. 文档目的

这份文档是 [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md) 的工程落地版本。

目标不是重复解释为什么要做 `v3`，而是把下面这些事情写清楚：

- 先改哪一层
- 每一阶段具体改哪些文件
- 新增哪些类 / 字段 / 函数
- 哪些逻辑暂时不要碰
- 每一阶段怎么验收

## 2. 当前代码基线

当前仓库里 `downlink QoS v2` 已经有这些可直接复用的能力：

- 服务端 `room-scoped` downlink planner 入口
  - [RoomService.cpp](../src/RoomService.cpp)
- subscriber 显式预算分配
  - [SubscriberBudgetAllocator.cpp](../src/qos/SubscriberBudgetAllocator.cpp)
- producer demand 聚合
  - [ProducerDemandAggregator.cpp](../src/qos/ProducerDemandAggregator.cpp)
- publisher track-scoped clamp / clear
  - [PublisherSupplyController.cpp](../src/qos/PublisherSupplyController.cpp)
- publisher 侧 track-scoped override merge / ttl / clear
  - [controller.js](../src/client/lib/qos/controller.js)
- browser v2 harness
  - [browser_downlink_v2.mjs](../tests/qos_harness/browser_downlink_v2.mjs)
- downlink 报告骨架
  - [run_qos_tests.sh](../scripts/run_qos_tests.sh)

这意味着 `v3` 不需要重新做：

- room-scoped planner 骨架
- track-scoped `maxLevelClamp`
- `downlink_v2_zero_demand_hold`
- 基本的 track clear

`v3` 应该只聚焦 `v2` 刻意延后的能力。

## 3. 实施原则

建议固定下面 5 条，不然后面很容易重新把范围做炸。

1. 先补 `zero-demand pause / resume` 这条控制闭环，再做更复杂策略
2. 先把 planner 变成真正的 coalesced tick，再调 pause / resume 阈值
3. 先补 browser / integration 验证，再决定是否需要 worker 改造
4. `downlink` 的 matrix 要和 `uplink` 对齐，但断言对象必须不同
5. 只有明确发现 client / SFU control plane 不够，才进入 worker 深改

这里需要明确：

- `v3` 并不是排斥改 worker 源代码
- `mediasoup-cpp` 目录下本身就有 worker 相关代码入口，例如：
  - [src/Worker.cpp](../src/Worker.cpp)
  - [src/Worker.h](../src/Worker.h)
  - 仓库根目录下的 `mediasoup-worker`

所以“worker 改造”是 `v3` 的正式实现选项，只是不应该在没有黑盒证据时提前展开。

## 4. v3 目标调用链

`v3` 最终要形成这条链：

1. subscriber 上报 `downlinkClientStats`
2. `RoomService` 标记 room dirty
3. per-room coalesced tick 统一执行 planner
4. planner 对每个 subscriber 计算 receive allocation
5. planner 按 `producerId` 聚合 aggregate demand
6. producer demand state machine 决定：
   - `maxLevelClamp`
   - `pauseUpstream`
   - `resumeUpstream`
7. publisher 侧 controller / adapter 应用：
   - clamp
   - pause
   - resume
8. browser harness / report 验证这条链是否稳定

也就是：

- subscriber side demand
- SFU side room planning
- publisher side supply clamp
- publisher side upstream pause / resume

## 5. 阶段 1：把 planner 升级成真正的房间级 tick

### 5.1 目标

把当前“收到 snapshot 就尽快排队执行”的机制，升级成真正的：

- room-scoped dirty
- coalesced tick
- fixed minimum planning interval

这是 `v3` 的总入口，不先做这层，后面的 pause / resume 很容易振荡。

### 5.2 需要修改的文件

- [RoomService.h](../src/RoomService.h)
- [RoomService.cpp](../src/RoomService.cpp)

### 5.3 `RoomService` 新增字段

建议新增：

```cpp
struct DownlinkRoomPlanState {
	int64_t lastPlannedAtMs{ 0 };
	int64_t nextEligiblePlanAtMs{ 0 };
	uint32_t pendingReasonMask{ 0u };
};

std::unordered_map<std::string, DownlinkRoomPlanState> downlinkRoomPlanStates_;
int64_t downlinkPlanningIntervalMs_{ 100 };
```

说明：

- `lastPlannedAtMs`
  记录最近一次实际规划时间
- `nextEligiblePlanAtMs`
  防止一个 room 在很短时间内被重复计算
- `pendingReasonMask`
  记录是 stats 更新、leave、reconnect、publisher mapping 变化等哪类事件触发
- `downlinkPlanningIntervalMs_`
  建议先从 `100ms` 起步，必要时可调到 `150-200ms`

### 5.4 `RoomService` 需要调整的函数

建议保留现有函数名，但修改语义：

```cpp
void markDownlinkRoomDirty(const std::string& roomId, uint32_t reasonMask = ...);
void continueDownlinkPlanning();
void runDownlinkPlanningForRoom(const std::string& roomId);
```

目标行为：

- `markDownlinkRoomDirty()`
  只负责标记 dirty，不承诺立刻跑
- `continueDownlinkPlanning()`
  按时间窗统一挑选当前可执行的 room
- `runDownlinkPlanningForRoom()`
  只做真正规划，不再承担节流逻辑

### 5.5 当前阶段先不要做

- 复杂的 cross-room fairness
- 多线程并发执行多个 room planner
- 基于房间规模动态调整 interval

### 5.6 验收标准

- 高频 `downlinkClientStats` 不会导致同一个 room 被每条消息重算
- `leave/reconnect` 后仍能及时触发一次规划
- 同一房间多 subscriber 在一个 tick 窗口内的变化能统一看见

## 6. 阶段 2：把 producer demand 升级成 pause / resume 状态机

### 6.1 目标

在当前 `zeroDemandSinceMs / holdUntilMs` 的基础上，扩展出真正可驱动 `pauseUpstream / resumeUpstream` 的 producer 状态机。

### 6.2 需要修改的文件

- [ProducerDemandAggregator.h](../src/qos/ProducerDemandAggregator.h)
- [ProducerDemandAggregator.cpp](../src/qos/ProducerDemandAggregator.cpp)
- [PublisherSupplyController.h](../src/qos/PublisherSupplyController.h)
- [PublisherSupplyController.cpp](../src/qos/PublisherSupplyController.cpp)

### 6.3 `ProducerDemandState` 需要扩展

建议新增：

```cpp
enum class ProducerSupplyState {
	kActive,
	kLowClamped,
	kZeroDemandHold,
	kPaused,
	kResumeWarmup,
};

ProducerSupplyState supplyState{ ProducerSupplyState::kActive };
int64_t lastNonZeroDemandAtMs{ 0 };
int64_t pauseEligibleAtMs{ 0 };
int64_t resumeEligibleAtMs{ 0 };
int64_t lastPauseSentAtMs{ 0 };
int64_t lastResumeSentAtMs{ 0 };
```

### 6.4 `ProducerDemandAggregator::finalize()` 需要实现

在当前聚合结果上补：

- demand 刚掉到 0
  - `kZeroDemandHold`
- hold 超时仍为 0
  - `kPaused`
- paused 后 demand 恢复
  - `kResumeWarmup`
- warmup 稳定
  - `kActive` / `kLowClamped`

建议先用简单时间窗：

- `holdMs = 2000`
- `pauseConfirmMs = 4000`
- `resumeWarmupMs = 1000`

后面再微调，不要一开始就调很多参数。

### 6.5 `PublisherSupplyController` 需要扩展

建议新增：

```cpp
QosOverride BuildTrackPauseOverride(...);
QosOverride BuildTrackResumeOverride(...);
bool shouldPause(const ProducerDemandState& state) const;
bool shouldResume(const ProducerDemandState& state) const;
```

优先级建议：

1. 如果需要 `pause`
   先于 low clamp / clear
2. 如果需要 `resume`
   先恢复 upstream，再决定 clamp 到几层
3. `resume` 后不要立刻 clear 到满层
   先走 warmup / 低层恢复

### 6.6 当前阶段先不要做

- 一次 pause 多条 track 的 batch API
- 同时对 audio / video 一起建复杂联合状态机
- 根据 codec 类型定制不同 pause 规则

### 6.7 验收标准

- 持续 zero-demand 时会从 hold 进入 paused
- paused 后 demand 恢复时会明确进入 resume 路径
- 不会因为单个 sample 的 visible 抖动导致 pause / resume 来回翻

## 7. 阶段 3：补齐协议与客户端 publisher 控制

### 7.1 目标

把服务端的 supply state machine 真正翻译成 publisher 侧可执行动作。

### 7.2 需要修改的文件

- [protocol.js](../src/client/lib/qos/protocol.js)
- [controller.js](../src/client/lib/qos/controller.js)
- [types.d.ts](../src/client/lib/qos/types.d.ts)
- 如有需要：
  - [producerAdapter.js](../src/client/lib/qos/adapters/producerAdapter.js)
  - [executor.js](../src/client/lib/qos/executor.js)

### 7.3 协议建议

如果现有 `qosOverride` schema 字段不够，建议补：

```json
{
  "schema": "mediasoup.qos.override.v1",
  "scope": "track",
  "trackId": "camera-main",
  "pauseUpstream": true,
  "ttlMs": 5000,
  "reason": "downlink_v3_zero_demand_pause"
}
```

和：

```json
{
  "schema": "mediasoup.qos.override.v1",
  "scope": "track",
  "trackId": "camera-main",
  "resumeUpstream": true,
  "ttlMs": 5000,
  "reason": "downlink_v3_demand_resumed"
}
```

### 7.4 `PublisherQosController` 需要补的语义

当前已支持：

- track-scoped `maxLevelClamp`
- ttl / clear / merge

`v3` 需要补：

- `pauseUpstream`
- `resumeUpstream`
- `resume` 后优先触发 keyframe / warmup
- `downlink_v2_*` / `downlink_v3_*` 与 manual override 的共存规则

### 7.5 需要补的客户端测试

至少补这些：

- matching track 上 `pauseUpstream` 被应用
- other track 的 `pauseUpstream` 被忽略
- `resumeUpstream` 不清掉不相关 manual override
- paused -> resume 后 actionExecutor 顺序正确

### 7.6 当前阶段先不要做

- 在 controller 里把 uplink / downlink override merge 成一个大状态机
- 自动把 `pauseUpstream` 映射成 `forceAudioOnly`
- 对所有 source 类型做特殊化

### 7.7 验收标准

- 服务端发 `pauseUpstream` 时，客户端能执行真实 pause
- 服务端发 `resumeUpstream` 时，客户端能执行真实 resume
- manual override 与 `downlink_v3_*` 不互相误清

## 8. 阶段 4：做 worker / data plane 黑盒验证

### 8.1 目标

在决定是否深改 worker 之前，先把下面这些问题用黑盒方式跑明白：

1. `pauseUpstream` 后浏览器 sender 是否真的停发 RTP
2. `resumeUpstream` 后首帧恢复时间是多少
3. resume 后是否必须显式请求 keyframe
4. 当前 producer / transport API 是否足以实现目标

这里的“worker”不是抽象概念，而是包括：

- `mediasoup-cpp` 仓库中的 worker 相关 C++ 代码
- 仓库实际运行使用的 `mediasoup-worker`

阶段 4 的目标不是继续重复“先不改 worker”，而是形成明确结论：

- 当前 control plane 已足够
  或
- 必须进入 worker 源代码修改

### 8.2 输出形式

建议单独补一份验证文档，例如：

- `docs/downlink-qos-v3-worker-validation_cn.md`

文档至少回答：

- 哪些行为现有 API 已足够
- 哪些行为必须进入 worker 侧
- 哪些只是 browser 行为差异，不应归咎于 worker

### 8.3 当前阶段先不要做

- 没有黑盒证据就直接改 worker forwarding
- 直接引入大规模 C++ / worker API 重构

### 8.4 验收标准

- 能明确写出“需要 worker 改造”还是“当前 control plane 已足够”
- 如果需要 worker 改造，能明确写出：
  - 改哪一层
  - 为什么 client / adapter 层无法单独解决
  - 预期修改的是 `mediasoup-cpp` 内哪类 worker 代码 / 运行时行为

## 9. 阶段 5：补齐 downlink matrix 与报告体系

### 9.1 目标

让 `downlink` 拥有和 `uplink` 同级别的：

- case matrix
- full / targeted 模式
- case report
- archive / metadata

### 9.2 需要修改的文件

- [run_qos_tests.sh](../scripts/run_qos_tests.sh)
- `tests/qos_harness/run_downlink_matrix.mjs`
- `tests/qos_harness/browser/downlink-matrix-entry.js`
- `tests/qos_harness/render_downlink_case_report.mjs`
- 或者在现有 report pipeline 上加 `downlink` 维度

### 9.3 downlink matrix case 组

建议至少有：

1. `baseline`
2. `bw_sweep`
3. `loss_sweep`
4. `rtt_sweep`
5. `jitter_sweep`
6. `transition`
7. `competition`
8. `zero_demand`

### 9.4 case 断言重点

不要只断言状态字符串。

至少断言：

- consumer:
  - `paused`
  - `preferredSpatialLayer`
  - `preferredTemporalLayer`
  - `priority`
- publisher:
  - active override
  - `maxLevelClamp`
  - `pauseUpstream`
  - `resumeUpstream`
- 恢复：
  - first resumed frame
  - first stable receive layer
  - 是否发生振荡

### 9.5 报告产物建议

建议和 uplink 对齐成：

- `docs/downlink-qos-case-results.md`
- `docs/generated/downlink-qos-case-results.targeted.md`
- `docs/generated/downlink-qos-matrix-report.json`
- `docs/generated/downlink-qos-matrix-report.targeted.json`
- `docs/archive/downlink-qos-runs/<timestamp>/...`
- `metadata.json`

### 9.6 验收标准

- full / targeted 都能生成 markdown 报告
- archive / metadata 结构和 uplink 对齐
- recovery / loss / RTT / jitter / competition / zero-demand 都有可点击 case 结果

## 10. 推荐提交顺序

建议按下面节奏拆 commit，不要攒一个超大 patch：

1. `refactor: add coalesced downlink planner tick`
2. `feat: add producer zero-demand pause state machine`
3. `feat: add track-scoped pauseUpstream and resumeUpstream override`
4. `feat: apply publisher pause and resume in client qos controller`
5. `test: add downlink pause-resume integration coverage`
6. `test: add downlink matrix and report pipeline`

## 11. 完成标准

`v3` 只有在下面都满足时才算完成：

1. room planner 已经是 coalesced tick，不是单纯事件驱动
2. 持续 zero-demand 后，publisher 不只是低层，而是真正 `pauseUpstream`
3. demand 恢复后，publisher 能 `resumeUpstream`，且恢复不明显抖动
4. `downlink` 已有和 `uplink` 对齐的 matrix / report / archive
5. 可以用测试和报告直接回答：
   - planner 是否稳定
   - demand 聚合是否正确
   - publisher pause / resume 是否正确
   - 恢复为什么慢 / 为什么抖

## 12. 总结

`v2` 的主题是：

- room demand 可见
- publisher clamp 可用

`v3` 的主题则应该是：

- zero-demand pause / resume 可控
- planner 抗抖可签收
- downlink matrix 与报告体系成型

只要按本文阶段拆开做，`v3` 可以逐步验证、逐步交付，不需要一开始就跳进 `mediasoup-worker` 深改。
