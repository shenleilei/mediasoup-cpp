# 文档导航

这个目录同时包含：

- 当前仍在使用的主文档
- QoS 专项文档
- 运维 / 上线文档
- 历史 review / 开发记录
- 机器生成的辅助结果

为了避免把“历史快照”和“当前结论”混在一起，建议按下面顺序阅读。

## 1. 按角色阅读

| 角色 | 建议起点 |
|---|---|
| 新接手开发 | [DEVELOPMENT.md](/root/mediasoup-cpp/docs/DEVELOPMENT.md) |
| 做 QoS 改动 | [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md) → [review_qos.md](/root/mediasoup-cpp/docs/review_qos.md) → [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh) |
| 查 QoS 详细 case 结果 | [uplink-qos-case-analysis.md](/root/mediasoup-cpp/docs/uplink-qos-case-analysis.md) |
| 做上线 / 运维准备 | [PRODUCTION_CHECKLIST.md](/root/mediasoup-cpp/docs/PRODUCTION_CHECKLIST.md) → [MONITORING_RUNBOOK.md](/root/mediasoup-cpp/docs/MONITORING_RUNBOOK.md) |
| 查历史背景 | [archive/CODE_REVIEW_2026-04-08.md](/root/mediasoup-cpp/docs/archive/CODE_REVIEW_2026-04-08.md) / [archive/DEVELOPMENT_2026-04-08.md](/root/mediasoup-cpp/docs/archive/DEVELOPMENT_2026-04-08.md) |

## 2. 当前主文档

| 文档 | 用途 |
|---|---|
| [DEVELOPMENT.md](/root/mediasoup-cpp/docs/DEVELOPMENT.md) | 项目开发主文档，包含架构、线程模型、构建与测试入口。 |
| [MONITORING_RUNBOOK.md](/root/mediasoup-cpp/docs/MONITORING_RUNBOOK.md) | 监控、告警与值班处理流程。 |
| [PRODUCTION_CHECKLIST.md](/root/mediasoup-cpp/docs/PRODUCTION_CHECKLIST.md) | 上线前检查项。 |
| [REDIS_KEY_GUIDELINES.md](/root/mediasoup-cpp/docs/REDIS_KEY_GUIDELINES.md) | Redis key 设计和约束。 |

## 3. QoS 专项文档

推荐阅读顺序：

1. [uplink-qos-briefing.md](/root/mediasoup-cpp/docs/uplink-qos-briefing.md)
2. [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md)
3. [uplink-qos-test-results-summary.md](/root/mediasoup-cpp/docs/uplink-qos-test-results-summary.md)
4. [uplink-qos-case-results.md](/root/mediasoup-cpp/docs/uplink-qos-case-results.md)
5. [uplink-qos-priority-roadmap.md](/root/mediasoup-cpp/docs/uplink-qos-priority-roadmap.md)
6. [uplink-qos-case-analysis.md](/root/mediasoup-cpp/docs/uplink-qos-case-analysis.md)
7. [uplink-qos-test-execution-checklist.md](/root/mediasoup-cpp/docs/uplink-qos-test-execution-checklist.md)
8. [review_qos.md](/root/mediasoup-cpp/docs/review_qos.md)

说明：

- [review_qos.md](/root/mediasoup-cpp/docs/review_qos.md) 不是“当前结论总表”，而是“原始 review 发现 + 后续有效项判定”的混合文档。
- 当前签收口径，以 [uplink-qos-final-report.md](/root/mediasoup-cpp/docs/uplink-qos-final-report.md) 和 [uplink-qos-test-results-summary.md](/root/mediasoup-cpp/docs/uplink-qos-test-results-summary.md) 为准。
- 当前保留的 targeted rerun 机器结果在 [generated/uplink-qos-matrix-report.json](/root/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.json)。

## 4. QoS 测试入口

统一脚本：

- [run_qos_tests.sh](/root/mediasoup-cpp/scripts/run_qos_tests.sh)

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
- `node-harness`: Node QoS harness
- `browser-harness`: browser signaling / loopback
- `matrix`: browser loopback 弱网矩阵

## 5. 运维 / 缺陷专题

| 文档 | 用途 |
|---|---|
| [fix-subscriber-busyloop.md](/root/mediasoup-cpp/docs/fix-subscriber-busyloop.md) | subscriber busy loop 问题的根因和修复过程。 |

## 6. 历史记录 / 归档

| 文档 | 用途 |
|---|---|
| [archive/CODE_REVIEW_2026-04-08.md](/root/mediasoup-cpp/docs/archive/CODE_REVIEW_2026-04-08.md) | 2026-04-08 的静态代码审查记录。 |
| [archive/DEVELOPMENT_2026-04-08.md](/root/mediasoup-cpp/docs/archive/DEVELOPMENT_2026-04-08.md) | 2026-04-08 的阶段性开发笔记。 |

## 7. 目录约定

后续建议按下面规则继续维护：

- 当前结论文档：放在“当前主文档”或“QoS 专项文档”
- 历史 review / 一次性记录：保留日期后缀，放在“历史记录 / 归档”
- 机器生成结果：优先用 `.json` / artifact 形式保留，不当作主结论文档
- 临时文件不要留在 `docs/`
