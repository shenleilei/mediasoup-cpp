# downlink QoS matrix 历史归档

每次 `run_downlink_matrix.mjs` 执行后，结果按生成时间归档到子目录。

结构和 `uplink-qos-runs/` 对齐：

```
<timestamp>/
  docs/generated/downlink-qos-matrix-report.json
  docs/downlink-qos-case-results.md          (如果 render 过)
  metadata.json
```
