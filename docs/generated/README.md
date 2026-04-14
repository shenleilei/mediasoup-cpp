# 机器生成结果

这个目录用于存放自动化脚本生成的辅助结果。

当前收录：

- [uplink-qos-matrix-report.json](./uplink-qos-matrix-report.json)
- downlink-qos-matrix-report.json（由 `run_downlink_matrix.mjs` 生成）

说明：

- 这里的文件可能会被后续脚本覆盖
- 它们用于追溯或辅助分析，不直接等同于最终签收口径
- 当前 QoS 结论文档仍以 [uplink-qos-final-report.md](../uplink-qos-final-report.md) 为准
- downlink 报告由 `render_downlink_case_report.mjs` 渲染到 `docs/downlink-qos-case-results.md`
