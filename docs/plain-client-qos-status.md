# PlainTransport C++ Client QoS 当前状态

> 文档性质
>
> 这是 Linux `PlainTransport C++ client` QoS 的稳定摘要入口。
> 它只回答当前状态、签收范围和应该看哪些结果，不展开历史设计过程。

## 1. 当前结论

截至 `2026-04-21`，仓库内 Linux `PlainTransport C++ client` 的 `uplink QoS` 主链路已经进入可回归状态，而且 threaded plain-client 主路径已经把发送侧 transport control、TWCC send-side estimate 和默认 A/B 回归入口接进稳定口径。

当前可以稳定使用的口径是：

- `ClientQos*` 纯逻辑对位测试已具备
- `cpp-client-harness` 已覆盖 publish / stale-seq / policy-update / override / clear 主链路
- `cpp-client-matrix` 已进入统一测试与报告体系
- 当前机器的 full matrix 结果为 `43 / 43 PASS`
- 当前 threaded plain-client 主路径使用 `NetworkThread + SenderTransportController + send-side BWE`
- 默认全量 QoS 回归会刷新 stable TWCC A/B 报告

## 2. 当前签收范围

这份状态页里的“已完成”口径，当前只指：

- JS uplink QoS 已有主测试资产
- 在 Linux `PlainTransport C++ client` 上已具备对位入口、可追溯结果和 TWCC A/B 稳定报告入口

明确不包括：

- downlink browser matrix 的 C++ client 对位
- `dynacast`
- room-level global bitrate budgeting

## 3. 结果入口

### 3.1 当前摘要

- 对位覆盖清单：
  [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)

### 3.2 full 结果

- 人类可读结果：
  [plain-client-qos-case-results.md](./plain-client-qos-case-results.md)
- 结构化 artifact：
  [generated/uplink-qos-cpp-client-matrix-report.json](./generated/uplink-qos-cpp-client-matrix-report.json)

### 3.3 TWCC A/B 结果

- 最新稳定摘要：
  [generated/twcc-ab-report.md](./generated/twcc-ab-report.md)
- 说明文档：
  [twcc-ab-test.md](./twcc-ab-test.md)
- 分支 TWCC 变更汇总：
  [plain-client-twcc-change-summary.md](./plain-client-twcc-change-summary.md)

### 3.4 targeted 结果

- 人类可读结果：
  [generated/uplink-qos-cpp-client-case-results.targeted.md](./generated/uplink-qos-cpp-client-case-results.targeted.md)
- 结构化 artifact：
  [generated/uplink-qos-cpp-client-matrix-report.targeted.json](./generated/uplink-qos-cpp-client-matrix-report.targeted.json)

## 4. 架构入口

如果要理解 Linux client 本身怎么工作，直接看：

- [linux-client-architecture_cn.md](./linux-client-architecture_cn.md)
- [architecture_cn.md](./architecture_cn.md)
- [full-architecture-flow_cn.md](./full-architecture-flow_cn.md)
- [plain-client-twcc-change-summary.md](./plain-client-twcc-change-summary.md)

## 5. 维护规则

后续如果 plain-client QoS 能力继续演进，优先更新：

1. 这份状态页
2. [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)
3. [linux-client-architecture_cn.md](./linux-client-architecture_cn.md)
4. [plain-client-twcc-change-summary.md](./plain-client-twcc-change-summary.md)
5. `docs/generated/` 下的结构化结果入口
