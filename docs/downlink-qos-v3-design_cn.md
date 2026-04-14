# 下行 QoS v3 设计方案

## 1. 文档目的

这份文档定义当前仓库 `downlink QoS v3` 的设计边界。

这里说的 `v3`，不是重新发明一套新的 downlink QoS，而是在已经落地的 `v2` 之上继续补齐两类能力：

- 从“能观察房间 demand，并把 publisher clamp 到低层”升级到“持续 zero-demand 时真正收缩上游发送”
- 从“有若干 downlink browser case”升级到“有和 uplink 同级别的弱网 / 恢复矩阵与签收口径”

前置文档：

- [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)
- [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md)
- [uplink-qos-design_cn.md](./uplink-qos-design_cn.md)

## 2. 当前 v2 已完成什么

当前仓库里的 `downlink QoS v2` 已经形成一条基本闭环：

1. subscriber 上报 `downlinkClientStats`
2. `RoomService` 按 room 聚合 downlink snapshot
3. `SubscriberBudgetAllocator` 为 subscriber 计算 receive allocation
4. `ProducerDemandAggregator` 聚合出 producer demand
5. `PublisherSupplyController` 生成 track-scoped `qosOverride(maxLevelClamp)`
6. publisher 侧 controller 应用 clamp / clear

当前已存在的关键代码：

- [RoomService.cpp](../src/RoomService.cpp)
- [SubscriberBudgetAllocator.cpp](../src/qos/SubscriberBudgetAllocator.cpp)
- [ProducerDemandAggregator.cpp](../src/qos/ProducerDemandAggregator.cpp)
- [PublisherSupplyController.cpp](../src/qos/PublisherSupplyController.cpp)
- [RoomDownlinkPlanner.cpp](../src/qos/RoomDownlinkPlanner.cpp)
- [controller.js](../src/client/lib/qos/controller.js)
- [browser_downlink_v2.mjs](../tests/qos_harness/browser_downlink_v2.mjs)

这意味着 `v3` 不该再去重复解决：

- room-scoped planning 骨架
- track-scoped `maxLevelClamp`
- 基本的 zero-demand hold
- 基本的 track clear

`v3` 要解决的，是 `v2` 明确延后但仍然有价值的那部分。

## 3. 为什么需要 v3

`v2` 的主价值，是把“房间级 demand -> publisher 低层 clamp”这条链路跑通。

但它还有 4 个结构性缺口。

### 3.1 还没有真正做到 zero-demand pause / resume

当前 zero-demand 的处理方式仍然是：

- 进入 hold
- 把 publisher clamp 到最低层

而不是：

- 持续 zero-demand 后真正暂停上游发送
- demand 恢复后再恢复发送

因此当前仍然存在：

- publisher 仍在发最低层
- 没有人看时，上游编码与带宽没有被真正释放
- 长时间 hidden / background 场景下，节省的只是高层，不是整个 video upstream

### 3.2 还没有完整的恢复防抖规则

`v2` 已经有：

- duplicate suppression
- zero-demand hold
- stale snapshot 不回退 demand

但还没有严格定义：

- pause -> resume 时的恢复窗口
- resume 后是否先请求 keyframe、如何 debounce
- resize / hidden / restore 高频切换时的统一节流规则
- planner 的固定 tick / coalescing 周期

这意味着 `v2` 在复杂交互下“能工作”，但还不够“可预测、可签收”。

### 3.3 还没有 worker / data plane 层面的明确能力边界

`v2` 刻意不改 `mediasoup-worker`，这是对的。

但到了 `v3`，必须回答一个明确问题：

- 现有 control plane 是否已经足够实现 `pauseUpstream / resumeUpstream`
- 如果不够，缺的是协议字段、client adapter，还是 worker forwarding 行为

如果这个边界不先明确，后面的实现会反复卡在：

- “服务端想 pause”
- “客户端本地标记 pause 了”
- “worker 仍在向下游 / 上游保持某些行为”

这里需要明确一点：

