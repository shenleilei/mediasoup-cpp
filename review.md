分支 Review 状态跟踪

**分支**: `dev/downside_qos` vs `main`  
**原始 Review 日期**: `2026-04-14`  
**最近整理日期**: `2026-04-15`  
**目的**: 这份文档不再保留“原始问题清单”形态，而是只回答哪些问题已经完成、哪些只完成一半、哪些暂不做，以及哪些问题需要按新边界重定义。

---

## 1. 已完成

下面这些问题已经在当前分支实现里落地，可以从后续执行清单中移除：

- `S1` `Consumer::close()` 空指针保护
- `S2` `RoomService` 目录句柄泄漏
- `S3` `QosValidator` 非有限值守卫
- `S4` `DownlinkQosRegistry` seq 回绕 / reset 处理
- `S5` `public/qos-demo.js` HTML 注入
- `S6` 客户端 `seq` 溢出处理
- `H1` `WorkerThread` 中 `epoll_ctl` / `timerfd_settime` 错误检查
- `H2` `ProducerDemandAggregator::finalize()` 初始化语义
- `H3` `DownlinkAllocator::ComputeDiff()` 状态同步问题
- `H4` `resolveProducerOwnerPeerId()` O(n×m) 查找
- `H5` `packetsLost` 累计值误用
- `H6` browser harness 临时目录未清理
- `H7` browser harness 固定端口冲突
- `H8` `SignalingServer` registry worker shutdown 丢任务
- `M1` `Peer::close()` 幂等性
- `M3` `downlinkSampler.js` 访问 transport 私有属性
- `M4` `downlinkSampler.js` 变异 WebRTC stats 对象
- `M5` `parseDownlinkSnapshot()` 修改入参
- `M6` `pauseUpstream` 和 `resumeUpstream` 可同时为 true
- `M7` integration test 硬编码 sleep 等待 respawn
- `M8` `CMakeLists.txt` glob 收集测试
- `L1` `DownlinkHealthMonitor` 冗余条件
- `L2` `downlinkHints.js` 对未知 consumer 静默忽略
- `L3` `wasActionApplied()` 默认返回 `true`
- `L5` `run_downlink_matrix.mjs` stdout/stderr 混淆
- `Q2` `consumerId ↔ producerId` 服务端一致性校验
- `Q3` `subscriptions.consumerId` 唯一性校验
- `Q4` `tsMs` 合理性约束
- `Q6` 无 `subscriberInputs` 时的 demand 衰减
- `Q9` `media kind` 模型边界

---

## 2. 部分完成

下面这些问题不是“完全没做”，但还没有达到原始 review 文档想表达的完整目标。

### 2.1 `M2` `SubscriberBudgetAllocator` 带宽估算硬编码常数

当前状态：

- 已支持环境变量覆盖：
  - `MEDIASOUP_QOS_BASE_BITRATE_BPS`
  - `MEDIASOUP_QOS_SCREENSHARE_BASE_BITRATE_BPS`

还差：

- 正式配置文件 / API 参数接入
- 更清晰的运行时可观测配置来源

### 2.2 `Q1` 订阅统计映射策略

当前状态：

- 已支持更稳定的匹配键：
  - `ssrc`
  - `mid`
  - `trackIdentifier`

还差：

- fallback 路径仍然存在
- 映射失败时缺少更强的可观测性 / 指标

### 2.3 `Q5` `downlinkClientStats` 频率限流

当前状态：

- 已有 peer 级最小间隔 rate limit
- 已有定向集成测试验证

还差：

- token bucket 级别的更完整控制
- 更细粒度拒绝指标 / 观测

---

## 3. 暂不做

下面这些问题当前明确不进入后续 patch 主线。

### 3.1 `Q7` override ACK / 重传闭环

当前决定：

- 暂不实现

原因：

- 当前主线先收口 worker 边界和 downlink control plane 职责
- ACK/重传不是当前最关键阻塞项

---

## 4. 需重定义

下面的问题原始问题意识仍然有价值，但原始表述已经不适合继续当作当前实现目标。

### 4.1 `Q8` 控制面 allocator 抗抖

原始定义：

- “预算分配每轮从零重算，缺少切换惩罚与驻留约束”

当前决定：

- 不继续按这个方向推进
- 当前控制面里的 `Q8` 相关 dwell 改动已经先回退

原因：

- `mediasoup-worker 3.14.6` 已经在 transport 内做了：
  - consumer bitrate allocation
  - priority 驱动分配
  - simulcast / SVC 层切换 anti-flap
- 控制面如果继续做“最终 consumer 分配器”，会与 worker transport allocator 重叠

新的问题定义应该是：

- control plane 给 worker 的高层意图是否稳定
- producer demand 是否能对齐 worker 实际 committed 的下行状态

进一步说明见：

- [downlink-qos-v3-design_cn.md](/root/mediasoup-cpp/docs/downlink-qos-v3-design_cn.md)
- [downlink-qos-v3-implementation-plan_cn.md](/root/mediasoup-cpp/docs/downlink-qos-v3-implementation-plan_cn.md)
- [mediasoup-worker-architecture-analysis_cn.md](/root/mediasoup-cpp/docs/mediasoup-worker-architecture-analysis_cn.md)

---

## 5. 当前剩余有效事项

如果只看当前还值得继续排期的 review 事项，优先级可以收敛成下面这几项：

1. `Q7` override ACK / 重传闭环
2. `Q1` 订阅统计映射剩余问题
3. `Q5` 更完整的 `downlinkClientStats` 限流 / 指标
4. `M2` bitrate 基线配置正式化
5. `Q8` 的新问题定义：
   - 意图稳定性
   - producer demand 与 worker 实际状态对齐

---

## 6. 推荐阅读顺序

如果后面继续做 downlink QoS，建议按下面顺序看：

1. [mediasoup-worker-architecture-analysis_cn.md](/root/mediasoup-cpp/docs/mediasoup-worker-architecture-analysis_cn.md)
2. [downlink-qos-v3-design_cn.md](/root/mediasoup-cpp/docs/downlink-qos-v3-design_cn.md)
3. [downlink-qos-v3-implementation-plan_cn.md](/root/mediasoup-cpp/docs/downlink-qos-v3-implementation-plan_cn.md)

这份 `review.md` 之后只保留“状态跟踪”用途，不再作为原始 patch todo 使用。
