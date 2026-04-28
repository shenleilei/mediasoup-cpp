# Tasks

- [x] 1. 对齐 legacy fallback 默认目标码率
  - Outcome: `NetworkThread` 注册视频 track 时具备与 transport-controller 相同的默认目标码率。
  - Main files: `client/NetworkThread.h`, `client/SenderTransportController.h`
  - Verification: `./mediasoup_thread_integration_tests --gtest_filter=NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback`

- [x] 2. 为 legacy shutdown 增加有界 drain / drop 护栏
  - Outcome: `NetworkThread::stop()` 在 legacy pacing 队列零进展时仍能有界返回。
  - Main files: `client/NetworkThread.h`
  - Verification: 新增零目标码率 shutdown 测试

- [x] 3. 增加线程集成回归覆盖并更新规格
  - Outcome: 原回归与新 guardrail 都有自动化覆盖，运行时规格反映 accepted behavior。
  - Main files: `tests/test_thread_integration.cpp`, `specs/current/runtime-safety.md`
  - Verification: `./build/mediasoup_thread_integration_tests --gtest_filter=NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback:NetworkThreadIntegration.LegacyPacingShutdownWithZeroTargetDoesNotSpin:Pacing.PacketsAreSpreadOverTime`

## Verification Notes

- 针对 legacy fallback 的原回归、新增 zero-target shutdown 护栏测试，以及相邻 pacing / pause 路径已通过。
- 当前 dirty worktree 下，完整 `mediasoup_thread_integration_tests` 仍存在一个与本变更无关的现有失败：`NetworkPause.PauseAckRequiresQuiescedTransport`。
