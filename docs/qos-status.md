# QoS 当前状态总览

> 文档性质
>
> 这是仓库内 QoS 的总摘要入口。
> 它只回答当前范围、当前结果入口和各分支文档应该怎么读，
> 不承担历史设计过程或逐 case 细节展开。

## 1. 当前结论

当前仓库的 QoS 文档体系已经拆成四条主线：

1. browser uplink QoS
2. WebRTC QoS P2 push/play client
3. WebRTC QoS P3 push/play thread model
4. downlink QoS

当前稳定口径：

- browser uplink 主 gate：`55 case`；当前增量验证为原 `43 case` 主 gate `PASS` + `GD1-GD12` targeted `PASS`
- WebRTC QoS P2：native push/play 主报告以 `sourceMode=copy` 覆盖 baseline/delay/loss/bandwidth/recovery，`6 / 6 PASS`，`failedChecks=0`；`qosMainline/sdkRuntimeObservability/nativeDecodeQoe/recoveryFirstFrame/weakNetworkCoverage` 为 `PASS`，`encoderRuntime=SKIP` 因 copy 输入不经过实时 x264 encoder
- WebRTC QoS P3：`p3-thread-model-report` 自动化入口当前可通过，覆盖静态 owner/队列/日志边界、two-track synthetic、two-track MP4 decode-loop、慢编码注入、慢 sink 注入、弱网 two-track 和 per-track QoE；V4L2 capability 报告当前因缺 `/dev/video0` 和 `/dev/video1` 记录 `PARTIAL/SKIP`
- WebRTC QoS P3 生产签收：入口是 `scripts/run_qos_tests.sh p3-thread-model-acceptance`，要求 netem 弱网和真实双 camera V4L2 全部 PASS；当前机器 weak-network strict 已 PASS，V4L2 strict 未签收
- downlink 当前范围：subscriber receive control + zero-demand publisher pause/resume coordination
- FEC/RTX、`dynacast` 与 room-level global bitrate budgeting 仍是后续能力，不计入当前已完成口径

## 2. 按主题查看

### 2.1 browser uplink

- 最终结论：
  [uplink-qos-final-report.md](./uplink-qos-final-report.md)
- 结果汇总：
  [uplink-qos-test-results-summary.md](./uplink-qos-test-results-summary.md)
- 逐 case：
  [uplink-qos-case-results.md](./uplink-qos-case-results.md)

### 2.2 WebRTC QoS P2

- 推拉流客户端总设计：
  [webrtc-qos-push-play-client-design_cn.md](./webrtc-qos-push-play-client-design_cn.md)
- WebRTC QoS P2 执行方案：
  [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md)
- 实现对照清单：
  [webrtc-qos-push-play-client-implementation-checklist_cn.md](./webrtc-qos-push-play-client-implementation-checklist_cn.md)
- WebRTC QoS P2 当前主报告：
  [generated/webrtc-qos-plain-p2-smoke-report.md](./generated/webrtc-qos-plain-p2-smoke-report.md)
- WebRTC QoS P2 恢复首帧专项报告：
  [generated/webrtc-qos-plain-p2-recovery-first-frame-report.md](./generated/webrtc-qos-plain-p2-recovery-first-frame-report.md)

### 2.3 WebRTC QoS P3 thread model

- 线程模型升级设计：
  [webrtc-qos-push-play-client-thread-model-design_cn.md](./webrtc-qos-push-play-client-thread-model-design_cn.md)
- 静态边界报告：
  [generated/webrtc-qos-plain-thread-model-boundary-report.md](./generated/webrtc-qos-plain-thread-model-boundary-report.md)
- two-track synthetic 报告：
  [generated/webrtc-qos-plain-p3-thread-model-smoke-report.md](./generated/webrtc-qos-plain-p3-thread-model-smoke-report.md)
- two-track MP4 decode-loop 报告：
  [generated/webrtc-qos-plain-p3-thread-model-decode-loop-report.md](./generated/webrtc-qos-plain-p3-thread-model-decode-loop-report.md)
- 慢编码注入报告：
  [generated/webrtc-qos-plain-p3-thread-model-slow-encoder-report.md](./generated/webrtc-qos-plain-p3-thread-model-slow-encoder-report.md)
- 慢 sink 注入报告：
  [generated/webrtc-qos-plain-p3-thread-model-slow-sink-report.md](./generated/webrtc-qos-plain-p3-thread-model-slow-sink-report.md)
- weak-network two-track 报告：
  [generated/webrtc-qos-plain-p3-thread-model-weak-network-report.md](./generated/webrtc-qos-plain-p3-thread-model-weak-network-report.md)
- V4L2 capability 报告：
  [generated/webrtc-qos-plain-p3-thread-model-v4l2-report.md](./generated/webrtc-qos-plain-p3-thread-model-v4l2-report.md)

### 2.4 downlink

- 当前状态摘要：
  [downlink-qos-status.md](./downlink-qos-status.md)
- 当前机器汇总页：
  [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md)
- 当前机器结果页：
  [downlink-qos-case-results.md](./downlink-qos-case-results.md)
- 测试覆盖地图：
  [qos-test-coverage_cn.md](./qos-test-coverage_cn.md)

## 3. 当前范围边界

### 3.1 已进入“当前主口径”的内容

- browser uplink 主测试集
- WebRTC QoS P2 copy 输入弱网主报告、恢复首帧、MP4 decode-loop、browser receiver 和 V4L2 环境能力门禁
- WebRTC QoS P3 自动化线程模型报告：静态边界、two-track synthetic、two-track MP4 decode-loop、慢编码/慢 sink 注入、弱网 two-track、per-track QoE 和 V4L2 capability SKIP/PARTIAL 规则
- downlink 现有 subscriber-side / publisher-supply 协调能力

### 3.2 不在“当前已完成”口径里的内容

- WebRTC QoS P3 真实双 camera V4L2 生产签收
- WebRTC QoS native push/play FEC/RTX 能力验收
- `dynacast`
- room-level global bitrate budgeting
- 非 uplink 范围外的 browser UI / demo 层签收

## 4. 维护规则

后续只要 QoS 能力继续变化，优先更新：

1. 这份总状态页
2. 对应分支状态页或设计页
   - [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md)
   - [webrtc-qos-push-play-client-thread-model-design_cn.md](./webrtc-qos-push-play-client-thread-model-design_cn.md)
   - [downlink-qos-status.md](./downlink-qos-status.md)
3. 对应主结果页
4. `docs/generated/README.md`
