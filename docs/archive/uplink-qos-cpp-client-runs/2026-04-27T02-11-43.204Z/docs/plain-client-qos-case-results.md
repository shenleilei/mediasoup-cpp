# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-27T02:11:43.204Z`

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
| impairment timing | t_detect_stable=17ms |
| recovery timing | t_detect_stable=114ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=17ms |
| raw recovery timing | t_detect_stable=114ms |

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
| impairment timing | t_detect_stable=239ms |
| recovery timing | t_detect_stable=680ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=239ms |
| raw recovery timing | t_detect_stable=680ms |

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
| recovery timing | t_detect_warning=959ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=383ms |
| raw recovery timing | t_detect_warning=959ms |

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
| impairment timing | t_detect_stable=206ms |
| recovery timing | t_detect_stable=592ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=206ms |
| raw recovery timing | t_detect_stable=592ms |

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
| impairment timing | t_detect_warning=1140ms；t_detect_recovering=10336ms；t_detect_stable=135ms；t_detect_congested=2181ms；t_first_action=1140ms；t_level_1=1140ms；t_level_2=2181ms；t_level_3=3224ms；t_level_4=4216ms；t_audio_only=4216ms |
| recovery timing | t_detect_recovering=3510ms；t_detect_stable=4511ms；t_detect_congested=510ms；t_first_action=3510ms；t_level_0=12721ms；t_level_1=9596ms；t_level_2=6570ms；t_level_3=3510ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1140ms；t_detect_recovering=10336ms；t_detect_stable=135ms；t_detect_congested=2181ms；t_first_action=1140ms；t_level_1=1140ms；t_level_2=2181ms；t_level_3=3224ms；t_level_4=4216ms；t_audio_only=4216ms |
| raw recovery timing | t_detect_recovering=3510ms；t_detect_stable=4511ms；t_detect_congested=510ms；t_first_action=3510ms；t_level_0=12721ms；t_level_1=9596ms；t_level_2=6570ms；t_level_3=3510ms |

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
| impairment timing | t_detect_warning=1235ms；t_detect_recovering=10385ms；t_detect_stable=189ms；t_detect_congested=2303ms；t_first_action=1235ms；t_level_1=1235ms；t_level_2=2303ms；t_level_3=3319ms；t_level_4=4307ms；t_audio_only=4307ms |
| recovery timing | t_detect_recovering=3567ms；t_detect_stable=4567ms；t_detect_congested=567ms；t_first_action=3567ms；t_level_0=12735ms；t_level_1=9659ms；t_level_2=6621ms；t_level_3=3567ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1235ms；t_detect_recovering=10385ms；t_detect_stable=189ms；t_detect_congested=2303ms；t_first_action=1235ms；t_level_1=1235ms；t_level_2=2303ms；t_level_3=3319ms；t_level_4=4307ms；t_audio_only=4307ms |
| raw recovery timing | t_detect_recovering=3567ms；t_detect_stable=4567ms；t_detect_congested=567ms；t_first_action=3567ms；t_level_0=12735ms；t_level_1=9659ms；t_level_2=6621ms；t_level_3=3567ms |

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
| impairment timing | t_detect_warning=1316ms；t_detect_stable=301ms；t_detect_congested=2346ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2346ms；t_level_3=3359ms；t_level_4=4382ms；t_audio_only=4382ms |
| recovery timing | t_detect_recovering=4628ms；t_detect_stable=5628ms；t_detect_congested=549ms；t_first_action=4628ms；t_level_0=13838ms；t_level_1=10717ms；t_level_2=7684ms；t_level_3=4628ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1316ms；t_detect_stable=301ms；t_detect_congested=2346ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2346ms；t_level_3=3359ms；t_level_4=4382ms；t_audio_only=4382ms |
| raw recovery timing | t_detect_recovering=4628ms；t_detect_stable=5628ms；t_detect_congested=549ms；t_first_action=4628ms；t_level_0=13838ms；t_level_1=10717ms；t_level_2=7684ms；t_level_3=4628ms |

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
| impairment timing | t_detect_warning=1167ms；t_detect_stable=161ms；t_detect_congested=2205ms；t_first_action=1167ms；t_level_1=1167ms；t_level_2=2205ms；t_level_3=3256ms；t_level_4=4281ms；t_audio_only=4281ms |
| recovery timing | t_detect_recovering=4585ms；t_detect_stable=5585ms；t_detect_congested=545ms；t_first_action=4585ms；t_level_0=13792ms；t_level_1=10683ms；t_level_2=7630ms；t_level_3=4585ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1167ms；t_detect_stable=161ms；t_detect_congested=2205ms；t_first_action=1167ms；t_level_1=1167ms；t_level_2=2205ms；t_level_3=3256ms；t_level_4=4281ms；t_audio_only=4281ms |
| raw recovery timing | t_detect_recovering=4585ms；t_detect_stable=5585ms；t_detect_congested=545ms；t_first_action=4585ms；t_level_0=13792ms；t_level_1=10683ms；t_level_2=7630ms；t_level_3=4585ms |

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
| impairment timing | t_detect_warning=1319ms；t_detect_stable=268ms；t_detect_congested=2355ms；t_first_action=1319ms；t_level_1=1319ms；t_level_2=2355ms；t_level_3=3394ms；t_level_4=4425ms；t_audio_only=4425ms |
| recovery timing | t_detect_recovering=4530ms；t_detect_stable=5571ms；t_detect_congested=449ms；t_first_action=4530ms；t_level_0=13776ms；t_level_1=10660ms；t_level_2=7620ms；t_level_3=4530ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1319ms；t_detect_stable=268ms；t_detect_congested=2355ms；t_first_action=1319ms；t_level_1=1319ms；t_level_2=2355ms；t_level_3=3394ms；t_level_4=4425ms；t_audio_only=4425ms |
| raw recovery timing | t_detect_recovering=4530ms；t_detect_stable=5571ms；t_detect_congested=449ms；t_first_action=4530ms；t_level_0=13776ms；t_level_1=10660ms；t_level_2=7620ms；t_level_3=4530ms |

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
| impairment timing | t_detect_stable=276ms |
| recovery timing | t_detect_stable=541ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=276ms |
| raw recovery timing | t_detect_stable=541ms |

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
| impairment timing | t_detect_stable=181ms |
| recovery timing | t_detect_stable=646ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=181ms |
| raw recovery timing | t_detect_stable=646ms |

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
| impairment timing | t_detect_stable=125ms |
| recovery timing | t_detect_stable=291ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=125ms |
| raw recovery timing | t_detect_stable=291ms |

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
| impairment timing | t_detect_warning=4232ms；t_detect_stable=188ms；t_first_action=4232ms；t_level_1=4232ms |
| recovery timing | t_detect_warning=673ms；t_detect_stable=2730ms；t_first_action=2730ms；t_level_0=2730ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4232ms；t_detect_stable=188ms；t_first_action=4232ms；t_level_1=4232ms |
| raw recovery timing | t_detect_warning=673ms；t_detect_stable=2730ms；t_first_action=2730ms；t_level_0=2730ms |

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
| impairment timing | t_detect_warning=2352ms；t_detect_recovering=14660ms；t_detect_stable=262ms；t_detect_congested=4427ms；t_first_action=2352ms；t_level_1=2352ms；t_level_2=4427ms；t_level_3=5466ms；t_level_4=6503ms；t_audio_only=6503ms |
| recovery timing | t_detect_warning=723ms；t_detect_stable=3807ms；t_first_action=3807ms；t_level_0=9950ms；t_level_1=6886ms；t_level_2=3807ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2352ms；t_detect_recovering=14660ms；t_detect_stable=262ms；t_detect_congested=4427ms；t_first_action=2352ms；t_level_1=2352ms；t_level_2=4427ms；t_level_3=5466ms；t_level_4=6503ms；t_audio_only=6503ms |
| raw recovery timing | t_detect_warning=723ms；t_detect_stable=3807ms；t_first_action=3807ms；t_level_0=9950ms；t_level_1=6886ms；t_level_2=3807ms |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=early_warning/L3)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | setEncodingParameters, enterAudioOnly, exitAudioOnly（共 8 次非 noop） |
| impairment timing | t_detect_warning=1302ms；t_detect_recovering=15487ms；t_detect_stable=286ms；t_detect_congested=2345ms；t_first_action=1302ms；t_level_1=1302ms；t_level_2=2345ms；t_level_3=3371ms；t_level_4=4407ms；t_audio_only=4407ms |
| recovery timing | t_detect_warning=582ms；t_detect_stable=4677ms；t_first_action=4677ms；t_level_0=10912ms；t_level_1=7786ms；t_level_2=4677ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1302ms；t_detect_recovering=15487ms；t_detect_stable=286ms；t_detect_congested=2345ms；t_first_action=1302ms；t_level_1=1302ms；t_level_2=2345ms；t_level_3=3371ms；t_level_4=4407ms；t_audio_only=4407ms |
| raw recovery timing | t_detect_warning=582ms；t_detect_stable=4677ms；t_first_action=4677ms；t_level_0=10912ms；t_level_1=7786ms；t_level_2=4677ms |

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
| impairment timing | t_detect_warning=1148ms；t_detect_stable=142ms；t_detect_congested=2186ms；t_first_action=1148ms；t_level_1=1148ms；t_level_2=2186ms；t_level_3=3233ms；t_level_4=4261ms；t_audio_only=4261ms |
| recovery timing | t_detect_recovering=4513ms；t_detect_stable=5554ms；t_detect_congested=432ms；t_first_action=4513ms；t_level_0=13814ms；t_level_1=10733ms；t_level_2=7598ms；t_level_3=4513ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1148ms；t_detect_stable=142ms；t_detect_congested=2186ms；t_first_action=1148ms；t_level_1=1148ms；t_level_2=2186ms；t_level_3=3233ms；t_level_4=4261ms；t_audio_only=4261ms |
| raw recovery timing | t_detect_recovering=4513ms；t_detect_stable=5554ms；t_detect_congested=432ms；t_first_action=4513ms；t_level_0=13814ms；t_level_1=10733ms；t_level_2=7598ms；t_level_3=4513ms |

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
| impairment timing | t_detect_warning=1258ms；t_detect_stable=212ms；t_detect_congested=2299ms；t_first_action=1258ms；t_level_1=1258ms；t_level_2=2299ms；t_level_3=3301ms；t_level_4=4291ms；t_audio_only=4291ms |
| recovery timing | t_detect_recovering=4673ms；t_detect_stable=5716ms；t_detect_congested=592ms；t_first_action=4673ms；t_level_0=13845ms；t_level_1=10768ms；t_level_2=7769ms；t_level_3=4673ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1258ms；t_detect_stable=212ms；t_detect_congested=2299ms；t_first_action=1258ms；t_level_1=1258ms；t_level_2=2299ms；t_level_3=3301ms；t_level_4=4291ms；t_audio_only=4291ms |
| raw recovery timing | t_detect_recovering=4673ms；t_detect_stable=5716ms；t_detect_congested=592ms；t_first_action=4673ms；t_level_0=13845ms；t_level_1=10768ms；t_level_2=7769ms；t_level_3=4673ms |

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
| impairment timing | t_detect_stable=182ms |
| recovery timing | t_detect_stable=595ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=182ms |
| raw recovery timing | t_detect_stable=595ms |

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
| impairment timing | t_detect_stable=386ms |
| recovery timing | t_detect_stable=660ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=386ms |
| raw recovery timing | t_detect_stable=660ms |

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
| impairment timing | t_detect_stable=227ms |
| recovery timing | t_detect_stable=653ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=227ms |
| raw recovery timing | t_detect_stable=653ms |

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
| impairment timing | t_detect_warning=4269ms；t_detect_stable=170ms；t_first_action=4269ms；t_level_1=4269ms |
| recovery timing | t_detect_warning=780ms；t_detect_stable=2865ms；t_first_action=2865ms；t_level_0=2865ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4269ms；t_detect_stable=170ms；t_first_action=4269ms；t_level_1=4269ms |
| raw recovery timing | t_detect_warning=780ms；t_detect_stable=2865ms；t_first_action=2865ms；t_level_0=2865ms |

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
| impairment timing | t_detect_warning=2280ms；t_detect_stable=271ms；t_detect_congested=6438ms；t_first_action=2280ms；t_level_1=2280ms；t_level_2=6438ms；t_level_3=7475ms；t_level_4=8510ms；t_audio_only=8510ms |
| recovery timing | t_detect_recovering=4735ms；t_detect_stable=5776ms；t_detect_congested=694ms；t_first_action=4735ms；t_level_0=14065ms；t_level_1=10945ms；t_level_2=7820ms；t_level_3=4735ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2280ms；t_detect_stable=271ms；t_detect_congested=6438ms；t_first_action=2280ms；t_level_1=2280ms；t_level_2=6438ms；t_level_3=7475ms；t_level_4=8510ms；t_audio_only=8510ms |
| raw recovery timing | t_detect_recovering=4735ms；t_detect_stable=5776ms；t_detect_congested=694ms；t_first_action=4735ms；t_level_0=14065ms；t_level_1=10945ms；t_level_2=7820ms；t_level_3=4735ms |

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
| impairment timing | t_detect_warning=2360ms；t_detect_stable=310ms；t_detect_congested=3398ms；t_first_action=2360ms；t_level_1=2360ms；t_level_2=3398ms；t_level_3=4437ms；t_level_4=5470ms；t_audio_only=5470ms |
| recovery timing | t_detect_recovering=6573ms；t_detect_stable=7573ms；t_detect_congested=533ms；t_first_action=6573ms；t_level_0=15782ms；t_level_1=12662ms；t_level_2=9622ms；t_level_3=6573ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2360ms；t_detect_stable=310ms；t_detect_congested=3398ms；t_first_action=2360ms；t_level_1=2360ms；t_level_2=3398ms；t_level_3=4437ms；t_level_4=5470ms；t_audio_only=5470ms |
| raw recovery timing | t_detect_recovering=6573ms；t_detect_stable=7573ms；t_detect_congested=533ms；t_first_action=6573ms；t_level_0=15782ms；t_level_1=12662ms；t_level_2=9622ms；t_level_3=6573ms |

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
| impairment timing | t_detect_stable=152ms |
| recovery timing | t_detect_stable=503ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=152ms |
| raw recovery timing | t_detect_stable=503ms |

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
| impairment timing | t_detect_stable=278ms |
| recovery timing | t_detect_stable=472ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=278ms |
| raw recovery timing | t_detect_stable=472ms |

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
| impairment timing | t_detect_warning=3265ms；t_detect_stable=178ms；t_first_action=3265ms；t_level_1=3265ms |
| recovery timing | t_detect_warning=667ms；t_detect_stable=2749ms；t_first_action=2749ms；t_level_0=2749ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3265ms；t_detect_stable=178ms；t_first_action=3265ms；t_level_1=3265ms |
| raw recovery timing | t_detect_warning=667ms；t_detect_stable=2749ms；t_first_action=2749ms；t_level_0=2749ms |

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
| impairment timing | t_detect_warning=2304ms；t_detect_stable=260ms；t_first_action=2304ms；t_level_1=2304ms |
| recovery timing | t_detect_warning=801ms；t_detect_stable=4888ms；t_first_action=4888ms；t_level_0=4888ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2304ms；t_detect_stable=260ms；t_first_action=2304ms；t_level_1=2304ms |
| raw recovery timing | t_detect_warning=801ms；t_detect_stable=4888ms；t_first_action=4888ms；t_level_0=4888ms |

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
| impairment timing | t_detect_warning=1249ms；t_detect_recovering=10442ms；t_detect_stable=242ms；t_detect_congested=2292ms；t_first_action=1249ms；t_level_0=19659ms；t_level_1=1249ms；t_level_2=2292ms；t_level_3=3327ms；t_level_4=4365ms；t_audio_only=4365ms |
| recovery timing | t_detect_stable=665ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1249ms；t_detect_recovering=10442ms；t_detect_stable=242ms；t_detect_congested=2292ms；t_first_action=1249ms；t_level_0=19659ms；t_level_1=1249ms；t_level_2=2292ms；t_level_3=3327ms；t_level_4=4365ms；t_audio_only=4365ms |
| raw recovery timing | t_detect_stable=665ms |

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
| impairment timing | t_detect_stable=48ms |
| recovery timing | t_detect_stable=384ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=48ms |
| raw recovery timing | t_detect_stable=384ms |

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
| impairment timing | t_detect_warning=1340ms；t_detect_recovering=10490ms；t_detect_stable=333ms；t_detect_congested=2392ms；t_first_action=1340ms；t_level_1=1340ms；t_level_2=2392ms；t_level_3=3419ms；t_level_4=4451ms；t_audio_only=4451ms |
| recovery timing | t_detect_recovering=3584ms；t_detect_stable=4584ms；t_detect_congested=584ms；t_first_action=3584ms；t_level_0=12790ms；t_level_1=9670ms；t_level_2=6643ms；t_level_3=3584ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1340ms；t_detect_recovering=10490ms；t_detect_stable=333ms；t_detect_congested=2392ms；t_first_action=1340ms；t_level_1=1340ms；t_level_2=2392ms；t_level_3=3419ms；t_level_4=4451ms；t_audio_only=4451ms |
| raw recovery timing | t_detect_recovering=3584ms；t_detect_stable=4584ms；t_detect_congested=584ms；t_first_action=3584ms；t_level_0=12790ms；t_level_1=9670ms；t_level_2=6643ms；t_level_3=3584ms |

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
| impairment timing | t_detect_warning=1218ms；t_detect_stable=201ms；t_detect_congested=2253ms；t_first_action=1218ms；t_level_1=1218ms；t_level_2=2253ms；t_level_3=3291ms；t_level_4=4320ms；t_audio_only=4320ms |
| recovery timing | t_detect_recovering=3554ms；t_detect_stable=4562ms；t_detect_congested=554ms；t_first_action=3554ms；t_level_0=12768ms；t_level_1=9688ms；t_level_2=6600ms；t_level_3=3554ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1218ms；t_detect_stable=201ms；t_detect_congested=2253ms；t_first_action=1218ms；t_level_1=1218ms；t_level_2=2253ms；t_level_3=3291ms；t_level_4=4320ms；t_audio_only=4320ms |
| raw recovery timing | t_detect_recovering=3554ms；t_detect_stable=4562ms；t_detect_congested=554ms；t_first_action=3554ms；t_level_0=12768ms；t_level_1=9688ms；t_level_2=6600ms；t_level_3=3554ms |

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
| impairment timing | t_detect_warning=4401ms；t_detect_stable=316ms；t_first_action=4401ms；t_level_1=4401ms |
| recovery timing | t_detect_warning=919ms；t_detect_stable=3004ms；t_first_action=3004ms；t_level_0=3004ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4401ms；t_detect_stable=316ms；t_first_action=4401ms；t_level_1=4401ms |
| raw recovery timing | t_detect_warning=919ms；t_detect_stable=3004ms；t_first_action=3004ms；t_level_0=3004ms |

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
| impairment timing | t_detect_warning=1168ms；t_detect_recovering=15316ms；t_detect_stable=158ms；t_detect_congested=2182ms；t_first_action=1168ms；t_level_1=1168ms；t_level_2=2182ms；t_level_3=3205ms；t_level_4=4238ms；t_audio_only=4238ms |
| recovery timing | t_detect_recovering=7459ms；t_detect_stable=8499ms；t_detect_congested=379ms；t_first_action=7459ms；t_level_0=16784ms；t_level_1=13683ms；t_level_2=10547ms；t_level_3=7459ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1168ms；t_detect_recovering=15316ms；t_detect_stable=158ms；t_detect_congested=2182ms；t_first_action=1168ms；t_level_1=1168ms；t_level_2=2182ms；t_level_3=3205ms；t_level_4=4238ms；t_audio_only=4238ms |
| raw recovery timing | t_detect_recovering=7459ms；t_detect_stable=8499ms；t_detect_congested=379ms；t_first_action=7459ms；t_level_0=16784ms；t_level_1=13683ms；t_level_2=10547ms；t_level_3=7459ms |

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
| impairment timing | t_detect_warning=4328ms；t_detect_stable=234ms；t_first_action=4328ms；t_level_1=4328ms |
| recovery timing | t_detect_warning=849ms；t_detect_stable=2949ms；t_first_action=2949ms；t_level_0=2949ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4328ms；t_detect_stable=234ms；t_first_action=4328ms；t_level_1=4328ms |
| raw recovery timing | t_detect_warning=849ms；t_detect_stable=2949ms；t_first_action=2949ms；t_level_0=2949ms |

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
| impairment timing | t_detect_warning=3272ms；t_detect_stable=228ms；t_first_action=3272ms；t_level_1=3272ms |
| recovery timing | t_detect_warning=682ms；t_detect_stable=2767ms；t_first_action=2767ms；t_level_0=2767ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3272ms；t_detect_stable=228ms；t_first_action=3272ms；t_level_1=3272ms |
| raw recovery timing | t_detect_warning=682ms；t_detect_stable=2767ms；t_first_action=2767ms；t_level_0=2767ms |

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
| impairment timing | t_detect_warning=1320ms；t_detect_stable=307ms；t_detect_congested=2354ms；t_first_action=1320ms；t_level_1=1320ms；t_level_2=2354ms；t_level_3=3394ms；t_level_4=4426ms；t_audio_only=4426ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1320ms；t_detect_stable=307ms；t_detect_congested=2354ms；t_first_action=1320ms；t_level_1=1320ms；t_level_2=2354ms；t_level_3=3394ms；t_level_4=4426ms；t_audio_only=4426ms |
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
| impairment timing | t_detect_warning=1119ms；t_detect_stable=100ms；t_detect_congested=2161ms；t_first_action=1119ms；t_level_1=1119ms；t_level_2=2161ms；t_level_3=3202ms；t_level_4=4234ms；t_audio_only=4234ms |
| recovery timing | t_detect_recovering=6640ms；t_detect_stable=7641ms；t_detect_congested=601ms；t_first_action=6640ms；t_level_0=15815ms；t_level_1=12740ms；t_level_2=9688ms；t_level_3=6640ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1119ms；t_detect_stable=100ms；t_detect_congested=2161ms；t_first_action=1119ms；t_level_1=1119ms；t_level_2=2161ms；t_level_3=3202ms；t_level_4=4234ms；t_audio_only=4234ms |
| raw recovery timing | t_detect_recovering=6640ms；t_detect_stable=7641ms；t_detect_congested=601ms；t_first_action=6640ms；t_level_0=15815ms；t_level_1=12740ms；t_level_2=9688ms；t_level_3=6640ms |

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
| impairment timing | t_detect_warning=1235ms；t_detect_stable=187ms；t_detect_congested=2273ms；t_first_action=1235ms；t_level_1=1235ms；t_level_2=2273ms；t_level_3=3317ms；t_level_4=4346ms；t_audio_only=4346ms |
| recovery timing | t_detect_recovering=7406ms；t_detect_stable=8406ms；t_detect_congested=406ms；t_first_action=7406ms；t_level_0=16533ms；t_level_1=13509ms；t_level_2=10459ms；t_level_3=7406ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1235ms；t_detect_stable=187ms；t_detect_congested=2273ms；t_first_action=1235ms；t_level_1=1235ms；t_level_2=2273ms；t_level_3=3317ms；t_level_4=4346ms；t_audio_only=4346ms |
| raw recovery timing | t_detect_recovering=7406ms；t_detect_stable=8406ms；t_detect_congested=406ms；t_first_action=7406ms；t_level_0=16533ms；t_level_1=13509ms；t_level_2=10459ms；t_level_3=7406ms |

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
| impairment timing | t_detect_warning=1124ms；t_detect_stable=99ms；t_detect_congested=2153ms；t_first_action=1124ms；t_level_1=1124ms；t_level_2=2153ms；t_level_3=3207ms；t_level_4=4217ms；t_audio_only=4217ms |
| recovery timing | t_detect_recovering=5751ms；t_detect_stable=6783ms；t_detect_congested=662ms；t_first_action=5751ms；t_level_0=15049ms；t_level_1=11975ms；t_level_2=8792ms；t_level_3=5751ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1124ms；t_detect_stable=99ms；t_detect_congested=2153ms；t_first_action=1124ms；t_level_1=1124ms；t_level_2=2153ms；t_level_3=3207ms；t_level_4=4217ms；t_audio_only=4217ms |
| raw recovery timing | t_detect_recovering=5751ms；t_detect_stable=6783ms；t_detect_congested=662ms；t_first_action=5751ms；t_level_0=15049ms；t_level_1=11975ms；t_level_2=8792ms；t_level_3=5751ms |

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
| impairment timing | t_detect_warning=2229ms；t_detect_stable=220ms；t_detect_congested=4311ms；t_first_action=2229ms；t_level_1=2229ms；t_level_2=4311ms |
| recovery timing | t_detect_recovering=7465ms；t_detect_stable=8506ms；t_detect_congested=323ms；t_first_action=323ms；t_level_0=16633ms；t_level_1=13594ms；t_level_2=10560ms；t_level_3=323ms；t_level_4=1345ms；t_audio_only=1345ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2229ms；t_detect_stable=220ms；t_detect_congested=4311ms；t_first_action=2229ms；t_level_1=2229ms；t_level_2=4311ms |
| raw recovery timing | t_detect_recovering=7465ms；t_detect_stable=8506ms；t_detect_congested=323ms；t_first_action=323ms；t_level_0=16633ms；t_level_1=13594ms；t_level_2=10560ms；t_level_3=323ms；t_level_4=1345ms；t_audio_only=1345ms |

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
| impairment timing | t_detect_warning=1323ms；t_detect_stable=268ms；t_detect_congested=2354ms；t_first_action=1323ms；t_level_1=1323ms；t_level_2=2354ms；t_level_3=3395ms；t_level_4=4388ms；t_audio_only=4388ms |
| recovery timing | t_detect_recovering=4494ms；t_detect_stable=5531ms；t_detect_congested=449ms；t_first_action=4494ms；t_level_0=13742ms；t_level_1=10663ms；t_level_2=7585ms；t_level_3=4494ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1323ms；t_detect_stable=268ms；t_detect_congested=2354ms；t_first_action=1323ms；t_level_1=1323ms；t_level_2=2354ms；t_level_3=3395ms；t_level_4=4388ms；t_audio_only=4388ms |
| raw recovery timing | t_detect_recovering=4494ms；t_detect_stable=5531ms；t_detect_congested=449ms；t_first_action=4494ms；t_level_0=13742ms；t_level_1=10663ms；t_level_2=7585ms；t_level_3=4494ms |

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
| impairment timing | t_detect_warning=3175ms；t_detect_stable=86ms；t_first_action=3175ms；t_level_1=3175ms |
| recovery timing | t_detect_warning=210ms；t_detect_stable=3338ms；t_first_action=3338ms；t_level_0=3338ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3175ms；t_detect_stable=86ms；t_first_action=3175ms；t_level_1=3175ms |
| raw recovery timing | t_detect_warning=210ms；t_detect_stable=3338ms；t_first_action=3338ms；t_level_0=3338ms |

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
| impairment timing | t_detect_warning=2249ms；t_detect_stable=203ms；t_first_action=2249ms；t_level_1=2249ms |
| recovery timing | t_detect_warning=329ms；t_detect_stable=4501ms；t_first_action=4501ms；t_level_0=4501ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2249ms；t_detect_stable=203ms；t_first_action=2249ms；t_level_1=2249ms |
| raw recovery timing | t_detect_warning=329ms；t_detect_stable=4501ms；t_first_action=4501ms；t_level_0=4501ms |

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
| impairment timing | t_detect_stable=119ms |
| recovery timing | t_detect_stable=226ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=119ms |
| raw recovery timing | t_detect_stable=226ms |

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
| impairment timing | t_detect_warning=1020ms；t_detect_congested=2055ms；t_first_action=1020ms；t_level_1=1020ms；t_level_2=2055ms；t_level_3=3093ms；t_level_4=4128ms；t_audio_only=4128ms |
| recovery timing | t_detect_congested=119ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1020ms；t_detect_congested=2055ms；t_first_action=1020ms；t_level_1=1020ms；t_level_2=2055ms；t_level_3=3093ms；t_level_4=4128ms；t_audio_only=4128ms |
| raw recovery timing | t_detect_congested=119ms |

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
| impairment timing | t_detect_warning=1116ms；t_detect_stable=69ms；t_detect_congested=2152ms；t_first_action=1116ms；t_level_1=1116ms；t_level_2=2152ms；t_level_3=3197ms；t_level_4=4228ms；t_audio_only=4228ms |
| recovery timing | t_detect_congested=359ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1116ms；t_detect_stable=69ms；t_detect_congested=2152ms；t_first_action=1116ms；t_level_1=1116ms；t_level_2=2152ms；t_level_3=3197ms；t_level_4=4228ms；t_audio_only=4228ms |
| raw recovery timing | t_detect_congested=359ms |

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
| impairment timing | t_detect_warning=1122ms；t_detect_recovering=10351ms；t_detect_stable=75ms；t_detect_congested=2165ms；t_first_action=1122ms；t_level_1=1122ms；t_level_2=2165ms；t_level_3=3199ms；t_level_4=4232ms；t_audio_only=4232ms |
| recovery timing | t_detect_stable=449ms；t_first_action=2516ms；t_level_0=5619ms；t_level_1=2516ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1122ms；t_detect_recovering=10351ms；t_detect_stable=75ms；t_detect_congested=2165ms；t_first_action=1122ms；t_level_1=1122ms；t_level_2=2165ms；t_level_3=3199ms；t_level_4=4232ms；t_audio_only=4232ms |
| raw recovery timing | t_detect_stable=449ms；t_first_action=2516ms；t_level_0=5619ms；t_level_1=2516ms |

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
| impairment timing | t_detect_warning=1128ms；t_detect_recovering=10319ms；t_detect_stable=121ms；t_detect_congested=2141ms；t_first_action=1128ms；t_level_1=1128ms；t_level_2=2141ms；t_level_3=3177ms；t_level_4=4202ms；t_audio_only=4202ms |
| recovery timing | t_detect_stable=416ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1128ms；t_detect_recovering=10319ms；t_detect_stable=121ms；t_detect_congested=2141ms；t_first_action=1128ms；t_level_1=1128ms；t_level_2=2141ms；t_level_3=3177ms；t_level_4=4202ms；t_audio_only=4202ms |
| raw recovery timing | t_detect_stable=416ms |

