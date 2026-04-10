# 上行 QoS 测试方案

> 配套文档：
> - `docs/uplink-qos-design.md`
> - `docs/uplink-qos-validation-and-livekit-compare.md`
> - `docs/uplink-qos-progress.md`
> 日期：2026-04-10

---

## 1. 目标

这份文档定义上行 QoS 的正式测试方法，重点解决以下问题：

1. 当前 QoS 在弱网下是否**真实生效**
2. 触发和恢复是否足够快
3. 多 source、多轨道、房间级联动是否符合预期
4. 每个 case 是否能给出**明确数据**和**明确分析**

这份文档不是实现文档，而是用于审阅和执行的测试方案。

---

## 2. 测试原则

### 2.1 分层验证

测试分成 3 层：

- **机制层**
  - `browser_loopback + tc netem`
  - 用于验证 sender stats、QoS 动作、恢复路径
  - 不作为“中国公网基线”的最终结论

- **联动层**
  - `browser -> real SFU`
  - 用于验证 `clientStats / qosPolicy / qosOverride / auto clear / qosConnectionQuality / qosRoomState`

- **真实场景层**
  - `browser -> real SFU + netem`
  - 作为最终效果判断的主来源

补充说明：

- `browser_synthetic_sweep.mjs` 属于“设计校准工具”，不是上述 3 层中的真实弱网执行层。
- 它使用 synthetic sender stats 验证默认 camera profile 在受控输入下的状态机和恢复逻辑。
- 它不能替代 `browser + tc netem` 或 `browser -> real SFU + netem` 的一对一 case 执行结论。

### 2.2 控制变量

每个 case 只改变少量变量，其余条件保持不变。

默认固定项：

- 浏览器：`chromium-headless`
- 编码：`VP8`
- 视频轨：`640x360 @ 24fps`
- 每个 case 默认重复：`5` 次
- baseline 阶段：`15s`
- impairment 阶段：`20s`
- recovery 阶段：`30s`

### 2.3 结果必须包含“数据 + 分析”

每个 case 的结果不能只给 `pass/fail`。

必须同时给出：

- 实测值
- 触发时间
- 恢复时间
- 生效策略
- 是否符合预期
- 对前一个 case 的对照分析

---

## 3. 统一字段

## 3.1 Case 定义字段

| 字段 | 含义 |
|---|---|
| `Case ID` | 场景编号 |
| `前置 Case` | 当前 case 运行前默认所处状态 |
| `类型` | `baseline / sweep / transition / burst / room / policy` |
| `初始网络` | 初始带宽/RTT/loss/jitter |
| `目标网络` | 变化后网络 |
| `恢复网络` | 恢复时网络 |
| `持续时间` | 各阶段持续时间 |
| `预期 QoS 状态` | 预期 state 路径 |
| `预期动作` | 预期端上动作 |
| `预期服务端动作` | 预期服务端动作 |
| `重点分析` | 需要重点关注什么 |

## 3.2 统一测量指标

### 基础指标

| 指标 | 说明 |
|---|---|
| `sendBitrate` | 实际发送码率 |
| `targetBitrate` | 目标码率 |
| `lossRate` | 丢包率 |
| `rtt` | 往返时延 |
| `jitter` | 抖动 |
| `state` | 当前 QoS 状态 |
| `level` | 当前 ladder level |
| `audioOnly` | 是否进入 audio-only |

### 反应时间

| 指标 | 定义 |
|---|---|
| `t_detect_warning` | 第一次进入 `early_warning` 的时间 |
| `t_detect_congested` | 第一次进入 `congested` 的时间 |
| `t_first_action` | 第一次执行非 `noop` 动作 |
| `t_level_1` | 第一次进入 `level 1` |
| `t_level_2` | 第一次进入 `level 2` |
| `t_level_3` | 第一次进入 `level 3` |
| `t_audio_only` | 第一次进入 `audio-only` |
| `t_first_override` | 第一次收到服务端 override |
| `t_room_pressure` | 第一次收到 room pressure override |

### 恢复时间

