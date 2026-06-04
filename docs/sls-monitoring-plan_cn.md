# SLS 日志监控统一方案

这份文档是当前唯一推荐的 SLS 方案入口，统一回答 4 个问题：

1. 现在的日志能不能直接做监控
2. 首页大盘先做什么
3. 每个指标具体用哪条日志
4. 哪些告警可以先落

这份文档只基于**当前已经存在的日志**来写，不再把“未来建议”和“当前可落地”混在一起。

---

## 1. 当前结论

基于当前代码里的日志，**可以直接落地一版比较完整的业务监控**。

当前已经有这些关键日志：

- `producer closed`
- `consumer closed`
- `globalSnapshot`
- `roomSnapshot`

所以首页最关心的“房间数 / 主播数 / 观众数 / 全站人数”已经不需要再靠事件流硬推断。

但在实施前，必须先接受下面 3 条总约束：

1. 所有“当前值”卡片都必须带**快照新鲜度阈值**，不能只取最近一条就当作实时值。
2. 所有通知类日志都必须先定义清楚是按**原始日志行数**统计，还是按**逻辑事件去重后**统计。
3. 首页必须严格区分：
   - **人数/快照类指标**
   - **链路事件量指标**

否则第一版大盘会出现两种风险：

- 看起来很精确，实际是重复计数
- 看起来很实时，实际是陈旧快照

当前指标可以分成 3 类：

### 1.1 请求级/事件级指标

这类可以直接按事件统计：

- `join` 请求成功率
- `createWebRtcTransport` 请求成功率
- `produce` 请求成功率
- `consume` 请求成功率
- `peerJoined / peerLeft` 通知次数
- `newConsumer` 下发次数
- `auto-subscribe FAILED` 次数和原因
- `qosConnectionQuality` 分布
- `qosOverride` 次数和 `reason` 分布
- `qosRoomState` 分布
- `statsReport` 广播次数
- `serverRestart` 次数

说明：

- 这里的“事件级指标”，指的是**一条日志代表一类业务事实**
- 但如果日志天然有广播 fan-out，例如 `targetPeerCount / targetPeers`，就不能默认“1 行 = 1 个逻辑事件”

### 1.2 快照指标

这类直接用低频快照日志：

- 当前房间数
- 当前主播数
- 当前订阅端数
- 当前全站人数
- 当前房间人数

说明：

- 这些指标只有在**快照仍然新鲜**时才能称为“当前值”
- 一旦超过约定阈值未更新，就必须降级成：
  - stale
  - 置灰
  - 或直接告警
- 在 **SLS 聚拢所有节点日志** 的前提下，快照卡片**不能直接取全局最近一条日志**
- 必须先做一层固定快照加工
- `当前房间人数` 默认用于房间页 drill-down，不作为首页全局主卡片

### 1.3 窗口活跃指标

这类仍然有价值，但更适合作为趋势盘补充：

- 近 1 分钟活跃房间数
- 近 5 分钟有 produce 的主播数
- 近 5 分钟有 downlink 上报的观众数
- 最近 5 分钟事件观察到的平均房间人数

说明：

- 这些是**窗口行为值**
- 不应和“当前快照值”混成同一语义

---

## 2. 当前已经有的关键日志

这里只列监控需要的日志，不列全部。

## 2.1 请求结果与配对口径

日志：

```text
[request] room=demo peer=alice method=join id=1 session=... payload=...
[request-done] room=demo peer=alice method=join id=1 session=... ok=true redirect=- error=-
```

适合做：

- QPS
- 已完成请求成功率
- 已完成请求失败率
- 错误原因
- 不完整请求识别

默认口径：

- 第一版首页默认展示：
  - `已完成请求成功率`
- 如果没有实现 `[request]` 与 `[request-done]` 的超时配对，则默认展示：
  - `已完成请求成功率`
- 如果已经实现稳定配对，则展示：
  - `完整请求成功率`
  - `不完整请求数`

## 2.2 房间与成员

日志：

