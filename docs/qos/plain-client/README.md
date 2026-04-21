# PlainTransport C++ Client QoS 当前状态

> 文档性质
>
> 这是 Linux `PlainTransport C++ client` uplink QoS 的稳定摘要入口。
> 它定义当前签收范围、结果入口和主文档阅读顺序，
> 不展开历史 review 过程。
>
> 在当前文档体系里，
> `TWCC / transport control / send-side BWE`
> 属于 plain-client uplink QoS 的主口径，不是额外附录。

## 1. 当前结论

截至 `2026-04-21`，仓库内 Linux `PlainTransport C++ client` 的 `uplink QoS` 已进入稳定可回归状态。

当前稳定口径包括：

- `ClientQos*` 纯逻辑对位测试资产
- `cpp-client-harness` 对 publish / stale-seq / policy-update / override / clear 主链路的覆盖
- `cpp-client-matrix` 的统一结果入口
- 当前机器 full matrix：`43 / 43 PASS`
- 当前 threaded plain-client 主路径：
  - `NetworkThread`
  - `SenderTransportController`
  - transport-cc feedback handling
  - TWCC send-side estimate
  - probe lifecycle
- 默认 full QoS 回归会刷新 stable TWCC A/B 报告

## 2. 当前签收范围

这份状态页里的“已完成”口径，当前指：

- JS uplink QoS 已有主测试资产，plain-client 已具备对位入口
- Linux `PlainTransport C++ client` 已具备可追溯的 full matrix 结果入口
- Linux `PlainTransport C++ client` 的 threaded 主路径，已将发送侧 transport control、TWCC feedback、send-side BWE、probe 生命周期接入当前 accepted QoS 范围
- TWCC A/B 评估已经收敛成 stable 文档入口，而不是只留在 change artifacts 里

明确不包括：

- downlink browser matrix 的 C++ client 对位
- `dynacast`
- room-level global bitrate budgeting
- 声称与 LiveKit 下行 allocator 对象模型“完全等价”

## 3. TWCC 在当前 QoS 口径中的位置

当前 plain-client uplink QoS 的主口径，可以拆成四层：

1. 协商层
   - `plainPublish` 返回 transport-cc 所需的 header extension 信息
2. 发送执行层
   - `NetworkThread + SenderTransportController` 负责 RTP/RTCP、pacing、重传、transport-wide sequence 与队列管理
3. 估算与反馈层
   - RTCP transport-cc feedback 进入 send-side BWE 主路径
   - published transport estimate 会回写发送 pacing
4. 结果与验证层
   - full matrix 证明主 QoS 路径不退化
   - stable TWCC A/B 报告用于观察 transport-control / estimate 的白盒效果

当前 accepted behavior 的正式定义见：

- [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

## 4. 结果与证据入口

### 4.1 full matrix

- 人类可读结果：
  [plain-client-qos-case-results.md](../../plain-client-qos-case-results.md)
- 结构化 artifact：
  [generated/uplink-qos-cpp-client-matrix-report.json](../../generated/uplink-qos-cpp-client-matrix-report.json)

### 4.2 TWCC / transport control

- 当前稳定摘要：
  [plain-client-twcc-change-summary.md](twcc-change-summary.md)
- stable TWCC A/B 报告：
  [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
- TWCC A/B 运行说明：
  [twcc-ab-test.md](twcc-ab-test.md)
- accepted behavior：
  [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

### 4.3 targeted 结果

- 人类可读结果：
  [generated/uplink-qos-cpp-client-case-results.targeted.md](../../generated/uplink-qos-cpp-client-case-results.targeted.md)
- 结构化 artifact：
  [generated/uplink-qos-cpp-client-matrix-report.targeted.json](../../generated/uplink-qos-cpp-client-matrix-report.targeted.json)

### 4.4 对位范围

- 对位覆盖清单：
  [plain-client-qos-parity-checklist.md](parity-checklist.md)

## 5. 架构入口

如果要理解 Linux client 如何把 QoS 与发送主路径接起来，继续看：

- [linux-client-architecture_cn.md](architecture_cn.md)
- [architecture_cn.md](../../platform/architecture_cn.md)
- [full-architecture-flow_cn.md](../../platform/full-architecture-flow_cn.md)
- [plain-client-twcc-change-summary.md](twcc-change-summary.md)

## 6. 阅读顺序建议

如果问题集中在 plain-client uplink QoS，建议按下面顺序阅读：

1. [plain-client-qos-status.md](README.md)
2. [plain-client-twcc-change-summary.md](twcc-change-summary.md)
3. [linux-client-architecture_cn.md](architecture_cn.md)
4. [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)
5. [twcc-ab-test.md](twcc-ab-test.md)
6. [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
7. [plain-client-qos-case-results.md](../../plain-client-qos-case-results.md)

## 7. 维护规则

后续如果 plain-client QoS 能力继续演进，优先更新：

1. 这份状态页
2. [plain-client-twcc-change-summary.md](twcc-change-summary.md)
3. [linux-client-architecture_cn.md](architecture_cn.md)
4. [plain-client-qos-parity-checklist.md](parity-checklist.md)
5. `docs/generated/` 下的 stable 结果入口
