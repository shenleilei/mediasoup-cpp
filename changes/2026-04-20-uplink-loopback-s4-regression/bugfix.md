# Bugfix Analysis

## Summary

`scripts/run_all_tests.sh` 触发的 full regression 中，uplink browser `matrix` 当前出现两个问题：

- `S4` 在 loopback runner 下稳定回退为 `congested/L4`
- `R1` 在 full run 中偶发以 `baseline=early_warning/L1` 开始，导致 mild RTT case 被误判失败

其中：

- `S4` 已用 targeted rerun 复现，属于当前主问题
- `R1` 单跑通过，暂时只作为 full-run baseline 瞬态记录

## Reproduction

1. 查看 `/var/log/run_all_tests.log`
2. 运行 `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`

## Observed Behavior

- full run 中：
  - `R1` 失败时 `baseline(current=early_warning/L1)`，impairment peak 仍为 `early_warning/L1`
  - `S4` 失败时 `impairment(peak=congested/L4)`，超出 `expectByRunner.loopback.maxLevel=2`
- targeted rerun 中：
  - `R1` 恢复为 `PASS`
  - `S4` 仍稳定 `FAIL`

## Expected Behavior

- `S4` 在 loopback runner 下应回到已接受的历史窗口，不应系统性冲到 `congested/L4`
- `R1` 这类 mild RTT case 不应因为 baseline 建立阶段的瞬态而在 full run 中随机变红

## Known Scope

- `tests/qos_harness/loopback_runner.mjs`
- `tests/qos_harness/run_matrix.mjs`
- 可能涉及 `docs/` 中的 uplink matrix 结果说明

## Must Not Regress

- `S4` 的 loopback expectation 不应被无依据放宽到掩盖真实 runner 回退
- `R1-R6` 与 `S1-S4` 的其余 matrix case 不应因修复 runner 而整体漂移
- 现有 crash diagnostics / report artifact 结构保持不变

## Suspected Root Cause

- `S4` 的直接回退来自 `loopback_runner.mjs` 中近期把 root `netem` 改成 `prio + UDP-only netem` 拓扑；这个改动放大了 loopback burst 瞬态，使 `S4` 从历史上的 `early_warning/L1` 冲到 `congested/L4`
- `R1` 更像是 full run 过程中的 baseline warmup / 资源瞬态，而非稳定功能回退

## Acceptance Criteria

### Requirement 1

`S4` SHALL 在 loopback runner 下重新落回当前 accepted 历史窗口。

#### Scenario: burst jitter loopback case remains mild-to-medium

- WHEN 运行 `node tests/qos_harness/run_matrix.mjs --cases=S4`
- THEN `S4` 应通过
- AND impairment peak 不得稳定超过当前 `expectByRunner.loopback.maxLevel`

### Requirement 2

`R1` 的当前处置 SHALL 以避免误判为前提，但不得把真实 mild RTT 回退静默吞掉。

#### Scenario: mild RTT case does not fail from startup baseline transient

- WHEN 运行 `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`
- THEN `R1` 应保持通过
- AND 若未直接修复 full-run baseline 瞬态，变更说明中必须记录剩余风险与后续观察点

## Regression Expectations

- Required automated regression coverage:
  - `node tests/qos_harness/run_matrix.mjs --cases=S4`
  - `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`
- Required manual/full-run follow-up:
  - 如 targeted 结果恢复，再评估是否需要重新执行 `scripts/run_qos_tests.sh matrix --matrix-cases=R1,S4`