```text
[demo system] room created
[demo alice] join done reconnect=false participants=2
[demo alice] notify peerJoined joinedPeerId=alice displayName=alice reconnect=false targetPeerCount=1 targetPeers=["bob"] participantCount=2
[demo alice] notify peerLeft leftPeerId=alice targetPeerCount=1 targetPeers=["bob"] participantCount=1
[demo alice] leave done room_empty=true
```

适合做：

- 房间创建数
- join/leave 生命周期趋势
- 抖动房间
- 通知链路趋势
- 房间人数事件变化

## 2.3 媒体与订阅

日志：

```text
[demo alice] produce done transportId=... kind=video source=camera producerId=...
[demo alice] notify newConsumer target=bob producerId=... consumerId=... kind=video
[demo alice] auto-subscribe FAILED for bob: no compatible codecs
[demo alice] producer closed producerId=... kind=video source=camera reason=transportclose remainingProducers=0
[demo bob] consumer closed consumerId=... producerId=... kind=video reason=producerclose remainingConsumers=0
```

适合做：

- produce 成功数
- newConsumer 事件量
- auto-subscribe 失败原因
- producer 生命周期结束统计
- consumer 生命周期结束统计

## 2.4 QoS

日志：

```text
[demo alice] clientStats done seq=17 quality=poor stale=false tracks=1
[demo bob] downlinkClientStats done seq=16 subscriptions=1 dropped=0
[demo alice] notify qosConnectionQuality target=alice quality=poor targetPeerCount=1 targetPeers=["alice"] payload={...}
[demo system] notify qosRoomState quality=poor peers=2 targetPeerCount=2 targetPeers=["alice","bob"] payload={...}
[demo alice] notify qosOverride target=alice reason=server_auto_poor targetPeerCount=1 targetPeers=["alice"]
[demo system] notify statsReport targetPeerCount=2 targetPeers=["alice","bob"] statsPeers=2
```

适合做：

- peer 级质量分布
- room 级质量分布
- override 分布
- stats 广播活跃度

## 2.5 快照

日志：

```text
[system] globalSnapshot rooms=123 publishers=456 subscribers=789 peers=1011
[roomId system] roomSnapshot participants=8 publishers=2 subscribers=6
```

适合做：

- 首页房间数
- 首页主播数
- 首页订阅端数
- 首页全站人数
- 单房间人数和角色分布
- 房间页 drill-down 数据源

实施前提：

- `globalSnapshot` 必须能识别 `nodeId`
- `roomSnapshot` 必须能识别 `roomId`
- `snapshotTsMs` 应视为强依赖；如果缺失，freshness 只能退化为采集时间近似值

如果日志正文里没有 `nodeId / snapshotTsMs`：

- 必须从 SLS 采集元数据里稳定拿到节点维度与可用时间戳
- 如果 `nodeId` 不稳定，这组卡片**不得**上线为首页主卡片
- 如果 `snapshotTsMs` 缺失，只能临时用采集时间近似替代，并在文档/看板中明确标注

## 2.6 稳定性

日志：

```text
[demo system] notify serverRestart reason=worker crashed targetPeerCount=2 targetPeers=["alice","bob"]
```

适合做：

- 强告警
- 节点稳定性统计

---

## 3. 首页大盘怎么做

首页优先放**精确快照 + 核心成功率 + 关键质量指标**。

建议首页 8 个卡片：

1. 当前房间数
2. 当前主播数
3. 当前订阅端数
4. 当前全站人数
5. `join` 请求成功率
6. `produce` 请求成功率
7. `poor/lost` peer 比例
8. `error` 白名单计数

再额外加 2 个稳定性卡片：

9. `serverRestart` 次数
10. 快照新鲜度状态

这已经足够回答：

- 现在有多少房间
- 现在有多少主播
- 现在有多少订阅端
- 人能不能进房
- 人能不能发流
- 质量是不是在恶化
- 服务是不是在抖

