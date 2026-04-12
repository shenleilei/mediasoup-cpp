# 上行 QoS 逐 Case 预期与实测分析

日期：`2026-04-12`

## 1. 文档目的

这份文档补足 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md) 和 [uplink-qos-test-results-summary.md](/root/mediasoup-cpp/docs/uplink-qos-test-results-summary.md) 的细节缺口，按当前仓库里保留的 targeted rerun artifact，逐条给出：

- case 预期
- case 实测
- PASS/FAIL 的判定依据
- 需要特别说明的口径差异

当前可复核的机器数据来自 [generated/uplink-qos-matrix-report.json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.json)，只覆盖最后一次 targeted rerun 的 `8` 个 case：

- `T4`
- `T6`
- `T7`
- `T8`
- `S1`
- `S2`
- `S3`
- `S4`

因此，这份文档是“逐 case 详细分析补充件”，不是全量 `41` 个 matrix case 的完整归档。

## 2. 判定口径

本轮 matrix 结果不是简单拿 phase 结束瞬时状态做判断，而是按下面口径判定：

- `transition / burst` 组的 impairment 结果取该 phase 的 `peak`，即 phase 内出现过的最强保护状态。
- recovery 结果取 recovery phase 的 `best`，即恢复窗口内达到过的最佳状态，而不是 recovery 结束瞬间。
- 若 `expect.recovery !== false`，则 recovery 的 `best` 必须同时满足：state 不高于 baseline、level 不高于 baseline。
- expectation 里的 `states` 是允许区间，不代表“必须达到列表里最差那个状态”。
- expectation 里的 `maxLevel` 是上限约束，不代表“必须升到该 level”。
- 若 `expect.recovery === false`，则不要求 recovery phase 回到 baseline。

这也是为什么部分 case 的 expectation 写了 `["early_warning", "congested"]`，但实际 peak 只到 `early_warning/L1` 仍然判定为 `PASS`。

## 3. 汇总表

| Case | 组别 | 预期 | baseline 实测 | impairment 实测 | recovery 实测 | 结论 |
|---|---|---|---|---|---|---|
| `T4` | `transition` | `states=[early_warning, congested]`，`maxLevel=4` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |
| `T6` | `transition` | `states=[early_warning, congested]`，`maxLevel=4` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |
| `T7` | `transition` | `states=[early_warning, congested]`，`maxLevel=4` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |
| `T8` | `transition` | `state=congested`，`maxLevel=4`，`recovery=false` | `early_warning/L1` | `peak=congested/L4` | `best=congested/L4` | `PASS` |
| `S1` | `burst` | `states=[early_warning, congested]`，`maxLevel=4` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |
| `S2` | `burst` | `state=congested`，`maxLevel=4` | `stable/L0` | `peak=congested/L4` | `best=stable/L0` | `PASS` |
| `S3` | `burst` | `state=early_warning`，`maxLevel=2` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |
| `S4` | `burst` | `state=early_warning`，`maxLevel=2` | `stable/L0` | `peak=early_warning/L1` | `best=stable/L0` | `PASS` |

## 4. 逐 Case 详细分析

### 4.1 `T4`

- 场景：
  baseline 为 `4000kbps / 25ms / 0.1% loss / 5ms jitter`，impairment 切换为 `4000kbps / 25ms / 5% loss / 5ms jitter`。
- 预期：
  impairment phase 至少触发 `early_warning`，允许进一步进入 `congested`，level 不得超过 `L4`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `6` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=5359ms`，未观测到 `congested`，recovery 后恢复到 `stable/L0`。
- 结果分析：
  该 case 的 expectation 是允许区间，不要求必须进入 `congested`。实际达到 `early_warning/L1`，处于允许区间内，恢复也回到 baseline，因此判定 `PASS`。

### 4.2 `T6`

- 场景：
  baseline 为 `4000kbps / 25ms / 0.1% loss / 5ms jitter`，impairment 切换为 `4000kbps / 180ms / 0.1% loss / 5ms jitter`。
- 预期：
  impairment phase 允许落在 `early_warning ~ congested` 区间，level 上限 `L4`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `2` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=3289ms`，未进入 `congested`；recovery 最佳状态回到 `stable/L0`。
- 结果分析：
  `180ms RTT` 在当前 loopback 模型下触发的是轻中度保护，没有跨到 `congested`。这仍在 expectation 允许范围内，因此判定 `PASS`。

### 4.3 `T7`

- 场景：
  baseline 为 `4000kbps / 25ms / 0.1% loss / 5ms jitter`，impairment 切换为 `4000kbps / 25ms / 0.1% loss / 40ms jitter`。
