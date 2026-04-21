# 下行 QoS 前置验证清单（worker / API 黑盒）

## 1. 文档目的

在正式实现 `subscriber/downlink QoS` 之前，需要先验证当前仓库依赖的 `Consumer` 控制能力，
在真实 `mediasoup-worker` 上的黑盒行为是否符合预期。

原因很简单：

- 当前仓库已经封装了 `Consumer::pause/resume/setPreferredLayers/setPriority/requestKeyFrame`
- 但如果不先做黑盒验证，就只能依据 API 名称推断行为
- 下行 QoS 一旦要以这些能力为基础，就必须先把语义边界验证清楚

这个文档不讨论 downlink QoS 策略本身，而是列出“在做策略之前必须补齐的 worker / API 行为验证项”。

## 2. 当前可用的 Consumer 控制能力

当前仓库已经暴露这些动作：

- `pause()`
- `resume()`
- `setPreferredLayers(spatialLayer, temporalLayer)`
- `setPriority(priority)`
- `requestKeyFrame()`

相关代码：

- [Consumer.h](../../../src/Consumer.h)
- [Consumer.cpp](../../../src/Consumer.cpp)
- [request.fbs](../../../fbs/request.fbs)

这些动作将是后续 downlink QoS v1 的执行基础。

## 3. 为什么必须先补验证

如果后续下行 QoS controller 直接依赖这些动作，但前面没有把它们的真实行为验证清楚，
就会出现几个典型风险：

- `pause()` 只是本地状态变化，但实际 RTP 还在流
- `resume()` 恢复后需要很长时间才能真正出画
- `setPreferredLayers()` 对 simulcast 生效，但对当前实际测试编码形态几乎没有效果
- `setPriority()` 改了数值，但在受限带宽下没有实际差异
- `requestKeyFrame()` 调用成功，但升档恢复速度没有明显改善

所以要先做“黑盒语义验证”，再把它们纳入 downlink QoS。

## 4. 需要验证的最小语义

### 4.1 `consumer.pause()`

需要验证：

- 对某条视频 consumer 调 `pause()` 后：
  - subscriber 是否不再收到新视频 RTP
  - stats / score /画面更新是否停止或显著下降
  - 其他 consumer 是否不受影响

验收标准：

- 对应 subscriber 的该路视频确实停住
- 不是只是本地标志变化

### 4.2 `consumer.resume()`

需要验证：

- `resume()` 后视频是否能恢复
- 恢复延迟大概是多少
- 是否需要额外 keyframe 才能快速恢复

验收标准：

- consumer 能稳定恢复
- 恢复后 stats / score /画面继续变化

### 4.3 `consumer.setPreferredLayers()`

需要验证：

- 对 simulcast 或可分层视频流：
  - `low -> medium -> high` 是否会改变实际接收分辨率 / fps / bitrate
- 降层和升层是否都能生效
- 升层是否明显慢于降层

验收标准：

- 订阅侧能观测到实际接收质量变化
- 不是只有内部状态变了

### 4.4 `consumer.setPriority()`

需要验证：

- 当同一 subscriber 同时接收多个远端视频，且下行受限时：
  - 高优先级 consumer 是否更能保住层级 / fps
  - 低优先级 consumer 是否更早降档或更容易被牺牲

验收标准：

- 在受限带宽场景下，高优先级 consumer 有可观测收益

### 4.5 `consumer.requestKeyFrame()`

需要验证：

- consumer 升层或 resume 后调用 `requestKeyFrame()`：
  - 恢复画面的时间是否明显缩短
  - 是否比“不请求 keyframe”更快锁定新层

验收标准：

- 它对“恢复辅助”有稳定正收益
- 而不是只是一个无害但没效果的调用

## 5. 建议优先补的测试

### 5.1 第一批：能力存在性验证

先补最小闭环，不引入复杂网络控制：

1. `pause/resume` 行为验证
2. `setPreferredLayers` 基本行为验证
3. `requestKeyFrame` 恢复行为验证

这三项优先级最高。

### 5.2 第二批：带宽竞争验证

再补：

4. `setPriority` 在受限带宽下的行为验证

这个测试价值很高，但搭建环境会更重，建议放第二批。

## 6. 建议测试分层

### 6.1 集成测试（首选）

建议先放在现有 integration / qos integration 测试体系里。

原因：

- 当前仓库已经有真实 SFU + WebSocket 客户端黑盒测试框架
- 不需要先引入浏览器页面驱动
- 可以先把信令和控制链路跑通

优先落点：

- [test_integration.cpp](../../../tests/test_integration.cpp)
- [test_qos_integration.cpp](../../../tests/test_qos_integration.cpp)

### 6.2 Browser harness（第二阶段）

当需要验证真实浏览器接收效果时，再补 browser harness：

- 分辨率变化
- FPS 变化
- freeze rate 变化
- 恢复时延

这部分更接近 downlink QoS 最终验收口径。

## 7. 建议的具体测试项

### Test A: Consumer pause stops downstream video

场景：

1. Alice 发布视频
2. Bob 订阅 Alice 视频
3. 服务端对 Bob 对应的 consumer 调 `pause()`

验证：

- Bob 侧视频 stats 不再继续增长
- 或对应 consumer score /实际播放更新停止

### Test B: Consumer resume restores downstream video

场景：

1. 在 Test A 基础上继续
2. 服务端对同一 consumer 调 `resume()`

验证：

- Bob 侧视频恢复
- 统计继续增长

### Test C: Preferred layers lower actual receive quality

场景：

1. Alice 发布分层视频
2. Bob 订阅
3. 服务端依次给 Bob 的 consumer 下发不同 preferred layers

验证：

- Bob 侧接收分辨率 / FPS / bitrate 有明显差异

### Test D: Request keyframe speeds recovery

场景：

1. Bob 的 consumer 从低层升回高层，或 resume 后恢复
2. 对比：
   - 不请求 keyframe
   - 请求 keyframe

验证：

- 请求 keyframe 的恢复时间更短

### Test E: Priority changes who survives under constrained downlink

场景：

1. Bob 同时订阅两个远端视频
2. 制造下行受限场景
3. 对两个 consumer 设置不同 priority

验证：

- 高优先级 consumer 更稳定保住质量
- 低优先级 consumer 更早降档或停掉

## 8. 做完这些验证后，downlink QoS v1 才能安全开工

只要上面这些测试跑通，我们就能更有把握地说：

- `pause/resume` 可以作为 hidden / offscreen 控制动作
- `setPreferredLayers` 可以作为大小窗 / 优先级差异的主要工具
- `setPriority` 可以作为带宽竞争时的补充控制
- `requestKeyFrame` 可以作为恢复辅助动作

这时再去写 `SubscriberQosController`，风险才可控。

## 9. 总结

在正式做 downlink QoS 之前，最应该补的不是更多策略文档，而是：

- 一组围绕 `Consumer` 控制能力的黑盒验证测试

建议优先级：

1. `pause/resume`
2. `setPreferredLayers`
3. `requestKeyFrame`
4. `setPriority`

只有这些能力在真实 `mediasoup-worker` 上的行为被验证清楚，
downlink QoS v1 才值得继续推进。