- `v3` 不是原则上禁止改 worker
- `mediasoup-cpp` 目录下本身就有 worker 相关代码入口，例如：
  - [src/Worker.cpp](../src/Worker.cpp)
  - [src/Worker.h](../src/Worker.h)
  - 仓库根目录下的 `mediasoup-worker`

也就是说，`v3` 完全可以把“修改 worker 源代码”纳入正式范围；
只是这件事应该在证据驱动的前提下进入，而不是一开始就默认它是第一步。

### 3.4 还没有和 uplink 对齐的 downlink matrix

当前 `uplink QoS` 已经有：

- baseline / bw / loss / RTT / jitter / transition / burst matrix
- full / targeted 报告
- 归档与 metadata

`downlink QoS` 当前虽然已经有 browser harness，但还没有同等级的签收矩阵。

这会导致两个问题：

- 无法系统回答“downlink 在 loss / RTT / jitter / recovery 下到底表现怎样”
- 每次回归更多依赖局部 case，而不是完整矩阵与报告

## 4. v3 目标与非目标

## 4.1 目标

`v3` 需要完成这 6 件事：

1. 为 publisher 引入真正的 `pauseUpstream / resumeUpstream` 语义
2. 为 room planner 引入更稳定的 tick / coalescing / hysteresis 规则
3. 为 zero-demand -> pause -> resume 定义完整状态机
4. 明确 client / SFU / worker 三层的职责边界
5. 设计与 uplink 对齐的 downlink 弱网 / 恢复矩阵
6. 建立可归档、可回归的 downlink 报告格式与签收口径

## 4.2 非目标

`v3` 先不做这些：

- 全局最优 multicast / simulcast packing 求解器
- 多 codec / 多 SVC profile 的复杂特化策略
- 把 uplink / downlink controller 重写成一套统一大状态机
- 一开始就深改所有 worker forwarding 路径
- 追求“所有 case 永不振荡”的理想化控制

## 5. 设计原则

### 5.1 先补完整闭环，再补复杂策略

`v3` 第一优先级不是“更聪明地决定哪一路要 S1 还是 S2”，而是：

- 长时间 zero-demand 时真正停发
- 恢复时可预测
- matrix 能签收

先把闭环补完整，再谈更复杂的优化。

### 5.2 先做可证明的 pause / resume，再决定是否下探 worker

`v3` 不应一上来就改 `mediasoup-worker`。

应该按下面顺序做判断：

1. 现有 `qosOverride` / client controller 是否足够表达 `pauseUpstream / resumeUpstream`
2. 现有 producer adapter 是否能无歧义执行
3. 只有确认 data plane 还缺关键能力时，再考虑 worker 改造

这里的“worker 改造”，明确包括：

- `mediasoup-cpp` 目录下的 worker 相关 C++ 代码
- 如有必要，仓库中实际使用的 `mediasoup-worker` 源码 / 构建入口

所以 `v3` 的结论不应该是“永远不改 worker”，而应该是：

- 先用 control plane + browser / integration 验证把问题收敛
- 再决定是否需要修改 worker 源代码来支撑真正的数据面 pause / resume

### 5.3 planner 必须从“事件驱动”升级到“时钟驱动”

`v2` 的 dirty queue 已经比逐条 snapshot 执行更稳，但仍然偏事件驱动。

`v3` 应该进一步升级成：

- per-room coalesced tick
- 固定最小规划周期，例如 `100-200ms`
- 降级快、恢复慢
- 单个 burst 不直接触发 pause / resume 翻转

### 5.4 测试矩阵要围绕 downlink 的断言对象

`uplink` 主要断言：

- sender state
- sender action

`downlink` 在 `v3` 应主要断言：

- consumer `preferredSpatialLayer / preferredTemporalLayer / priority / paused`
- publisher `track-scoped override`
- `maxLevelClamp`
- `pauseUpstream / resumeUpstream`
- 恢复链路是否稳定

不能把 uplink 的判定模板机械复制过来。

## 6. v3 总体架构

