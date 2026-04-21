# 文档导航

`docs/` 现在按**功能域**组织，而不是靠一长串相似文件名平铺在根目录。

核心规则只有两条：

1. 先按功能找目录，再在目录内找具体文档。
2. 根目录只保留少量“当前结果入口”和总索引，不再堆放过程文档。

## 0. Codex 接续

如果是继续上一次的本地协作，先读：

1. [docs/.codex](.codex)
2. [docs/README.md](./README.md)
3. 如果问题集中在 QoS 边界或 loopback，再看：
   - [qos/uplink/loopback-boundary-investigation.md](./qos/uplink/loopback-boundary-investigation.md)
   - [.kiro/skills/qos-boundary-analysis.md](../.kiro/skills/qos-boundary-analysis.md)

## 1. 功能域总览

### 1.1 Platform

- 入口：[platform/README.md](./platform/README.md)
- 覆盖：
  - 开发入口与线程模型
  - 系统架构
  - 构建依赖
  - worker 架构分析
  - Redis key 规范

### 1.2 QoS

- 总入口：[qos/README.md](./qos/README.md)
- 子域：
  - uplink：[qos/uplink/README.md](./qos/uplink/README.md)
  - plain-client：[qos/plain-client/README.md](./qos/plain-client/README.md)
  - downlink：[qos/downlink/README.md](./qos/downlink/README.md)
  - shared reference：[qos/shared/test-coverage_cn.md](./qos/shared/test-coverage_cn.md)

### 1.3 Operations

- 入口：[operations/README.md](./operations/README.md)
- 覆盖：
  - 排障
  - 监控 / 告警
  - 上线检查
  - nightly 回归

### 1.4 Planning

- 入口：[planning/README.md](./planning/README.md)
- 覆盖：
  - 中长期路线图
  - 商业化计划
  - 当前阶段执行计划

### 1.5 AI Coding Rules

- 入口：[aicoding/README.md](./aicoding/README.md)

### 1.6 Generated And Archive

- 机器生成结果：[generated/README.md](./generated/README.md)
- 历史归档：[archive/README.md](./archive/README.md)

## 2. 当前结果入口

这些文件虽然还保留在 `docs/` 根目录，但它们是**当前结果入口**，不是长期设计文档：

- [full-regression-test-results.md](./full-regression-test-results.md)
- [uplink-qos-case-results.md](./uplink-qos-case-results.md)
- [plain-client-qos-case-results.md](./plain-client-qos-case-results.md)
- [downlink-qos-test-results-summary.md](./downlink-qos-test-results-summary.md)
- [downlink-qos-case-results.md](./downlink-qos-case-results.md)

原因：

- 这些文件由脚本直接刷新，当前先保留稳定输出路径
- 与之配套的解释性和设计性文档已经下沉到对应功能域目录

## 3. 按任务找入口

| 任务 | 建议起点 |
|---|---|
| 新接手平台开发 | [platform/README.md](./platform/README.md) |
| 查系统架构 | [platform/architecture_cn.md](./platform/architecture_cn.md) |
| 查环境依赖 | [platform/dependencies_cn.md](./platform/dependencies_cn.md) |
| 做 QoS 改动 | [qos/README.md](./qos/README.md) |
| 做 uplink QoS 改动 | [qos/uplink/README.md](./qos/uplink/README.md) |
| 做 plain-client/TWCC 改动 | [qos/plain-client/README.md](./qos/plain-client/README.md) |
| 做 downlink QoS 改动 | [qos/downlink/README.md](./qos/downlink/README.md) |
| 查当前回归结果 | 根目录结果入口 + [generated/README.md](./generated/README.md) |
| 做线上排障 | [operations/README.md](./operations/README.md) |
| 做上线准备 | [operations/PRODUCTION_CHECKLIST.md](./operations/PRODUCTION_CHECKLIST.md) |
| 看路线图 / 商业化计划 | [planning/README.md](./planning/README.md) |
| 查历史 review / 旧结论 | [archive/README.md](./archive/README.md) |

## 4. 维护规则

后续新增或重写文档时，按下面规则放置：

- Platform 相关：
  放 `docs/platform/`
- QoS 相关：
  先判断属于 `uplink / plain-client / downlink / shared`
  再放入 `docs/qos/<domain>/`
- Operations 相关：
  放 `docs/operations/`
- Planning 相关：
  放 `docs/planning/`
- 机器生成结果：
  放 `docs/generated/`
- 历史 review / 一次性调查 / 已过期过程稿：
  放 `docs/archive/` 或 `changes/`

额外约束：

- 不要再把 hand-written 过程文档直接堆回 `docs/` 根目录
- 同一功能域优先使用目录表达分类，不再靠越来越长的相似文件名前缀硬分辨
- 一个功能域的当前入口优先使用该目录下的 `README.md`
