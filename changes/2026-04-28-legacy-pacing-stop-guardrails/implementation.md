# Implementation Notes

## Applied Changes

- 将 `SenderTransportController` 的默认视频目标码率公开为共享常量，并在 `NetworkThread::registerVideoTrack()` 中用于初始化 legacy pacing track 状态。
- 将 legacy pacing shutdown 逻辑从无限 `while (!pacingQueue_.empty()) pacingFlush();` 改为有界 drain / drop helper，避免 teardown busy-spin。
- 为线程集成测试新增：
  - 原回归的 `stop()` 有界时延断言
  - 显式零目标码率的 shutdown guardrail 回归测试
- 更新 `specs/current/runtime-safety.md`，记录 network-thread shutdown 与 legacy fallback 默认目标码率要求。

## Verification Evidence

- Build:
  - `cmake --build build --target mediasoup_thread_integration_tests -j4`
- Targeted tests:
  - `./build/mediasoup_thread_integration_tests --gtest_filter=NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback:NetworkThreadIntegration.LegacyPacingShutdownWithZeroTargetDoesNotSpin:Pacing.PacketsAreSpreadOverTime`
  - Result: 3/3 passed
- Full thread integration suite:
  - `timeout 120s ./build/mediasoup_thread_integration_tests --gtest_color=no`
  - Result: legacy pacing regression no longer hangs; suite still reports one existing failure outside this change scope: `NetworkPause.PauseAckRequiresQuiescedTransport`

## Follow-Up Debt

- `NetworkPause.PauseAckRequiresQuiescedTransport` currently fails on the dirty worktree because pause ack is emitted before transport is fully quiesced on the transport-controller path. This is a separate behavior bug and should be handled in its own Structured Change.
