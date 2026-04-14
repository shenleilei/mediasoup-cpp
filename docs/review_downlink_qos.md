# 下行 QoS 分支 Code Review 问题清单

**分支**: `dev/downside_qos` vs `main`
**Review 日期**: 2026-04-14
**改动规模**: ~100 个文件
**功能范围**: 下行自适应 QoS 控制 (Downlink Adaptive Quality of Service)

---

## 目录

- [一、🔴 严重问题（6 项）](#一-严重问题6-项)
- [二、🟠 高优先级问题（8 项）](#二-高优先级问题8-项)
- [三、🟡 中等问题（8 项）](#三-中等问题8-项)
- [四、🟢 低优先级 / 代码风味（7 项）](#四-低优先级--代码风味7-项)
- [五、优先处理建议](#五优先处理建议)
- [六、总结](#六总结)

---

## 一、🔴 严重问题（6 项）

> 可能导致崩溃、数据损坏或安全漏洞

### S1. `Consumer::close()` 未校验 `channel_` 空指针

| 属性 | 值 |
|------|-----|
| **文件** | `src/Consumer.cpp` (约 59-62 行) |
| **类型** | 空指针解引用 / 崩溃 |
| **严重性** | 🔴 Critical |

**问题描述**：`close()` 中直接解引用 `channel_` 发送 FBS 请求，但未做 null 检查。若 `channel_` 为空（如 Worker 已析构），会触发空指针崩溃。

**修复建议**：
```cpp
void Consumer::close() {
    if (!channel_) return;  // 加入空指针保护
    // ... 原有逻辑
}
```

---

### S2. `RoomService` 目录操作泄漏文件句柄

| 属性 | 值 |
|------|-----|
| **文件** | `src/RoomService.cpp` (约 669-695 行) |
| **类型** | 资源泄漏 (fd leak) |
| **严重性** | 🔴 Critical |

**问题描述**：`DIR*` 由 `opendir` 打开后只在正常路径调用 `closedir`，若中间抛异常则泄漏。内层 `rdir` 同理。长期运行将耗尽文件描述符。

**修复建议**：使用 RAII 包装器（如自定义 `unique_dir`）或在 catch / finally 路径中确保 `closedir`。

---

### S3. `QosValidator` 中 NaN / Infinity 导致未定义行为

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/QosValidator.cpp` (约 485 行) |
| **类型** | 未定义行为 (UB) |
| **严重性** | 🔴 Critical |

**问题描述**：`std::clamp(s.value("freezeRate", 0.0), 0.0, 1.0)` 若 JSON 值为 NaN，`std::clamp` 行为**未定义**（C++17 标准）。`jitter`、`framesPerSecond`、`currentRoundTripTime` 等字段同理，均缺少 `std::isfinite()` 守卫。

**风险**：恶意客户端可注入 NaN/Infinity 触发 UB，影响整个 QoS 决策链路。

**修复建议**：
```cpp
double val = s.value("freezeRate", 0.0);
if (!std::isfinite(val)) val = 0.0;
val = std::clamp(val, 0.0, 1.0);
```

---

### S4. `DownlinkQosRegistry` 序列号回绕处理错误

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/DownlinkQosRegistry.cpp` (12-17 行) |
| **类型** | 逻辑 Bug |
| **严重性** | 🔴 Critical |

**问题描述**：判断逻辑 `prevSeq - snapshot.seq <= kSeqResetThreshold` 中，两者都是 `uint64_t`，减法溢出后变成巨大正数，永远不满足条件。序列号真正回绕时全部被误拒。

**影响**：客户端 seq 溢出后，所有后续快照都会被服务端以 "stale-seq" 拒绝。

**修复建议**：改用有符号差值或显式处理回绕情况：
```cpp
int64_t diff = static_cast<int64_t>(prevSeq) - static_cast<int64_t>(snapshot.seq);
if (diff > 0 && static_cast<uint64_t>(diff) <= kSeqResetThreshold) {
    // stale
}
```

---

### S5. HTML 注入漏洞（Demo 页面）

| 属性 | 值 |
|------|-----|
| **文件** | `public/qos-demo.js` (约 59 行) |
| **类型** | XSS / HTML 注入 |
| **严重性** | 🔴 Critical |

**问题描述**：`els.status.innerHTML = '<strong>状态：</strong> ' + text` 直接拼接未转义内容。若 `text` 来自服务器消息可注入脚本。

**修复建议**：
```javascript
els.status.textContent = '状态：' + text;
// 或用 DOM API 分离标签与文本
```

---

### S6. 客户端 `seq` 计数器溢出无处理

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/controller.js` (约 663 行) |
| **类型** | 逻辑 Bug / 数据丢失 |
| **严重性** | 🔴 Critical |

**问题描述**：`++this.seq` 持续自增，超过 `QOS_MAX_SEQ (2^53-1)` 后，所有快照会被服务端拒绝（seq out of range），直到控制器重建。无取模或重置机制。

**与 S4 联动**：客户端 seq 溢出 + 服务端回绕判断缺陷共同作用，导致 QoS 完全失效。

**修复建议**：
```javascript
this.seq = (this.seq + 1) % (QOS_MAX_SEQ + 1);
```

---

## 二、🟠 高优先级问题（8 项）

> 资源泄漏、并发问题、逻辑错误

### H1. `WorkerThread` 中多处 `epoll_ctl` 返回值未检查

| 属性 | 值 |
|------|-----|
| **文件** | `src/WorkerThread.h` (87-96, 110, 125 行) |
| **类型** | 错误处理缺失 |
| **严重性** | 🟠 High |

**问题描述**：若 `epoll_ctl` 注册失败，fd 不会被监听，事件丢失但无任何日志或异常。定时器 / 事件 fd 可能静默失效。

**修复建议**：检查返回值，失败时 log 并考虑 fallback 或 abort。

---

### H2. `ProducerDemandAggregator::finalize()` 状态初始化歧义

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/ProducerDemandAggregator.cpp` (约 106, 134, 166 行) |
| **类型** | 潜在未初始化变量 |
| **严重性** | 🟠 High |

**问题描述**：当 `prevIt == prev.end()` 时，`state.pauseEligibleAtMs` 和 `state.resumeEligibleAtMs` 默认为 `0`。后续用 `if (state.pauseEligibleAtMs == 0)` 判断"从未设置"，但无法区分"已清零"两种语义。首次迭代可能使用垃圾值。

**修复建议**：使用 `std::optional<uint64_t>` 或哨兵值（如 `UINT64_MAX`）明确"未初始化"语义。

---

### H3. `DownlinkAllocator::ComputeDiff()` 状态跟踪与实际 diff 不一致

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/DownlinkAllocator.cpp` (约 100-127 行) |
| **类型** | 状态同步 Bug |
| **严重性** | 🟠 High |

**问题描述**：`lastState` 在 diff 过滤后**全量更新为目标状态**。若某 action 被过滤（认为 unchanged），而 Worker 端实际状态不同步（如 consumer 被 Worker 侧恢复），后续 diff 永远认为"已同步"不再发送指令。

**修复建议**：仅在 action 真正被 apply 后更新对应的 `lastState` 字段，而非总是写入目标值。

---

### H4. `resolveProducerOwnerPeerId()` O(n×m) 低效查找

| 属性 | 值 |
|------|-----|
| **文件** | `src/RoomService.cpp` (约 1505-1510 行) |
| **类型** | 性能问题 |
| **严重性** | 🟠 High |

**问题描述**：每次 downlink planning 对每个 demand state 都遍历全部 peer × producer。Room 规模大时开销显著。

**修复建议**：维护 `producerId → peerId` 反向索引 map，在 producer 创建/销毁时更新。

---

### H5. `DownlinkHealthMonitor` 的 `packetsLost` 可能是累计值

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/DownlinkHealthMonitor.cpp` (约 22 行) |
| **类型** | 逻辑 Bug |
| **严重性** | 🟠 High |

**问题描述**：`packetsLost / 100.0` 当作"丢包率"，但 WebRTC `getStats()` 的 `packetsLost` 是**累计值**。如果客户端不做增量处理直接上报，该值会单调递增，导致长时间运行后**永远判定为 congested**。

**修复建议**：客户端需上报增量丢包率（delta packets lost / delta packets received），或服务端需维护前后两次快照的差值。

---

### H6. 所有 browser harness 临时目录未清理

| 属性 | 值 |
|------|-----|
| **文件** | `tests/qos_harness/browser_downlink_controls.mjs`, `browser_downlink_e2e.mjs`, `browser_downlink_v2.mjs`, `browser_downlink_v3.mjs`, `run_downlink_matrix.mjs` |
| **类型** | 资源泄漏 (磁盘) |
| **严重性** | 🟠 High |

**问题描述**：`fs.mkdtempSync()` 创建的 tmpDir 在 finally 块中都**没有 `rmSync`**。CI 反复运行会耗尽磁盘空间。

**修复建议**：在 finally 块添加：
```javascript
fs.rmSync(tmpDir, { recursive: true, force: true });
```

---

### H7. 测试硬编码端口号（14013-14018）引发并行冲突

| 属性 | 值 |
|------|-----|
| **文件** | 各 `browser_downlink_*.mjs` 文件 |
| **类型** | 测试可靠性 |
| **严重性** | 🟠 High |

**问题描述**：端口固定，若两个 CI job 并行或上一次 test crash 留下 TIME_WAIT 则 EADDRINUSE 失败。

**修复建议**：使用 `port: 0` 让 OS 分配随机端口，再从 `server.address().port` 读取实际端口。

---

### H8. `SignalingServer` shutdown 丢失 registry 任务

| 属性 | 值 |
|------|-----|
| **文件** | `src/SignalingServer.cpp` (约 300-317 行) |
| **类型** | 竞态条件 |
| **严重性** | 🟠 High |

**问题描述**：在 `startRegistryWorker()` 中，线程等待 `registryTaskCv_` 后检查 `stopRegistryThread_`。如果任务在 stop 信号和最终检查之间入队，该任务会被丢弃不执行。

**修复建议**：shutdown 时先 drain 队列再设 stop 标志，或 stop 后仍执行剩余任务。

---

## 三、🟡 中等问题（8 项）

> 逻辑瑕疵、健壮性、可维护性

### M1. `Peer::close()` 幂等性不保证

| 属性 | 值 |
|------|-----|
| **文件** | `src/Peer.h` (25-36 行) |
| **类型** | 多次调用安全 |
| **严重性** | 🟡 Medium |

**问题描述**：虽然检查了 `sendTransport` 非空后再 close + reset，但如果 `close()` 被多线程或重入调用，`producer/consumer` 的 `closed()` 状态在第一轮 close 和 reset 之间可能不一致。

**修复建议**：加入 `closed_` 标志位，第二次调用直接 return。

---

### M2. `SubscriberBudgetAllocator` 带宽估算使用硬编码常数

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/SubscriberBudgetAllocator.cpp` (约 10-15 行) |
| **类型** | 可配置性 |
| **严重性** | 🟡 Medium |

**问题描述**：`kBaseBitrateBps=100000`、`kScreenShareBaseBps=300000` 是写死值，无法通过 config / runtime override。实际编码器出码可能差异很大。

**修复建议**：从配置文件或 API 参数中读取这些值。

---

### M3. 客户端 `downlinkSampler.js` 访问 transport 私有属性

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/downlinkSampler.js` (45 行) |
| **类型** | 脆弱依赖 |
| **严重性** | 🟡 Medium |

**问题描述**：`this._transport?._handler?._pc` 依赖 mediasoup-client 内部实现结构，库升级后很可能 break。

**修复建议**：通过 mediasoup-client 公开 API 获取 PeerConnection，或显式传入 `pc` 对象。

---

### M4. 客户端 `downlinkSampler.js` 变异 WebRTC stats 对象

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/downlinkSampler.js` (75-78, 101-103 行) |
| **类型** | 副作用 / 非预期修改 |
| **严重性** | 🟡 Medium |

**问题描述**：给浏览器返回的 `RTCStatsReport` 条目上挂 `_matched` 属性再删除，违反只读语义。某些浏览器实现可能抛错或静默忽略。

**修复建议**：使用本地 `Set<ssrc>` 跟踪已匹配项，不修改原始 stats 对象。

---

### M5. 客户端 `protocol.js` 中 `parseDownlinkSnapshot()` 直接修改入参

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/protocol.js` (约 62-77 行) |
| **类型** | 副作用 |
| **严重性** | 🟡 Medium |

**问题描述**：`payload.schema = exports.DOWNLINK_SCHEMA_V1` 会改写调用方传入的对象，可能引发难以追踪的 bug。

**修复建议**：返回新对象 `{ ...payload, schema: exports.DOWNLINK_SCHEMA_V1 }`。

---

### M6. `pauseUpstream` 和 `resumeUpstream` 可同时为 true

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/protocol.js` (约 279-316 行) + `src/qos/QosValidator.cpp` |
| **类型** | 输入校验缺失 |
| **严重性** | 🟡 Medium |

**问题描述**：两端都没校验 `pauseUpstream` 和 `resumeUpstream` 这两个互斥标志不能同时为 true。同时为 true 时行为未定义。

**修复建议**：在 parse 阶段校验互斥性，若同时为 true 则抛错或以后者为准。

---

### M7. 测试使用硬编码 sleep 等待异步操作

| 属性 | 值 |
|------|-----|
| **文件** | `tests/test_integration.cpp` (约 965, 989 行) |
| **类型** | 测试稳定性 |
| **严重性** | 🟡 Medium |

**问题描述**：`usleep(2000000)` / `usleep(4000000)` 依赖绝对时间。CI 负载高时 worker 可能来不及 respawn，导致 flaky failure。

**修复建议**：使用轮询 + 超时模式等待预期状态，如：
```cpp
for (int i = 0; i < 40; i++) {
    usleep(100000);
    if (workerReady()) break;
}
```

---

### M8. `CMakeLists.txt` 使用 glob 收集测试再按名字排除

| 属性 | 值 |
|------|-----|
| **文件** | `CMakeLists.txt` (105-112 行) |
| **类型** | 构建可维护性 |
| **严重性** | 🟡 Medium |

**问题描述**：`file(GLOB TEST_SOURCES ...)` + 逐个 `list(FILTER ... EXCLUDE)` 脆弱，新文件可能意外被包含或遗漏。CMake 官方也不推荐用 GLOB 管理源文件列表。

**修复建议**：显式列出需要编译的测试文件，或使用 CTest `add_test()` 分开注册。

---

## 四、🟢 低优先级 / 代码风味（7 项）

| 编号 | 文件 | 问题描述 |
|------|------|----------|
| L1 | `src/qos/DownlinkHealthMonitor.cpp:65` | `else if (observed != kCongested)` 冗余条件——已在 `kCongested` case 里，永远为 false |
| L2 | `src/client/lib/qos/downlinkHints.js` | `setVisible/setPinned` 等对不存在的 consumerId 静默忽略，无日志。建议 `console.warn` |
| L3 | `src/client/lib/qos/controller.js:24-40` | `wasActionApplied()` 对未知对象形状默认返回 `true`，应改为 `false` 更安全 |
| L4 | `tests/test_downlink_allocator.cpp:72-81` | 断言只检查前 2 个 action，不验证总数，漏掉多余 action |
| L5 | `tests/qos_harness/run_downlink_matrix.mjs:98` | SFU stdout 被 redirect 到 `process.stderr`，日志混淆。应写入 `process.stdout` |
| L6 | `scripts/run_qos_tests.sh:246` | `set +e` 关闭 errexit 后未及时恢复，后续命令静默失败 |
| L7 | `src/client/lib/qos/controller.d.ts` | 缺少 `activeOverrides: Map` 类型声明，TS 项目引用时无法获得完整类型提示 |

---

## 五、优先处理建议

### 🏆 Top 5 必须优先修复

| 优先级 | 编号 | 问题 | 理由 |
|--------|------|------|------|
| 1 | S3 | NaN/Infinity 守卫 | 恶意客户端可注入 NaN 触发 UB，影响所有 QoS 决策 |
| 2 | H5 | packetsLost 累计值误用 | 导致长时间运行后永久 congested，QoS 完全失效 |
| 3 | S4+S6 | seq 回绕 + 客户端 seq 溢出 | 客户端 + 服务端联动 bug，导致 QoS 数据拒收 |
| 4 | S2 | 目录句柄泄漏 | 长期运行 fd 耗尽，影响整个进程 |
| 5 | H1 | epoll_ctl 失败无处理 | 定时器/事件丢失无声，影响核心事件循环 |

### 📋 快速修复清单（工作量小，收益高）

- [ ] S1: `Consumer::close()` 加 null guard（1 行）
- [ ] S5: `qos-demo.js` 改用 `textContent`（1 行）
- [ ] H6: 所有 browser harness 添加 `rmSync(tmpDir)`（5 处各 1 行）
- [ ] L1: 删除冗余 `else if` 分支（1 行）
- [ ] L5: `run_downlink_matrix.mjs` stdout 写到 stdout（1 行）

---

## 六、总结

| 严重程度 | 数量 | 关键风险 |
|----------|------|----------|
| 🔴 严重 (Critical) | 6 | 空指针崩溃、UB、XSS 注入、数据拒收 |
| 🟠 高 (High) | 11 | 资源泄漏、状态不一致、性能退化 |
| 🟡 中 (Medium) | 14 | 逻辑瑕疵、健壮性、可维护性 |
| 🟢 低 (Low) | 7 | 代码风味 / 测试质量 |
| **合计** | **38** | |

> **建议**：优先处理 Top 5 问题后进行一轮回归测试，再批量处理快速修复清单中的项目。中/低优先级可纳入后续迭代。

---

## 七、补充问题（QoS 设计完整性 / 鲁棒性，9 项）

### Q1. 订阅统计映射策略不可靠，可能把 A 流劣化归因到 B 流

| 属性 | 值 |
|------|-----|
| **文件** | `src/client/lib/qos/downlinkSampler.js` (66-79 行) |
| **类型** | QoS 观测准确性 |
| **严重性** | 🟠 High |

**问题描述**：当前按“第一个未匹配 inbound-rtp”做 best-effort 绑定 consumer，缺乏稳定映射键。多流场景下可能错配，导致后续预算与降级决策失真。  
**修复建议**：建立稳定映射（consumerId/mid/ssrc 显式关联），并在映射失败时上报降级可观测日志。

---

### Q2. `consumerId ↔ producerId` 关系缺少服务端一致性校验

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/QosValidator.cpp`、`src/qos/ProducerDemandAggregator.cpp` |
| **类型** | 输入可信边界 |
| **严重性** | 🟠 High |

**问题描述**：下行快照仅校验 `consumerId/producerId` 非空，未校验是否与房间内真实 Consumer 关系一致；聚合器随后直接信任 `producerId` 聚合供给需求。  
**修复建议**：在 `RoomService` 中对快照条目做房间内拓扑一致性校验，不一致条目应丢弃并计数。

---

### Q3. 未校验 `subscriptions` 中 `consumerId` 唯一性

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/QosValidator.cpp` |
| **类型** | 数据完整性 |
| **严重性** | 🟡 Medium |

**问题描述**：同一 `consumerId` 可在一次 payload 中重复出现，可能导致需求聚合重复计数。  
**修复建议**：解析阶段维护去重集合，重复条目拒绝或仅保留首条并打点。

---

### Q4. 下行快照新鲜度判断主要依赖 `receivedAtMs`，缺少 `tsMs` 合理性约束

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/DownlinkQosRegistry.cpp`、`src/RoomService.cpp` |
| **类型** | 时序鲁棒性 |
| **严重性** | 🟡 Medium |

**问题描述**：当前 stale 逻辑主要基于服务器接收时间；若 payload 时钟异常/重放但 seq 可通过，可能在窗口内污染决策。  
**修复建议**：增加 `tsMs` 漂移窗口检查与跨样本单调约束，异常数据降权或拒绝。

---

### Q5. `downlinkClientStats` 缺少频率限流

| 属性 | 值 |
|------|-----|
| **文件** | `src/SignalingServer.cpp` (579-617 行) |
| **类型** | 控制面 QoS |
| **严重性** | 🟠 High |

**问题描述**：解析通过后直接投递 worker，无 peer 级速率控制。高频上报会放大控制平面负载。  
**修复建议**：增加 peer 级 token bucket / 最小上报间隔门限，并暴露拒绝计数指标。

---

### Q6. `subscriberInputs` 为空时直接返回，供给状态可能无法平滑衰减

| 属性 | 值 |
|------|-----|
| **文件** | `src/RoomService.cpp` (1467 行) |
| **类型** | 状态机完备性 |
| **严重性** | 🟡 Medium |

**问题描述**：当房间内无可用下行快照时直接 return，可能导致 producer 供给状态停留在历史态，恢复/回收策略延迟。  
**修复建议**：即使无新输入也触发一次“零需求推进”或按超时规则推进状态机。

---

### Q7. override 下发缺少 ACK/重传闭环

| 属性 | 值 |
|------|-----|
| **文件** | `src/RoomService.cpp` (`maybeSendTrackQosOverride`) |
| **类型** | 控制可靠性 |
| **严重性** | 🟡 Medium |

**问题描述**：当前仅按签名去重与 TTL 记录，通知丢失后无确认与重试机制。  
**修复建议**：引入轻量 ACK（含 seq）+ 超时重发，或在下一周期携带状态确认。

---

### Q8. 预算分配每轮“从零重算”，缺少切换惩罚与驻留约束

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/SubscriberBudgetAllocator.cpp` |
| **类型** | 体验稳定性 |
| **严重性** | 🟡 Medium |

**问题描述**：当前贪心分配不考虑前态切换成本，易在边界带宽抖动下产生层级翻转（flapping）。  
**修复建议**：引入最小驻留时间/升级冷却窗口/切换惩罚项。

---

### Q9. 下行订阅模型缺少 media kind，执行层可能对非视频 consumer 下发层级动作

| 属性 | 值 |
|------|-----|
| **文件** | `src/qos/DownlinkQosTypes.h`、`src/qos/SubscriberQosController.cpp` |
| **类型** | 模型边界清晰度 |
| **严重性** | 🟡 Medium |

**问题描述**：订阅结构未显式声明 kind，执行阶段对 `kSetLayers` 直接调用 `setPreferredLayers`，边界假设依赖外部约束。  
**修复建议**：在模型中显式携带 kind，或在执行层增加仅视频可下发层级动作的硬校验。
