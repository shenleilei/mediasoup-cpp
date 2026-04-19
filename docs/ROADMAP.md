# mediasoup-cpp 12 个月迭代清单

时间线：2026.04 → 2027.04
- M0-M3（4月→7月）：上线准备期
- M3-M7（7月→11月）：生产打磨期
- M7-M13（11月→次年4月）：业界顶尖冲刺期

---

## 第一阶段：上线准备（M0-M3）

### Sprint 1（M0-M0.5，4月中→4月底）：v3 收尾 + 工程卫生

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 1.1 | downlink QoS v3 阶段 5：matrix runner 骨架 | P0 | 2d | `tests/qos_harness/run_downlink_matrix.mjs`, `tests/qos_harness/browser/downlink-matrix-entry.js` | 无 | `node tests/qos_harness/run_downlink_matrix.mjs` 能跑通 baseline case 组 |
| 1.2 | downlink matrix case 定义：baseline / bw / loss / rtt / jitter / transition / zero_demand | P0 | 2d | `tests/qos_harness/run_downlink_matrix.mjs` | 1.1 | 8 个 case 组全部有定义和断言 |
| 1.3 | downlink matrix 报告渲染 | P1 | 1d | `tests/qos_harness/render_downlink_case_report.mjs` | 1.2 | 生成 `docs/downlink-qos-case-results.md` 和 `docs/generated/downlink-qos-matrix-report.json` |
| 1.4 | downlink matrix 归档对齐 uplink | P1 | 0.5d | `scripts/run_qos_tests.sh` | 1.3 | `docs/archive/downlink-qos-runs/<timestamp>/` 结构和 uplink 一致 |
| 1.5 | `QosValidator` 补 `pauseUpstream`/`resumeUpstream` 结构化校验 | P1 | 0.5d | `src/qos/QosValidator.cpp` | 无 | 非布尔值的 `pauseUpstream` 被拒绝，单测覆盖 |
| 1.6 | bundle 自动构建脚本 | P1 | 0.5d | `scripts/build_qos_bundle.sh` 或 `src/client/package.json` scripts | 无 | `npm run build:bundle` 自动生成 `public/mediasoup-client.qos.bundle.js` |
| 1.7 | worker 黑盒验证文档 | P1 | 1d | `docs/downlink-qos-v3-worker-validation_cn.md` | 1.1 跑完 | 明确回答 pause 后 RTP 是否停发、resume 首帧时间 |

### Sprint 2（M0.5-M1，5月初→5月中）：生产运维基础

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 2.1 | 日志 JSON 结构化 | P0 | 2d | `src/Logger.h`, `src/main.cpp` | 无 | `--logFormat=json` 输出 JSON 行，每行含 `ts`, `level`, `module`, `roomId`, `peerId`, `msg` |
| 2.2 | Prometheus `/metrics` endpoint | P0 | 3d | `src/SignalingServer.cpp`, 新增 `src/MetricsExporter.h` | 无 | `/metrics` 返回 Prometheus text format，含 `mediasoup_rooms_total`, `mediasoup_peers_total`, `mediasoup_worker_cpu_percent`, `mediasoup_transports_active` |
| 2.3 | `/healthz` + `/readyz` endpoint | P0 | 0.5d | `src/SignalingServer.cpp` | 无 | `/healthz` 返回 200，`/readyz` 在 worker 未 ready 或 Redis 不可达时返回 503 |
| 2.4 | 优雅关停 | P0 | 1d | `src/main.cpp`, `src/SignalingServer.cpp` | 无 | SIGTERM 后停止 accept，等待现有 room drain（最长 30s），然后退出 |
| 2.5 | 录制路径 crash guard | P1 | 1d | `src/Recorder.h` | 无 | FFmpeg muxing 异常时 catch 并关闭录制，不影响 room |
| 2.6 | `announcedIp` 多网卡 fallback | P1 | 0.5d | `src/main.cpp` | 无 | 多网卡时优先选非 loopback、非 docker bridge 的地址 |
| 2.7 | QoS policy 热加载 | P2 | 1d | `src/RoomService.cpp`, `src/SignalingServer.cpp` | 无 | `SIGHUP` 重新读取 `config.json` 中的 QoS policy 字段，不重启 |