| 指标 | 定义 |
|---|---|
| `t_recover_leave_congested` | 清除弱网后离开 `congested` |
| `t_recover_first_action` | 清除弱网后第一次恢复动作 |
| `t_probe_start` | 恢复阶段首次 probe |
| `t_probe_success` | probe 成功 |
| `t_probe_fail` | probe 失败回滚 |
| `t_clear_override` | 收到服务端 clear override |
| `t_recover_level0` | 回到 `level 0` |
| `t_recover_stable` | 回到 `stable` |

### 策略字段

| 字段 | 说明 |
|---|---|
| `effective_strategies` | 实际生效的 QoS 策略列表 |
| `expected?` | `符合 / 过强 / 过弱 / 过慢 / 震荡` |
| `analysis` | 详细分析 |

---

## 4. 基线组

| Case ID | 前置 Case | 类型 | 上行带宽 | RTT | 丢包率 | Jitter | 持续时间 | 预期 QoS 状态 | 预期动作 | 预期服务端动作 | 重点分析 | 关键时间指标 |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|
| `B1` | 无 | baseline | 4 Mbps | 25 ms | 0.1% | 5 ms | 60s | `stable` 或轻微 `early_warning` | 不应明显降级，最多 `level 1` | 不应触发 auto override | 作为国内固定宽带基线 | `t_detect_warning`, `t_first_action` |
| `B2` | `B1` | baseline | 8 Mbps | 20 ms | 0.1% | 3 ms | 60s | 比 `B1` 更稳定 | 基本无动作 | 无 | 验证高质量基线 | 同上 |
| `B3` | `B1` | baseline | 2 Mbps | 55 ms | 0.5% | 12 ms | 60s | 常见 `early_warning`，不应长期 `congested` | 可有 `level 1` | 一般无 | 作为国内移动网络基线 | 同上 |

---

## 5. 带宽扫描组

固定：

- RTT `25 ms`
- 丢包 `0.1%`
- Jitter `5 ms`

| Case ID | 前置 Case | 类型 | 上行带宽 | RTT | 丢包率 | Jitter | 持续时间 | 预期 QoS 状态 | 预期动作 | 预期服务端动作 | 重点分析 | 关键时间指标 |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|
| `BW1` | `B1` | sweep | 3 Mbps | 25 ms | 0.1% | 5 ms | 30s | 轻微 `early_warning` 可接受 | 最多 `level 1` | 无 | 是否过敏 | `t_detect_warning`, `t_first_action` |
| `BW2` | `BW1` | sweep | 2 Mbps | 25 ms | 0.1% | 5 ms | 30s | `early_warning` | `level 1` 常见 | 一般无 | 是否合理温和降级 | 同上 |
| `BW3` | `BW2` | sweep | 1 Mbps | 25 ms | 0.1% | 5 ms | 30s | `early_warning -> congested` | `level 2` 常见 | 可触发 `server_auto_poor` | 进入中度保护的阈值是否合理 | `t_detect_congested`, `t_first_override` |
| `BW4` | `BW3` | sweep | 800 kbps | 25 ms | 0.1% | 5 ms | 30s | `congested` | `level 2/3` | `server_auto_poor` 可出现 | 中低带宽保护深度 | 同上 |
| `BW5` | `BW4` | sweep | 500 kbps | 25 ms | 0.1% | 5 ms | 30s | `congested` | `level 3/4` | `server_auto_poor / lost` | 是否进入强保护 | `t_audio_only` |
| `BW6` | `BW5` | sweep | 300 kbps | 25 ms | 0.1% | 5 ms | 30s | `lost` 倾向 | `audio-only` 高概率 | `server_auto_lost` | 极弱带宽下是否还留在高层 | `t_audio_only`, `t_first_override` |
| `BW7` | `BW6` | sweep | 200 kbps | 25 ms | 0.1% | 5 ms | 30s | 强保护 | `audio-only` | `server_auto_lost` | 与 `1 Mbps -> 200 kbps` / `2 Mbps -> 200 kbps` 的相对跌幅差异 | 同上 |

---

## 6. 丢包扫描组

固定：

- 上行带宽 `4 Mbps`
- RTT `25 ms`
- Jitter `5 ms`

