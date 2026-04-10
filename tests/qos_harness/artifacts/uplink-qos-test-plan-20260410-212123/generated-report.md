
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

- loopback baseline: `early_warning / level 1`
- loopback impaired: `early_warning / level 1`
- loopback recovered: `early_warning / level 1`
- impaired trace actions: `setEncodingParameters`

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
