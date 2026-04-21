# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-21T07:15:44.754Z`

## 1. 汇总

- 总 Case：`43`
- 已执行：`43`
- 通过：`12`
- 失败：`31`
- 错误：`0`
- runner：`cpp_client`

### 1.1 失败 / 错误 Case

| Case ID | 结果 | 说明 |
|---|---|---|
| [B3](#b3) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| [BW3](#bw3) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [BW4](#bw4) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [BW5](#bw5) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [BW6](#bw6) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [BW7](#bw7) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [L4](#l4) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [L5](#l5) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [L6](#l6) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [L7](#l7) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [L8](#l8) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [R4](#r4) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| [R5](#r5) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [R6](#r6) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [J3](#j3) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| [J4](#j4) | `FAIL` | stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱 |
| [J5](#j5) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T2](#t2) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T3](#t3) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T4](#t4) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T5](#t5) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=false, analysis=过弱 |
| [T6](#t6) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T7](#t7) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T8](#t8) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [T9](#t9) | `FAIL` | stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢 |
| [T10](#t10) | `FAIL` | stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢 |
| [T11](#t11) | `FAIL` | stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢 |
| [S1](#s1) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [S2](#s2) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [S3](#s3) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |
| [S4](#s4) | `FAIL` | stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱 |

## 2. 快速跳转

- 失败 / 错误：[B3](#b3)、[BW3](#bw3)、[BW4](#bw4)、[BW5](#bw5)、[BW6](#bw6)、[BW7](#bw7)、[L4](#l4)、[L5](#l5)、[L6](#l6)、[L7](#l7)、[L8](#l8)、[R4](#r4)、[R5](#r5)、[R6](#r6)、[J3](#j3)、[J4](#j4)、[J5](#j5)、[T2](#t2)、[T3](#t3)、[T4](#t4)、[T5](#t5)、[T6](#t6)、[T7](#t7)、[T8](#t8)、[T9](#t9)、[T10](#t10)、[T11](#t11)、[S1](#s1)、[S2](#s2)、[S3](#s3)、[S4](#s4)
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
| impairment timing | t_detect_stable=484ms |
| recovery timing | t_detect_stable=484ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=484ms |
| raw recovery timing | t_detect_stable=484ms |

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
| impairment timing | t_detect_stable=489ms |
| recovery timing | t_detect_stable=466ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=489ms |
| raw recovery timing | t_detect_stable=466ms |

### B3

| 字段 | 内容 |
|---|---|
| Case ID | `B3` |
| 类型 | `baseline` / priority `P0` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| recovery 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=494ms |
| recovery timing | t_detect_warning=485ms；t_first_action=485ms；t_level_1=485ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=494ms |
| raw recovery timing | t_detect_warning=485ms；t_first_action=485ms；t_level_1=485ms |

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
| impairment timing | t_detect_stable=495ms |
| recovery timing | t_detect_stable=509ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=495ms |
| raw recovery timing | t_detect_stable=509ms |

### BW3

| 字段 | 内容 |
|---|---|
| Case ID | `BW3` |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=550ms |
| recovery timing | t_detect_stable=536ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=550ms |
| raw recovery timing | t_detect_stable=536ms |

### BW4

| 字段 | 内容 |
|---|---|
| Case ID | `BW4` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 800kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=420ms |
| recovery timing | t_detect_stable=442ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=420ms |
| raw recovery timing | t_detect_stable=442ms |

### BW5

| 字段 | 内容 |
|---|---|
| Case ID | `BW5` |
| 类型 | `bw_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=487ms |
| recovery timing | t_detect_stable=469ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=487ms |
| raw recovery timing | t_detect_stable=469ms |

### BW6

| 字段 | 内容 |
|---|---|
| Case ID | `BW6` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=473ms |
| recovery timing | t_detect_stable=457ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=473ms |
| raw recovery timing | t_detect_stable=457ms |

### BW7

| 字段 | 内容 |
|---|---|
| Case ID | `BW7` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 200kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=562ms |
| recovery timing | t_detect_stable=536ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=562ms |
| raw recovery timing | t_detect_stable=536ms |

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
| impairment timing | t_detect_stable=546ms |
| recovery timing | t_detect_stable=534ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=546ms |
| raw recovery timing | t_detect_stable=534ms |

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
| impairment timing | t_detect_stable=505ms |
| recovery timing | t_detect_stable=493ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=505ms |
| raw recovery timing | t_detect_stable=493ms |

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
| impairment timing | t_detect_stable=468ms |
| recovery timing | t_detect_stable=491ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=468ms |
| raw recovery timing | t_detect_stable=491ms |

### L4

| 字段 | 内容 |
|---|---|
| Case ID | `L4` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=627ms |
| recovery timing | t_detect_stable=613ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=627ms |
| raw recovery timing | t_detect_stable=613ms |

### L5

| 字段 | 内容 |
|---|---|
| Case ID | `L5` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=497ms |
| recovery timing | t_detect_warning=491ms；t_first_action=491ms；t_level_1=491ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=497ms |
| raw recovery timing | t_detect_warning=491ms；t_first_action=491ms；t_level_1=491ms |

### L6

| 字段 | 内容 |
|---|---|
| Case ID | `L6` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=congested/L3) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 3 次非 noop） |
| impairment timing | t_detect_stable=495ms |
| recovery timing | t_detect_warning=557ms；t_detect_congested=1566ms；t_first_action=557ms；t_level_1=557ms；t_level_2=1566ms；t_level_3=2595ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=495ms |
| raw recovery timing | t_detect_warning=557ms；t_detect_congested=1566ms；t_first_action=557ms；t_level_1=557ms；t_level_2=1566ms；t_level_3=2595ms |

### L7

| 字段 | 内容 |
|---|---|
| Case ID | `L7` |
| 类型 | `loss_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 40% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=congested/L3) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 3 次非 noop） |
| impairment timing | t_detect_stable=452ms |
| recovery timing | t_detect_warning=485ms；t_detect_congested=1522ms；t_first_action=485ms；t_level_1=485ms；t_level_2=1522ms；t_level_3=2559ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=452ms |
| raw recovery timing | t_detect_warning=485ms；t_detect_congested=1522ms；t_first_action=485ms；t_level_1=485ms；t_level_2=1522ms；t_level_3=2559ms |

### L8

| 字段 | 内容 |
|---|---|
| Case ID | `L8` |
| 类型 | `loss_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 60% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=congested/L3) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 3 次非 noop） |
| impairment timing | t_detect_stable=457ms |
| recovery timing | t_detect_warning=442ms；t_detect_congested=1479ms；t_first_action=442ms；t_level_1=442ms；t_level_2=1479ms；t_level_3=2518ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=457ms |
| raw recovery timing | t_detect_warning=442ms；t_detect_congested=1479ms；t_first_action=442ms；t_level_1=442ms；t_level_2=1479ms；t_level_3=2518ms |

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
| impairment timing | t_detect_stable=484ms |
| recovery timing | t_detect_stable=469ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=484ms |
| raw recovery timing | t_detect_stable=469ms |

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
| impairment timing | t_detect_stable=538ms |
| recovery timing | t_detect_stable=586ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=538ms |
| raw recovery timing | t_detect_stable=586ms |

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
| impairment timing | t_detect_stable=549ms |
| recovery timing | t_detect_stable=526ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=549ms |
| raw recovery timing | t_detect_stable=526ms |

### R4

| 字段 | 内容 |
|---|---|
| Case ID | `R4` |
| 类型 | `rtt_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=526ms |
| recovery timing | t_detect_stable=550ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=526ms |
| raw recovery timing | t_detect_stable=550ms |

### R5

| 字段 | 内容 |
|---|---|
| Case ID | `R5` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 250ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=509ms |
| recovery timing | t_detect_warning=542ms；t_first_action=542ms；t_level_1=542ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=509ms |
| raw recovery timing | t_detect_warning=542ms；t_first_action=542ms；t_level_1=542ms |

### R6

| 字段 | 内容 |
|---|---|
| Case ID | `R6` |
| 类型 | `rtt_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 350ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=489ms |
| recovery timing | t_detect_warning=520ms；t_first_action=520ms；t_level_1=520ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=489ms |
| raw recovery timing | t_detect_warning=520ms；t_first_action=520ms；t_level_1=520ms |

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
| impairment timing | t_detect_stable=509ms |
| recovery timing | t_detect_stable=531ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=509ms |
| raw recovery timing | t_detect_stable=531ms |

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
| impairment timing | t_detect_stable=438ms |
| recovery timing | t_detect_stable=501ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=438ms |
| raw recovery timing | t_detect_stable=501ms |

### J3

| 字段 | 内容 |
|---|---|
| Case ID | `J3` |
| 类型 | `jitter_sweep` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；minLevel=1；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=490ms |
| recovery timing | t_detect_stable=474ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=490ms |
| raw recovery timing | t_detect_stable=474ms |

### J4

| 字段 | 内容 |
|---|---|
| Case ID | `J4` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；minLevel=1；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=false, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=511ms |
| recovery timing | t_detect_warning=457ms；t_first_action=457ms；t_level_1=457ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=511ms |
| raw recovery timing | t_detect_warning=457ms；t_first_action=457ms；t_level_1=457ms |

### J5

| 字段 | 内容 |
|---|---|
| Case ID | `J5` |
| 类型 | `jitter_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 100ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=early_warning/L1) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 1 次非 noop） |
| impairment timing | t_detect_stable=412ms |
| recovery timing | t_detect_warning=482ms；t_first_action=482ms；t_level_1=482ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=412ms |
| raw recovery timing | t_detect_warning=482ms；t_first_action=482ms；t_level_1=482ms |

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
| impairment timing | t_detect_stable=520ms |
| recovery timing | t_detect_stable=538ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=520ms |
| raw recovery timing | t_detect_stable=538ms |

### T2

| 字段 | 内容 |
|---|---|
| Case ID | `T2` |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 1000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=462ms |
| recovery timing | t_detect_stable=450ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=462ms |
| raw recovery timing | t_detect_stable=450ms |

### T3

| 字段 | 内容 |
|---|---|
| Case ID | `T3` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 500kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=474ms |
| recovery timing | t_detect_stable=488ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=474ms |
| raw recovery timing | t_detect_stable=488ms |

### T4

| 字段 | 内容 |
|---|---|
| Case ID | `T4` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 5% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=530ms |
| recovery timing | t_detect_stable=540ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=530ms |
| raw recovery timing | t_detect_stable=540ms |

### T5

| 字段 | 内容 |
|---|---|
| Case ID | `T5` |
| 类型 | `transition` / priority `P0` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 20% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=early_warning/L1, current=congested/L3) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=false, analysis=过弱） |
| 动作摘要 | setEncodingParameters（共 3 次非 noop） |
| impairment timing | t_detect_stable=505ms |
| recovery timing | t_detect_warning=497ms；t_detect_congested=1537ms；t_first_action=497ms；t_level_1=497ms；t_level_2=1537ms；t_level_3=2547ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=505ms |
| raw recovery timing | t_detect_warning=497ms；t_detect_congested=1537ms；t_first_action=497ms；t_level_1=497ms；t_level_2=1537ms；t_level_3=2547ms |

### T6

| 字段 | 内容 |
|---|---|
| Case ID | `T6` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 180ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=477ms |
| recovery timing | t_detect_stable=430ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=477ms |
| raw recovery timing | t_detect_stable=430ms |

### T7

| 字段 | 内容 |
|---|---|
| Case ID | `T7` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 40ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=535ms |
| recovery timing | t_detect_stable=537ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=535ms |
| raw recovery timing | t_detect_stable=537ms |

### T8

| 字段 | 内容 |
|---|---|
| Case ID | `T8` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 2000kbps / RTT 55ms / loss 0.5% / jitter 12ms |
| impairment 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| recovery 网络 | 800kbps / RTT 120ms / loss 3% / jitter 30ms |
| 预期 QoS | 期望状态=congested；maxLevel=4；recovery=disabled |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=533ms |
| recovery timing | - |
| 诊断 | - |
| raw impairment timing | t_detect_stable=533ms |
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
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | FAIL（stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=2513ms；t_detect_stable=508ms；t_detect_congested=3554ms；t_first_action=2513ms；t_level_1=2513ms；t_level_2=3554ms；t_level_3=4595ms；t_level_4=5628ms；t_audio_only=5628ms |
| recovery timing | t_detect_congested=631ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2513ms；t_detect_stable=508ms；t_detect_congested=3554ms；t_first_action=2513ms；t_level_1=2513ms；t_level_2=3554ms；t_level_3=4595ms；t_level_4=5628ms；t_audio_only=5628ms |
| raw recovery timing | t_detect_congested=631ms |

### T10

| 字段 | 内容 |
|---|---|
| Case ID | `T10` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 15000kbps / RTT 15ms / loss 0.1% / jitter 1ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | FAIL（stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=2600ms；t_detect_stable=553ms；t_detect_congested=3641ms；t_first_action=2600ms；t_level_1=2600ms；t_level_2=3641ms；t_level_3=4677ms；t_level_4=5715ms；t_audio_only=5715ms |
| recovery timing | t_detect_congested=810ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2600ms；t_detect_stable=553ms；t_detect_congested=3641ms；t_first_action=2600ms；t_level_1=2600ms；t_level_2=3641ms；t_level_3=4677ms；t_level_4=5715ms；t_audio_only=5715ms |
| raw recovery timing | t_detect_congested=810ms |

### T11

| 字段 | 内容 |
|---|---|
| Case ID | `T11` |
| 类型 | `transition` / priority `P1` |
| baseline 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| impairment 网络 | 200kbps / RTT 500ms / loss 20% / jitter 50ms |
| recovery 网络 | 30000kbps / RTT 10ms / loss 0.1% / jitter 1ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=congested/L4, current=congested/L4)；recovery(best=congested/L4, current=congested/L4) |
| 结果 | FAIL（stateMatch=true, levelMatch=true, recoveryPassed=false, analysis=恢复过慢） |
| 动作摘要 | setEncodingParameters, enterAudioOnly（共 4 次非 noop） |
| impairment timing | t_detect_warning=2500ms；t_detect_stable=495ms；t_detect_congested=3540ms；t_first_action=2500ms；t_level_1=2500ms；t_level_2=3540ms；t_level_3=4579ms；t_level_4=5616ms；t_audio_only=5616ms |
| recovery timing | t_detect_congested=677ms |
| 诊断 | - |
| raw impairment timing | t_detect_warning=2500ms；t_detect_stable=495ms；t_detect_congested=3540ms；t_first_action=2500ms；t_level_1=2500ms；t_level_2=3540ms；t_level_3=4579ms；t_level_4=5616ms；t_audio_only=5616ms |
| raw recovery timing | t_detect_congested=677ms |

### S1

| 字段 | 内容 |
|---|---|
| Case ID | `S1` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 10% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning / congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=495ms |
| recovery timing | t_detect_stable=481ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=495ms |
| raw recovery timing | t_detect_stable=481ms |

### S2

| 字段 | 内容 |
|---|---|
| Case ID | `S2` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 300kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=congested；maxLevel=4 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=523ms |
| recovery timing | t_detect_stable=545ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=523ms |
| raw recovery timing | t_detect_stable=545ms |

### S3

| 字段 | 内容 |
|---|---|
| Case ID | `S3` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 200ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=392ms |
| recovery timing | t_detect_stable=375ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=392ms |
| raw recovery timing | t_detect_stable=375ms |

### S4

| 字段 | 内容 |
|---|---|
| Case ID | `S4` |
| 类型 | `burst` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 60ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=early_warning；maxLevel=2 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | FAIL（stateMatch=false, levelMatch=true, recoveryPassed=true, analysis=过弱） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=561ms |
| recovery timing | t_detect_stable=583ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=561ms |
| raw recovery timing | t_detect_stable=583ms |

