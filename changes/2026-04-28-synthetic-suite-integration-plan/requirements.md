# Requirements

## Summary

在当前阶段继续保留并改良 QoS synthetic 回归测试体系，但必须将其明确限定为 `synthetic-only regression suite`，并把 `copilot/analyze-network-condition-processing` 分支中有价值的 synthetic 校准成果，按当前主线约束重新集成，而不是直接合并分支。

## Problem Statement

当前 synthetic 方向有两个相互独立的问题：

1. **方向问题已经澄清**  
   团队已接受 synthetic suite 继续存在，用于稳定的 QoS 状态机/控制器回归，而非现实物理网络签收。

2. **集成方式仍然错误**  
   远程分支虽然补强了 synthetic 模型、文档和测试，但它相对当前主线仍然会：
   - 回退已修复的 harness/runtime/build 修复
   - 带入不同的 build/test hook 约定
   - 混入与 synthetic 校准无关的分支漂移与第三方指针变化

因此，需要一个“集成方案”，定义：

- synthetic 校准内容中哪些值得保留
- 哪些 legacy forcing 允许暂时保留
- 哪些主线修复绝不能回退
- 应按什么顺序在当前主线上重做

## Business Goal

建立一条可执行的集成路线，使团队能够：

- 保留 synthetic suite 的回归稳定性收益
- 不再把 synthetic suite 误当成物理网络验证
- 在当前主线基础上吸收有价值的校准工作
- 避免回退当前已修复的 build/harness/runtime 问题

## In Scope

- synthetic suite 的职责边界与命名口径
- `copilot/analyze-network-condition-processing` 分支可采纳内容的筛选
- synthetic 模型校准（loss / RTT / jitter / utilization）的集成方式
- `applyMatrixTestProfile()` convergence 扩展的集成方式
- JS / C++ synthetic pipeline 测试的集成方式
- 当前主线修复的保留约束
- retained legacy overrides 的过渡策略
- 集成步骤与提交拆分建议

## Out Of Scope

- 直接合并远程分支
- 本 change 中直接实现 physical-E2E suite
- 本 change 中直接重写全部 browser/cpp matrix 入口
- 本 change 中直接删掉所有 synthetic legacy forcing

## Requirements

### Requirement 1: Synthetic-Only Boundary

系统 SHALL 明确 synthetic suite 的职责仅为：

- QoS 状态机 / controller / planner / executor 稳定回归
- synthetic 输入下的行为一致性验证

系统 SHALL NOT 将 synthetic suite 的结果作为现实网络条件的直接解释依据。

### Requirement 2: Current-Branch-First Integration

synthetic 校准工作 SHALL 以当前主线分支为基线重做或摘取。

#### Scenario: Remote branch contains useful calibration work

- WHEN 远程分支包含可用的 synthetic 模型改进
- THEN 这些改动 SHALL 以 cherry-pick / reimplementation 方式集成到当前主线
- AND SHALL NOT 通过直接 merge 整个远程分支来进入主线

### Requirement 3: No Regression Of Current Fixes

synthetic 改良集成 SHALL NOT 回退当前主线已修复的问题。

#### Must preserve

- `cpp_client_runner` 的 stderr/spdlog `QOS_TRACE` 解析
- `netem guard` preflight / stale sweep / fast-fail
- report verdict 一致性修复
- plain-client build wiring / test hook 主线约定
- review follow-up runtime hardening

### Requirement 4: Test Hook Safety

所有 `QOS_TEST_*` synthetic 输入注入 SHALL 继续受 `MEDIASOUP_TEST_HOOKS` 保护。

系统 SHALL NOT 重新引入生产构建可触发的 synthetic 注入路径。

### Requirement 5: Synthetic Calibration Scope

允许集成的 synthetic 改良包括：

- loss utilization 校准
- RTT amplification 校准
- jitter smoothing
- phase convergence simulation
- synthetic pipeline / controller 测试
- synthetic representativeness 文档

这些内容必须显式标注为 synthetic suite 改良，而不是物理网络修复。

### Requirement 6: Legacy Override Policy

retained legacy overrides 如果暂时保留，必须满足：

- 有文档解释其为何保留
- 有测试证明其输出仍在 synthetic 模型可接受区间内
- 被标明为过渡性 synthetic forcing，而不是现实网络等价值

### Requirement 7: Incremental Landing

集成 SHALL 以小步、可审阅的提交顺序落地。

建议至少拆分为：

1. synthetic scope/labeling & docs
2. core synthetic model recalibration
3. C++ convergence implementation
4. JS/C++ synthetic tests

## Acceptance Criteria

- 综合方案明确列出：保留、丢弃、重做三类内容
- 方案明确说明为什么不能直接 merge 远程分支
- 方案明确当前主线必须保留的修复清单
- 方案明确 synthetic suite 与未来 physical suite 的边界