`v3` 建议分成两条线并行推进：

### 6.1 功能线

围绕 `zero-demand pause / resume` 展开：

- room planner tick
- producer demand state machine
- publisher supply policy
- client publisher controller / adapter 执行

### 6.2 验证线

围绕 `downlink matrix + 报告` 展开：

- server integration case
- browser harness case
- matrix runner
- case report / archive / metadata

两条线要同步推进。

如果只有功能线，没有验证线，`v3` 仍然不能算完成。

## 7. 功能设计

## 7.1 planner 调度升级

### 7.1.1 目标

把当前 `markDownlinkRoomDirty() -> continueDownlinkPlanning()` 的机制升级成真正的房间级 coalesced planner。

### 7.1.2 建议规则

- 每个 room 维护：
  - `lastPlannedAtMs`
  - `nextEligiblePlanAtMs`
  - `pendingPlanReasonMask`
- downlink snapshot 到达时不立即执行，只标记 dirty
- 统一 tick 每 `100-200ms` 拉取 dirty room 执行
- 如果同房间在 tick 窗口内来了多次变化，只执行一次规划

### 7.1.3 价值

- 减少 resize / visible 切换风暴
- 让多 subscriber 的变化在一个窗口内被统一看见
- 让恢复判定与 zero-demand pause 更稳定

## 7.2 producer demand 状态机升级

当前 `ProducerDemandAggregator` 已经有：

- `zeroDemandSinceMs`
- `holdUntilMs`

`v3` 应继续扩展成：

- `lastNonZeroDemandAtMs`
- `pauseEligibleAtMs`
- `resumeEligibleAtMs`
- `lastPauseSentAtMs`
- `lastResumeSentAtMs`

建议状态：

- `active`
- `low_clamped`
- `zero_demand_hold`
- `paused`
- `resume_warmup`

建议转移：

1. 有 demand
   `active` / `low_clamped`
2. demand 降到 0
   进入 `zero_demand_hold`
3. hold 超时仍为 0
   进入 `paused`
4. paused 后 demand 恢复
   进入 `resume_warmup`
5. warmup 完成且恢复稳定
   回到 `active` / `low_clamped`

## 7.3 publisher 控制语义升级

### 7.3.1 v2 已有

- `track-scoped maxLevelClamp`
- `downlink_v2_room_demand`
- `downlink_v2_zero_demand_hold`
- `downlink_v2_demand_restored`

### 7.3.2 v3 需要新增

建议新增明确的 track-scoped 语义：

- `pauseUpstream = true`
- `resumeUpstream = true`
- 或者等价的 `paused = true/false`

建议 reason：

- `downlink_v3_zero_demand_pause`
- `downlink_v3_demand_resumed`

如果现有 `qosOverride` schema 不够表达，再扩 schema；
但不要把 pause/resume 混成“ttl=0 的特殊魔法”。

## 7.4 client publisher controller / adapter

`v3` 需要明确区分两类动作：

- coordination clamp
  - `maxLevelClamp`
- upstream activity control
  - `pauseUpstream`
  - `resumeUpstream`

当前 [controller.js](../src/client/lib/qos/controller.js) 已经能处理：

- track-scoped merge
- ttl / clear
- `forceAudioOnly`
- `maxLevelClamp`

`v3` 应补：

- matching track 上的 `pauseUpstream`
- matching track 上的 `resumeUpstream`
- resume 后 keyframe / probe / warmup 策略
- 与现有 manual override 的优先级规则

## 7.5 worker / data plane 边界

`v3` 需要做一轮专门的边界确认：

1. client adapter 执行 `pauseUpstream` 后：
   - sender 是否真的停止编码 / 停止发 RTP
   - 本地 snapshot 会如何表现
2. `resumeUpstream` 后：
   - 是否需要显式 keyframe
   - 首帧恢复时间是否可接受
3. 如果现有浏览器 / producer adapter 行为不够稳定：
   - 再决定是否需要 worker 侧额外支持

