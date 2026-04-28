# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-28T03:13:41.302Z`

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
| impairment timing | t_detect_stable=80ms |
| recovery timing | t_detect_stable=148ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=80ms |
| raw recovery timing | t_detect_stable=148ms |

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
| impairment timing | t_detect_stable=121ms |
| recovery timing | t_detect_stable=345ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=121ms |
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
| impairment timing | t_detect_warning=441ms |
| recovery timing | t_detect_warning=106ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=441ms |
| raw recovery timing | t_detect_warning=106ms |

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
| impairment timing | t_detect_stable=113ms |
| recovery timing | t_detect_stable=454ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=113ms |
| raw recovery timing | t_detect_stable=454ms |

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
| impairment timing | t_detect_warning=1051ms；t_detect_recovering=10204ms；t_detect_stable=45ms；t_detect_congested=2090ms；t_first_action=1051ms；t_level_1=1051ms；t_level_2=2090ms；t_level_3=3130ms；t_level_4=4165ms；t_audio_only=4165ms |
| recovery timing | t_detect_recovering=3391ms；t_detect_stable=4391ms；t_detect_congested=351ms；t_first_action=3391ms；t_level_0=12597ms；t_level_1=9477ms；t_level_2=6437ms；t_level_3=3391ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1051ms；t_detect_recovering=10204ms；t_detect_stable=45ms；t_detect_congested=2090ms；t_first_action=1051ms；t_level_1=1051ms；t_level_2=2090ms；t_level_3=3130ms；t_level_4=4165ms；t_audio_only=4165ms |
| raw recovery timing | t_detect_recovering=3391ms；t_detect_stable=4391ms；t_detect_congested=351ms；t_first_action=3391ms；t_level_0=12597ms；t_level_1=9477ms；t_level_2=6437ms；t_level_3=3391ms |

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
| impairment timing | t_detect_warning=1149ms；t_detect_recovering=10263ms；t_detect_stable=144ms；t_detect_congested=2189ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2189ms；t_level_3=3188ms；t_level_4=4224ms；t_audio_only=4224ms |
| recovery timing | t_detect_recovering=3367ms；t_detect_stable=4367ms；t_detect_congested=367ms；t_first_action=3367ms；t_level_0=12573ms；t_level_1=9453ms；t_level_2=6412ms；t_level_3=3367ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1149ms；t_detect_recovering=10263ms；t_detect_stable=144ms；t_detect_congested=2189ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2189ms；t_level_3=3188ms；t_level_4=4224ms；t_audio_only=4224ms |
| raw recovery timing | t_detect_recovering=3367ms；t_detect_stable=4367ms；t_detect_congested=367ms；t_first_action=3367ms；t_level_0=12573ms；t_level_1=9453ms；t_level_2=6412ms；t_level_3=3367ms |

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
| impairment timing | t_detect_warning=1052ms；t_detect_stable=47ms；t_detect_congested=2092ms；t_first_action=1052ms；t_level_1=1052ms；t_level_2=2092ms；t_level_3=3131ms；t_level_4=4168ms；t_audio_only=4168ms |
| recovery timing | t_detect_recovering=4312ms；t_detect_stable=5312ms；t_detect_congested=312ms；t_first_action=4312ms；t_level_0=13518ms；t_level_1=10399ms；t_level_2=7357ms；t_level_3=4312ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1052ms；t_detect_stable=47ms；t_detect_congested=2092ms；t_first_action=1052ms；t_level_1=1052ms；t_level_2=2092ms；t_level_3=3131ms；t_level_4=4168ms；t_audio_only=4168ms |
| raw recovery timing | t_detect_recovering=4312ms；t_detect_stable=5312ms；t_detect_congested=312ms；t_first_action=4312ms；t_level_0=13518ms；t_level_1=10399ms；t_level_2=7357ms；t_level_3=4312ms |

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
| impairment timing | t_detect_warning=1134ms；t_detect_stable=86ms；t_detect_congested=2170ms；t_first_action=1134ms；t_level_1=1134ms；t_level_2=2170ms；t_level_3=3209ms；t_level_4=4245ms；t_audio_only=4245ms |
| recovery timing | t_detect_recovering=4271ms；t_detect_stable=5271ms；t_detect_congested=271ms；t_first_action=4271ms；t_level_0=13478ms；t_level_1=10359ms；t_level_2=7316ms；t_level_3=4271ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1134ms；t_detect_stable=86ms；t_detect_congested=2170ms；t_first_action=1134ms；t_level_1=1134ms；t_level_2=2170ms；t_level_3=3209ms；t_level_4=4245ms；t_audio_only=4245ms |
| raw recovery timing | t_detect_recovering=4271ms；t_detect_stable=5271ms；t_detect_congested=271ms；t_first_action=4271ms；t_level_0=13478ms；t_level_1=10359ms；t_level_2=7316ms；t_level_3=4271ms |

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
| impairment timing | t_detect_warning=1012ms；t_detect_stable=6ms；t_detect_congested=2051ms；t_first_action=1012ms；t_level_1=1012ms；t_level_2=2051ms；t_level_3=3094ms；t_level_4=4126ms；t_audio_only=4126ms |
| recovery timing | t_detect_recovering=4192ms；t_detect_stable=5193ms；t_detect_congested=155ms；t_first_action=4192ms；t_level_0=13399ms；t_level_1=10286ms；t_level_2=7237ms；t_level_3=4192ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1012ms；t_detect_stable=6ms；t_detect_congested=2051ms；t_first_action=1012ms；t_level_1=1012ms；t_level_2=2051ms；t_level_3=3094ms；t_level_4=4126ms；t_audio_only=4126ms |
| raw recovery timing | t_detect_recovering=4192ms；t_detect_stable=5193ms；t_detect_congested=155ms；t_first_action=4192ms；t_level_0=13399ms；t_level_1=10286ms；t_level_2=7237ms；t_level_3=4192ms |

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
| impairment timing | t_detect_stable=163ms |
| recovery timing | t_detect_stable=189ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=163ms |
| raw recovery timing | t_detect_stable=189ms |

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
| impairment timing | t_detect_stable=951ms |
| recovery timing | t_detect_stable=138ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=951ms |
| raw recovery timing | t_detect_stable=138ms |

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
| impairment timing | t_detect_stable=117ms |
| recovery timing | t_detect_stable=422ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=117ms |
| raw recovery timing | t_detect_stable=422ms |

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
| impairment timing | t_detect_warning=4349ms；t_detect_stable=263ms；t_first_action=4349ms；t_level_1=4349ms |
| recovery timing | t_detect_warning=967ms；t_detect_stable=3056ms；t_first_action=3056ms；t_level_0=3056ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4349ms；t_detect_stable=263ms；t_first_action=4349ms；t_level_1=4349ms |
| raw recovery timing | t_detect_warning=967ms；t_detect_stable=3056ms；t_first_action=3056ms；t_level_0=3056ms |

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
| impairment timing | t_detect_warning=2047ms；t_detect_recovering=14241ms；t_detect_stable=42ms；t_detect_congested=4128ms；t_first_action=2047ms；t_level_1=2047ms；t_level_2=4128ms；t_level_3=5166ms；t_level_4=6202ms；t_audio_only=6202ms |
| recovery timing | t_detect_warning=306ms；t_detect_stable=4390ms；t_first_action=4390ms；t_level_0=10631ms；t_level_1=7512ms；t_level_2=4390ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2047ms；t_detect_recovering=14241ms；t_detect_stable=42ms；t_detect_congested=4128ms；t_first_action=2047ms；t_level_1=2047ms；t_level_2=4128ms；t_level_3=5166ms；t_level_4=6202ms；t_audio_only=6202ms |
| raw recovery timing | t_detect_warning=306ms；t_detect_stable=4390ms；t_first_action=4390ms；t_level_0=10631ms；t_level_1=7512ms；t_level_2=4390ms |

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
| impairment timing | t_detect_warning=912ms；t_detect_recovering=14105ms；t_detect_stable=15105ms；t_detect_congested=1956ms；t_first_action=912ms；t_level_1=912ms；t_level_2=1956ms；t_level_3=2995ms；t_level_4=3988ms；t_audio_only=3988ms |
| recovery timing | t_detect_recovering=7327ms；t_detect_stable=8327ms；t_detect_congested=167ms；t_first_action=7327ms；t_level_0=16534ms；t_level_1=13417ms；t_level_2=10372ms；t_level_3=7327ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=912ms；t_detect_recovering=14105ms；t_detect_stable=15105ms；t_detect_congested=1956ms；t_first_action=912ms；t_level_1=912ms；t_level_2=1956ms；t_level_3=2995ms；t_level_4=3988ms；t_audio_only=3988ms |
| raw recovery timing | t_detect_recovering=7327ms；t_detect_stable=8327ms；t_detect_congested=167ms；t_first_action=7327ms；t_level_0=16534ms；t_level_1=13417ms；t_level_2=10372ms；t_level_3=7327ms |

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
| impairment timing | t_detect_warning=1082ms；t_detect_stable=77ms；t_detect_congested=2122ms；t_first_action=1082ms；t_level_1=1082ms；t_level_2=2122ms；t_level_3=3161ms；t_level_4=4197ms；t_audio_only=4197ms |
| recovery timing | t_detect_recovering=4222ms；t_detect_stable=5222ms；t_detect_congested=222ms；t_first_action=4222ms；t_level_0=13428ms；t_level_1=10308ms；t_level_2=7266ms；t_level_3=4222ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1082ms；t_detect_stable=77ms；t_detect_congested=2122ms；t_first_action=1082ms；t_level_1=1082ms；t_level_2=2122ms；t_level_3=3161ms；t_level_4=4197ms；t_audio_only=4197ms |
| raw recovery timing | t_detect_recovering=4222ms；t_detect_stable=5222ms；t_detect_congested=222ms；t_first_action=4222ms；t_level_0=13428ms；t_level_1=10308ms；t_level_2=7266ms；t_level_3=4222ms |

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
| impairment timing | t_detect_warning=1198ms；t_detect_stable=193ms；t_detect_congested=2237ms；t_first_action=1198ms；t_level_1=1198ms；t_level_2=2237ms；t_level_3=3276ms；t_level_4=4312ms；t_audio_only=4312ms |
| recovery timing | t_detect_recovering=4418ms；t_detect_stable=5418ms；t_detect_congested=379ms；t_first_action=4418ms；t_level_0=13624ms；t_level_1=10504ms；t_level_2=7463ms；t_level_3=4418ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1198ms；t_detect_stable=193ms；t_detect_congested=2237ms；t_first_action=1198ms；t_level_1=1198ms；t_level_2=2237ms；t_level_3=3276ms；t_level_4=4312ms；t_audio_only=4312ms |
| raw recovery timing | t_detect_recovering=4418ms；t_detect_stable=5418ms；t_detect_congested=379ms；t_first_action=4418ms；t_level_0=13624ms；t_level_1=10504ms；t_level_2=7463ms；t_level_3=4418ms |

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
| impairment timing | t_detect_stable=109ms |
| recovery timing | t_detect_stable=133ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=109ms |
| raw recovery timing | t_detect_stable=133ms |

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
| impairment timing | t_detect_stable=112ms |
| recovery timing | t_detect_stable=207ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=112ms |
| raw recovery timing | t_detect_stable=207ms |

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
| impairment timing | t_detect_stable=50ms |
| recovery timing | t_detect_stable=34ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=50ms |
| raw recovery timing | t_detect_stable=34ms |

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
| impairment timing | t_detect_warning=4197ms；t_detect_stable=190ms；t_first_action=4197ms；t_level_1=4197ms |
| recovery timing | t_detect_warning=776ms；t_detect_stable=2819ms；t_first_action=2819ms；t_level_0=2819ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4197ms；t_detect_stable=190ms；t_first_action=4197ms；t_level_1=4197ms |
| raw recovery timing | t_detect_warning=776ms；t_detect_stable=2819ms；t_first_action=2819ms；t_level_0=2819ms |

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
| impairment timing | t_detect_warning=2117ms；t_detect_stable=111ms；t_detect_congested=6275ms；t_first_action=2117ms；t_level_1=2117ms；t_level_2=6275ms；t_level_3=7316ms；t_level_4=8352ms；t_audio_only=8352ms |
| recovery timing | t_detect_recovering=5616ms；t_detect_stable=6617ms；t_detect_congested=577ms；t_first_action=5616ms；t_level_0=14862ms；t_level_1=11743ms；t_level_2=8625ms；t_level_3=5616ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2117ms；t_detect_stable=111ms；t_detect_congested=6275ms；t_first_action=2117ms；t_level_1=2117ms；t_level_2=6275ms；t_level_3=7316ms；t_level_4=8352ms；t_audio_only=8352ms |
| raw recovery timing | t_detect_recovering=5616ms；t_detect_stable=6617ms；t_detect_congested=577ms；t_first_action=5616ms；t_level_0=14862ms；t_level_1=11743ms；t_level_2=8625ms；t_level_3=5616ms |

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
| impairment timing | t_detect_warning=2202ms；t_detect_stable=158ms；t_detect_congested=3241ms；t_first_action=2202ms；t_level_1=2202ms；t_level_2=3241ms；t_level_3=4280ms；t_level_4=5316ms；t_audio_only=5316ms |
| recovery timing | t_detect_recovering=6382ms；t_detect_stable=7382ms；t_detect_congested=382ms；t_first_action=6382ms；t_level_0=15601ms；t_level_1=12468ms；t_level_2=9427ms；t_level_3=6382ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2202ms；t_detect_stable=158ms；t_detect_congested=3241ms；t_first_action=2202ms；t_level_1=2202ms；t_level_2=3241ms；t_level_3=4280ms；t_level_4=5316ms；t_audio_only=5316ms |
| raw recovery timing | t_detect_recovering=6382ms；t_detect_stable=7382ms；t_detect_congested=382ms；t_first_action=6382ms；t_level_0=15601ms；t_level_1=12468ms；t_level_2=9427ms；t_level_3=6382ms |

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
| impairment timing | t_detect_stable=39ms |
| recovery timing | t_detect_stable=146ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=39ms |
| raw recovery timing | t_detect_stable=146ms |

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
| impairment timing | t_detect_stable=136ms |
| recovery timing | t_detect_stable=358ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=136ms |
| raw recovery timing | t_detect_stable=358ms |

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
| impairment timing | t_detect_warning=3088ms；t_detect_stable=83ms；t_first_action=3088ms；t_level_1=3088ms |
| recovery timing | t_detect_warning=669ms；t_detect_stable=3794ms；t_first_action=3794ms；t_level_0=3794ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3088ms；t_detect_stable=83ms；t_first_action=3088ms；t_level_1=3088ms |
| raw recovery timing | t_detect_warning=669ms；t_detect_stable=3794ms；t_first_action=3794ms；t_level_0=3794ms |

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
| impairment timing | t_detect_warning=2238ms；t_detect_stable=233ms；t_first_action=2238ms；t_level_1=2238ms |
| recovery timing | t_detect_warning=848ms；t_detect_stable=5009ms；t_first_action=5009ms；t_level_0=5009ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2238ms；t_detect_stable=233ms；t_first_action=2238ms；t_level_1=2238ms |
| raw recovery timing | t_detect_warning=848ms；t_detect_stable=5009ms；t_first_action=5009ms；t_level_0=5009ms |

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
| impairment timing | t_detect_warning=960ms；t_detect_recovering=10111ms；t_detect_stable=11111ms；t_detect_congested=1997ms；t_first_action=960ms；t_level_0=19322ms；t_level_1=960ms；t_level_2=1997ms；t_level_3=3036ms；t_level_4=4072ms；t_audio_only=4072ms |
| recovery timing | t_detect_stable=338ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=960ms；t_detect_recovering=10111ms；t_detect_stable=11111ms；t_detect_congested=1997ms；t_first_action=960ms；t_level_0=19322ms；t_level_1=960ms；t_level_2=1997ms；t_level_3=3036ms；t_level_4=4072ms；t_audio_only=4072ms |
| raw recovery timing | t_detect_stable=338ms |

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
| impairment timing | t_detect_stable=193ms |
| recovery timing | t_detect_stable=300ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=193ms |
| raw recovery timing | t_detect_stable=300ms |

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
| impairment timing | t_detect_warning=1043ms；t_detect_recovering=10157ms；t_detect_stable=11157ms；t_detect_congested=2083ms；t_first_action=1043ms；t_level_1=1043ms；t_level_2=2083ms；t_level_3=3090ms；t_level_4=4121ms；t_audio_only=4121ms |
| recovery timing | t_detect_recovering=3223ms；t_detect_stable=4223ms；t_detect_congested=223ms；t_first_action=3223ms；t_level_0=12389ms；t_level_1=9271ms；t_level_2=6271ms；t_level_3=3223ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1043ms；t_detect_recovering=10157ms；t_detect_stable=11157ms；t_detect_congested=2083ms；t_first_action=1043ms；t_level_1=1043ms；t_level_2=2083ms；t_level_3=3090ms；t_level_4=4121ms；t_audio_only=4121ms |
| raw recovery timing | t_detect_recovering=3223ms；t_detect_stable=4223ms；t_detect_congested=223ms；t_first_action=3223ms；t_level_0=12389ms；t_level_1=9271ms；t_level_2=6271ms；t_level_3=3223ms |

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
| impairment timing | t_detect_warning=1164ms；t_detect_stable=159ms；t_detect_congested=2205ms；t_first_action=1164ms；t_level_1=1164ms；t_level_2=2205ms；t_level_3=3246ms；t_level_4=4279ms；t_audio_only=4279ms |
| recovery timing | t_detect_recovering=4305ms；t_detect_stable=5305ms；t_detect_congested=305ms；t_first_action=4305ms；t_level_0=13511ms；t_level_1=10391ms；t_level_2=7349ms；t_level_3=4305ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1164ms；t_detect_stable=159ms；t_detect_congested=2205ms；t_first_action=1164ms；t_level_1=1164ms；t_level_2=2205ms；t_level_3=3246ms；t_level_4=4279ms；t_audio_only=4279ms |
| raw recovery timing | t_detect_recovering=4305ms；t_detect_stable=5305ms；t_detect_congested=305ms；t_first_action=4305ms；t_level_0=13511ms；t_level_1=10391ms；t_level_2=7349ms；t_level_3=4305ms |

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
| impairment timing | t_detect_warning=4307ms；t_detect_stable=221ms；t_first_action=4307ms；t_level_1=4307ms |
| recovery timing | t_detect_warning=929ms；t_detect_stable=3018ms；t_first_action=3018ms；t_level_0=3018ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4307ms；t_detect_stable=221ms；t_first_action=4307ms；t_level_1=4307ms |
| raw recovery timing | t_detect_warning=929ms；t_detect_stable=3018ms；t_first_action=3018ms；t_level_0=3018ms |

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
| impairment timing | t_detect_warning=1039ms；t_detect_recovering=15272ms；t_detect_stable=16272ms；t_detect_congested=2082ms；t_first_action=1039ms；t_level_1=1039ms；t_level_2=2082ms；t_level_3=3121ms；t_level_4=4152ms；t_audio_only=4152ms |
| recovery timing | t_detect_recovering=7295ms；t_detect_stable=8295ms；t_detect_congested=255ms；t_first_action=7295ms；t_level_0=16464ms；t_level_1=13340ms；t_level_2=10299ms；t_level_3=7295ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1039ms；t_detect_recovering=15272ms；t_detect_stable=16272ms；t_detect_congested=2082ms；t_first_action=1039ms；t_level_1=1039ms；t_level_2=2082ms；t_level_3=3121ms；t_level_4=4152ms；t_audio_only=4152ms |
| raw recovery timing | t_detect_recovering=7295ms；t_detect_stable=8295ms；t_detect_congested=255ms；t_first_action=7295ms；t_level_0=16464ms；t_level_1=13340ms；t_level_2=10299ms；t_level_3=7295ms |

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
| impairment timing | t_detect_warning=4343ms；t_detect_stable=297ms；t_first_action=4343ms；t_level_1=4343ms |
| recovery timing | t_detect_warning=964ms；t_detect_stable=3048ms；t_first_action=3048ms；t_level_0=3048ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4343ms；t_detect_stable=297ms；t_first_action=4343ms；t_level_1=4343ms |
| raw recovery timing | t_detect_warning=964ms；t_detect_stable=3048ms；t_first_action=3048ms；t_level_0=3048ms |

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
| impairment timing | t_detect_warning=3025ms；t_detect_stable=20ms；t_first_action=3025ms；t_level_1=3025ms |
| recovery timing | t_detect_warning=687ms；t_detect_stable=3818ms；t_first_action=3818ms；t_level_0=3818ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3025ms；t_detect_stable=20ms；t_first_action=3025ms；t_level_1=3025ms |
| raw recovery timing | t_detect_warning=687ms；t_detect_stable=3818ms；t_first_action=3818ms；t_level_0=3818ms |

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
| impairment timing | t_detect_warning=1066ms；t_detect_stable=54ms；t_detect_congested=2108ms；t_first_action=1066ms；t_level_1=1066ms；t_level_2=2108ms；t_level_3=3145ms；t_level_4=4177ms；t_audio_only=4177ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1066ms；t_detect_stable=54ms；t_detect_congested=2108ms；t_first_action=1066ms；t_level_1=1066ms；t_level_2=2108ms；t_level_3=3145ms；t_level_4=4177ms；t_audio_only=4177ms |
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
| impairment timing | t_detect_warning=1081ms；t_detect_stable=76ms；t_detect_congested=2120ms；t_first_action=1081ms；t_level_1=1081ms；t_level_2=2120ms；t_level_3=3160ms；t_level_4=4196ms；t_audio_only=4196ms |
| recovery timing | t_detect_recovering=6737ms；t_detect_stable=7736ms；t_detect_congested=736ms；t_first_action=6737ms；t_level_0=15942ms；t_level_1=12822ms；t_level_2=9790ms；t_level_3=6737ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1081ms；t_detect_stable=76ms；t_detect_congested=2120ms；t_first_action=1081ms；t_level_1=1081ms；t_level_2=2120ms；t_level_3=3160ms；t_level_4=4196ms；t_audio_only=4196ms |
| raw recovery timing | t_detect_recovering=6737ms；t_detect_stable=7736ms；t_detect_congested=736ms；t_first_action=6737ms；t_level_0=15942ms；t_level_1=12822ms；t_level_2=9790ms；t_level_3=6737ms |

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
| impairment timing | t_detect_warning=1093ms；t_detect_stable=88ms；t_detect_congested=2133ms；t_first_action=1093ms；t_level_1=1093ms；t_level_2=2133ms；t_level_3=3176ms；t_level_4=4209ms；t_audio_only=4209ms |
| recovery timing | t_detect_recovering=7672ms；t_detect_stable=8672ms；t_detect_congested=672ms；t_first_action=7672ms；t_level_0=16879ms；t_level_1=13761ms；t_level_2=10717ms；t_level_3=7672ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1093ms；t_detect_stable=88ms；t_detect_congested=2133ms；t_first_action=1093ms；t_level_1=1093ms；t_level_2=2133ms；t_level_3=3176ms；t_level_4=4209ms；t_audio_only=4209ms |
| raw recovery timing | t_detect_recovering=7672ms；t_detect_stable=8672ms；t_detect_congested=672ms；t_first_action=7672ms；t_level_0=16879ms；t_level_1=13761ms；t_level_2=10717ms；t_level_3=7672ms |

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
| impairment timing | t_detect_warning=1185ms；t_detect_stable=178ms；t_detect_congested=2227ms；t_first_action=1185ms；t_level_1=1185ms；t_level_2=2227ms；t_level_3=3276ms；t_level_4=4305ms；t_audio_only=4305ms |
| recovery timing | t_detect_recovering=6164ms；t_detect_stable=7164ms；t_detect_congested=124ms；t_first_action=6164ms；t_level_0=15372ms；t_level_1=12259ms；t_level_2=9209ms；t_level_3=6164ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1185ms；t_detect_stable=178ms；t_detect_congested=2227ms；t_first_action=1185ms；t_level_1=1185ms；t_level_2=2227ms；t_level_3=3276ms；t_level_4=4305ms；t_audio_only=4305ms |
| raw recovery timing | t_detect_recovering=6164ms；t_detect_stable=7164ms；t_detect_congested=124ms；t_first_action=6164ms；t_level_0=15372ms；t_level_1=12259ms；t_level_2=9209ms；t_level_3=6164ms |

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
| impairment timing | t_detect_warning=2168ms；t_detect_stable=155ms；t_detect_congested=4208ms；t_first_action=2168ms；t_level_1=2168ms；t_level_2=4208ms |
| recovery timing | t_detect_recovering=7301ms；t_detect_stable=8301ms；t_detect_congested=227ms；t_first_action=227ms；t_level_0=16507ms；t_level_1=13387ms；t_level_2=10346ms；t_level_3=227ms；t_level_4=1262ms；t_audio_only=1262ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2168ms；t_detect_stable=155ms；t_detect_congested=4208ms；t_first_action=2168ms；t_level_1=2168ms；t_level_2=4208ms |
| raw recovery timing | t_detect_recovering=7301ms；t_detect_stable=8301ms；t_detect_congested=227ms；t_first_action=227ms；t_level_0=16507ms；t_level_1=13387ms；t_level_2=10346ms；t_level_3=227ms；t_level_4=1262ms；t_audio_only=1262ms |

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
| impairment timing | t_detect_warning=1058ms；t_detect_stable=53ms；t_detect_congested=2098ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2098ms；t_level_3=3097ms；t_level_4=4133ms；t_audio_only=4133ms |
| recovery timing | t_detect_recovering=4237ms；t_detect_stable=5237ms；t_detect_congested=159ms；t_first_action=4237ms；t_level_0=13445ms；t_level_1=10327ms；t_level_2=7285ms；t_level_3=4237ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1058ms；t_detect_stable=53ms；t_detect_congested=2098ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2098ms；t_level_3=3097ms；t_level_4=4133ms；t_audio_only=4133ms |
| raw recovery timing | t_detect_recovering=4237ms；t_detect_stable=5237ms；t_detect_congested=159ms；t_first_action=4237ms；t_level_0=13445ms；t_level_1=10327ms；t_level_2=7285ms；t_level_3=4237ms |

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
| impairment timing | t_detect_warning=3161ms；t_detect_stable=115ms；t_first_action=3161ms；t_level_1=3161ms |
| recovery timing | t_detect_warning=224ms；t_detect_stable=3348ms；t_first_action=3348ms；t_level_0=3348ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3161ms；t_detect_stable=115ms；t_first_action=3161ms；t_level_1=3161ms |
| raw recovery timing | t_detect_warning=224ms；t_detect_stable=3348ms；t_first_action=3348ms；t_level_0=3348ms |

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
| impairment timing | t_detect_warning=2307ms；t_detect_stable=222ms；t_first_action=2307ms；t_level_1=2307ms |
| recovery timing | t_detect_warning=407ms；t_detect_stable=4574ms；t_first_action=4574ms；t_level_0=4574ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2307ms；t_detect_stable=222ms；t_first_action=2307ms；t_level_1=2307ms |
| raw recovery timing | t_detect_warning=407ms；t_detect_stable=4574ms；t_first_action=4574ms；t_level_0=4574ms |

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
| impairment timing | t_detect_stable=234ms |
| recovery timing | t_detect_stable=407ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=234ms |
| raw recovery timing | t_detect_stable=407ms |

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
| impairment timing | t_detect_warning=1177ms；t_detect_stable=140ms；t_detect_congested=2216ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2216ms；t_level_3=3255ms；t_level_4=4290ms；t_audio_only=4290ms |
| recovery timing | t_detect_congested=353ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1177ms；t_detect_stable=140ms；t_detect_congested=2216ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2216ms；t_level_3=3255ms；t_level_4=4290ms；t_audio_only=4290ms |
| raw recovery timing | t_detect_congested=353ms |

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
| impairment timing | t_detect_warning=1141ms；t_detect_stable=93ms；t_detect_congested=2179ms；t_first_action=1141ms；t_level_1=1141ms；t_level_2=2179ms；t_level_3=3217ms；t_level_4=4251ms；t_audio_only=4251ms |
| recovery timing | t_detect_congested=218ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1141ms；t_detect_stable=93ms；t_detect_congested=2179ms；t_first_action=1141ms；t_level_1=1141ms；t_level_2=2179ms；t_level_3=3217ms；t_level_4=4251ms；t_audio_only=4251ms |
| raw recovery timing | t_detect_congested=218ms |

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
| impairment timing | t_detect_warning=1149ms；t_detect_recovering=10375ms；t_detect_stable=137ms；t_detect_congested=2182ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2182ms；t_level_3=3221ms；t_level_4=4256ms；t_audio_only=4256ms |
| recovery timing | t_detect_stable=471ms；t_first_action=2529ms；t_level_0=5642ms；t_level_1=2529ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1149ms；t_detect_recovering=10375ms；t_detect_stable=137ms；t_detect_congested=2182ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2182ms；t_level_3=3221ms；t_level_4=4256ms；t_audio_only=4256ms |
| raw recovery timing | t_detect_stable=471ms；t_first_action=2529ms；t_level_0=5642ms；t_level_1=2529ms |

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
| impairment timing | t_detect_warning=1047ms；t_detect_recovering=10199ms；t_detect_stable=2ms；t_detect_congested=2089ms；t_first_action=1047ms；t_level_1=1047ms；t_level_2=2089ms；t_level_3=3129ms；t_level_4=4159ms；t_audio_only=4159ms |
| recovery timing | t_detect_stable=221ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1047ms；t_detect_recovering=10199ms；t_detect_stable=2ms；t_detect_congested=2089ms；t_first_action=1047ms；t_level_1=1047ms；t_level_2=2089ms；t_level_3=3129ms；t_level_4=4159ms；t_audio_only=4159ms |
| raw recovery timing | t_detect_stable=221ms |

