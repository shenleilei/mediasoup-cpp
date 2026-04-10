# 上行 QoS 验证结果与 LiveKit 对比

> 配套文档：
> - `docs/uplink-qos-design.md`
> - `docs/uplink-qos-implementation-plan.md`
> - `docs/uplink-qos-progress.md`
> 日期：2026-04-10

---

## 1. 目的

这份文档回答 3 个问题：

1. 当前 `mediasoup-cpp` 上行 QoS 到底测了哪些 case
2. 弱网是否**真实生效**，有没有具体数据
3. 相对 LiveKit，目前已经追到哪，仍差在哪

---

## 2. 今日已验证的 case

## 2.1 Client 侧 Jest

已覆盖：

- `signals`
- `stateMachine`
- `planner`
- `probe`
- `sampler`
- `controller`
- `executor`
- `statsProvider`
- `signalChannel`
- `coordinator`
- `factory`
- `peerSession`
- `traceReplay`
- `protocol`

当前统计：

- `15` suites
- `169` tests
- 结果：全绿

执行命令：

```bash
cd src/client
npm run lint
npm test -- --runInBand
```

## 2.2 服务端 QoS 单元测试

已覆盖：

- `QosValidator`
- `QosRegistry`
- `QosAggregator`
- `QosRoomAggregator`
- `QosOverrideBuilder`

执行命令：

```bash
./build/mediasoup_tests --gtest_filter='QosProtocolTest.*:QosValidatorTest.*:QosRegistryTest.*:QosAggregatorTest.*:QosRoomAggregatorTest.*:QosOverrideBuilderTest.*'
```

结果：

- 全绿

## 2.3 服务端 QoS 黑盒集成

已覆盖：

- QoS schema 存储
- `seq` 去重，旧快照不覆盖新快照
- `statsReport` 中的 `qos`
- join 后 `qosPolicy`
- 手动 `setQosPolicy`
- 手动 `setQosOverride`
- 自动 `server_auto_poor`
- 自动 `server_auto_lost`
- 自动 `server_auto_clear`
- `qosConnectionQuality`
- `qosRoomState`
- `room pressure override`

当前统计：

- `QosIntegrationTest`: `22` 个 case
- `QosRecordingTest`: `7` 个 case
- 合计：`29` 个 case

执行命令：

```bash
./build/mediasoup_qos_integration_tests
```

结果：

- 全绿

## 2.4 Node 双端 harness

脚本：

- `tests/qos_harness/run.mjs`
- `tests/qos_harness/browser_loopback.mjs`
- `tests/qos_harness/browser_server_signal.mjs`

已跑通：

- `quick`
- `browser_loopback`
- `browser_server_signal`

结果：

- 全绿

---

## 3. 弱网是否真的生效

结论：

- **是，确实生效**
- 不是 mock 指标，而是 `chromium-headless + tc netem` 下跑出来的真实 sender stats

## 3.1 测试方法

使用：

- `chromium-headless`
- loopback `RTCPeerConnection`
- Linux `tc netem`

当前注入的网络损伤：

- `delay 120ms`
- `loss 10%`

脚本：

```bash
node tests/qos_harness/browser_loopback.mjs
```

## 3.2 实测数据

以下数据来自实际运行输出。

### 基线

- `state`: `early_warning`
- `level`: `1`
- `sendBitrateBps`: `74-86 kbps`
- `targetBitrateBps`: `100-120 kbps`
- `rttMs`: `1.00-1.19 ms`
- `jitterMs`: `0.42-0.59 ms`
- `lossRate`: `0`

### 注入弱网后

- `state`: 仍处于 `early_warning`，但网络指标显著恶化
- `sendBitrateBps`: `37.9-42.2 kbps`
- `targetBitrateBps`: `25.3-44.0 kbps`
- `rttMs`: `240.4-240.7 ms`
- `jitterMs`: `5.8-27.9 ms`
- `lossRate`: 峰值约 `23.1%`
- trace 中出现 QoS 动作，说明 controller 对退化做了响应

### 清除弱网后

- `sendBitrateBps`: `59.3-72.6 kbps`
- `targetBitrateBps`: `74.8-100 kbps`
- `rttMs`: 回落到约 `1 ms`
- `jitterMs`: 回落到 `0.32-3.4 ms`
- `lossRate`: 回落到 `0`

## 3.3 为什么这些数据能证明“改动有效”

因为链路里同时出现了两类变化：

1. **网络层/传输层退化**
   - RTT 明显抬升
   - jitter 明显抬升
   - 丢包升高
2. **QoS 控制层响应**
   - target bitrate 下探
   - actual send bitrate 下探
   - trace 中记录了 planner/action 执行
   - 清除弱网后，关键指标回升

如果只是网络变化而没有 QoS 变化，无法证明改动有效。  
如果只是 QoS 动作变化而没有真实 sender stats 变化，也无法证明改动有效。

这里两者同时存在，所以可以证明：

