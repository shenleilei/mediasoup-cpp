# Design

## Context

`copilot/analyze-network-condition-processing` 分支并没有朝“真实网络 -> 真实输入”方向前进，而是在继续强化 synthetic suite：

- synthetic_sweep_shared 的经验校准
- applyMatrixTestProfile 的时间收敛模型
- JS/C++ synthetic pipeline tests
- synthetic representativeness 文档

在团队已经接受“先保留 synthetic 方法做稳定回归”的前提下，这些内容不是没有价值；问题在于**集成方法**：

- 分支整体落后于当前主线
- 直接 merge 会回退当前已修复问题
- 部分 build/test hook 约定与当前主线不同
- synthetic 改良和无关分支漂移混在一起

因此，本方案不是讨论“要不要 synthetic”，而是定义：

> 如何在当前主线基础上，吸收 synthetic 校准成果，同时不破坏当前已修复的内容。

## Adoption Decision

### Keep

以下内容值得保留并迁入当前主线：

1. **JS synthetic model recalibration**
   - `normalizeLossStress(lossPct / 6)`
   - RTT logarithmic amplification
   - jitter smoothing
   - updated utilization weights/caps

2. **C++ convergence enhancement**
   - `exponentialConverge`
   - sendCeiling / RTT / jitter / lossRate convergence

3. **Synthetic validation assets**
   - expanded `test.synthetic_sweep.mjs`
   - `ExponentialConverge.*`
   - `SyntheticPipeline.*`
   - `SyntheticRunnerPipeline.*`

4. **Synthetic documentation**
   - synthetic representativeness analysis
   - synthetic-only scope boundary

### Keep Temporarily With Explicit Legacy Status

以下内容可以暂时保留，但必须被明示为 legacy forcing：

1. `bw<=1000 -> sendCeilingBps *= 0.75`
2. burst `qualityLimitationReason='bandwidth'`
3. jitter floor override

保留理由：

- 它们是当前 synthetic suite 稳定性的一部分
- 当前分支已开始为它们补兼容性测试
- 立即删除会造成 synthetic suite 结果剧烈漂移

但这些内容必须：

- 在文档中标成 retained legacy overrides
- 不得被解释为物理网络语义

### Drop / Must Not Adopt

以下内容不能跟分支整体一起进入当前主线：

1. **Direct branch merge**
   - 会回退当前分支已修复的多项问题

2. **Alternative build/test hook contract**
   - 不引入与当前主线冲突的 client build 启用方式
   - 继续复用当前主线的 `MEDIASOUP_TEST_HOOKS` 保护策略

3. **Unrelated third-party / gitlink drift**
   - `src/mediasoup-worker-src`
   - `third_party/*` 指针变化

4. **Any rollback of current harness fixes**
   - stdout-only trace parsing
   - old netem guard behavior
   - old report verdict handling

## Integration Strategy

### Step 1: Scope Boundary First

在代码改动前，先把 synthetic suite 的身份讲清楚。

Deliverables:

- 新增/更新文档，明确：
  - synthetic suite 只用于 regression
  - 不用于现实网络签收
- 在 runner/report 中添加 legacy / synthetic 标签（如适用）

Why first:

- 防止后续校准工作再次被误解为 physical-E2E 修复

### Step 2: Core Synthetic Model Recalibration

在当前主线代码上吸收以下变化：

- `synthetic_sweep_shared.mjs`
  - loss normalization/weight
  - RTT logarithmic model
  - jitter smoothing
  - calibrated utilization caps

Guardrails:

- 不动 current physical-E2E methodology docs
- 所有变更都以 synthetic-only 为前提描述

### Step 3: C++ Convergence Implementation

吸收：

- `PlainClientSupport.h`
  - `exponentialConverge`
  - convergence state
- `PlainClientSupport.cpp`
  - convergence for sendCeiling / RTT / jitter / lossRate

Constraints:

- 保留当前主线的 `MEDIASOUP_TEST_HOOKS` 访问保护方式
- 不回退其它主线运行时修复

### Step 4: Validation Integration

吸收：

- JS synthetic validation tests
- C++ formula tests
- C++ pipeline/controller tests

But:

- 这些 tests 只作为 synthetic suite 可信度增强
- 不将它们描述为物理网络验证

### Step 5: Legacy Overrides Review Marker

在 design/docs 中为 retained overrides 加统一标记：

- `retained_legacy_synthetic_override`

后续如要逐步移除，可单独开 change 处理。

## Integration Constraints

### Constraint 1: Current Fixes Are Source Of Truth

当前主线以下内容必须保留：

- stderr/spdlog trace parsing
- netem guard preflight
- report verdict consistency
- build/test hook safety
- runtime hardening

### Constraint 2: Synthetic Inputs Stay Test-Only

`QOS_TEST_*` 输入必须继续在 test hook 宏保护下。

### Constraint 3: No Mixed Messaging

任何 synthetic 校准文档都必须避免使用：

- “真实网络修复”
- “更准确反映现实网络”

更准确表述应是：

- “提高 synthetic regression 与经验数据的一致性”
- “使 synthetic suite 更可信”

## Suggested Landing Sequence

1. `docs: clarify synthetic-only scope`
2. `test: recalibrate synthetic JS model`
3. `test: add C++ convergence support to synthetic profile`
4. `test: add JS/C++ synthetic validation coverage`

This sequence keeps reviewable scope small and avoids mixing methodology, runtime,
and generated-artifact churn in one patch.

## Risks

### Risk 1: Synthetic suite still mistaken for physical truth

Mitigation:

- explicit scope boundary docs
- legacy/synthetic labels

### Risk 2: Current mainline fixes accidentally regress

Mitigation:

- no direct branch merge
- current-branch-first reimplementation/cherry-pick

### Risk 3: Retained overrides continue to hide model weaknesses

Mitigation:

- explicit retained-legacy labeling
- compatibility tests
- follow-up removal plan

## Decision Summary

1. Keep the synthetic calibration direction.
2. Do not merge the branch directly.
3. Re-integrate useful changes onto current `HEAD`.
4. Preserve all current harness/runtime/build fixes.
5. Treat retained overrides as documented legacy synthetic forcing, not as calibrated physical truth.
