# Bugfix Analysis

## Summary

`scripts/run_all_tests.sh` 触发的 full QoS regression 当前还会被两个后续问题卡住：

- `QosIntegrationTest.DownlinkClientStatsStored` 偶发失败
- `browser-harness:downlink-priority` 中 `PriorityCompetitionScreenShareVsGrid` 偶发失败

## Reproduction

1. 运行 `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsStored --gtest_repeat=15 --gtest_break_on_failure`
2. 运行 `node tests/qos_harness/browser_downlink_priority.mjs`
3. 观察 full QoS regression 偶发失败

## Observed Behavior

- `DownlinkClientStatsStored` 在 `downlinkClientStats` 返回成功后立即调用 `getStats`，偶发读不到 `downlinkQos`
- `PriorityCompetitionScreenShareVsGrid` 偶发出现：
  - 高优先级 screen-share consumer 被 pause，低优先级 grid consumer 继续保活
  - 或两者都被压到同档，导致 `highBetter=0/5`

## Expected Behavior

- 接受的 downlink snapshot 必须稳定出现在 `getStats` 中，相关集成测试不能依赖 worker 异步 planning 的瞬时调度
- 在单 subscriber 的紧预算竞争下，screen-share consumer 不应系统性落后于 visible grid consumer
- full QoS regression 的这两个入口必须可稳定回归，不再作为 flake 挡住 `run_all_tests.sh`

## Known Scope

- `tests/test_qos_integration.cpp`
- `tests/qos_harness/browser_downlink_priority.mjs`
- `src/qos/SubscriberBudgetAllocator.cpp`
- 可能涉及 `specs/current/`

## Must Not Regress

- `downlinkClientStats` 入库、校验、过滤语义不变
- downlink priority harness 其余场景保持可运行
- pinned / visible / hidden 现有 downlink 规则不回退

## Suspected Root Cause

- `DownlinkClientStatsStored`：测试把异步 worker planning 当成同步完成，且使用了不真实的 consumer / producer 映射，置信度高
- `PriorityCompetitionScreenShareVsGrid`：screen-share 默认成本模型过重，导致紧预算下 admission / upgrade 结果偏向 grid 或无法形成可观察优势，置信度高

## Acceptance Criteria

### Requirement 1

集成测试 SHALL 使用真实 downlink consumer / producer 映射，并对异步 planning 收敛做显式等待。

#### Scenario: Stored downlink stats become observable

- WHEN subscriber 成功上报有效 `downlinkClientStats`
- THEN `getStats` 能稳定看到入库后的 `downlinkClientStats`
- AND 测试只在 `downlinkQos` 异步收敛后再断言 health / degradeLevel

### Requirement 2

默认 downlink subscriber 预算模型 SHALL 让 screen-share 在紧预算下不落后于 visible grid。

#### Scenario: Screen-share competes with grid

- WHEN 一个 subscriber 同时消费 screen-share 与 grid video，且预算只够保留有限层级
- THEN screen-share 的 admission / layer 结果不低于 grid

## Regression Expectations

- Existing unaffected behavior:
  - malformed / stale downlink stats 校验保持不变
  - pinned / visible / hidden 的 downlink 控制保持不变
- Required automated regression coverage:
  - `DownlinkClientStatsStored` targeted gtest
  - 新增或更新的 SubscriberBudgetAllocator 单测
  - `browser_downlink_priority.mjs` 重复运行
- Required manual smoke checks:
  - 必要时执行 `scripts/run_qos_tests.sh cpp-integration browser-harness`
