# PlainTransport C++ Client QoS Matrix 逐 Case 结果

生成时间：`2026-04-16T14:59:42.300Z`

## 1. 汇总

- 总 Case：`1`
- 已执行：`1`
- 通过：`1`
- 失败：`0`
- 错误：`0`
- runner：`cpp_client`

## 2. 快速跳转

- 失败 / 错误：无
- bw_sweep：[BW2](#bw2)

## 3. 逐 Case 结果

### BW2

| 字段 | 内容 |
|---|---|
| Case ID | `BW2` |
| 类型 | `bw_sweep` / priority `P1` |
| baseline 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| impairment 网络 | 2000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| recovery 网络 | 4000kbps / RTT 25ms / loss 0.1% / jitter 5ms |
| 预期 QoS | 期望状态=stable / early_warning；maxLevel=1 |
| 实际 QoS | baseline(current=stable/L0)；impairment(peak=stable/L0, current=stable/L0)；recovery(best=stable/L0, current=stable/L0) |
| 结果 | PASS（符合） |
| 动作摘要 | 无非 noop 动作 |
| impairment timing | t_detect_stable=64ms |
| recovery timing | t_detect_stable=53ms |
| 诊断 | - |
| raw impairment timing | t_detect_stable=64ms |
| raw recovery timing | t_detect_stable=53ms |