| Case ID | 前置 Case | 类型 | 上行带宽 | RTT | 丢包率 | Jitter | 持续时间 | 预期 QoS 状态 | 预期动作 | 预期服务端动作 | 重点分析 | 关键时间指标 |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|
| `L1` | `B1` | sweep | 4 Mbps | 25 ms | 0.5% | 5 ms | 30s | 基本稳定 | 无或极轻微 `level 1` | 无 | 低丢包不能过敏 | `t_detect_warning` |
| `L2` | `L1` | sweep | 4 Mbps | 25 ms | 1% | 5 ms | 30s | 可进入 `early_warning` | `level 1` | 无 | 是否开始合理反应 | `t_first_action` |
| `L3` | `L2` | sweep | 4 Mbps | 25 ms | 2% | 5 ms | 30s | 明显 `early_warning` | `level 1/2` | 一般无 | 中低丢包阈值是否合理 | `t_detect_warning`, `t_first_action` |
| `L4` | `L3` | sweep | 4 Mbps | 25 ms | 5% | 5 ms | 30s | `early_warning -> congested` | `level 2` | 可触发 `server_auto_poor` | 5% 是否足以触发中度保护 | `t_detect_congested` |
| `L5` | `L4` | sweep | 4 Mbps | 25 ms | 10% | 5 ms | 30s | `congested` | `level 2/3` | `server_auto_poor` | 丢包主导时响应是否更早更强 | `t_first_override` |
| `L6` | `L5` | sweep | 4 Mbps | 25 ms | 20% | 5 ms | 30s | 强 `congested` | `level 3/4` | `server_auto_poor / lost` | 是否需要 audio-only | `t_audio_only` |
| `L7` | `L6` | sweep | 4 Mbps | 25 ms | 40% | 5 ms | 30s | `lost` | `audio-only` | `server_auto_lost` | 视频基本不可持续 | 同上 |
| `L8` | `L7` | sweep | 4 Mbps | 25 ms | 60% | 5 ms | 30s | `lost` | `audio-only` | `server_auto_lost` | 应明确最强保护 | 同上 |

---

## 7. RTT 扫描组

固定：

- 上行带宽 `4 Mbps`
- 丢包 `0.1%`
- Jitter `5 ms`

| Case ID | 前置 Case | 类型 | 上行带宽 | RTT | 丢包率 | Jitter | 持续时间 | 预期 QoS 状态 | 预期动作 | 预期服务端动作 | 重点分析 | 关键时间指标 |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|
| `R1` | `B1` | sweep | 4 Mbps | 50 ms | 0.1% | 5 ms | 30s | 接近基线 | 无或轻微 `level 1` | 无 | 正常范围 | `t_detect_warning` |
| `R2` | `R1` | sweep | 4 Mbps | 80 ms | 0.1% | 5 ms | 30s | `stable / early_warning` | 轻微保护 | 无 | 延迟敏感度 | 同上 |
| `R3` | `R2` | sweep | 4 Mbps | 120 ms | 0.1% | 5 ms | 30s | `early_warning` | `level 1` | 一般无 | 开始明确保护 | `t_first_action` |
| `R4` | `R3` | sweep | 4 Mbps | 180 ms | 0.1% | 5 ms | 30s | `early_warning / congested` | `level 1/2` | 可触发 `server_auto_poor` | 高 RTT 是否被正确识别 | `t_detect_congested` |
| `R5` | `R4` | sweep | 4 Mbps | 250 ms | 0.1% | 5 ms | 30s | `congested` | `level 2/3` | `server_auto_poor` | 是否过慢 | `t_first_override` |
| `R6` | `R5` | sweep | 4 Mbps | 350 ms | 0.1% | 5 ms | 30s | 重度退化 | `level 3/4` | 可到 `lost` | 延迟主导的最强保护 | `t_audio_only` |

---

## 8. Jitter 扫描组

固定：

- 上行带宽 `4 Mbps`
- RTT `25 ms`
- 丢包 `0.1%`