- 预期：
  impairment phase 允许落在 `early_warning ~ congested` 区间，level 上限 `L4`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `2` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=4299ms`，未进入 `congested`；recovery 最佳状态恢复到 `stable/L0`。
- 结果分析：
  `40ms jitter` 在当前模型下只触发一级保护。由于 expectation 允许 `early_warning`，且恢复正常，故判定 `PASS`。

### 4.4 `T8`

- 场景：
  baseline 即弱网：`2000kbps / 55ms / 0.5% loss / 12ms jitter`；
  impairment 进一步恶化为 `800kbps / 120ms / 3% loss / 30ms jitter`；
  expectation 明确 `recovery=false`。
- 预期：
  impairment 必须达到 `congested`，最高允许到 `L4`，且 recovery 不要求恢复。
- 实测：
  baseline `early_warning/L1`；
  impairment `peak=congested/L4`，`current=congested/L4`；
  recovery `best=congested/L4`，`current=congested/L4`。
- 动作：
  共 `38` 次非 `noop`，均为持续 `setEncodingParameters`。
- 时序：
  `t_detect_warning=103ms`；
  `t_detect_congested=2104ms`；
  `t_level_2=2104ms`；
  `t_level_3=2603ms`；
  `t_level_4=3103ms`。
- 结果分析：
  该 case 的设计目标就是验证“在更差基础网况上继续恶化时，应快速进入重保护且不要求恢复”。实测完全符合该目标，因此判定 `PASS`。

### 4.5 `S1`

- 场景：
  burst case，impairment 持续仅 `5s`，参数为 `4000kbps / 25ms / 10% loss / 5ms jitter`。
- 预期：
  burst 内允许触发 `early_warning ~ congested`，level 不超过 `L4`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `2` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=2866ms`，未进入 `congested`；recovery 最佳状态回到 `stable/L0`。
- 结果分析：
  由于 burst 时长较短，系统只来得及进入一级保护，没有进一步打到 `congested`。这在 expectation 允许区间内，因此判定 `PASS`。

### 4.6 `S2`

- 场景：
  burst case，impairment 持续 `5s`，参数为 `300kbps / 25ms / 0.1% loss / 5ms jitter`。
- 预期：
  burst phase 必须达到 `congested`，level 允许打到 `L4`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=congested/L4`，`current=congested/L4`；
  recovery `best=stable/L0`，`current=congested/L4`。
- 动作：
  共 `99` 次非 `noop`，为高频 `setEncodingParameters`。
- 时序：
  `t_detect_warning=2327ms`；
  `t_detect_congested=2827ms`；
  `t_level_1=2327ms`；
  `t_level_2=2827ms`；
  `t_level_3=3327ms`；
  `t_level_4=3827ms`。
- 结果分析：
  该 case 的核心验收点是 burst 期间是否打到 `congested/L4`。实测已满足；
  recovery phase 虽然结束瞬间仍停留在 `congested/L4`，但 recovery window 内的 `best` 已回到 `stable/L0`，因此按当前 recovery 判定口径仍为 `PASS`。

### 4.7 `S3`

- 场景：
  burst case，impairment 持续 `5s`，参数为 `4000kbps / 200ms / 0.1% loss / 5ms jitter`。
- 预期：
  impairment 目标为 `early_warning`，level 不超过 `L2`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `2` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=2279ms`，未进入 `congested`。
- 结果分析：
  `200ms RTT` 的短时 burst 在当前模型下触发一级保护，未超过 expectation 上限 `L2`，且恢复正常，因此判定 `PASS`。

### 4.8 `S4`

- 场景：
  burst case，impairment 持续 `5s`，参数为 `4000kbps / 25ms / 0.1% loss / 60ms jitter`。
- 预期：
  impairment 目标为 `early_warning`，level 不超过 `L2`。
- 实测：
  baseline `stable/L0`；
  impairment `peak=early_warning/L1`，`current=early_warning/L1`；
  recovery `best=stable/L0`，`current=stable/L0`。
- 动作：
  共 `2` 次非 `noop`，均为 `setEncodingParameters`。
- 时序：
  `t_detect_warning=2853ms`，未进入 `congested`。
- 结果分析：
  `60ms jitter` 的短时 burst 触发一级保护但未超过 `L2`，与 expectation 一致，因此判定 `PASS`。

## 5. 结论

基于当前保留的 targeted rerun 机器数据，可以明确给出下面的 case-level 结论：

- 当前可复核的 `8` 个 case 全部存在“预期定义”和“实际观测”之间的一一对应关系。
- 这些 `PASS` 不是“只看最终结果蒙混过关”，而是按既定判定口径逐 case 得出的。
- 容易产生误读的点主要有两个：
  - `states=[early_warning, congested]` 表示允许区间，不表示必须打到 `congested`。
  - recovery 看 `best`，不是看 recovery phase 结束瞬时状态。

如果后续需要补全 `41` 个 matrix case 的同类详细分析，需要先重新执行 full matrix，并保留不被覆盖的 per-case artifact。
