
<!-- BEGIN GENERATED UPLINK QOS TEST REPORT -->

## 16. 测试执行报告（自动追加）

### 16.1 实际执行项

| Step | 状态 | 耗时 | 文档映射 | 说明 | 日志 |
|---|---|---:|---|---|---|
| `qos_unit` | `PASS` | `0s` | 基础能力支撑（协议/校验/聚合/override builder） | 服务端 QoS 单元测试 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-190805/qos_unit.log` |
| `qos_integration` | `PASS` | `34s` | P2 / P3 / P5 / RM2-RM4 + clientStats 聚合链路 | 服务端 uplink QoS 黑盒集成测试子集 | `/root/mediasoup-cpp/tests/qos_harness/artifacts/uplink-qos-test-plan-20260410-190805/qos_integration.log` |

### 16.2 关键观察

- `browser_loopback` 未产出可解析 JSON，无法提取 baseline/impaired/recovered 摘要。

### 16.3 与测试计划 Case 的对应关系

| 文档 Case | 执行结论 | 证据 |
|---|---|---|
| `P2` | 已直接执行 | `run.mjs policy_update` + `QosIntegrationTest.SetQosPolicyNotifiesTargetPeer` |
| `P3` | 已直接执行 | `run.mjs override_force_audio_only` + `QosIntegrationTest.SetQosOverrideNotifiesTargetPeer` |
| `P4` | 已直接执行 | `run.mjs override_force_audio_only` 验证 `forceAudioOnly` 触发 `enterAudioOnly` |
| `P5` | 部分覆盖 | `browser_server_signal.mjs` + `AutomaticQosOverrideClearsWhenQualityRecovers` 覆盖 automatic clear；当前未见单独 manual clear harness |
| `RM2` | 已直接执行 | `QosIntegrationTest.RoomQosStateAndRoomPressureOverride` |
| `RM3` | 已直接执行 | `QosIntegrationTest.RoomQosStateAndRoomPressureOverride` 覆盖 room lost / pressure |
| `RM4` | 已直接执行 | `browser_server_signal.mjs` + `RoomQosStateAndRoomPressureOverride` 覆盖 clear |
| `L5` / `L8` | 相关覆盖 | `auto_override_poor`、`AutomaticQosOverrideOnPoorQuality`、`AutomaticQosOverrideOnLostQuality` 验证 severe loss 下自动保护，但不是文档表格的一对一参数扫描 |
| `R4` / `J3` / `T5` | 相关覆盖 | `browser_loopback.mjs` 与 `browser_server_signal.mjs` 验证弱网退化与恢复，但当前 harness 为固定场景，不是文档中的精确矩阵参数 |

### 16.4 当前未一对一执行的 Case


### 16.5 结论


<!-- END GENERATED UPLINK QOS TEST REPORT -->
