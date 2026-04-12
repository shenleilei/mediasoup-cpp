# 上行 QoS 执行清单

日期：`2026-04-12`

## 0. 统一执行入口

当前 QoS 测试建议优先通过统一脚本执行：

- [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh)

常用命令：

```bash
cd /root/mediasoup-cpp
./scripts/run_qos_tests.sh
./scripts/run_qos_tests.sh --skip-browser
./scripts/run_qos_tests.sh client-js cpp-unit
```

## 1. 已完成项

- [x] 服务端 QoS 单元测试通过
  结果：`18 / 18 PASS`

- [x] 服务端 QoS 集成测试通过
  结果：`12 / 12 PASS`

- [x] client QoS JS 单测通过
  结果：`27 / 27 PASS`

- [x] Node harness 回归通过
  范围：
  `publish_snapshot`
  `stale_seq`
  `policy_update`
  `auto_override_poor`
  `override_force_audio_only`
  `manual_clear`

- [x] browser signaling harness 回归通过
  范围：`browser_server_signal`

- [x] browser loopback 固定弱网回归通过
  范围：`browser_loopback`

- [x] browser matrix 基线组通过
  范围：`B1-B3`

- [x] browser matrix 带宽扫描组通过
  范围：`BW1-BW7`

- [x] browser matrix 丢包扫描组通过
  范围：`L1-L8`

- [x] browser matrix RTT 扫描组通过
  范围：`R1-R6`

- [x] browser matrix jitter 扫描组通过
  范围：`J1-J5`

- [x] browser matrix transition 组通过
  范围：`T1-T8`

- [x] browser matrix burst 组通过
  范围：`S1-S4`

## 2. 本轮关闭的问题类型

- [x] `level 4` 保护后无法恢复
- [x] recovery 判定被 phase 尾部抖动误伤
- [x] RTT case 过强或过弱
- [x] jitter case 过弱
- [x] `qualityLimitationReason=bandwidth` 恢复尾部假阳性
- [x] severe case 在 loopback 中因 track 停送导致 stats 不连续

## 3. 文档口径

- [x] 已更新 [uplink-qos-test-results-summary.md](/root/mediasoup-cpp/docs/uplink-qos-test-results-summary.md)
- [x] 已新增 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md)
- [x] 已新增 [README.md](/root/mediasoup-cpp/docs/README.md) 作为 docs 导航入口

## 4. 已决定的后续事项

- [x] 需要补一份“单次串行 41 case”的 full-matrix 独立归档 artifact
  说明：这项工作已归入 P1 的“full matrix 稳定性固化与归档”。
  当前 `run_matrix.mjs` 的 JSON 输出会被后续 targeted rerun 覆盖，因此仍需要补一份不被覆盖的独立产物；
  但这不影响本轮“自动化验证已收敛并可签收”的结论。
