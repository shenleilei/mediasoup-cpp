# Bugfix Design

## Context

当前 full QoS regression 剩余的两个失败点分别来自：

- 一个错误的集成测试同步假设
- 一个在 tight-budget 条件下不够保守的 screen-share 默认成本模型

二者都会把 full regression 结果拉成红色，但性质不同：

- 前者是测试缺陷
- 后者是预算模型与 harness 期望之间的真实行为偏差

## Root Cause

### 1. `DownlinkClientStatsStored`

`downlinkClientStats` 在 WebSocket 层只保证“请求已投递到 worker”，不保证 downlink planning 已经跑完。

`getStats` 中的 `downlinkQos` 则依赖 `subscriber controller` 与 health monitor，属于异步 planning 结果。

当前测试在请求成功后立即断言 `downlinkQos`，并且使用了假的 consumer / producer id，导致：

- 有效 snapshot 虽然已入库
- 但 `downlinkQos` 可能尚未 materialize
- 测试因此出现竞态失败

### 2. `PriorityCompetitionScreenShareVsGrid`

当前 `SubscriberBudgetAllocator` 的默认 screen-share 基线成本高于 camera/grid。

在 loopback throttle 下，candidate-pair 暴露的 `availableIncomingBitrate` 可能掉到非常低的范围，导致：

- screen-share base layer 不一定先被 admit
- 即使 priority 更高，也可能被 pause 或与 grid 一起压到相同层

这会让 browser harness 的 `highBetter` 判定偶发失败。

## Fix Strategy

### 1. 修正 `DownlinkClientStatsStored` 的测试契约

- 用真实 produce / consume 建立合法 consumer / producer 映射
- 在断言 `downlinkQos` 前轮询 `getStats`，等待异步 planning 收敛
- 保持服务端异步设计不变，不把 planning 强行改成同步

这样能避免把“系统异步特性”误报成回归。

### 2. 调整默认 screen-share 预算基线

- 下调 `SubscriberBudgetAllocator` 的默认 `kScreenShareBaseBps`
- 保留环境变量 override 作为诊断/调参入口
- 新增 tighter-budget 单测，锁定“screen-share 至少不低于 grid”的默认行为

### 3. 最小幅度硬化 browser harness

如果代码修正后仍存在边界时序抖动，则在 priority harness 中把固定 sleep 基线等待改成收敛式等待：

- 先多次发送 snapshot
- 再等待服务端 consumer state 收敛到可评估状态

这一步只在必要时引入，避免扩大改动面。

## Risk Assessment

- 下调 screen-share 默认成本可能影响已有 downlink allocation 结果
- 集成测试改用真实映射后，执行时间会略增
- browser harness 若额外引入收敛等待，需要避免把真实失败掩盖成无限等待

必须保持：

- stale / malformed downlink stats 行为不变
- hidden / pinned / visible 的既有分配规则不倒退
- full regression 入口和报告结构不变

## Test Strategy

- Reproduction test:
  - `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsStored --gtest_repeat=15 --gtest_break_on_failure`
  - `node tests/qos_harness/browser_downlink_priority.mjs`
- Regression test:
  - targeted `QosIntegrationTest.DownlinkClientStatsStored`
  - updated `SubscriberBudgetAllocator` unit tests
- Adjacent-path checks:
  - `DownlinkClientStatsDropsInconsistentSubscriptions`
  - 其他 priority harness case
- Integration test:
  - `scripts/run_qos_tests.sh cpp-integration browser-harness`

## Observability

- 不新增生产日志
- harness 失败时继续依赖已有 timeline 与 diagnostics

## Rollout Notes

- 无迁移需求
- 若 screen-share 默认成本下调引出意外行为，可通过环境变量快速回退到旧值
