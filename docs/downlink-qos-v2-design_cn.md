# 下行 QoS v2 设计方案

## 1. 文档目的

这份文档定义当前仓库 `downlink QoS v2` 的单一事实源。

这里说的 `v2`，不是把 `v1` 推倒重来，而是在现有下行能力之上补齐两个关键缺口：

- 从“按 subscriber 单独做接收控制”升级到“按真实带宽做显式分配”
- 从“只调 consumer”升级到“把房间里的 subscriber demand 反向反馈到 publisher supply”

`v1` 的范围和落地顺序，继续看：

- [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
- [downlink-qos-implementation-plan_cn.md](./downlink-qos-implementation-plan_cn.md)

`v2` 的分阶段实施方案见：

- [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md)

## 2. 当前基线

当前仓库已经不是“只有想法，没有实现”的状态。

已经落地的下行基础能力包括：

- `downlinkClientStats` 上报与校验
- `DownlinkQosRegistry` 保存 subscriber 最新快照
- `DownlinkHealthMonitor` 做 `stable / early_warning / congested / recovering`
- `DownlinkAllocator` 输出 `pause / resume / setPreferredLayers / setPriority`
- `SubscriberQosController` 把动作应用到 `Consumer`

相关代码：

- [RoomServiceStats.cpp](../src/RoomServiceStats.cpp)
- [DownlinkQosRegistry.h](../src/qos/DownlinkQosRegistry.h)
- [DownlinkAllocator.h](../src/qos/DownlinkAllocator.h)
- [DownlinkHealthMonitor.h](../src/qos/DownlinkHealthMonitor.h)
- [SubscriberQosController.h](../src/qos/SubscriberQosController.h)
- [downlinkReporter.js](../src/client/lib/qos/downlinkReporter.js)

这意味着 `v2` 不该再写成“从零开始的理想化方案”，而应该直接基于这些现有组件演进。

## 3. 为什么需要 v2

`v1` 已经能解决一批高价值问题：

- hidden 视频暂停
- 大小窗分层
- 差网时按健康度降档
- 恢复时逐步升档并请求关键帧

但它还有 3 个结构性缺口。

### 3.1 只能看单个 subscriber，看不到整个房间的 demand

当前每个 subscriber 都是独立计算：

- 这个 subscriber 想看哪一层
- 这个 subscriber 现在该 pause 哪一路

但没有房间级聚合去回答：

- 某个 producer 的高层现在到底有没有任何 subscriber 在用
- 哪些 simulcast layer 可以回收
- publisher 是否还在白白发送没人看的层

### 3.2 `setPriority()` 已经有代码，但还不是可证明的带宽分配

现在代码里已经有：

- `DownlinkAllocator::ComputePriority()`
- `Consumer::setPriority()`
- 对应单测

但这不等于“真实弱网竞争里，高优先级就一定更好”。

现有浏览器竞争测试本身已经把这个风险写得很清楚：

- [browser_downlink_priority.mjs](../tests/qos_harness/browser_downlink_priority.mjs)

当前更像“有 hint”，还不是“有显式 budget allocator”。

### 3.3 还没有形成完整闭环

当前闭环只做到：

- subscriber demand
- consumer-side receive control

还没有做到：

- subscriber demand
- room-level aggregate demand
- publisher-side layer supply control

所以现在的下行能力，本质上仍然是“接收侧自适应”，不是完整的 `adaptive stream + dynacast` 闭环。

## 4. v2 目标与非目标

## 4.1 目标

`v2` 需要明确完成这 5 件事：

1. 为单个 subscriber 引入显式 bitrate-aware allocator，而不是只靠 `degradeLevel + priority`
2. 为单个 producer 聚合全房间 subscriber demand
3. 把 aggregate demand 反向翻译成 publisher 侧供给限制
4. 让整个控制链路具备抗抖动的 hysteresis / hold / debounce
5. 让浏览器级测试能稳定证明“分配确实发生了”

## 4.2 非目标

`v2` 先不做这些：

- 重写现有 uplink QoS controller
- 一上来就修改 `mediasoup-worker`
- 求“整个房间所有 subscriber 的全局最优解”
- 一开始就做 codec / SVC 全矩阵特化
- 把音频策略和视频策略重新统一改写

## 5. 设计原则

建议固定这几个原则，不然后面会再次把范围做散。

### 5.1 先复用现有协议和控制面

第一阶段尽量复用现有能力：

- `downlinkClientStats`
- `qosOverride`
- track-scoped override
- publisher 侧现有 `maxLevelClamp`

如果只为了 `maxLevelClamp`，不应先引入新消息类型。

### 5.2 先做 clamp，再做 zero-demand pause

`dynacast` 最有价值的第一步，是先停止“没人需要高层却继续发送高层”。

所以第一阶段先做：

- publisher `maxLevelClamp`

而不是一上来就做：

- “无人订阅时立刻 pause upstream”

后者更容易引入恢复抖动、关键帧风暴和 UI 闪烁。

### 5.3 把分配问题拆成两个作用域

`v2` 里要显式区分两个 allocator：

- subscriber scope：
  在一个 subscriber 自己的下行预算里，决定每路 consumer 收什么层
- producer scope：
  在一个 producer 面向全房间的 demand 里，决定 publisher 最多需要发送到哪一层

这两个问题不能混成一个“大而全 allocator”。

### 5.4 没有可验证测试，不算完成

`v2` 不是加几个类名就算做完。

必须至少做到：

- 浏览器弱网竞争里，高优先级流稳定优于低优先级流
- 所有 subscriber 都把某 producer 缩到低层后，publisher 供给也能观察到下降

## 6. 总体架构

`v2` 建议采用“两阶段规划、两端执行”。

### 6.1 阶段 A：subscriber receive allocation

输入：

- `downlinkClientStats`
- `visible / pinned / activeSpeaker / isScreenShare / targetWidth / targetHeight`
- subscriber 当前 `availableIncomingBitrate`
- 服务端当前 consumer / producer 状态

输出：

- 每个 consumer 的目标接收层
- 每个 consumer 的优先级
- pause / resume / keyframe 动作

这一步解决的是：

- “这个 subscriber 自己怎么花它的下行预算”

### 6.2 阶段 B：producer supply aggregation

把所有 subscriber 的阶段 A 结果按 `producerId` 聚合，得到：

- 某 producer 当前被要求的最高 spatial layer
- 某 producer 当前被要求的最高 temporal layer
- 当前是否还有任何可见 subscriber 需要这路视频
- 是否应该保持短暂 hold，避免频繁回弹

这一步解决的是：

- “这个 producer 最多还需要发到哪一层”

### 6.3 执行端

执行端仍然分两条链路：

- subscriber 侧：
  继续用 `Consumer::pause/resume/setPreferredLayers/setPriority/requestKeyFrame`
- publisher 侧：
  通过 track-scoped `qosOverride` 下发 `maxLevelClamp`

这样可以在不改 worker 的前提下，先把 v2 的主价值做出来。

## 7. Subscriber 侧 allocator 设计

## 7.1 不再只用 `degradeLevel`

当前 `degradeLevel` 适合做紧急保护，但不适合做精细分配。

`v2` 建议改成：

- `degradeLevel`
  继续保留，作为健康度触发的快速降级保险丝
- `budgetAllocator`
  作为正常路径里的显式预算分配器

也就是：

- 差网突发时，`degradeLevel` 先保命
- 常态稳态时，`budgetAllocator` 决定谁拿高层、谁拿低层

## 7.2 输入模型

对每个 subscription，计算两类值：

- `utility`
  这一路视频对当前页面体验的重要程度
- `cost`
  把它升级到某一层需要付出的预估带宽

建议的 utility 基础权重：

- screenshare
- pinned
- active speaker
- visible grid
- hidden

建议再乘上两个修正项：

- 目标显示面积
- 最近冻结 / 掉帧惩罚

## 7.3 预算来源

subscriber 预算建议取保守值，而不是只信单一指标。

推荐：

`subscriberBudget = min(availableIncomingBitrate * alpha, recentGoodput * beta, serverSafetyCap)`

其中：

- `availableIncomingBitrate`
  来自 browser stats
- `recentGoodput`
  来自最近接收实际速率
- `serverSafetyCap`
  是服务端留的保护上限

如果这些值缺失或明显不可信，则回退到：

- 当前 `degradeLevel` 路径

## 7.4 分配算法

建议采用“先保底，再做增量升级”的贪心算法。

步骤：

1. hidden 直接不分配视频层
2. 所有 visible 视频先拿一个最低可用层
3. 把每次“从当前层升级一档”的收益记为 `deltaUtility / deltaCost`
4. 按收益密度从高到低排序
5. 在预算内逐步发放升级

可以把它理解成：

- 先保证“人人看得到”
- 再决定“谁值得更清楚、更流畅”

建议伪代码：

```text
reserve base layer for all visible subscriptions
while budget remains:
  pick best upgrade step by marginal utility / marginal bitrate
  if step fits budget:
    apply upgrade
  else:
    break
```

## 8. Producer 侧 demand 聚合与供给控制

## 8.1 聚合结果

对每个 producer 维护一份 `ProducerDemandState`：

```json
{
  "producerId": "producer-1",
  "publisherPeerId": "alice",
  "trackId": "camera-track-1",
  "maxSpatialLayer": 1,
  "maxTemporalLayer": 2,
  "visibleSubscriberCount": 3,
  "highPrioritySubscriberCount": 1,
  "zeroDemandSinceMs": 0,
  "holdUntilMs": 1710000005000
}
```

关键点：

- 这里聚合的是“阶段 A 分配后的结果”，不是原始 UI hint
- 聚合粒度必须是 `producerId`
- 往 publisher 下发控制时再映射到 `trackId`

## 8.2 推荐下发方式

第一阶段推荐直接复用现有 `qosOverride`：

- `scope = track`
- `trackId = publisher local track id`
- `maxLevelClamp = aggregated max spatial`
- `reason = downlink_v2_room_demand`

这样可以直接复用现有：

- [controller.js](../src/client/lib/qos/controller.js)
- [protocol.js](../src/client/lib/qos/protocol.js)

## 8.3 zero-demand 处理

当没有任何 visible subscriber 需要某路视频时，建议分两步做：

### Phase 1

先把 publisher clamp 到最低可用层，不立刻 pause。

价值：

- 先验证 aggregate demand -> supply clamp 这条链路
- 风险更低

### Phase 2

再增加“持续 zero-demand 后 pause upstream”的能力。

这一步需要补：

- 新的 track-scoped pause 语义
- 恢复时的 keyframe / debounce 规则

如果现有 `qosOverride` 字段不够，再考虑加新字段；不要在 Phase 1 就把协议一起做大。

## 9. 调度与抗抖动

## 9.1 不要在每条 snapshot 上立刻全量重算

当前 `RoomService::setDownlinkClientStats()` 是“收到一条就直接算一次”。

到了 `v2`，如果还这样做，房间级聚合会变成：

- 更新抖动大
- 同一房间多个 subscriber 顺序依赖强
- 高频 resize / visible 切换时容易抖

建议改为：

- 标记 room dirty
- 由 room worker 上的 coalesced planner 每 `100-200ms` 统一算一次

## 9.2 需要的 hysteresis

至少需要这几条：

- clamp 降级可以快，恢复必须慢
- producer 高层在降到低层前要有短暂 hold
- zero-demand pause 需要连续多个窗口确认
- subscriber 从 hidden 恢复时，先恢复 consumer，再允许 publisher 升供给

否则最容易出现：

- 大小窗拖动时反复切层
- 页面切后台 / 回前台时瞬时风暴
- 多个 subscriber 同时切换导致 publisher 供给来回抖动

## 10. 协议策略

## 10.1 `downlinkClientStats` 先不升级 schema

推荐先继续使用：

- `mediasoup.qos.downlink.client.v1`

原因：

- 现有字段已经足够支撑 allocator 第一版
- 现在真正缺的是房间级规划与供给反馈，不是采样字段本身

只有在以下场景才考虑升 schema：

- 需要上报 subscriber 本地估计的 layer bitrate
- 需要把 viewport class / rendering mode 做成强类型字段
- 需要带上更精细的 per-consumer receive throughput

## 10.2 publisher 控制优先复用 track-scoped override

优先顺序建议是：

1. 复用现有 `qosOverride`
2. 只新增最小必要字段
3. 只有语义实在不适合时，才新增专门的 `publisherDemandHint`

这样可以避免：

- 再造一套重复的信令类型
- client / server / tests 三边一起爆改

## 11. 组件与文件建议

建议新增的核心组件：

- `src/qos/SubscriberBudgetAllocator.h`
- `src/qos/SubscriberBudgetAllocator.cpp`
- `src/qos/ProducerDemandAggregator.h`
- `src/qos/ProducerDemandAggregator.cpp`
- `src/qos/PublisherSupplyController.h`
- `src/qos/PublisherSupplyController.cpp`
- `src/qos/RoomDownlinkPlanner.h`
- `src/qos/RoomDownlinkPlanner.cpp`

建议修改的现有组件：

- [RoomService.h](../src/RoomService.h)
- [RoomServiceStats.cpp](../src/RoomServiceStats.cpp)
- [RoomServiceDownlink.cpp](../src/RoomServiceDownlink.cpp)
- [DownlinkAllocator.h](../src/qos/DownlinkAllocator.h)
- [DownlinkAllocator.cpp](../src/qos/DownlinkAllocator.cpp)
- [DownlinkQosRegistry.h](../src/qos/DownlinkQosRegistry.h)
- [DownlinkQosRegistry.cpp](../src/qos/DownlinkQosRegistry.cpp)
- [controller.js](../src/client/lib/qos/controller.js)
- [protocol.js](../src/client/lib/qos/protocol.js)
- [types.d.ts](../src/client/lib/qos/types.d.ts)

## 12. 测试与验收

## 12.1 单测

至少补这些：

- subscriber budget allocator 会优先保 screenshare / pinned
- budget 不足时先牺牲 grid，再牺牲 pinned
- producer demand 取所有 subscriber 目标层的最大值
- hysteresis 能阻止单个 sample 把 publisher 供给来回翻转

## 12.2 集成测试

至少补这些：

- 两个 subscriber 请求同一 producer 的不同层，publisher clamp 取更高需求
- 所有 subscriber 都降到低层后，publisher 收到更低的 track clamp
- stale `downlinkClientStats` 不会把 producer demand 回退

## 12.3 Browser harness

至少补这些：

1. 双 publisher、单 subscriber、受限带宽竞争
2. 单 publisher、双 subscriber、不同视图尺寸
3. 所有 subscriber 隐藏同一路视频后，publisher 供给下降
4. visible 恢复后，consumer 与 publisher 都能稳定恢复，不明显振荡

## 12.4 验收标准

`v2` 至少要满足：

- 真实弱网竞争里，高优先级视频稳定优于低优先级视频
- 同一 producer 没人需要高层时，publisher 发送层能被实际收缩
- resize / hidden / restore 场景下，不出现明显层级抖动
- 失败日志能回答“预算不够、聚合错误还是 publisher clamp 未生效”

## 13. 推荐推进顺序

建议按下面顺序推进，不要一口气把所有 v2 能力一起上：

1. 把 `RoomService` 改成 room-scoped dirty planner，但保持 v1 行为不变
2. 引入 `SubscriberBudgetAllocator`，先解决“显式分配”
3. 引入 `ProducerDemandAggregator`，先算出 aggregate demand
4. 用 track-scoped `maxLevelClamp` 跑通 publisher supply clamp
5. 补齐浏览器竞争测试和 publisher clamp 观测
6. 最后再做 zero-demand pause / resume

## 14. 总结

下行 QoS `v2` 的本质，不是“再多加几条 if-else 规则”，而是把当前 `v1` 的局部接收控制，升级成一个完整闭环：

- subscriber 侧显式预算分配
- producer 侧房间级 demand 聚合
- publisher 侧供给收缩

只要把这三件事按顺序做出来，当前仓库的下行 QoS 才会从“能用的 v1”走到“可证明有效的 v2”。
