# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-19T13:57:28.283Z`

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
| impairment timing | t_detect_stable=151ms |
| recovery timing | t_detect_stable=333ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=151ms |
| raw recovery timing | t_detect_stable=333ms |

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
| impairment timing | t_detect_stable=1ms |
| recovery timing | t_detect_stable=269ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=1ms |
| raw recovery timing | t_detect_stable=269ms |

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
| impairment timing | t_detect_warning=445ms |
| recovery timing | t_detect_warning=110ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=445ms |
| raw recovery timing | t_detect_warning=110ms |

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
| impairment timing | t_detect_stable=26ms |
| recovery timing | t_detect_stable=448ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=26ms |
| raw recovery timing | t_detect_stable=448ms |

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
| impairment timing | t_detect_warning=1168ms；t_detect_recovering=10320ms；t_detect_stable=162ms；t_detect_congested=2206ms；t_first_action=1168ms；t_level_1=1168ms；t_level_2=2206ms；t_level_3=3246ms；t_level_4=4281ms；t_audio_only=4281ms |
| recovery timing | t_detect_recovering=3464ms；t_detect_stable=4505ms；t_detect_congested=463ms；t_first_action=3464ms；t_level_0=12755ms；t_level_1=9669ms；t_level_2=6549ms；t_level_3=3464ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1168ms；t_detect_recovering=10320ms；t_detect_stable=162ms；t_detect_congested=2206ms；t_first_action=1168ms；t_level_1=1168ms；t_level_2=2206ms；t_level_3=3246ms；t_level_4=4281ms；t_audio_only=4281ms |
| raw recovery timing | t_detect_recovering=3464ms；t_detect_stable=4505ms；t_detect_congested=463ms；t_first_action=3464ms；t_level_0=12755ms；t_level_1=9669ms；t_level_2=6549ms；t_level_3=3464ms |

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
| impairment timing | t_detect_warning=1315ms；t_detect_recovering=10468ms；t_detect_stable=273ms；t_detect_congested=2355ms；t_first_action=1315ms；t_level_1=1315ms；t_level_2=2355ms；t_level_3=3400ms；t_level_4=4429ms；t_audio_only=4429ms |
| recovery timing | t_detect_recovering=3570ms；t_detect_stable=4570ms；t_detect_congested=570ms；t_first_action=3570ms；t_level_0=12782ms；t_level_1=9658ms；t_level_2=6614ms；t_level_3=3570ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1315ms；t_detect_recovering=10468ms；t_detect_stable=273ms；t_detect_congested=2355ms；t_first_action=1315ms；t_level_1=1315ms；t_level_2=2355ms；t_level_3=3400ms；t_level_4=4429ms；t_audio_only=4429ms |
| raw recovery timing | t_detect_recovering=3570ms；t_detect_stable=4570ms；t_detect_congested=570ms；t_first_action=3570ms；t_level_0=12782ms；t_level_1=9658ms；t_level_2=6614ms；t_level_3=3570ms |

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
| impairment timing | t_detect_warning=1089ms；t_detect_stable=83ms；t_detect_congested=2127ms；t_first_action=1089ms；t_level_1=1089ms；t_level_2=2127ms；t_level_3=3131ms；t_level_4=4163ms；t_audio_only=4163ms |
| recovery timing | t_detect_recovering=4507ms；t_detect_stable=5505ms；t_detect_congested=386ms；t_first_action=4507ms；t_level_0=13751ms；t_level_1=10633ms；t_level_2=7517ms；t_level_3=4507ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1089ms；t_detect_stable=83ms；t_detect_congested=2127ms；t_first_action=1089ms；t_level_1=1089ms；t_level_2=2127ms；t_level_3=3131ms；t_level_4=4163ms；t_audio_only=4163ms |
| raw recovery timing | t_detect_recovering=4507ms；t_detect_stable=5505ms；t_detect_congested=386ms；t_first_action=4507ms；t_level_0=13751ms；t_level_1=10633ms；t_level_2=7517ms；t_level_3=4507ms |

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
| impairment timing | t_detect_warning=1242ms；t_detect_stable=187ms；t_detect_congested=2270ms；t_first_action=1242ms；t_level_1=1242ms；t_level_2=2270ms；t_level_3=3310ms；t_level_4=4345ms；t_audio_only=4345ms |
| recovery timing | t_detect_recovering=4372ms；t_detect_stable=5411ms；t_detect_congested=370ms；t_first_action=4372ms；t_level_0=13698ms；t_level_1=10577ms；t_level_2=7455ms；t_level_3=4372ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1242ms；t_detect_stable=187ms；t_detect_congested=2270ms；t_first_action=1242ms；t_level_1=1242ms；t_level_2=2270ms；t_level_3=3310ms；t_level_4=4345ms；t_audio_only=4345ms |
| raw recovery timing | t_detect_recovering=4372ms；t_detect_stable=5411ms；t_detect_congested=370ms；t_first_action=4372ms；t_level_0=13698ms；t_level_1=10577ms；t_level_2=7455ms；t_level_3=4372ms |

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
| impairment timing | t_detect_warning=1013ms；t_detect_stable=6ms；t_detect_congested=2050ms；t_first_action=1013ms；t_level_1=1013ms；t_level_2=2050ms；t_level_3=3094ms；t_level_4=4126ms；t_audio_only=4126ms |
| recovery timing | t_detect_recovering=4390ms；t_detect_stable=5428ms；t_detect_congested=348ms；t_first_action=4390ms；t_level_0=13714ms；t_level_1=10600ms；t_level_2=7474ms；t_level_3=4390ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1013ms；t_detect_stable=6ms；t_detect_congested=2050ms；t_first_action=1013ms；t_level_1=1013ms；t_level_2=2050ms；t_level_3=3094ms；t_level_4=4126ms；t_audio_only=4126ms |
| raw recovery timing | t_detect_recovering=4390ms；t_detect_stable=5428ms；t_detect_congested=348ms；t_first_action=4390ms；t_level_0=13714ms；t_level_1=10600ms；t_level_2=7474ms；t_level_3=4390ms |

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
| impairment timing | t_detect_stable=77ms |
| recovery timing | t_detect_stable=141ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=77ms |
| raw recovery timing | t_detect_stable=141ms |

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
| impairment timing | t_detect_stable=950ms |
| recovery timing | t_detect_stable=976ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=950ms |
| raw recovery timing | t_detect_stable=976ms |

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
| impairment timing | t_detect_stable=249ms |
| recovery timing | t_detect_stable=513ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=249ms |
| raw recovery timing | t_detect_stable=513ms |

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
| impairment timing | t_detect_warning=4262ms；t_detect_stable=212ms；t_first_action=4262ms；t_level_1=4262ms |
| recovery timing | t_detect_warning=876ms；t_detect_stable=2960ms；t_first_action=2960ms；t_level_0=2960ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4262ms；t_detect_stable=212ms；t_first_action=4262ms；t_level_1=4262ms |
| raw recovery timing | t_detect_warning=876ms；t_detect_stable=2960ms；t_first_action=2960ms；t_level_0=2960ms |

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
| impairment timing | t_detect_warning=2124ms；t_detect_recovering=14316ms；t_detect_stable=118ms；t_detect_congested=4203ms；t_first_action=2124ms；t_level_1=2124ms；t_level_2=4203ms；t_level_3=5242ms；t_level_4=6237ms；t_audio_only=6237ms |
| recovery timing | t_detect_warning=382ms；t_detect_stable=4467ms；t_first_action=4467ms；t_level_0=10638ms；t_level_1=7508ms；t_level_2=4467ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2124ms；t_detect_recovering=14316ms；t_detect_stable=118ms；t_detect_congested=4203ms；t_first_action=2124ms；t_level_1=2124ms；t_level_2=4203ms；t_level_3=5242ms；t_level_4=6237ms；t_audio_only=6237ms |
| raw recovery timing | t_detect_warning=382ms；t_detect_stable=4467ms；t_first_action=4467ms；t_level_0=10638ms；t_level_1=7508ms；t_level_2=4467ms |

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
| impairment timing | t_detect_warning=1270ms；t_detect_recovering=15463ms；t_detect_stable=264ms；t_detect_congested=2309ms；t_first_action=1270ms；t_level_1=1270ms；t_level_2=2309ms；t_level_3=3349ms；t_level_4=4384ms；t_audio_only=4384ms |
| recovery timing | t_detect_recovering=7529ms；t_detect_stable=8528ms；t_detect_congested=529ms；t_first_action=7529ms；t_level_0=16735ms；t_level_1=13616ms；t_level_2=10573ms；t_level_3=7529ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1270ms；t_detect_recovering=15463ms；t_detect_stable=264ms；t_detect_congested=2309ms；t_first_action=1270ms；t_level_1=1270ms；t_level_2=2309ms；t_level_3=3349ms；t_level_4=4384ms；t_audio_only=4384ms |
| raw recovery timing | t_detect_recovering=7529ms；t_detect_stable=8528ms；t_detect_congested=529ms；t_first_action=7529ms；t_level_0=16735ms；t_level_1=13616ms；t_level_2=10573ms；t_level_3=7529ms |

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
| impairment timing | t_detect_warning=1165ms；t_detect_stable=120ms；t_detect_congested=2176ms；t_first_action=1165ms；t_level_1=1165ms；t_level_2=2176ms；t_level_3=3203ms；t_level_4=4239ms；t_audio_only=4239ms |
| recovery timing | t_detect_recovering=4261ms；t_detect_stable=5261ms；t_detect_congested=261ms；t_first_action=4261ms；t_level_0=13467ms；t_level_1=10347ms；t_level_2=7307ms；t_level_3=4261ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1165ms；t_detect_stable=120ms；t_detect_congested=2176ms；t_first_action=1165ms；t_level_1=1165ms；t_level_2=2176ms；t_level_3=3203ms；t_level_4=4239ms；t_audio_only=4239ms |
| raw recovery timing | t_detect_recovering=4261ms；t_detect_stable=5261ms；t_detect_congested=261ms；t_first_action=4261ms；t_level_0=13467ms；t_level_1=10347ms；t_level_2=7307ms；t_level_3=4261ms |

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
| impairment timing | t_detect_warning=1196ms；t_detect_stable=191ms；t_detect_congested=2246ms；t_first_action=1196ms；t_level_1=1196ms；t_level_2=2246ms；t_level_3=3276ms；t_level_4=4311ms；t_audio_only=4311ms |
| recovery timing | t_detect_recovering=4412ms；t_detect_stable=5412ms；t_detect_congested=333ms；t_first_action=4412ms；t_level_0=13618ms；t_level_1=10497ms；t_level_2=7458ms；t_level_3=4412ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1196ms；t_detect_stable=191ms；t_detect_congested=2246ms；t_first_action=1196ms；t_level_1=1196ms；t_level_2=2246ms；t_level_3=3276ms；t_level_4=4311ms；t_audio_only=4311ms |
| raw recovery timing | t_detect_recovering=4412ms；t_detect_stable=5412ms；t_detect_congested=333ms；t_first_action=4412ms；t_level_0=13618ms；t_level_1=10497ms；t_level_2=7458ms；t_level_3=4412ms |

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
| impairment timing | t_detect_stable=155ms |
| recovery timing | t_detect_stable=581ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=155ms |
| raw recovery timing | t_detect_stable=581ms |

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
| impairment timing | t_detect_stable=146ms |
| recovery timing | t_detect_stable=290ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=146ms |
| raw recovery timing | t_detect_stable=290ms |

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
| impairment timing | t_detect_stable=10ms |
| recovery timing | t_detect_stable=193ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=10ms |
| raw recovery timing | t_detect_stable=193ms |

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
| impairment timing | t_detect_warning=4176ms；t_detect_stable=131ms；t_first_action=4176ms；t_level_1=4176ms |
| recovery timing | t_detect_warning=791ms；t_detect_stable=2875ms；t_first_action=2875ms；t_level_0=2875ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4176ms；t_detect_stable=131ms；t_first_action=4176ms；t_level_1=4176ms |
| raw recovery timing | t_detect_warning=791ms；t_detect_stable=2875ms；t_first_action=2875ms；t_level_0=2875ms |

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
| impairment timing | t_detect_warning=2243ms；t_detect_stable=161ms；t_detect_congested=6402ms；t_first_action=2243ms；t_level_1=2243ms；t_level_2=6402ms；t_level_3=7442ms；t_level_4=8479ms；t_audio_only=8479ms |
| recovery timing | t_detect_recovering=5704ms；t_detect_stable=6704ms；t_detect_congested=623ms；t_first_action=5704ms；t_level_0=14910ms；t_level_1=11791ms；t_level_2=8748ms；t_level_3=5704ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2243ms；t_detect_stable=161ms；t_detect_congested=6402ms；t_first_action=2243ms；t_level_1=2243ms；t_level_2=6402ms；t_level_3=7442ms；t_level_4=8479ms；t_audio_only=8479ms |
| raw recovery timing | t_detect_recovering=5704ms；t_detect_stable=6704ms；t_detect_congested=623ms；t_first_action=5704ms；t_level_0=14910ms；t_level_1=11791ms；t_level_2=8748ms；t_level_3=5704ms |

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
| impairment timing | t_detect_warning=2204ms；t_detect_stable=160ms；t_detect_congested=3246ms；t_first_action=2204ms；t_level_1=2204ms；t_level_2=3246ms；t_level_3=4285ms；t_level_4=5319ms；t_audio_only=5319ms |
| recovery timing | t_detect_recovering=6343ms；t_detect_stable=7343ms；t_detect_congested=343ms；t_first_action=6343ms；t_level_0=15550ms；t_level_1=12428ms；t_level_2=9389ms；t_level_3=6343ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2204ms；t_detect_stable=160ms；t_detect_congested=3246ms；t_first_action=2204ms；t_level_1=2204ms；t_level_2=3246ms；t_level_3=4285ms；t_level_4=5319ms；t_audio_only=5319ms |
| raw recovery timing | t_detect_recovering=6343ms；t_detect_stable=7343ms；t_detect_congested=343ms；t_first_action=6343ms；t_level_0=15550ms；t_level_1=12428ms；t_level_2=9389ms；t_level_3=6343ms |

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
| impairment timing | t_detect_stable=1000ms |
| recovery timing | t_detect_stable=106ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=1000ms |
| raw recovery timing | t_detect_stable=106ms |

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
| impairment timing | t_detect_stable=115ms |
| recovery timing | t_detect_stable=342ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=115ms |
| raw recovery timing | t_detect_stable=342ms |

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
| impairment timing | t_detect_warning=3200ms；t_detect_stable=155ms；t_first_action=3200ms；t_level_1=3200ms |
| recovery timing | t_detect_warning=858ms；t_detect_stable=2947ms；t_first_action=2947ms；t_level_0=2947ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3200ms；t_detect_stable=155ms；t_first_action=3200ms；t_level_1=3200ms |
| raw recovery timing | t_detect_warning=858ms；t_detect_stable=2947ms；t_first_action=2947ms；t_level_0=2947ms |

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
| impairment timing | t_detect_warning=2114ms；t_detect_stable=70ms；t_first_action=2114ms；t_level_1=2114ms |
| recovery timing | t_detect_warning=733ms；t_detect_stable=4896ms；t_first_action=4896ms；t_level_0=4896ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2114ms；t_detect_stable=70ms；t_first_action=2114ms；t_level_1=2114ms |
| raw recovery timing | t_detect_warning=733ms；t_detect_stable=4896ms；t_first_action=4896ms；t_level_0=4896ms |

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
| impairment timing | t_detect_warning=1156ms；t_detect_recovering=10386ms；t_detect_stable=110ms；t_detect_congested=2197ms；t_first_action=1156ms；t_level_0=19592ms；t_level_1=1156ms；t_level_2=2197ms；t_level_3=3234ms；t_level_4=4268ms；t_audio_only=4268ms |
| recovery timing | t_detect_stable=573ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1156ms；t_detect_recovering=10386ms；t_detect_stable=110ms；t_detect_congested=2197ms；t_first_action=1156ms；t_level_0=19592ms；t_level_1=1156ms；t_level_2=2197ms；t_level_3=3234ms；t_level_4=4268ms；t_audio_only=4268ms |
| raw recovery timing | t_detect_stable=573ms |

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
| impairment timing | t_detect_stable=69ms |
| recovery timing | t_detect_stable=294ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=69ms |
| raw recovery timing | t_detect_stable=294ms |

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
| impairment timing | t_detect_warning=1324ms；t_detect_recovering=10475ms；t_detect_stable=316ms；t_detect_congested=2360ms；t_first_action=1324ms；t_level_1=1324ms；t_level_2=2360ms；t_level_3=3400ms；t_level_4=4435ms；t_audio_only=4435ms |
| recovery timing | t_detect_recovering=3649ms；t_detect_stable=4649ms；t_detect_congested=609ms；t_first_action=3649ms；t_level_0=12857ms；t_level_1=9816ms；t_level_2=6699ms；t_level_3=3649ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1324ms；t_detect_recovering=10475ms；t_detect_stable=316ms；t_detect_congested=2360ms；t_first_action=1324ms；t_level_1=1324ms；t_level_2=2360ms；t_level_3=3400ms；t_level_4=4435ms；t_audio_only=4435ms |
| raw recovery timing | t_detect_recovering=3649ms；t_detect_stable=4649ms；t_detect_congested=609ms；t_first_action=3649ms；t_level_0=12857ms；t_level_1=9816ms；t_level_2=6699ms；t_level_3=3649ms |

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
| impairment timing | t_detect_warning=1215ms；t_detect_stable=206ms；t_detect_congested=2250ms；t_first_action=1215ms；t_level_1=1215ms；t_level_2=2250ms；t_level_3=3288ms；t_level_4=4324ms；t_audio_only=4324ms |
| recovery timing | t_detect_recovering=4512ms；t_detect_stable=5554ms；t_detect_congested=510ms；t_first_action=4512ms；t_level_0=13837ms；t_level_1=10715ms；t_level_2=7595ms；t_level_3=4512ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1215ms；t_detect_stable=206ms；t_detect_congested=2250ms；t_first_action=1215ms；t_level_1=1215ms；t_level_2=2250ms；t_level_3=3288ms；t_level_4=4324ms；t_audio_only=4324ms |
| raw recovery timing | t_detect_recovering=4512ms；t_detect_stable=5554ms；t_detect_congested=510ms；t_first_action=4512ms；t_level_0=13837ms；t_level_1=10715ms；t_level_2=7595ms；t_level_3=4512ms |

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
| impairment timing | t_detect_warning=4235ms；t_detect_stable=189ms；t_first_action=4235ms；t_level_1=4235ms |
| recovery timing | t_detect_warning=854ms；t_detect_stable=2939ms；t_first_action=2939ms；t_level_0=2939ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4235ms；t_detect_stable=189ms；t_first_action=4235ms；t_level_1=4235ms |
| raw recovery timing | t_detect_warning=854ms；t_detect_stable=2939ms；t_first_action=2939ms；t_level_0=2939ms |

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
| impairment timing | t_detect_warning=1136ms；t_detect_recovering=15360ms；t_detect_stable=120ms；t_detect_congested=2170ms；t_first_action=1136ms；t_level_1=1136ms；t_level_2=2170ms；t_level_3=3203ms；t_level_4=4241ms；t_audio_only=4241ms |
| recovery timing | t_detect_recovering=7501ms；t_detect_stable=8502ms；t_detect_congested=461ms；t_first_action=7501ms；t_level_0=16788ms；t_level_1=13667ms；t_level_2=10553ms；t_level_3=7501ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1136ms；t_detect_recovering=15360ms；t_detect_stable=120ms；t_detect_congested=2170ms；t_first_action=1136ms；t_level_1=1136ms；t_level_2=2170ms；t_level_3=3203ms；t_level_4=4241ms；t_audio_only=4241ms |
| raw recovery timing | t_detect_recovering=7501ms；t_detect_stable=8502ms；t_detect_congested=461ms；t_first_action=7501ms；t_level_0=16788ms；t_level_1=13667ms；t_level_2=10553ms；t_level_3=7501ms |

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
| impairment timing | t_detect_warning=4167ms；t_detect_stable=124ms；t_first_action=4167ms；t_level_1=4167ms |
| recovery timing | t_detect_warning=784ms；t_detect_stable=2877ms；t_first_action=2877ms；t_level_0=2877ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4167ms；t_detect_stable=124ms；t_first_action=4167ms；t_level_1=4167ms |
| raw recovery timing | t_detect_warning=784ms；t_detect_stable=2877ms；t_first_action=2877ms；t_level_0=2877ms |

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
| impairment timing | t_detect_warning=3376ms；t_detect_stable=329ms；t_first_action=3376ms；t_level_1=3376ms |
| recovery timing | t_detect_warning=1032ms；t_detect_stable=3123ms；t_first_action=3123ms；t_level_0=3123ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3376ms；t_detect_stable=329ms；t_first_action=3376ms；t_level_1=3376ms |
| raw recovery timing | t_detect_warning=1032ms；t_detect_stable=3123ms；t_first_action=3123ms；t_level_0=3123ms |

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
| impairment timing | t_detect_warning=1127ms；t_detect_stable=121ms；t_detect_congested=2166ms；t_first_action=1127ms；t_level_1=1127ms；t_level_2=2166ms；t_level_3=3205ms；t_level_4=4241ms；t_audio_only=4241ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1127ms；t_detect_stable=121ms；t_detect_congested=2166ms；t_first_action=1127ms；t_level_1=1127ms；t_level_2=2166ms；t_level_3=3205ms；t_level_4=4241ms；t_audio_only=4241ms |
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
| impairment timing | t_detect_warning=1155ms；t_detect_stable=109ms；t_detect_congested=2194ms；t_first_action=1155ms；t_level_1=1155ms；t_level_2=2194ms；t_level_3=3231ms；t_level_4=4226ms；t_audio_only=4226ms |
| recovery timing | t_detect_recovering=7529ms；t_detect_stable=8529ms；t_detect_congested=489ms；t_first_action=7529ms；t_level_0=16695ms；t_level_1=13654ms；t_level_2=10574ms；t_level_3=7529ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1155ms；t_detect_stable=109ms；t_detect_congested=2194ms；t_first_action=1155ms；t_level_1=1155ms；t_level_2=2194ms；t_level_3=3231ms；t_level_4=4226ms；t_audio_only=4226ms |
| raw recovery timing | t_detect_recovering=7529ms；t_detect_stable=8529ms；t_detect_congested=489ms；t_first_action=7529ms；t_level_0=16695ms；t_level_1=13654ms；t_level_2=10574ms；t_level_3=7529ms |

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
| impairment timing | t_detect_warning=1096ms；t_detect_stable=82ms；t_detect_congested=2127ms；t_first_action=1096ms；t_level_1=1096ms；t_level_2=2127ms；t_level_3=3166ms；t_level_4=4201ms；t_audio_only=4201ms |
| recovery timing | t_detect_recovering=7387ms；t_detect_stable=8387ms；t_detect_congested=387ms；t_first_action=7387ms；t_level_0=16602ms；t_level_1=13477ms；t_level_2=10435ms；t_level_3=7387ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1096ms；t_detect_stable=82ms；t_detect_congested=2127ms；t_first_action=1096ms；t_level_1=1096ms；t_level_2=2127ms；t_level_3=3166ms；t_level_4=4201ms；t_audio_only=4201ms |
| raw recovery timing | t_detect_recovering=7387ms；t_detect_stable=8387ms；t_detect_congested=387ms；t_first_action=7387ms；t_level_0=16602ms；t_level_1=13477ms；t_level_2=10435ms；t_level_3=7387ms |

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
| impairment timing | t_detect_warning=1163ms；t_detect_stable=146ms；t_detect_congested=2199ms；t_first_action=1163ms；t_level_1=1163ms；t_level_2=2199ms；t_level_3=3243ms；t_level_4=4264ms；t_audio_only=4264ms |
| recovery timing | t_detect_recovering=6607ms；t_detect_stable=7607ms；t_detect_congested=607ms；t_first_action=6607ms；t_level_0=15813ms；t_level_1=12693ms；t_level_2=9653ms；t_level_3=6607ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1163ms；t_detect_stable=146ms；t_detect_congested=2199ms；t_first_action=1163ms；t_level_1=1163ms；t_level_2=2199ms；t_level_3=3243ms；t_level_4=4264ms；t_audio_only=4264ms |
| raw recovery timing | t_detect_recovering=6607ms；t_detect_stable=7607ms；t_detect_congested=607ms；t_first_action=6607ms；t_level_0=15813ms；t_level_1=12693ms；t_level_2=9653ms；t_level_3=6607ms |

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
| impairment timing | t_detect_warning=2043ms；t_detect_stable=37ms；t_detect_congested=4121ms；t_first_action=2043ms；t_level_1=2043ms；t_level_2=4121ms |
| recovery timing | t_detect_recovering=7182ms；t_detect_stable=8182ms；t_detect_congested=149ms；t_first_action=149ms；t_level_0=16347ms；t_level_1=13230ms；t_level_2=10188ms；t_level_3=149ms；t_level_4=1182ms；t_audio_only=1182ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2043ms；t_detect_stable=37ms；t_detect_congested=4121ms；t_first_action=2043ms；t_level_1=2043ms；t_level_2=4121ms |
| raw recovery timing | t_detect_recovering=7182ms；t_detect_stable=8182ms；t_detect_congested=149ms；t_first_action=149ms；t_level_0=16347ms；t_level_1=13230ms；t_level_2=10188ms；t_level_3=149ms；t_level_4=1182ms；t_audio_only=1182ms |

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
| impairment timing | t_detect_warning=1321ms；t_detect_stable=315ms；t_detect_congested=2358ms；t_first_action=1321ms；t_level_1=1321ms；t_level_2=2358ms；t_level_3=3360ms；t_level_4=4394ms；t_audio_only=4394ms |
| recovery timing | t_detect_recovering=4410ms；t_detect_stable=5410ms；t_detect_congested=410ms；t_first_action=4410ms；t_level_0=13616ms；t_level_1=10496ms；t_level_2=7455ms；t_level_3=4410ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1321ms；t_detect_stable=315ms；t_detect_congested=2358ms；t_first_action=1321ms；t_level_1=1321ms；t_level_2=2358ms；t_level_3=3360ms；t_level_4=4394ms；t_audio_only=4394ms |
| raw recovery timing | t_detect_recovering=4410ms；t_detect_stable=5410ms；t_detect_congested=410ms；t_first_action=4410ms；t_level_0=13616ms；t_level_1=10496ms；t_level_2=7455ms；t_level_3=4410ms |

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
| impairment timing | t_detect_warning=3059ms；t_detect_stable=53ms；t_first_action=3059ms；t_level_1=3059ms |
| recovery timing | t_detect_warning=119ms；t_detect_stable=3245ms；t_first_action=3245ms；t_level_0=3245ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3059ms；t_detect_stable=53ms；t_first_action=3059ms；t_level_1=3059ms |
| raw recovery timing | t_detect_warning=119ms；t_detect_stable=3245ms；t_first_action=3245ms；t_level_0=3245ms |

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
| impairment timing | t_detect_warning=2261ms；t_detect_stable=215ms；t_first_action=2261ms；t_level_1=2261ms |
| recovery timing | t_detect_warning=359ms；t_detect_stable=4446ms；t_first_action=4446ms；t_level_0=4446ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2261ms；t_detect_stable=215ms；t_first_action=2261ms；t_level_1=2261ms |
| raw recovery timing | t_detect_warning=359ms；t_detect_stable=4446ms；t_first_action=4446ms；t_level_0=4446ms |

