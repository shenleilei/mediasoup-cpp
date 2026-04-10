
<!-- BEGIN GENERATED UPLINK QOS TEST REPORT -->

## 16. 测试执行报告（自动追加）

### 16.1 实际执行项

| Step | 状态 | 耗时 | 文档映射 | 说明 | 日志 |
|---|---|---:|---|---|---|
| `qos_unit` | `PASS` | `0s` | 基础能力支撑（协议/校验/聚合/override builder） | 服务端 QoS 单元测试 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-211116/qos_unit.log` |

### 16.2 关键观察

- 中断时最后一个已开始但未完成记录的 step：`qos_integration`（服务端 uplink QoS 黑盒集成测试子集）。
- `browser_loopback` 未产出可解析 JSON，无法提取 baseline/impaired/recovered 摘要。

### 16.3 与测试计划 Case 的对应关系

| 文档 Case | 执行结论 | 证据 |
|---|---|---|
| `P2` | 部分直接覆盖 | `QosIntegrationTest.SetQosPolicyNotifiesTargetPeer` 直接覆盖服务端通知链路；若本次有 `policy_update` step 日志，才可进一步认定 client runtime 热更新已执行 |
| `P3` | 部分直接覆盖 | `QosIntegrationTest.SetQosOverrideNotifiesTargetPeer` 直接覆盖服务端 override 下发；client 端是否实际应用仍依赖 `override_force_audio_only` harness |
| `P4` | 依赖单独 harness | 需要 `run.mjs override_force_audio_only` 或等价 browser harness 才能证明 `forceAudioOnly -> enterAudioOnly` 已直接执行 |
| `P5` | 部分覆盖 | `AutomaticQosOverrideClearsWhenQualityRecovers` 覆盖 automatic clear；manual clear 仍需单独 harness/日志 |
| `RM2` | 已直接执行 | `QosIntegrationTest.RoomQosStateAndRoomPressureOverride` |
| `RM3` | 本次无直接覆盖 | 当前 gtest 直接覆盖的是 `poorPeers=2 -> room pressure`；`lostPeers>=1` 仍需单独 room-lost case |
| `RM4` | 依赖单独 harness | 需要 `browser_server_signal.mjs` 或等价 clear case 日志；当前 gtest 不直接证明 room pressure clear |
| `L5` / `L8` | 相关覆盖 | `AutomaticQosOverrideOnPoorQuality`、`AutomaticQosOverrideOnLostQuality` 基于合成 `clientStats` 验证 severe poor/lost 下自动保护，但不是文档表格的一对一参数扫描 |
| `R4` / `J3` / `T5` | 相关覆盖 | 只有在 `browser_loopback.mjs` / `browser_server_signal.mjs` 产出完成日志时，才能说明固定弱网场景下退化/恢复链路可工作；即使通过也不是矩阵一对一执行 |

### 16.4 当前未一对一执行的 Case


### 16.5 结论


<!-- END GENERATED UPLINK QOS TEST REPORT -->
