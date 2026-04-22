# Linux PlainTransport C++ Client 线程化测试补充清单

## 1. 文档定位

这份文档用于补齐 Linux `PlainTransport C++ client` 线程化路径当前仍缺失的测试覆盖。

它基于三部分信息整理而来：

- 主设计文档：
  [linux-client-multi-source-thread-model_cn.md](multi-source-thread-model_cn.md)
- 当前轮次实施清单：
  [linux-client-threaded-implementation-checklist_cn.md](threaded-implementation-checklist_cn.md)
- 当前已经存在的线程化测试：
  - [test_thread_model.cpp](../../../tests/test_thread_model.cpp)
  - [test_thread_integration.cpp](../../../tests/test_thread_integration.cpp)

这份清单只关注“当前实现已经进入代码，但还没有充分验证”的部分。

当前**不纳入**这份测试清单的范围：

- 独立 `demux thread`
- `SharedDecodeMultiTrackMode`
- 单文件多 track 共享 decode

这些项尚未实现，不应该先为不存在的实现写测试。

## 2. 当前已有覆盖

当前线程化测试已经覆盖了这些主线：

- `SpscQueue` 基础语义
- `EncodedAccessUnit` / `SenderStatsSnapshot` / `TrackControlCommand` 基础值对象
- `SourceWorker` 的基础输出、Pause/Resume、ForceKeyframe
- `NetworkThread` 的基础 RTP 发送和 stats 输出
- stale generation 跳过
- PLI 路由
- rejected ack retry
- queue-full 不创建 pending command
- pause/resume ack 驱动状态推进
- generation 过滤
- commandId 错配忽略
- recreate 才 bump configGeneration
- pacing 基本行为
- peer stats 存在但该 track 缺失时不误采样
- 多 source worker 的基础 E2E

因此后续补充测试的重点，不是再重复验证“线程骨架能跑”，而是补齐下面几类边界语义。

## 3. 单元测试补充

### P0 命令/ack 协议边界

#### 1. 入队失败不应创建 pending command

状态：`已覆盖`

类型：unit
建议位置：`tests/test_thread_model.cpp`

需要覆盖：

- `actionSink` 入队失败时，不应创建 `PendingCommand`
- `QosController` 不应把该动作记成 pending applied state

对应设计约束：

- “命令已入队”和“命令已应用”必须严格区分

对应实现入口：

- [client/main.cpp](../../../client/main.cpp)

#### 2. 非编码命令也必须走 ack 语义

状态：`已覆盖基础版本`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- `PauseTrack`
- `ResumeTrack`
- `ForceKeyframe`

验证点：

- ack 到来前，controller 不推进状态
- ack applied 后，`confirmAction()` 推进状态
- ack rejected 后，只允许重试，不推进状态

对应设计约束：

- 非编码命令不能绕开 async command/ack 模型

#### 3. 重复 ack / 乱序 ack / 过期 ack

状态：`已覆盖基础版本`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- 相同 `commandId` 的重复 ack 被忽略
- 更老的 `commandId` 在新 pending command 存在时被忽略
- ack 到达顺序打乱时，不会推进错误的 pending command

对应设计约束：

- `control` 必须以 `commandId` 匹配完成态

### P1 generation 语义边界

#### 4. 只有 encoder recreate 才 bump `configGeneration`

状态：`已覆盖`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- 只改 bitrate 时，`configGeneration` 不增加
- 改分辨率 / 改 fps 导致 recreate 时，`configGeneration` 增加

对应设计约束：

- generation 表示配置代际，不是每个命令都递增

#### 5. 旧 generation ack 不推进当前状态

状态：`已覆盖基础版本`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- worker 已进入新 generation 后，旧 generation 的 ack 到达
- control 必须忽略该 ack

对应设计约束：

- 旧消息不能污染当前配置

#### 6. 旧 generation keyframe / retransmission cache 不可复用

状态：`已覆盖基础版本`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- generation 切换后，旧 keyframe cache 被丢弃
- generation 切换后，旧 retransmission store 不再被 `NACK` 使用

对应设计约束：

- old-message filtering 不只针对 AU，也针对缓存状态

### P2 stale stats 语义边界

#### 7. 没有 fresh stats 的 track 不进入 peer coordination

状态：`已覆盖基础版本`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- 某个 track 无 fresh local stats
- 也无该 track 对应的 server stats
- 该 track 不应进入：
  - `peerTrackStates`
  - `trackBudgetRequests`
  - `peerRawSnapshots`

对应设计约束：

- peer coordination 只基于本轮 sampled tracks

#### 8. 有 peer-level server stats 但无该 track stats 时，不能误当 sampled

状态：`已覆盖`

类型：unit
建议位置：`tests/test_thread_integration.cpp`

