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
| 做线上排障 | [troubleshooting_cn.md](./troubleshooting_cn.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |
| 做双机 PlainTransport 压测改造 | [plain-auto-return-pressure-design_cn.md](./plain-auto-return-pressure-design_cn.md) |
| 做 QoS 改动 | [qos-status.md](./qos-status.md) → [uplink-qos-design_cn.md](./uplink-qos-design_cn.md) → [downlink-qos-design_cn.md](./downlink-qos-design_cn.md) → [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md) → [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md) → [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md) → [run_qos_tests.sh](../scripts/run_qos_tests.sh) |
| 查 IPC/FlatBuffers 再生成规则 | [ipc-regeneration-runbook_cn.md](./ipc-regeneration-runbook_cn.md) |
| 查 worker 架构 | [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) |
| 查 QoS 详细 case 结果 | [downlink-qos-case-results.md](./downlink-qos-case-results.md) |
| 查 QoS 测试覆盖地图 | [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) |
| 做上线 / 运维准备 | [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) → [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) |

## 2. 当前主文档

| 文档 | 用途 |
|---|---|
| [full-architecture-flow_cn.md](./full-architecture-flow_cn.md) | 历史全链路架构流程图，包含旧的多节点/录制实现说明；不要将其作为当前运行事实来源。 |
| [architecture_cn.md](./architecture_cn.md) | 历史架构详解，包含旧的多节点/录制实现说明；不要将其作为当前运行事实来源。 |
| [dependencies_cn.md](./dependencies_cn.md) | 构建 / 运行 / 测试依赖总览，统一说明系统包、vendored 依赖、Node harness 依赖和 `setup.sh` / CMake 解析规则。 |
| [qos-status.md](./qos-status.md) | QoS 总状态摘要，统一给出 browser uplink / server QoS / downlink 的当前口径与结果入口。 |
| [uplink-downlink-api_cn.md](./uplink-downlink-api_cn.md) | 上下行接口总文档，统一说明客户端发什么、服务端回什么、服务端推什么以及正确顺序。 |
| [minimal-subscribe-access_cn.md](./minimal-subscribe-access_cn.md) | 最小订阅接入说明，只覆盖 `join -> device.load -> recvTransport -> existingProducers/newConsumer` 这条最短链。 |
| [room-client-sdk-access_cn.md](./room-client-sdk-access_cn.md) | 新房间客户端 SDK 接入说明，面向 `MediasoupRoomClient` / `TalkbackClient` 的业务接入。 |
| [audio-render-client-minimal-access_cn.md](./audio-render-client-minimal-access_cn.md) | 音频受限端客户端最小接入说明，按实际使用顺序覆盖受限端入会、普通端选择目标、claim/produce、close/release 和通知清理。 |
| [client-connectivity-failure-handling_cn.md](./client-connectivity-failure-handling_cn.md) | 客户端连通性故障处理总文档，按媒体断信令在、信令断媒体在、服务端 peer 已清理三类 case 整理检测信号、典型时间和处理动作。 |
| [client-signaling-media-recovery-access_cn.md](./client-signaling-media-recovery-access_cn.md) | 客户端信令与媒体恢复接入说明，覆盖 websocket 重连、`joinMode` 分支、10 秒媒体 grace window、`restartIce` 和重建 transport。 |
| [server-notify-categories_cn.md](./server-notify-categories_cn.md) | 服务端主动通知分类，按成员、订阅、QoS、统计、恢复说明客户端应处理什么。 |
| [client-reporting-protocol_cn.md](./client-reporting-protocol_cn.md) | 端上上报协议，按 join、transport、produce、consume、QoS/统计上报说明客户端要发什么。 |
| [sls-monitoring-plan_cn.md](./sls-monitoring-plan_cn.md) | 面向阿里云 SLS 的统一监控方案，合并说明现有日志能做什么、首页大盘怎么做、每个指标具体用哪条日志、怎么出报表和怎么配告警。 |
| [full-regression-test-results.md](./full-regression-test-results.md) | 最新一次 `scripts/run_all_tests.sh` 生成的仓库全量回归结果页，按选择分组记录逐任务 PASS/FAIL 和耗时。 |
| [nightly-full-regression.md](./nightly-full-regression.md) | nightly 全量回归自动化说明，包含 03:00 cron 安装、日志目录、邮件摘要和 Markdown 附件约定。 |
| [downlink-qos-status.md](./downlink-qos-status.md) | downlink QoS 当前状态摘要，说明当前范围、当前结果入口和后续边界。 |
| [troubleshooting_cn.md](./troubleshooting_cn.md) | 运行时排障手册；其中 Redis/录制章节属于历史实现说明。 |
| [DEVELOPMENT.md](./DEVELOPMENT.md) | 项目开发主文档，包含架构、线程模型、构建与测试入口。 |
| [MONITORING_RUNBOOK.md](./MONITORING_RUNBOOK.md) | 监控架构、部署、告警、验收与值班处理流程。 |
| [PRODUCTION_CHECKLIST.md](./PRODUCTION_CHECKLIST.md) | 上线前检查项。 |
| [mediasoup-worker-architecture-analysis_cn.md](./mediasoup-worker-architecture-analysis_cn.md) | 基于 `mediasoup-worker 3.14.6` 源码的架构与模块分析，从进程入口到 `Router/Transport/Producer/Consumer/BWE`。 |
| [REDIS_KEY_GUIDELINES.md](./REDIS_KEY_GUIDELINES.md) | 历史 Redis key 设计说明，当前 local-only 运行路径不再使用。 |
| [plain-auto-return-pressure-design_cn.md](./plain-auto-return-pressure-design_cn.md) | 双机内网 PlainTransport 压测 auto-return 设计，定义 subscriber socket 发送 `conn` 首包并由服务端学习回流 tuple 的方案。 |
| [ipc-regeneration-runbook_cn.md](./ipc-regeneration-runbook_cn.md) | IPC/FlatBuffers 再生成与回归规则，定义哪些改动必须触发“再生成 + 重编 + 单入口回归”。 |

## 3. QoS 专项文档

推荐阅读顺序：

1. [qos-status.md](./qos-status.md)
2. [downlink-qos-status.md](./downlink-qos-status.md)
3. [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md)
4. [downlink-qos-case-results.md](./downlink-qos-case-results.md)
5. [uplink-qos-design_cn.md](./uplink-qos-design_cn.md)
6. [downlink-qos-design_cn.md](./downlink-qos-design_cn.md)
7. [downlink-qos-v2-design_cn.md](./downlink-qos-v2-design_cn.md)
8. [downlink-qos-v2-implementation-plan_cn.md](./downlink-qos-v2-implementation-plan_cn.md)
9. [downlink-qos-worker-validation_cn.md](./downlink-qos-worker-validation_cn.md)
10. [downlink-qos-implementation-plan_cn.md](./downlink-qos-implementation-plan_cn.md)
11. [downlink-qos-v3-design_cn.md](./downlink-qos-v3-design_cn.md)
12. [downlink-qos-v3-implementation-plan_cn.md](./downlink-qos-v3-implementation-plan_cn.md)
13. [uplink-qos-boundaries.md](./uplink-qos-boundaries.md)
14. [qos-test-coverage_cn.md](./qos-test-coverage_cn.md)

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
- [qos-test-coverage_cn.md](./qos-test-coverage_cn.md) 用来回答“上下行 QoS 现在分别测了哪些场景、在哪一层验证”。
- 根目录原生 WebRTC QoS plain push/play client 及其 P2/P3 报告已经移除；当前 QoS 回归以 browser/server 路径为主。
- downlink 当前状态摘要在 [downlink-qos-status.md](./downlink-qos-status.md)。
- 一次性过程稿、历史 review、商业化排期和旧归档快照不再保留在 `docs/` 中；如需追历史判断，请直接看 git 历史。

## 4. QoS 测试入口

统一脚本：

- [run_qos_tests.sh](../scripts/run_qos_tests.sh)

常用命令：

```bash
cd /root/workspace/mediasoup-cpp
./scripts/run_qos_tests.sh
./scripts/run_qos_tests.sh --skip-browser
./scripts/run_qos_tests.sh client-js cpp-unit
./scripts/run_qos_tests.sh --list
```

压测约定：

- 如果没有额外说明，默认压测入口是
  `node tests/qos_harness/multi_process_pressure.mjs`。

分组说明：

- `client-js`: 客户端 QoS JS 单测
- `cpp-unit`: 服务端 QoS 单测
- `cpp-integration`: 服务端 QoS 集成测试
- `cpp-accuracy`: QoS 统计精度测试
- `node-harness`: Node QoS harness
- `browser-harness`: browser signaling / downlink harness
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
