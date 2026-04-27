# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-27T10:07:36.425Z`

## 1. 汇总

- 总 Case：`48`
- 已执行：`48`
- 通过：`48`
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
| impairment timing | t_detect_stable=316ms |
| recovery timing | t_detect_stable=581ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=316ms |
| raw recovery timing | t_detect_stable=581ms |

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
| impairment timing | t_detect_stable=200ms |
| recovery timing | t_detect_stable=345ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=200ms |
| raw recovery timing | t_detect_stable=345ms |

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
| impairment timing | t_detect_warning=434ms |
| recovery timing | t_detect_warning=112ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=434ms |
| raw recovery timing | t_detect_warning=112ms |

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
| impairment timing | t_detect_stable=239ms |
| recovery timing | t_detect_stable=543ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=239ms |
| raw recovery timing | t_detect_stable=543ms |

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
| impairment timing | t_detect_warning=1106ms；t_detect_recovering=10259ms；t_detect_stable=100ms；t_detect_congested=2145ms；t_first_action=1106ms；t_level_1=1106ms；t_level_2=2145ms；t_level_3=3188ms；t_level_4=4220ms；t_audio_only=4220ms |
| recovery timing | t_detect_recovering=3402ms；t_detect_stable=4403ms；t_detect_congested=362ms；t_first_action=3402ms；t_level_0=12608ms；t_level_1=9488ms；t_level_2=6446ms；t_level_3=3402ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1106ms；t_detect_recovering=10259ms；t_detect_stable=100ms；t_detect_congested=2145ms；t_first_action=1106ms；t_level_1=1106ms；t_level_2=2145ms；t_level_3=3188ms；t_level_4=4220ms；t_audio_only=4220ms |
| raw recovery timing | t_detect_recovering=3402ms；t_detect_stable=4403ms；t_detect_congested=362ms；t_first_action=3402ms；t_level_0=12608ms；t_level_1=9488ms；t_level_2=6446ms；t_level_3=3402ms |

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
| impairment timing | t_detect_warning=1058ms；t_detect_recovering=10212ms；t_detect_stable=53ms；t_detect_congested=2098ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2098ms；t_level_3=3138ms；t_level_4=4172ms；t_audio_only=4172ms |
| recovery timing | t_detect_recovering=3436ms；t_detect_stable=4442ms；t_detect_congested=435ms；t_first_action=3436ms；t_level_0=12642ms；t_level_1=9521ms；t_level_2=6480ms；t_level_3=3436ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1058ms；t_detect_recovering=10212ms；t_detect_stable=53ms；t_detect_congested=2098ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2098ms；t_level_3=3138ms；t_level_4=4172ms；t_audio_only=4172ms |
| raw recovery timing | t_detect_recovering=3436ms；t_detect_stable=4442ms；t_detect_congested=435ms；t_first_action=3436ms；t_level_0=12642ms；t_level_1=9521ms；t_level_2=6480ms；t_level_3=3436ms |

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
| impairment timing | t_detect_warning=1114ms；t_detect_stable=108ms；t_detect_congested=2152ms；t_first_action=1114ms；t_level_1=1114ms；t_level_2=2152ms；t_level_3=3198ms；t_level_4=4227ms；t_audio_only=4227ms |
| recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13537ms；t_level_1=10423ms；t_level_2=7338ms；t_level_3=4328ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1114ms；t_detect_stable=108ms；t_detect_congested=2152ms；t_first_action=1114ms；t_level_1=1114ms；t_level_2=2152ms；t_level_3=3198ms；t_level_4=4227ms；t_audio_only=4227ms |
| raw recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13537ms；t_level_1=10423ms；t_level_2=7338ms；t_level_3=4328ms |

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
| impairment timing | t_detect_warning=1232ms；t_detect_stable=187ms；t_detect_congested=2271ms；t_first_action=1232ms；t_level_1=1232ms；t_level_2=2271ms；t_level_3=3283ms；t_level_4=4305ms；t_audio_only=4305ms |
| recovery timing | t_detect_recovering=4371ms；t_detect_stable=5371ms；t_detect_congested=331ms；t_first_action=4371ms；t_level_0=13577ms；t_level_1=10457ms；t_level_2=7416ms；t_level_3=4371ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1232ms；t_detect_stable=187ms；t_detect_congested=2271ms；t_first_action=1232ms；t_level_1=1232ms；t_level_2=2271ms；t_level_3=3283ms；t_level_4=4305ms；t_audio_only=4305ms |
| raw recovery timing | t_detect_recovering=4371ms；t_detect_stable=5371ms；t_detect_congested=331ms；t_first_action=4371ms；t_level_0=13577ms；t_level_1=10457ms；t_level_2=7416ms；t_level_3=4371ms |

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
| impairment timing | t_detect_warning=1215ms；t_detect_stable=174ms；t_detect_congested=2254ms；t_first_action=1215ms；t_level_1=1215ms；t_level_2=2254ms；t_level_3=3302ms；t_level_4=4329ms；t_audio_only=4329ms |
| recovery timing | t_detect_recovering=4474ms；t_detect_stable=5513ms；t_detect_congested=432ms；t_first_action=4474ms；t_level_0=13798ms；t_level_1=10680ms；t_level_2=7558ms；t_level_3=4474ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1215ms；t_detect_stable=174ms；t_detect_congested=2254ms；t_first_action=1215ms；t_level_1=1215ms；t_level_2=2254ms；t_level_3=3302ms；t_level_4=4329ms；t_audio_only=4329ms |
| raw recovery timing | t_detect_recovering=4474ms；t_detect_stable=5513ms；t_detect_congested=432ms；t_first_action=4474ms；t_level_0=13798ms；t_level_1=10680ms；t_level_2=7558ms；t_level_3=4474ms |

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
| impairment timing | t_detect_stable=152ms |
| recovery timing | t_detect_stable=377ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=152ms |
| raw recovery timing | t_detect_stable=377ms |

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
| impairment timing | t_detect_stable=40ms |
| recovery timing | t_detect_stable=462ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=40ms |
| raw recovery timing | t_detect_stable=462ms |

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
| impairment timing | t_detect_stable=206ms |
| recovery timing | t_detect_stable=475ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=206ms |
| raw recovery timing | t_detect_stable=475ms |

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
| impairment timing | t_detect_warning=4230ms；t_detect_stable=147ms；t_first_action=4230ms；t_level_1=4230ms |
| recovery timing | t_detect_warning=847ms；t_detect_stable=2933ms；t_first_action=2933ms；t_level_0=2933ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4230ms；t_detect_stable=147ms；t_first_action=4230ms；t_level_1=4230ms |
| raw recovery timing | t_detect_warning=847ms；t_detect_stable=2933ms；t_first_action=2933ms；t_level_0=2933ms |

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
| impairment timing | t_detect_warning=2175ms；t_detect_recovering=14366ms；t_detect_stable=90ms；t_detect_congested=4252ms；t_first_action=2175ms；t_level_1=2175ms；t_level_2=4252ms；t_level_3=5293ms；t_level_4=6327ms；t_audio_only=6327ms |
| recovery timing | t_detect_warning=431ms；t_detect_stable=4517ms；t_first_action=4517ms；t_level_0=10679ms；t_level_1=7597ms；t_level_2=4517ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2175ms；t_detect_recovering=14366ms；t_detect_stable=90ms；t_detect_congested=4252ms；t_first_action=2175ms；t_level_1=2175ms；t_level_2=4252ms；t_level_3=5293ms；t_level_4=6327ms；t_audio_only=6327ms |
| raw recovery timing | t_detect_warning=431ms；t_detect_stable=4517ms；t_first_action=4517ms；t_level_0=10679ms；t_level_1=7597ms；t_level_2=4517ms |

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
| impairment timing | t_detect_warning=1383ms；t_detect_recovering=15528ms；t_detect_stable=331ms；t_detect_congested=2418ms；t_first_action=1383ms；t_level_1=1383ms；t_level_2=2418ms；t_level_3=3454ms；t_level_4=4489ms；t_audio_only=4489ms |
| recovery timing | t_detect_recovering=7569ms；t_detect_stable=8569ms；t_detect_congested=569ms；t_first_action=7569ms；t_level_0=16775ms；t_level_1=13656ms；t_level_2=10614ms；t_level_3=7569ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1383ms；t_detect_recovering=15528ms；t_detect_stable=331ms；t_detect_congested=2418ms；t_first_action=1383ms；t_level_1=1383ms；t_level_2=2418ms；t_level_3=3454ms；t_level_4=4489ms；t_audio_only=4489ms |
| raw recovery timing | t_detect_recovering=7569ms；t_detect_stable=8569ms；t_detect_congested=569ms；t_first_action=7569ms；t_level_0=16775ms；t_level_1=13656ms；t_level_2=10614ms；t_level_3=7569ms |

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
| impairment timing | t_detect_warning=1271ms；t_detect_stable=226ms；t_detect_congested=2311ms；t_first_action=1271ms；t_level_1=1271ms；t_level_2=2311ms；t_level_3=3352ms；t_level_4=4387ms；t_audio_only=4387ms |
| recovery timing | t_detect_recovering=4446ms；t_detect_stable=5448ms；t_detect_congested=446ms；t_first_action=4446ms；t_level_0=13733ms；t_level_1=10614ms；t_level_2=7552ms；t_level_3=4446ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1271ms；t_detect_stable=226ms；t_detect_congested=2311ms；t_first_action=1271ms；t_level_1=1271ms；t_level_2=2311ms；t_level_3=3352ms；t_level_4=4387ms；t_audio_only=4387ms |
| raw recovery timing | t_detect_recovering=4446ms；t_detect_stable=5448ms；t_detect_congested=446ms；t_first_action=4446ms；t_level_0=13733ms；t_level_1=10614ms；t_level_2=7552ms；t_level_3=4446ms |

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
| impairment timing | t_detect_warning=1410ms；t_detect_stable=404ms；t_detect_congested=2452ms；t_first_action=1410ms；t_level_1=1410ms；t_level_2=2452ms；t_level_3=3489ms；t_level_4=4524ms；t_audio_only=4524ms |
| recovery timing | t_detect_recovering=4773ms；t_detect_stable=5773ms；t_detect_congested=734ms；t_first_action=4773ms；t_level_0=13861ms；t_level_1=10825ms；t_level_2=7822ms；t_level_3=4773ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1410ms；t_detect_stable=404ms；t_detect_congested=2452ms；t_first_action=1410ms；t_level_1=1410ms；t_level_2=2452ms；t_level_3=3489ms；t_level_4=4524ms；t_audio_only=4524ms |
| raw recovery timing | t_detect_recovering=4773ms；t_detect_stable=5773ms；t_detect_congested=734ms；t_first_action=4773ms；t_level_0=13861ms；t_level_1=10825ms；t_level_2=7822ms；t_level_3=4773ms |

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
| impairment timing | t_detect_stable=143ms |
| recovery timing | t_detect_stable=289ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=143ms |
| raw recovery timing | t_detect_stable=289ms |

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
| impairment timing | t_detect_stable=46ms |
| recovery timing | t_detect_stable=266ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=46ms |
| raw recovery timing | t_detect_stable=266ms |

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
| impairment timing | t_detect_stable=338ms |
| recovery timing | t_detect_stable=511ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=338ms |
| raw recovery timing | t_detect_stable=511ms |

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
| impairment timing | t_detect_warning=4224ms；t_detect_stable=129ms；t_first_action=4224ms；t_level_1=4224ms |
| recovery timing | t_detect_warning=756ms；t_detect_stable=2841ms；t_first_action=2841ms；t_level_0=2841ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4224ms；t_detect_stable=129ms；t_first_action=4224ms；t_level_1=4224ms |
| raw recovery timing | t_detect_warning=756ms；t_detect_stable=2841ms；t_first_action=2841ms；t_level_0=2841ms |

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
| impairment timing | t_detect_warning=2245ms；t_detect_stable=192ms；t_detect_congested=6396ms；t_first_action=2245ms；t_level_1=2245ms；t_level_2=6396ms；t_level_3=7436ms；t_level_4=8472ms；t_audio_only=8472ms |
| recovery timing | t_detect_recovering=5575ms；t_detect_stable=6575ms；t_detect_congested=535ms；t_first_action=5575ms；t_level_0=14701ms；t_level_1=11662ms；t_level_2=8619ms；t_level_3=5575ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2245ms；t_detect_stable=192ms；t_detect_congested=6396ms；t_first_action=2245ms；t_level_1=2245ms；t_level_2=6396ms；t_level_3=7436ms；t_level_4=8472ms；t_audio_only=8472ms |
| raw recovery timing | t_detect_recovering=5575ms；t_detect_stable=6575ms；t_detect_congested=535ms；t_first_action=5575ms；t_level_0=14701ms；t_level_1=11662ms；t_level_2=8619ms；t_level_3=5575ms |

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
| impairment timing | t_detect_warning=2173ms；t_detect_stable=120ms；t_detect_congested=3210ms；t_first_action=2173ms；t_level_1=2173ms；t_level_2=3210ms；t_level_3=4249ms；t_level_4=5283ms；t_audio_only=5283ms |
| recovery timing | t_detect_recovering=6584ms；t_detect_stable=7625ms；t_detect_congested=503ms；t_first_action=6584ms；t_level_0=15911ms；t_level_1=12790ms；t_level_2=9671ms；t_level_3=6584ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2173ms；t_detect_stable=120ms；t_detect_congested=3210ms；t_first_action=2173ms；t_level_1=2173ms；t_level_2=3210ms；t_level_3=4249ms；t_level_4=5283ms；t_audio_only=5283ms |
| raw recovery timing | t_detect_recovering=6584ms；t_detect_stable=7625ms；t_detect_congested=503ms；t_first_action=6584ms；t_level_0=15911ms；t_level_1=12790ms；t_level_2=9671ms；t_level_3=6584ms |

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
| impairment timing | t_detect_stable=130ms |
| recovery timing | t_detect_stable=314ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=130ms |
| raw recovery timing | t_detect_stable=314ms |

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
| impairment timing | t_detect_stable=311ms |
| recovery timing | t_detect_stable=494ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=311ms |
| raw recovery timing | t_detect_stable=494ms |

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
| impairment timing | t_detect_warning=3209ms；t_detect_stable=167ms；t_first_action=3209ms；t_level_1=3209ms |
| recovery timing | t_detect_warning=709ms；t_detect_stable=3839ms；t_first_action=3839ms；t_level_0=3839ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3209ms；t_detect_stable=167ms；t_first_action=3209ms；t_level_1=3209ms |
| raw recovery timing | t_detect_warning=709ms；t_detect_stable=3839ms；t_first_action=3839ms；t_level_0=3839ms |

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
| impairment timing | t_detect_warning=2232ms；t_detect_stable=187ms；t_first_action=2232ms；t_level_1=2232ms |
| recovery timing | t_detect_warning=848ms；t_detect_stable=5015ms；t_first_action=5015ms；t_level_0=5015ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2232ms；t_detect_stable=187ms；t_first_action=2232ms；t_level_1=2232ms |
| raw recovery timing | t_detect_warning=848ms；t_detect_stable=5015ms；t_first_action=5015ms；t_level_0=5015ms |

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
| impairment timing | t_detect_warning=1184ms；t_detect_recovering=10458ms；t_detect_stable=148ms；t_detect_congested=2224ms；t_first_action=1184ms；t_level_0=19664ms；t_level_1=1184ms；t_level_2=2224ms；t_level_3=3264ms；t_level_4=4299ms；t_audio_only=4299ms |
| recovery timing | t_detect_stable=638ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1184ms；t_detect_recovering=10458ms；t_detect_stable=148ms；t_detect_congested=2224ms；t_first_action=1184ms；t_level_0=19664ms；t_level_1=1184ms；t_level_2=2224ms；t_level_3=3264ms；t_level_4=4299ms；t_audio_only=4299ms |
| raw recovery timing | t_detect_stable=638ms |

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
| impairment timing | t_detect_stable=181ms |
| recovery timing | t_detect_stable=444ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=181ms |
| raw recovery timing | t_detect_stable=444ms |

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
| impairment timing | t_detect_warning=1124ms；t_detect_recovering=10235ms；t_detect_stable=116ms；t_detect_congested=2162ms；t_first_action=1124ms；t_level_1=1124ms；t_level_2=2162ms；t_level_3=3202ms；t_level_4=4235ms；t_audio_only=4235ms |
| recovery timing | t_detect_recovering=3341ms；t_detect_stable=4341ms；t_detect_congested=341ms；t_first_action=3341ms；t_level_0=12509ms；t_level_1=9387ms；t_level_2=6345ms；t_level_3=3341ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1124ms；t_detect_recovering=10235ms；t_detect_stable=116ms；t_detect_congested=2162ms；t_first_action=1124ms；t_level_1=1124ms；t_level_2=2162ms；t_level_3=3202ms；t_level_4=4235ms；t_audio_only=4235ms |
| raw recovery timing | t_detect_recovering=3341ms；t_detect_stable=4341ms；t_detect_congested=341ms；t_first_action=3341ms；t_level_0=12509ms；t_level_1=9387ms；t_level_2=6345ms；t_level_3=3341ms |

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
| impairment timing | t_detect_warning=1228ms；t_detect_stable=209ms；t_detect_congested=2259ms；t_first_action=1228ms；t_level_1=1228ms；t_level_2=2259ms；t_level_3=3294ms；t_level_4=4328ms；t_audio_only=4328ms |
| recovery timing | t_detect_recovering=4431ms；t_detect_stable=5431ms；t_detect_congested=431ms；t_first_action=4431ms；t_level_0=13642ms；t_level_1=10518ms；t_level_2=7477ms；t_level_3=4431ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1228ms；t_detect_stable=209ms；t_detect_congested=2259ms；t_first_action=1228ms；t_level_1=1228ms；t_level_2=2259ms；t_level_3=3294ms；t_level_4=4328ms；t_audio_only=4328ms |
| raw recovery timing | t_detect_recovering=4431ms；t_detect_stable=5431ms；t_detect_congested=431ms；t_first_action=4431ms；t_level_0=13642ms；t_level_1=10518ms；t_level_2=7477ms；t_level_3=4431ms |

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
| impairment timing | t_detect_warning=4299ms；t_detect_stable=213ms；t_first_action=4299ms；t_level_1=4299ms |
| recovery timing | t_detect_warning=911ms；t_detect_stable=2996ms；t_first_action=2996ms；t_level_0=2996ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4299ms；t_detect_stable=213ms；t_first_action=4299ms；t_level_1=4299ms |
| raw recovery timing | t_detect_warning=911ms；t_detect_stable=2996ms；t_first_action=2996ms；t_level_0=2996ms |

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
| impairment timing | t_detect_warning=1200ms；t_detect_recovering=14313ms；t_detect_stable=194ms；t_detect_congested=2240ms；t_first_action=1200ms；t_level_1=1200ms；t_level_2=2240ms；t_level_3=3278ms；t_level_4=4274ms；t_audio_only=4274ms |
| recovery timing | t_detect_recovering=7378ms；t_detect_stable=8378ms；t_detect_congested=378ms；t_first_action=7378ms；t_level_0=16585ms；t_level_1=13465ms；t_level_2=10423ms；t_level_3=7378ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1200ms；t_detect_recovering=14313ms；t_detect_stable=194ms；t_detect_congested=2240ms；t_first_action=1200ms；t_level_1=1200ms；t_level_2=2240ms；t_level_3=3278ms；t_level_4=4274ms；t_audio_only=4274ms |
| raw recovery timing | t_detect_recovering=7378ms；t_detect_stable=8378ms；t_detect_congested=378ms；t_first_action=7378ms；t_level_0=16585ms；t_level_1=13465ms；t_level_2=10423ms；t_level_3=7378ms |

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
| impairment timing | t_detect_warning=4102ms；t_detect_stable=45ms；t_first_action=4102ms；t_level_1=4102ms |
| recovery timing | t_detect_warning=629ms；t_detect_stable=3755ms；t_first_action=3755ms；t_level_0=3755ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4102ms；t_detect_stable=45ms；t_first_action=4102ms；t_level_1=4102ms |
| raw recovery timing | t_detect_warning=629ms；t_detect_stable=3755ms；t_first_action=3755ms；t_level_0=3755ms |

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
| impairment timing | t_detect_warning=3206ms；t_detect_stable=161ms；t_first_action=3206ms；t_level_1=3206ms |
| recovery timing | t_detect_warning=838ms；t_detect_stable=2855ms；t_first_action=2855ms；t_level_0=2855ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3206ms；t_detect_stable=161ms；t_first_action=3206ms；t_level_1=3206ms |
| raw recovery timing | t_detect_warning=838ms；t_detect_stable=2855ms；t_first_action=2855ms；t_level_0=2855ms |

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
| impairment timing | t_detect_warning=1276ms；t_detect_stable=232ms；t_detect_congested=2283ms；t_first_action=1276ms；t_level_1=1276ms；t_level_2=2283ms；t_level_3=3321ms；t_level_4=4352ms；t_audio_only=4352ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1276ms；t_detect_stable=232ms；t_detect_congested=2283ms；t_first_action=1276ms；t_level_1=1276ms；t_level_2=2283ms；t_level_3=3321ms；t_level_4=4352ms；t_audio_only=4352ms |
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
| impairment timing | t_detect_warning=1180ms；t_detect_stable=168ms；t_detect_congested=2212ms；t_first_action=1180ms；t_level_1=1180ms；t_level_2=2212ms；t_level_3=3253ms；t_level_4=4288ms；t_audio_only=4288ms |
| recovery timing | t_detect_recovering=6991ms；t_detect_stable=8032ms；t_detect_congested=990ms；t_first_action=6991ms；t_level_0=16318ms；t_level_1=13198ms；t_level_2=10076ms；t_level_3=6991ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1180ms；t_detect_stable=168ms；t_detect_congested=2212ms；t_first_action=1180ms；t_level_1=1180ms；t_level_2=2212ms；t_level_3=3253ms；t_level_4=4288ms；t_audio_only=4288ms |
| raw recovery timing | t_detect_recovering=6991ms；t_detect_stable=8032ms；t_detect_congested=990ms；t_first_action=6991ms；t_level_0=16318ms；t_level_1=13198ms；t_level_2=10076ms；t_level_3=6991ms |

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
| impairment timing | t_detect_warning=1311ms；t_detect_stable=265ms；t_detect_congested=2345ms；t_first_action=1311ms；t_level_1=1311ms；t_level_2=2345ms；t_level_3=3385ms；t_level_4=4420ms；t_audio_only=4420ms |
| recovery timing | t_detect_recovering=7325ms；t_detect_stable=8365ms；t_detect_congested=243ms；t_first_action=7325ms；t_level_0=16649ms；t_level_1=13533ms；t_level_2=10503ms；t_level_3=7325ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1311ms；t_detect_stable=265ms；t_detect_congested=2345ms；t_first_action=1311ms；t_level_1=1311ms；t_level_2=2345ms；t_level_3=3385ms；t_level_4=4420ms；t_audio_only=4420ms |
| raw recovery timing | t_detect_recovering=7325ms；t_detect_stable=8365ms；t_detect_congested=243ms；t_first_action=7325ms；t_level_0=16649ms；t_level_1=13533ms；t_level_2=10503ms；t_level_3=7325ms |

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
| impairment timing | t_detect_warning=1265ms；t_detect_stable=219ms；t_detect_congested=2299ms；t_first_action=1265ms；t_level_1=1265ms；t_level_2=2299ms；t_level_3=3343ms；t_level_4=4373ms；t_audio_only=4373ms |
| recovery timing | t_detect_recovering=5760ms；t_detect_stable=6760ms；t_detect_congested=759ms；t_first_action=5760ms；t_level_0=14966ms；t_level_1=11848ms；t_level_2=8811ms；t_level_3=5760ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1265ms；t_detect_stable=219ms；t_detect_congested=2299ms；t_first_action=1265ms；t_level_1=1265ms；t_level_2=2299ms；t_level_3=3343ms；t_level_4=4373ms；t_audio_only=4373ms |
| raw recovery timing | t_detect_recovering=5760ms；t_detect_stable=6760ms；t_detect_congested=759ms；t_first_action=5760ms；t_level_0=14966ms；t_level_1=11848ms；t_level_2=8811ms；t_level_3=5760ms |

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
| impairment timing | t_detect_warning=2268ms；t_detect_stable=222ms；t_detect_congested=4345ms；t_first_action=2268ms；t_level_1=2268ms；t_level_2=4345ms |
| recovery timing | t_detect_recovering=7480ms；t_detect_stable=8483ms；t_detect_congested=366ms；t_first_action=366ms；t_level_0=16805ms；t_level_1=13685ms；t_level_2=10566ms；t_level_3=366ms；t_level_4=1401ms；t_audio_only=1401ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2268ms；t_detect_stable=222ms；t_detect_congested=4345ms；t_first_action=2268ms；t_level_1=2268ms；t_level_2=4345ms |
| raw recovery timing | t_detect_recovering=7480ms；t_detect_stable=8483ms；t_detect_congested=366ms；t_first_action=366ms；t_level_0=16805ms；t_level_1=13685ms；t_level_2=10566ms；t_level_3=366ms；t_level_4=1401ms；t_audio_only=1401ms |

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
| impairment timing | t_detect_warning=1369ms；t_detect_stable=295ms；t_detect_congested=2361ms；t_first_action=1369ms；t_level_1=1369ms；t_level_2=2361ms；t_level_3=3402ms；t_level_4=4436ms；t_audio_only=4436ms |
| recovery timing | t_detect_recovering=4494ms；t_detect_stable=5536ms；t_detect_congested=454ms；t_first_action=4494ms；t_level_0=13750ms；t_level_1=10629ms；t_level_2=7581ms；t_level_3=4494ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1369ms；t_detect_stable=295ms；t_detect_congested=2361ms；t_first_action=1369ms；t_level_1=1369ms；t_level_2=2361ms；t_level_3=3402ms；t_level_4=4436ms；t_audio_only=4436ms |
| raw recovery timing | t_detect_recovering=4494ms；t_detect_stable=5536ms；t_detect_congested=454ms；t_first_action=4494ms；t_level_0=13750ms；t_level_1=10629ms；t_level_2=7581ms；t_level_3=4494ms |

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
| impairment timing | t_detect_warning=3225ms；t_detect_stable=134ms；t_first_action=3225ms；t_level_1=3225ms |
| recovery timing | t_detect_warning=267ms；t_detect_stable=3317ms；t_first_action=3317ms；t_level_0=3317ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3225ms；t_detect_stable=134ms；t_first_action=3225ms；t_level_1=3225ms |
| raw recovery timing | t_detect_warning=267ms；t_detect_stable=3317ms；t_first_action=3317ms；t_level_0=3317ms |

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
| impairment timing | t_detect_warning=2176ms；t_detect_stable=167ms；t_first_action=2176ms；t_level_1=2176ms |
| recovery timing | t_detect_warning=259ms；t_detect_stable=4419ms；t_first_action=4419ms；t_level_0=4419ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2176ms；t_detect_stable=167ms；t_first_action=2176ms；t_level_1=2176ms |
| raw recovery timing | t_detect_warning=259ms；t_detect_stable=4419ms；t_first_action=4419ms；t_level_0=4419ms |

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
| impairment timing | t_detect_stable=48ms |
| recovery timing | t_detect_stable=141ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=48ms |
| raw recovery timing | t_detect_stable=141ms |

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
| impairment timing | t_detect_warning=1084ms；t_detect_stable=77ms；t_detect_congested=2123ms；t_first_action=1084ms；t_level_1=1084ms；t_level_2=2123ms；t_level_3=3163ms；t_level_4=4202ms；t_audio_only=4202ms |
| recovery timing | t_detect_congested=259ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1084ms；t_detect_stable=77ms；t_detect_congested=2123ms；t_first_action=1084ms；t_level_1=1084ms；t_level_2=2123ms；t_level_3=3163ms；t_level_4=4202ms；t_audio_only=4202ms |
| raw recovery timing | t_detect_congested=259ms |

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
| impairment timing | t_detect_warning=1170ms；t_detect_stable=159ms；t_detect_congested=2204ms；t_first_action=1170ms；t_level_1=1170ms；t_level_2=2204ms；t_level_3=3243ms；t_level_4=4278ms；t_audio_only=4278ms |
| recovery timing | t_detect_congested=337ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1170ms；t_detect_stable=159ms；t_detect_congested=2204ms；t_first_action=1170ms；t_level_1=1170ms；t_level_2=2204ms；t_level_3=3243ms；t_level_4=4278ms；t_audio_only=4278ms |
| raw recovery timing | t_detect_congested=337ms |

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
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1136ms；t_detect_recovering=10327ms；t_detect_stable=128ms；t_detect_congested=2173ms；t_first_action=1136ms；t_level_1=1136ms；t_level_2=2173ms；t_level_3=3213ms；t_level_4=4247ms；t_audio_only=4247ms |
| recovery timing | t_detect_stable=351ms；t_first_action=2436ms；t_level_0=5563ms；t_level_1=2436ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1136ms；t_detect_recovering=10327ms；t_detect_stable=128ms；t_detect_congested=2173ms；t_first_action=1136ms；t_level_1=1136ms；t_level_2=2173ms；t_level_3=3213ms；t_level_4=4247ms；t_audio_only=4247ms |
| raw recovery timing | t_detect_stable=351ms；t_first_action=2436ms；t_level_0=5563ms；t_level_1=2436ms |

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
| impairment timing | t_detect_warning=1059ms；t_detect_recovering=10212ms；t_detect_stable=53ms；t_detect_congested=2103ms；t_first_action=1059ms；t_level_1=1059ms；t_level_2=2103ms；t_level_3=3138ms；t_level_4=4174ms；t_audio_only=4174ms |
| recovery timing | t_detect_stable=229ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1059ms；t_detect_recovering=10212ms；t_detect_stable=53ms；t_detect_congested=2103ms；t_first_action=1059ms；t_level_1=1059ms；t_level_2=2103ms；t_level_3=3138ms；t_level_4=4174ms；t_audio_only=4174ms |
| raw recovery timing | t_detect_stable=229ms |

