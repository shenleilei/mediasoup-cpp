# 下行 QoS 分阶段实施计划

## 1. 文档目的

这份文档不是“设计说明”，而是“落地实施计划”。

本文只覆盖 `downlink QoS v1`。

`v2` 的房间级 demand 聚合、publisher 供给回收和显式 bitrate allocator，统一见：

- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)

目标是把当前仓库从：

- 已有 uplink QoS
- 缺少 subscriber/downlink QoS

逐步推进到：

- 具备可验证的 downlink QoS v1

本文重点回答：

- 下一步先做什么
- 每一阶段改哪些文件
- 每一阶段不要做什么
- 每一阶段的验收标准是什么

## 2. 总原则

实施顺序必须克制，不要一次性把整个 downlink QoS 做满。

建议遵守这几个原则：

1. 先验证现有 `Consumer` API 的黑盒行为，再做自动控制器
2. 先做手动控制链路，再做自动决策
3. 先做 `pause/resume + preferredLayers`，最后再做 `priority`
4. `v2` 内容不在本文展开
5. 先不改 `mediasoup-worker`

## 3. 阶段总览

建议按以下 6 个阶段推进：

1. Consumer 控制接口与最小集成测试
2. 浏览器黑盒验证 Consumer 动作
3. downlink snapshot schema 与服务端存储
4. 最小 downlink QoS v1：可见性与尺寸驱动
5. 健康度驱动的降档与恢复
6. 优先级效果验证与收口

---

## 4. 阶段 1：Consumer 控制接口与最小集成测试

### 4.1 目标

先把服务端对单个 consumer 的控制链路走通。

这一阶段的目标不是“自动 downlink QoS”，而是：

- 有可调用的 consumer 控制接口
- 有最小黑盒测试
- 后续 browser harness 能直接复用

### 4.2 需要修改的文件

- [RoomService.h](../src/RoomService.h)
- [RoomService.cpp](../src/RoomService.cpp)
- [SignalingServer.cpp](../src/SignalingServer.cpp)
- [Consumer.h](../src/Consumer.h)
- [Consumer.cpp](../src/Consumer.cpp)
- [test_integration.cpp](../tests/test_integration.cpp)

### 4.3 新增的 RoomService 方法

建议新增：

- `pauseConsumer(roomId, peerId, consumerId)`
- `resumeConsumer(roomId, peerId, consumerId)`
- `setConsumerPreferredLayers(roomId, peerId, consumerId, spatialLayer, temporalLayer)`
- `setConsumerPriority(roomId, peerId, consumerId, priority)`
- `requestConsumerKeyFrame(roomId, peerId, consumerId)`

### 4.4 行为要求

这些方法只允许操作“调用者自己名下的 consumer”。

处理逻辑：

1. 找 room
2. 找 peer
3. 在 `peer->consumers` 中找 `consumerId`
4. 找不到时返回明确错误

成功后统一返回：

- `consumer->toJson()`

### 4.5 Consumer wrapper 需要补的状态

为了让控制面可调试，建议给 `Consumer` 增加本地状态字段：

- `preferredSpatialLayer_`
- `preferredTemporalLayer_`
- `priority_`

并在 `toJson()` 中返回。

### 4.6 Signaling method

建议新增：

- `pauseConsumer`
- `resumeConsumer`
- `setConsumerPreferredLayers`
- `setConsumerPriority`
- `requestConsumerKeyFrame`

### 4.7 测试

这一阶段只补最小控制面测试：

- `IntegrationTest.PauseResumeConsumerControl`
- `IntegrationTest.SetConsumerPriorityControl`
- `IntegrationTest.RequestConsumerKeyFrameControl`

先不要写：

- `setPreferredLayers` 的真实效果测试
- `priority` 的真实带宽竞争测试

### 4.8 验收标准

- `mediasoup-sfu` 能编译通过
- 新增 integration test 通过
- signaling method 可正常返回

---

## 5. 阶段 2：浏览器黑盒验证 Consumer 动作

### 5.1 目标

验证这些 `Consumer` 动作在真实浏览器 subscriber 上有实际效果。

也就是验证：

- `pauseConsumer`
- `resumeConsumer`
- `requestConsumerKeyFrame`

### 5.2 需要新增的文件

- `tests/qos_harness/browser/downlink-controls-entry.js`
- `tests/qos_harness/browser_downlink_controls.mjs`

### 5.3 复用风格

