# Bugfix Design

## Context

当前 uplink browser matrix 的主阻断点不是 expectation 解析错误，而是：

- `S4` 在当前仓库里已经从历史上的 `early_warning/L1` 回退到可复现的 `congested/L4`
- `R1` 只在 full run 中出现一次 baseline 异常，单跑暂未复现

因此本次修复应优先恢复 `S4` 的 loopback runner 行为，而不是先扩大 expectation。

## Diagnosis

现有证据：

1. 历史全通过快照中：
   - `R1 = stable/L0`
   - `S4 = early_warning/L1`
2. 当前 full run 中：
   - `R1 baseline = early_warning/L1`
   - `S4 impairment peak = congested/L4`
3. 当前 targeted rerun 中：
   - `R1` 通过
   - `S4` 仍失败

这说明：

- `R1` 目前不是稳定回退，先不把主修复压到它上面
- `S4` 是当前稳定回退，必须先恢复

## Fix Strategy

### 1. 收敛到最小 runner 修复

优先检查并恢复 `loopback_runner.mjs` 中近期对 `tc/netem` 拓扑的改动，目标是：

- 保持 crash diagnostics 增强
- 只修正会改变 loopback 网络形态的部分
- 不改 matrix case 的 accepted expectation

已确认是 `prio + UDP filter` 拓扑放大了 `S4` 的 burst 瞬态，因此回退到历史稳定的 root `netem` 结构。

### 2. 保持 `R1` 作为观察项，不在本次盲目改 expectation

`R1` 单跑已通过，当前不应直接把 mild RTT expectation 放宽到 `early_warning`。

若 runner 修复后 `R1,S4` 组合 targeted 一并恢复，则把 `R1` 记录为 full-run baseline transient；
若 `R1` 仍异常，再单独处理 baseline warmup / gating 逻辑。

## Risks

- 回退 runner 的 `tc/netem` 拓扑可能影响此前为 diagnostics 或 isolation 引入的假设
- 若 `S4` 的回退并非来自 runner，而是来自其他未发现的 uplink 行为漂移，单纯回退 netem 拓扑可能不足
- `R1` 目前只在 full run 中失败，本次 targeted 验证可能无法证明其完全消除

## Verification Strategy

- `node tests/qos_harness/run_matrix.mjs --cases=S4`
- `node tests/qos_harness/run_matrix.mjs --cases=R1,S4`

若修复后 targeted 恢复，再补充说明：

- `R1` 当前未在 targeted 中复现
- 仍建议后续在 full matrix 中继续观察 baseline transient
