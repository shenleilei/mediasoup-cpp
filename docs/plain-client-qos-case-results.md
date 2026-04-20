# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-19T20:01:12.967Z`

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
| impairment timing | t_detect_stable=27ms |
| recovery timing | t_detect_stable=170ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=27ms |
| raw recovery timing | t_detect_stable=170ms |

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
| impairment timing | t_detect_stable=252ms |
| recovery timing | t_detect_stable=398ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=252ms |
| raw recovery timing | t_detect_stable=398ms |

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
| impairment timing | t_detect_warning=373ms |
| recovery timing | t_detect_warning=38ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=373ms |
| raw recovery timing | t_detect_warning=38ms |

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
| impairment timing | t_detect_stable=234ms |
| recovery timing | t_detect_stable=581ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=234ms |
| raw recovery timing | t_detect_stable=581ms |

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
| impairment timing | t_detect_warning=1248ms；t_detect_recovering=10398ms；t_detect_stable=239ms；t_detect_congested=2283ms；t_first_action=1248ms；t_level_1=1248ms；t_level_2=2283ms；t_level_3=3324ms；t_level_4=4359ms；t_audio_only=4359ms |
| recovery timing | t_detect_recovering=3462ms；t_detect_stable=4462ms；t_detect_congested=462ms；t_first_action=3462ms；t_level_0=12591ms；t_level_1=9509ms；t_level_2=6466ms；t_level_3=3462ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1248ms；t_detect_recovering=10398ms；t_detect_stable=239ms；t_detect_congested=2283ms；t_first_action=1248ms；t_level_1=1248ms；t_level_2=2283ms；t_level_3=3324ms；t_level_4=4359ms；t_audio_only=4359ms |
| raw recovery timing | t_detect_recovering=3462ms；t_detect_stable=4462ms；t_detect_congested=462ms；t_first_action=3462ms；t_level_0=12591ms；t_level_1=9509ms；t_level_2=6466ms；t_level_3=3462ms |

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
| impairment timing | t_detect_warning=1157ms；t_detect_recovering=10311ms；t_detect_stable=113ms；t_detect_congested=2197ms；t_first_action=1157ms；t_level_1=1157ms；t_level_2=2197ms；t_level_3=3237ms；t_level_4=4272ms；t_audio_only=4272ms |
| recovery timing | t_detect_recovering=3418ms；t_detect_stable=4418ms；t_detect_congested=418ms；t_first_action=3418ms；t_level_0=12625ms；t_level_1=9505ms；t_level_2=6463ms；t_level_3=3418ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1157ms；t_detect_recovering=10311ms；t_detect_stable=113ms；t_detect_congested=2197ms；t_first_action=1157ms；t_level_1=1157ms；t_level_2=2197ms；t_level_3=3237ms；t_level_4=4272ms；t_audio_only=4272ms |
| raw recovery timing | t_detect_recovering=3418ms；t_detect_stable=4418ms；t_detect_congested=418ms；t_first_action=3418ms；t_level_0=12625ms；t_level_1=9505ms；t_level_2=6463ms；t_level_3=3418ms |

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
| impairment timing | t_detect_warning=1206ms；t_detect_stable=201ms；t_detect_congested=2246ms；t_first_action=1206ms；t_level_1=1206ms；t_level_2=2246ms；t_level_3=3284ms；t_level_4=4321ms；t_audio_only=4321ms |
| recovery timing | t_detect_recovering=4381ms；t_detect_stable=5381ms；t_detect_congested=381ms；t_first_action=4381ms；t_level_0=13549ms；t_level_1=10429ms；t_level_2=7385ms；t_level_3=4381ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1206ms；t_detect_stable=201ms；t_detect_congested=2246ms；t_first_action=1206ms；t_level_1=1206ms；t_level_2=2246ms；t_level_3=3284ms；t_level_4=4321ms；t_audio_only=4321ms |
| raw recovery timing | t_detect_recovering=4381ms；t_detect_stable=5381ms；t_detect_congested=381ms；t_first_action=4381ms；t_level_0=13549ms；t_level_1=10429ms；t_level_2=7385ms；t_level_3=4381ms |

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
| impairment timing | t_detect_warning=1139ms；t_detect_stable=134ms；t_detect_congested=2179ms；t_first_action=1139ms；t_level_1=1139ms；t_level_2=2179ms；t_level_3=3221ms；t_level_4=4254ms；t_audio_only=4254ms |
| recovery timing | t_detect_recovering=4356ms；t_detect_stable=5356ms；t_detect_congested=315ms；t_first_action=4356ms；t_level_0=13482ms；t_level_1=10442ms；t_level_2=7400ms；t_level_3=4356ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1139ms；t_detect_stable=134ms；t_detect_congested=2179ms；t_first_action=1139ms；t_level_1=1139ms；t_level_2=2179ms；t_level_3=3221ms；t_level_4=4254ms；t_audio_only=4254ms |
| raw recovery timing | t_detect_recovering=4356ms；t_detect_stable=5356ms；t_detect_congested=315ms；t_first_action=4356ms；t_level_0=13482ms；t_level_1=10442ms；t_level_2=7400ms；t_level_3=4356ms |

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
| impairment timing | t_detect_warning=1283ms；t_detect_stable=237ms；t_detect_congested=2321ms；t_first_action=1283ms；t_level_1=1283ms；t_level_2=2321ms；t_level_3=3359ms；t_level_4=4394ms；t_audio_only=4394ms |
| recovery timing | t_detect_recovering=4400ms；t_detect_stable=5400ms；t_detect_congested=400ms；t_first_action=4400ms；t_level_0=13606ms；t_level_1=10487ms；t_level_2=7444ms；t_level_3=4400ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1283ms；t_detect_stable=237ms；t_detect_congested=2321ms；t_first_action=1283ms；t_level_1=1283ms；t_level_2=2321ms；t_level_3=3359ms；t_level_4=4394ms；t_audio_only=4394ms |
| raw recovery timing | t_detect_recovering=4400ms；t_detect_stable=5400ms；t_detect_congested=400ms；t_first_action=4400ms；t_level_0=13606ms；t_level_1=10487ms；t_level_2=7444ms；t_level_3=4400ms |

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
| impairment timing | t_detect_stable=60ms |
| recovery timing | t_detect_stable=126ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=60ms |
| raw recovery timing | t_detect_stable=126ms |

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
| impairment timing | t_detect_stable=143ms |
| recovery timing | t_detect_stable=409ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=143ms |
| raw recovery timing | t_detect_stable=409ms |

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
| impairment timing | t_detect_stable=99ms |
| recovery timing | t_detect_stable=207ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=99ms |
| raw recovery timing | t_detect_stable=207ms |

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
| impairment timing | t_detect_warning=4216ms；t_detect_stable=128ms；t_first_action=4216ms；t_level_1=4216ms |
| recovery timing | t_detect_warning=829ms；t_detect_stable=2914ms；t_first_action=2914ms；t_level_0=2914ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4216ms；t_detect_stable=128ms；t_first_action=4216ms；t_level_1=4216ms |
| raw recovery timing | t_detect_warning=829ms；t_detect_stable=2914ms；t_first_action=2914ms；t_level_0=2914ms |

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
| impairment timing | t_detect_warning=2021ms；t_detect_recovering=16295ms；t_detect_stable=1016ms；t_detect_congested=4100ms；t_first_action=2021ms；t_level_1=2021ms；t_level_2=4100ms；t_level_3=5140ms；t_level_4=6176ms；t_audio_only=6176ms |
| recovery timing | t_detect_stable=322ms；t_first_action=2366ms；t_level_0=8611ms；t_level_1=5487ms；t_level_2=2366ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2021ms；t_detect_recovering=16295ms；t_detect_stable=1016ms；t_detect_congested=4100ms；t_first_action=2021ms；t_level_1=2021ms；t_level_2=4100ms；t_level_3=5140ms；t_level_4=6176ms；t_audio_only=6176ms |
| raw recovery timing | t_detect_stable=322ms；t_first_action=2366ms；t_level_0=8611ms；t_level_1=5487ms；t_level_2=2366ms |

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
| impairment timing | t_detect_warning=1210ms；t_detect_recovering=15483ms；t_detect_stable=205ms；t_detect_congested=2249ms；t_first_action=1210ms；t_level_1=1210ms；t_level_2=2249ms；t_level_3=3288ms；t_level_4=4324ms；t_audio_only=4324ms |
| recovery timing | t_detect_recovering=7546ms；t_detect_stable=8586ms；t_detect_congested=545ms；t_first_action=7546ms；t_level_0=16791ms；t_level_1=13671ms；t_level_2=10631ms；t_level_3=7546ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1210ms；t_detect_recovering=15483ms；t_detect_stable=205ms；t_detect_congested=2249ms；t_first_action=1210ms；t_level_1=1210ms；t_level_2=2249ms；t_level_3=3288ms；t_level_4=4324ms；t_audio_only=4324ms |
| raw recovery timing | t_detect_recovering=7546ms；t_detect_stable=8586ms；t_detect_congested=545ms；t_first_action=7546ms；t_level_0=16791ms；t_level_1=13671ms；t_level_2=10631ms；t_level_3=7546ms |

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
| impairment timing | t_detect_warning=1119ms；t_detect_stable=114ms；t_detect_congested=2161ms；t_first_action=1119ms；t_level_1=1119ms；t_level_2=2161ms；t_level_3=3199ms；t_level_4=4233ms；t_audio_only=4233ms |
| recovery timing | t_detect_recovering=4416ms；t_detect_stable=5416ms；t_detect_congested=416ms；t_first_action=4416ms；t_level_0=13623ms；t_level_1=10502ms；t_level_2=7460ms；t_level_3=4416ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1119ms；t_detect_stable=114ms；t_detect_congested=2161ms；t_first_action=1119ms；t_level_1=1119ms；t_level_2=2161ms；t_level_3=3199ms；t_level_4=4233ms；t_audio_only=4233ms |
| raw recovery timing | t_detect_recovering=4416ms；t_detect_stable=5416ms；t_detect_congested=416ms；t_first_action=4416ms；t_level_0=13623ms；t_level_1=10502ms；t_level_2=7460ms；t_level_3=4416ms |

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
| impairment timing | t_detect_warning=1212ms；t_detect_stable=207ms；t_detect_congested=2253ms；t_first_action=1212ms；t_level_1=1212ms；t_level_2=2253ms；t_level_3=3291ms；t_level_4=4327ms；t_audio_only=4327ms |
| recovery timing | t_detect_recovering=4392ms；t_detect_stable=5392ms；t_detect_congested=392ms；t_first_action=4392ms；t_level_0=13609ms；t_level_1=10477ms；t_level_2=7436ms；t_level_3=4392ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1212ms；t_detect_stable=207ms；t_detect_congested=2253ms；t_first_action=1212ms；t_level_1=1212ms；t_level_2=2253ms；t_level_3=3291ms；t_level_4=4327ms；t_audio_only=4327ms |
| raw recovery timing | t_detect_recovering=4392ms；t_detect_stable=5392ms；t_detect_congested=392ms；t_first_action=4392ms；t_level_0=13609ms；t_level_1=10477ms；t_level_2=7436ms；t_level_3=4392ms |

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
| impairment timing | t_detect_stable=42ms |
| recovery timing | t_detect_stable=188ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=42ms |
| raw recovery timing | t_detect_stable=188ms |

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
| impairment timing | t_detect_stable=65ms |
| recovery timing | t_detect_stable=289ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=65ms |
| raw recovery timing | t_detect_stable=289ms |

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
| impairment timing | t_detect_stable=1036ms |
| recovery timing | t_detect_stable=304ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=1036ms |
| raw recovery timing | t_detect_stable=304ms |

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
| impairment timing | t_detect_warning=4244ms；t_detect_stable=198ms；t_first_action=4244ms；t_level_1=4244ms |
| recovery timing | t_detect_warning=814ms；t_detect_stable=2865ms；t_first_action=2865ms；t_level_0=2865ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4244ms；t_detect_stable=198ms；t_first_action=4244ms；t_level_1=4244ms |
| raw recovery timing | t_detect_warning=814ms；t_detect_stable=2865ms；t_first_action=2865ms；t_level_0=2865ms |

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
| impairment timing | t_detect_warning=2090ms；t_detect_stable=44ms；t_detect_congested=6208ms；t_first_action=2090ms；t_level_1=2090ms；t_level_2=6208ms；t_level_3=7207ms；t_level_4=8202ms；t_audio_only=8202ms |
| recovery timing | t_detect_recovering=5259ms；t_detect_stable=6299ms；t_detect_congested=219ms；t_first_action=5259ms；t_level_0=14585ms；t_level_1=11463ms；t_level_2=8343ms；t_level_3=5259ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2090ms；t_detect_stable=44ms；t_detect_congested=6208ms；t_first_action=2090ms；t_level_1=2090ms；t_level_2=6208ms；t_level_3=7207ms；t_level_4=8202ms；t_audio_only=8202ms |
| raw recovery timing | t_detect_recovering=5259ms；t_detect_stable=6299ms；t_detect_congested=219ms；t_first_action=5259ms；t_level_0=14585ms；t_level_1=11463ms；t_level_2=8343ms；t_level_3=5259ms |

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
| impairment timing | t_detect_warning=2329ms；t_detect_stable=324ms；t_detect_congested=3369ms；t_first_action=2329ms；t_level_1=2329ms；t_level_2=3369ms；t_level_3=4408ms；t_level_4=5443ms；t_audio_only=5443ms |
| recovery timing | t_detect_recovering=6748ms；t_detect_stable=7748ms；t_detect_congested=628ms；t_first_action=6748ms；t_level_0=15921ms；t_level_1=12834ms；t_level_2=9793ms；t_level_3=6748ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2329ms；t_detect_stable=324ms；t_detect_congested=3369ms；t_first_action=2329ms；t_level_1=2329ms；t_level_2=3369ms；t_level_3=4408ms；t_level_4=5443ms；t_audio_only=5443ms |
| raw recovery timing | t_detect_recovering=6748ms；t_detect_stable=7748ms；t_detect_congested=628ms；t_first_action=6748ms；t_level_0=15921ms；t_level_1=12834ms；t_level_2=9793ms；t_level_3=6748ms |

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
| impairment timing | t_detect_stable=36ms |
| recovery timing | t_detect_stable=185ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=36ms |
| raw recovery timing | t_detect_stable=185ms |

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
| impairment timing | t_detect_stable=95ms |
| recovery timing | t_detect_stable=158ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=95ms |
| raw recovery timing | t_detect_stable=158ms |

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
| impairment timing | t_detect_warning=3327ms；t_detect_stable=282ms；t_first_action=3327ms；t_level_1=3327ms |
| recovery timing | t_detect_warning=986ms；t_detect_stable=3070ms；t_first_action=3070ms；t_level_0=3070ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3327ms；t_detect_stable=282ms；t_first_action=3327ms；t_level_1=3327ms |
| raw recovery timing | t_detect_warning=986ms；t_detect_stable=3070ms；t_first_action=3070ms；t_level_0=3070ms |

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
| impairment timing | t_detect_warning=2108ms；t_detect_stable=65ms；t_first_action=2108ms；t_level_1=2108ms |
| recovery timing | t_detect_warning=812ms；t_detect_stable=4976ms；t_first_action=4976ms；t_level_0=4976ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2108ms；t_detect_stable=65ms；t_first_action=2108ms；t_level_1=2108ms |
| raw recovery timing | t_detect_warning=812ms；t_detect_stable=4976ms；t_first_action=4976ms；t_level_0=4976ms |

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
| impairment timing | t_detect_warning=1290ms；t_detect_recovering=10444ms；t_detect_stable=286ms；t_detect_congested=2329ms；t_first_action=1290ms；t_level_0=19649ms；t_level_1=1290ms；t_level_2=2329ms；t_level_3=3369ms；t_level_4=4405ms；t_audio_only=4405ms |
| recovery timing | t_detect_stable=624ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1290ms；t_detect_recovering=10444ms；t_detect_stable=286ms；t_detect_congested=2329ms；t_first_action=1290ms；t_level_0=19649ms；t_level_1=1290ms；t_level_2=2329ms；t_level_3=3369ms；t_level_4=4405ms；t_audio_only=4405ms |
| raw recovery timing | t_detect_stable=624ms |

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
| impairment timing | t_detect_stable=1ms |
| recovery timing | t_detect_stable=146ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=1ms |
| raw recovery timing | t_detect_stable=146ms |

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
| impairment timing | t_detect_warning=1252ms；t_detect_recovering=10405ms；t_detect_stable=207ms；t_detect_congested=2290ms；t_first_action=1252ms；t_level_1=1252ms；t_level_2=2290ms；t_level_3=3331ms；t_level_4=4366ms；t_audio_only=4366ms |
| recovery timing | t_detect_recovering=3552ms；t_detect_stable=4552ms；t_detect_congested=552ms；t_first_action=3552ms；t_level_0=12758ms；t_level_1=9638ms；t_level_2=6597ms；t_level_3=3552ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1252ms；t_detect_recovering=10405ms；t_detect_stable=207ms；t_detect_congested=2290ms；t_first_action=1252ms；t_level_1=1252ms；t_level_2=2290ms；t_level_3=3331ms；t_level_4=4366ms；t_audio_only=4366ms |
| raw recovery timing | t_detect_recovering=3552ms；t_detect_stable=4552ms；t_detect_congested=552ms；t_first_action=3552ms；t_level_0=12758ms；t_level_1=9638ms；t_level_2=6597ms；t_level_3=3552ms |

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
| impairment timing | t_detect_warning=1120ms；t_detect_stable=80ms；t_detect_congested=2159ms；t_first_action=1120ms；t_level_1=1120ms；t_level_2=2159ms；t_level_3=3203ms；t_level_4=4235ms；t_audio_only=4235ms |
| recovery timing | t_detect_recovering=3739ms；t_detect_stable=4739ms；t_detect_congested=739ms；t_first_action=3739ms；t_level_0=12987ms；t_level_1=9865ms；t_level_2=6745ms；t_level_3=3739ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1120ms；t_detect_stable=80ms；t_detect_congested=2159ms；t_first_action=1120ms；t_level_1=1120ms；t_level_2=2159ms；t_level_3=3203ms；t_level_4=4235ms；t_audio_only=4235ms |
| raw recovery timing | t_detect_recovering=3739ms；t_detect_stable=4739ms；t_detect_congested=739ms；t_first_action=3739ms；t_level_0=12987ms；t_level_1=9865ms；t_level_2=6745ms；t_level_3=3739ms |

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
| impairment timing | t_detect_warning=4156ms；t_detect_stable=31ms；t_first_action=4156ms；t_level_1=4156ms |
| recovery timing | t_detect_warning=771ms；t_detect_stable=2856ms；t_first_action=2856ms；t_level_0=2856ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4156ms；t_detect_stable=31ms；t_first_action=4156ms；t_level_1=4156ms |
| raw recovery timing | t_detect_warning=771ms；t_detect_stable=2856ms；t_first_action=2856ms；t_level_0=2856ms |

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
| impairment timing | t_detect_warning=966ms；t_detect_recovering=15240ms；t_detect_stable=16281ms；t_detect_congested=2006ms；t_first_action=966ms；t_level_1=966ms；t_level_2=2006ms；t_level_3=3044ms；t_level_4=4080ms；t_audio_only=4080ms |
| recovery timing | t_detect_recovering=7425ms；t_detect_stable=8425ms；t_detect_congested=306ms；t_first_action=7425ms；t_level_0=16631ms；t_level_1=13511ms；t_level_2=10471ms；t_level_3=7425ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=966ms；t_detect_recovering=15240ms；t_detect_stable=16281ms；t_detect_congested=2006ms；t_first_action=966ms；t_level_1=966ms；t_level_2=2006ms；t_level_3=3044ms；t_level_4=4080ms；t_audio_only=4080ms |
| raw recovery timing | t_detect_recovering=7425ms；t_detect_stable=8425ms；t_detect_congested=306ms；t_first_action=7425ms；t_level_0=16631ms；t_level_1=13511ms；t_level_2=10471ms；t_level_3=7425ms |

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
| impairment timing | t_detect_warning=4343ms；t_detect_stable=257ms；t_first_action=4343ms；t_level_1=4343ms |
| recovery timing | t_detect_warning=964ms；t_detect_stable=3050ms；t_first_action=3050ms；t_level_0=3050ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4343ms；t_detect_stable=257ms；t_first_action=4343ms；t_level_1=4343ms |
| raw recovery timing | t_detect_warning=964ms；t_detect_stable=3050ms；t_first_action=3050ms；t_level_0=3050ms |

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
| impairment timing | t_detect_warning=3162ms；t_detect_stable=157ms；t_first_action=3162ms；t_level_1=3162ms |
| recovery timing | t_detect_warning=824ms；t_detect_stable=2909ms；t_first_action=2909ms；t_level_0=2909ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3162ms；t_detect_stable=157ms；t_first_action=3162ms；t_level_1=3162ms |
| raw recovery timing | t_detect_warning=824ms；t_detect_stable=2909ms；t_first_action=2909ms；t_level_0=2909ms |

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
| impairment timing | t_detect_warning=1199ms；t_detect_stable=194ms；t_detect_congested=2241ms；t_first_action=1199ms；t_level_1=1199ms；t_level_2=2241ms；t_level_3=3277ms；t_level_4=4313ms；t_audio_only=4313ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1199ms；t_detect_stable=194ms；t_detect_congested=2241ms；t_first_action=1199ms；t_level_1=1199ms；t_level_2=2241ms；t_level_3=3277ms；t_level_4=4313ms；t_audio_only=4313ms |
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
| impairment timing | t_detect_warning=1046ms；t_detect_stable=41ms；t_detect_congested=2086ms；t_first_action=1046ms；t_level_1=1046ms；t_level_2=2086ms；t_level_3=3124ms；t_level_4=4161ms；t_audio_only=4161ms |
| recovery timing | t_detect_recovering=7305ms；t_detect_stable=8306ms；t_detect_congested=305ms；t_first_action=7305ms；t_level_0=16511ms；t_level_1=13394ms；t_level_2=10350ms；t_level_3=7305ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1046ms；t_detect_stable=41ms；t_detect_congested=2086ms；t_first_action=1046ms；t_level_1=1046ms；t_level_2=2086ms；t_level_3=3124ms；t_level_4=4161ms；t_audio_only=4161ms |
| raw recovery timing | t_detect_recovering=7305ms；t_detect_stable=8306ms；t_detect_congested=305ms；t_first_action=7305ms；t_level_0=16511ms；t_level_1=13394ms；t_level_2=10350ms；t_level_3=7305ms |

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
| impairment timing | t_detect_warning=1216ms；t_detect_stable=169ms；t_detect_congested=2254ms；t_first_action=1216ms；t_level_1=1216ms；t_level_2=2254ms；t_level_3=3293ms；t_level_4=4331ms；t_audio_only=4331ms |
| recovery timing | t_detect_recovering=6714ms；t_detect_stable=7714ms；t_detect_congested=714ms；t_first_action=6714ms；t_level_0=15927ms；t_level_1=12804ms；t_level_2=9760ms；t_level_3=6714ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1216ms；t_detect_stable=169ms；t_detect_congested=2254ms；t_first_action=1216ms；t_level_1=1216ms；t_level_2=2254ms；t_level_3=3293ms；t_level_4=4331ms；t_audio_only=4331ms |
| raw recovery timing | t_detect_recovering=6714ms；t_detect_stable=7714ms；t_detect_congested=714ms；t_first_action=6714ms；t_level_0=15927ms；t_level_1=12804ms；t_level_2=9760ms；t_level_3=6714ms |

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
| impairment timing | t_detect_warning=1099ms；t_detect_stable=56ms；t_detect_congested=2144ms；t_first_action=1099ms；t_level_1=1099ms；t_level_2=2144ms；t_level_3=3180ms；t_level_4=4214ms；t_audio_only=4214ms |
| recovery timing | t_detect_recovering=5920ms；t_detect_stable=6920ms；t_detect_congested=880ms；t_first_action=5920ms；t_level_0=15166ms；t_level_1=12045ms；t_level_2=8925ms；t_level_3=5920ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1099ms；t_detect_stable=56ms；t_detect_congested=2144ms；t_first_action=1099ms；t_level_1=1099ms；t_level_2=2144ms；t_level_3=3180ms；t_level_4=4214ms；t_audio_only=4214ms |
| raw recovery timing | t_detect_recovering=5920ms；t_detect_stable=6920ms；t_detect_congested=880ms；t_first_action=5920ms；t_level_0=15166ms；t_level_1=12045ms；t_level_2=8925ms；t_level_3=5920ms |

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
| impairment timing | t_detect_warning=2053ms；t_detect_stable=47ms；t_detect_congested=4131ms；t_first_action=2053ms；t_level_1=2053ms；t_level_2=4131ms |
| recovery timing | t_detect_recovering=7232ms；t_detect_stable=8232ms；t_detect_congested=157ms；t_first_action=157ms；t_level_0=16438ms；t_level_1=13317ms；t_level_2=10276ms；t_level_3=157ms；t_level_4=1193ms；t_audio_only=1193ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2053ms；t_detect_stable=47ms；t_detect_congested=4131ms；t_first_action=2053ms；t_level_1=2053ms；t_level_2=4131ms |
| raw recovery timing | t_detect_recovering=7232ms；t_detect_stable=8232ms；t_detect_congested=157ms；t_first_action=157ms；t_level_0=16438ms；t_level_1=13317ms；t_level_2=10276ms；t_level_3=157ms；t_level_4=1193ms；t_audio_only=1193ms |

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
| impairment timing | t_detect_warning=957ms；t_detect_congested=1997ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1997ms；t_level_3=3048ms；t_level_4=4072ms；t_audio_only=4072ms |
| recovery timing | t_detect_recovering=4136ms；t_detect_stable=5136ms；t_detect_congested=136ms；t_first_action=4136ms；t_level_0=13342ms；t_level_1=10221ms；t_level_2=7181ms；t_level_3=4136ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=957ms；t_detect_congested=1997ms；t_first_action=957ms；t_level_1=957ms；t_level_2=1997ms；t_level_3=3048ms；t_level_4=4072ms；t_audio_only=4072ms |
| raw recovery timing | t_detect_recovering=4136ms；t_detect_stable=5136ms；t_detect_congested=136ms；t_first_action=4136ms；t_level_0=13342ms；t_level_1=10221ms；t_level_2=7181ms；t_level_3=4136ms |

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
| impairment timing | t_detect_warning=3328ms；t_detect_stable=321ms；t_first_action=3328ms；t_level_1=3328ms |
| recovery timing | t_detect_warning=375ms；t_detect_stable=3499ms；t_first_action=3499ms；t_level_0=3499ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3328ms；t_detect_stable=321ms；t_first_action=3328ms；t_level_1=3328ms |
| raw recovery timing | t_detect_warning=375ms；t_detect_stable=3499ms；t_first_action=3499ms；t_level_0=3499ms |

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
| impairment timing | t_detect_warning=2066ms；t_detect_stable=59ms；t_first_action=2066ms；t_level_1=2066ms |
| recovery timing | t_detect_warning=166ms；t_detect_stable=4254ms；t_first_action=4254ms；t_level_0=4254ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2066ms；t_detect_stable=59ms；t_first_action=2066ms；t_level_1=2066ms |
| raw recovery timing | t_detect_warning=166ms；t_detect_stable=4254ms；t_first_action=4254ms；t_level_0=4254ms |