`newConsumer` 仍然重要，但更适合作为第二屏“信令与媒体盘”的内部链路指标，
不建议继续把它当成首页业务主 KPI。

首页实施约束：

- `newConsumer` 不进入首页主卡片
- 首页所有快照卡片都必须同时展示 freshness 状态
- 首页所有成功率卡片都必须写明“请求级”

---

## 4. 首页执行表

## 4.1 当前房间数

### 使用日志

- `globalSnapshot rooms=...`

### 统计口径

不能直接取“全局最近一条”。

必须按下面规则加工：

1. 固定有效窗口：`最近 30s`
2. 先按 `nodeId` 分组
3. 每个 `nodeId` 只取窗口内最新一条 `globalSnapshot`
4. 只有“窗口内有有效快照”的节点参与求和
5. 对这些节点的 `rooms` 求和

重要约束：

- stale / broken 节点的旧值**不得**继续带入当前总值

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照聚合值

### 新鲜度要求

- 默认按当前服务端输出周期看待
- 实施建议统一阈值：
  - `30s` 内未更新：正常
  - `30s ~ 60s`：stale / 置灰
  - `>= 60s`：触发快照中断告警
- 首页卡片默认只展示三态之一：
  - `normal`
  - `stale`
  - `broken`
- 不建议首页直接展示“距离上次快照多少秒”的原始数值来替代状态

## 4.2 当前主播数

### 使用日志

- `globalSnapshot publishers=...`

### 统计口径

- 与 `当前房间数` 相同的快照加工规则
- 先按 `nodeId` 取窗口内最新一条 `globalSnapshot`
- 再对 `publishers` 求和

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照聚合值

## 4.3 当前订阅端数

### 使用日志

- `globalSnapshot subscribers=...`

### 统计口径

- 与 `当前房间数` 相同的快照加工规则
- 先按 `nodeId` 取窗口内最新一条 `globalSnapshot`
- 再对 `subscribers` 求和

### 命名约束

- 首页默认命名为：`当前订阅端数`
- 第一版首页不要默认命名成“当前观众数”
- 如果未来对外展示成“观众数”，必须额外注明：
  - `口径=当前有 consumer 的 peer 数`

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照聚合值

## 4.4 当前全站人数

### 使用日志

- `globalSnapshot peers=...`

### 统计口径

- 与 `当前房间数` 相同的快照加工规则
- 先按 `nodeId` 取窗口内最新一条 `globalSnapshot`
- 再对 `peers` 求和

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照聚合值

## 4.5 join 请求成功率

### 使用日志

- `[request] method=join ...`
- `[request-done] method=join ...`

### 统计口径

- 成功请求：按 `id + session` 为主键，`room + peer + method` 为辅助维度，成功配对到 `[request-done]` 且 `ok=true`
- 失败请求：按同样规则配对到 `[request-done]` 且 `ok=false`
- 不完整请求：出现 `[request]` 后，在 `30s` 内没有配对到对应 `[request-done]`

### 指标语义

- 这是**请求级成功率**
- 不是用户级成功率
- 重试、重连、重复 join、重定向都会影响分母

### 实施建议

- 主配对键：
  - `id + session`
- 辅助校验维度：
  - `room`
  - `peer`
  - `method`
- 如果 `session=0`、缺失或不可用：
  - 退化为 `room + peer + method + id`
  - 同时标记为“低置信配对”
- 如果 SLS 查询能力足够：
  - 分母 = 成功请求 + 失败请求 + 不完整请求
- 如果当前 SLS 配对能力不足：
  - 首页只展示“已完成 join 请求成功率”
  - 不默认承诺 `不完整 join 请求数`

### 默认展示口径

- 首页默认：`已完成 join 请求成功率`
- 等 SLS 配对能力稳定后，再升级成：
  - `完整 join 请求成功率`
  - `不完整 join 请求数`

## 4.6 produce 请求成功率

### 使用日志

- `[request] method=produce ...`
- `[request-done] method=produce ...`

### 统计口径

