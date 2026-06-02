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
- 当前观众数
- 当前全站人数
- 当前房间人数

说明：

- 这些指标只有在**快照仍然新鲜**时才能称为“当前值”
- 一旦超过约定阈值未更新，就必须降级成：
  - stale
  - 置灰
  - 或直接告警

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

## 2.1 请求入口和请求结果

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
- 首页观众数
- 首页全站人数
- 房间级人数和角色分布

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
3. 当前观众数
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
- 现在有多少观众
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

- 直接取最近一条 `globalSnapshot.rooms`

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照

### 新鲜度要求

- 默认按当前服务端输出周期看待
- 实施建议统一阈值：
  - `30s` 内未更新：正常
  - `30s ~ 60s`：stale / 置灰
  - `>= 60s`：触发快照中断告警

## 4.2 当前主播数

### 使用日志

- `globalSnapshot publishers=...`

### 统计口径

- 直接取最近一条 `globalSnapshot.publishers`

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照

### 新鲜度要求

- 与 `globalSnapshot` 同步检查新鲜度

## 4.3 当前观众数

### 使用日志

- `globalSnapshot subscribers=...`

### 统计口径

- 直接取最近一条 `globalSnapshot.subscribers`

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照

### 新鲜度要求

- 与 `globalSnapshot` 同步检查新鲜度

## 4.4 当前全站人数

### 使用日志

- `globalSnapshot peers=...`

### 统计口径

- 直接取最近一条 `globalSnapshot.peers`

### 推荐图表

- 单值卡片
- 15 分钟趋势折线

### 精度

- 精确快照

### 新鲜度要求

- 与 `globalSnapshot` 同步检查新鲜度

## 4.5 join 请求成功率

### 使用日志

- `[request] method=join ...`
- `[request-done] method=join ...`

### 统计口径

- 成功请求：在窗口内按 `id + session` 为主键，`room + peer + method` 为辅助维度，成功配对到 `[request-done]` 且 `ok=true`
- 失败请求：在窗口内按同样规则配对到 `[request-done]` 且 `ok=false`
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
  - 同时将该样本标记为“低置信配对”
- 如果 SLS 查询能力足够：
  - 分母 = 成功请求 + 失败请求 + 不完整请求
- 如果当前 SLS 配对能力不足：
  - 首页先展示“已完成 join 请求成功率”
  - 另起一张卡展示“不完整 join 请求数”

## 4.6 produce 请求成功率

### 使用日志

- `[request] method=produce ...`
- `[request-done] method=produce ...`

### 统计口径

- 成功请求：在窗口内按 `id + session` 为主键，`room + peer + method` 为辅助维度，成功配对到 `[request-done]` 且 `ok=true`
- 失败请求：在窗口内按同样规则配对到 `[request-done]` 且 `ok=false`
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
  - 同时将该样本标记为“低置信配对”
- 如果 SLS 查询能力足够：
  - 分母 = 成功请求 + 失败请求 + 不完整请求
- 如果当前 SLS 配对能力不足：
  - 首页先展示“已完成 produce 请求成功率”
  - 另起一张卡展示“不完整 produce 请求数”

## 4.7 poor/lost peer 比例

### 使用日志

- `notify qosConnectionQuality target=... quality=...`

### 统计口径

- 固定窗口：`5m`
- 每个 `room + target` 在窗口内只取最后一条 `qosConnectionQuality`
- 用最终状态统计 `poor/lost` 占比

### 实施原因

- 不能直接按原始日志条数算比例
- 否则高频上报的 peer 会被放大

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

实施要求：

- 白名单规则必须维护成一份共享配置
- 不能让不同同学各自写一套 SLS 过滤表达式

### 实施原因

- 不建议直接统计所有 `warning/error`
- worker 内部有大量高频、低动作性的 warning
- 直接按 severity 聚合会长期噪音化

## 4.9 serverRestart 次数

### 使用日志

- `notify serverRestart reason=... targetPeerCount=... targetPeers=...`

### 统计口径

- 固定窗口：`5m`
- 不按原始广播行数直接计数
- 建议的逻辑事件键：
  - `room + reason + restart_window_bucket`
- 当前建议的 `restart_window_bucket`：`60s`

也就是：

- 同一房间
- 同一原因
- 在 `60s` 内重复广播
- 只算一次逻辑重启事件

### 告警

- `5m` 内逻辑重启事件 `>= 1`
- 最好再结合容器存活 / 健康检查第二信号

### 节点级补充口径

房间级重启事件用于回答：

- 哪些房间受影响

节点级重启事件用于回答：

- 这是不是一次节点事故

节点级建议额外再聚合一层：

- `node + reason + 60s`

这样可以避免一次节点级事故被拆成多个房间事件后放大。

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

不要把两者混成一个指标。

### 抖动房间 TopN 口径

如果要做“抖动房间 TopN”，统一采用：

- `join done + leave done`

作为主口径。

`peerJoined + peerLeft` 只用于：

- 观察通知风暴
- 诊断广播异常

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

### 6.2 room 级质量

- 固定窗口：`5m`
- 每个 `room` 只取最后状态

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
- 当前观众数
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

- 首页快照卡片必须有 freshness guard
- 通知类日志必须先定义“按行统计”还是“按逻辑事件统计”
- `newConsumer` 是事件量，不是人数
- `join/produce` 是请求级成功率，不是用户级成功率

做到这几点，第一版大盘就不会在定义上跑偏。
