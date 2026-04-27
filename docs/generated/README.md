# 机器生成结果

这个目录用于存放自动化脚本生成的辅助结果。

当前收录：

- [uplink-qos-matrix-report.json](./uplink-qos-matrix-report.json)
- [uplink-qos-cpp-client-matrix-report.json](./uplink-qos-cpp-client-matrix-report.json)
- [uplink-qos-cpp-client-matrix-report.targeted.json](./uplink-qos-cpp-client-matrix-report.targeted.json)
- [plain-client-qos-case-results.md](../plain-client-qos-case-results.md)
- downlink-qos-matrix-report.json（由 `run_downlink_matrix.mjs` 生成）

说明：

- 这里的文件可能会被后续脚本覆盖
- 它们用于追溯或辅助分析，不直接等同于最终签收口径
- 当前 QoS 总状态摘要见 [qos-status.md](../qos-status.md)
- Linux plain-client QoS 当前状态摘要见 [plain-client-qos-status.md](../plain-client-qos-status.md)
- downlink 当前状态摘要见 [downlink-qos-status.md](../downlink-qos-status.md)
- downlink 报告由 `render_downlink_case_report.mjs` 渲染到 `docs/downlink-qos-case-results.md`
