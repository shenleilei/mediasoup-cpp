# QoS 当前状态总览

> 文档性质
>
> 这是仓库内 QoS 的总入口。
> 它定义当前签收口径、当前结果入口和各分支文档的阅读顺序，
> 不承担历史评审过程或逐 case 复盘。
>
> 对 Linux `PlainTransport C++ client` 而言，
> `TWCC / transport control / send-side BWE`
> 是当前 plain-client uplink QoS 主口径的一部分，不是独立于 QoS 体系之外的附录主题。

## 1. 当前结论

当前仓库的 QoS 文档体系分成三条主线：

1. browser uplink QoS
2. Linux `PlainTransport C++ client` uplink QoS
3. downlink QoS

当前稳定口径如下：

- browser uplink 主 gate：`43 / 43 PASS`
- Linux plain-client full matrix：`43 / 43 PASS`
- Linux plain-client 当前签收范围包含：
  - threaded 主路径 `NetworkThread + SenderTransportController`
  - transport-cc 协商与反馈主路径
  - TWCC send-side estimate 与 probe 生命周期
  - stable TWCC A/B 报告入口
- 默认 full QoS 回归会刷新 stable TWCC A/B 报告
- downlink 当前签收范围：subscriber receive control + zero-demand publisher pause/resume coordination
- `dynacast` 与 room-level global bitrate budgeting 仍是后续能力，不计入当前已完成口径

## 2. 按主题查看

### 2.1 browser uplink QoS

- 当前总报告：
  [uplink-qos-final-report.md](uplink/README.md)
- 当前摘要：
  [uplink-qos-test-results-summary.md](uplink/test-results-summary.md)
- 当前逐 case：
  [uplink-qos-case-results.md](../uplink-qos-case-results.md)

### 2.2 Linux plain-client uplink QoS

当前 plain-client uplink QoS 的主口径，不只包括 matrix 结果，还包括当前 threaded 发送主路径里的 transport control / TWCC / send-side BWE。

当前应按下面的层次理解：

- 状态入口：
  [plain-client-qos-status.md](plain-client/README.md)
- TWCC / transport control 汇总：
  [plain-client-twcc-change-summary.md](plain-client/twcc-change-summary.md)
- Linux client 当前架构：
  [linux-client-architecture_cn.md](plain-client/architecture_cn.md)
- accepted behavior：
  [../../../specs/current/plain-client-send-side-bwe.md](../../specs/current/plain-client-send-side-bwe.md)
- stable TWCC A/B 报告：
  [generated/twcc-ab-report.md](../generated/twcc-ab-report.md)
- TWCC A/B 运行说明：
  [twcc-ab-test.md](plain-client/twcc-ab-test.md)
- full matrix：
  [plain-client-qos-case-results.md](../plain-client-qos-case-results.md)
- 对位清单：
  [plain-client-qos-parity-checklist.md](plain-client/parity-checklist.md)

### 2.3 downlink QoS

- 当前状态摘要：
  [downlink-qos-status.md](downlink/README.md)
- 当前机器摘要：
  [downlink-qos-test-results-summary.md](../downlink-qos-test-results-summary.md)
- 当前机器逐项结果：
  [downlink-qos-case-results.md](../downlink-qos-case-results.md)
- 测试覆盖地图：
  [qos-test-coverage_cn.md](shared/test-coverage_cn.md)

## 3. 推荐阅读顺序

如果问题集中在 plain-client uplink QoS，建议按下面顺序阅读：

1. [qos-status.md](README.md)
2. [plain-client-qos-status.md](plain-client/README.md)
3. [plain-client-twcc-change-summary.md](plain-client/twcc-change-summary.md)
4. [linux-client-architecture_cn.md](plain-client/architecture_cn.md)
5. [../../../specs/current/plain-client-send-side-bwe.md](../../specs/current/plain-client-send-side-bwe.md)
6. [twcc-ab-test.md](plain-client/twcc-ab-test.md)
7. [generated/twcc-ab-report.md](../generated/twcc-ab-report.md)
8. [plain-client-qos-case-results.md](../plain-client-qos-case-results.md)

## 4. 当前范围边界

### 4.1 已进入“当前主口径”的内容

- browser uplink 主测试集
- Linux plain-client uplink 对位与 full matrix
- Linux plain-client threaded 主路径中的 transport control / TWCC feedback / send-side BWE / stable TWCC A/B 报告
- downlink 现有 subscriber-side / publisher-supply 协调能力

### 4.2 不在“当前已完成”口径里的内容

- `dynacast`
- room-level global bitrate budgeting
- 完整的 LiveKit downlink `streamallocator` 对象模型
- 非 uplink 范围外的 browser UI / demo 层签收

## 5. 维护规则

只要 QoS 能力继续变化，优先更新：

1. 这份总状态页
2. 对应分支状态页
   - [plain-client-qos-status.md](plain-client/README.md)
   - [downlink-qos-status.md](downlink/README.md)
3. 对应主结果页或稳定报告页
4. `docs/generated/` 下的稳定结果入口