这一步的结论应该单独沉淀成验证文档，而不是直接把 worker 改掉。

但要明确：

- 如果黑盒验证表明现有 control plane 无法保证“真正停发 / 真正恢复”
- 或者 pause / resume 后的数据面行为与预期不一致

那么 `v3` 后续阶段就应该正式进入 worker 源代码修改，而不是继续停留在外围绕行。

## 8. 测试设计

## 8.1 单测

至少补这些：

- producer demand state machine 的 pause / resume 转移
- `downlink_v3_zero_demand_pause` 只在持续 zero-demand 后触发
- `resume_warmup` 不会立刻恢复到最高层
- pause / resume 与 manual override 共存时优先级正确

## 8.2 服务端集成测试

至少补这些：

1. 单 producer、单 subscriber、持续 hidden
   - 先 low clamp
   - 再 pause upstream
2. paused 后恢复 visible
   - publisher 收到 resume
   - consumer 最终恢复
3. 两个 subscriber，其中一个 stale
   - stale snapshot 不触发 pause
   - stale snapshot 不回退已有 demand
4. 高频 hide/show
   - 不应 pause / resume 风暴

## 8.3 browser harness

`v3` 的 browser harness 不应只验证“通知是否到了”，而应验证真实效果。

至少补这些 case：

### A. loss sweep

- 低丢包
- 中丢包
- 高丢包
- 恢复后观察 clamp / pause / resume

### B. RTT sweep

- 低 RTT
- 中 RTT
- 高 RTT
- 恢复后观察恢复窗口

### C. jitter sweep

- 低 jitter
- 中 jitter
- 高 jitter
- 短 burst jitter 与持续 jitter 分开看

### D. recovery transition

- severe impairment -> full recovery
- hidden -> visible
- pinned -> grid
- zero-demand hold -> pause -> resume

### E. competition

- 双 subscriber 抢同一 producer
- 双 producer、单 subscriber、受限带宽竞争
- screenshare / pinned / grid 优先级差异

## 8.4 报告体系

建议 `downlink` 和 `uplink` 对齐成同一套机制：

- full / targeted 模式
- `docs/downlink-qos-case-results.md`
- `docs/generated/downlink-qos-case-results.targeted.md`
- `docs/generated/downlink-qos-matrix-report.json`
- `docs/generated/downlink-qos-matrix-report.targeted.json`
- `docs/archive/downlink-qos-runs/<timestamp>/...`
- `metadata.json`

这样后续每次回归都可以直接比较：

- 哪些 case 退化
- 哪些 case 恢复
- 当前 `v3` 是不是比 `v2` 更稳

## 9. 建议实施顺序

建议按下面顺序落地，不要一次性大 patch：

1. `refactor: add coalesced room planning tick for downlink`
2. `feat: add producer zero-demand pause state machine`
3. `feat: add track-scoped pauseUpstream/resumeUpstream override`
4. `feat: apply publisher upstream pause/resume in client controller`
5. `test: add downlink recovery and zero-demand integration cases`
6. `test: add downlink matrix and report pipeline`

## 10. 验收标准

`v3` 只有在下面都满足时才算完成：

1. 持续 zero-demand 后，publisher 不只是低层，而是真正 pause upstream
2. demand 恢复后，publisher 能 resume，且恢复不明显抖动
3. loss / RTT / jitter / recovery 矩阵在 browser harness 中可复现
4. downlink 报告格式、归档和 metadata 与 uplink 对齐
5. 可以明确回答某个失败是：
   - planner 问题
   - aggregate demand 问题
   - publisher override 问题
   - client resume / keyframe 问题

## 11. 总结

`v2` 的主题是：

- 让房间 demand 看得见
- 让 publisher 能被 clamp

`v3` 的主题则应该是：

- 让真正的 zero-demand pause / resume 成为可控行为
- 让恢复链路成为可签收行为
- 让 downlink 具备和 uplink 同级别的弱网验证与报告体系

只有这三件事做完，downlink QoS 才算从“有功能”走到“有完整工程闭环”。