### Sprint 3（M1-M1.5，5月中→5月底）：安全与配置

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 3.1 | WebSocket origin 校验 | P0 | 0.5d | `src/SignalingServer.cpp` | 无 | `--allowedOrigins` 配置项，不在白名单的 origin 拒绝 upgrade |
| 3.2 | 连接级 rate limiting | P0 | 1d | `src/SignalingServer.cpp` | 无 | 单 IP 每秒最多 N 个新连接（默认 10），超限返回 429 |
| 3.3 | 消息级 rate limiting | P1 | 1d | `src/SignalingServer.cpp` | 无 | 单 socket 每秒最多 M 条消息（默认 50），超限断开 |
| 3.4 | DTLS 证书配置 | P1 | 0.5d | `src/main.cpp`, `src/Worker.cpp` | 无 | `--dtlsCert` / `--dtlsKey` 支持自定义证书 |
| 3.5 | Redis 认证 | P1 | 0.5d | `src/RoomRegistry.h` | 无 | `--redisPassword` 支持 |
| 3.6 | 配置文件 schema 校验 | P2 | 1d | `src/main.cpp` | 无 | 启动时校验 `config.json` 格式，错误字段给出明确报错 |

### Sprint 4（M1.5-M2，6月初→6月中）：压测框架

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 4.1 | 压测脚本：N 个 1v1 room 并发 | P0 | 2d | `tests/bench/stress_rooms.mjs` | 无 | 可配置 room 数、持续时间、上报间隔 |
| 4.2 | 压测指标采集：CPU / RSS / PPS / room 数 | P0 | 1d | `tests/bench/stress_rooms.mjs` | 4.1 | 输出 CSV 或 JSON 时序数据 |
| 4.3 | 24h 稳定性测试 | P0 | 1d（执行） | 无 | 4.1, 4.2 | 100 个 1v1 room 运行 24h，无 OOM、无 worker crash、CPU < 70% |
| 4.4 | 内存泄漏检测 | P1 | 1d | 无 | 4.3 | valgrind / ASan 跑 1h 压测，无 leak |
| 4.5 | worker crash 注入测试 | P1 | 1d | `tests/test_stability_integration.cpp` | 无 | kill worker 进程后 5s 内 respawn，room 内 peer 收到 error 通知 |

### Sprint 5（M2-M2.5，6月中→6月底）：多节点验证

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 5.1 | 2 节点部署脚本 | P0 | 1d | `deploy/multi-node/` | 无 | docker-compose 一键拉起 2 个 SFU + 1 个 Redis |
| 5.2 | room ownership 切换测试 | P0 | 1d | `tests/test_multinode_integration.cpp` | 5.1 | node A 下线后，node B 能接管 room |
| 5.3 | geo routing 端到端验证 | P1 | 1d | `tests/test_multinode_integration.cpp` | 5.1 | 中国 IP 路由到中国节点，美国 IP 路由到美国节点 |
| 5.4 | Redis 断连降级测试 | P1 | 0.5d | `tests/test_multinode_integration.cpp` | 5.1 | Redis 断开后降级到 local-only，恢复后自动重连 |

### Sprint 6（M2.5-M3，7月初→7月中）：上线前收口

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 6.1 | uplink + downlink QoS matrix 全量跑通 | P0 | 1d（执行） | 无 | 1.1-1.4 | 全部 PASS 或已知 exception 有文档说明 |
| 6.2 | `PRODUCTION_CHECKLIST.md` 更新 | P0 | 1d | `docs/PRODUCTION_CHECKLIST.md` | 全部 | 可照做的上线检查清单 |
| 6.3 | `MONITORING_RUNBOOK.md` 更新 | P0 | 0.5d | `docs/MONITORING_RUNBOOK.md` | 2.2 | 告警规则 + 处理流程 |
| 6.4 | Grafana dashboard 更新 | P1 | 1d | `deploy/monitoring/grafana/dashboards/` | 2.2 | 新增 QoS 面板：uplink/downlink 状态分布、pause/resume 频率 |
| 6.5 | changelog + release tag | P1 | 0.5d | `CHANGELOG.md` | 全部 | v1.0.0 tag |

---

## 第二阶段：生产打磨（M3-M7）