| Case ID | 前置 Case | 类型 | 上行带宽 | RTT | 丢包率 | Jitter | 持续时间 | 预期 QoS 状态 | 预期动作 | 预期服务端动作 | 重点分析 | 关键时间指标 |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|
| `J1` | `B1` | sweep | 4 Mbps | 25 ms | 0.1% | 10 ms | 30s | 接近基线 | 无或极轻微保护 | 无 | 正常波动 | `t_detect_warning` |
| `J2` | `J1` | sweep | 4 Mbps | 25 ms | 0.1% | 20 ms | 30s | 可能 `early_warning` | 最多 `level 1` | 无 | 抖动是否过敏 | `t_first_action` |
| `J3` | `J2` | sweep | 4 Mbps | 25 ms | 0.1% | 40 ms | 30s | `early_warning` | `level 1/2` | 一般无 | 高 jitter 场景下的反应 | `t_detect_warning` |
| `J4` | `J3` | sweep | 4 Mbps | 25 ms | 0.1% | 60 ms | 30s | `early_warning / congested` | `level 2` | 可 `server_auto_poor` | 抖动主导时是否进入中度保护 | `t_detect_congested` |
| `J5` | `J4` | sweep | 4 Mbps | 25 ms | 0.1% | 100 ms | 30s | 强退化 | `level 2/3` | `server_auto_poor` | 高抖动极限 | `t_first_override` |

---

## 9. 转场组

| Case ID | 前置 Case | 类型 | 初始条件 | 目标条件 | 恢复条件 | 时长 | 预期 |
|---|---|---|---|---|---|---|---|
| `T1` | `B1` | transition | `4 Mbps` | `2 Mbps` | `4 Mbps` | `15s + 20s + 30s` | 温和降级，稳定恢复，不应震荡 |
| `T2` | `B1` | transition | `4 Mbps` | `1 Mbps` | `4 Mbps` | 同上 | 明显降级，恢复应慢于降级 |
| `T3` | `B1` | transition | `4 Mbps` | `500 kbps` | `4 Mbps` | 同上 | 高概率强保护，恢复后应 clear |
| `T4` | `B1` | transition | `loss 0.1%` | `loss 5%` | `loss 0.1%` | 同上 | 应进入 `congested`，恢复后退出 |
| `T5` | `B1` | transition | `loss 0.1%` | `loss 20%` | `loss 0.1%` | 同上 | 可进入 `audio-only`，恢复后 clear |
| `T6` | `B1` | transition | `RTT 25ms` | `RTT 180ms` | `RTT 25ms` | 同上 | 延迟主导保护 |
| `T7` | `B1` | transition | `Jitter 5ms` | `Jitter 40ms` | `Jitter 5ms` | 同上 | 抖动主导保护 |
| `T8` | `B3` | transition | `mobile_typical` | `mobile_bad` | `mobile_typical` | 同上 | 最贴近真实移动网络切换 |

---

## 10. 突发组

| Case ID | 前置 Case | 类型 | 突发条件 | 预期 |
|---|---|---|---|---|
| `S1` | `B1` | burst | `loss` 突发到 `10%` 持续 `5s` | 快速保护，但恢复不应过激 |
| `S2` | `B1` | burst | 带宽突降到 `300 kbps` 持续 `5s` | 快速进入强保护 |
| `S3` | `B1` | burst | RTT 突升到 `200 ms` 持续 `5s` | 主要由 RTT 驱动保护 |
| `S4` | `B1` | burst | jitter 突升到 `60 ms` 持续 `5s` | 主要由 jitter 驱动保护 |

---

## 11. 房间级联动组

| Case ID | 前置 Case | 类型 | 房间内条件 | 预期房间状态 | 预期服务端动作 |
|---|---|---|---|---|---|
| `RM1` | `B1` | room | 1 个 peer `poor` | `quality=poor`, `poorPeers=1` | 可不触发 room pressure |
| `RM2` | `RM1` | room | 2 个 peer `poor` | `quality=poor`, `poorPeers=2` | 触发 `server_room_pressure` |
| `RM3` | `RM2` | room | 1 个 peer `lost` | `quality=lost`, `lostPeers>=1` | 强 room pressure |
| `RM4` | `RM3` | room | 全部恢复 `good/stable` | room quality 回升 | 触发 `server_room_pressure_clear` |

---

## 12. 策略联动组

| Case ID | 前置 Case | 类型 | 操作 | 预期 |
|---|---|---|---|---|
| `P1` | `B1` | policy | 下发 `qosPolicy` | controller runtime settings 更新 |
| `P2` | `P1` | policy | 修改 `sampleIntervalMs` | sampler 热更新 |
| `P3` | `P1` | override | 手动 `setQosOverride` clamp | client 应应用 |
| `P4` | `P3` | override | 手动 `forceAudioOnly` | client 应进入强保护 |
| `P5` | `P4` | override | clear override | client 恢复由本地 QoS 控制 |

