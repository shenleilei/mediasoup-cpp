# Tasks

## 1. Reproduce

- [x] 1.1 固化两个失败入口的可复现实验
  Verification: 记录 targeted 重跑结果

## 2. Fix

- [x] 2.1 修正 `DownlinkClientStatsStored` 的测试前置与等待逻辑
  Files: `tests/test_qos_integration.cpp`
  Verification: `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsStored`

- [x] 2.2 调整 screen-share 默认预算模型
  Files: `src/qos/SubscriberBudgetAllocator.cpp`, `tests/test_downlink_v2.cpp`
  Verification: `node tests/qos_harness/browser_downlink_priority.mjs`

## 3. Unit And Integration Coverage

- [x] 3.1 增补 tight-budget screen-share vs grid 单测
  Verification: targeted gtest for updated allocator coverage

- [x] 3.2 确认 `DownlinkClientStatsStored` 与邻近 downlink stats 用例稳定通过
  Verification: targeted `mediasoup_qos_integration_tests` filter

## 4. Guard Against Regression

- [x] 4.1 重复执行 priority harness，确认不再出现 `ScreenShareVsGrid` flake
  Verification: repeated `node tests/qos_harness/browser_downlink_priority.mjs`

## 5. Validate Adjacent Behavior

- [x] 5.1 校验 downlink malformed / inconsistent subscription 用例
  Verification: targeted `mediasoup_qos_integration_tests` filter

## 6. Delivery Gates

- [x] 6.1 运行必要的 targeted 构建/测试入口
  Verification: 记录命令结果

- [x] 6.2 Review `DELIVERY_CHECKLIST.md`
  Verification: applicable items resolved

## 7. Review

- [x] 7.1 基于 `REVIEW.md` 做 whole-system self-review
  Verification: 记录 root cause、边界与回归风险

## 8. Knowledge Update

- [x] 8.1 将稳定 accepted behavior 合并进 `specs/current/`
  Verification: 新 spec 与最终实现一致
