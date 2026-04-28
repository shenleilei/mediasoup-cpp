# Bugfix Analysis

## Summary

`mediasoup_thread_integration_tests` 中使用 legacy pacing fallback 的线程集成测试会在 `NetworkThread::stop()` 阶段长时间自旋，导致测试进程持续占满 CPU 且无法退出。

## Reproduction

1. 构造 `NetworkThread`，设置 `enableTransportController = false`。
2. 注册视频 track，但不额外下发 transport bitrate hint。
3. 推入一个会产生多包 RTP 的大 H264 access unit，随后调用 `stop()`。
4. 观察测试进程在 `stop()` 内长时间运行，`perf` 热点集中在 `steady_clock::now()`。

## Observed Behavior

- legacy pacing fallback 没有显式 transport hint 时，默认目标码率为 0，无法累积 pacing budget。
- 当 pacing 队列里仍有待发送包时，`NetworkThread::stop()` 会反复调用 `pacingFlush()`，但在零 budget 条件下永远不产生进展。
- 结果是测试 teardown busy-spin，线程集成测试长时间“狂奔”。

## Expected Behavior

- legacy pacing fallback 在未收到显式 transport hint 前，仍应具备非零默认目标码率并能够发送 RTP。
- `NetworkThread::stop()` 必须有界完成；即使 legacy pacing 队列无法完全发送，也不能 busy-spin。

## Known Scope

- Affected modules
  - `client/NetworkThread.h`
  - `client/SenderTransportController.h`
  - `tests/test_thread_integration.cpp`
  - `specs/current/runtime-safety.md`
- Related components
  - legacy pacing fallback
  - threaded plain-client network teardown

## Must Not Regress

- transport-controller 启用路径下的 pacing、TWCC 和 shutdown 行为保持不变。
- legacy pacing fallback 现有可发送 RTP 的场景不能因为 shutdown guardrail 被提前丢包到完全不发。
- pause / resume / stats / probe 相关线程集成测试不能被本次修复破坏。

## Suspected Root Cause

高置信度：legacy pacing 使用的 `TrackNetState.targetBitrateBps` 初始值为 0，而 `stop()` 的 legacy flush 分支又依赖 pacing budget 驱动发送。零 budget + 非空队列导致 teardown 循环只做时间轮询而不前进。

## Acceptance Criteria

### Requirement 1

在 transport controller 关闭时，legacy pacing fallback SHALL 在未收到显式 transport hint 前仍然能发送 RTP。

#### Scenario: Default legacy pacing budget

- WHEN `NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback` 运行
- THEN 测试 SHALL 观察到至少一个 RTP 包被发出
- AND `stop()` SHALL 在有界时间内返回

### Requirement 2

`NetworkThread::stop()` SHALL 在 legacy pacing 队列无法继续推进时有界退出，而不是 busy-spin。

#### Scenario: Zero target bitrate shutdown

- WHEN legacy pacing 队列里仍有待发送包且有效目标码率为 0
- THEN `stop()` SHALL 在有界时间内返回
- AND 剩余队列 SHALL 被安全 drain 或 drop

## Regression Expectations

- Existing unaffected behavior:
  - transport-controller 启用路径下的发送端 BWE 行为
  - pause / resume ack 相关线程测试
- Required automated regression coverage:
  - `NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback`
  - 新增 legacy shutdown guardrail 集成测试
  - 邻近线程集成回归用例
- Required manual smoke checks:
  - 无