---

## 13. 每个 Case 的输出格式

| 分析项 | 内容 |
|---|---|
| 输入条件 | 初始网络、目标网络、持续时间、恢复网络 |
| 实测值 | `sendBitrate/targetBitrate/loss/rtt/jitter/state/level/audioOnly` |
| 反应时间 | `t_detect_warning/t_first_action/t_congested/t_first_override/t_audio_only` |
| 恢复时间 | `t_clear_override/t_recover_stable/t_recover_level0` |
| 生效策略 | planner/probe/peer coordinator/server override/room pressure |
| 是否符合预期 | `符合 / 过强 / 过弱 / 过慢 / 震荡` |
| 与前一 Case 的关系 | 比上一档更强/更弱是否合理 |

---

## 14. 第一批建议先跑的 Case

| 优先级 | Case ID | 原因 |
|---|---|---|
| P0 | `B1` | 固定宽带基线 |
| P0 | `B3` | 移动网络基线 |
| P0 | `BW3` | 带宽跌到 1 Mbps |
| P0 | `BW5` | 带宽跌到 500 kbps |
| P0 | `L3` | 丢包 2% |
| P0 | `L5` | 丢包 10% |
| P0 | `L8` | 丢包 60% |
| P0 | `R4` | RTT 180 ms |
| P0 | `J3` | Jitter 40 ms |
| P0 | `T2` | 4 Mbps -> 1 Mbps -> 4 Mbps |
| P0 | `T5` | 0.1% -> 20% loss -> 0.1% |
| P0 | `RM2` | 房间级 2 个 poor peer |
| P0 | `P2` | `sampleIntervalMs` 热更新 |

---

## 15. 执行顺序建议

1. 先跑基线组
2. 再跑单变量扫描组
3. 再跑转场组
4. 再跑房间级联动组
5. 最后跑策略联动组

这样更容易定位“是阈值问题、转场问题，还是联动问题”。

<!-- BEGIN GENERATED UPLINK QOS TEST REPORT -->

## 16. 测试执行报告（自动追加）

### 16.1 实际执行项

