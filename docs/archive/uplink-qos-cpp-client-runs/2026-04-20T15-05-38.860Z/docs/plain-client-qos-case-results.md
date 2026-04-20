# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-20T15:05:38.860Z`

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
| impairment timing | t_detect_stable=171ms |
| recovery timing | t_detect_stable=277ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=171ms |
| raw recovery timing | t_detect_stable=277ms |

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
| impairment timing | t_detect_stable=208ms |
| recovery timing | t_detect_stable=463ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=208ms |
| raw recovery timing | t_detect_stable=463ms |

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
| impairment timing | t_detect_warning=513ms |
| recovery timing | t_detect_warning=179ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=513ms |
| raw recovery timing | t_detect_warning=179ms |

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
| impairment timing | t_detect_stable=165ms |
| recovery timing | t_detect_stable=432ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=165ms |
| raw recovery timing | t_detect_stable=432ms |

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
| impairment timing | t_detect_warning=1243ms；t_detect_recovering=10477ms；t_detect_stable=197ms；t_detect_congested=2283ms；t_first_action=1243ms；t_level_1=1243ms；t_level_2=2283ms；t_level_3=3321ms；t_level_4=4357ms；t_audio_only=4357ms |
| recovery timing | t_detect_recovering=3676ms；t_detect_stable=4676ms；t_detect_congested=637ms；t_first_action=3676ms；t_level_0=12884ms；t_level_1=9762ms；t_level_2=6720ms；t_level_3=3676ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1243ms；t_detect_recovering=10477ms；t_detect_stable=197ms；t_detect_congested=2283ms；t_first_action=1243ms；t_level_1=1243ms；t_level_2=2283ms；t_level_3=3321ms；t_level_4=4357ms；t_audio_only=4357ms |
| raw recovery timing | t_detect_recovering=3676ms；t_detect_stable=4676ms；t_detect_congested=637ms；t_first_action=3676ms；t_level_0=12884ms；t_level_1=9762ms；t_level_2=6720ms；t_level_3=3676ms |

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
| impairment timing | t_detect_warning=1191ms；t_detect_recovering=10336ms；t_detect_stable=138ms；t_detect_congested=2224ms；t_first_action=1191ms；t_level_1=1191ms；t_level_2=2224ms；t_level_3=3262ms；t_level_4=4297ms；t_audio_only=4297ms |
| recovery timing | t_detect_recovering=3402ms；t_detect_stable=4402ms；t_detect_congested=402ms；t_first_action=3402ms；t_level_0=12609ms；t_level_1=9489ms；t_level_2=6447ms；t_level_3=3402ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1191ms；t_detect_recovering=10336ms；t_detect_stable=138ms；t_detect_congested=2224ms；t_first_action=1191ms；t_level_1=1191ms；t_level_2=2224ms；t_level_3=3262ms；t_level_4=4297ms；t_audio_only=4297ms |
| raw recovery timing | t_detect_recovering=3402ms；t_detect_stable=4402ms；t_detect_congested=402ms；t_first_action=3402ms；t_level_0=12609ms；t_level_1=9489ms；t_level_2=6447ms；t_level_3=3402ms |

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
| impairment timing | t_detect_warning=1250ms；t_detect_stable=234ms；t_detect_congested=2278ms；t_first_action=1250ms；t_level_1=1250ms；t_level_2=2278ms；t_level_3=3317ms；t_level_4=4312ms；t_audio_only=4312ms |
| recovery timing | t_detect_recovering=4455ms；t_detect_stable=5455ms；t_detect_congested=455ms；t_first_action=4455ms；t_level_0=13669ms；t_level_1=10542ms；t_level_2=7500ms；t_level_3=4455ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1250ms；t_detect_stable=234ms；t_detect_congested=2278ms；t_first_action=1250ms；t_level_1=1250ms；t_level_2=2278ms；t_level_3=3317ms；t_level_4=4312ms；t_audio_only=4312ms |
| raw recovery timing | t_detect_recovering=4455ms；t_detect_stable=5455ms；t_detect_congested=455ms；t_first_action=4455ms；t_level_0=13669ms；t_level_1=10542ms；t_level_2=7500ms；t_level_3=4455ms |

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
| impairment timing | t_detect_warning=1177ms；t_detect_stable=158ms；t_detect_congested=2201ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2201ms；t_level_3=3241ms；t_level_4=4276ms；t_audio_only=4276ms |
| recovery timing | t_detect_recovering=4501ms；t_detect_stable=5501ms；t_detect_congested=422ms；t_first_action=4501ms；t_level_0=13788ms；t_level_1=10676ms；t_level_2=7548ms；t_level_3=4501ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1177ms；t_detect_stable=158ms；t_detect_congested=2201ms；t_first_action=1177ms；t_level_1=1177ms；t_level_2=2201ms；t_level_3=3241ms；t_level_4=4276ms；t_audio_only=4276ms |
| raw recovery timing | t_detect_recovering=4501ms；t_detect_stable=5501ms；t_detect_congested=422ms；t_first_action=4501ms；t_level_0=13788ms；t_level_1=10676ms；t_level_2=7548ms；t_level_3=4501ms |

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
| impairment timing | t_detect_warning=1343ms；t_detect_stable=327ms；t_detect_congested=2373ms；t_first_action=1343ms；t_level_1=1343ms；t_level_2=2373ms；t_level_3=3411ms；t_level_4=4447ms；t_audio_only=4447ms |
| recovery timing | t_detect_recovering=4462ms；t_detect_stable=5462ms；t_detect_congested=462ms；t_first_action=4462ms；t_level_0=13668ms；t_level_1=10549ms；t_level_2=7509ms；t_level_3=4462ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1343ms；t_detect_stable=327ms；t_detect_congested=2373ms；t_first_action=1343ms；t_level_1=1343ms；t_level_2=2373ms；t_level_3=3411ms；t_level_4=4447ms；t_audio_only=4447ms |
| raw recovery timing | t_detect_recovering=4462ms；t_detect_stable=5462ms；t_detect_congested=462ms；t_first_action=4462ms；t_level_0=13668ms；t_level_1=10549ms；t_level_2=7509ms；t_level_3=4462ms |

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
| impairment timing | t_detect_stable=172ms |
| recovery timing | t_detect_stable=436ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=172ms |
| raw recovery timing | t_detect_stable=436ms |

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
| impairment timing | t_detect_stable=13ms |
| recovery timing | t_detect_stable=316ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=13ms |
| raw recovery timing | t_detect_stable=316ms |

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
| impairment timing | t_detect_stable=92ms |
| recovery timing | t_detect_stable=120ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=92ms |
| raw recovery timing | t_detect_stable=120ms |

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
| impairment timing | t_detect_warning=4045ms；t_detect_stable=40ms；t_first_action=4045ms；t_level_1=4045ms |
| recovery timing | t_detect_warning=501ms；t_detect_stable=3625ms；t_first_action=3625ms；t_level_0=3625ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4045ms；t_detect_stable=40ms；t_first_action=4045ms；t_level_1=4045ms |
| raw recovery timing | t_detect_warning=501ms；t_detect_stable=3625ms；t_first_action=3625ms；t_level_0=3625ms |

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
| impairment timing | t_detect_warning=2159ms；t_detect_recovering=14312ms；t_detect_stable=153ms；t_detect_congested=4238ms；t_first_action=2159ms；t_level_1=2159ms；t_level_2=4238ms；t_level_3=5277ms；t_level_4=6273ms；t_audio_only=6273ms |
| recovery timing | t_detect_warning=379ms；t_detect_stable=4463ms；t_first_action=4463ms；t_level_0=10668ms；t_level_1=7584ms；t_level_2=4463ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2159ms；t_detect_recovering=14312ms；t_detect_stable=153ms；t_detect_congested=4238ms；t_first_action=2159ms；t_level_1=2159ms；t_level_2=4238ms；t_level_3=5277ms；t_level_4=6273ms；t_audio_only=6273ms |
| raw recovery timing | t_detect_warning=379ms；t_detect_stable=4463ms；t_first_action=4463ms；t_level_0=10668ms；t_level_1=7584ms；t_level_2=4463ms |

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
| impairment timing | t_detect_warning=1316ms；t_detect_recovering=15500ms；t_detect_stable=266ms；t_detect_congested=2347ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2347ms；t_level_3=3390ms；t_level_4=4422ms；t_audio_only=4422ms |
| recovery timing | t_detect_recovering=7606ms；t_detect_stable=8606ms；t_detect_congested=566ms；t_first_action=7606ms；t_level_0=16828ms；t_level_1=13693ms；t_level_2=10651ms；t_level_3=7606ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1316ms；t_detect_recovering=15500ms；t_detect_stable=266ms；t_detect_congested=2347ms；t_first_action=1316ms；t_level_1=1316ms；t_level_2=2347ms；t_level_3=3390ms；t_level_4=4422ms；t_audio_only=4422ms |
| raw recovery timing | t_detect_recovering=7606ms；t_detect_stable=8606ms；t_detect_congested=566ms；t_first_action=7606ms；t_level_0=16828ms；t_level_1=13693ms；t_level_2=10651ms；t_level_3=7606ms |

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
| impairment timing | t_detect_warning=1125ms；t_detect_stable=83ms；t_detect_congested=2165ms；t_first_action=1125ms；t_level_1=1125ms；t_level_2=2165ms；t_level_3=3204ms；t_level_4=4200ms；t_audio_only=4200ms |
| recovery timing | t_detect_recovering=4224ms；t_detect_stable=5225ms；t_detect_congested=224ms；t_first_action=4224ms；t_level_0=13431ms；t_level_1=10309ms；t_level_2=7268ms；t_level_3=4224ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1125ms；t_detect_stable=83ms；t_detect_congested=2165ms；t_first_action=1125ms；t_level_1=1125ms；t_level_2=2165ms；t_level_3=3204ms；t_level_4=4200ms；t_audio_only=4200ms |
| raw recovery timing | t_detect_recovering=4224ms；t_detect_stable=5225ms；t_detect_congested=224ms；t_first_action=4224ms；t_level_0=13431ms；t_level_1=10309ms；t_level_2=7268ms；t_level_3=4224ms |

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
| impairment timing | t_detect_warning=1280ms；t_detect_stable=275ms；t_detect_congested=2325ms；t_first_action=1280ms；t_level_1=1280ms；t_level_2=2325ms；t_level_3=3361ms；t_level_4=4395ms；t_audio_only=4395ms |
| recovery timing | t_detect_recovering=4499ms；t_detect_stable=5499ms；t_detect_congested=499ms；t_first_action=4499ms；t_level_0=13705ms；t_level_1=10585ms；t_level_2=7544ms；t_level_3=4499ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1280ms；t_detect_stable=275ms；t_detect_congested=2325ms；t_first_action=1280ms；t_level_1=1280ms；t_level_2=2325ms；t_level_3=3361ms；t_level_4=4395ms；t_audio_only=4395ms |
| raw recovery timing | t_detect_recovering=4499ms；t_detect_stable=5499ms；t_detect_congested=499ms；t_first_action=4499ms；t_level_0=13705ms；t_level_1=10585ms；t_level_2=7544ms；t_level_3=4499ms |

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
| impairment timing | t_detect_stable=164ms |
| recovery timing | t_detect_stable=430ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=164ms |
| raw recovery timing | t_detect_stable=430ms |

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
| impairment timing | t_detect_stable=199ms |
| recovery timing | t_detect_stable=423ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=199ms |
| raw recovery timing | t_detect_stable=423ms |

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
| impairment timing | t_detect_stable=960ms |
| recovery timing | t_detect_stable=176ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=960ms |
| raw recovery timing | t_detect_stable=176ms |

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
| impairment timing | t_detect_warning=4277ms；t_detect_stable=272ms；t_first_action=4277ms；t_level_1=4277ms |
| recovery timing | t_detect_warning=818ms；t_detect_stable=2903ms；t_first_action=2903ms；t_level_0=2903ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4277ms；t_detect_stable=272ms；t_first_action=4277ms；t_level_1=4277ms |
| raw recovery timing | t_detect_warning=818ms；t_detect_stable=2903ms；t_first_action=2903ms；t_level_0=2903ms |

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
| impairment timing | t_detect_warning=2187ms；t_detect_stable=143ms；t_detect_congested=6349ms；t_first_action=2187ms；t_level_1=2187ms；t_level_2=6349ms；t_level_3=7385ms；t_level_4=8421ms；t_audio_only=8421ms |
| recovery timing | t_detect_recovering=5647ms；t_detect_stable=6647ms；t_detect_congested=568ms；t_first_action=5647ms；t_level_0=14854ms；t_level_1=11733ms；t_level_2=8692ms；t_level_3=5647ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2187ms；t_detect_stable=143ms；t_detect_congested=6349ms；t_first_action=2187ms；t_level_1=2187ms；t_level_2=6349ms；t_level_3=7385ms；t_level_4=8421ms；t_audio_only=8421ms |
| raw recovery timing | t_detect_recovering=5647ms；t_detect_stable=6647ms；t_detect_congested=568ms；t_first_action=5647ms；t_level_0=14854ms；t_level_1=11733ms；t_level_2=8692ms；t_level_3=5647ms |

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
| impairment timing | t_detect_warning=2113ms；t_detect_stable=68ms；t_detect_congested=3153ms；t_first_action=2113ms；t_level_1=2113ms；t_level_2=3153ms；t_level_3=4192ms；t_level_4=5227ms；t_audio_only=5227ms |
| recovery timing | t_detect_recovering=6491ms；t_detect_stable=7490ms；t_detect_congested=410ms；t_first_action=6491ms；t_level_0=15703ms；t_level_1=12577ms；t_level_2=9534ms；t_level_3=6491ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2113ms；t_detect_stable=68ms；t_detect_congested=3153ms；t_first_action=2113ms；t_level_1=2113ms；t_level_2=3153ms；t_level_3=4192ms；t_level_4=5227ms；t_audio_only=5227ms |
| raw recovery timing | t_detect_recovering=6491ms；t_detect_stable=7490ms；t_detect_congested=410ms；t_first_action=6491ms；t_level_0=15703ms；t_level_1=12577ms；t_level_2=9534ms；t_level_3=6491ms |

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
| impairment timing | t_detect_stable=189ms |
| recovery timing | t_detect_stable=490ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=189ms |
| raw recovery timing | t_detect_stable=490ms |

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
| impairment timing | t_detect_stable=207ms |
| recovery timing | t_detect_stable=393ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=207ms |
| raw recovery timing | t_detect_stable=393ms |

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
| impairment timing | t_detect_warning=3244ms；t_detect_stable=159ms；t_first_action=3244ms；t_level_1=3244ms |
| recovery timing | t_detect_warning=824ms；t_detect_stable=2909ms；t_first_action=2909ms；t_level_0=2909ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3244ms；t_detect_stable=159ms；t_first_action=3244ms；t_level_1=3244ms |
| raw recovery timing | t_detect_warning=824ms；t_detect_stable=2909ms；t_first_action=2909ms；t_level_0=2909ms |

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
| impairment timing | t_detect_warning=2203ms；t_detect_stable=196ms；t_first_action=2203ms；t_level_1=2203ms |
| recovery timing | t_detect_warning=823ms；t_detect_stable=4988ms；t_first_action=4988ms；t_level_0=4988ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2203ms；t_detect_stable=196ms；t_first_action=2203ms；t_level_1=2203ms |
| raw recovery timing | t_detect_warning=823ms；t_detect_stable=4988ms；t_first_action=4988ms；t_level_0=4988ms |

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
| impairment timing | t_detect_warning=1258ms；t_detect_recovering=10401ms；t_detect_stable=243ms；t_detect_congested=2289ms；t_first_action=1258ms；t_level_0=19687ms；t_level_1=1258ms；t_level_2=2289ms；t_level_3=3332ms；t_level_4=4362ms；t_audio_only=4362ms |
| recovery timing | t_detect_stable=656ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1258ms；t_detect_recovering=10401ms；t_detect_stable=243ms；t_detect_congested=2289ms；t_first_action=1258ms；t_level_0=19687ms；t_level_1=1258ms；t_level_2=2289ms；t_level_3=3332ms；t_level_4=4362ms；t_audio_only=4362ms |
| raw recovery timing | t_detect_stable=656ms |

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
| impairment timing | t_detect_stable=155ms |
| recovery timing | t_detect_stable=302ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=155ms |
| raw recovery timing | t_detect_stable=302ms |

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
| impairment timing | t_detect_warning=1093ms；t_detect_recovering=10247ms；t_detect_stable=88ms；t_detect_congested=2133ms；t_first_action=1093ms；t_level_1=1093ms；t_level_2=2133ms；t_level_3=3172ms；t_level_4=4208ms；t_audio_only=4208ms |
| recovery timing | t_detect_recovering=3345ms；t_detect_stable=4345ms；t_detect_congested=345ms；t_first_action=3345ms；t_level_0=12550ms；t_level_1=9438ms；t_level_2=6389ms；t_level_3=3345ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1093ms；t_detect_recovering=10247ms；t_detect_stable=88ms；t_detect_congested=2133ms；t_first_action=1093ms；t_level_1=1093ms；t_level_2=2133ms；t_level_3=3172ms；t_level_4=4208ms；t_audio_only=4208ms |
| raw recovery timing | t_detect_recovering=3345ms；t_detect_stable=4345ms；t_detect_congested=345ms；t_first_action=3345ms；t_level_0=12550ms；t_level_1=9438ms；t_level_2=6389ms；t_level_3=3345ms |

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
| impairment timing | t_detect_warning=1149ms；t_detect_stable=142ms；t_detect_congested=2188ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2188ms；t_level_3=3227ms；t_level_4=4262ms；t_audio_only=4262ms |
| recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13538ms；t_level_1=10416ms；t_level_2=7373ms；t_level_3=4328ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1149ms；t_detect_stable=142ms；t_detect_congested=2188ms；t_first_action=1149ms；t_level_1=1149ms；t_level_2=2188ms；t_level_3=3227ms；t_level_4=4262ms；t_audio_only=4262ms |
| raw recovery timing | t_detect_recovering=4328ms；t_detect_stable=5328ms；t_detect_congested=328ms；t_first_action=4328ms；t_level_0=13538ms；t_level_1=10416ms；t_level_2=7373ms；t_level_3=4328ms |

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
| impairment timing | t_detect_warning=4241ms；t_detect_stable=197ms；t_first_action=4241ms；t_level_1=4241ms |
| recovery timing | t_detect_warning=863ms；t_detect_stable=2918ms；t_first_action=2918ms；t_level_0=2918ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4241ms；t_detect_stable=197ms；t_first_action=4241ms；t_level_1=4241ms |
| raw recovery timing | t_detect_warning=863ms；t_detect_stable=2918ms；t_first_action=2918ms；t_level_0=2918ms |

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
| impairment timing | t_detect_warning=1054ms；t_detect_recovering=15248ms；t_detect_stable=49ms；t_detect_congested=2093ms；t_first_action=1054ms；t_level_1=1054ms；t_level_2=2093ms；t_level_3=3133ms；t_level_4=4169ms；t_audio_only=4169ms |
| recovery timing | t_detect_recovering=7395ms；t_detect_stable=8435ms；t_detect_congested=314ms；t_first_action=7395ms；t_level_0=16647ms；t_level_1=13601ms；t_level_2=10479ms；t_level_3=7395ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1054ms；t_detect_recovering=15248ms；t_detect_stable=49ms；t_detect_congested=2093ms；t_first_action=1054ms；t_level_1=1054ms；t_level_2=2093ms；t_level_3=3133ms；t_level_4=4169ms；t_audio_only=4169ms |
| raw recovery timing | t_detect_recovering=7395ms；t_detect_stable=8435ms；t_detect_congested=314ms；t_first_action=7395ms；t_level_0=16647ms；t_level_1=13601ms；t_level_2=10479ms；t_level_3=7395ms |

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
| impairment timing | t_detect_warning=4152ms；t_detect_stable=108ms；t_first_action=4152ms；t_level_1=4152ms |
| recovery timing | t_detect_warning=770ms；t_detect_stable=2856ms；t_first_action=2856ms；t_level_0=2856ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4152ms；t_detect_stable=108ms；t_first_action=4152ms；t_level_1=4152ms |
| raw recovery timing | t_detect_warning=770ms；t_detect_stable=2856ms；t_first_action=2856ms；t_level_0=2856ms |

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
| impairment timing | t_detect_warning=3041ms；t_detect_stable=1035ms；t_first_action=3041ms；t_level_1=3041ms |
| recovery timing | t_detect_warning=702ms；t_detect_stable=3827ms；t_first_action=3827ms；t_level_0=3827ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3041ms；t_detect_stable=1035ms；t_first_action=3041ms；t_level_1=3041ms |
| raw recovery timing | t_detect_warning=702ms；t_detect_stable=3827ms；t_first_action=3827ms；t_level_0=3827ms |

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
| impairment timing | t_detect_warning=1255ms；t_detect_stable=211ms；t_detect_congested=2296ms；t_first_action=1255ms；t_level_1=1255ms；t_level_2=2296ms；t_level_3=3335ms；t_level_4=4371ms；t_audio_only=4371ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1255ms；t_detect_stable=211ms；t_detect_congested=2296ms；t_first_action=1255ms；t_level_1=1255ms；t_level_2=2296ms；t_level_3=3335ms；t_level_4=4371ms；t_audio_only=4371ms |
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
| impairment timing | t_detect_warning=1040ms；t_detect_congested=2080ms；t_first_action=1040ms；t_level_1=1040ms；t_level_2=2080ms；t_level_3=3119ms；t_level_4=4155ms；t_audio_only=4155ms |
| recovery timing | t_detect_recovering=7500ms；t_detect_stable=8501ms；t_detect_congested=500ms；t_first_action=7500ms；t_level_0=16706ms；t_level_1=13585ms；t_level_2=10544ms；t_level_3=7500ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1040ms；t_detect_congested=2080ms；t_first_action=1040ms；t_level_1=1040ms；t_level_2=2080ms；t_level_3=3119ms；t_level_4=4155ms；t_audio_only=4155ms |
| raw recovery timing | t_detect_recovering=7500ms；t_detect_stable=8501ms；t_detect_congested=500ms；t_first_action=7500ms；t_level_0=16706ms；t_level_1=13585ms；t_level_2=10544ms；t_level_3=7500ms |

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
| impairment timing | t_detect_warning=1153ms；t_detect_stable=148ms；t_detect_congested=2194ms；t_first_action=1153ms；t_level_1=1153ms；t_level_2=2194ms；t_level_3=3233ms；t_level_4=4268ms；t_audio_only=4268ms |
| recovery timing | t_detect_recovering=7494ms；t_detect_stable=8494ms；t_detect_congested=454ms；t_first_action=7494ms；t_level_0=16666ms；t_level_1=13580ms；t_level_2=10539ms；t_level_3=7494ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1153ms；t_detect_stable=148ms；t_detect_congested=2194ms；t_first_action=1153ms；t_level_1=1153ms；t_level_2=2194ms；t_level_3=3233ms；t_level_4=4268ms；t_audio_only=4268ms |
| raw recovery timing | t_detect_recovering=7494ms；t_detect_stable=8494ms；t_detect_congested=454ms；t_first_action=7494ms；t_level_0=16666ms；t_level_1=13580ms；t_level_2=10539ms；t_level_3=7494ms |

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
| impairment timing | t_detect_warning=1126ms；t_detect_stable=108ms；t_detect_congested=2166ms；t_first_action=1126ms；t_level_1=1126ms；t_level_2=2166ms；t_level_3=3214ms；t_level_4=4240ms；t_audio_only=4240ms |
| recovery timing | t_detect_recovering=6586ms；t_detect_stable=7586ms；t_detect_congested=586ms；t_first_action=6586ms；t_level_0=15792ms；t_level_1=12672ms；t_level_2=9633ms；t_level_3=6586ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1126ms；t_detect_stable=108ms；t_detect_congested=2166ms；t_first_action=1126ms；t_level_1=1126ms；t_level_2=2166ms；t_level_3=3214ms；t_level_4=4240ms；t_audio_only=4240ms |
| raw recovery timing | t_detect_recovering=6586ms；t_detect_stable=7586ms；t_detect_congested=586ms；t_first_action=6586ms；t_level_0=15792ms；t_level_1=12672ms；t_level_2=9633ms；t_level_3=6586ms |

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
| impairment timing | t_detect_warning=2261ms；t_detect_stable=256ms；t_detect_congested=4340ms；t_first_action=2261ms；t_level_1=2261ms；t_level_2=4340ms |
| recovery timing | t_detect_recovering=7436ms；t_detect_stable=8436ms；t_detect_congested=362ms；t_first_action=362ms；t_level_0=16602ms；t_level_1=13522ms；t_level_2=10481ms；t_level_3=362ms；t_level_4=1397ms；t_audio_only=1397ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2261ms；t_detect_stable=256ms；t_detect_congested=4340ms；t_first_action=2261ms；t_level_1=2261ms；t_level_2=4340ms |
| raw recovery timing | t_detect_recovering=7436ms；t_detect_stable=8436ms；t_detect_congested=362ms；t_first_action=362ms；t_level_0=16602ms；t_level_1=13522ms；t_level_2=10481ms；t_level_3=362ms；t_level_4=1397ms；t_audio_only=1397ms |

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
| impairment timing | t_detect_warning=1291ms；t_detect_stable=244ms；t_detect_congested=2328ms；t_first_action=1291ms；t_level_1=1291ms；t_level_2=2328ms；t_level_3=3368ms；t_level_4=4401ms；t_audio_only=4401ms |
| recovery timing | t_detect_recovering=4468ms；t_detect_stable=5508ms；t_detect_congested=427ms；t_first_action=4468ms；t_level_0=13713ms；t_level_1=10673ms；t_level_2=7553ms；t_level_3=4468ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1291ms；t_detect_stable=244ms；t_detect_congested=2328ms；t_first_action=1291ms；t_level_1=1291ms；t_level_2=2328ms；t_level_3=3368ms；t_level_4=4401ms；t_audio_only=4401ms |
| raw recovery timing | t_detect_recovering=4468ms；t_detect_stable=5508ms；t_detect_congested=427ms；t_first_action=4468ms；t_level_0=13713ms；t_level_1=10673ms；t_level_2=7553ms；t_level_3=4468ms |

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
| impairment timing | t_detect_warning=3083ms；t_detect_stable=76ms；t_first_action=3083ms；t_level_1=3083ms |
| recovery timing | t_detect_warning=141ms；t_detect_stable=3267ms；t_first_action=3267ms；t_level_0=3267ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3083ms；t_detect_stable=76ms；t_first_action=3083ms；t_level_1=3083ms |
| raw recovery timing | t_detect_warning=141ms；t_detect_stable=3267ms；t_first_action=3267ms；t_level_0=3267ms |

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
| impairment timing | t_detect_warning=2084ms；t_detect_stable=74ms；t_first_action=2084ms；t_level_1=2084ms |
| recovery timing | t_detect_warning=184ms；t_detect_stable=4347ms；t_first_action=4347ms；t_level_0=4347ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2084ms；t_detect_stable=74ms；t_first_action=2084ms；t_level_1=2084ms |
| raw recovery timing | t_detect_warning=184ms；t_detect_stable=4347ms；t_first_action=4347ms；t_level_0=4347ms |

