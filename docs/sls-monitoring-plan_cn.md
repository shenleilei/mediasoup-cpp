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

而且由于这轮已经补上了：

- `producer closed`
- `consumer closed`
- `globalSnapshot`
- `roomSnapshot`

所以首页最关心的几个数字已经不必再靠事件流硬推断。

当前可以分成 3 类指标：

### 1.1 精确事件指标

这类可以直接按事件统计：

- `join` 成功率
- `createWebRtcTransport` 成功率
- `produce` 成功率
- `consume` 成功率
- `peerJoined / peerLeft` 次数
- `newConsumer` 下发次数
- `auto-subscribe FAILED` 次数和原因
- `qosConnectionQuality` 分布
- `qosOverride` 次数和 `reason` 分布
- `qosRoomState` 分布
- `statsReport` 广播次数
- `serverRestart` 次数

### 1.2 精确快照指标

这类直接用低频快照日志：

- 当前房间数
- 当前主播数
- 当前观众数
- 当前全站人数
- 当前房间人数

### 1.3 窗口活跃指标

这类仍然有价值，但更适合作为趋势盘补充：

- 近 1 分钟活跃房间数
- 近 5 分钟有 produce 的主播数
- 近 5 分钟有 downlink 上报的观众数
- 最近 5 分钟事件观察到的平均房间人数

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
- 成功率
- 失败率
- 错误原因

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
- join/leave 趋势
- 抖动房间
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
- newConsumer 次数
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
5. `join` 成功率
6. `produce` 成功率
7. `newConsumer` 下发量
8. `poor/lost` peer 比例

再额外加 2 个稳定性卡片：

9. `serverRestart` 次数
10. `error/warn` 总量

这已经足够回答：

- 现在有多少房间
- 现在有多少主播
- 现在有多少观众
- 人能不能进房
- 人能不能发流
- 订阅链是否正常
- 质量是不是在恶化
- 服务是不是在抖

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

### 是否建议报警

- 一般不建议

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

### 是否建议报警

- 一般不建议

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

### 是否建议报警

- 一般不建议

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

### 是否建议报警

- 一般不建议

## 4.5 join 成功率

### 使用日志

- `[request-done] method=join ok=true/false`

### 统计口径

- `ok=true / 全部 join 请求`

### 推荐图表

- 单值卡片
- 5 分钟趋势折线

### 是否建议报警

- 建议

## 4.6 produce 成功率

### 使用日志

- `[request-done] method=produce ok=true/false`

### 统计口径

- `ok=true / 全部 produce 请求`

### 推荐图表

- 单值卡片
- 按 `kind/source` 分组趋势

### 是否建议报警

- 建议

## 4.7 newConsumer 下发量

### 使用日志

- `notify newConsumer target=... producerId=... consumerId=... kind=...`

### 统计口径

- 每分钟条数

### 推荐图表

- 分钟级柱状图
- 按 `kind` 分组

### 是否建议报警

- 一般不单独报警

## 4.8 poor/lost peer 比例

### 使用日志

- `notify qosConnectionQuality target=... quality=...`

### 统计口径

- 最近 5 分钟：
  - `quality in (poor,lost)` 数量
  - 除以全部 connection quality 通知数量

### 推荐图表

- 单值卡片
- `excellent/good/poor/lost` 分布图

### 是否建议报警

- 建议

## 4.9 serverRestart 次数

### 使用日志

- `notify serverRestart`

### 统计口径

- 最近 5 分钟计数

### 推荐图表

- 单值卡片
- 时间线

### 是否建议报警

- 强烈建议

## 4.10 error/warn 总量

### 使用日志

- `warning`
- `error`

### 统计口径

- 最近 5 分钟 error/warn 数

### 推荐图表

- 双折线
- TopN 错误文本

### 是否建议报警

- error 可做观察告警

---

## 5. 第二屏：信令与媒体

建议放：

- `createWebRtcTransport` 成功率
- `consume` 成功率
- `auto-subscribe FAILED` 次数
- `peerJoined / peerLeft` 趋势
- `newConsumer` 次数
- `produce kind/source` 分布
- `producer closed` 次数
- `consumer closed` 次数

## 5.1 createWebRtcTransport 成功率

使用日志：

- `[request-done] method=createWebRtcTransport ok=true/false`

## 5.2 consume 成功率

使用日志：

