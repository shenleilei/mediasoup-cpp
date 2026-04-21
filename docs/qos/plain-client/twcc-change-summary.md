# Plain-Client TWCC / Transport Control 变更汇总

> 文档性质
>
> 这是一份当前有效的 reader-facing 文档。
> 它的目标不是回放历史 review 过程，
> 而是把 plain-client `TWCC / transport control / send-side BWE`
> 在当前仓库里的位置、范围和验证入口讲清楚。
>
> 如果只想先看当前 QoS 总口径，请先读：
> - [qos-status.md](../README.md)
> - [plain-client-qos-status.md](README.md)

## 1. 这份文档回答什么

这份文档回答四个问题：

1. 相对 `origin/codex/2026-04-20-functional-refactors`，plain-client TWCC 相关到底增加了什么
2. 这些变化在当前 QoS 体系里属于什么范围
3. 当前 threaded plain-client 主路径是如何把 transport control、TWCC feedback 和 send-side BWE 接起来的
4. 当前应看哪些稳定报告、accepted spec 和架构文档

## 2. TWCC 在当前 QoS 体系中的位置

在当前仓库里，TWCC 不是“独立于 QoS 之外的 transport 小专题”，而是 Linux `PlainTransport C++ client` uplink QoS 主口径的一部分。

当前 plain-client uplink QoS 的 accepted 范围，至少包括：

- transport-cc 协商信息进入 `plainPublish` 返回值
- threaded 主路径里的 transport-wide sequence 维护
- RTCP transport-cc feedback 进入 sender 侧处理链路
- send-side estimate 回写发送 pacing
- probe lifecycle 与相关白盒指标
- stable TWCC A/B 报告入口

accepted behavior 的正式口径见：