### Sprint 7（M3-M3.5）：上线后稳定性

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 7.1 | 生产 QoS 数据落盘 | P0 | 2d | `src/RoomService.cpp`, 新增 `src/QosDataSink.h` | 无 | clientStats / downlinkClientStats 按 room 写入 JSONL 文件，可配置开关 |
| 7.2 | worker crash room-level graceful recovery | P0 | 3d | `src/WorkerThread.h`, `src/RoomService.cpp` | 无 | worker crash 后，该 worker 上的 room 内 peer 收到 reconnect 通知而非直接断开 |
| 7.3 | WebSocket ping/pong + idle timeout | P1 | 1d | `src/SignalingServer.cpp` | 无 | 60s 无消息发 ping，90s 无 pong 断开 |
| 7.4 | reconnect 后 session 恢复边界 case | P1 | 2d | `src/SignalingServer.cpp`, `src/RoomService.cpp` | 无 | 补测试：reconnect 时旧 consumer 清理、新 consumer 重建、QoS 状态重置 |
| 7.5 | 录制中断自动重试 | P2 | 1d | `src/Recorder.h` | 无 | 录制 FFmpeg 进程异常退出后自动重启，最多重试 3 次 |

### Sprint 8（M3.5-M4.5）：规模化 - WorkerThread

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 8.1 | 多 WorkerThread 负载均衡策略 | P0 | 2d | `src/SignalingServer.cpp`, `src/WorkerManager.h` | 无 | room 分配到负载最低的 WorkerThread，而非 round-robin |
| 8.2 | 多 WorkerThread 压测 | P0 | 1d（执行） | `tests/bench/stress_rooms.mjs` | 8.1 | 2-4 个 WorkerThread，200 room 稳定 |
| 8.3 | 单 WorkerThread 多 worker 进程 | P1 | 3d | `src/WorkerThread.h`, `src/WorkerManager.h` | 无 | 1 个 WorkerThread 管理 2-4 个 mediasoup-worker，按 CPU 分配 room |
| 8.4 | worker 进程动态扩缩 | P2 | 2d | `src/WorkerManager.h` | 8.3 | 负载超阈值时自动 spawn 新 worker，空闲时回收 |

### Sprint 9（M4.5-M5.5）：规模化 - 大房间

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 9.1 | selective forwarding：只转发 active speaker + pinned | P0 | 5d | `src/RoomService.cpp`, `src/Consumer.cpp`, 新增 `src/SelectiveForwarder.h` | 无 | 10 人房间，每个 subscriber 最多收 4 路视频（1 active speaker + 3 pinned/grid） |
| 9.2 | active speaker 检测 | P0 | 2d | 新增 `src/ActiveSpeakerDetector.h` | 无 | 基于 audio level 的 top-N speaker 检测，500ms 更新周期 |
| 9.3 | 大房间压测 | P0 | 1d（执行） | `tests/bench/stress_rooms.mjs` | 9.1, 9.2 | 20 人房间稳定运行，CPU < 80% |
| 9.4 | Redis 连接池 | P1 | 1d | `src/RoomRegistry.h` | 无 | 连接池大小可配置，默认 4 |
| 9.5 | Redis sentinel 支持 | P2 | 2d | `src/RoomRegistry.h` | 9.4 | `--redisSentinel` 配置项 |

### Sprint 10（M5.5-M7）：QoS 深化

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 10.1 | 生产 QoS 数据分析脚本 | P0 | 2d | `scripts/analyze_qos_data.py` | 7.1 | 输出 loss/RTT/jitter 分布、状态转移频率、pause/resume 统计 |
| 10.2 | uplink QoS profile 阈值校准 | P0 | 3d | `src/client/lib/qos/profiles.js` | 10.1 | 基于生产数据调整 stableLossRate / stableRttMs / stableJitterMs |
| 10.3 | downlink pause/resume 阈值校准 | P0 | 2d | `src/qos/ProducerDemandAggregator.h` | 10.1 | 基于生产数据调整 kPauseConfirmMs / kResumeWarmupMs / kHoldMs |
| 10.4 | simulcast 编码参数优化 | P1 | 2d | `public/qos-demo.js`, `src/client/lib/qos/profiles.js` | 10.1 | 根据实际网络分布调整 3 层 simulcast 的分辨率和码率 |
| 10.5 | 端到端延迟采集 | P1 | 3d | `src/client/lib/qos/downlinkSampler.js`, `src/RoomService.cpp` | 无 | sender timestamp → receiver timestamp 差值采集，在 stats 中上报 |
| 10.6 | 延迟告警规则 | P2 | 1d | `deploy/monitoring/prometheus/rules/` | 10.5 | P95 延迟 > 500ms 触发告警 |

