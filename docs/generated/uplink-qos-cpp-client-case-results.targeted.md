# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-22T16:54:09.204Z`

## 1. 汇总

- 总 Case：`4`
- 已执行：`4`
- 通过：`4`
- 失败：`0`
- 错误：`0`
- runner：`cpp_client`

## 2. 快速跳转

- 失败 / 错误：无
- baseline：[B2](#b2)
- bw_sweep：[BW1](#bw1)
- rtt_sweep：[R4](#r4)
- jitter_sweep：[J3](#j3)

## 3. 逐 Case 结果

### B2

| 字段 | 内容 |
|---|---|
| Case ID | `B2` |
| 类型 | `baseline` / priority `P1` |
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| impairment 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| 预期 QoS | 期望状态=stable；maxLevel=0 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L1) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=4ms |
| recovery timing | t_detect_warning=23467ms；t_detect_stable=247ms；t_first_action=24470ms；t_level_1=24470ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=4ms |
| raw recovery timing | t_detect_warning=23467ms；t_detect_stable=247ms；t_first_action=24470ms；t_level_1=24470ms |

### BW1

| 字段 | 内容 |
|---|---|
| Case ID | `BW1` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 3000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=112ms |
| recovery timing | t_detect_stable=322ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=112ms |
| raw recovery timing | t_detect_stable=322ms |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 类型 | `rtt_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2986ms；t_detect_stable=961ms；t_first_action=3991ms；t_level_0=9053ms；t_level_1=3991ms |
| recovery timing | t_detect_stable=90ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2986ms；t_detect_stable=961ms；t_first_action=3991ms；t_level_0=9053ms；t_level_1=3991ms |
| raw recovery timing | t_detect_stable=90ms |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 类型 | `jitter_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2093ms；t_detect_stable=83ms；t_first_action=3127ms；t_level_0=8159ms；t_level_1=3127ms |
| recovery timing | t_detect_stable=229ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2093ms；t_detect_stable=83ms；t_first_action=3127ms；t_level_0=8159ms；t_level_1=3127ms |
| raw recovery timing | t_detect_stable=229ms |

