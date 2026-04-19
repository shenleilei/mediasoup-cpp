# 下行 QoS 逐 Case 最终结果

生成时间：`2026-04-14T03:31:35.053Z`

## 1. 汇总

- 总 Case：`8`
- 已执行：`6`
- 通过：`6`
- 失败：`0`
- 错误：`2`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [D3](#d3) | `ERROR` | ws connect timeout |
| [D6](#d6) | `ERROR` | ws connect timeout |

## 2. 快速跳转

- 失败 / 错误：[D3](#d3)、[D6](#d6)
- baseline：[D1](#d1)
- bw_sweep：[D2](#d2)
- loss_sweep：[D3](#d3)
- rtt_sweep：[D4](#d4)
- jitter_sweep：[D5](#d5)
- transition：[D6](#d6)
- competition：[D7](#d7)
- zero_demand：[D8](#d8)

## 3. 逐 Case 结果

### D1

| 字段 | 内容 |
|---|---|
| Case ID | `D1` |
| 类型 | `baseline` / priority `P0` |
| 说明 | good network, all consumers visible |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 10000ms |
| 预期 | consumerPaused=false；preferredSpatialLayer=2 |
| 实际结果 | PASS（ok） |

### D2

| 字段 | 内容 |
|---|---|
| Case ID | `D2` |
| 类型 | `bw_sweep` / priority `P0` |
| 说明 | bandwidth drops to 300kbps, expect layer downgrade |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 300kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤0；recovers after impairment |
| 实际结果 | PASS（ok） |

### D3

| 字段 | 内容 |
|---|---|
| Case ID | `D3` |
| 类型 | `loss_sweep` / priority `P0` |
| 说明 | 15% packet loss, expect health degrade |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 15% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤1；recovers after impairment |
| 实际结果 | ERROR：ws connect timeout |

### D4

| 字段 | 内容 |
|---|---|
| Case ID | `D4` |
| 类型 | `rtt_sweep` / priority `P0` |
| 说明 | RTT 500ms, expect layer reduction |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 500ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤1；recovers after impairment |
| 实际结果 | PASS（ok） |

### D5

| 字段 | 内容 |
|---|---|
| Case ID | `D5` |
| 类型 | `jitter_sweep` / priority `P0` |
| 说明 | jitter 80ms, expect layer reduction |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 80ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | consumerPaused=false；maxSpatialLayer≤1；recovers after impairment |
| 实际结果 | PASS（ok） |

### D6

| 字段 | 内容 |
|---|---|
| Case ID | `D6` |
| 类型 | `transition` / priority `P0` |
| 说明 | good -> bad -> good, verify recovery without oscillation |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 200kbps / RTT 200ms / loss 10% / jitter 40ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 20000ms |
| 预期 | consumerPaused=false；recovers after impairment |
| 实际结果 | ERROR：ws connect timeout |

### D7

| 字段 | 内容 |
|---|---|
| Case ID | `D7` |
| 类型 | `competition` / priority `P1` |
| 说明 | two subscribers, high-priority gets better layer under constrained bw |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 500kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 15000ms / recovery 15000ms |
| 预期 | highPriority gets better layer；recovers after impairment |
| 实际结果 | PASS（ok） |

### D8

| 字段 | 内容 |
|---|---|
| Case ID | `D8` |
| 类型 | `zero_demand` / priority `P0` |
| 说明 | all consumers hidden, expect pauseUpstream after kPauseConfirmMs |
| baseline 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| impairment 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| recovery 网络 | 5000kbps / RTT 30ms / loss 0% / jitter 2ms |
| 持续时间 | baseline 10000ms / impairment 10000ms / recovery 15000ms |
| 预期 | pauseUpstream=true；resumeUpstream=true；recovers after impairment |
| 实际结果 | PASS（ok） |