---

## 第三阶段：业界顶尖冲刺（M7-M13）

### Sprint 11（M7-M8.5）：dynacast

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 11.1 | producer demand → simulcast layer 供给控制设计文档 | P0 | 2d | `docs/dynacast-design_cn.md` | 无 | 明确 worker 改造范围 |
| 11.2 | mediasoup-worker 源码分析：Producer forwarding 路径 | P0 | 3d | `mediasoup-worker` 源码 | 无 | 文档化 Producer::HandleRtpPacket → SimulcastConsumer 的转发逻辑 |
| 11.3 | worker 侧 layer pause API | P0 | 5d | `mediasoup-worker` 源码, `src/Channel.cpp` | 11.2 | 新增 FlatBuffers IPC 消息 `producer.pauseLayer(spatialLayer)` / `producer.resumeLayer(spatialLayer)` |
| 11.4 | SFU 侧 dynacast controller | P0 | 3d | 新增 `src/qos/DynacastController.h` | 11.3 | 聚合所有 subscriber 对某 producer 的最高需求层，调用 worker API 停发未消费层 |
| 11.5 | dynacast 集成测试 | P0 | 2d | `tests/test_qos_integration.cpp` | 11.4 | 所有 subscriber hidden 后，producer 停发所有视频层；恢复后重新发送 |
| 11.6 | dynacast browser harness | P1 | 2d | `tests/qos_harness/browser_dynacast.mjs` | 11.4 | 浏览器端验证 sender 实际停止编码 |

### Sprint 12（M8.5-M9.5）：SVC + WHIP/WHEP

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 12.1 | VP9 SVC codec 支持 | P0 | 5d | `src/supportedRtpCapabilities.h`, `src/ortc.h`, `src/Transport.cpp` | 无 | VP9 SVC produce/consume 端到端跑通 |
| 12.2 | SVC layer 选择逻辑 | P0 | 3d | `src/qos/SubscriberBudgetAllocator.cpp`, `src/Consumer.cpp` | 12.1 | setPreferredLayers 对 SVC consumer 生效 |
| 12.3 | SVC + simulcast fallback | P1 | 2d | `src/RoomService.cpp` | 12.1 | Safari 不支持 SVC 时自动 fallback 到 simulcast |
| 12.4 | WHIP endpoint | P1 | 3d | `src/SignalingServer.cpp`, 新增 `src/WhipHandler.h` | 无 | `POST /whip` 接受 SDP offer，返回 answer，创建 producer |
| 12.5 | WHEP endpoint | P1 | 3d | `src/SignalingServer.cpp`, 新增 `src/WhepHandler.h` | 无 | `POST /whep` 接受 SDP offer，返回 answer，创建 consumer |

### Sprint 13（M9.5-M10.5）：级联 + 全局 allocator

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 13.1 | 节点间 PipeTransport 级联 | P0 | 5d | `src/RoomService.cpp`, `src/PipeTransport.cpp`, `src/RoomRegistry.h` | 无 | room 跨 2 个节点，peer A 在 node 1 produce，peer B 在 node 2 consume |
| 13.2 | 级联 QoS：pipe consumer 不参与 downlink allocator | P0 | 1d | `src/qos/SubscriberBudgetAllocator.cpp` | 13.1 | pipe consumer 不被 pause/setPreferredLayers |
| 13.3 | 全局 bitrate allocator 设计 | P0 | 2d | `docs/global-allocator-design_cn.md` | 无 | 房间级带宽预算分配算法设计 |
| 13.4 | 全局 bitrate allocator 实现 | P0 | 5d | 新增 `src/qos/GlobalBitrateAllocator.h` | 13.3 | 房间总带宽受限时，按优先级分配给各 subscriber |
| 13.5 | 级联压测 | P1 | 1d（执行） | 无 | 13.1 | 2 节点级联，10 room 稳定 |