- 成功请求：按 `id + session` 为主键，`room + peer + method` 为辅助维度，成功配对到 `[request-done]` 且 `ok=true`
- 失败请求：按同样规则配对到 `[request-done]` 且 `ok=false`
- 不完整请求：出现 `[request]` 后，在 `30s` 内没有配对到对应 `[request-done]`

### 指标语义

- 这是**请求级成功率**
- 不是“发流用户成功率”
- 同一个用户反复重试会重复计入分母

### 实施建议

- 主配对键：
  - `id + session`
- 辅助校验维度：
  - `room`
  - `peer`
  - `method`
- 如果 `session=0`、缺失或不可用：
  - 退化为 `room + peer + method + id`
  - 同时标记为“低置信配对”
- 如果 SLS 查询能力足够：
  - 分母 = 成功请求 + 失败请求 + 不完整请求
- 如果当前 SLS 配对能力不足：
  - 首页只展示“已完成 produce 请求成功率”
  - 不默认承诺 `不完整 produce 请求数`

### 默认展示口径

- 首页默认：`已完成 produce 请求成功率`
- 等 SLS 配对能力稳定后，再升级成：
  - `完整 produce 请求成功率`
  - `不完整 produce 请求数`

## 4.7 poor/lost peer 比例

### 使用日志

- `notify qosConnectionQuality target=... quality=...`

### 统计口径

- 固定窗口：`5m`
- 每个 `room + target` 在窗口内只取最后一条 `qosConnectionQuality`
- 如果某个 `room + target` 在当前窗口没有新状态：
  - 默认记为 `unknown`
  - 不继承上一个窗口状态
  - 不直接计入 `excellent/good/poor/lost` 分母
- 用最终状态统计 `poor/lost` 占比

## 4.8 error 白名单计数

### 使用日志

- 只统计明确白名单的业务错误

建议第一批白名单：

- `auto-subscribe FAILED`
- `notify serverRestart`
- `[request-done] ... ok=false` 的关键方法
- 明确的：
  - `room not found`
  - `peer not found`
  - `transport not found`
  - `invalid payload`

### 匹配与归类规则

- 匹配优先级：
  1. 精确事件类型匹配
  2. 再做错误文本匹配
- 第一版建议采用：
  - 精确匹配固定前缀
  - 精确匹配固定 `method`
  - 对自由文本只做受控的包含匹配

建议统一归并成以下类别名：

- `auto_subscribe_failed`
- `server_restart`
- `request_failed_join`
- `request_failed_create_transport`
- `request_failed_produce`
- `request_failed_consume`
- `room_missing`
- `peer_missing`
- `transport_missing`
- `invalid_payload`

### 默认展示层级

- 首页：展示白名单错误总量
- 第二屏或排障页：展示按错误类别归并后的趋势和 TopN
- 原始错误明细只放排障页，不放首页

### 默认时间窗口

- 首页白名单错误总量：`5m`
- 第二屏错误趋势：`5m`
- 排障页 TopN：默认 `15m`，必要时切到 `5m`

## 4.9 serverRestart 次数

### 使用日志

- `notify serverRestart reason=... targetPeerCount=... targetPeers=...`

### 房间级统计口径

- 固定窗口：`5m`
- 房间级逻辑事件键：
  - `room + reason + 60s`
- 用途：
  - 受影响房间分析
  - 异常房间 TopN

### 节点级统计口径

- 固定窗口：`5m`
- 节点级逻辑事件键：
  - `node + reason + 60s`
- 用途：
  - 首页卡片
  - P1 告警
  - 节点事故次数

### 重要约束

- 房间级 `serverRestart` **不能**用于统计节点事故总次数
- 节点事故总次数一律使用节点级去重口径

---

## 5. 第二屏：信令与媒体

建议放：

- `createWebRtcTransport` 请求成功率
- `consume` 请求成功率
- `auto-subscribe FAILED` 次数
- `join done / leave done` 生命周期趋势
- `peerJoined / peerLeft` 通知趋势
- `newConsumer` 事件量
- `produce kind/source` 分布
- `producer closed` 次数与原因
- `consumer closed` 次数与原因

