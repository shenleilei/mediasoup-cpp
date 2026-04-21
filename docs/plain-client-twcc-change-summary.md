# Plain-Client TWCC 变更汇总

## 1. 文档目的

这份文档把当前分支里和 plain-client `TWCC / send-side transport control / send-side BWE` 相关的改动，统一整理成一个读者入口。

它回答的是：

- 相对 `codex/2026-04-20-functional-refactors`，TWCC 相关到底改了什么
- 这些改动落在哪些模块
- 服务端协商、客户端发送链路、测试与报告入口各自怎么变化
- 当前签收范围和明确非目标是什么

## 2. 比较基线说明

本地 `codex/2026-04-20-functional-refactors` 已经提前包含同一批 TWCC 相关提交，所以如果直接对本地分支做 diff，基本只会看到最后一层 A/B 报告接线。

为了把 TWCC 这批变化完整讲清，这份文档统一以：

- `origin/codex/2026-04-20-functional-refactors`

作为比较基线来整理当前分支的完整 TWCC 增量。

## 3. 一句话结论

当前分支把 Linux plain-client 的发送侧，从“只有本地 pacing 和 RTCP/RTP 基础处理”的路径，推进到了“支持 transport-cc 协商、稳定 transport-wide sequence、TWCC feedback 驱动的 send-side estimate、padding probe 生命周期，以及默认可回归的 TWCC A/B 报告”的路径。

## 4. 变更分层

### 4.1 服务端协商与观测面

服务端这轮不是只改客户端；它先补了 plain-client 需要的 transport-cc 协商与观测能力。

主要变化：

- `src/RoomServiceMedia.cpp`
  - plain publish 返回值里增加了 `videoTransportCcExtId`、`audioTransportCcExtId`
  - video / audio RTP 参数显式带上 transport-cc header extension
  - plain-client producer 的 codec 参数里保留 `rtcpFeedback`
- `src/Router.cpp`
  - 让 Router 在没有自定义能力输入时仍能回退到 supported RTP capabilities，避免 plain-client 协商链路缺口
- `src/Transport.h`, `src/Transport.cpp`
  - 增加 transport dump / richer stats 字段，补强对 transport 层行为的诊断
- `src/MainBootstrap.cpp`
  - 支持通过环境变量打开 mediasoup-worker 日志级别和 tag，方便 TWCC / transport-cc 现场排查

这层变化的意义是：plain-client 不再只是“自己私下发 RTP”，而是和服务端在 header extension / feedback 能力上真正对齐。

### 4.2 plain-client 发送执行层

这一层的核心是把发送路径从“直接 send + 局部队列”收敛为明确的 transport-control 边界。

主要文件：

- `client/SenderTransportController.h`
- `client/UdpSendHelpers.h`
- `client/TransportCcHelpers.h`

主要语义：

- 显式区分 `Sent / WouldBlock / HardError`
- 显式区分 `Control / AudioRtp / VideoRetransmission / VideoMedia`
- fresh video 不再只靠固定 burst；改成 byte-budget pacing
- 音频引入 deadline-aware queue，因此会暴露 `audioDeadlineDrops`
- 重传与 fresh video 的保留、丢弃、计数规则变成可观测行为

这层变化是后续 TWCC estimate 能真正接入发送执行面的前提。

### 4.3 `NetworkThread` 主路径接入 TWCC

客户端主路径的关键改动集中在：

- `client/NetworkThread.h`
- `client/PlainClientThreaded.cpp`
- `client/ThreadTypes.h`
- `client/ThreadedControlHelpers.h`

当前 threaded plain-client 主路径里：

- `NetworkThread` 是唯一 UDP / RTP / RTCP / pacing owner
- source worker 只产出 encoded access unit，不直接持有网络发送语义
- audio 也通过队列进入 network-thread 发送链路
- `QosController` 产生的动作，一部分变成 source-worker 编码命令，一部分变成 network-thread 的 transport hint

和 TWCC 直接相关的行为包括：

- 为音频 / 视频 / 重传 RTP 重写 transport-cc sequence
- 本地 `WouldBlock` 重试时复用同一个 transport-wide sequence
- 只有真正 `Sent` 的包才记录进 send-side BWE tracker
- 解析 RTCP transport-cc feedback 并发布 transport estimate
- 将 published transport estimate 回写到 `SenderTransportController`
- 在 threaded trace / stats snapshot 里输出白盒 transport 指标

### 4.4 send-side BWE 与 probe 生命周期

客户端新增了完整的 send-side BWE 模块边界：

- `client/sendsidebwe/*`
- `client/ccutils/*`

主要能力：

- packet tracking
- TWCC feedback 时间序和 reference-time wrap 处理
- packet groups / congestion detector
- probe regulator / prober / observer
- probe start -> send -> goal reached early stop -> finalize

