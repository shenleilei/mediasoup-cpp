# QoS 当前状态总览

> 文档性质
>
> 这是仓库内 QoS 的总摘要入口。
> 它只回答当前范围、当前结果入口和各分支文档应该怎么读，
> 不承担历史设计过程或逐 case 细节展开。

## 1. 当前结论

当前仓库的 QoS 文档体系已经拆成三条主线：

1. browser uplink QoS
2. Linux `PlainTransport C++ client` uplink QoS
3. downlink QoS

当前稳定口径：

- browser uplink 主 gate：`43 / 43 PASS`
- Linux plain-client matrix：`43 / 43 PASS`
- downlink 当前范围：subscriber receive control + zero-demand publisher pause/resume coordination
- `dynacast` 与 room-level global bitrate budgeting 仍是后续能力，不计入当前已完成口径

## 2. 按主题查看

### 2.1 browser uplink

- 最终结论：
  [uplink-qos-final-report.md](./uplink-qos-final-report.md)
- 结果汇总：
  [uplink-qos-test-results-summary.md](./uplink-qos-test-results-summary.md)
- 逐 case：
  [uplink-qos-case-results.md](./uplink-qos-case-results.md)

### 2.2 Linux plain-client

- 当前状态摘要：
  [plain-client-qos-status.md](./plain-client-qos-status.md)
- Linux client 架构：
  [linux-client-architecture_cn.md](./linux-client-architecture_cn.md)
- 对位清单：
  [plain-client-qos-parity-checklist.md](./plain-client-qos-parity-checklist.md)
- full matrix：
  [plain-client-qos-case-results.md](./plain-client-qos-case-results.md)

### 2.3 downlink

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
- Linux plain-client uplink 对位与 matrix
- downlink 现有 subscriber-side / publisher-supply 协调能力

### 3.2 不在“当前已完成”口径里的内容

- `dynacast`
- room-level global bitrate budgeting
- 非 uplink 范围外的 browser UI / demo 层签收

## 4. 维护规则

后续只要 QoS 能力继续变化，优先更新：

1. 这份总状态页
2. 对应分支状态页
   - [plain-client-qos-status.md](./plain-client-qos-status.md)
   - [downlink-qos-status.md](./downlink-qos-status.md)
3. 对应主结果页
4. `docs/generated/README.md`