### 5.1 join/leave 抖动

如果目标是“真实用户状态迁移趋势”，优先基于：

- `join done`
- `leave done`

如果目标是“通知链路是否稳定”，看：

- `peerJoined`
- `peerLeft`

默认口径：

- `抖动房间 TopN`：基于 `join done + leave done`
- `通知链路趋势`：基于 `peerJoined + peerLeft`

### 5.2 newConsumer

推荐定义：

- `newConsumer/min` = 订阅建立事件量
- `subscribers` = 快照人数

这两者必须分开展示，不能混用。

---

## 6. 第三屏：QoS

建议放：

- `qosConnectionQuality` 分布
- `qosRoomState` 分布
- `qosOverride reason` 分布
- `statsReport` 广播量

### 6.1 peer 级质量

- 固定窗口：`5m`
- 每个 `room + target` 只取最后状态
- 如果当前窗口没有新状态：
  - 记为 `unknown`
  - 不继承上一个窗口状态
  - 不进入 `excellent/good/poor/lost` 分母

### 6.2 room 级质量

- 固定窗口：`5m`
- 每个 `room` 只取最后状态
- 如果当前窗口没有新状态：
  - 记为 `unknown`
  - 不继承上一个窗口状态
  - 不进入 `excellent/good/poor/lost` 分母

### 6.3 qosOverride reason

必须拆成两组，不要混成一张主图：

降级类：

- `server_auto_poor`
- `server_auto_lost`
- 其他主动压降类

恢复/清除类：

- `server_auto_clear`
- `server_room_pressure_clear`
- `server_ttl_expired`
- `downlink_v2_demand_restored`

同时建议保留两种统计口径：

- 原始通知次数
- 去重后的 `room + target + reason` 逻辑事件次数

默认展示口径：

- 第二屏主图：去重后的逻辑事件次数
- 排障页辅助图：原始通知次数

---

## 7. 第四屏：异常房间

建议放：

- `auto-subscribe FAILED` 最多的房间
- `poor/lost` 最多的房间
- `join/leave` 最抖的房间
- 白名单错误最多的房间
- `serverRestart` 影响最多的房间

这屏主要用于排障，不是首页。

---

## 8. 当前仍不适合直接做的项

当前还不适合直接做成高质量指标的主要是：

### 8.1 请求耗时 P95/P99

原因：

- 还没有统一 `costMs`

### 8.2 统一错误码维度

原因：

- 现在很多错误还是自由文本

### 8.3 更细粒度的观众分类

例如：

- 只听音频的人
- 真正在看视频的人
- 页面不可见但仍有订阅的人

原因：

- 现在只有 `subscribers` 这个快照粒度

---

## 9. 推荐的实际落地顺序

### 第一批立刻做

- 当前房间数
- 当前主播数
- 当前订阅端数
- 当前全站人数
- join 请求成功率
- produce 请求成功率
- poor/lost peer 比例
- error 白名单计数
- serverRestart 告警

### 第二批补齐

- consume 请求成功率
- newConsumer 事件量
- auto-subscribe FAILED TopN
- qosRoomState 分布
- qosOverride 降级类 / 恢复类分布
- producer/consumer closed 原因分布
- 异常房间 TopN

### 第三批再优化

- `costMs`
- `errorCode`
- 更细粒度 viewer 快照
- SLS 中对不完整请求的稳定配对实现

---

## 10. 最后一句话

如果你现在就要让 SLS 同学开工，这份文档已经足够。

最关键的一点是：

- 快照首页卡片必须先做“按节点取最新，再跨节点求和”
- 首页快照卡片必须有 freshness guard
- 通知类日志必须先定义“按行统计”还是“按逻辑事件统计”
- `newConsumer` 是事件量，不是人数
- `join/produce` 是请求级成功率，不是用户级成功率

做到这几点，第一版大盘就不会在定义上跑偏。