需要覆盖：

- `cachedPeerStats` 有值
- 但目标 track 没有匹配 producer stats
- 该 track 仍必须视为 stale/unsampled

对应设计约束：

- 不能因为“peer stats 非空”就让所有 track 都过采样门槛

## 4. 集成测试补充

### P0 threaded plain-client 与真实 server 的链路

#### 9. threaded 模式真实 `plainPublish -> getStats -> clientStats`

状态：`已覆盖基础 smoke`

类型：integration
建议位置：新建 `tests/test_threaded_plain_client_integration.cpp` 或扩展现有 harness

需要覆盖：

- threaded 模式下 join / plainPublish 成功
- `getStats` 可返回 producer stats
- `clientStats` 正常发送
- `qosPolicy` / `qosOverride` 能被 threaded path 消费

对应设计约束：

- threaded path 不能只在 loopback UDP 单测里成立

#### 10. threaded 模式下 audio worker 真实可用

状态：`已覆盖基础 smoke`

类型：integration
建议位置：同上

需要覆盖：

- audio worker 成功启动
- 真实音频 RTP 与 audio SR 发送
- `peerHasAudioTrack` 语义与实际音频链路一致

对应设计约束：

- 音频属于 session 级共享轨，且其可用性要和 control 视角一致

### P1 多 track / 多 source 真值测试

#### 11. 多 source worker 下 peer budget 分配不回退

类型：integration
建议位置：扩展 threaded harness

状态：`已覆盖基础版本`

需要覆盖：

- 两路及以上 source worker 并发发送
- peer budget 仍按期望分配
- 某一路 stale / 某一路带宽受限时，其余 track 不被错误拖下水

对应设计约束：

- peer-level coordination 必须对 sampled tracks 的集合稳定工作

说明：

- 当前已覆盖：
  - 本地 `QOS_TRACE` 多 track bitrate ordering smoke
  - 真实 server 的 multi-track `clientStats` 聚合 smoke
  - 服务端 `clientStats` 窗口内至少一次 send bitrate ordering 观测
- 仍可继续加强更长时间窗口、更多路数和更复杂时序的真值审计。

#### 12. 配置切换期间旧 generation 不泄漏到真实发送

类型：integration
建议位置：扩展 threaded harness

需要覆盖：

- QoS 触发 encoder recreate
- 切换期间旧 AU 不再出现在网络侧发送
- PLI/FIR 不会补发旧 generation 关键帧

对应设计约束：

- generation 过滤在真实 transport 侧成立

## 5. Harness / Matrix 补充

### 13. threaded client matrix smoke

状态：`已覆盖基础 harness smoke`

类型：harness
建议位置：

- `tests/qos_harness/run_cpp_client_harness.mjs`
- `tests/qos_harness/run_cpp_client_matrix.mjs`

需要覆盖：

- threaded path 基础 smoke case
- single-track threaded case
- two-source threaded case

目标不是一次把所有 matrix 都切到 threaded，而是先有最小 smoke。

当前已覆盖：

- `threaded_publish_snapshot`
- `threaded_audio_stats_smoke`
- `threaded_multi_track_snapshot`
- `threaded_multi_video_budget`
- `threaded_default_multi_track_snapshot`
- `threaded_quick`

### 14. threaded QoS regression case

状态：`已覆盖基础 harness regression`

类型：harness

当前已覆盖：

- `threaded_generation_switch`

另外，下列边界已经转到 gtest / integration 覆盖，而不是继续保留进程内 harness 注入：

- stale stats freshness
- stale / mismatched command ack
- stale config-generation reject

另外，当前还提供了一个顺序聚合入口：

- `threaded_quick`

### 15. default startup case

类型：harness

需要覆盖：

- 多 track 单文件
- 不显式提供 `PLAIN_CLIENT_VIDEO_SOURCES` 时，仍直接进入默认 threaded runtime

对应设计约束：

- 单文件 bootstrap 只负责输入发现与初始化，不再触发 sender QoS 的 legacy fallback

## 6. 建议优先级

建议按下面顺序补测试：

1. `P0`：更复杂的命令/ack 乱序与失败语义
2. `P1`：剩余 generation / stale stats 真值场景
3. `P2`：audio worker 与多 source 的真实 server 集成链路
4. `P3`：threaded harness smoke

原因：

- 当前剩余风险主要集中在协议边界
- 这些问题不补，后面的多 source / harness 扩展意义有限

## 7. 退出标准

这份测试补充清单可以认为完成，当且仅当：

- 命令/ack 的关键边界场景都有自动化覆盖
- generation / stale stats 的关键边界场景都有自动化覆盖
- 至少存在一条 threaded plain-client 对真实 server 的 smoke 集成链路
- harness 里至少有一条 threaded smoke case
