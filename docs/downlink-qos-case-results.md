# 下行 QoS 逐项最终结果

生成时间：`2026-04-14T13:00:27.000Z`

## 1. 汇总

- 总任务：`8`
- 已执行：`8`
- 通过：`8`
- 失败：`0`
- 未执行：`0`
- 执行脚本：`scripts/run_qos_tests.sh`
- 本次选择目标：`client-js`, `cpp-unit`, `cpp-integration`, `cpp-accuracy`, `cpp-recording`, `node-harness`, `browser-harness`, `matrix`, `downlink-matrix`

### 1.1 失败任务

- 无

## 2. 快速跳转

- 失败任务：无
- server：[cpp-unit](#cpp-unit)、[cpp-integration](#cpp-integration)
- browser：[browser-harness:downlink-controls](#browser-harness-downlink-controls)、[browser-harness:downlink-e2e](#browser-harness-downlink-e2e)、[browser-harness:downlink-priority](#browser-harness-downlink-priority)、[browser-harness:downlink-v2](#browser-harness-downlink-v2)、[browser-harness:downlink-v3](#browser-harness-downlink-v3)、[downlink-matrix](#downlink-matrix)

## 3. 逐项结果

### cpp-unit

| 字段 | 内容 |
|---|---|
| 任务 ID | `cpp-unit` |
| 类别 | `server` |
| 说明 | 服务端 downlink QoS 相关单测（allocator / planner / aggregator / publisher supply） |
| 状态 | `PASS` |
| 耗时 | `0s` |
| 对应命令 | `./build/mediasoup_tests --gtest_filter=*Downlink*:*ProducerDemand*:*PublisherSupply*:*SubscriberBudgetAllocator*` |

### cpp-integration

| 字段 | 内容 |
|---|---|
| 任务 ID | `cpp-integration` |
| 类别 | `server` |
| 说明 | 服务端 downlink QoS 集成测试（consumer state、publisher clamp、stale snapshot 回归） |
| 状态 | `PASS` |
| 耗时 | `85s` |
| 对应命令 | `./build/mediasoup_qos_integration_tests --gtest_filter=QosIntegrationTest.Downlink*` |

### browser-harness:downlink-controls

| 字段 | 内容 |
|---|---|
| 任务 ID | `browser-harness:downlink-controls` |
| 类别 | `browser` |
| 说明 | 浏览器信令控制验证：pause / resume / requestKeyFrame 基本控制链路 |
| 状态 | `PASS` |
| 耗时 | `4s` |
| 对应命令 | `node tests/qos_harness/browser_downlink_controls.mjs` |

### browser-harness:downlink-e2e

| 字段 | 内容 |
|---|---|
| 任务 ID | `browser-harness:downlink-e2e` |
| 类别 | `browser` |
| 说明 | 浏览器端到端验证：downlinkClientStats -> consumer pause/resume / priority |
| 状态 | `PASS` |
| 耗时 | `4s` |
| 对应命令 | `node tests/qos_harness/browser_downlink_e2e.mjs` |

### browser-harness:downlink-priority

| 字段 | 内容 |
|---|---|
| 任务 ID | `browser-harness:downlink-priority` |
| 类别 | `browser` |
| 说明 | 浏览器弱网竞争验证：高优先级 subscriber 分配优于低优先级 |
| 状态 | `PASS` |
| 耗时 | `41s` |
| 对应命令 | `node tests/qos_harness/browser_downlink_priority.mjs` |

### browser-harness:downlink-v2

| 字段 | 内容 |
|---|---|
| 任务 ID | `browser-harness:downlink-v2` |
| 类别 | `browser` |
| 说明 | 浏览器 v2 验证：subscriber demand -> track-scoped publisher clamp / clear / zero-demand hold |
| 状态 | `PASS` |
| 耗时 | `4s` |
| 对应命令 | `node tests/qos_harness/browser_downlink_v2.mjs` |

### browser-harness:downlink-v3

| 字段 | 内容 |
|---|---|
| 任务 ID | `browser-harness:downlink-v3` |
| 类别 | `browser` |
| 说明 | 浏览器 v3 验证：sustained zero-demand -> pauseUpstream / resumeUpstream / flicker 防抖 |
| 状态 | `PASS` |
| 耗时 | `16s` |
| 对应命令 | `node tests/qos_harness/browser_downlink_v3.mjs` |

### downlink-matrix

| 字段 | 内容 |
|---|---|
| 任务 ID | `downlink-matrix` |
| 类别 | `browser` |
| 说明 | downlink 弱网矩阵：baseline / bw / loss / rtt / jitter / transition / competition / zero-demand |
| 状态 | `PASS` |
| 耗时 | `336s` |
| 对应命令 | `node tests/qos_harness/run_downlink_matrix.mjs` |