- [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

## 3. 比较基线与范围

### 3.1 比较基线

这份文档统一使用：

- `origin/codex/2026-04-20-functional-refactors`

作为 plain-client TWCC 相关增量的比较基线。

原因是本地 `codex/2026-04-20-functional-refactors` 已经提前包含同一批 TWCC 相关提交；如果直接对本地分支做 diff，会把真正的增量隐藏掉。

### 3.2 当前文档范围

这份文档覆盖的是：

- 服务端协商与观测面变化
- plain-client threaded runtime 的主路径变化
- transport execution 边界变化
- send-side BWE / probe 变化
- 默认回归入口、稳定报告和阅读路径

它不承担：

- 历史评审讨论逐条回放
- 每个实验 artifact 的逐项解释
- 新的非 accepted scope 承诺

## 4. 变更总览

| 层次 | 当前增量 | 结果 |
|---|---|---|
| 协商层 | `plainPublish` / RTP 参数显式带出 transport-cc 所需信息 | plain-client 不再依赖“私下发 RTP”的隐式假设 |
| runtime 层 | threaded plain-client 主路径收敛为 `PlainClientApp -> PlainClientThreaded -> NetworkThread` | 当前主路径有明确的线程职责和 transport owner 边界 |
| 发送执行层 | 引入 `SenderTransportController`、`TransportCcHelpers`、按包类区分的 pacing / queue / drop 规则 | 发送控制从“局部 send 逻辑”收敛成可观测的 transport-control 边界 |
| 估算层 | RTCP transport-cc feedback 接入 send-side BWE，estimate 回写 pacing | TWCC 不再只是 header extension 接线，而是进入真实发送决策链路 |
| 验证与报告层 | stable TWCC A/B 报告接入默认 QoS full regression | 当前仓库有固定、稳定的 TWCC 结果入口 |

## 5. 分层说明

### 5.1 服务端协商与观测面

plain-client TWCC 相关工作，不是只改客户端发送端。

服务端这轮补齐了 plain-client 需要的协商与诊断基础，包括：

- plain publish 返回值中的 transport-cc header extension 信息
- plain-client producer codec 参数中的 `rtcpFeedback` 保留
- router 在特定能力输入缺失时对 supported RTP capabilities 的回退
- 更丰富的 transport dump / stats 字段
- 可通过环境变量打开的 mediasoup-worker 日志级别与 tags

这层变化的意义是：

- plain-client 与服务端在 transport-cc 能力上真正对齐
- transport 层问题可以通过 dump / stats / worker logs 做定点排查

### 5.2 plain-client threaded runtime

当前 plain-client 的稳定主路径，应理解成下面这组职责分层：

- `PlainClientApp`
  - bootstrap
  - runtime mode select
  - threaded / legacy 切换
- `PlainClientThreaded`
  - control-thread 编排
  - signaling / QoS / command routing
- `NetworkThread`
  - UDP / RTP / RTCP / pacing / transport feedback owner
- `SourceWorker` / audio worker
  - 只负责输入、解码、编码与 access unit 生产

这意味着当前文档不应再把 plain-client 主路径描述成“main thread 自己推进全部媒体循环”的单线程模型。

### 5.3 发送执行边界

当前发送执行层的核心边界是：

- `client/SenderTransportController.h`
- `client/TransportCcHelpers.h`
- `client/UdpSendHelpers.h`

这层解决的是“什么时候发、按什么优先级发、阻塞时怎么保留或丢弃”的问题。

当前已进入主口径的语义包括：

- `Sent / WouldBlock / HardError` 的显式区分
- `Control / AudioRtp / VideoRetransmission / VideoMedia` 的包类区分
- fresh video 的 byte-budget pacing
- 音频 queue 的 deadline-aware 丢弃统计
- retransmission / fresh-video 的可观测 retain / drop 规则

这层是 send-side estimate 能真正参与发送控制的前提。

### 5.4 TWCC feedback 与 send-side BWE

当前 threaded 主路径里的 TWCC 主链路是：

```text
plainPublish transportCcExtId
  -> RTP header extension rewrite
  -> queued packet keeps stable transport-wide sequence across local retries
  -> sent packet tracker records only truly sent packets
  -> RTCP transport-cc feedback enters send-side BWE
  -> published transport estimate
  -> SenderTransportController effective pacing update
```

新增模块主要位于：

- `client/sendsidebwe/*`
- `client/ccutils/*`

当前签收口径里已经明确包含：

- feedback reference-time 扩展与主路径解析
- sender-side estimate 发布
- probe cluster start / send / early stop / finalize
- sender white-box transport 指标输出

### 5.5 回归入口与稳定报告

这轮工作不只补了单测与集成测试，也把 TWCC A/B 报告接进了默认 QoS 回归入口。

当前稳定入口包括：

- TWCC A/B 运行说明：
  [twcc-ab-test.md](twcc-ab-test.md)
- 最新 stable 报告：
  [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
- plain-client QoS 状态页：
  [plain-client-qos-status.md](README.md)
- accepted behavior：
  [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)

这意味着：

- TWCC A/B 不再只是 change folder 里的临时实验产物
- default full QoS regression 会刷新 stable TWCC 报告

## 6. 当前稳定证据入口

如果要验证当前 plain-client TWCC 主口径，优先看下面几类文档：

### 6.1 状态与架构

- [plain-client-qos-status.md](README.md)
- [linux-client-architecture_cn.md](architecture_cn.md)
- [architecture_cn.md](../../platform/architecture_cn.md)
- [full-architecture-flow_cn.md](../../platform/full-architecture-flow_cn.md)

### 6.2 accepted behavior 与回归入口

- [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)
- [../../../specs/current/test-entrypoints.md](../../../specs/current/test-entrypoints.md)
- [twcc-ab-test.md](twcc-ab-test.md)

### 6.3 当前结果

- [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
- [plain-client-qos-case-results.md](../../plain-client-qos-case-results.md)
- [generated/uplink-qos-cpp-client-matrix-report.json](../../generated/uplink-qos-cpp-client-matrix-report.json)

## 7. 当前明确非目标

当前仍然不应被表述成“已完成”的内容：

- LiveKit downlink `streamallocator` 的完整对象模型
- 新的 weight-aware multi-track transport fairness 模型
- 浏览器 uplink sender 路径重写
- room-level global bitrate budgeting

## 8. 推荐阅读顺序

如果问题是“TWCC 到底怎么进入 plain-client uplink QoS 主口径的”，建议按下面顺序阅读：

1. [qos-status.md](../README.md)
2. [plain-client-qos-status.md](README.md)
3. [plain-client-twcc-change-summary.md](twcc-change-summary.md)
4. [linux-client-architecture_cn.md](architecture_cn.md)
5. [../../../specs/current/plain-client-send-side-bwe.md](../../../specs/current/plain-client-send-side-bwe.md)
6. [twcc-ab-test.md](twcc-ab-test.md)
7. [generated/twcc-ab-report.md](../../generated/twcc-ab-report.md)