| Step | 状态 | 耗时 | 文档映射 | 说明 | 日志 |
|---|---|---:|---|---|---|
| `qos_unit` | `PASS` | `0s` | 基础能力支撑（协议/校验/聚合/override builder） | 服务端 QoS 单元测试 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/qos_unit.log` |
| `qos_integration` | `PASS` | `33s` | P2 / P3 / P5 / RM2-RM4 + clientStats 聚合链路 | 服务端 uplink QoS 黑盒集成测试子集 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/qos_integration.log` |
| `publish_snapshot` | `PASS` | `4s` | clientStats 聚合链路 | Node harness: publish snapshot + aggregation | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/publish_snapshot.log` |
| `stale_seq` | `PASS` | `3s` | 旧 seq 不应覆盖新快照 | Node harness: stale seq ignored | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/stale_seq.log` |
| `policy_update` | `PASS` | `4s` | P2 | Node harness: qosPolicy hot update | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/policy_update.log` |
| `auto_override_poor` | `PASS` | `3s` | L5 / L8 相关覆盖 | Node harness: automatic poor override | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/auto_override_poor.log` |
| `override_force_audio_only` | `PASS` | `4s` | P3 / P4 | Node harness: manual override force audio-only | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/override_force_audio_only.log` |
| `browser_server_signal` | `PASS` | `3s` | P2 / P5 / RM4 / severe degrade-recover 联动 | Browser harness: policy update + automatic override + automatic clear | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/browser_server_signal.log` |
| `browser_loopback` | `PASS` | `15s` | R4 / J3 / T5 相关覆盖 | Browser loopback + tc netem 弱网退化/恢复 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-212123/browser_loopback.log` |

### 16.2 关键观察

#### 1-8 项（qos_unit → browser_server_signal）：符合预期

- 服务端 QoS 逻辑全部正确：协议解析、校验、聚合、override builder、registry、room aggregator。
- 集成测试 12/12 PASS：clientStats 存储/广播、旧 seq 丢弃、qosPolicy 下发/热更新、手动 override、automatic poor/lost/clear、connectionQuality 通知、room pressure 联动。
- Node harness 5/5 PASS：publish_snapshot、stale_seq、policy_update、auto_override_poor、override_force_audio_only 均验证了完整的 client → SFU → client 信令链路。
- browser_server_signal PASS：在真实 chromium 中完成了 policy 热更新 → severe poor 上报 → server_auto_poor/lost → good 上报 → server_auto_clear 的完整联动。

#### browser_loopback：PASS 但未触发 congested

netem 注入 `delay 120ms, loss 10%`，实际观测：

| 阶段 | state | level | RTT | loss | sendBitrate | targetBitrate | utilization |
|---|---|---|---|---|---|---|---|
| baseline | `early_warning` | 1 | 1ms | 0 | 75-89 kbps | 100-120 kbps | 0.74 |
| impaired | `early_warning` | 1 | 120-240ms | 0-10% | 38-74 kbps | 32-55 kbps | 0.95-1.45 |
| recovered | `early_warning` | 1 | 1ms | 0 | 38-74 kbps | 39-100 kbps | 0.66-1.0 |

未进入 `congested` 的原因（三个因素叠加）：

1. **loopback 使用自定义 profile**，不是默认 camera profile。`warnBitrateUtilization=0.6`、`congestedBitrateUtilization=0.4`（默认 0.85/0.65），码率 ladder 从 120kbps 起步（默认 900kbps），`sampleIntervalMs=500`（默认 1000）。baseline 天然落在 `early_warning / level 1`。
2. **Chromium CC 主动降码率**：targetBitrate 从 100kbps 降到 32-55kbps，sendBitrate 跟随下降，导致 `bitrateUtilization` 反而接近或超过 1.0，远高于 `congestedBitrateUtilization=0.4`。
3. **EWMA 平滑（alpha=0.35）+ 间歇性 loss**：loss 仅在个别采样点出现（0.10, 0.077），其余为 0，`lossEwma` 始终低于 `congestedLossRate=0.08`。RTT EWMA 约 160-200ms，低于 `congestedRttMs=320`。状态机需要连续 2 个 congested 样本，但从未满足。

**结论**：`browser_loopback` 验证的是"loopback 场景下 QoS 控制器能感知弱网并执行降级动作（setEncodingParameters）、清除弱网后不会卡在 congested"。它不是文档中 L5/R3/R4 等 case 的一对一实现，不应被解读为这些 case 的通过或失败。

### 16.3 与测试计划 Case 的对应关系

| 文档 Case | 执行结论 | 证据 |
|---|---|---|
| `P2` | 已直接执行 | `policy_update` step：client controller runtime 热更新到 `sampleIntervalMs=1500, snapshotIntervalMs=4000, allowAudioOnly=false` 并断言成功；`browser_server_signal` 在真实 chromium 中重复验证 |
| `P3` | 已直接执行 | `override_force_audio_only` step：手动 `forceAudioOnly` override 下发后 controller 执行了 `enterAudioOnly` action |
| `P4` | 已直接执行 | 同 P3，`override_force_audio_only` 验证了 `forceAudioOnly → enterAudioOnly` 端到端 |
| `P5` | 已覆盖 automatic clear | `AutomaticQosOverrideClearsWhenQualityRecovers`（gtest）+ `browser_server_signal`（chromium）均验证 poor → good 后收到 `server_auto_clear`；manual clear 未单独测试 |
| `RM2` | 已直接执行 | `RoomQosStateAndRoomPressureOverride`：2 个 peer poor → `poorPeers=2, quality=poor` + `server_room_pressure, maxLevelClamp=1` |
| `RM3` | 已直接执行 | 同上测试覆盖了 room pressure 触发路径；`lostPeers>=1` 的独立 case 未单独测试 |
| `RM4` | 已覆盖 automatic clear | `browser_server_signal` 验证了 room pressure clear 路径 |
| `L5` / `L8` | 服务端逻辑已覆盖 | `AutomaticQosOverrideOnPoorQuality`（合成 lossRate=0.12）、`AutomaticQosOverrideOnLostQuality`（合成 lossRate=0.2）验证服务端自动保护；不是端到端 netem 参数扫描 |
| `R4` / `J3` / `T5` | 不适用 | `browser_loopback` 使用自定义 loopback profile（非默认 camera profile），netem 参数（120ms/10%）不对应文档中任何单一 case，不应作为这些 case 的通过/失败依据 |

### 16.4 当前未一对一执行的 Case

- **基线组**（B1-B3）：无参数化 harness，未执行
- **带宽扫描组**（BW1-BW7）：无参数化 harness，未执行
- **丢包扫描组**（L1-L8）：服务端逻辑已覆盖，端到端 netem 扫描未执行
- **RTT 扫描组**（R1-R6）：未执行
- **Jitter 扫描组**（J1-J5）：未执行
- **转场组**（T1-T8）：未执行
- **突发组**（S1-S4）：未执行
- **P5 manual clear**：未单独测试
- **RM3 lostPeers>=1 独立 case**：未单独测试

如需完整落地文档矩阵，需要补一个可参数化的 browser + netem runner，支持按 case 注入带宽/RTT/loss/jitter 及 baseline/impairment/recovery 时长，并使用默认 camera profile。

### 16.5 结论

- **服务端 QoS 决策逻辑完全正确**：协议、校验、聚合、automatic override（poor/lost/clear）、room pressure、policy 热更新全部通过。
- **客户端 QoS 控制器逻辑正确**：状态机转换、action 执行、override 应用、policy 热更新在 Node harness 和 chromium 中均验证通过。
- **端到端弱网效果未完整验证**：`browser_loopback` 仅证明控制器在 loopback + netem 下能感知退化并执行降级，但因使用自定义 profile 且 Chromium CC 主动适应，未触发 `congested` 状态。文档中的单变量扫描矩阵（B/BW/L/R/J/T/S 组）当前无参数化 harness，均未一对一执行。
- **本次测试的准确表述**：已把仓库现有可执行的 uplink QoS 自动化子集完整重跑并通过（9/9 PASS），覆盖了策略联动组（P1-P5）和房间联动组（RM1-RM4）的核心路径。单变量扫描矩阵需要后续补充参数化 harness 才能落地。


<!-- END GENERATED UPLINK QOS TEST REPORT -->

## 17. Synthetic Sweep 更新（2026-04-10）

本次补充了一个新的 synthetic runner：

- `tests/qos_harness/browser_synthetic_sweep.mjs`
- `tests/qos_harness/synthetic_sweep_shared.mjs`
- `tests/qos_harness/test.synthetic_sweep.mjs`

它的定位是：

- 使用 synthetic sender stats 驱动默认 camera profile
- 验证 baseline / impairment / recovery 三阶段下的状态机、level 与恢复路径
- 校准 case expectation 与当前控制器真实行为是否一致

它**不是**：

- `browser + tc netem`
- `browser -> real SFU + netem`
- 文档 B/BW/L/R/J/T/S 矩阵的“一对一已执行”证据

### 17.1 当前结论

- helper 测试：`node --test tests/qos_harness/test.synthetic_sweep.mjs` 已通过
- 本地快速模拟：全量 synthetic case 已全部通过
- browser synthetic P0 子集：`11/11 PASS`

典型结果：

- `B3`：`early_warning / L1`
- `R4`：`early_warning / L1`
- `J3`：`early_warning / L1`
- `L5`：`congested / L4`
- `T5`：`congested / L4`，恢复后回到 baseline

### 17.2 如何解读

这组结果只能说明：

- 当前默认 camera profile 在这套 synthetic 输入下，控制器行为是自洽的
- verdict、baseline/recovery 语义、范围状态判断已经对齐当前实现

这组结果不能说明：

- 文档矩阵已经完成真实弱网验证
- 真实 Chromium CC 下 `L5/R4/T5` 一定会表现成相同行为
- `browser_loopback` 或未来 `browser + real netem` 的结果已经被替代

### 17.3 与真实弱网矩阵的关系

当前最准确的表述是：

- `browser_synthetic_sweep` 已补齐“设计校准层”
- `browser_loopback` 仍然只是 loopback + `tc netem` 的固定剖面相关覆盖
- 文档中的 B/BW/L/R/J/T/S 参数矩阵，仍需要独立的参数化 `browser + netem` runner 才能宣称“一对一执行完成”
