# TWCC A/B 有效性评估结果

生成时间：`<ISO8601>`

## 1. 实验信息

| 字段 | 内容 |
|---|---|
| Baseline 版本 | `<git-commit-or-tag>` |
| Candidate 版本 | `<git-commit-or-tag>` |
| 机器/内核 | `<cpu / mem / kernel / distro>` |
| 测试网卡与限速方式 | `<ifb/tc/netem or others>` |
| 输入素材 | `<file / codec / resolution / fps>` |
| 每场景重复次数 | `<N>` |
| 结果归档目录 | `<docs/archive/...>` |

## 2. 汇总

| 指标 | Baseline | Candidate | 变化 |
|---|---:|---:|---:|
| 总体 Goodput (kbps) | `<v>` | `<v>` | `<%>` |
| 总体 Loss (%) | `<v>` | `<v>` | `<%>` |
| 拥塞阶段恢复前排队压力（规范化） | `<v>` | `<v>` | `<%>` |
| 升档恢复时间 (ms) | `<v>` | `<v>` | `<%>` |
| 稳态抖动幅度 (P90-P50, kbps) | `<v>` | `<v>` | `<%>` |
| 多路权重偏差（越小越好） | `<v>` | `<v>` | `<%>` |

结论：`PASS/FAIL`

## 3. 判定门槛检查

| 规则 | 目标 | 实际 | 结论 |
|---|---|---|---|
| AB-002 拥塞阶段 Loss + 队列压力改善 | 至少 20% | `<actual>` | `PASS/FAIL` |
| AB-003 升档恢复时间 | 不劣化，目标改善 15%+ | `<actual>` | `PASS/FAIL` |
| AB-001 稳态 Goodput 退化 | 不超过 5% | `<actual>` | `PASS/FAIL` |
| AB-005 多路权重偏差 | 不放大 | `<actual>` | `PASS/FAIL` |
| 回归约束（Pause/Shutdown/Generation） | 全绿 | `<actual>` | `PASS/FAIL` |

## 4. 场景级结果

| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |
|---|---|---|---|---|---|
| AB-001 稳态 | `<bw/rtt/loss/jitter>` | `<summary>` | `<summary>` | `<%>` | `PASS/FAIL` |
| AB-002 阶跃降带宽 | `<from->to + loss>` | `<summary>` | `<summary>` | `<%>` | `PASS/FAIL` |
| AB-003 阶跃升带宽 | `<from->to>` | `<summary>` | `<summary>` | `<%>` | `PASS/FAIL` |
| AB-004 高 RTT/Jitter | `<rtt/jitter>` | `<summary>` | `<summary>` | `<%>` | `PASS/FAIL` |
| AB-005 多路权重 | `<tracks/weights>` | `<summary>` | `<summary>` | `<%>` | `PASS/FAIL` |

## 5. 逐场景明细（示例结构）

### AB-002

| 字段 | 内容 |
|---|---|
| 场景描述 | `<step-down with loss>` |
| 运行次数 | `<N>` |
| Baseline Goodput (mean / P50 / P90) | `<...>` |
| Candidate Goodput (mean / P50 / P90) | `<...>` |
| Baseline Loss (mean / P50 / P90) | `<...>` |
| Candidate Loss (mean / P50 / P90) | `<...>` |
| Baseline 恢复时间 (mean / P50 / P90) | `<...>` |
| Candidate 恢复时间 (mean / P50 / P90) | `<...>` |
| Baseline 队列压力（wouldBlock/retention/drop） | `<...>` |
| Candidate 队列压力（wouldBlock/retention/drop） | `<...>` |
| 关键差异分析 | `<text>` |
| 结论 | `PASS/FAIL` |

## 6. 白盒诊断（可选）

| 指标 | Baseline | Candidate | 说明 |
|---|---:|---:|---|
| `transportEstimatedBitrateBps` 轨迹 | `<path-or-summary>` | `<path-or-summary>` | 估算是否过冲/滞后 |
| `effectivePacingBitrateBps` 轨迹 | `<...>` | `<...>` | pacing 上限收敛 |
| `transportCcFeedbackReports` | `<...>` | `<...>` | feedback 到达率 |
| `wouldBlockByClass` | `<...>` | `<...>` | 回压位置 |
| `queuedVideoRetentions` | `<...>` | `<...>` | 队列滞留 |
| `audioDeadlineDrops` | `<...>` | `<...>` | 音频保障 |
| `retransmissionDrops` | `<...>` | `<...>` | 重传尾损 |

## 7. 回归与风险

| 项目 | 结果 | 备注 |
|---|---|---|
| Pause 后视频泄漏 | `PASS/FAIL` | `<test ref>` |
| Shutdown 丢尾 | `PASS/FAIL` | `<test ref>` |
| Generation 切换旧重传泄漏 | `PASS/FAIL` | `<test ref>` |
| 健康检查阻塞风险 | `PASS/FAIL` | `<test ref>` |

残余风险：

- `<risk-1>`
- `<risk-2>`

## 8. 产物链接

- 原始指标 JSON：`<path>`
- 聚合对比 JSON：`<path>`
- Case 报告：`<path>`
- 日志与 trace：`<path>`
