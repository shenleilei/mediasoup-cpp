# Design

## Context

该问题同时包含一个正常运行路径缺陷和一个 teardown 安全护栏缺陷：

1. legacy pacing fallback 的默认目标码率与 transport-controller 路径不对齐，导致未下发 hint 时无法发送。
2. legacy pacing shutdown 逻辑对“零预算但队列非空”的状态没有退出护栏，导致 busy-spin。

## Chosen Approach

### 1. 对齐 legacy fallback 的默认目标码率

- 将 `SenderTransportController` 的默认视频目标码率提升为可复用常量。
- `NetworkThread::registerVideoTrack()` 在本地 `TrackNetState` 上也使用同一默认值初始化 `targetBitrateBps`。

这样可以保证：

- transport-controller 启用和关闭两条路径对默认目标码率的语义一致。
- legacy fallback 在没有额外 transport hint 时仍能累积 pacing budget 并发送 RTP。

### 2. 为 legacy shutdown 增加有界 drain / drop 护栏

- 将 legacy shutdown flush 提取为专用 helper。
- shutdown 时先给 legacy pacing 一个足够大的临时 budget，尽力 drain 剩余队列。
- 若 drain 后仍有残留包，则记录告警并直接清空队列，避免 `stop()` 无限循环。

这样可以保证：

- teardown 不再依赖正常 pacing budget 的推进。
- 即使底层 socket `WouldBlock` 或未来引入新的停止条件，shutdown 仍然有界完成。

## Alternatives Considered

### Only fix `stop()`

拒绝。这样只能避免 hang，不能修复 legacy fallback 默认无法发送 RTP 的行为漂移。

### Only set a default bitrate

不够。即使默认码率修复了当前回归，`stop()` 仍然缺少独立的 shutdown guardrail，后续仍可能因别的零进展条件 busy-spin。

## Verification Plan

- 运行 legacy fallback 原回归用例，验证能发 RTP 且 `stop()` 不超时。
- 新增并运行显式零目标码率的 shutdown guardrail 测试，验证 `stop()` 有界返回。
- 运行紧邻的线程集成用例，确认未破坏 pause / resume 路径。