这批代码早期经历过一个本地 delay-based TWCC BWE 过渡版本，但当前签收口径已经收敛到：

- LiveKit-aligned send-side BWE 结构
- sender-side equivalent 的 probe goal 计算
- probe RTP 与普通重传/主媒体 bookkeeping 的语义隔离

当前 accepted behavior 见：

- [../specs/current/plain-client-send-side-bwe.md](../specs/current/plain-client-send-side-bwe.md)

### 4.5 threaded runtime / plain-client 启动面

`client/PlainClientApp.*` 不再只是一个“main thread media loop”的包装。

它现在负责：

- 解析 `PLAIN_CLIENT_THREADED`
- 解析 `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER`
- 解析 `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE`
- 在 threaded / legacy runtime 之间做选择
- 把 plain publish 返回的 `transportCcExtId`、track metadata、QoS state 注入 threaded runtime

因此 plain-client 当前稳定主路径应该理解成：

- `PlainClientApp` 负责 bootstrap / mode select
- `PlainClientThreaded` 负责 control-thread 编排
- `NetworkThread` 负责 transport owner 语义
- `SourceWorker` / audio worker 负责编码或输入侧推进

而不是旧文档里那种“main thread 跑完所有媒体逻辑”的单路径描述。

## 5. 测试、报告与默认入口变化

这轮 TWCC 相关改动不仅补了单测，还把报告链路接进了默认 QoS 全量入口。

### 5.1 单测与集成测试

主要新增/增强：

- `tests/test_thread_model.cpp`
  - `SenderTransportController`
  - `TransportCcHelpers`
  - send-side BWE / probe 相关单测
- `tests/test_thread_integration.cpp`
  - threaded plain-client 主路径
  - real worker transport-cc feedback
  - probe 行为与 transport estimate 行为
- `tests/TransportCcTestHelpers.h`
  - transport-cc / TWCC 定向测试辅助

### 5.2 harness 与 trace 扩展

主要文件：

- `tests/qos_harness/cpp_client_runner.mjs`
- `tests/qos_harness/run_cpp_client_harness.mjs`

新增白盒字段包括：

- `senderUsageBps`
- `transportEstimateBps`
- `effectivePacingBps`
- `feedbackReports`
- `probePackets`
- `probeClusterStarts / Completes / EarlyStops`
- `wouldBlockTotal`
- `queuedVideoRetentions`
- `audioDeadlineDrops`
- `retransmissionDrops / Sent`

### 5.3 TWCC A/B 报告

主要文件：

- `tests/qos_harness/run_twcc_ab_eval.mjs`
- `tests/qos_harness/render_twcc_ab_report.mjs`
- `docs/twcc-ab-test.md`
- `docs/generated/twcc-ab-report.md`

当前仓库已经把 plain-client TWCC A/B 评估收敛成固定 3 组：

- `G0`: 旧发送路径
- `G1`: 新 transport controller，关闭 transport estimate
- `G2`: 新 transport controller，打开 transport estimate

稳定入口：

- 说明文档：[twcc-ab-test.md](./twcc-ab-test.md)
- 最新稳定报告：[generated/twcc-ab-report.md](./generated/twcc-ab-report.md)

### 5.4 默认全量 QoS 入口

脚本层面，TWCC A/B 不再是独立手工步骤。

主要变化：

- `scripts/run_qos_tests.sh`
  - 增加 `cpp-client-ab` 分组
- `scripts/run_all_tests.sh`
  - full QoS 报告默认链接最新 TWCC A/B 结果
- `specs/current/test-entrypoints.md`
  - accepted entrypoint 行为已同步

这意味着：

- default QoS full regression 现在会刷新 stable TWCC A/B 报告
- plain-client TWCC 有了固定的报告入口，而不只是 change folder 里的临时 artifact

## 6. 当前签收范围

当前这批改动已经签收的，是：

- plain-client 发送侧 transport control 边界
- transport-cc 协商与 feedback 主路径
- send-side estimate 发布
- padding probe 生命周期
- default QoS regression 中的 stable TWCC A/B 报告入口

## 7. 明确非目标

当前仍然不应被表述成“已经完成”的内容：

- LiveKit downlink `streamallocator` 的完整对象模型
- 新的 weight-aware multi-track transport fairness 模型
- 浏览器 uplink sender 路径重写
- room-level global bitrate budgeting

## 8. 推荐阅读顺序

如果要理解这批变更，建议按这个顺序：

1. [plain-client-qos-status.md](./plain-client-qos-status.md)
2. [linux-client-architecture_cn.md](./linux-client-architecture_cn.md)
3. [twcc-ab-test.md](./twcc-ab-test.md)
4. [../specs/current/plain-client-send-side-bwe.md](../specs/current/plain-client-send-side-bwe.md)
5. `changes/2026-04-21-plain-client-sender-transport-control/`
6. `changes/2026-04-21-livekit-aligned-send-side-bwe/`