优先参考现有 browser harness：

- [loopback-entry.js](../tests/qos_harness/browser/loopback-entry.js)
- [browser_loopback.mjs](../tests/qos_harness/browser_loopback.mjs)
- [browser_server_signal.mjs](../tests/qos_harness/browser_server_signal.mjs)

### 5.4 页面里要模拟的角色

同一个页面里模拟两个 peer：

- publisher
- subscriber

publisher：

- join
- create send transport
- publish video

subscriber：

- join
- create recv transport
- 接收 remote consumer
- 播放远端视频

### 5.5 页面需要暴露的方法

建议挂到 `window.__downlinkControlsHarness`：

- `init(...)`
- `waitForConsumerReady()`
- `getSubscriberVideoStats()`
- `pauseConsumer()`
- `resumeConsumer()`
- `requestConsumerKeyFrame()`

### 5.6 subscriber 需要采的指标

从 `recvTransport._handler._pc.getStats()` 与 video element 中汇总：

- `framesDecoded`
- `bytesReceived`
- `framesPerSecond`
- `frameWidth`
- `frameHeight`
- `packetsLost`
- `jitter`
- `video.currentTime`

### 5.7 先做的 3 个 case

1. `pauseConsumer stops subscriber video`
2. `resumeConsumer restores subscriber video`
3. `requestConsumerKeyFrame improves resume recovery`

### 5.8 当前阶段先不要做

- `setPreferredLayers` 效果验证
- `setPriority` 带宽竞争验证
- 自动 downlink controller
- netem 弱网矩阵

### 5.9 验收标准

```bash
node tests/qos_harness/browser_downlink_controls.mjs
```

需要得到：

- baseline active
- paused inactive
- resumed active
- 带 keyframe 与不带 keyframe 的恢复时间对比

---

## 6. 阶段 3：downlink snapshot schema 与服务端存储

### 6.1 目标

先把 subscriber 的下行状态和 UI 需求送到服务端，但还不做自动决策。

### 6.2 建议新增文件

- `src/qos/DownlinkQosTypes.h`
- `src/qos/DownlinkQosRegistry.h`
- `src/qos/DownlinkQosRegistry.cpp`
- `src/client/lib/qos/downlinkProtocol.js`
- `src/client/lib/qos/downlinkTypes.d.ts`
- `src/client/lib/qos/downlinkSampler.js`
- `src/client/lib/qos/downlinkHints.js`

### 6.3 建议修改文件

- [SignalingServer.cpp](../src/SignalingServer.cpp)
- [RoomService.h](../src/RoomService.h)
- [RoomService.cpp](../src/RoomService.cpp)
- [CMakeLists.txt](../CMakeLists.txt)

### 6.4 新 signaling method

- `downlinkClientStats`

### 6.5 最小 schema

建议最小字段：

- `schema`
- `seq`
- `tsMs`
- `subscriberPeerId`
- `transport.availableIncomingBitrate`
- `transport.currentRoundTripTime`
- `subscriptions[]`
  - `consumerId`
  - `producerId`
  - `visible`
  - `pinned`
  - `activeSpeaker`
  - `isScreenShare`
  - `targetWidth`
  - `targetHeight`
  - `packetsLost`
  - `jitter`
  - `framesPerSecond`
  - `frameWidth`
  - `frameHeight`
  - `freezeRate`

### 6.6 RoomService 新方法

- `setDownlinkClientStats(...)`

### 6.7 测试

建议补到：

- [test_qos_integration.cpp](../tests/test_qos_integration.cpp)

测试名称建议：

- `DownlinkClientStatsStored`
- `DownlinkClientStatsRejectsMalformedPayload`
- `DownlinkClientStatsRejectsStaleSeq`

### 6.8 验收标准

- downlink snapshot 能从 browser 发送到 server
- server 能校验与存储
- `getStats` 或调试输出能看见 downlink snapshot

---

## 7. 阶段 4：最小 downlink QoS v1（可见性 + 尺寸）

### 7.1 目标

实现最值钱、最稳定的 downlink QoS 第一版。

### 7.2 建议新增文件

- `src/qos/DownlinkAllocator.h`
- `src/qos/DownlinkAllocator.cpp`
- `src/qos/SubscriberQosController.h`
- `src/qos/SubscriberQosController.cpp`

### 7.3 建议修改文件

- [RoomService.h](../src/RoomService.h)
- [RoomService.cpp](../src/RoomService.cpp)

