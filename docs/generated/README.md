# 机器生成结果

这个目录用于存放自动化脚本生成的辅助结果。

当前收录：

## Browser uplink

- [uplink-qos-matrix-report.json](./uplink-qos-matrix-report.json)
- [uplink-qos-matrix-report.targeted.json](./uplink-qos-matrix-report.targeted.json)
- [uplink-qos-case-results.targeted.md](./uplink-qos-case-results.targeted.md)

## PlainTransport C++ client uplink

- [uplink-qos-cpp-client-matrix-report.json](./uplink-qos-cpp-client-matrix-report.json)
- [uplink-qos-cpp-client-matrix-report.targeted.json](./uplink-qos-cpp-client-matrix-report.targeted.json)
- [uplink-qos-cpp-client-case-results.targeted.md](./uplink-qos-cpp-client-case-results.targeted.md)

## Downlink

- [downlink-qos-matrix-report.json](./downlink-qos-matrix-report.json)
- [downlink-qos-matrix-report.targeted.json](./downlink-qos-matrix-report.targeted.json)
- [downlink-qos-case-results.targeted.md](./downlink-qos-case-results.targeted.md)

## TWCC A/B

- [twcc-ab-report.md](./twcc-ab-report.md)
- [twcc-ab-g1-vs-g2.md](./twcc-ab-g1-vs-g2.md)
- [twcc-ab-g1-vs-g2.json](./twcc-ab-g1-vs-g2.json)
- [twcc-ab-g0-vs-g2.md](./twcc-ab-g0-vs-g2.md)
- [twcc-ab-g0-vs-g2.json](./twcc-ab-g0-vs-g2.json)
- [twcc-ab-raw-groups.json](./twcc-ab-raw-groups.json)

说明：

- 这里的文件可能会被后续脚本覆盖
- 它们用于追溯或辅助分析，不直接等同于最终签收口径
- 当前 QoS 总状态摘要见 [qos-status.md](../qos/README.md)
- Linux plain-client QoS 当前状态摘要见 [plain-client-qos-status.md](../qos/plain-client/README.md)
- downlink 当前状态摘要见 [downlink-qos-status.md](../qos/downlink/README.md)
- downlink 报告由 `render_downlink_case_report.mjs` 渲染到 `docs/downlink-qos-case-results.md`
- TWCC A/B 报告由 `tests/qos_harness/run_twcc_ab_eval.mjs` 生成；默认全量 QoS 入口会刷新这些文件
