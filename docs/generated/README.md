# 机器生成结果

这个目录用于存放自动化脚本生成的辅助结果。

当前收录：

- [uplink-qos-matrix-report.json](./uplink-qos-matrix-report.json)
- [webrtc-qos-plain-p2-smoke-report.md](./webrtc-qos-plain-p2-smoke-report.md)
- [webrtc-qos-plain-p2-smoke-report.json](./webrtc-qos-plain-p2-smoke-report.json)
- [webrtc-qos-plain-thread-model-boundary-report.md](./webrtc-qos-plain-thread-model-boundary-report.md)
- [webrtc-qos-plain-p3-thread-model-smoke-report.md](./webrtc-qos-plain-p3-thread-model-smoke-report.md)
- [webrtc-qos-plain-p3-thread-model-decode-loop-report.md](./webrtc-qos-plain-p3-thread-model-decode-loop-report.md)
- [webrtc-qos-plain-p3-thread-model-slow-encoder-report.md](./webrtc-qos-plain-p3-thread-model-slow-encoder-report.md)
- [webrtc-qos-plain-p3-thread-model-slow-sink-report.md](./webrtc-qos-plain-p3-thread-model-slow-sink-report.md)
- [webrtc-qos-plain-p3-thread-model-weak-network-report.md](./webrtc-qos-plain-p3-thread-model-weak-network-report.md)
- [webrtc-qos-plain-p3-thread-model-v4l2-report.md](./webrtc-qos-plain-p3-thread-model-v4l2-report.md)
- downlink-qos-matrix-report.json（由 `run_downlink_matrix.mjs` 生成）

说明：

- 这里的文件可能会被后续脚本覆盖
- 它们用于追溯或辅助分析，不直接等同于最终签收口径
- 当前 QoS 总状态摘要见 [qos-status.md](../qos-status.md)
- WebRTC QoS P2 smoke 当前报告见 [webrtc-qos-plain-p2-smoke-report.md](./webrtc-qos-plain-p2-smoke-report.md)
- WebRTC QoS P3 线程模型当前报告见 [webrtc-qos-plain-thread-model-boundary-report.md](./webrtc-qos-plain-thread-model-boundary-report.md) 和 `webrtc-qos-plain-p3-thread-model-*.md`
- downlink 当前状态摘要见 [downlink-qos-status.md](../downlink-qos-status.md)
- downlink 报告由 `render_downlink_case_report.mjs` 渲染到 `docs/downlink-qos-case-results.md`
