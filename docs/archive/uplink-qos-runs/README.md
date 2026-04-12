# uplink-qos-runs

这个目录用于保存 uplink QoS 报告的历史快照。

约定：

- 每次 matrix / case report 生成时，都会按报告里的 `generatedAt` 创建一个同名目录。
- 目录名中的 `:` 会替换为 `-`，便于文件系统保存，例如：
  `2026-04-12T08-53-56.649Z`
- 归档目录内会保留该轮生成的 JSON / markdown 以及 `metadata.json`，用于和上一轮直接对比。

当前主报告仍使用：

- `docs/generated/uplink-qos-matrix-report.json`
- `docs/generated/uplink-qos-matrix-report.targeted.json`
- `docs/uplink-qos-case-results.md`
- `docs/generated/uplink-qos-case-results.targeted.md`