### Sprint 14（M10.5-M11.5）：智能化

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 14.1 | 服务端 TWcc 分析 | P0 | 5d | `src/Channel.cpp`, 新增 `src/TwccAnalyzer.h` | 无 | 从 worker 获取 TWcc feedback，计算服务端侧带宽估计 |
| 14.2 | 双向带宽估计融合 | P1 | 3d | `src/qos/RoomDownlinkPlanner.cpp` | 14.1 | 结合 client 上报和服务端 TWcc 做更准确的带宽判断 |
| 14.3 | 自适应 FEC | P1 | 5d | `mediasoup-worker` 源码 | 无 | 根据实时丢包率动态调整 FEC 冗余度（0-50%） |
| 14.4 | 网络路径探测 | P2 | 3d | `src/GeoRouter.h` | 无 | 节点间 RTT 探测，动态更新路由权重 |

### Sprint 15（M11.5-M13）：极致体验 + 开源

| # | Task | 优先级 | 工时 | 涉及文件 | 依赖 | 验收标准 |
|---|---|---|---|---|---|---|
| 15.1 | E2EE：Insertable Streams 集成 | P0 | 5d | `public/qos-demo.js`, 新增 `src/client/lib/e2ee/` | 无 | 开启 E2EE 后 SFU 无法解密媒体内容 |
| 15.2 | 超低延迟模式 | P1 | 3d | `src/Transport.cpp`, `src/Worker.cpp` | 无 | `--lowLatency` 模式下 P95 端到端延迟 < 100ms |
| 15.3 | 大规模直播模式 | P1 | 5d | 新增 `src/BroadcastMode.h`, `src/SignalingServer.cpp` | 9.1 | 1 publisher → 1000 subscriber，服务端 ABR |
| 15.4 | QoS 可观测平台 | P1 | 5d | `deploy/monitoring/grafana/dashboards/` | 10.5 | 实时 dashboard：每 peer 网络状态、QoS 决策链路、历史趋势 |
| 15.5 | 文档英文化 | P1 | 3d | `docs/` | 无 | 核心设计文档全部有英文版 |
| 15.6 | API 稳定化 + semver | P1 | 2d | `src/SignalingServer.cpp` | 无 | WebSocket 协议版本号，向后兼容保证 |
| 15.7 | CI/CD pipeline | P0 | 2d | `.github/workflows/ci.yml` | 无 | PR 自动跑 unit test + integration test + lint |
| 15.8 | contributor guide | P2 | 1d | `CONTRIBUTING.md` | 无 | 开发环境搭建、代码规范、PR 流程 |

---

## 总工时估算

| 阶段 | Sprint | 工时（人天） |
|---|---|---|
| 上线准备 | Sprint 1-6 | ~45d |
| 生产打磨 | Sprint 7-10 | ~50d |
| 业界顶尖 | Sprint 11-15 | ~95d |
| **合计** | | **~190d（约 9.5 人月）** |

注：以上按 1 人全职估算。如果 2 人并行，关键路径可压缩约 40%。

## 关键路径

```
Sprint 1-2 (v3收尾+运维基础)
    → Sprint 4 (压测)
        → Sprint 6 (上线收口)
            → M3 上线
                → Sprint 7 (稳定性)
                    → Sprint 8-9 (规模化)
                        → Sprint 10 (QoS校准)
                            → Sprint 11 (dynacast)
                                → Sprint 13 (级联+allocator)
                                    → Sprint 14-15 (智能化+极致体验)
                                        → M13 业界顶尖
```

## 风险登记

| 风险 | 影响 | 概率 | 缓解措施 |
|---|---|---|---|
| mediasoup-worker 源码改造复杂度超预期 | Sprint 11, 14 延期 | 高 | M7 前先做 worker 源码分析（11.2），提前评估 |
| VP9 SVC Safari 兼容性 | Sprint 12 部分功能受限 | 中 | 12.3 做 simulcast fallback |
| 生产环境网络分布与测试环境差异大 | Sprint 10 校准结果不准 | 中 | 7.1 尽早开始数据回收，积累足够样本 |
| Redis 级联一致性边界 case 多 | Sprint 13 延期 | 中 | 5.2-5.4 提前验证基础多节点场景 |
| 大规模直播模式需要重新设计 consumer 管理 | Sprint 15.3 延期 | 中 | 9.1 selective forwarding 先打基础 |
| 1 人全职可能不够 | 整体延期 | 高 | M3 后考虑加人，或砍 P2 项 |
