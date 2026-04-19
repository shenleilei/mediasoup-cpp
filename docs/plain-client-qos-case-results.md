# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-19T06:12:41.709Z`

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
| impairment timing | t_detect_stable=972ms |
| recovery timing | t_detect_stable=961ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=972ms |
| raw recovery timing | t_detect_stable=961ms |

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
| impairment timing | t_detect_stable=75ms |
| recovery timing | t_detect_stable=102ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=75ms |
| raw recovery timing | t_detect_stable=102ms |

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
| impairment timing | t_detect_warning=395ms |
| recovery timing | t_detect_warning=143ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=395ms |
| raw recovery timing | t_detect_warning=143ms |

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
| impairment timing | t_detect_stable=958ms |
| recovery timing | t_detect_stable=947ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=958ms |
| raw recovery timing | t_detect_stable=947ms |

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
| impairment timing | t_detect_warning=1157ms；t_detect_recovering=10351ms；t_detect_stable=112ms；t_detect_congested=2196ms；t_first_action=1157ms；t_level_1=1157ms；t_level_2=2196ms；t_level_3=3236ms；t_level_4=4272ms；t_audio_only=4272ms |
| recovery timing | t_detect_recovering=3540ms；t_detect_stable=4540ms；t_detect_congested=540ms；t_first_action=3540ms；t_level_0=12746ms；t_level_1=9625ms；t_level_2=6584ms；t_level_3=3540ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1157ms；t_detect_recovering=10351ms；t_detect_stable=112ms；t_detect_congested=2196ms；t_first_action=1157ms；t_level_1=1157ms；t_level_2=2196ms；t_level_3=3236ms；t_level_4=4272ms；t_audio_only=4272ms |
| raw recovery timing | t_detect_recovering=3540ms；t_detect_stable=4540ms；t_detect_congested=540ms；t_first_action=3540ms；t_level_0=12746ms；t_level_1=9625ms；t_level_2=6584ms；t_level_3=3540ms |

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
| impairment timing | t_detect_warning=959ms；t_detect_recovering=10113ms；t_detect_stable=11113ms；t_detect_congested=1998ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1998ms；t_level_3=3040ms；t_level_4=4074ms；t_audio_only=4074ms |
| recovery timing | t_detect_recovering=3220ms；t_detect_stable=4220ms；t_detect_congested=220ms；t_first_action=3220ms；t_level_0=12425ms；t_level_1=9305ms；t_level_2=6264ms；t_level_3=3220ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=959ms；t_detect_recovering=10113ms；t_detect_stable=11113ms；t_detect_congested=1998ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1998ms；t_level_3=3040ms；t_level_4=4074ms；t_audio_only=4074ms |
| raw recovery timing | t_detect_recovering=3220ms；t_detect_stable=4220ms；t_detect_congested=220ms；t_first_action=3220ms；t_level_0=12425ms；t_level_1=9305ms；t_level_2=6264ms；t_level_3=3220ms |

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
| impairment timing | t_detect_warning=957ms；t_detect_congested=1997ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1997ms；t_level_3=3037ms；t_level_4=4073ms；t_audio_only=4073ms |
| recovery timing | t_detect_recovering=4100ms；t_detect_stable=5100ms；t_detect_congested=100ms；t_first_action=4100ms；t_level_0=13305ms；t_level_1=10185ms；t_level_2=7144ms；t_level_3=4100ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=957ms；t_detect_congested=1997ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1997ms；t_level_3=3037ms；t_level_4=4073ms；t_audio_only=4073ms |
| raw recovery timing | t_detect_recovering=4100ms；t_detect_stable=5100ms；t_detect_congested=100ms；t_first_action=4100ms；t_level_0=13305ms；t_level_1=10185ms；t_level_2=7144ms；t_level_3=4100ms |

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
| impairment timing | t_detect_warning=959ms；t_detect_congested=1998ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1998ms；t_level_3=3038ms；t_level_4=4074ms；t_audio_only=4074ms |
| recovery timing | t_detect_recovering=4101ms；t_detect_stable=5101ms；t_detect_congested=101ms；t_first_action=4101ms；t_level_0=13306ms；t_level_1=10186ms；t_level_2=7145ms；t_level_3=4101ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=959ms；t_detect_congested=1998ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1998ms；t_level_3=3038ms；t_level_4=4074ms；t_audio_only=4074ms |
| raw recovery timing | t_detect_recovering=4101ms；t_detect_stable=5101ms；t_detect_congested=101ms；t_first_action=4101ms；t_level_0=13306ms；t_level_1=10186ms；t_level_2=7145ms；t_level_3=4101ms |

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
| impairment timing | t_detect_warning=1118ms；t_detect_stable=114ms；t_detect_congested=2157ms；t_first_action=1118ms；t_level_1=1118ms；t_level_2=2157ms；t_level_3=3197ms；t_level_4=4233ms；t_audio_only=4233ms |
| recovery timing | t_detect_recovering=4300ms；t_detect_stable=5300ms；t_detect_congested=300ms；t_first_action=4300ms；t_level_0=13505ms；t_level_1=10385ms；t_level_2=7344ms；t_level_3=4300ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1118ms；t_detect_stable=114ms；t_detect_congested=2157ms；t_first_action=1118ms；t_level_1=1118ms；t_level_2=2157ms；t_level_3=3197ms；t_level_4=4233ms；t_audio_only=4233ms |
| raw recovery timing | t_detect_recovering=4300ms；t_detect_stable=5300ms；t_detect_congested=300ms；t_first_action=4300ms；t_level_0=13505ms；t_level_1=10385ms；t_level_2=7344ms；t_level_3=4300ms |

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
| impairment timing | t_detect_stable=987ms |
| recovery timing | t_detect_stable=976ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=987ms |
| raw recovery timing | t_detect_stable=976ms |

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
| impairment timing | t_detect_stable=954ms |
| recovery timing | t_detect_stable=982ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=954ms |
| raw recovery timing | t_detect_stable=982ms |

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
| impairment timing | t_detect_stable=957ms |
| recovery timing | t_detect_stable=986ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=957ms |
| raw recovery timing | t_detect_stable=986ms |

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
| impairment timing | t_detect_warning=3996ms；t_detect_stable=992ms；t_first_action=3996ms；t_level_1=3996ms |
| recovery timing | t_detect_warning=620ms；t_detect_stable=3744ms；t_first_action=3744ms；t_level_0=3744ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3996ms；t_detect_stable=992ms；t_first_action=3996ms；t_level_1=3996ms |
| raw recovery timing | t_detect_warning=620ms；t_detect_stable=3744ms；t_first_action=3744ms；t_level_0=3744ms |

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
| impairment timing | t_detect_warning=1957ms；t_detect_recovering=16111ms；t_detect_stable=952ms；t_detect_congested=4035ms；t_first_action=1957ms；t_level_1=1957ms；t_level_2=4035ms；t_level_3=5075ms；t_level_4=6111ms；t_audio_only=6111ms |
| recovery timing | t_detect_warning=99ms；t_detect_stable=3102ms；t_first_action=3102ms；t_level_0=9264ms；t_level_1=6143ms；t_level_2=3102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1957ms；t_detect_recovering=16111ms；t_detect_stable=952ms；t_detect_congested=4035ms；t_first_action=1957ms；t_level_1=1957ms；t_level_2=4035ms；t_level_3=5075ms；t_level_4=6111ms；t_audio_only=6111ms |
| raw recovery timing | t_detect_warning=99ms；t_detect_stable=3102ms；t_first_action=3102ms；t_level_0=9264ms；t_level_1=6143ms；t_level_2=3102ms |

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
| impairment timing | t_detect_warning=957ms；t_detect_recovering=14072ms；t_detect_stable=15072ms；t_detect_congested=1996ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1996ms；t_level_3=3036ms；t_level_4=4072ms；t_audio_only=4072ms |
| recovery timing | t_detect_recovering=7061ms；t_detect_stable=8061ms；t_detect_congested=61ms；t_first_action=7061ms；t_level_0=16266ms；t_level_1=13145ms；t_level_2=10117ms；t_level_3=7061ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=957ms；t_detect_recovering=14072ms；t_detect_stable=15072ms；t_detect_congested=1996ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1996ms；t_level_3=3036ms；t_level_4=4072ms；t_audio_only=4072ms |
| raw recovery timing | t_detect_recovering=7061ms；t_detect_stable=8061ms；t_detect_congested=61ms；t_first_action=7061ms；t_level_0=16266ms；t_level_1=13145ms；t_level_2=10117ms；t_level_3=7061ms |

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
| impairment timing | t_detect_warning=1036ms；t_detect_stable=32ms；t_detect_congested=2075ms；t_first_action=1036ms；t_level_1=1036ms；t_level_2=2075ms；t_level_3=3115ms；t_level_4=4151ms；t_audio_only=4151ms |
| recovery timing | t_detect_recovering=4179ms；t_detect_stable=5179ms；t_detect_congested=179ms；t_first_action=4179ms；t_level_0=13384ms；t_level_1=10264ms；t_level_2=7223ms；t_level_3=4179ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1036ms；t_detect_stable=32ms；t_detect_congested=2075ms；t_first_action=1036ms；t_level_1=1036ms；t_level_2=2075ms；t_level_3=3115ms；t_level_4=4151ms；t_audio_only=4151ms |
| raw recovery timing | t_detect_recovering=4179ms；t_detect_stable=5179ms；t_detect_congested=179ms；t_first_action=4179ms；t_level_0=13384ms；t_level_1=10264ms；t_level_2=7223ms；t_level_3=4179ms |

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
| impairment timing | t_detect_warning=957ms；t_detect_congested=1996ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1996ms；t_level_3=3035ms；t_level_4=4072ms；t_audio_only=4072ms |
| recovery timing | t_detect_recovering=4099ms；t_detect_stable=5099ms；t_detect_congested=99ms；t_first_action=4099ms；t_level_0=13304ms；t_level_1=10184ms；t_level_2=7143ms；t_level_3=4099ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=957ms；t_detect_congested=1996ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1996ms；t_level_3=3035ms；t_level_4=4072ms；t_audio_only=4072ms |
| raw recovery timing | t_detect_recovering=4099ms；t_detect_stable=5099ms；t_detect_congested=99ms；t_first_action=4099ms；t_level_0=13304ms；t_level_1=10184ms；t_level_2=7143ms；t_level_3=4099ms |

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
| impairment timing | t_detect_stable=1032ms |
| recovery timing | t_detect_stable=20ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=1032ms |
| raw recovery timing | t_detect_stable=20ms |

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
| impairment timing | t_detect_stable=952ms |
| recovery timing | t_detect_stable=941ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=952ms |
| raw recovery timing | t_detect_stable=941ms |

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
| impairment timing | t_detect_stable=72ms |
| recovery timing | t_detect_stable=60ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=72ms |
| raw recovery timing | t_detect_stable=60ms |

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
| impairment timing | t_detect_warning=3955ms；t_detect_stable=951ms；t_first_action=3955ms；t_level_1=3955ms |
| recovery timing | t_detect_warning=580ms；t_detect_stable=3704ms；t_first_action=3704ms；t_level_0=3704ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3955ms；t_detect_stable=951ms；t_first_action=3955ms；t_level_1=3955ms |
| raw recovery timing | t_detect_warning=580ms；t_detect_stable=3704ms；t_first_action=3704ms；t_level_0=3704ms |

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
| impairment timing | t_detect_warning=2039ms；t_detect_stable=1034ms；t_detect_congested=6198ms；t_first_action=2039ms；t_level_1=2039ms；t_level_2=6198ms；t_level_3=7238ms；t_level_4=8274ms；t_audio_only=8274ms |
| recovery timing | t_detect_recovering=5302ms；t_detect_stable=6302ms；t_detect_congested=302ms；t_first_action=5302ms；t_level_0=14514ms；t_level_1=11387ms；t_level_2=8346ms；t_level_3=5302ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2039ms；t_detect_stable=1034ms；t_detect_congested=6198ms；t_first_action=2039ms；t_level_1=2039ms；t_level_2=6198ms；t_level_3=7238ms；t_level_4=8274ms；t_audio_only=8274ms |
| raw recovery timing | t_detect_recovering=5302ms；t_detect_stable=6302ms；t_detect_congested=302ms；t_first_action=5302ms；t_level_0=14514ms；t_level_1=11387ms；t_level_2=8346ms；t_level_3=5302ms |

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
| impairment timing | t_detect_warning=1958ms；t_detect_stable=954ms；t_detect_congested=2998ms；t_first_action=1958ms；t_level_1=1958ms；t_level_2=2998ms；t_level_3=4037ms；t_level_4=5074ms；t_audio_only=5074ms |
| recovery timing | t_detect_recovering=6102ms；t_detect_stable=7102ms；t_detect_congested=102ms；t_first_action=6102ms；t_level_0=15307ms；t_level_1=12187ms；t_level_2=9146ms；t_level_3=6102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1958ms；t_detect_stable=954ms；t_detect_congested=2998ms；t_first_action=1958ms；t_level_1=1958ms；t_level_2=2998ms；t_level_3=4037ms；t_level_4=5074ms；t_audio_only=5074ms |
| raw recovery timing | t_detect_recovering=6102ms；t_detect_stable=7102ms；t_detect_congested=102ms；t_first_action=6102ms；t_level_0=15307ms；t_level_1=12187ms；t_level_2=9146ms；t_level_3=6102ms |

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
| impairment timing | t_detect_stable=953ms |
| recovery timing | t_detect_stable=942ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=953ms |
| raw recovery timing | t_detect_stable=942ms |

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
| impairment timing | t_detect_stable=191ms |
| recovery timing | t_detect_stable=258ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=191ms |
| raw recovery timing | t_detect_stable=258ms |

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
| impairment timing | t_detect_warning=2958ms；t_detect_stable=953ms；t_first_action=2958ms；t_level_1=2958ms |
| recovery timing | t_detect_warning=622ms；t_detect_stable=3746ms；t_first_action=3746ms；t_level_0=3746ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2958ms；t_detect_stable=953ms；t_first_action=2958ms；t_level_1=2958ms |
| raw recovery timing | t_detect_warning=622ms；t_detect_stable=3746ms；t_first_action=3746ms；t_level_0=3746ms |

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
| impairment timing | t_detect_warning=1961ms；t_detect_stable=957ms；t_first_action=1961ms；t_level_1=1961ms |
| recovery timing | t_detect_warning=665ms；t_detect_stable=5868ms；t_first_action=5868ms；t_level_0=5868ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1961ms；t_detect_stable=957ms；t_first_action=1961ms；t_level_1=1961ms |
| raw recovery timing | t_detect_warning=665ms；t_detect_stable=5868ms；t_first_action=5868ms；t_level_0=5868ms |

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
| impairment timing | t_detect_warning=1076ms；t_detect_recovering=10230ms；t_detect_stable=72ms；t_detect_congested=2115ms；t_first_action=1076ms；t_level_0=19515ms；t_level_1=1076ms；t_level_2=2115ms；t_level_3=3155ms；t_level_4=4191ms；t_audio_only=4191ms |
| recovery timing | t_detect_stable=499ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1076ms；t_detect_recovering=10230ms；t_detect_stable=72ms；t_detect_congested=2115ms；t_first_action=1076ms；t_level_0=19515ms；t_level_1=1076ms；t_level_2=2115ms；t_level_3=3155ms；t_level_4=4191ms；t_audio_only=4191ms |
| raw recovery timing | t_detect_stable=499ms |

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
| impairment timing | t_detect_stable=954ms |
| recovery timing | t_detect_stable=943ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=954ms |
| raw recovery timing | t_detect_stable=943ms |

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
| impairment timing | t_detect_warning=1037ms；t_detect_recovering=10192ms；t_detect_stable=11192ms；t_detect_congested=2077ms；t_first_action=1037ms；t_level_1=1037ms；t_level_2=2077ms；t_level_3=3116ms；t_level_4=4152ms；t_audio_only=4152ms |
| recovery timing | t_detect_recovering=3301ms；t_detect_stable=4301ms；t_detect_congested=301ms；t_first_action=3301ms；t_level_0=12506ms；t_level_1=9386ms；t_level_2=6344ms；t_level_3=3301ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1037ms；t_detect_recovering=10192ms；t_detect_stable=11192ms；t_detect_congested=2077ms；t_first_action=1037ms；t_level_1=1037ms；t_level_2=2077ms；t_level_3=3116ms；t_level_4=4152ms；t_audio_only=4152ms |
| raw recovery timing | t_detect_recovering=3301ms；t_detect_stable=4301ms；t_detect_congested=301ms；t_first_action=3301ms；t_level_0=12506ms；t_level_1=9386ms；t_level_2=6344ms；t_level_3=3301ms |

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
| impairment timing | t_detect_warning=967ms；t_detect_congested=2006ms；t_first_action=967ms；t_level_1=967ms；t_level_2=2006ms；t_level_3=3046ms；t_level_4=4082ms；t_audio_only=4082ms |
| recovery timing | t_detect_recovering=4110ms；t_detect_stable=5110ms；t_detect_congested=110ms；t_first_action=4110ms；t_level_0=13315ms；t_level_1=10195ms；t_level_2=7154ms；t_level_3=4110ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=967ms；t_detect_congested=2006ms；t_first_action=967ms；t_level_1=967ms；t_level_2=2006ms；t_level_3=3046ms；t_level_4=4082ms；t_audio_only=4082ms |
| raw recovery timing | t_detect_recovering=4110ms；t_detect_stable=5110ms；t_detect_congested=110ms；t_first_action=4110ms；t_level_0=13315ms；t_level_1=10195ms；t_level_2=7154ms；t_level_3=4110ms |

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
| impairment timing | t_detect_warning=3957ms；t_detect_stable=953ms；t_first_action=3957ms；t_level_1=3957ms |
| recovery timing | t_detect_warning=582ms；t_detect_stable=3706ms；t_first_action=3706ms；t_level_0=3706ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3957ms；t_detect_stable=953ms；t_first_action=3957ms；t_level_1=3957ms |
| raw recovery timing | t_detect_warning=582ms；t_detect_stable=3706ms；t_first_action=3706ms；t_level_0=3706ms |

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
| impairment timing | t_detect_warning=1076ms；t_detect_recovering=15270ms；t_detect_stable=72ms；t_detect_congested=2115ms；t_first_action=1076ms；t_level_1=1076ms；t_level_2=2115ms；t_level_3=3155ms；t_level_4=4191ms；t_audio_only=4191ms |
| recovery timing | t_detect_recovering=7418ms；t_detect_stable=8418ms；t_detect_congested=338ms；t_first_action=7418ms；t_level_0=16623ms；t_level_1=13503ms；t_level_2=10462ms；t_level_3=7418ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1076ms；t_detect_recovering=15270ms；t_detect_stable=72ms；t_detect_congested=2115ms；t_first_action=1076ms；t_level_1=1076ms；t_level_2=2115ms；t_level_3=3155ms；t_level_4=4191ms；t_audio_only=4191ms |
| raw recovery timing | t_detect_recovering=7418ms；t_detect_stable=8418ms；t_detect_congested=338ms；t_first_action=7418ms；t_level_0=16623ms；t_level_1=13503ms；t_level_2=10462ms；t_level_3=7418ms |

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
| impairment timing | t_detect_warning=3956ms；t_detect_stable=951ms；t_first_action=3956ms；t_level_1=3956ms |
| recovery timing | t_detect_warning=581ms；t_detect_stable=3705ms；t_first_action=3705ms；t_level_0=3705ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3956ms；t_detect_stable=951ms；t_first_action=3956ms；t_level_1=3956ms |
| raw recovery timing | t_detect_warning=581ms；t_detect_stable=3705ms；t_first_action=3705ms；t_level_0=3705ms |

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
| impairment timing | t_detect_warning=3078ms；t_detect_stable=33ms；t_first_action=3078ms；t_level_1=3078ms |
| recovery timing | t_detect_warning=742ms；t_detect_stable=2827ms；t_first_action=2827ms；t_level_0=2827ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3078ms；t_detect_stable=33ms；t_first_action=3078ms；t_level_1=3078ms |
| raw recovery timing | t_detect_warning=742ms；t_detect_stable=2827ms；t_first_action=2827ms；t_level_0=2827ms |

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
| impairment timing | t_detect_warning=956ms；t_detect_congested=1995ms；t_first_action=956ms；t_level_1=956ms；t_level_2=1995ms；t_level_3=3034ms；t_level_4=4071ms；t_audio_only=4071ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=956ms；t_detect_congested=1995ms；t_first_action=956ms；t_level_1=956ms；t_level_2=1995ms；t_level_3=3034ms；t_level_4=4071ms；t_audio_only=4071ms |
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
| impairment timing | t_detect_warning=959ms；t_detect_congested=1999ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1999ms；t_level_3=3038ms；t_level_4=4074ms；t_audio_only=4074ms |
| recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=102ms；t_first_action=7102ms；t_level_0=16307ms；t_level_1=13187ms；t_level_2=10146ms；t_level_3=7102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=959ms；t_detect_congested=1999ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1999ms；t_level_3=3038ms；t_level_4=4074ms；t_audio_only=4074ms |
| raw recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=102ms；t_first_action=7102ms；t_level_0=16307ms；t_level_1=13187ms；t_level_2=10146ms；t_level_3=7102ms |

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
| impairment timing | t_detect_warning=958ms；t_detect_congested=1998ms；t_first_action=958ms；t_level_1=958ms；t_level_2=1998ms；t_level_3=3038ms；t_level_4=4073ms；t_audio_only=4073ms |
| recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=102ms；t_first_action=7102ms；t_level_0=16307ms；t_level_1=13187ms；t_level_2=10146ms；t_level_3=7102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=958ms；t_detect_congested=1998ms；t_first_action=958ms；t_level_1=958ms；t_level_2=1998ms；t_level_3=3038ms；t_level_4=4073ms；t_audio_only=4073ms |
| raw recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=102ms；t_first_action=7102ms；t_level_0=16307ms；t_level_1=13187ms；t_level_2=10146ms；t_level_3=7102ms |

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
| impairment timing | t_detect_warning=958ms；t_detect_congested=1997ms；t_first_action=958ms；t_level_1=958ms；t_level_2=1997ms；t_level_3=3037ms；t_level_4=4073ms；t_audio_only=4073ms |
| recovery timing | t_detect_recovering=6261ms；t_detect_stable=7261ms；t_detect_congested=261ms；t_first_action=6261ms；t_level_0=15467ms；t_level_1=12346ms；t_level_2=9305ms；t_level_3=6261ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=958ms；t_detect_congested=1997ms；t_first_action=958ms；t_level_1=958ms；t_level_2=1997ms；t_level_3=3037ms；t_level_4=4073ms；t_audio_only=4073ms |
| raw recovery timing | t_detect_recovering=6261ms；t_detect_stable=7261ms；t_detect_congested=261ms；t_first_action=6261ms；t_level_0=15467ms；t_level_1=12346ms；t_level_2=9305ms；t_level_3=6261ms |

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
| impairment timing | t_detect_warning=1959ms；t_detect_stable=955ms；t_detect_congested=4038ms；t_first_action=1959ms；t_level_1=1959ms；t_level_2=4038ms |
| recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=66ms；t_first_action=66ms；t_level_0=16267ms；t_level_1=13146ms；t_level_2=10105ms；t_level_3=66ms；t_level_4=1102ms；t_audio_only=1102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1959ms；t_detect_stable=955ms；t_detect_congested=4038ms；t_first_action=1959ms；t_level_1=1959ms；t_level_2=4038ms |
| raw recovery timing | t_detect_recovering=7102ms；t_detect_stable=8102ms；t_detect_congested=66ms；t_first_action=66ms；t_level_0=16267ms；t_level_1=13146ms；t_level_2=10105ms；t_level_3=66ms；t_level_4=1102ms；t_audio_only=1102ms |

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
| impairment timing | t_detect_warning=959ms；t_detect_congested=1999ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1999ms；t_level_3=3039ms；t_level_4=4075ms；t_audio_only=4075ms |
| recovery timing | t_detect_recovering=4102ms；t_detect_stable=5102ms；t_detect_congested=102ms；t_first_action=4102ms；t_level_0=13307ms；t_level_1=10187ms；t_level_2=7146ms；t_level_3=4102ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=959ms；t_detect_congested=1999ms；t_first_action=959ms；t_level_1=959ms；t_level_2=1999ms；t_level_3=3039ms；t_level_4=4075ms；t_audio_only=4075ms |
| raw recovery timing | t_detect_recovering=4102ms；t_detect_stable=5102ms；t_detect_congested=102ms；t_first_action=4102ms；t_level_0=13307ms；t_level_1=10187ms；t_level_2=7146ms；t_level_3=4102ms |

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
| impairment timing | t_detect_warning=3037ms；t_detect_stable=1032ms；t_first_action=3037ms；t_level_1=3037ms |
| recovery timing | t_detect_warning=101ms；t_detect_stable=3225ms；t_first_action=3225ms；t_level_0=3225ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3037ms；t_detect_stable=1032ms；t_first_action=3037ms；t_level_1=3037ms |
| raw recovery timing | t_detect_warning=101ms；t_detect_stable=3225ms；t_first_action=3225ms；t_level_0=3225ms |

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
| impairment timing | t_detect_warning=1957ms；t_detect_stable=952ms；t_first_action=1957ms；t_level_1=1957ms |
| recovery timing | t_detect_warning=61ms；t_detect_stable=4225ms；t_first_action=4225ms；t_level_0=4225ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1957ms；t_detect_stable=952ms；t_first_action=1957ms；t_level_1=1957ms |
| raw recovery timing | t_detect_warning=61ms；t_detect_stable=4225ms；t_first_action=4225ms；t_level_0=4225ms |

