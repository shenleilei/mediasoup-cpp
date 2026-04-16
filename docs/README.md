# 文档导航

这个目录同时包含：

- 当前仍在使用的主文档
- QoS 专项文档
- 运维 / 上线文档
- 历史 review / 开发记录
- 机器生成的辅助结果

为了避免把“历史快照”和“当前结论”混在一起，建议按下面顺序阅读。

## 0. Codex 接续

如果是重新打开 Codex 继续做上一次的工作，先读：

1. [docs/.codex](.codex)
2. [docs/README.md](./README.md)
3. 如果是 QoS 边界 / loopback 问题，再读：
   - [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md)
   - [.kiro/skills/qos-boundary-analysis.md](../.kiro/skills/qos-boundary-analysis.md)

说明：

- 当前仓库没有平台级“自动加载上下文”能力。
- `docs/.codex` 是约定的 handoff 文件，用来记录“下次直接继续做什么”。
- 如果要长期保持上下文连续性，每次中断前都应该更新 `docs/.codex`。

## 1. 按角色阅读

| 角色 | 建议起点 |
|---|---|
| 新接手开发 | [DEVELOPMENT.md](./DEVELOPMENT.md) |
| 做线上排障 | [troubleshooting_cn.md](./troubleshooting_cn.md) → [architecture_cn.md](./architecture_cn.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |
| 做 QoS 改动 | [qos-status.md](./qos-status.md) → [uplink-qos-design_cn.md](./uplink-qos-design_cn.md) → [downlink-qos-design_cn.md](./downlink-qos-design_cn.md) → [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) → [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md) → [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md) → [run_qos_tests.sh](../scripts/run_qos_tests.sh) |
| 做 Linux client / PlainTransport C++ client | [plain-client-qos-status.md](./plain-client-qos-status.md) → [linux-client-architecture_cn.md](./linux-client-architecture_cn.md) → [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md) |
| 查 worker 架构 | [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) |
| 查 QoS 详细 case 结果 | [uplink-qos-case-analysis.md](./uplink-qos-case-analysis.md) |
| 查 QoS 测试覆盖地图 | [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) |
| 做上线 / 运维准备 | [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |
| 做商业化排期 | [commercialization-plan_cn.md](./commercialization-plan_cn.md) → [q1.md](./q1.md) |
| 查历史背景 | [archive/CODE_REVIEW_2026-04-08.md](./archive/CODE_REVIEW_2026-04-08.md) / [archive/DEVELOPMENT_2026-04-08.md](./archive/DEVELOPMENT_2026-04-08.md) |

## 2. 当前主文档

| 文档 | 用途 |
|---|---|
| [full-architecture-flow_cn.md](./full-architecture-flow_cn.md) | 全链路架构流程图：从信令加入到 Worker 媒体转发，覆盖进程模型、SDP/DTLS/ICE、IPC、BWE、Redis 多节点、QoS。 |
| [architecture_cn.md](./architecture_cn.md) | 运行时架构详解，覆盖线程/进程模型、关键时序、IPC、多节点与故障恢复。 |
| [linux-client-architecture_cn.md](./linux-client-architecture_cn.md) | Linux `PlainTransport C++ client` 架构说明，覆盖 FFmpeg、RTP/RTCP、QoS 控制器、多 track 和服务端交互。 |
| [qos-status.md](./qos-status.md) | QoS 总状态摘要，统一给出 browser uplink / plain-client / downlink 的当前口径与结果入口。 |
| [plain-client-qos-status.md](./plain-client-qos-status.md) | Linux `PlainTransport C++ client` QoS 当前状态摘要，集中给出签收范围、结果入口与主文档。 |
| [downlink-qos-status.md](./downlink-qos-status.md) | downlink QoS 当前状态摘要，说明当前范围、当前结果入口和后续边界。 |
| [troubleshooting_cn.md](./troubleshooting_cn.md) | 运行时排障手册，覆盖 join/IPC/Redis/QoS/录制/worker crash 的定位路径。 |
| [DEVELOPMENT.md](./DEVELOPMENT.md) | 项目开发主文档，包含架构、线程模型、构建与测试入口。 |
| [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) | 监控、告警与值班处理流程。 |
| [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) | 上线前检查项。 |
| [commercialization-plan_cn.md](./commercialization-plan_cn.md) | 商业化路线图，回答三个月可商用与十二个月领先要补什么。 |
| [q1.md](./q1.md) | 截止 `2026-07-14` 的执行版计划，按 `2026-07-01` 上线和 `2026-07-14` QoS 强化拆分。 |
| [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) | 基于 `mediasoup-worker 3.14.6` 源码的架构与模块分析，从进程入口到 `Router/Transport/Producer/Consumer/BWE`。 |
| [REDIS_KEY_GUIDELINES.md](./REDIS_KEY_GUIDELINES.md) | Redis key 设计和约束。 |

## 3. QoS 专项文档

推荐阅读顺序：

1. [qos-status.md](./qos-status.md)
2. [uplink-qos-final-report.md](./uplink-qos-final-report.md)
3. [uplink-qos-test-results-summary.md](./uplink-qos-test-results-summary.md)
4. [uplink-qos-case-results.md](./uplink-qos-case-results.md)
5. [plain-client-qos-status.md](./plain-client-qos-status.md)
6. [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)
7. [downlink-qos-status.md](./downlink-qos-status.md)
8. [downlink-qos-case-results.md](./downlink-qos-case-results.md)
9. [uplink-qos-design_cn.md](./uplink-qos-design_cn.md)
10. [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
11. [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)
12. [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md)
13. [downlink-qos-worker-validation_cn.md](./downlink-qos-worker-validation_cn.md)
14. [downlink-qos-implementation-plan_cn.md](./downlink-qos-implementation-plan_cn.md)
15. [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md)
16. [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md)
17. [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)
18. [uplink-qos-briefing.md](./uplink-qos-briefing.md)
19. [uplink-qos-case-analysis.md](./uplink-qos-case-analysis.md)
20. [uplink-qos-blind-spot-scenario.md](./uplink-qos-blind-spot-scenario.md)
21. [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md)
22. [uplink-qos-priority-roadmap.md](./uplink-qos-priority-roadmap.md)
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
- [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md) 用中文列出 JS uplink QoS 测试资产与 C++ client 对位覆盖的当前状态。
- [linux-client-architecture_cn.md](./linux-client-architecture_cn.md) 是当前 Linux client 架构主入口。
- [plain-client-qos-status.md](./plain-client-qos-status.md) 是当前 plain-client QoS 结果与签收范围的稳定摘要入口。
- [downlink-qos-status.md](./downlink-qos-status.md) 是当前 downlink 范围和结果入口摘要。
- 当前签收口径，以 [qos-status.md](./qos-status.md) 为总入口，并按各分支状态页下钻。
- [uplink-qos-boundaries.md](./uplink-qos-boundaries.md) 用来说明“底层 WebRTC 自动能力”和“本仓库 uplink QoS 策略能力”的职责边界。
- [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) 用来回答“上下行 QoS 现在分别测了哪些场景、在哪一层验证”。
- [uplink-qos-blind-spot-scenario.md](./uplink-qos-blind-spot-scenario.md) 用来汇总“高质量网络突入长时盲区再恢复”这一类极端转场场景的理论时序和实测结果。
- [uplink-qos-loopback-boundary-investigation.md](./uplink-qos-loopback-boundary-investigation.md) 用来记录 `BW2` 一类 loopback 边界 case 的专项排查结论、runner 特性和后续治理方向。
- full matrix 当前机器结果在 [generated/uplink-qos-matrix-report.json](./generated/uplink-qos-matrix-report.json)。
- targeted rerun 当前机器结果在 [generated/uplink-qos-matrix-report.targeted.json](./generated/uplink-qos-matrix-report.targeted.json)。
- Linux plain-client QoS 当前状态摘要在 [plain-client-qos-status.md](./plain-client-qos-status.md)。
- PlainTransport C++ client full matrix 当前机器结果在 [plain-client-qos-case-results.md](./plain-client-qos-case-results.md) 和 [generated/uplink-qos-cpp-client-matrix-report.json](./generated/uplink-qos-cpp-client-matrix-report.json)。
- PlainTransport C++ client targeted rerun 当前机器结果在 [generated/uplink-qos-cpp-client-case-results.targeted.md](./generated/uplink-qos-cpp-client-case-results.targeted.md) 和 [generated/uplink-qos-cpp-client-matrix-report.targeted.json](./generated/uplink-qos-cpp-client-matrix-report.targeted.json)。
- downlink 当前状态摘要在 [downlink-qos-status.md](./downlink-qos-status.md)。
- 每次报告生成的历史快照都归档在 [archive/uplink-qos-runs](./archive/uplink-qos-runs)。
- 已删除的 plain-client 历史迁移 / review / blocker / matrix 方案过程稿不再保留在 `docs/` 中；如需追历史判断，请直接看 git 历史。

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
- `cpp-client-matrix`: PlainTransport C++ client 弱网矩阵
- `node-harness`: Node QoS harness
- `browser-harness`: browser signaling / loopback
- `matrix`: browser loopback 弱网矩阵

## 5. 运维 / 缺陷专题

| 文档 | 用途 |
|---|---|
| [fix-subscriber-busyloop.md](./fix-subscriber-busyloop.md) | subscriber busy loop 问题的根因和修复过程。 |

## 6. 历史记录 / 归档

| 文档 | 用途 |
|---|---|
| [archive/CODE_REVIEW_2026-04-08.md](./archive/CODE_REVIEW_2026-04-08.md) | 2026-04-08 的静态代码审查记录。 |
| [archive/DEVELOPMENT_2026-04-08.md](./archive/DEVELOPMENT_2026-04-08.md) | 2026-04-08 的阶段性开发笔记。 |

## 7. 目录约定

后续建议按下面规则继续维护：

- 当前结论文档：放在“当前主文档”或“QoS 专项文档”
- 历史 review / 一次性记录：保留日期后缀，放在“历史记录 / 归档”
- 机器生成结果：优先用 `.json` / artifact 形式保留，不当作主结论文档
- 临时文件不要留在 `docs/`
