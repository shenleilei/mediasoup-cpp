# PlainTransport C++ Client QoS Matrix 逐 Case 结果

> 当前状态说明
>
> 这是 full matrix 的当前机器结果展开页。
> 如果只想先看摘要口径，再决定是否下钻 case 细节，
> 先看 [plain-client-qos-status.md](./plain-client-qos-status.md)。

生成时间：`2026-04-16T10:42:47.053Z`

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
| impairment timing | t_detect_stable=947ms |
| recovery timing | t_detect_stable=936ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=947ms |
| raw recovery timing | t_detect_stable=936ms |

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
| impairment timing | t_detect_stable=944ms |
| recovery timing | t_detect_stable=932ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=944ms |
| raw recovery timing | t_detect_stable=932ms |

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
| impairment timing | t_detect_warning=386ms |
| recovery timing | t_detect_warning=134ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=386ms |
| raw recovery timing | t_detect_warning=134ms |

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
| impairment timing | t_detect_warning=983ms；t_detect_recovering=10142ms；t_detect_stable=11142ms；t_detect_congested=2023ms；t_first_action=983ms；t_level_1=983ms；t_level_2=2023ms；t_level_3=3063ms；t_level_4=4103ms；t_audio_only=4103ms |
| recovery timing | t_detect_recovering=4331ms；t_detect_stable=5331ms；t_detect_congested=291ms；t_first_action=4331ms；t_level_0=13532ms；t_level_1=10412ms；t_level_2=7371ms；t_level_3=4331ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=983ms；t_detect_recovering=10142ms；t_detect_stable=11142ms；t_detect_congested=2023ms；t_first_action=983ms；t_level_1=983ms；t_level_2=2023ms；t_level_3=3063ms；t_level_4=4103ms；t_audio_only=4103ms |
| raw recovery timing | t_detect_recovering=4331ms；t_detect_stable=5331ms；t_detect_congested=291ms；t_first_action=4331ms；t_level_0=13532ms；t_level_1=10412ms；t_level_2=7371ms；t_level_3=4331ms |

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
| impairment timing | t_detect_warning=942ms；t_detect_recovering=10061ms；t_detect_congested=1982ms；t_first_action=942ms；t_level_1=942ms；t_level_2=1982ms；t_level_3=3023ms；t_level_4=4062ms；t_audio_only=4062ms |
| recovery timing | t_detect_recovering=3090ms；t_detect_stable=4090ms；t_detect_congested=90ms；t_first_action=3090ms；t_level_0=12252ms；t_level_1=9131ms；t_level_2=6090ms；t_level_3=3090ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=942ms；t_detect_recovering=10061ms；t_detect_congested=1982ms；t_first_action=942ms；t_level_1=942ms；t_level_2=1982ms；t_level_3=3023ms；t_level_4=4062ms；t_audio_only=4062ms |
| raw recovery timing | t_detect_recovering=3090ms；t_detect_stable=4090ms；t_detect_congested=90ms；t_first_action=3090ms；t_level_0=12252ms；t_level_1=9131ms；t_level_2=6090ms；t_level_3=3090ms |

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
| impairment timing | t_detect_warning=945ms；t_detect_congested=1985ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1985ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=4093ms；t_detect_stable=5093ms；t_detect_congested=93ms；t_first_action=4093ms；t_level_0=13254ms；t_level_1=10134ms；t_level_2=7093ms；t_level_3=4093ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=945ms；t_detect_congested=1985ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1985ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=4093ms；t_detect_stable=5093ms；t_detect_congested=93ms；t_first_action=4093ms；t_level_0=13254ms；t_level_1=10134ms；t_level_2=7093ms；t_level_3=4093ms |

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
| impairment timing | t_detect_warning=945ms；t_detect_congested=1984ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1984ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=4051ms；t_detect_stable=5051ms；t_detect_congested=51ms；t_first_action=4051ms；t_level_0=13213ms；t_level_1=10092ms；t_level_2=7051ms；t_level_3=4051ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=945ms；t_detect_congested=1984ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1984ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=4051ms；t_detect_stable=5051ms；t_detect_congested=51ms；t_first_action=4051ms；t_level_0=13213ms；t_level_1=10092ms；t_level_2=7051ms；t_level_3=4051ms |

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
| impairment timing | t_detect_warning=952ms；t_detect_congested=1992ms；t_first_action=952ms；t_level_1=952ms；t_level_2=1992ms；t_level_3=3032ms；t_level_4=4072ms；t_audio_only=4072ms |
| recovery timing | t_detect_recovering=4100ms；t_detect_stable=5100ms；t_detect_congested=100ms；t_first_action=4100ms；t_level_0=13301ms；t_level_1=10181ms；t_level_2=7140ms；t_level_3=4100ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=952ms；t_detect_congested=1992ms；t_first_action=952ms；t_level_1=952ms；t_level_2=1992ms；t_level_3=3032ms；t_level_4=4072ms；t_audio_only=4072ms |
| raw recovery timing | t_detect_recovering=4100ms；t_detect_stable=5100ms；t_detect_congested=100ms；t_first_action=4100ms；t_level_0=13301ms；t_level_1=10181ms；t_level_2=7140ms；t_level_3=4100ms |

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
| impairment timing | t_detect_stable=983ms |
| recovery timing | t_detect_stable=11ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=983ms |
| raw recovery timing | t_detect_stable=11ms |

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
| impairment timing | t_detect_stable=945ms |
| recovery timing | t_detect_stable=935ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=945ms |
| raw recovery timing | t_detect_stable=935ms |

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
| impairment timing | t_detect_stable=63ms |
| recovery timing | t_detect_stable=52ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=63ms |
| raw recovery timing | t_detect_stable=52ms |

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
| impairment timing | t_detect_warning=4946ms；t_detect_stable=946ms；t_first_action=4946ms；t_level_1=4946ms |
| recovery timing | t_detect_warning=534ms；t_detect_stable=3654ms；t_first_action=3654ms；t_level_0=3654ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4946ms；t_detect_stable=946ms；t_first_action=4946ms；t_level_1=4946ms |
| raw recovery timing | t_detect_warning=534ms；t_detect_stable=3654ms；t_first_action=3654ms；t_level_0=3654ms |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=stable/L3)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=946ms；t_detect_recovering=16144ms；t_detect_stable=17144ms；t_detect_congested=4066ms；t_first_action=946ms；t_level_1=946ms；t_level_2=4066ms；t_level_3=5105ms；t_level_4=6145ms；t_audio_only=6145ms |
| recovery timing | t_detect_stable=132ms；t_first_action=2132ms；t_level_0=8294ms；t_level_1=5173ms；t_level_2=2132ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=946ms；t_detect_recovering=16144ms；t_detect_stable=17144ms；t_detect_congested=4066ms；t_first_action=946ms；t_level_1=946ms；t_level_2=4066ms；t_level_3=5105ms；t_level_4=6145ms；t_audio_only=6145ms |
| raw recovery timing | t_detect_stable=132ms；t_first_action=2132ms；t_level_0=8294ms；t_level_1=5173ms；t_level_2=2132ms |

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
| impairment timing | t_detect_warning=986ms；t_detect_recovering=15104ms；t_detect_congested=2025ms；t_first_action=986ms；t_level_1=986ms；t_level_2=2025ms；t_level_3=3065ms；t_level_4=4105ms；t_audio_only=4105ms |
| recovery timing | t_detect_recovering=7089ms；t_detect_stable=8089ms；t_detect_congested=90ms；t_first_action=7089ms；t_level_0=16251ms；t_level_1=13130ms；t_level_2=10089ms；t_level_3=7089ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=986ms；t_detect_recovering=15104ms；t_detect_congested=2025ms；t_first_action=986ms；t_level_1=986ms；t_level_2=2025ms；t_level_3=3065ms；t_level_4=4105ms；t_audio_only=4105ms |
| raw recovery timing | t_detect_recovering=7089ms；t_detect_stable=8089ms；t_detect_congested=90ms；t_first_action=7089ms；t_level_0=16251ms；t_level_1=13130ms；t_level_2=10089ms；t_level_3=7089ms |

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
| impairment timing | t_detect_warning=1062ms；t_detect_stable=62ms；t_detect_congested=2102ms；t_first_action=1062ms；t_level_1=1062ms；t_level_2=2102ms；t_level_3=3142ms；t_level_4=4182ms；t_audio_only=4182ms |
| recovery timing | t_detect_recovering=4210ms；t_detect_stable=5210ms；t_detect_congested=210ms；t_first_action=4210ms；t_level_0=13411ms；t_level_1=10291ms；t_level_2=7250ms；t_level_3=4210ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1062ms；t_detect_stable=62ms；t_detect_congested=2102ms；t_first_action=1062ms；t_level_1=1062ms；t_level_2=2102ms；t_level_3=3142ms；t_level_4=4182ms；t_audio_only=4182ms |
| raw recovery timing | t_detect_recovering=4210ms；t_detect_stable=5210ms；t_detect_congested=210ms；t_first_action=4210ms；t_level_0=13411ms；t_level_1=10291ms；t_level_2=7250ms；t_level_3=4210ms |

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
| impairment timing | t_detect_warning=984ms；t_detect_congested=2024ms；t_first_action=984ms；t_level_1=984ms；t_level_2=2024ms；t_level_3=3064ms；t_level_4=4104ms；t_audio_only=4104ms |
| recovery timing | t_detect_recovering=4132ms；t_detect_stable=5132ms；t_detect_congested=132ms；t_first_action=4132ms；t_level_0=13333ms；t_level_1=10213ms；t_level_2=7172ms；t_level_3=4132ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=984ms；t_detect_congested=2024ms；t_first_action=984ms；t_level_1=984ms；t_level_2=2024ms；t_level_3=3064ms；t_level_4=4104ms；t_audio_only=4104ms |
| raw recovery timing | t_detect_recovering=4132ms；t_detect_stable=5132ms；t_detect_congested=132ms；t_first_action=4132ms；t_level_0=13333ms；t_level_1=10213ms；t_level_2=7172ms；t_level_3=4132ms |

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
| impairment timing | t_detect_stable=64ms |
| recovery timing | t_detect_stable=53ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=64ms |
| raw recovery timing | t_detect_stable=53ms |

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
| impairment timing | t_detect_stable=945ms |
| recovery timing | t_detect_stable=934ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=945ms |
| raw recovery timing | t_detect_stable=934ms |

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
| impairment timing | t_detect_stable=62ms |
| recovery timing | t_detect_stable=51ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=62ms |
| raw recovery timing | t_detect_stable=51ms |

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
| impairment timing | t_detect_warning=945ms；t_first_action=945ms；t_level_1=945ms |
| recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=945ms；t_first_action=945ms；t_level_1=945ms |
| raw recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |

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
| impairment timing | t_detect_warning=1025ms；t_detect_stable=25ms；t_detect_congested=6225ms；t_first_action=1025ms；t_level_1=1025ms；t_level_2=6225ms；t_level_3=7265ms；t_level_4=8305ms；t_audio_only=8305ms |
| recovery timing | t_detect_recovering=5373ms；t_detect_stable=6373ms；t_detect_congested=333ms；t_first_action=5373ms；t_level_0=14494ms；t_level_1=11454ms；t_level_2=8413ms；t_level_3=5373ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1025ms；t_detect_stable=25ms；t_detect_congested=6225ms；t_first_action=1025ms；t_level_1=1025ms；t_level_2=6225ms；t_level_3=7265ms；t_level_4=8305ms；t_audio_only=8305ms |
| raw recovery timing | t_detect_recovering=5373ms；t_detect_stable=6373ms；t_detect_congested=333ms；t_first_action=5373ms；t_level_0=14494ms；t_level_1=11454ms；t_level_2=8413ms；t_level_3=5373ms |

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
| impairment timing | t_detect_warning=1062ms；t_detect_stable=62ms；t_detect_congested=3142ms；t_first_action=1062ms；t_level_1=1062ms；t_level_2=3142ms；t_level_3=4182ms；t_level_4=5222ms；t_audio_only=5222ms |
| recovery timing | t_detect_recovering=6250ms；t_detect_stable=7250ms；t_detect_congested=250ms；t_first_action=6250ms；t_level_0=15451ms；t_level_1=12331ms；t_level_2=9290ms；t_level_3=6250ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1062ms；t_detect_stable=62ms；t_detect_congested=3142ms；t_first_action=1062ms；t_level_1=1062ms；t_level_2=3142ms；t_level_3=4182ms；t_level_4=5222ms；t_audio_only=5222ms |
| raw recovery timing | t_detect_recovering=6250ms；t_detect_stable=7250ms；t_detect_congested=250ms；t_first_action=6250ms；t_level_0=15451ms；t_level_1=12331ms；t_level_2=9290ms；t_level_3=6250ms |

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
| impairment timing | t_detect_stable=944ms |
| recovery timing | t_detect_stable=933ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=944ms |
| raw recovery timing | t_detect_stable=933ms |

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
| impairment timing | t_detect_stable=183ms |
| recovery timing | t_detect_stable=291ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=183ms |
| raw recovery timing | t_detect_stable=291ms |

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
| impairment timing | t_detect_warning=943ms；t_first_action=943ms；t_level_1=943ms |
| recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=943ms；t_first_action=943ms；t_level_1=943ms |
| raw recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |

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
| impairment timing | t_detect_warning=1065ms；t_detect_stable=65ms；t_first_action=1065ms；t_level_1=1065ms |
| recovery timing | t_detect_warning=814ms；t_detect_stable=4974ms；t_first_action=4974ms；t_level_0=4974ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1065ms；t_detect_stable=65ms；t_first_action=1065ms；t_level_1=1065ms |
| raw recovery timing | t_detect_warning=814ms；t_detect_stable=4974ms；t_first_action=4974ms；t_level_0=4974ms |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 100ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 12 次非 noop） |
| impairment timing | t_detect_warning=1185ms；t_detect_recovering=10343ms；t_detect_stable=144ms；t_detect_congested=2224ms；t_first_action=1185ms；t_level_1=1185ms；t_level_2=2224ms；t_level_3=3264ms；t_level_4=4305ms；t_audio_only=4305ms |
| recovery timing | t_detect_recovering=6612ms；t_detect_stable=7612ms；t_detect_congested=572ms；t_first_action=6612ms；t_level_0=15814ms；t_level_1=12693ms；t_level_2=9652ms；t_level_3=6612ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1185ms；t_detect_recovering=10343ms；t_detect_stable=144ms；t_detect_congested=2224ms；t_first_action=1185ms；t_level_1=1185ms；t_level_2=2224ms；t_level_3=3264ms；t_level_4=4305ms；t_audio_only=4305ms |
| raw recovery timing | t_detect_recovering=6612ms；t_detect_stable=7612ms；t_detect_congested=572ms；t_first_action=6612ms；t_level_0=15814ms；t_level_1=12693ms；t_level_2=9652ms；t_level_3=6612ms |

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
| impairment timing | t_detect_stable=983ms |
| recovery timing | t_detect_stable=972ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=983ms |
| raw recovery timing | t_detect_stable=972ms |

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
| impairment timing | t_detect_warning=983ms；t_detect_recovering=10142ms；t_detect_stable=11142ms；t_detect_congested=2023ms；t_first_action=983ms；t_level_1=983ms；t_level_2=2023ms；t_level_3=3063ms；t_level_4=4103ms；t_audio_only=4103ms |
| recovery timing | t_detect_recovering=4291ms；t_detect_stable=5291ms；t_detect_congested=291ms；t_first_action=4291ms；t_level_0=13492ms；t_level_1=10372ms；t_level_2=7331ms；t_level_3=4291ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=983ms；t_detect_recovering=10142ms；t_detect_stable=11142ms；t_detect_congested=2023ms；t_first_action=983ms；t_level_1=983ms；t_level_2=2023ms；t_level_3=3063ms；t_level_4=4103ms；t_audio_only=4103ms |
| raw recovery timing | t_detect_recovering=4291ms；t_detect_stable=5291ms；t_detect_congested=291ms；t_first_action=4291ms；t_level_0=13492ms；t_level_1=10372ms；t_level_2=7331ms；t_level_3=4291ms |

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
| impairment timing | t_detect_warning=945ms；t_detect_congested=1985ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1985ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=4092ms；t_detect_stable=5092ms；t_detect_congested=92ms；t_first_action=4092ms；t_level_0=13293ms；t_level_1=10173ms；t_level_2=7132ms；t_level_3=4092ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=945ms；t_detect_congested=1985ms；t_first_action=945ms；t_level_1=945ms；t_level_2=1985ms；t_level_3=3025ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=4092ms；t_detect_stable=5092ms；t_detect_congested=92ms；t_first_action=4092ms；t_level_0=13293ms；t_level_1=10173ms；t_level_2=7132ms；t_level_3=4092ms |

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
| impairment timing | t_detect_warning=3943ms；t_detect_stable=943ms；t_first_action=3943ms；t_level_1=3943ms |
| recovery timing | t_detect_warning=572ms；t_detect_stable=3692ms；t_first_action=3692ms；t_level_0=3692ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3943ms；t_detect_stable=943ms；t_first_action=3943ms；t_level_1=3943ms |
| raw recovery timing | t_detect_warning=572ms；t_detect_stable=3692ms；t_first_action=3692ms；t_level_0=3692ms |

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
| impairment timing | t_detect_warning=984ms；t_detect_recovering=15143ms；t_detect_congested=2024ms；t_first_action=984ms；t_level_1=984ms；t_level_2=2024ms；t_level_3=3064ms；t_level_4=4105ms；t_audio_only=4105ms |
| recovery timing | t_detect_recovering=7131ms；t_detect_stable=8131ms；t_detect_congested=131ms；t_first_action=7131ms；t_level_0=16293ms；t_level_1=13172ms；t_level_2=10131ms；t_level_3=7131ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=984ms；t_detect_recovering=15143ms；t_detect_congested=2024ms；t_first_action=984ms；t_level_1=984ms；t_level_2=2024ms；t_level_3=3064ms；t_level_4=4105ms；t_audio_only=4105ms |
| raw recovery timing | t_detect_recovering=7131ms；t_detect_stable=8131ms；t_detect_congested=131ms；t_first_action=7131ms；t_level_0=16293ms；t_level_1=13172ms；t_level_2=10131ms；t_level_3=7131ms |

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
| impairment timing | t_detect_warning=943ms；t_first_action=943ms；t_level_1=943ms |
| recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=943ms；t_first_action=943ms；t_level_1=943ms |
| raw recovery timing | t_detect_warning=693ms；t_detect_stable=3813ms；t_first_action=3813ms；t_level_0=3813ms |

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
| impairment timing | t_detect_warning=944ms；t_first_action=944ms；t_level_1=944ms |
| recovery timing | t_detect_warning=692ms；t_detect_stable=3812ms；t_first_action=3812ms；t_level_0=3812ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=944ms；t_first_action=944ms；t_level_1=944ms |
| raw recovery timing | t_detect_warning=692ms；t_detect_stable=3812ms；t_first_action=3812ms；t_level_0=3812ms |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| recovery 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| 预期 QoS | 期望状态=congested；maxLevel=4；recovery=disabled |
| 实际 QoS | baseline(current=early_warning/L1)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=383ms；t_detect_congested=1423ms；t_first_action=1423ms；t_level_2=1423ms；t_level_3=2463ms；t_level_4=3503ms；t_audio_only=3503ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=383ms；t_detect_congested=1423ms；t_first_action=1423ms；t_level_2=1423ms；t_level_3=2463ms；t_level_4=3503ms；t_audio_only=3503ms |
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
| impairment timing | t_detect_warning=943ms；t_detect_congested=1984ms；t_first_action=943ms；t_level_1=943ms；t_level_2=1984ms；t_level_3=3024ms；t_level_4=4063ms；t_audio_only=4063ms |
| recovery timing | t_detect_recovering=7050ms；t_detect_stable=8050ms；t_detect_congested=50ms；t_first_action=7050ms；t_level_0=16252ms；t_level_1=13132ms；t_level_2=10091ms；t_level_3=7050ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=943ms；t_detect_congested=1984ms；t_first_action=943ms；t_level_1=943ms；t_level_2=1984ms；t_level_3=3024ms；t_level_4=4063ms；t_audio_only=4063ms |
| raw recovery timing | t_detect_recovering=7050ms；t_detect_stable=8050ms；t_detect_congested=50ms；t_first_action=7050ms；t_level_0=16252ms；t_level_1=13132ms；t_level_2=10091ms；t_level_3=7050ms |

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
| impairment timing | t_detect_warning=944ms；t_detect_congested=1984ms；t_first_action=944ms；t_level_1=944ms；t_level_2=1984ms；t_level_3=3024ms；t_level_4=4064ms；t_audio_only=4064ms |
| recovery timing | t_detect_recovering=7092ms；t_detect_stable=8092ms；t_detect_congested=92ms；t_first_action=7092ms；t_level_0=16293ms；t_level_1=13173ms；t_level_2=10132ms；t_level_3=7092ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=944ms；t_detect_congested=1984ms；t_first_action=944ms；t_level_1=944ms；t_level_2=1984ms；t_level_3=3024ms；t_level_4=4064ms；t_audio_only=4064ms |
| raw recovery timing | t_detect_recovering=7092ms；t_detect_stable=8092ms；t_detect_congested=92ms；t_first_action=7092ms；t_level_0=16293ms；t_level_1=13173ms；t_level_2=10132ms；t_level_3=7092ms |

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
| impairment timing | t_detect_warning=979ms；t_detect_congested=2019ms；t_first_action=979ms；t_level_1=979ms；t_level_2=2019ms；t_level_3=3059ms；t_level_4=4099ms；t_audio_only=4099ms |
| recovery timing | t_detect_recovering=6126ms；t_detect_stable=7126ms；t_detect_congested=126ms；t_first_action=6126ms；t_level_0=15327ms；t_level_1=12207ms；t_level_2=9166ms；t_level_3=6126ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=979ms；t_detect_congested=2019ms；t_first_action=979ms；t_level_1=979ms；t_level_2=2019ms；t_level_3=3059ms；t_level_4=4099ms；t_audio_only=4099ms |
| raw recovery timing | t_detect_recovering=6126ms；t_detect_stable=7126ms；t_detect_congested=126ms；t_first_action=6126ms；t_level_0=15327ms；t_level_1=12207ms；t_level_2=9166ms；t_level_3=6126ms |

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
| impairment timing | t_detect_warning=945ms；t_detect_congested=4065ms；t_first_action=945ms；t_level_1=945ms；t_level_2=4065ms |
| recovery timing | t_detect_recovering=7173ms；t_detect_stable=8173ms；t_detect_congested=94ms；t_first_action=94ms；t_level_0=16374ms；t_level_1=13254ms；t_level_2=10213ms；t_level_3=94ms；t_level_4=1134ms；t_audio_only=1134ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=945ms；t_detect_congested=4065ms；t_first_action=945ms；t_level_1=945ms；t_level_2=4065ms |
| raw recovery timing | t_detect_recovering=7173ms；t_detect_stable=8173ms；t_detect_congested=94ms；t_first_action=94ms；t_level_0=16374ms；t_level_1=13254ms；t_level_2=10213ms；t_level_3=94ms；t_level_4=1134ms；t_audio_only=1134ms |

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
| impairment timing | t_detect_warning=943ms；t_detect_congested=1983ms；t_first_action=943ms；t_level_1=943ms；t_level_2=1983ms；t_level_3=3023ms；t_level_4=4062ms；t_audio_only=4062ms |
| recovery timing | t_detect_recovering=4050ms；t_detect_stable=5050ms；t_detect_congested=50ms；t_first_action=4050ms；t_level_0=13251ms；t_level_1=10131ms；t_level_2=7090ms；t_level_3=4050ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=943ms；t_detect_congested=1983ms；t_first_action=943ms；t_level_1=943ms；t_level_2=1983ms；t_level_3=3023ms；t_level_4=4062ms；t_audio_only=4062ms |
| raw recovery timing | t_detect_recovering=4050ms；t_detect_stable=5050ms；t_detect_congested=50ms；t_first_action=4050ms；t_level_0=13251ms；t_level_1=10131ms；t_level_2=7090ms；t_level_3=4050ms |

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
| impairment timing | t_detect_warning=985ms；t_first_action=985ms；t_level_1=985ms |
| recovery timing | t_detect_warning=135ms；t_detect_stable=3255ms；t_first_action=3255ms；t_level_0=3255ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=985ms；t_first_action=985ms；t_level_1=985ms |
| raw recovery timing | t_detect_warning=135ms；t_detect_stable=3255ms；t_first_action=3255ms；t_level_0=3255ms |

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
| impairment timing | t_detect_warning=942ms；t_first_action=942ms；t_level_1=942ms |
| recovery timing | t_detect_warning=89ms；t_detect_stable=4249ms；t_first_action=4249ms；t_level_0=4249ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=942ms；t_first_action=942ms；t_level_1=942ms |
| raw recovery timing | t_detect_warning=89ms；t_detect_stable=4249ms；t_first_action=4249ms；t_level_0=4249ms |