### 7.4 `v1` 只做这 3 条规则

1. hidden -> `pauseConsumer`
2. visible + small tile -> low preferred layer
3. visible + large/pinned -> higher preferred layer

### 7.5 恢复顺序

从 hidden 恢复时，动作顺序建议：

1. `resumeConsumer`
2. `setPreferredLayers`
3. `requestConsumerKeyFrame`

### 7.6 暂时不要做

- `setPriority` 自动化
- freeze/loss/jitter 复杂状态机
- publisher 供给回收

### 7.7 测试建议

单测：

- `DownlinkAllocatorHiddenPausesConsumer`
- `DownlinkAllocatorPinnedGetsHigherLayer`
- `DownlinkAllocatorSmallTileSelectsLowLayer`

集成测试：

- hidden 视频自动暂停
- 恢复可见后自动恢复
- 大小窗收到不同层

### 7.8 验收标准

- hidden 视频自动停
- visible 后自动恢复
- 大小窗可区分层级

---

## 8. 阶段 5：健康度驱动的降档与恢复

### 8.1 目标

让 downlink QoS 不只是 UI 自适应，而是根据真实接收质量做控制。

### 8.2 输入信号

核心关注：

- `freezeRate`
- `framesPerSecond`
- `packetsLost`
- `jitter`
- `availableIncomingBitrate`
- `currentRoundTripTime`

### 8.3 状态机

建议新增状态：

- `stable`
- `early_warning`
- `congested`
- `recovering`

### 8.4 行为建议

- freeze 上升 -> 快速降档
- FPS 明显下降 -> 降档
- loss / jitter / RTT 变差 -> 降档
- 恢复时每次只升一档
- 升档后请求 keyframe

### 8.5 测试建议

- `DownlinkControllerFreezeTriggersDegrade`
- `DownlinkControllerRecoveryIsStepwise`
- browser harness 增加 freeze/recovery 场景

### 8.6 验收标准

- 差网下会真实降档
- 恢复不明显振荡

---

## 9. 阶段 6：优先级效果验证与收口

### 9.1 当前状态判断

这一阶段当前不能只按“代码已写完”来判定完成。

如果仓库里已经具备：

- `DownlinkAllocator::ComputePriority()`
- 自动 `kSetPriority` action
- `SubscriberQosController` 调用 `consumer->setPriority()`
- 单测验证 priority 常量排序

那么这只能说明：

- 阶段 6 的“代码逻辑部分”已具备

但仍然不能说明：

- 阶段 6 已经完成

真正的完成标准是：

- 浏览器级真实竞争场景里，能稳定观察到高优先级 consumer 优于低优先级 consumer

如果没有这个证据，阶段 6 应标记为：

- 代码逻辑部分完成
- 浏览器级竞争验证未完成

### 9.2 本阶段目标

本阶段的目标不是继续堆规则，而是验证现有 priority 逻辑在真实受限带宽场景下是否可观察、可重复。

具体要证明两件事：

1. 同一 subscriber 同时接收两路视频时，高优先级 consumer 在受限带宽下优于低优先级 consumer
2. 带宽恢复后，高优先级 consumer 的恢复不晚于低优先级 consumer

### 9.3 非目标

这一阶段先不要扩展到以下内容：

- `v2` 的显式 bitrate allocator
- `v2` 的 publisher 供给回收
- “低优先级 consumer 必然暂停”的强假设

换句话说，这一阶段是：

- 验证现有 `setPriority()` 与 allocator 行为

而不是：

- 立即实现真正的 bitrate-aware allocator

### 9.4 优先级模型

建议默认优先级：

- screenshare: `255`
- pinned: `220`
- active speaker: `180`
- visible grid: `120`
- hidden: `0`

### 9.5 Claude 执行清单

下面这份清单是给 Claude 直接执行的，不需要再做产品层决策。

1. 盘点现有 browser harness 能力
2. 复用现有 downlink browser e2e 基础设施，不新起一套测试框架
3. 扩展为双 publisher、单 subscriber 的竞争场景
4. 确保 subscriber 能同时拿到两路 `consumerId`
5. 为每一路 consumer 单独发送 downlink snapshot
6. 用 snapshot 字段区分优先级
7. 一路使用 `pinned=true` 或 `isScreenShare=true`
8. 另一路保持普通 `visible grid`
9. 引入稳定的下行带宽限制手段
10. 在限速前做基线采样
11. 在限速期间连续采样两路 consumer 状态
12. 在恢复阶段继续采样并比较恢复顺序
13. 失败时输出两路 consumer 的时间序列状态

