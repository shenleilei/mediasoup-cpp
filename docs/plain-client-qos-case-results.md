# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-17T01:06:00.336Z`

## 1. 汇总

- 总 Case：`43`
- 已执行：`43`
- 通过：`43`
- 失败：`0`
- 错误：`0`
- runner：`cpp_client`

## 2. 快速跳转

- 失败 / 错误：无
- baseline：[B1](#b1)、[B2](#b2)、[B3](#b3)
- bw_sweep：[BW1](#bw1)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)
- loss_sweep：[L1](#l1)、[L2](#l2)、[L3](#l3)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)
- rtt_sweep：[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)
- jitter_sweep：[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)
- burst：[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)

## 3. 逐 Case 结果

### B1

| 字段 | 内容 |
|---|---|
| Case ID | `B1` |
| 类型 | `baseline` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=943ms |
| recovery timing | t_detect_stable=932ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=943ms |
| raw recovery timing | t_detect_stable=932ms |

### B2

| 字段 | 内容 |
|---|---|
| Case ID | `B2` |
| 类型 | `baseline` / priority `P1` |
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| impairment 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 3ms |
| 预期 QoS | 期望状态=stable；maxLevel=0 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=983ms |
| recovery timing | t_detect_stable=12ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=983ms |
| raw recovery timing | t_detect_stable=12ms |

### B3

| 字段 | 内容 |
|---|---|
| Case ID | `B3` |
| 类型 | `baseline` / priority `P0` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| recovery 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=4 |
| 实际 QoS | baseline(current=early_warning/L1)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_warning=383ms |
| recovery timing | t_detect_warning=132ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=383ms |
| raw recovery timing | t_detect_warning=132ms |

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
| impairment timing | t_detect_stable=946ms |
| recovery timing | t_detect_stable=934ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=946ms |
| raw recovery timing | t_detect_stable=934ms |

### BW3

| 字段 | 内容 |
|---|---|
| Case ID | `BW3` |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 12 次非 noop） |
| impairment timing | t_detect_warning=1069ms；t_detect_recovering=10264ms；t_detect_stable=65ms；t_detect_congested=2109ms；t_first_action=1069ms；t_level_1=1069ms；t_level_2=2109ms；t_level_3=3148ms；t_level_4=4184ms；t_audio_only=4184ms |
| recovery timing | t_detect_recovering=3371ms；t_detect_stable=4371ms；t_detect_congested=331ms；t_first_action=3371ms；t_level_0=12621ms；t_level_1=9497ms；t_level_2=6375ms；t_level_3=3371ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1069ms；t_detect_recovering=10264ms；t_detect_stable=65ms；t_detect_congested=2109ms；t_first_action=1069ms；t_level_1=1069ms；t_level_2=2109ms；t_level_3=3148ms；t_level_4=4184ms；t_audio_only=4184ms |
| raw recovery timing | t_detect_recovering=3371ms；t_detect_stable=4371ms；t_detect_congested=331ms；t_first_action=3371ms；t_level_0=12621ms；t_level_1=9497ms；t_level_2=6375ms；t_level_3=3371ms |

### BW4

| 字段 | 内容 |
|---|---|
| Case ID | `BW4` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 800kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 12 次非 noop） |
| impairment timing | t_detect_warning=947ms；t_detect_recovering=10101ms；t_detect_stable=11101ms；t_detect_congested=1986ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1986ms；t_level_3=3025ms；t_level_4=4062ms；t_audio_only=4062ms |
| recovery timing | t_detect_recovering=3211ms；t_detect_stable=4211ms；t_detect_congested=211ms；t_first_action=3211ms；t_level_0=12419ms；t_level_1=9296ms；t_level_2=6255ms；t_level_3=3211ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=947ms；t_detect_recovering=10101ms；t_detect_stable=11101ms；t_detect_congested=1986ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1986ms；t_level_3=3025ms；t_level_4=4062ms；t_audio_only=4062ms |
| raw recovery timing | t_detect_recovering=3211ms；t_detect_stable=4211ms；t_detect_congested=211ms；t_first_action=3211ms；t_level_0=12419ms；t_level_1=9296ms；t_level_2=6255ms；t_level_3=3211ms |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=987ms；t_detect_congested=2027ms；t_first_action=987ms；t_level_1=987ms；t_level_2=2027ms；t_level_3=3066ms；t_level_4=4102ms；t_audio_only=4102ms |
| recovery timing | t_detect_recovering=4091ms；t_detect_stable=5091ms；t_detect_congested=91ms；t_first_action=4091ms；t_level_0=13256ms；t_level_1=10135ms；t_level_2=7094ms；t_level_3=4091ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=987ms；t_detect_congested=2027ms；t_first_action=987ms；t_level_1=987ms；t_level_2=2027ms；t_level_3=3066ms；t_level_4=4102ms；t_audio_only=4102ms |
| raw recovery timing | t_detect_recovering=4091ms；t_detect_stable=5091ms；t_detect_congested=91ms；t_first_action=4091ms；t_level_0=13256ms；t_level_1=10135ms；t_level_2=7094ms；t_level_3=4091ms |

### BW6

| 字段 | 内容 |
|---|---|
| Case ID | `BW6` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3029ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=4090ms；t_detect_stable=5090ms；t_detect_congested=90ms；t_first_action=4090ms；t_level_0=13295ms；t_level_1=10175ms；t_level_2=7134ms；t_level_3=4090ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3029ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=4090ms；t_detect_stable=5090ms；t_detect_congested=90ms；t_first_action=4090ms；t_level_0=13295ms；t_level_1=10175ms；t_level_2=7134ms；t_level_3=4090ms |

### BW7

| 字段 | 内容 |
|---|---|
| Case ID | `BW7` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 200kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=950ms；t_detect_congested=1990ms；t_first_action=950ms；t_level_1=950ms；t_level_2=1990ms；t_level_3=3030ms；t_level_4=4066ms；t_audio_only=4066ms |
| recovery timing | t_detect_recovering=4092ms；t_detect_stable=5092ms；t_detect_congested=92ms；t_first_action=4092ms；t_level_0=13297ms；t_level_1=10177ms；t_level_2=7136ms；t_level_3=4092ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=950ms；t_detect_congested=1990ms；t_first_action=950ms；t_level_1=950ms；t_level_2=1990ms；t_level_3=3030ms；t_level_4=4066ms；t_audio_only=4066ms |
| raw recovery timing | t_detect_recovering=4092ms；t_detect_stable=5092ms；t_detect_congested=92ms；t_first_action=4092ms；t_level_0=13297ms；t_level_1=10177ms；t_level_2=7136ms；t_level_3=4092ms |

### L1

| 字段 | 内容 |
|---|---|
| Case ID | `L1` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=979ms |
| recovery timing | t_detect_stable=968ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=979ms |
| raw recovery timing | t_detect_stable=968ms |

### L2

| 字段 | 内容 |
|---|---|
| Case ID | `L2` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=942ms |
| recovery timing | t_detect_stable=931ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=942ms |
| raw recovery timing | t_detect_stable=931ms |

### L3

| 字段 | 内容 |
|---|---|
| Case ID | `L3` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 2% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=941ms |
| recovery timing | t_detect_stable=930ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=941ms |
| raw recovery timing | t_detect_stable=930ms |

### L4

| 字段 | 内容 |
|---|---|
| Case ID | `L4` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=3948ms；t_detect_stable=944ms；t_first_action=3948ms；t_level_1=3948ms |
| recovery timing | t_detect_warning=492ms；t_detect_stable=3616ms；t_first_action=3616ms；t_level_0=3616ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3948ms；t_detect_stable=944ms；t_first_action=3948ms；t_level_1=3948ms |
| raw recovery timing | t_detect_warning=492ms；t_detect_stable=3616ms；t_first_action=3616ms；t_level_0=3616ms |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=early_warning/L3)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1986ms；t_detect_recovering=14181ms；t_detect_stable=982ms；t_detect_congested=4066ms；t_first_action=1986ms；t_level_1=1986ms；t_level_2=4066ms；t_level_3=5106ms；t_level_4=6142ms；t_audio_only=6142ms |
| recovery timing | t_detect_warning=251ms；t_detect_stable=4335ms；t_first_action=4335ms；t_level_0=10575ms；t_level_1=7455ms；t_level_2=4335ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1986ms；t_detect_recovering=14181ms；t_detect_stable=982ms；t_detect_congested=4066ms；t_first_action=1986ms；t_level_1=1986ms；t_level_2=4066ms；t_level_3=5106ms；t_level_4=6142ms；t_audio_only=6142ms |
| raw recovery timing | t_detect_warning=251ms；t_detect_stable=4335ms；t_first_action=4335ms；t_level_0=10575ms；t_level_1=7455ms；t_level_2=4335ms |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 10 次非 noop） |
| impairment timing | t_detect_warning=947ms；t_detect_recovering=15102ms；t_detect_stable=16102ms；t_detect_congested=1987ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1987ms；t_level_3=3026ms；t_level_4=4063ms；t_audio_only=4063ms |
| recovery timing | t_detect_recovering=7170ms；t_detect_stable=8170ms；t_detect_congested=170ms；t_first_action=7170ms；t_level_0=16375ms；t_level_1=13255ms；t_level_2=10214ms；t_level_3=7170ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=947ms；t_detect_recovering=15102ms；t_detect_stable=16102ms；t_detect_congested=1987ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1987ms；t_level_3=3026ms；t_level_4=4063ms；t_audio_only=4063ms |
| raw recovery timing | t_detect_recovering=7170ms；t_detect_stable=8170ms；t_detect_congested=170ms；t_first_action=7170ms；t_level_0=16375ms；t_level_1=13255ms；t_level_2=10214ms；t_level_3=7170ms |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 40% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=951ms；t_detect_congested=1990ms；t_first_action=951ms；t_level_1=951ms；t_level_2=1990ms；t_level_3=3030ms；t_level_4=4066ms；t_audio_only=4066ms |
| recovery timing | t_detect_recovering=4093ms；t_detect_stable=5093ms；t_detect_congested=93ms；t_first_action=4093ms；t_level_0=13298ms；t_level_1=10178ms；t_level_2=7137ms；t_level_3=4093ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=951ms；t_detect_congested=1990ms；t_first_action=951ms；t_level_1=951ms；t_level_2=1990ms；t_level_3=3030ms；t_level_4=4066ms；t_audio_only=4066ms |
| raw recovery timing | t_detect_recovering=4093ms；t_detect_stable=5093ms；t_detect_congested=93ms；t_first_action=4093ms；t_level_0=13298ms；t_level_1=10178ms；t_level_2=7137ms；t_level_3=4093ms |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 60% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3027ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=4091ms；t_detect_stable=5091ms；t_detect_congested=91ms；t_first_action=4091ms；t_level_0=13296ms；t_level_1=10176ms；t_level_2=7135ms；t_level_3=4091ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3027ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=4091ms；t_detect_stable=5091ms；t_detect_congested=91ms；t_first_action=4091ms；t_level_0=13296ms；t_level_1=10176ms；t_level_2=7135ms；t_level_3=4091ms |

### R1

| 字段 | 内容 |
|---|---|
| Case ID | `R1` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 50ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=946ms |
| recovery timing | t_detect_stable=935ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=946ms |
| raw recovery timing | t_detect_stable=935ms |

### R2

| 字段 | 内容 |
|---|---|
| Case ID | `R2` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 80ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=942ms |
| recovery timing | t_detect_stable=932ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=942ms |
| raw recovery timing | t_detect_stable=932ms |

### R3

| 字段 | 内容 |
|---|---|
| Case ID | `R3` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 120ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=983ms |
| recovery timing | t_detect_stable=971ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=983ms |
| raw recovery timing | t_detect_stable=971ms |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 类型 | `rtt_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=3948ms；t_detect_stable=944ms；t_first_action=3948ms；t_level_1=3948ms |
| recovery timing | t_detect_warning=573ms；t_detect_stable=3697ms；t_first_action=3697ms；t_level_0=3697ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3948ms；t_detect_stable=944ms；t_first_action=3948ms；t_level_1=3948ms |
| raw recovery timing | t_detect_warning=573ms；t_detect_stable=3697ms；t_first_action=3697ms；t_level_0=3697ms |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 250ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1948ms；t_detect_stable=943ms；t_detect_congested=6107ms；t_first_action=1948ms；t_level_1=1948ms；t_level_2=6107ms；t_level_3=7146ms；t_level_4=8182ms；t_audio_only=8182ms |
| recovery timing | t_detect_recovering=5171ms；t_detect_stable=6171ms；t_detect_congested=171ms；t_first_action=5171ms；t_level_0=14338ms；t_level_1=11215ms；t_level_2=8175ms；t_level_3=5171ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1948ms；t_detect_stable=943ms；t_detect_congested=6107ms；t_first_action=1948ms；t_level_1=1948ms；t_level_2=6107ms；t_level_3=7146ms；t_level_4=8182ms；t_audio_only=8182ms |
| raw recovery timing | t_detect_recovering=5171ms；t_detect_stable=6171ms；t_detect_congested=171ms；t_first_action=5171ms；t_level_0=14338ms；t_level_1=11215ms；t_level_2=8175ms；t_level_3=5171ms |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 350ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1948ms；t_detect_stable=944ms；t_detect_congested=2988ms；t_first_action=1948ms；t_level_1=1948ms；t_level_2=2988ms；t_level_3=4027ms；t_level_4=5063ms；t_audio_only=5063ms |
| recovery timing | t_detect_recovering=6051ms；t_detect_stable=7051ms；t_detect_congested=51ms；t_first_action=6051ms；t_level_0=15218ms；t_level_1=12095ms；t_level_2=9054ms；t_level_3=6051ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1948ms；t_detect_stable=944ms；t_detect_congested=2988ms；t_first_action=1948ms；t_level_1=1948ms；t_level_2=2988ms；t_level_3=4027ms；t_level_4=5063ms；t_audio_only=5063ms |
| raw recovery timing | t_detect_recovering=6051ms；t_detect_stable=7051ms；t_detect_congested=51ms；t_first_action=6051ms；t_level_0=15218ms；t_level_1=12095ms；t_level_2=9054ms；t_level_3=6051ms |

### J1

| 字段 | 内容 |
|---|---|
| Case ID | `J1` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 10ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=945ms |
| recovery timing | t_detect_stable=935ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=945ms |
| raw recovery timing | t_detect_stable=935ms |

### J2

| 字段 | 内容 |
|---|---|
| Case ID | `J2` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 20ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=985ms |
| recovery timing | t_detect_stable=974ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=985ms |
| raw recovery timing | t_detect_stable=974ms |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 类型 | `jitter_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2948ms；t_detect_stable=943ms；t_first_action=2948ms；t_level_1=2948ms |
| recovery timing | t_detect_warning=532ms；t_detect_stable=3657ms；t_first_action=3657ms；t_level_0=3657ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2948ms；t_detect_stable=943ms；t_first_action=2948ms；t_level_1=2948ms |
| raw recovery timing | t_detect_warning=532ms；t_detect_stable=3657ms；t_first_action=3657ms；t_level_0=3657ms |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=1990ms；t_detect_stable=985ms；t_first_action=1990ms；t_level_1=1990ms |
| recovery timing | t_detect_warning=613ms；t_detect_stable=5824ms；t_first_action=5824ms；t_level_0=5824ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1990ms；t_detect_stable=985ms；t_first_action=1990ms；t_level_1=1990ms |
| raw recovery timing | t_detect_warning=613ms；t_detect_stable=5824ms；t_first_action=5824ms；t_level_0=5824ms |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 100ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_recovering=10064ms；t_detect_stable=11064ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_0=19229ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_stable=213ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_recovering=10064ms；t_detect_stable=11064ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_0=19229ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_stable=213ms |

### T1

| 字段 | 内容 |
|---|---|
| Case ID | `T1` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 2000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=183ms |
| recovery timing | t_detect_stable=371ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=183ms |
| raw recovery timing | t_detect_stable=371ms |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 12 次非 noop） |
| impairment timing | t_detect_warning=947ms；t_detect_recovering=10062ms；t_detect_stable=11062ms；t_detect_congested=1987ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1987ms；t_level_3=3026ms；t_level_4=4062ms；t_audio_only=4062ms |
| recovery timing | t_detect_recovering=3090ms；t_detect_stable=4090ms；t_detect_congested=90ms；t_first_action=3090ms；t_level_0=12255ms；t_level_1=9134ms；t_level_2=6093ms；t_level_3=3090ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=947ms；t_detect_recovering=10062ms；t_detect_stable=11062ms；t_detect_congested=1987ms；t_first_action=947ms；t_level_1=947ms；t_level_2=1987ms；t_level_3=3026ms；t_level_4=4062ms；t_audio_only=4062ms |
| raw recovery timing | t_detect_recovering=3090ms；t_detect_stable=4090ms；t_detect_congested=90ms；t_first_action=3090ms；t_level_0=12255ms；t_level_1=9134ms；t_level_2=6093ms；t_level_3=3090ms |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_congested=1989ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1989ms；t_level_3=3028ms；t_level_4=4065ms；t_audio_only=4065ms |
| recovery timing | t_detect_recovering=4094ms；t_detect_stable=5094ms；t_detect_congested=94ms；t_first_action=4094ms；t_level_0=13299ms；t_level_1=10179ms；t_level_2=7138ms；t_level_3=4094ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_congested=1989ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1989ms；t_level_3=3028ms；t_level_4=4065ms；t_audio_only=4065ms |
| raw recovery timing | t_detect_recovering=4094ms；t_detect_stable=5094ms；t_detect_congested=94ms；t_first_action=4094ms；t_level_0=13299ms；t_level_1=10179ms；t_level_2=7138ms；t_level_3=4094ms |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=3990ms；t_detect_stable=985ms；t_first_action=3990ms；t_level_1=3990ms |
| recovery timing | t_detect_warning=614ms；t_detect_stable=3738ms；t_first_action=3738ms；t_level_0=3738ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3990ms；t_detect_stable=985ms；t_first_action=3990ms；t_level_1=3990ms |
| raw recovery timing | t_detect_warning=614ms；t_detect_stable=3738ms；t_first_action=3738ms；t_level_0=3738ms |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 10 次非 noop） |
| impairment timing | t_detect_warning=1067ms；t_detect_recovering=15221ms；t_detect_stable=63ms；t_detect_congested=2107ms；t_first_action=1067ms；t_level_1=1067ms；t_level_2=2107ms；t_level_3=3146ms；t_level_4=4182ms；t_audio_only=4182ms |
| recovery timing | t_detect_recovering=7330ms；t_detect_stable=8329ms；t_detect_congested=289ms；t_first_action=7330ms；t_level_0=16534ms；t_level_1=13414ms；t_level_2=10373ms；t_level_3=7330ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1067ms；t_detect_recovering=15221ms；t_detect_stable=63ms；t_detect_congested=2107ms；t_first_action=1067ms；t_level_1=1067ms；t_level_2=2107ms；t_level_3=3146ms；t_level_4=4182ms；t_audio_only=4182ms |
| raw recovery timing | t_detect_recovering=7330ms；t_detect_stable=8329ms；t_detect_congested=289ms；t_first_action=7330ms；t_level_0=16534ms；t_level_1=13414ms；t_level_2=10373ms；t_level_3=7330ms |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=3946ms；t_detect_stable=942ms；t_first_action=3946ms；t_level_1=3946ms |
| recovery timing | t_detect_warning=571ms；t_detect_stable=3696ms；t_first_action=3696ms；t_level_0=3696ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3946ms；t_detect_stable=942ms；t_first_action=3946ms；t_level_1=3946ms |
| raw recovery timing | t_detect_warning=571ms；t_detect_stable=3696ms；t_first_action=3696ms；t_level_0=3696ms |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2946ms；t_detect_stable=942ms；t_first_action=2946ms；t_level_1=2946ms |
| recovery timing | t_detect_warning=611ms；t_detect_stable=3735ms；t_first_action=3735ms；t_level_0=3735ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2946ms；t_detect_stable=942ms；t_first_action=2946ms；t_level_1=2946ms |
| raw recovery timing | t_detect_warning=611ms；t_detect_stable=3735ms；t_first_action=3735ms；t_level_0=3735ms |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| recovery 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| 预期 QoS | 期望状态=congested；maxLevel=4；recovery=disabled |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_congested=1989ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1989ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_congested=1989ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1989ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | - |

### T9

| 字段 | 内容 |
|---|---|
| Case ID | `T9` |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 8000kbps / RTT 20ms / loss 0.1% / jitter 1ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=986ms；t_detect_congested=2026ms；t_first_action=986ms；t_level_1=986ms；t_level_2=2026ms；t_level_3=3066ms；t_level_4=4102ms；t_audio_only=4102ms |
| recovery timing | t_detect_recovering=7130ms；t_detect_stable=8130ms；t_detect_congested=130ms；t_first_action=7130ms；t_level_0=16335ms；t_level_1=13215ms；t_level_2=10174ms；t_level_3=7130ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=986ms；t_detect_congested=2026ms；t_first_action=986ms；t_level_1=986ms；t_level_2=2026ms；t_level_3=3066ms；t_level_4=4102ms；t_audio_only=4102ms |
| raw recovery timing | t_detect_recovering=7130ms；t_detect_stable=8130ms；t_detect_congested=130ms；t_first_action=7130ms；t_level_0=16335ms；t_level_1=13215ms；t_level_2=10174ms；t_level_3=7130ms |

### T10

| 字段 | 内容 |
|---|---|
| Case ID | `T10` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=7131ms；t_detect_stable=8131ms；t_detect_congested=131ms；t_first_action=7131ms；t_level_0=16337ms；t_level_1=13216ms；t_level_2=10175ms；t_level_3=7131ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=949ms；t_detect_congested=1988ms；t_first_action=949ms；t_level_1=949ms；t_level_2=1988ms；t_level_3=3028ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=7131ms；t_detect_stable=8131ms；t_detect_congested=131ms；t_first_action=7131ms；t_level_0=16337ms；t_level_1=13216ms；t_level_2=10175ms；t_level_3=7131ms |

### T11

| 字段 | 内容 |
|---|---|
| Case ID | `T11` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1106ms；t_detect_stable=102ms；t_detect_congested=2146ms；t_first_action=1106ms；t_level_1=1106ms；t_level_2=2146ms；t_level_3=3185ms；t_level_4=4221ms；t_audio_only=4221ms |
| recovery timing | t_detect_recovering=6647ms；t_detect_stable=7687ms；t_detect_congested=646ms；t_first_action=6647ms；t_level_0=15893ms；t_level_1=12774ms；t_level_2=9731ms；t_level_3=6647ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1106ms；t_detect_stable=102ms；t_detect_congested=2146ms；t_first_action=1106ms；t_level_1=1106ms；t_level_2=2146ms；t_level_3=3185ms；t_level_4=4221ms；t_audio_only=4221ms |
| raw recovery timing | t_detect_recovering=6647ms；t_detect_stable=7687ms；t_detect_congested=646ms；t_first_action=6647ms；t_level_0=15893ms；t_level_1=12774ms；t_level_2=9731ms；t_level_3=6647ms |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L2, current=congested/L2)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1949ms；t_detect_stable=944ms；t_detect_congested=4028ms；t_first_action=1949ms；t_level_1=1949ms；t_level_2=4028ms |
| recovery timing | t_detect_recovering=7132ms；t_detect_stable=8132ms；t_detect_congested=57ms；t_first_action=57ms；t_level_0=16337ms；t_level_1=13217ms；t_level_2=10177ms；t_level_3=57ms；t_level_4=1093ms；t_audio_only=1093ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1949ms；t_detect_stable=944ms；t_detect_congested=4028ms；t_first_action=1949ms；t_level_1=1949ms；t_level_2=4028ms |
| raw recovery timing | t_detect_recovering=7132ms；t_detect_stable=8132ms；t_detect_congested=57ms；t_first_action=57ms；t_level_0=16337ms；t_level_1=13217ms；t_level_2=10177ms；t_level_3=57ms；t_level_4=1093ms；t_audio_only=1093ms |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1186ms；t_detect_stable=182ms；t_detect_congested=2225ms；t_first_action=1186ms；t_level_1=1186ms；t_level_2=2225ms；t_level_3=3265ms；t_level_4=4301ms；t_audio_only=4301ms |
| recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13494ms；t_level_1=10413ms；t_level_2=7372ms；t_level_3=4328ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1186ms；t_detect_stable=182ms；t_detect_congested=2225ms；t_first_action=1186ms；t_level_1=1186ms；t_level_2=2225ms；t_level_3=3265ms；t_level_4=4301ms；t_audio_only=4301ms |
| raw recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13494ms；t_level_1=10413ms；t_level_2=7372ms；t_level_3=4328ms |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 200ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2950ms；t_detect_stable=946ms；t_first_action=2950ms；t_level_1=2950ms |
| recovery timing | t_detect_warning=14ms；t_detect_stable=3138ms；t_first_action=3138ms；t_level_0=3138ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2950ms；t_detect_stable=946ms；t_first_action=2950ms；t_level_1=2950ms |
| raw recovery timing | t_detect_warning=14ms；t_detect_stable=3138ms；t_first_action=3138ms；t_level_0=3138ms |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=early_warning/L1, current=early_warning/L1)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters（共 2 次非 noop） |
| impairment timing | t_detect_warning=2029ms；t_detect_stable=25ms；t_first_action=2029ms；t_level_1=2029ms |
| recovery timing | t_detect_warning=132ms；t_detect_stable=4295ms；t_first_action=4295ms；t_level_0=4295ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2029ms；t_detect_stable=25ms；t_first_action=2029ms；t_level_1=2029ms |
| raw recovery timing | t_detect_warning=132ms；t_detect_stable=4295ms；t_first_action=4295ms；t_level_0=4295ms |
