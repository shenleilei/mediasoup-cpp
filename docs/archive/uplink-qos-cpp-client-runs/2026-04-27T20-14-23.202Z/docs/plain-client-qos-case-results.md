# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-27T20:14:23.202Z`

## 1. 汇总

- 总 Case：`48`
- 已执行：`48`
- 通过：`47`
- 失败：`1`
- 错误：`0`
- runner：`cpp_client`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [O1](#o1) | `FAIL` | stateMatch=true, levelMatch=true, recoveryPassed=true, maxActionCountPassed=false, analysis=过强 |

## 2. 快速跳转

- 失败 / 错误：[O1](#o1)
- baseline：[B1](#b1)、[B2](#b2)、[B3](#b3)
- bw_sweep：[BW1](#bw1)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)
- loss_sweep：[L1](#l1)、[L2](#l2)、[L3](#l3)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)
- rtt_sweep：[R1](#r1)、[R2](#r2)、[R3](#r3)、[R4](#r4)、[R5](#r5)、[R6](#r6)
- jitter_sweep：[J1](#j1)、[J2](#j2)、[J3](#j3)、[J4](#j4)、[J5](#j5)
- transition：[T1](#t1)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)
- burst：[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)
- traffic_model：[M1](#m1)、[M2](#m2)、[M3](#m3)
- oscillation：[O1](#o1)、[O2](#o2)

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
| impairment timing | t_detect_stable=44ms |
| recovery timing | t_detect_stable=230ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=44ms |
| raw recovery timing | t_detect_stable=230ms |

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
| impairment timing | t_detect_stable=128ms |
| recovery timing | t_detect_stable=437ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=128ms |
| raw recovery timing | t_detect_stable=437ms |

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
| impairment timing | t_detect_warning=311ms |
| recovery timing | t_detect_warning=1016ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=311ms |
| raw recovery timing | t_detect_warning=1016ms |

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
| impairment timing | t_detect_stable=59ms |
| recovery timing | t_detect_stable=297ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=59ms |
| raw recovery timing | t_detect_stable=297ms |

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
| impairment timing | t_detect_warning=1087ms；t_detect_recovering=10220ms；t_detect_stable=61ms；t_detect_congested=2124ms；t_first_action=1087ms；t_level_1=1087ms；t_level_2=2124ms；t_level_3=3158ms；t_level_4=4181ms；t_audio_only=4181ms |
| recovery timing | t_detect_recovering=3327ms；t_detect_stable=4327ms；t_detect_congested=327ms；t_first_action=3327ms；t_level_0=12534ms；t_level_1=9413ms；t_level_2=6371ms；t_level_3=3327ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1087ms；t_detect_recovering=10220ms；t_detect_stable=61ms；t_detect_congested=2124ms；t_first_action=1087ms；t_level_1=1087ms；t_level_2=2124ms；t_level_3=3158ms；t_level_4=4181ms；t_audio_only=4181ms |
| raw recovery timing | t_detect_recovering=3327ms；t_detect_stable=4327ms；t_detect_congested=327ms；t_first_action=3327ms；t_level_0=12534ms；t_level_1=9413ms；t_level_2=6371ms；t_level_3=3327ms |

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
| impairment timing | t_detect_warning=1231ms；t_detect_recovering=10382ms；t_detect_stable=186ms；t_detect_congested=2273ms；t_first_action=1231ms；t_level_1=1231ms；t_level_2=2273ms；t_level_3=3308ms；t_level_4=4305ms；t_audio_only=4305ms |
| recovery timing | t_detect_recovering=3522ms；t_detect_stable=4522ms；t_detect_congested=483ms；t_first_action=3522ms；t_level_0=12728ms；t_level_1=9608ms；t_level_2=6569ms；t_level_3=3522ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1231ms；t_detect_recovering=10382ms；t_detect_stable=186ms；t_detect_congested=2273ms；t_first_action=1231ms；t_level_1=1231ms；t_level_2=2273ms；t_level_3=3308ms；t_level_4=4305ms；t_audio_only=4305ms |
| raw recovery timing | t_detect_recovering=3522ms；t_detect_stable=4522ms；t_detect_congested=483ms；t_first_action=3522ms；t_level_0=12728ms；t_level_1=9608ms；t_level_2=6569ms；t_level_3=3522ms |

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
| impairment timing | t_detect_warning=1132ms；t_detect_stable=118ms；t_detect_congested=2166ms；t_first_action=1132ms；t_level_1=1132ms；t_level_2=2166ms；t_level_3=3203ms；t_level_4=4239ms；t_audio_only=4239ms |
| recovery timing | t_detect_recovering=4498ms；t_detect_stable=5539ms；t_detect_congested=456ms；t_first_action=4498ms；t_level_0=13743ms；t_level_1=10633ms；t_level_2=7582ms；t_level_3=4498ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1132ms；t_detect_stable=118ms；t_detect_congested=2166ms；t_first_action=1132ms；t_level_1=1132ms；t_level_2=2166ms；t_level_3=3203ms；t_level_4=4239ms；t_audio_only=4239ms |
| raw recovery timing | t_detect_recovering=4498ms；t_detect_stable=5539ms；t_detect_congested=456ms；t_first_action=4498ms；t_level_0=13743ms；t_level_1=10633ms；t_level_2=7582ms；t_level_3=4498ms |

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
| impairment timing | t_detect_warning=1199ms；t_detect_stable=193ms；t_detect_congested=2238ms；t_first_action=1199ms；t_level_1=1199ms；t_level_2=2238ms；t_level_3=3279ms；t_level_4=4312ms；t_audio_only=4312ms |
| recovery timing | t_detect_recovering=4510ms；t_detect_stable=5509ms；t_detect_congested=430ms；t_first_action=4510ms；t_level_0=13727ms；t_level_1=10597ms；t_level_2=7557ms；t_level_3=4510ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1199ms；t_detect_stable=193ms；t_detect_congested=2238ms；t_first_action=1199ms；t_level_1=1199ms；t_level_2=2238ms；t_level_3=3279ms；t_level_4=4312ms；t_audio_only=4312ms |
| raw recovery timing | t_detect_recovering=4510ms；t_detect_stable=5509ms；t_detect_congested=430ms；t_first_action=4510ms；t_level_0=13727ms；t_level_1=10597ms；t_level_2=7557ms；t_level_3=4510ms |

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
| impairment timing | t_detect_warning=982ms；t_detect_congested=2022ms；t_first_action=982ms；t_level_1=982ms；t_level_2=2022ms；t_level_3=3073ms；t_level_4=4098ms；t_audio_only=4098ms |
| recovery timing | t_detect_recovering=4227ms；t_detect_stable=5227ms；t_detect_congested=227ms；t_first_action=4227ms；t_level_0=13440ms；t_level_1=10316ms；t_level_2=7273ms；t_level_3=4227ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=982ms；t_detect_congested=2022ms；t_first_action=982ms；t_level_1=982ms；t_level_2=2022ms；t_level_3=3073ms；t_level_4=4098ms；t_audio_only=4098ms |
| raw recovery timing | t_detect_recovering=4227ms；t_detect_stable=5227ms；t_detect_congested=227ms；t_first_action=4227ms；t_level_0=13440ms；t_level_1=10316ms；t_level_2=7273ms；t_level_3=4227ms |

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
| impairment timing | t_detect_stable=287ms |
| recovery timing | t_detect_stable=493ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=287ms |
| raw recovery timing | t_detect_stable=493ms |

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
| impairment timing | t_detect_stable=191ms |
| recovery timing | t_detect_stable=572ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=191ms |
| raw recovery timing | t_detect_stable=572ms |

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
| impairment timing | t_detect_stable=148ms |
| recovery timing | t_detect_stable=330ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=148ms |
| raw recovery timing | t_detect_stable=330ms |

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
| impairment timing | t_detect_warning=4203ms；t_detect_stable=155ms；t_first_action=4203ms；t_level_1=4203ms |
| recovery timing | t_detect_warning=616ms；t_detect_stable=3701ms；t_first_action=3701ms；t_level_0=3701ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4203ms；t_detect_stable=155ms；t_first_action=4203ms；t_level_1=4203ms |
| raw recovery timing | t_detect_warning=616ms；t_detect_stable=3701ms；t_first_action=3701ms；t_level_0=3701ms |

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
| impairment timing | t_detect_warning=2294ms；t_detect_recovering=16607ms；t_detect_stable=249ms；t_detect_congested=4385ms；t_first_action=2294ms；t_level_1=2294ms；t_level_2=4385ms；t_level_3=5422ms；t_level_4=6448ms；t_audio_only=6448ms |
| recovery timing | t_detect_stable=717ms；t_first_action=717ms；t_level_0=6879ms；t_level_1=3759ms；t_level_2=717ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2294ms；t_detect_recovering=16607ms；t_detect_stable=249ms；t_detect_congested=4385ms；t_first_action=2294ms；t_level_1=2294ms；t_level_2=4385ms；t_level_3=5422ms；t_level_4=6448ms；t_audio_only=6448ms |
| raw recovery timing | t_detect_stable=717ms；t_first_action=717ms；t_level_0=6879ms；t_level_1=3759ms；t_level_2=717ms |

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
| impairment timing | t_detect_warning=1299ms；t_detect_recovering=15573ms；t_detect_stable=258ms；t_detect_congested=2339ms；t_first_action=1299ms；t_level_1=1299ms；t_level_2=2339ms；t_level_3=3379ms；t_level_4=4414ms；t_audio_only=4414ms |
| recovery timing | t_detect_recovering=7721ms；t_detect_stable=8720ms；t_detect_congested=639ms；t_first_action=7721ms；t_level_0=16965ms；t_level_1=13854ms；t_level_2=10730ms；t_level_3=7721ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1299ms；t_detect_recovering=15573ms；t_detect_stable=258ms；t_detect_congested=2339ms；t_first_action=1299ms；t_level_1=1299ms；t_level_2=2339ms；t_level_3=3379ms；t_level_4=4414ms；t_audio_only=4414ms |
| raw recovery timing | t_detect_recovering=7721ms；t_detect_stable=8720ms；t_detect_congested=639ms；t_first_action=7721ms；t_level_0=16965ms；t_level_1=13854ms；t_level_2=10730ms；t_level_3=7721ms |

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
| impairment timing | t_detect_warning=1115ms；t_detect_stable=56ms；t_detect_congested=2137ms；t_first_action=1115ms；t_level_1=1115ms；t_level_2=2137ms；t_level_3=3178ms；t_level_4=4212ms；t_audio_only=4212ms |
| recovery timing | t_detect_recovering=4305ms；t_detect_stable=5305ms；t_detect_congested=305ms；t_first_action=4305ms；t_level_0=13442ms；t_level_1=10351ms；t_level_2=7310ms；t_level_3=4305ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1115ms；t_detect_stable=56ms；t_detect_congested=2137ms；t_first_action=1115ms；t_level_1=1115ms；t_level_2=2137ms；t_level_3=3178ms；t_level_4=4212ms；t_audio_only=4212ms |
| raw recovery timing | t_detect_recovering=4305ms；t_detect_stable=5305ms；t_detect_congested=305ms；t_first_action=4305ms；t_level_0=13442ms；t_level_1=10351ms；t_level_2=7310ms；t_level_3=4305ms |

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
| impairment timing | t_detect_warning=1337ms；t_detect_stable=301ms；t_detect_congested=2346ms；t_first_action=1337ms；t_level_1=1337ms；t_level_2=2346ms；t_level_3=3380ms；t_level_4=4416ms；t_audio_only=4416ms |
| recovery timing | t_detect_recovering=4718ms；t_detect_stable=5718ms；t_detect_congested=678ms；t_first_action=4718ms；t_level_0=13901ms；t_level_1=10806ms；t_level_2=7762ms；t_level_3=4718ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1337ms；t_detect_stable=301ms；t_detect_congested=2346ms；t_first_action=1337ms；t_level_1=1337ms；t_level_2=2346ms；t_level_3=3380ms；t_level_4=4416ms；t_audio_only=4416ms |
| raw recovery timing | t_detect_recovering=4718ms；t_detect_stable=5718ms；t_detect_congested=678ms；t_first_action=4718ms；t_level_0=13901ms；t_level_1=10806ms；t_level_2=7762ms；t_level_3=4718ms |

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
| impairment timing | t_detect_stable=220ms |
| recovery timing | t_detect_stable=526ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=220ms |
| raw recovery timing | t_detect_stable=526ms |

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
| impairment timing | t_detect_stable=255ms |
| recovery timing | t_detect_stable=557ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=255ms |
| raw recovery timing | t_detect_stable=557ms |

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
| impairment timing | t_detect_stable=288ms |
| recovery timing | t_detect_stable=544ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=288ms |
| raw recovery timing | t_detect_stable=544ms |

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
| impairment timing | t_detect_warning=4249ms；t_detect_stable=204ms；t_first_action=4249ms；t_level_1=4249ms |
| recovery timing | t_detect_warning=865ms；t_detect_stable=2959ms；t_first_action=2959ms；t_level_0=2959ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4249ms；t_detect_stable=204ms；t_first_action=4249ms；t_level_1=4249ms |
| raw recovery timing | t_detect_warning=865ms；t_detect_stable=2959ms；t_first_action=2959ms；t_level_0=2959ms |

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
| impairment timing | t_detect_warning=2373ms；t_detect_stable=292ms；t_detect_congested=6532ms；t_first_action=2373ms；t_level_1=2373ms；t_level_2=6532ms；t_level_3=7573ms；t_level_4=8607ms；t_audio_only=8607ms |
| recovery timing | t_detect_recovering=5644ms；t_detect_stable=6644ms；t_detect_congested=644ms；t_first_action=5644ms；t_level_0=14870ms；t_level_1=11732ms；t_level_2=8689ms；t_level_3=5644ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2373ms；t_detect_stable=292ms；t_detect_congested=6532ms；t_first_action=2373ms；t_level_1=2373ms；t_level_2=6532ms；t_level_3=7573ms；t_level_4=8607ms；t_audio_only=8607ms |
| raw recovery timing | t_detect_recovering=5644ms；t_detect_stable=6644ms；t_detect_congested=644ms；t_first_action=5644ms；t_level_0=14870ms；t_level_1=11732ms；t_level_2=8689ms；t_level_3=5644ms |

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
| impairment timing | t_detect_warning=2383ms；t_detect_stable=337ms；t_detect_congested=3418ms；t_first_action=2383ms；t_level_1=2383ms；t_level_2=3418ms；t_level_3=4457ms；t_level_4=5493ms；t_audio_only=5493ms |
| recovery timing | t_detect_recovering=5635ms；t_detect_stable=6635ms；t_detect_congested=595ms；t_first_action=5635ms；t_level_0=14841ms；t_level_1=11721ms；t_level_2=8681ms；t_level_3=5635ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2383ms；t_detect_stable=337ms；t_detect_congested=3418ms；t_first_action=2383ms；t_level_1=2383ms；t_level_2=3418ms；t_level_3=4457ms；t_level_4=5493ms；t_audio_only=5493ms |
| raw recovery timing | t_detect_recovering=5635ms；t_detect_stable=6635ms；t_detect_congested=595ms；t_first_action=5635ms；t_level_0=14841ms；t_level_1=11721ms；t_level_2=8681ms；t_level_3=5635ms |

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
| impairment timing | t_detect_stable=116ms |
| recovery timing | t_detect_stable=373ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=116ms |
| raw recovery timing | t_detect_stable=373ms |

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
| impairment timing | t_detect_stable=89ms |
| recovery timing | t_detect_stable=349ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=89ms |
| raw recovery timing | t_detect_stable=349ms |

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
| impairment timing | t_detect_warning=3243ms；t_detect_stable=197ms；t_first_action=3243ms；t_level_1=3243ms |
| recovery timing | t_detect_warning=825ms；t_detect_stable=2914ms；t_first_action=2914ms；t_level_0=2914ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3243ms；t_detect_stable=197ms；t_first_action=3243ms；t_level_1=3243ms |
| raw recovery timing | t_detect_warning=825ms；t_detect_stable=2914ms；t_first_action=2914ms；t_level_0=2914ms |

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
| impairment timing | t_detect_warning=2338ms；t_detect_stable=279ms；t_first_action=2338ms；t_level_1=2338ms |
| recovery timing | t_detect_warning=1012ms；t_detect_stable=5114ms；t_first_action=5114ms；t_level_0=5114ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2338ms；t_detect_stable=279ms；t_first_action=2338ms；t_level_1=2338ms |
| raw recovery timing | t_detect_warning=1012ms；t_detect_stable=5114ms；t_first_action=5114ms；t_level_0=5114ms |

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
| impairment timing | t_detect_warning=1169ms；t_detect_recovering=10319ms；t_detect_stable=160ms；t_detect_congested=2205ms；t_first_action=1169ms；t_level_0=19619ms；t_level_1=1169ms；t_level_2=2205ms；t_level_3=3249ms；t_level_4=4280ms；t_audio_only=4280ms |
| recovery timing | t_detect_stable=590ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1169ms；t_detect_recovering=10319ms；t_detect_stable=160ms；t_detect_congested=2205ms；t_first_action=1169ms；t_level_0=19619ms；t_level_1=1169ms；t_level_2=2205ms；t_level_3=3249ms；t_level_4=4280ms；t_audio_only=4280ms |
| raw recovery timing | t_detect_stable=590ms |

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
| impairment timing | t_detect_stable=966ms |
| recovery timing | t_detect_stable=189ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=966ms |
| raw recovery timing | t_detect_stable=189ms |

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
| impairment timing | t_detect_warning=1247ms；t_detect_recovering=10398ms；t_detect_stable=203ms；t_detect_congested=2285ms；t_first_action=1247ms；t_level_1=1247ms；t_level_2=2285ms；t_level_3=3338ms；t_level_4=4359ms；t_audio_only=4359ms |
| recovery timing | t_detect_recovering=3502ms；t_detect_stable=4502ms；t_detect_congested=502ms；t_first_action=3502ms；t_level_0=12707ms；t_level_1=9589ms；t_level_2=6548ms；t_level_3=3502ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1247ms；t_detect_recovering=10398ms；t_detect_stable=203ms；t_detect_congested=2285ms；t_first_action=1247ms；t_level_1=1247ms；t_level_2=2285ms；t_level_3=3338ms；t_level_4=4359ms；t_audio_only=4359ms |
| raw recovery timing | t_detect_recovering=3502ms；t_detect_stable=4502ms；t_detect_congested=502ms；t_first_action=3502ms；t_level_0=12707ms；t_level_1=9589ms；t_level_2=6548ms；t_level_3=3502ms |

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
| impairment timing | t_detect_warning=1376ms；t_detect_stable=335ms；t_detect_congested=2431ms；t_first_action=1376ms；t_level_1=1376ms；t_level_2=2431ms；t_level_3=3464ms；t_level_4=4491ms；t_audio_only=4491ms |
| recovery timing | t_detect_recovering=4616ms；t_detect_stable=5616ms；t_detect_congested=616ms；t_first_action=4616ms；t_level_0=13824ms；t_level_1=10702ms；t_level_2=7662ms；t_level_3=4616ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1376ms；t_detect_stable=335ms；t_detect_congested=2431ms；t_first_action=1376ms；t_level_1=1376ms；t_level_2=2431ms；t_level_3=3464ms；t_level_4=4491ms；t_audio_only=4491ms |
| raw recovery timing | t_detect_recovering=4616ms；t_detect_stable=5616ms；t_detect_congested=616ms；t_first_action=4616ms；t_level_0=13824ms；t_level_1=10702ms；t_level_2=7662ms；t_level_3=4616ms |

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
| impairment timing | t_detect_warning=4169ms；t_detect_stable=72ms；t_first_action=4169ms；t_level_1=4169ms |
| recovery timing | t_detect_warning=733ms；t_detect_stable=2778ms；t_first_action=2778ms；t_level_0=2778ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4169ms；t_detect_stable=72ms；t_first_action=4169ms；t_level_1=4169ms |
| raw recovery timing | t_detect_warning=733ms；t_detect_stable=2778ms；t_first_action=2778ms；t_level_0=2778ms |

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
| impairment timing | t_detect_warning=1120ms；t_detect_recovering=15309ms；t_detect_stable=110ms；t_detect_congested=2155ms；t_first_action=1120ms；t_level_1=1120ms；t_level_2=2155ms；t_level_3=3194ms；t_level_4=4230ms；t_audio_only=4230ms |
| recovery timing | t_detect_recovering=7572ms；t_detect_stable=8572ms；t_detect_congested=416ms；t_first_action=7572ms；t_level_0=16832ms；t_level_1=13699ms；t_level_2=10583ms；t_level_3=7572ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1120ms；t_detect_recovering=15309ms；t_detect_stable=110ms；t_detect_congested=2155ms；t_first_action=1120ms；t_level_1=1120ms；t_level_2=2155ms；t_level_3=3194ms；t_level_4=4230ms；t_audio_only=4230ms |
| raw recovery timing | t_detect_recovering=7572ms；t_detect_stable=8572ms；t_detect_congested=416ms；t_first_action=7572ms；t_level_0=16832ms；t_level_1=13699ms；t_level_2=10583ms；t_level_3=7572ms |

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
| impairment timing | t_detect_warning=4270ms；t_detect_stable=225ms；t_first_action=4270ms；t_level_1=4270ms |
| recovery timing | t_detect_warning=639ms；t_detect_stable=2689ms；t_first_action=2689ms；t_level_0=2689ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4270ms；t_detect_stable=225ms；t_first_action=4270ms；t_level_1=4270ms |
| raw recovery timing | t_detect_warning=639ms；t_detect_stable=2689ms；t_first_action=2689ms；t_level_0=2689ms |

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
| impairment timing | t_detect_warning=3270ms；t_detect_stable=184ms；t_first_action=3270ms；t_level_1=3270ms |
| recovery timing | t_detect_warning=900ms；t_detect_stable=2989ms；t_first_action=2989ms；t_level_0=2989ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3270ms；t_detect_stable=184ms；t_first_action=3270ms；t_level_1=3270ms |
| raw recovery timing | t_detect_warning=900ms；t_detect_stable=2989ms；t_first_action=2989ms；t_level_0=2989ms |

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
| impairment timing | t_detect_warning=1326ms；t_detect_stable=312ms；t_detect_congested=2368ms；t_first_action=1326ms；t_level_1=1326ms；t_level_2=2368ms；t_level_3=3397ms；t_level_4=4433ms；t_audio_only=4433ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1326ms；t_detect_stable=312ms；t_detect_congested=2368ms；t_first_action=1326ms；t_level_1=1326ms；t_level_2=2368ms；t_level_3=3397ms；t_level_4=4433ms；t_audio_only=4433ms |
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
| impairment timing | t_detect_warning=1177ms；t_detect_stable=170ms；t_detect_congested=2214ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2214ms；t_level_3=3254ms；t_level_4=4289ms；t_audio_only=4289ms |
| recovery timing | t_detect_recovering=7233ms；t_detect_stable=8233ms；t_detect_congested=233ms；t_first_action=7233ms；t_level_0=16458ms；t_level_1=13321ms；t_level_2=10279ms；t_level_3=7233ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1177ms；t_detect_stable=170ms；t_detect_congested=2214ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2214ms；t_level_3=3254ms；t_level_4=4289ms；t_audio_only=4289ms |
| raw recovery timing | t_detect_recovering=7233ms；t_detect_stable=8233ms；t_detect_congested=233ms；t_first_action=7233ms；t_level_0=16458ms；t_level_1=13321ms；t_level_2=10279ms；t_level_3=7233ms |

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
| impairment timing | t_detect_warning=1316ms；t_detect_stable=287ms；t_detect_congested=2336ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2336ms；t_level_3=3348ms；t_level_4=4385ms；t_audio_only=4385ms |
| recovery timing | t_detect_recovering=7634ms；t_detect_stable=8634ms；t_detect_congested=634ms；t_first_action=7634ms；t_level_0=16848ms；t_level_1=13722ms；t_level_2=10680ms；t_level_3=7634ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1316ms；t_detect_stable=287ms；t_detect_congested=2336ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2336ms；t_level_3=3348ms；t_level_4=4385ms；t_audio_only=4385ms |
| raw recovery timing | t_detect_recovering=7634ms；t_detect_stable=8634ms；t_detect_congested=634ms；t_first_action=7634ms；t_level_0=16848ms；t_level_1=13722ms；t_level_2=10680ms；t_level_3=7634ms |

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
| impairment timing | t_detect_warning=1173ms；t_detect_stable=147ms；t_detect_congested=2191ms；t_first_action=1173ms；t_level_1=1173ms；t_level_2=2191ms；t_level_3=3240ms；t_level_4=4270ms；t_audio_only=4270ms |
| recovery timing | t_detect_recovering=5848ms；t_detect_stable=6851ms；t_detect_congested=768ms；t_first_action=5848ms；t_level_0=15180ms；t_level_1=12056ms；t_level_2=8934ms；t_level_3=5848ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1173ms；t_detect_stable=147ms；t_detect_congested=2191ms；t_first_action=1173ms；t_level_1=1173ms；t_level_2=2191ms；t_level_3=3240ms；t_level_4=4270ms；t_audio_only=4270ms |
| raw recovery timing | t_detect_recovering=5848ms；t_detect_stable=6851ms；t_detect_congested=768ms；t_first_action=5848ms；t_level_0=15180ms；t_level_1=12056ms；t_level_2=8934ms；t_level_3=5848ms |

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
| impairment timing | t_detect_warning=2168ms；t_detect_stable=160ms；t_detect_congested=4244ms；t_first_action=2168ms；t_level_1=2168ms；t_level_2=4244ms |
| recovery timing | t_detect_recovering=7341ms；t_detect_stable=8342ms；t_detect_congested=267ms；t_first_action=267ms；t_level_0=16561ms；t_level_1=13429ms；t_level_2=10385ms；t_level_3=267ms；t_level_4=1302ms；t_audio_only=1302ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2168ms；t_detect_stable=160ms；t_detect_congested=4244ms；t_first_action=2168ms；t_level_1=2168ms；t_level_2=4244ms |
| raw recovery timing | t_detect_recovering=7341ms；t_detect_stable=8342ms；t_detect_congested=267ms；t_first_action=267ms；t_level_0=16561ms；t_level_1=13429ms；t_level_2=10385ms；t_level_3=267ms；t_level_4=1302ms；t_audio_only=1302ms |

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
| impairment timing | t_detect_warning=1222ms；t_detect_stable=205ms；t_detect_congested=2269ms；t_first_action=1222ms；t_level_1=1222ms；t_level_2=2269ms；t_level_3=3309ms；t_level_4=4329ms；t_audio_only=4329ms |
| recovery timing | t_detect_recovering=4322ms；t_detect_stable=5322ms；t_detect_congested=282ms；t_first_action=4322ms；t_level_0=13448ms；t_level_1=10428ms；t_level_2=7367ms；t_level_3=4322ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1222ms；t_detect_stable=205ms；t_detect_congested=2269ms；t_first_action=1222ms；t_level_1=1222ms；t_level_2=2269ms；t_level_3=3309ms；t_level_4=4329ms；t_audio_only=4329ms |
| raw recovery timing | t_detect_recovering=4322ms；t_detect_stable=5322ms；t_detect_congested=282ms；t_first_action=4322ms；t_level_0=13448ms；t_level_1=10428ms；t_level_2=7367ms；t_level_3=4322ms |

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
| impairment timing | t_detect_warning=3087ms；t_detect_stable=70ms；t_first_action=3087ms；t_level_1=3087ms |
| recovery timing | t_detect_warning=122ms；t_detect_stable=3252ms；t_first_action=3252ms；t_level_0=3252ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3087ms；t_detect_stable=70ms；t_first_action=3087ms；t_level_1=3087ms |
| raw recovery timing | t_detect_warning=122ms；t_detect_stable=3252ms；t_first_action=3252ms；t_level_0=3252ms |

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
| impairment timing | t_detect_warning=2207ms；t_detect_stable=202ms；t_first_action=2207ms；t_level_1=2207ms |
| recovery timing | t_detect_warning=304ms；t_detect_stable=4469ms；t_first_action=4469ms；t_level_0=4469ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2207ms；t_detect_stable=202ms；t_first_action=2207ms；t_level_1=2207ms |
| raw recovery timing | t_detect_warning=304ms；t_detect_stable=4469ms；t_first_action=4469ms；t_level_0=4469ms |

### M1

| 字段 | 内容 |
|---|---|
| Case ID | `M1` |
| 类型 | `traffic_model` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 3% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable；maxLevel=0 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=116ms |
| recovery timing | t_detect_stable=260ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=116ms |
| raw recovery timing | t_detect_stable=260ms |

### M2

| 字段 | 内容 |
|---|---|
| Case ID | `M2` |
| 类型 | `traffic_model` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；minLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=1018ms；t_detect_congested=2058ms；t_first_action=1018ms；t_level_1=1018ms；t_level_2=2058ms；t_level_3=3090ms；t_level_4=4130ms；t_audio_only=4130ms |
| recovery timing | t_detect_congested=214ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1018ms；t_detect_congested=2058ms；t_first_action=1018ms；t_level_1=1018ms；t_level_2=2058ms；t_level_3=3090ms；t_level_4=4130ms；t_audio_only=4130ms |
| raw recovery timing | t_detect_congested=214ms |

### M3

| 字段 | 内容 |
|---|---|
| Case ID | `M3` |
| 类型 | `traffic_model` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 400ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=1239ms；t_detect_stable=221ms；t_detect_congested=2285ms；t_first_action=1239ms；t_level_1=1239ms；t_level_2=2285ms；t_level_3=3314ms；t_level_4=4340ms；t_audio_only=4340ms |
| recovery timing | t_detect_congested=400ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1239ms；t_detect_stable=221ms；t_detect_congested=2285ms；t_first_action=1239ms；t_level_1=1239ms；t_level_2=2285ms；t_level_3=3314ms；t_level_4=4340ms；t_audio_only=4340ms |
| raw recovery timing | t_detect_congested=400ms |

### O1

| 字段 | 内容 |
|---|---|
| Case ID | `O1` |
| 类型 | `oscillation` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=stable/L2)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=true, levelMatch=true, recoveryPassed=true, maxActionCountPassed=false, analysis=过强） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1130ms；t_detect_recovering=10282ms；t_detect_stable=123ms；t_detect_congested=2170ms；t_first_action=1130ms；t_level_1=1130ms；t_level_2=2170ms；t_level_3=3207ms；t_level_4=4242ms；t_audio_only=4242ms |
| recovery timing | t_detect_stable=425ms；t_first_action=2443ms；t_level_0=5568ms；t_level_1=2443ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1130ms；t_detect_recovering=10282ms；t_detect_stable=123ms；t_detect_congested=2170ms；t_first_action=1130ms；t_level_1=1130ms；t_level_2=2170ms；t_level_3=3207ms；t_level_4=4242ms；t_audio_only=4242ms |
| raw recovery timing | t_detect_stable=425ms；t_first_action=2443ms；t_level_0=5568ms；t_level_1=2443ms |

### O2

| 字段 | 内容 |
|---|---|
| Case ID | `O2` |
| 类型 | `oscillation` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 150ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=stable/L2)；recovery(best=stable/L2, current=stable/L2) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 6 次非 noop） |
| impairment timing | t_detect_warning=1029ms；t_detect_recovering=10167ms；t_detect_stable=11ms；t_detect_congested=2064ms；t_first_action=1029ms；t_level_1=1029ms；t_level_2=2064ms；t_level_3=3093ms；t_level_4=4090ms；t_audio_only=4090ms |
| recovery timing | t_detect_stable=177ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1029ms；t_detect_recovering=10167ms；t_detect_stable=11ms；t_detect_congested=2064ms；t_first_action=1029ms；t_level_1=1029ms；t_level_2=2064ms；t_level_3=3093ms；t_level_4=4090ms；t_audio_only=4090ms |
| raw recovery timing | t_detect_stable=177ms |