- 弱网注入确实触达真实 WebRTC sender
- QoS 控制器确实在读这些信号并响应
- 恢复路径确实存在

---

## 4. 公开/权威依据

下面这些不是我们自己定义的口径，而是标准或官方文档里已有的能力与指标：

- `tc netem` 官方 Linux 手册，说明它支持模拟 `delay`、`loss` 等网络损伤：
  - https://www.man7.org/linux/man-pages/man8/tc-netem.8.html

- W3C WebRTC Stats 规范定义了我们读取的关键 sender / remote-inbound 指标，包括：
  - `targetBitrate`
  - `roundTripTime`
  - `jitter`
  - `packetsLost`
  - `qualityLimitationReason`
  - https://www.w3.org/TR/webrtc-stats/

- LiveKit 官方文档对能力边界的说明：
  - `adaptiveStream`：根据 UI 大小/可见性自动协调订阅层  
    https://docs.livekit.io/transport/media/subscribe/
  - `dynacast`：动态暂停未被消费的视频层，降低发布侧 CPU 和带宽  
    https://docs.livekit.io/home/client/tracks/advanced/
  - LiveKit 的 client protocol 本身就是客户端与服务端质量/订阅控制联动的一部分  
    https://docs.livekit.io/reference/internals/client-protocol/
  - LiveKit 强调 server 具备 QoS controls  
    https://docs.livekit.io/intro/about/

---

## 5. 与 LiveKit 的对比

## 5.1 对比口径

这里的对比分两类：

- **实测验证**
  - 我们对 `mediasoup-cpp` 当前实现做了真实运行验证
- **能力对比**
  - 对 LiveKit 使用官方文档和本地源码阅读做能力对照

重要说明：

- 今天**没有**做一套完全 apples-to-apples 的“同机、同网络、同媒体、同策略” LiveKit 性能 benchmark
- 所以下面的“对比数据”是：
  - 我们的实测数据
  - LiveKit 的官方能力与源码事实

## 5.2 能力矩阵

| 维度 | 当前 mediasoup-cpp | LiveKit | 结论 |
|------|--------------------|---------|------|
| 端上独立 QoS 子系统 | ✅ | ✅ | 已追上骨架 |
| 显式状态机 | ✅ | ✅ | 已有 |
| source-specific planner | ✅ `camera/audio/screenshare` | ✅ | 已有 |
| peer/session 协调 | ✅ 基础版 | ✅ 更成熟 | 还可加强 |
| probe/recheck 恢复 | ✅ 基础版 | ✅ 更成熟 | 还可加强 |
| 服务端 slow loop | ✅ | ✅ | 已有 |
| `qosPolicy`/`qosOverride` | ✅ | ✅（以不同协议形态体现） | 已有 |
| `qosConnectionQuality` 语义 | ✅ | ✅ `ConnectionQuality` | 已追上基础语义 |
| room-level QoS state | ✅ | ⚠️ 非完全同形态，但有更强服务端联动 | 我们已有基础 |
| 自动保护 / 自动清除 | ✅ | ✅ | 已有 |
| `dynacast` | ❌ | ✅ 官方支持 | 关键差距 |
| `subscriber-driven uplink` | ❌ | ✅ `SubscribedQualityUpdate` 等联动 | 关键差距 |
| `adaptiveStream` | ❌ | ✅ 官方支持 | 关键差距 |
| server allocator / capacity coupling | ⚠️ 基础 room pressure | ✅ 更强 | 仍落后 |
| browser 弱网 E2E | ✅ 已跑通 | LiveKit 官方能力成熟，但今天未做同机对打 | 我们已有真实验证 |

## 5.3 当前位置判断

如果只看“端上上行弱网 QoS”：

- 之前：我们只有底层原语，明显落后
- 现在：已经有完整的 client+server QoS 体系，并且有真实弱网验证

如果看“整体 LiveKit QoS 完整度”：

- 仍落后于 LiveKit
- 主要差距集中在：
  - `dynacast`
  - `subscriber-driven uplink`
  - 更强的 server-side allocator / capacity 协同

也就是说：

- **不是还在起步阶段**
- **已经进入可持续追赶阶段**
- **但还没有追平 LiveKit**

---

## 6. 效果评价

如果只问“今天改动有没有效果”：

- 有，而且是显著有效

如果问“今天有没有达到 LiveKit 的完整水平”：

- 没有

如果问“值不值得继续沿这个方向做”：

- 值得
- 因为现在已经有：
  - 真实弱网数据
  - 真实双端链路
  - 稳定测试体系
  - 产品化质量语义

后续继续补 `dynacast / subscriber-driven uplink / server allocation`，就是在现有地基上做增强，而不是推倒重来。

---

## 7. 当前最重要的待办

优先级建议：

1. `subscriber-driven uplink`
2. `dynacast`
3. 更强的 server-side capacity / allocation 联动

这是距离 LiveKit 当前水平最关键的剩余差距。