Claude 在这一阶段应优先复用：

- `getConsumerState`
- 现有 browser downlink harness
- 现有 signaling helper

不要在本阶段自行扩展到：

- allocator 新设计
- worker 改造
- `v2` 设计里的新供给控制链路

### 9.6 推荐测试列表

至少补以下浏览器级测试：

1. `PriorityCompetitionPinnedVsGrid`
2. `PriorityCompetitionScreenShareVsGrid`
3. `RecoveryAfterThrottleRelease`
4. `NoRegressionWithoutThrottle`

建议每个测试都写清：

- 输入场景
- 建链步骤
- 限速时机
- 采样窗口
- 断言条件
- 失败输出

### 9.7 具体断言策略

断言必须基于 consumer 实际状态，而不是 signaling 成功回包。

推荐主断言字段：

- `paused`
- `preferredSpatialLayer`
- `preferredTemporalLayer`
- `priority`

推荐比较顺序：

1. 未暂停优于暂停
2. 更高 `preferredSpatialLayer` 优于更低层
3. 若 spatial 相同，则更高 `preferredTemporalLayer` 优于更低层

这里 `priority` 的作用是：

- 确认服务端已应用预期优先级

而不是：

- 单独作为“竞争结果已成立”的证据

不要用单次采样做判定。

应要求：

- 连续多个采样点都体现高优先级优于低优先级

这样可以避免瞬时网络噪声导致假阳性。

### 9.8 失败输出要求

测试失败时必须打印至少以下信息：

- 时间点
- consumer 标识
- `paused`
- `preferredSpatialLayer`
- `preferredTemporalLayer`
- `priority`

如果可以稳定拿到附加指标，也建议输出：

- consumer stats score
- 恢复耗时

目标是让失败日志本身就能回答：

- 是限速没有生效
- 是优先级没有应用
- 还是两路 consumer 根本没有体现竞争差异

### 9.9 完成标准

阶段 6 只有在以下条件满足时才算完成：

1. 至少一个真实受限带宽场景稳定体现高优先级 consumer 优于低优先级 consumer
2. 带宽恢复后，高优先级 consumer 恢复不晚于低优先级 consumer
3. 断言基于实际 consumer 状态，而不是仅验证 API 调用成功

如果最终结果是：

- 所有限速场景里两路 consumer 都看不出稳定差异

那么结论不应是“e2e 不稳定”就算了，而应明确写成：

- 当前实现还不足以证明阶段 6 完成
- 需要转入 [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) 中定义的显式 allocator 方案

### 9.10 Claude 交付物要求

Claude 在这一阶段的输出应至少包括：

- 浏览器级双流竞争测试
- 失败时可读的时间序列日志
- 对照组测试（不限速）
- 恢复阶段测试

如果 Claude 无法在现有实现上稳定测出优先级差异，则应在提交说明里明确写出：

- 现有 `priority` 逻辑已存在
- 但尚不能在真实竞争场景中稳定证明其效果
- 下一步应转入 [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) 中定义的 allocator，而不是继续补表层 case

---

## 10. 推荐提交节奏

不要攒一个大 patch，建议每阶段一个 commit：

1. `feat: add manual consumer control signaling`
2. `test: add browser downlink control harness`
3. `feat: add downlink client stats ingestion`
4. `feat: add minimal downlink qos allocator`
5. `feat: add downlink degradation and recovery logic`
6. `feat: add consumer priority based allocation`

## 11. 当前最推荐先做的两步

### 优先级 1

先做：

- 阶段 1：Consumer 控制接口

### 优先级 2

再做：

- 阶段 2：浏览器黑盒验证

原因：

- 只有这两步完成后，才能确定现有 worker API 是否足够支撑 downlink QoS v1
- 如果这两步没跑通，后续自动 controller 都会建立在不稳的基础上

## 12. 总结

当前最合理的推进方式不是“一口气做完整 downlink QoS”，而是：

1. 先把 consumer 控制接口补齐
2. 先做真实浏览器黑盒验证
3. 再补 snapshot schema
4. 再做最小自动 downlink allocator
5. 最后才进入复杂恢复和优先级

这样推进风险最小，也最利于让 Claude 这类代码助手分阶段稳定交付。
