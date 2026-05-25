# 文档导航

这个目录同时包含：

- 当前仍在使用的主文档
- QoS 专项文档
- 运维 / 上线文档
- 机器生成的辅助结果

为了避免把一次性过程材料和当前结论文档混在一起，建议按下面顺序阅读。

## 1. 按角色阅读

| 角色 | 建议起点 |
|---|---|
| 新接手开发 | [DEVELOPMENT.md](./DEVELOPMENT.md) |
| 搭环境 / 查依赖 | [dependencies_cn.md](./dependencies_cn.md) → [README.md](../README.md) → [DEVELOPMENT.md](./DEVELOPMENT.md) |
| 做仓库全量回归 | [README.md](../README.md) → [DEVELOPMENT.md](./DEVELOPMENT.md) → [run_all_tests.sh](../scripts/run_all_tests.sh) → [full-regression-test-results.md](./full-regression-test-results.md) |
| 配 nightly 全量回归邮件 | [nightly-full-regression.md](./nightly-full-regression.md) → [run_all_tests.sh](../scripts/run_all_tests.sh) → [full-regression-test-results.md](./full-regression-test-results.md) |
| 做线上排障 | [troubleshooting_cn.md](./troubleshooting_cn.md) → [architecture_cn.md](./architecture_cn.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |
| 做 QoS 改动 | [qos-status.md](./qos-status.md) → [uplink-qos-design_cn.md](./uplink-qos-design_cn.md) → [downlink-qos-design_cn.md](./downlink-qos-design_cn.md) → [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) → [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md) → [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md) → [run_qos_tests.sh](../scripts/run_qos_tests.sh) |
| 做 WebRTC QoS 推拉流客户端 | [webrtc-qos-push-play-client-design_cn.md](./webrtc-qos-push-play-client-design_cn.md) → [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md) → [webrtc-qos-push-play-client-thread-model-design_cn.md](./webrtc-qos-push-play-client-thread-model-design_cn.md) → [webrtc-qos-push-play-client-implementation-checklist_cn.md](./webrtc-qos-push-play-client-implementation-checklist_cn.md) → `scripts/run_qos_tests.sh p2-acceptance` / `scripts/run_qos_tests.sh p3-thread-model-report` / 显式生产签收 `scripts/run_qos_tests.sh p3-thread-model-acceptance` |
| 查 worker 架构 | [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) |
| 查 QoS 详细 case 结果 | [uplink-qos-case-analysis.md](./uplink-qos-case-analysis.md) |
| 查 QoS 测试覆盖地图 | [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) |
| 做上线 / 运维准备 | [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |

## 2. 当前主文档

| 文档 | 用途 |
|---|---|
| [full-architecture-flow_cn.md](./full-architecture-flow_cn.md) | 全链路架构流程图：从信令加入到 Worker 媒体转发，覆盖进程模型、SDP/DTLS/ICE、IPC、BWE、Redis 多节点、QoS。 |
| [architecture_cn.md](./architecture_cn.md) | 运行时架构详解，覆盖线程/进程模型、关键时序、IPC、多节点与故障恢复。 |
| [dependencies_cn.md](./dependencies_cn.md) | 构建 / 运行 / 测试依赖总览，统一说明系统包、vendored 依赖、Node harness 依赖和 `setup.sh` / CMake 解析规则。 |
| [webrtc-qos-push-play-client-design_cn.md](./webrtc-qos-push-play-client-design_cn.md) | `webrtc_qos_sdk` 推拉流客户端设计方案，定义同一目录下的 push/play、信令复用、媒体面接 SDK、验收门禁和阶段计划。 |
| [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md) | 推拉流客户端“大第二期”设计方案，按可实施性、可验证性、可观测性三类门禁组织 QoS 闭环、弱网自动化、观测日志、video-only、实时编码器、输入源和 QoE 验证。 |
| [webrtc-qos-push-play-client-thread-model-design_cn.md](./webrtc-qos-push-play-client-thread-model-design_cn.md) | 推拉流客户端线程模型升级方案，定义多摄像头/多 track 场景下的 push/play 线程划分、SDK 单线程边界、队列背压、观测和验收门禁。 |
| [webrtc-qos-push-play-client-implementation-checklist_cn.md](./webrtc-qos-push-play-client-implementation-checklist_cn.md) | 推拉流客户端实现对照清单，记录已实现项、验证命令、版本差异和动态验收缺口。 |
| [qos-status.md](./qos-status.md) | QoS 总状态摘要，统一给出 browser uplink / WebRTC QoS P2/P3 / downlink 的当前口径与结果入口。 |
| [full-regression-test-results.md](./full-regression-test-results.md) | 最新一次 `scripts/run_all_tests.sh` 生成的仓库全量回归结果页，按选择分组记录逐任务 PASS/FAIL 和耗时。 |
| [nightly-full-regression.md](./nightly-full-regression.md) | nightly 全量回归自动化说明，包含 03:00 cron 安装、日志目录、邮件摘要和 Markdown 附件约定。 |
| [downlink-qos-status.md](./downlink-qos-status.md) | downlink QoS 当前状态摘要，说明当前范围、当前结果入口和后续边界。 |
| [troubleshooting_cn.md](./troubleshooting_cn.md) | 运行时排障手册，覆盖 join/IPC/Redis/QoS/录制/worker crash 的定位路径。 |
| [DEVELOPMENT.md](./DEVELOPMENT.md) | 项目开发主文档，包含架构、线程模型、构建与测试入口。 |
| [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) | 监控、告警与值班处理流程。 |
| [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) | 上线前检查项。 |
| [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) | 基于 `mediasoup-worker 3.14.6` 源码的架构与模块分析，从进程入口到 `Router/Transport/Producer/Consumer/BWE`。 |
| [REDIS_KEY_GUIDELINES.md](./REDIS_KEY_GUIDELINES.md) | Redis key 设计和约束。 |

## 3. QoS 专项文档

推荐阅读顺序：

1. [qos-status.md](./qos-status.md)
2. [uplink-qos-final-report.md](./uplink-qos-final-report.md)
3. [uplink-qos-test-results-summary.md](./uplink-qos-test-results-summary.md)
4. [uplink-qos-case-results.md](./uplink-qos-case-results.md)
5. [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md)
6. [webrtc-qos-push-play-client-thread-model-design_cn.md](./webrtc-qos-push-play-client-thread-model-design_cn.md)
7. [downlink-qos-status.md](./downlink-qos-status.md)
8. [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md)
9. [downlink-qos-case-results.md](./downlink-qos-case-results.md)
10. [uplink-qos-design_cn.md](./uplink-qos-design_cn.md)
11. [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
12. [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)
13. [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md)
14. [downlink-qos-worker-validation_cn.md](./downlink-qos-worker-validation_cn.md)
15. [downlink-qos-implementation-plan_cn.md](./downlink-qos-implementation-plan_cn.md)
16. [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md)
17. [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md)
18. [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)
19. [uplink-qos-briefing.md](./uplink-qos-briefing.md)
20. [uplink-qos-case-analysis.md](./uplink-qos-case-analysis.md)
21. [uplink-qos-blind-spot-scenario.md](./uplink-qos-blind-spot-scenario.md)
22. [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md)
23. [uplink-qos-test-execution-checklist.md](./uplink-qos-test-execution-checklist.md)
24. [qos-test-coverage_cn.md](./qos-test-coverage_cn.md)

说明：

- [qos-status.md](./qos-status.md) 是当前 QoS 总口径主入口。
- [uplink-qos-design_cn.md](./uplink-qos-design_cn.md) 用中文说明当前 uplink QoS 的设计、控制链路和边界。
- [downlink-qos-design_cn.md](./downlink-qos-design_cn.md) 用中文说明当前 `downlink QoS v1` 的设计边界与基础模型。
- [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) 用中文定义 `downlink QoS v2` 的房间级 demand 聚合、显式预算分配和 publisher 供给回收方案。
- [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md) 用中文给出 `downlink QoS v2` 的分阶段实施方案，细化到类和函数级别。
- [downlink-qos-worker-validation_cn.md](./downlink-qos-worker-validation_cn.md) 用中文整理正式做 downlink QoS 之前需要先补齐的 worker / API 黑盒验证项。
- [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md) 用中文定义 `downlink QoS v3` 的目标、worker 边界和 control plane 的职责修正。
- [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md) 用中文给出 `downlink QoS v3` 的实施路径，并明确当前“不继续做强控制面最终 allocator”的决策。
- [downlink-qos-implementation-plan_cn.md](./downlink-qos-implementation-plan_cn.md) 用中文给出 `downlink QoS v1` 的分阶段实施计划。
- [downlink-qos-status.md](./downlink-qos-status.md) 是当前 downlink 范围和结果入口摘要。
- 当前签收口径，以 [qos-status.md](./qos-status.md) 为总入口，并按各分支状态页下钻。
- [uplink-qos-boundaries.md](./uplink-qos-boundaries.md) 用来说明“底层 WebRTC 自动能力”和“本仓库 uplink QoS 策略能力”的职责边界。
- [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) 用来回答“上下行 QoS 现在分别测了哪些场景、在哪一层验证”。
- [uplink-qos-blind-spot-scenario.md](./uplink-qos-blind-spot-scenario.md) 用来汇总“高质量网络突入长时盲区再恢复”这一类极端转场场景的理论时序和实测结果。
- [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md) 用来记录 `BW2` 一类 loopback 边界 case 的专项排查结论、runner 特性和后续治理方向。
- full matrix 当前机器结果在 [generated/uplink-qos-matrix-report.json](./generated/uplink-qos-matrix-report.json)。
- targeted rerun 结果由脚本生成到 `docs/generated/uplink-qos-case-results.targeted.md` 和 `docs/generated/uplink-qos-matrix-report.targeted.json`；当前仓库不保留这类临时 targeted 结果。
- WebRTC QoS P2 smoke 当前机器报告在 [generated/webrtc-qos-plain-p2-smoke-report.md](./generated/webrtc-qos-plain-p2-smoke-report.md)，原始 JSON 在 [generated/webrtc-qos-plain-p2-smoke-report.json](./generated/webrtc-qos-plain-p2-smoke-report.json)。当前主报告为 `sourceMode=copy`、`6 / 6 PASS`；`qosMainline/sdkRuntimeObservability/nativeDecodeQoe/recoveryFirstFrame/weakNetworkCoverage` 为 `PASS`，`encoderRuntime=SKIP` 因 copy 输入不经过实时 x264 encoder。
- WebRTC QoS P2 执行方案在 [webrtc-qos-push-play-client-p2-design_cn.md](./webrtc-qos-push-play-client-p2-design_cn.md)，其中 P2-M8b 把 `drop_recover` 清网后 15 秒内 decoded frames 增长列为恢复首帧硬门禁；专项报告在 [generated/webrtc-qos-plain-p2-recovery-first-frame-report.md](./generated/webrtc-qos-plain-p2-recovery-first-frame-report.md)，原始 JSON 在 [generated/webrtc-qos-plain-p2-recovery-first-frame-report.json](./generated/webrtc-qos-plain-p2-recovery-first-frame-report.json)。
- WebRTC QoS P2 MP4 decode-loop baseline 报告在 [generated/webrtc-qos-plain-p2-mp4-decode-loop-report.md](./generated/webrtc-qos-plain-p2-mp4-decode-loop-report.md)，原始 JSON 在 [generated/webrtc-qos-plain-p2-mp4-decode-loop-report.json](./generated/webrtc-qos-plain-p2-mp4-decode-loop-report.json)。
- WebRTC QoS P2 browser receiver 报告在 [generated/webrtc-qos-plain-p2-browser-receiver-report.md](./generated/webrtc-qos-plain-p2-browser-receiver-report.md)，原始 JSON 在 [generated/webrtc-qos-plain-p2-browser-receiver-report.json](./generated/webrtc-qos-plain-p2-browser-receiver-report.json)。当前本机 headless Chromium 缺 H264 receive capability，所以浏览器收流 case 记录为环境 `SKIP`。
- WebRTC QoS P2 V4L2 source 报告在 [generated/webrtc-qos-plain-p2-v4l2-report.md](./generated/webrtc-qos-plain-p2-v4l2-report.md)，原始 JSON 在 [generated/webrtc-qos-plain-p2-v4l2-report.json](./generated/webrtc-qos-plain-p2-v4l2-report.json)。当前机器无 `/dev/video0`，所以 V4L2 baseline 记录为环境 `SKIP`。
- WebRTC QoS P2 聚合验收入口是 `scripts/run_qos_tests.sh p2-acceptance`；它执行构建、plain client 单测、ORTC/TWCC targeted test、PlainPublish 集成测试、边界检查和报告复核。脚本优先使用显式 `--cmake-prefix-path` 或环境变量 `CMAKE_PREFIX_PATH`，未配置时会自动探测 `../webrtc_qos_sdk/dist/linux-x86_64`。正式刷新报告入口是 `scripts/run_webrtc_qos_plain_p2_acceptance.sh --run-smoke --enable-netem`，会重跑主弱网、恢复首帧、MP4 decode-loop 和 V4L2 报告。轻量离线复核入口是 `scripts/run_qos_tests.sh p2-report`，只校验新 plain client 没有回退到旧自研 QoS/packetizer，并校验当前 `docs/generated` 下主弱网、MP4 decode-loop、V4L2、browser receiver、恢复首帧报告的 PASS/SKIP/PARTIAL 口径。
- WebRTC QoS P3 线程模型自动化验收入口是 `scripts/run_qos_tests.sh p3-thread-model-report`；它生成 [generated/webrtc-qos-plain-thread-model-boundary-report.md](./generated/webrtc-qos-plain-thread-model-boundary-report.md)、[generated/webrtc-qos-plain-p3-thread-model-smoke-report.md](./generated/webrtc-qos-plain-p3-thread-model-smoke-report.md)、[generated/webrtc-qos-plain-p3-thread-model-decode-loop-report.md](./generated/webrtc-qos-plain-p3-thread-model-decode-loop-report.md)、[generated/webrtc-qos-plain-p3-thread-model-slow-encoder-report.md](./generated/webrtc-qos-plain-p3-thread-model-slow-encoder-report.md)、[generated/webrtc-qos-plain-p3-thread-model-slow-sink-report.md](./generated/webrtc-qos-plain-p3-thread-model-slow-sink-report.md)、[generated/webrtc-qos-plain-p3-thread-model-weak-network-report.md](./generated/webrtc-qos-plain-p3-thread-model-weak-network-report.md) 和 [generated/webrtc-qos-plain-p3-thread-model-v4l2-report.md](./generated/webrtc-qos-plain-p3-thread-model-v4l2-report.md)，覆盖静态 owner/队列/日志边界、two-track synthetic push/play、two-track MP4 decode-loop、慢编码注入、慢 sink 注入、弱网 two-track、per-track play QoE 和 V4L2 capability 动态 smoke。V4L2 capture/raw/encode split 代码和静态边界已落地；生产签收入口是 `scripts/run_qos_tests.sh p3-thread-model-acceptance`，要求 netem 弱网和真实双 camera V4L2 全部 PASS，不能用 SKIP/PARTIAL 代替。当前 weak-network strict 已 PASS，V4L2 strict 因缺 `/dev/video0` 和 `/dev/video1` 仍不能签生产。
- downlink 当前状态摘要在 [downlink-qos-status.md](./downlink-qos-status.md)。
- 一次性过程稿、历史 review、商业化排期和旧归档快照不再保留在 `docs/` 中；如需追历史判断，请直接看 git 历史。

## 4. QoS 测试入口

统一脚本：

- [run_qos_tests.sh](../scripts/run_qos_tests.sh)

常用命令：

```bash
cd /root/mediasoup-cpp
./scripts/run_qos_tests.sh
./scripts/run_qos_tests.sh --skip-browser
./scripts/run_qos_tests.sh client-js cpp-unit
./scripts/run_qos_tests.sh --list
```

分组说明：

- `client-js`: 客户端 QoS JS 单测
- `cpp-unit`: 服务端 QoS 单测
- `cpp-integration`: 服务端 QoS 集成测试
- `cpp-accuracy`: QoS 统计精度测试
- `cpp-recording`: QoS 录制精度测试
- `p2-report`: WebRTC QoS P2 generated reports 离线验收
- `p2-acceptance`: WebRTC QoS P2 构建、单测、边界和报告聚合验收
- `p3-thread-model-report`: WebRTC QoS P3 线程模型自动化报告，覆盖静态边界、two-track synthetic/decode-loop、慢编码/慢 sink 注入、弱网和 V4L2 capability SKIP/PARTIAL 规则
- `p3-thread-model-acceptance`: WebRTC QoS P3 生产签收入口，必须显式运行，要求 netem 弱网和真实双 camera V4L2 全部 PASS
- `node-harness`: Node QoS harness
- `browser-harness`: browser signaling / loopback
- `matrix`: browser loopback 弱网矩阵
- `downlink-matrix`: browser downlink 弱网矩阵

## 5. 运维 / 缺陷专题

| 文档 | 用途 |
|---|---|
| [fix-subscriber-busyloop.md](./fix-subscriber-busyloop.md) | subscriber busy loop 问题的根因和修复过程。 |

## 6. 目录约定

后续建议按下面规则继续维护：

- 当前结论文档：放在“当前主文档”或“QoS 专项文档”
- 机器生成结果：优先用 `.json` / artifact 形式保留，只保留当前仍被 README 或状态页引用的结果
- 临时文件不要留在 `docs/`