- `[request-done] method=consume ok=true/false`

## 5.3 auto-subscribe FAILED

使用日志：

- `auto-subscribe FAILED for ...: ...`

建议附加：

- 错误原因 TopN
- room TopN
- peer TopN

## 5.4 join/leave 抖动

使用日志：

- `notify peerJoined`
- `notify peerLeft`

## 5.5 producer closed

使用日志：

- `producer closed producerId=... kind=... source=... reason=... remainingProducers=...`

适合做：

- producer 结束原因分布
- transportclose / close / reconnect 替换等结束原因分析

## 5.6 consumer closed

使用日志：

- `consumer closed consumerId=... producerId=... kind=... reason=... remainingConsumers=...`

适合做：

- consumer 结束原因分布
- `producerclose / transportclose / close` 区分

---

## 6. 第三屏：QoS

建议放：

- `qosConnectionQuality` 分布
- `qosRoomState` 分布
- `qosOverride reason` 分布
- `statsReport` 广播量

## 6.1 peer 级质量

使用日志：

- `notify qosConnectionQuality target=... quality=...`

## 6.2 room 级质量

使用日志：

- `notify qosRoomState quality=... peers=...`

## 6.3 QoS 干预频率

使用日志：

- `notify qosOverride target=... reason=...`

重点看：

- `server_auto_poor`
- `server_auto_lost`
- `server_auto_clear`
- `server_room_pressure_clear`
- `server_ttl_expired`
- `downlink_v2_demand_restored`

## 6.4 stats 活跃度

使用日志：

- `notify statsReport targetPeerCount=... targetPeers=... statsPeers=...`

---

## 7. 第四屏：异常房间

建议放：

- `auto-subscribe FAILED` 最多的房间
- `poor/lost` 最多的房间
- `join/leave` 最抖的房间
- `error` 最多的房间
- `serverRestart` 影响最多的房间

这屏主要用于排障，不是首页。

---

## 8. 告警执行表

## 8.1 P1 告警

### A. serverRestart

使用日志：

- `notify serverRestart`

规则：

- 任意 5 分钟 >= 1

### B. join 成功率下降

使用日志：

- `[request-done] method=join`

规则：

- 5 分钟成功率低于阈值

### C. produce 成功率下降

使用日志：

- `[request-done] method=produce`

规则：

- 5 分钟成功率低于阈值

## 8.2 P2 告警

### D. auto-subscribe FAILED 激增

使用日志：

- `auto-subscribe FAILED`

规则：

- 5 分钟内次数突增

### E. poor/lost peer 比例过高

使用日志：

- `notify qosConnectionQuality quality=poor/lost`

规则：

- 窗口占比高于阈值

### F. poor/lost 房间数过高

使用日志：

- `notify qosRoomState quality=poor/lost`

规则：

- 窗口内数量高于阈值

---

## 9. 当前仍不适合直接做的项

现在还不适合直接做成高质量指标的，主要是这三类：

## 9.1 请求耗时 P95/P99

原因：

- 还没有统一 `costMs`

## 9.2 统一错误码维度

原因：

- 现在很多错误还是自由文本

## 9.3 更细粒度的观众分类

例如：

- 只听音频的人
- 真正在看视频的人
- 页面不可见但仍有订阅的人

原因：

- 现在只有 `subscribers` 这个快照粒度

---

## 10. 推荐的实际落地顺序

### 第一批立刻做

- 当前房间数
- 当前主播数
- 当前观众数
- 当前全站人数
- join 成功率
- produce 成功率
- newConsumer 下发量
- poor/lost peer 比例
- serverRestart 告警

### 第二批补齐

- consume 成功率
- auto-subscribe FAILED TopN
- qosRoomState 分布
- qosOverride reason 分布
- producer/consumer closed 原因分布
- 异常房间 TopN

### 第三批再优化

- `costMs`
- `errorCode`
- 更细粒度 viewer 快照

---

## 11. 最后一句话

如果你现在就要让 SLS 同学开工，这份文档已经足够。

最关键的一点是：

- **首页已经可以直接用快照日志做当前房间 / 主播 / 观众 / 总人数**
- **同时保留窗口活跃值作为趋势盘，不要只看静态快照**
- **先把 join / produce / newConsumer / quality / restart 这五类最关键指标看住**

这样第一版既能落地，也不会误导后面的值班和业务方。
