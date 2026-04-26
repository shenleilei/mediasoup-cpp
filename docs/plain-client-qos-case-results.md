# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-26T16:33:53.562Z`

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
| impairment timing | t_detect_stable=305ms |
| recovery timing | t_detect_stable=769ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=305ms |
| raw recovery timing | t_detect_stable=769ms |

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
| impairment timing | t_detect_stable=333ms |
| recovery timing | t_detect_stable=586ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=333ms |
| raw recovery timing | t_detect_stable=586ms |

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
| impairment timing | t_detect_warning=217ms |
| recovery timing | t_detect_warning=922ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=217ms |
| raw recovery timing | t_detect_warning=922ms |

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
| impairment timing | t_detect_stable=402ms |
| recovery timing | t_detect_stable=709ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=402ms |
| raw recovery timing | t_detect_stable=709ms |

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
| impairment timing | t_detect_warning=1158ms；t_detect_recovering=10345ms；t_detect_stable=111ms；t_detect_congested=2192ms；t_first_action=1158ms；t_level_1=1158ms；t_level_2=2192ms；t_level_3=3230ms；t_level_4=4266ms；t_audio_only=4266ms |
| recovery timing | t_detect_recovering=3415ms；t_detect_stable=4415ms；t_detect_congested=415ms；t_first_action=3415ms；t_level_0=12625ms；t_level_1=9501ms；t_level_2=6468ms；t_level_3=3415ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1158ms；t_detect_recovering=10345ms；t_detect_stable=111ms；t_detect_congested=2192ms；t_first_action=1158ms；t_level_1=1158ms；t_level_2=2192ms；t_level_3=3230ms；t_level_4=4266ms；t_audio_only=4266ms |
| raw recovery timing | t_detect_recovering=3415ms；t_detect_stable=4415ms；t_detect_congested=415ms；t_first_action=3415ms；t_level_0=12625ms；t_level_1=9501ms；t_level_2=6468ms；t_level_3=3415ms |

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
| impairment timing | t_detect_warning=1320ms；t_detect_recovering=10590ms；t_detect_stable=312ms；t_detect_congested=2368ms；t_first_action=1320ms；t_level_1=1320ms；t_level_2=2368ms；t_level_3=3397ms；t_level_4=4432ms；t_audio_only=4432ms |
| recovery timing | t_detect_recovering=3758ms；t_detect_stable=4774ms；t_detect_congested=709ms；t_first_action=3758ms；t_level_0=13074ms；t_level_1=9944ms；t_level_2=6834ms；t_level_3=3758ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1320ms；t_detect_recovering=10590ms；t_detect_stable=312ms；t_detect_congested=2368ms；t_first_action=1320ms；t_level_1=1320ms；t_level_2=2368ms；t_level_3=3397ms；t_level_4=4432ms；t_audio_only=4432ms |
| raw recovery timing | t_detect_recovering=3758ms；t_detect_stable=4774ms；t_detect_congested=709ms；t_first_action=3758ms；t_level_0=13074ms；t_level_1=9944ms；t_level_2=6834ms；t_level_3=3758ms |

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
| impairment timing | t_detect_warning=1153ms；t_detect_stable=146ms；t_detect_congested=2160ms；t_first_action=1153ms；t_level_1=1153ms；t_level_2=2160ms；t_level_3=3192ms；t_level_4=4225ms；t_audio_only=4225ms |
| recovery timing | t_detect_recovering=4397ms；t_detect_stable=5404ms；t_detect_congested=397ms；t_first_action=4397ms；t_level_0=13613ms；t_level_1=10536ms；t_level_2=7462ms；t_level_3=4397ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1153ms；t_detect_stable=146ms；t_detect_congested=2160ms；t_first_action=1153ms；t_level_1=1153ms；t_level_2=2160ms；t_level_3=3192ms；t_level_4=4225ms；t_audio_only=4225ms |
| raw recovery timing | t_detect_recovering=4397ms；t_detect_stable=5404ms；t_detect_congested=397ms；t_first_action=4397ms；t_level_0=13613ms；t_level_1=10536ms；t_level_2=7462ms；t_level_3=4397ms |

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
| impairment timing | t_detect_warning=1143ms；t_detect_stable=100ms；t_detect_congested=2159ms；t_first_action=1143ms；t_level_1=1143ms；t_level_2=2159ms；t_level_3=3186ms；t_level_4=4215ms；t_audio_only=4215ms |
| recovery timing | t_detect_recovering=4479ms；t_detect_stable=5520ms；t_detect_congested=438ms；t_first_action=4479ms；t_level_0=13753ms；t_level_1=10609ms；t_level_2=7563ms；t_level_3=4479ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1143ms；t_detect_stable=100ms；t_detect_congested=2159ms；t_first_action=1143ms；t_level_1=1143ms；t_level_2=2159ms；t_level_3=3186ms；t_level_4=4215ms；t_audio_only=4215ms |
| raw recovery timing | t_detect_recovering=4479ms；t_detect_stable=5520ms；t_detect_congested=438ms；t_first_action=4479ms；t_level_0=13753ms；t_level_1=10609ms；t_level_2=7563ms；t_level_3=4479ms |

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
| impairment timing | t_detect_warning=1251ms；t_detect_stable=212ms；t_detect_congested=2285ms；t_first_action=1251ms；t_level_1=1251ms；t_level_2=2285ms；t_level_3=3350ms；t_level_4=4359ms；t_audio_only=4359ms |
| recovery timing | t_detect_recovering=3553ms；t_detect_stable=4553ms；t_detect_congested=513ms；t_first_action=3553ms；t_level_0=12843ms；t_level_1=9720ms；t_level_2=6604ms；t_level_3=3553ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1251ms；t_detect_stable=212ms；t_detect_congested=2285ms；t_first_action=1251ms；t_level_1=1251ms；t_level_2=2285ms；t_level_3=3350ms；t_level_4=4359ms；t_audio_only=4359ms |
| raw recovery timing | t_detect_recovering=3553ms；t_detect_stable=4553ms；t_detect_congested=513ms；t_first_action=3553ms；t_level_0=12843ms；t_level_1=9720ms；t_level_2=6604ms；t_level_3=3553ms |

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
| impairment timing | t_detect_stable=333ms |
| recovery timing | t_detect_stable=624ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=333ms |
| raw recovery timing | t_detect_stable=624ms |

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
| impairment timing | t_detect_stable=113ms |
| recovery timing | t_detect_stable=474ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=113ms |
| raw recovery timing | t_detect_stable=474ms |

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
| impairment timing | t_detect_stable=318ms |
| recovery timing | t_detect_stable=653ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=318ms |
| raw recovery timing | t_detect_stable=653ms |

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
| impairment timing | t_detect_warning=4249ms；t_detect_stable=199ms；t_first_action=4249ms；t_level_1=4249ms |
| recovery timing | t_detect_warning=868ms；t_detect_stable=2954ms；t_first_action=2954ms；t_level_0=2954ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4249ms；t_detect_stable=199ms；t_first_action=4249ms；t_level_1=4249ms |
| raw recovery timing | t_detect_warning=868ms；t_detect_stable=2954ms；t_first_action=2954ms；t_level_0=2954ms |

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
| impairment timing | t_detect_warning=2246ms；t_detect_recovering=14438ms；t_detect_stable=239ms；t_detect_congested=4333ms；t_first_action=2246ms；t_level_1=2246ms；t_level_2=4333ms；t_level_3=5364ms；t_level_4=6399ms；t_audio_only=6399ms |
| recovery timing | t_detect_warning=491ms；t_detect_stable=4578ms；t_first_action=4578ms；t_level_0=10820ms；t_level_1=7700ms；t_level_2=4578ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2246ms；t_detect_recovering=14438ms；t_detect_stable=239ms；t_detect_congested=4333ms；t_first_action=2246ms；t_level_1=2246ms；t_level_2=4333ms；t_level_3=5364ms；t_level_4=6399ms；t_audio_only=6399ms |
| raw recovery timing | t_detect_warning=491ms；t_detect_stable=4578ms；t_first_action=4578ms；t_level_0=10820ms；t_level_1=7700ms；t_level_2=4578ms |

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
| impairment timing | t_detect_warning=1327ms；t_detect_recovering=15477ms；t_detect_stable=320ms；t_detect_congested=2363ms；t_first_action=1327ms；t_level_1=1327ms；t_level_2=2363ms；t_level_3=3404ms；t_level_4=4437ms；t_audio_only=4437ms |
| recovery timing | t_detect_recovering=7697ms；t_detect_stable=8697ms；t_detect_congested=537ms；t_first_action=7697ms；t_level_0=16909ms；t_level_1=13792ms；t_level_2=10744ms；t_level_3=7697ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1327ms；t_detect_recovering=15477ms；t_detect_stable=320ms；t_detect_congested=2363ms；t_first_action=1327ms；t_level_1=1327ms；t_level_2=2363ms；t_level_3=3404ms；t_level_4=4437ms；t_audio_only=4437ms |
| raw recovery timing | t_detect_recovering=7697ms；t_detect_stable=8697ms；t_detect_congested=537ms；t_first_action=7697ms；t_level_0=16909ms；t_level_1=13792ms；t_level_2=10744ms；t_level_3=7697ms |

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
| impairment timing | t_detect_warning=1231ms；t_detect_stable=213ms；t_detect_congested=2273ms；t_first_action=1231ms；t_level_1=1231ms；t_level_2=2273ms；t_level_3=3300ms；t_level_4=4333ms；t_audio_only=4333ms |
| recovery timing | t_detect_recovering=4398ms；t_detect_stable=5398ms；t_detect_congested=398ms；t_first_action=4398ms；t_level_0=13620ms；t_level_1=10487ms；t_level_2=7454ms；t_level_3=4398ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1231ms；t_detect_stable=213ms；t_detect_congested=2273ms；t_first_action=1231ms；t_level_1=1231ms；t_level_2=2273ms；t_level_3=3300ms；t_level_4=4333ms；t_audio_only=4333ms |
| raw recovery timing | t_detect_recovering=4398ms；t_detect_stable=5398ms；t_detect_congested=398ms；t_first_action=4398ms；t_level_0=13620ms；t_level_1=10487ms；t_level_2=7454ms；t_level_3=4398ms |

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
| impairment timing | t_detect_warning=1288ms；t_detect_stable=283ms；t_detect_congested=2328ms；t_first_action=1288ms；t_level_1=1288ms；t_level_2=2328ms；t_level_3=3371ms；t_level_4=4403ms；t_audio_only=4403ms |
| recovery timing | t_detect_recovering=4534ms；t_detect_stable=5534ms；t_detect_congested=494ms；t_first_action=4534ms；t_level_0=13702ms；t_level_1=10665ms；t_level_2=7545ms；t_level_3=4534ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1288ms；t_detect_stable=283ms；t_detect_congested=2328ms；t_first_action=1288ms；t_level_1=1288ms；t_level_2=2328ms；t_level_3=3371ms；t_level_4=4403ms；t_audio_only=4403ms |
| raw recovery timing | t_detect_recovering=4534ms；t_detect_stable=5534ms；t_detect_congested=494ms；t_first_action=4534ms；t_level_0=13702ms；t_level_1=10665ms；t_level_2=7545ms；t_level_3=4534ms |

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
| impairment timing | t_detect_stable=180ms |
| recovery timing | t_detect_stable=559ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=180ms |
| raw recovery timing | t_detect_stable=559ms |

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
| impairment timing | t_detect_stable=124ms |
| recovery timing | t_detect_stable=479ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=124ms |
| raw recovery timing | t_detect_stable=479ms |

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
| impairment timing | t_detect_stable=56ms |
| recovery timing | t_detect_stable=433ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=56ms |
| raw recovery timing | t_detect_stable=433ms |

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
| impairment timing | t_detect_warning=4245ms；t_detect_stable=201ms；t_first_action=4245ms；t_level_1=4245ms |
| recovery timing | t_detect_warning=780ms；t_detect_stable=2863ms；t_first_action=2863ms；t_level_0=2863ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4245ms；t_detect_stable=201ms；t_first_action=4245ms；t_level_1=4245ms |
| raw recovery timing | t_detect_warning=780ms；t_detect_stable=2863ms；t_first_action=2863ms；t_level_0=2863ms |

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
| impairment timing | t_detect_warning=2311ms；t_detect_stable=264ms；t_detect_congested=6477ms；t_first_action=2311ms；t_level_1=2311ms；t_level_2=6477ms；t_level_3=7506ms；t_level_4=8542ms；t_audio_only=8542ms |
| recovery timing | t_detect_recovering=5666ms；t_detect_stable=6666ms；t_detect_congested=666ms；t_first_action=5666ms；t_level_0=14952ms；t_level_1=11832ms；t_level_2=8713ms；t_level_3=5666ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2311ms；t_detect_stable=264ms；t_detect_congested=6477ms；t_first_action=2311ms；t_level_1=2311ms；t_level_2=6477ms；t_level_3=7506ms；t_level_4=8542ms；t_audio_only=8542ms |
| raw recovery timing | t_detect_recovering=5666ms；t_detect_stable=6666ms；t_detect_congested=666ms；t_first_action=5666ms；t_level_0=14952ms；t_level_1=11832ms；t_level_2=8713ms；t_level_3=5666ms |

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
| impairment timing | t_detect_warning=2230ms；t_detect_stable=185ms；t_detect_congested=3271ms；t_first_action=2230ms；t_level_1=2230ms；t_level_2=3271ms；t_level_3=4309ms；t_level_4=5344ms；t_audio_only=5344ms |
| recovery timing | t_detect_recovering=6568ms；t_detect_stable=7568ms；t_detect_congested=528ms；t_first_action=6568ms；t_level_0=15776ms；t_level_1=12656ms；t_level_2=9613ms；t_level_3=6568ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2230ms；t_detect_stable=185ms；t_detect_congested=3271ms；t_first_action=2230ms；t_level_1=2230ms；t_level_2=3271ms；t_level_3=4309ms；t_level_4=5344ms；t_audio_only=5344ms |
| raw recovery timing | t_detect_recovering=6568ms；t_detect_stable=7568ms；t_detect_congested=528ms；t_first_action=6568ms；t_level_0=15776ms；t_level_1=12656ms；t_level_2=9613ms；t_level_3=6568ms |

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
| impairment timing | t_detect_stable=183ms |
| recovery timing | t_detect_stable=607ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=183ms |
| raw recovery timing | t_detect_stable=607ms |

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
| impairment timing | t_detect_stable=172ms |
| recovery timing | t_detect_stable=468ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=172ms |
| raw recovery timing | t_detect_stable=468ms |

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
| impairment timing | t_detect_warning=3315ms；t_detect_stable=252ms；t_first_action=3315ms；t_level_1=3315ms |
| recovery timing | t_detect_warning=823ms；t_detect_stable=2915ms；t_first_action=2915ms；t_level_0=2915ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3315ms；t_detect_stable=252ms；t_first_action=3315ms；t_level_1=3315ms |
| raw recovery timing | t_detect_warning=823ms；t_detect_stable=2915ms；t_first_action=2915ms；t_level_0=2915ms |

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
| impairment timing | t_detect_warning=2209ms；t_detect_stable=156ms；t_first_action=2209ms；t_level_1=2209ms |
| recovery timing | t_detect_warning=811ms；t_detect_stable=4975ms；t_first_action=4975ms；t_level_0=4975ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2209ms；t_detect_stable=156ms；t_first_action=2209ms；t_level_1=2209ms |
| raw recovery timing | t_detect_warning=811ms；t_detect_stable=4975ms；t_first_action=4975ms；t_level_0=4975ms |

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
| impairment timing | t_detect_warning=1186ms；t_detect_recovering=10443ms；t_detect_stable=130ms；t_detect_congested=2213ms；t_first_action=1186ms；t_level_0=19660ms；t_level_1=1186ms；t_level_2=2213ms；t_level_3=3253ms；t_level_4=4284ms；t_audio_only=4284ms |
| recovery timing | t_detect_stable=618ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1186ms；t_detect_recovering=10443ms；t_detect_stable=130ms；t_detect_congested=2213ms；t_first_action=1186ms；t_level_0=19660ms；t_level_1=1186ms；t_level_2=2213ms；t_level_3=3253ms；t_level_4=4284ms；t_audio_only=4284ms |
| raw recovery timing | t_detect_stable=618ms |

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
| impairment timing | t_detect_stable=337ms |
| recovery timing | t_detect_stable=640ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=337ms |
| raw recovery timing | t_detect_stable=640ms |

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
| impairment timing | t_detect_warning=1080ms；t_detect_recovering=10264ms；t_detect_stable=68ms；t_detect_congested=2112ms；t_first_action=1080ms；t_level_1=1080ms；t_level_2=2112ms；t_level_3=3150ms；t_level_4=4186ms；t_audio_only=4186ms |
| recovery timing | t_detect_recovering=3509ms；t_detect_stable=4509ms；t_detect_congested=431ms；t_first_action=3509ms；t_level_0=12717ms；t_level_1=9600ms；t_level_2=6573ms；t_level_3=3509ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1080ms；t_detect_recovering=10264ms；t_detect_stable=68ms；t_detect_congested=2112ms；t_first_action=1080ms；t_level_1=1080ms；t_level_2=2112ms；t_level_3=3150ms；t_level_4=4186ms；t_audio_only=4186ms |
| raw recovery timing | t_detect_recovering=3509ms；t_detect_stable=4509ms；t_detect_congested=431ms；t_first_action=3509ms；t_level_0=12717ms；t_level_1=9600ms；t_level_2=6573ms；t_level_3=3509ms |

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
| impairment timing | t_detect_warning=1212ms；t_detect_stable=205ms；t_detect_congested=2251ms；t_first_action=1212ms；t_level_1=1212ms；t_level_2=2251ms；t_level_3=3290ms；t_level_4=4325ms；t_audio_only=4325ms |
| recovery timing | t_detect_recovering=4502ms；t_detect_stable=5502ms；t_detect_congested=462ms；t_first_action=4502ms；t_level_0=13724ms；t_level_1=10593ms；t_level_2=7547ms；t_level_3=4502ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1212ms；t_detect_stable=205ms；t_detect_congested=2251ms；t_first_action=1212ms；t_level_1=1212ms；t_level_2=2251ms；t_level_3=3290ms；t_level_4=4325ms；t_audio_only=4325ms |
| raw recovery timing | t_detect_recovering=4502ms；t_detect_stable=5502ms；t_detect_congested=462ms；t_first_action=4502ms；t_level_0=13724ms；t_level_1=10593ms；t_level_2=7547ms；t_level_3=4502ms |

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
| impairment timing | t_detect_warning=4245ms；t_detect_stable=201ms；t_first_action=4245ms；t_level_1=4245ms |
| recovery timing | t_detect_warning=766ms；t_detect_stable=2883ms；t_first_action=2883ms；t_level_0=2883ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4245ms；t_detect_stable=201ms；t_first_action=4245ms；t_level_1=4245ms |
| raw recovery timing | t_detect_warning=766ms；t_detect_stable=2883ms；t_first_action=2883ms；t_level_0=2883ms |

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
| impairment timing | t_detect_warning=1058ms；t_detect_recovering=15207ms；t_detect_stable=11ms；t_detect_congested=2100ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2100ms；t_level_3=3133ms；t_level_4=4168ms；t_audio_only=4168ms |
| recovery timing | t_detect_recovering=7270ms；t_detect_stable=8270ms；t_detect_congested=270ms；t_first_action=7270ms；t_level_0=16483ms；t_level_1=13367ms；t_level_2=10315ms；t_level_3=7270ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1058ms；t_detect_recovering=15207ms；t_detect_stable=11ms；t_detect_congested=2100ms；t_first_action=1058ms；t_level_1=1058ms；t_level_2=2100ms；t_level_3=3133ms；t_level_4=4168ms；t_audio_only=4168ms |
| raw recovery timing | t_detect_recovering=7270ms；t_detect_stable=8270ms；t_detect_congested=270ms；t_first_action=7270ms；t_level_0=16483ms；t_level_1=13367ms；t_level_2=10315ms；t_level_3=7270ms |

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
| impairment timing | t_detect_warning=4201ms；t_detect_stable=113ms；t_first_action=4201ms；t_level_1=4201ms |
| recovery timing | t_detect_warning=693ms；t_detect_stable=2699ms；t_first_action=2699ms；t_level_0=2699ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=4201ms；t_detect_stable=113ms；t_first_action=4201ms；t_level_1=4201ms |
| raw recovery timing | t_detect_warning=693ms；t_detect_stable=2699ms；t_first_action=2699ms；t_level_0=2699ms |

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
| impairment timing | t_detect_warning=3162ms；t_detect_stable=155ms；t_first_action=3162ms；t_level_1=3162ms |
| recovery timing | t_detect_warning=657ms；t_detect_stable=3800ms；t_first_action=3800ms；t_level_0=3800ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3162ms；t_detect_stable=155ms；t_first_action=3162ms；t_level_1=3162ms |
| raw recovery timing | t_detect_warning=657ms；t_detect_stable=3800ms；t_first_action=3800ms；t_level_0=3800ms |

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
| impairment timing | t_detect_warning=1188ms；t_detect_stable=182ms；t_detect_congested=2226ms；t_first_action=1188ms；t_level_1=1188ms；t_level_2=2226ms；t_level_3=3270ms；t_level_4=4325ms；t_audio_only=4325ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1188ms；t_detect_stable=182ms；t_detect_congested=2226ms；t_first_action=1188ms；t_level_1=1188ms；t_level_2=2226ms；t_level_3=3270ms；t_level_4=4325ms；t_audio_only=4325ms |
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
| impairment timing | t_detect_warning=1239ms；t_detect_stable=230ms；t_detect_congested=2276ms；t_first_action=1239ms；t_level_1=1239ms；t_level_2=2276ms；t_level_3=3313ms；t_level_4=4349ms；t_audio_only=4349ms |
| recovery timing | t_detect_recovering=7492ms；t_detect_stable=8492ms；t_detect_congested=492ms；t_first_action=7492ms；t_level_0=16728ms；t_level_1=13659ms；t_level_2=10543ms；t_level_3=7492ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1239ms；t_detect_stable=230ms；t_detect_congested=2276ms；t_first_action=1239ms；t_level_1=1239ms；t_level_2=2276ms；t_level_3=3313ms；t_level_4=4349ms；t_audio_only=4349ms |
| raw recovery timing | t_detect_recovering=7492ms；t_detect_stable=8492ms；t_detect_congested=492ms；t_first_action=7492ms；t_level_0=16728ms；t_level_1=13659ms；t_level_2=10543ms；t_level_3=7492ms |

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
| impairment timing | t_detect_warning=1220ms；t_detect_stable=211ms；t_detect_congested=2259ms；t_first_action=1220ms；t_level_1=1220ms；t_level_2=2259ms；t_level_3=3296ms；t_level_4=4331ms；t_audio_only=4331ms |
| recovery timing | t_detect_recovering=7634ms；t_detect_stable=8673ms；t_detect_congested=632ms；t_first_action=7634ms；t_level_0=16927ms；t_level_1=13846ms；t_level_2=10718ms；t_level_3=7634ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1220ms；t_detect_stable=211ms；t_detect_congested=2259ms；t_first_action=1220ms；t_level_1=1220ms；t_level_2=2259ms；t_level_3=3296ms；t_level_4=4331ms；t_audio_only=4331ms |
| raw recovery timing | t_detect_recovering=7634ms；t_detect_stable=8673ms；t_detect_congested=632ms；t_first_action=7634ms；t_level_0=16927ms；t_level_1=13846ms；t_level_2=10718ms；t_level_3=7634ms |

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
| impairment timing | t_detect_warning=1204ms；t_detect_stable=189ms；t_detect_congested=2232ms；t_first_action=1204ms；t_level_1=1204ms；t_level_2=2232ms；t_level_3=3276ms；t_level_4=4308ms；t_audio_only=4308ms |
| recovery timing | t_detect_recovering=5842ms；t_detect_stable=6884ms；t_detect_congested=840ms；t_first_action=5842ms；t_level_0=15047ms；t_level_1=12008ms；t_level_2=8937ms；t_level_3=5842ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1204ms；t_detect_stable=189ms；t_detect_congested=2232ms；t_first_action=1204ms；t_level_1=1204ms；t_level_2=2232ms；t_level_3=3276ms；t_level_4=4308ms；t_audio_only=4308ms |
| raw recovery timing | t_detect_recovering=5842ms；t_detect_stable=6884ms；t_detect_congested=840ms；t_first_action=5842ms；t_level_0=15047ms；t_level_1=12008ms；t_level_2=8937ms；t_level_3=5842ms |

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
| impairment timing | t_detect_warning=2119ms；t_detect_stable=67ms；t_detect_congested=4193ms；t_first_action=2119ms；t_level_1=2119ms；t_level_2=4193ms |
| recovery timing | t_detect_recovering=7322ms；t_detect_stable=8322ms；t_detect_congested=215ms；t_first_action=215ms；t_level_0=16449ms；t_level_1=13407ms；t_level_2=10370ms；t_level_3=215ms；t_level_4=1243ms；t_audio_only=1243ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2119ms；t_detect_stable=67ms；t_detect_congested=4193ms；t_first_action=2119ms；t_level_1=2119ms；t_level_2=4193ms |
| raw recovery timing | t_detect_recovering=7322ms；t_detect_stable=8322ms；t_detect_congested=215ms；t_first_action=215ms；t_level_0=16449ms；t_level_1=13407ms；t_level_2=10370ms；t_level_3=215ms；t_level_4=1243ms；t_audio_only=1243ms |

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
| impairment timing | t_detect_warning=1339ms；t_detect_stable=294ms；t_detect_congested=2382ms；t_first_action=1339ms；t_level_1=1339ms；t_level_2=2382ms；t_level_3=3421ms；t_level_4=4415ms；t_audio_only=4415ms |
| recovery timing | t_detect_recovering=4367ms；t_detect_stable=5369ms；t_detect_congested=367ms；t_first_action=4367ms；t_level_0=13577ms；t_level_1=10454ms；t_level_2=7415ms；t_level_3=4367ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=1339ms；t_detect_stable=294ms；t_detect_congested=2382ms；t_first_action=1339ms；t_level_1=1339ms；t_level_2=2382ms；t_level_3=3421ms；t_level_4=4415ms；t_audio_only=4415ms |
| raw recovery timing | t_detect_recovering=4367ms；t_detect_stable=5369ms；t_detect_congested=367ms；t_first_action=4367ms；t_level_0=13577ms；t_level_1=10454ms；t_level_2=7415ms；t_level_3=4367ms |

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
| impairment timing | t_detect_warning=3248ms；t_detect_stable=241ms；t_first_action=3248ms；t_level_1=3248ms |
| recovery timing | t_detect_warning=260ms；t_detect_stable=3348ms；t_first_action=3348ms；t_level_0=3348ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=3248ms；t_detect_stable=241ms；t_first_action=3248ms；t_level_1=3248ms |
| raw recovery timing | t_detect_warning=260ms；t_detect_stable=3348ms；t_first_action=3348ms；t_level_0=3348ms |

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
| impairment timing | t_detect_warning=2307ms；t_detect_stable=225ms；t_first_action=2307ms；t_level_1=2307ms |
| recovery timing | t_detect_warning=381ms；t_detect_stable=4471ms；t_first_action=4471ms；t_level_0=4471ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2307ms；t_detect_stable=225ms；t_first_action=2307ms；t_level_1=2307ms |
| raw recovery timing | t_detect_warning=381ms；t_detect_stable=4471ms；t_first_action=4471ms；t_level_0=4471ms |

