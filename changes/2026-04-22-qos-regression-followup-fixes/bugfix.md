# Bugfix Analysis

## Summary

`/var/log/run_all_tests.log` 对应的 full regression 当前还剩 3 个独立失败簇：

- `mediasoup_qos_integration_tests` 中
  `QosIntegrationTest.DownlinkClientStatsAcceptsSeqResetFromHighValue`
- `cpp-client-harness` 中
  `multi_video_budget` 与 `threaded_quick`
- uplink browser `matrix` 中
  `BW3`、`T9`、`S4`

这 3 簇的性质不同，不应混成一次“把 tests 调绿”的无差别修补：

- downlink gtest 更像测试语义自相矛盾
- cpp harness 更像时序脆弱与过早取样
- matrix 是真实行为回归，至少 `T9` / `S4` 与既有 targeted pass 记录不一致

## Reproduction

1. 查看 `/var/log/run_all_tests.log`
2. 运行：
   - `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.DownlinkClientStatsAcceptsSeqResetFromHighValue:QosIntegrationTest.DownlinkClientStatsRejectsRegressedTs:QosIntegrationTest.DownlinkClientStatsRejectsStaleSeq`
   - `node tests/qos_harness/run_cpp_client_harness.mjs multi_video_budget`
   - `node tests/qos_harness/run_cpp_client_harness.mjs threaded_multi_track_snapshot`
   - `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`

## Observed Behavior

### 1. Downlink seq-reset gtest

- `DownlinkClientStatsAcceptsSeqResetFromHighValue` 当前稳定失败
- 测试名写的是 “Accepts”
- 但断言却要求第二次请求 `stored=false` 且 `reason=stale-ts`
- 同一个测试最后又要求服务端存储的 `seq == 1`

这三条断言彼此冲突，且与同文件 publisher `clientStats` 的 `gap > 1000` reset 规则不一致。

### 2. Cpp harness

- `multi_video_budget` 在 full run 中曾取到三路都为 `450000` 的最后样本而失败
- 单独重跑时该 case 可通过
- `threaded_quick` 的失败落在 `runThreadedMultiTrackSnapshotScenario()`
- 该场景只等待 “`clientStats.tracks` 是数组”，未等待数组长度达到预期 2 条 track
- 实际 threaded path 可先出现单 track 快照，后续才收敛到完整多 track 快照

### 3. Matrix

- `BW3`：判定为 `过弱`
- `T9`：判定为 `恢复过慢`
- `S4`：判定为 `过强`

其中：

- `T9` 当前最新报告显示 recovery window 内始终停在 `congested/L4`
- `S4` 当前最新报告显示 impairment peak 到 `congested/L4`
- `docs/qos/uplink/blind-spot-scenario.md` 里已有包含 `T9` 与 `S4` 的组合 targeted pass 记录

## Expected Behavior

- downlink seq-reset gtest 应与当前 accepted registry 语义一致：
  - 大幅回退 seq reset 在 `tsMs` 未回退时可被接受
  - 真正的时间戳回退应返回 `stale-ts`
- cpp harness 应等待可判定的收敛窗口，而不是基于单个过早样本或不完整快照直接失败
- `BW3` / `T9` / `S4` 应恢复到当前 scenario catalog 与既有 accepted 文档定义的行为窗口，不应通过放宽 expectation 掩盖回归

## Known Scope

- `tests/test_qos_integration.cpp`
- `tests/qos_harness/run_cpp_client_harness.mjs`
- `tests/qos_harness/run_matrix.mjs`
- `tests/qos_harness/scenarios/sweep_cases.json`
- `tests/qos_harness/loopback_runner.mjs`
- 可能涉及 browser/client QoS recovery or sensitivity logic
- 可能涉及 `docs/` 与 `specs/current/`

## Must Not Regress

- downlink `stale-seq` / `stale-ts` registry 行为
- threaded plain-client 多 track publish 的现有实现路径
- `cpp-client-harness` 其他 threaded cases
- matrix 的 case catalog、报告结构与 diagnostics 输出
- 不得为了通过 `BW3/T9/S4` 直接无依据放宽 scenario expectation

## Suspected Root Cause

Confidence:

- high: downlink gtest 断言陈旧/自相矛盾
- high: cpp harness 等待条件过早、取样窗口过窄
- medium: matrix 回归可能来自 loopback runner 网络形态、browser QoS recovery fast-path、或近期 sender/control 变更的组合影响，需 targeted 复现后再收敛

## Acceptance Criteria

### Requirement 1

Downlink seq-reset integration coverage SHALL reflect the accepted registry contract.

#### Scenario: large seq reset without timestamp regression

- WHEN downlink snapshot seq 从高值大幅回退且 `tsMs` 仍递增
- THEN 请求应被接受或测试应明确验证 accepted 语义
- AND 测试不得同时断言 `stored=false` 与最终 `seq == 1`

#### Scenario: timestamp regression

- WHEN downlink snapshot `tsMs` 相对已存记录明显回退
- THEN 请求应返回 `stored=false`
- AND `reason` 应为 `stale-ts`

### Requirement 2

Cpp harness SHALL wait for stable evidence before judging multi-track regressions.

#### Scenario: multi-video budget ordering

- WHEN `multi_video_budget` 进入收敛阶段
- THEN 断言应基于窗口内可重复观测的 ordering
- AND 不得只基于单个尾样本失败

#### Scenario: threaded multi-track snapshot

- WHEN threaded client 首次开始上报 `clientStats`
- THEN harness 应等待完整 expected track set
- AND 不得把中间态单 track 快照当成最终失败

### Requirement 3

Matrix regressions `BW3` / `T9` / `S4` SHALL be repaired against current accepted behavior, not hidden by weaker expectations.

#### Scenario: targeted rerun

- WHEN 运行 `node tests/qos_harness/run_matrix.mjs --cases=BW3,T9,S4`
- THEN 每个 case 都应基于当前 accepted behavior 通过
- OR 若某个 case 仍失败，change docs 必须记录已验证的根因与未解决边界，而不是 silently broaden expectation

## Regression Expectations

- Required automated regression coverage:
  - targeted downlink gtests
  - targeted cpp harness cases
  - targeted matrix reruns for `BW3,T9,S4`
- Required adjacent-path checks:
  - `threaded_quick`
  - neighboring downlink stale-seq / stale-ts cases
  - any matrix cases directly referenced by existing docs when validating `T9` / `S4`
