# 上行 QoS 当前进展

> 配套文档：
> - `docs/uplink-qos-design.md`
> - `docs/uplink-qos-implementation-plan.md`
> - `docs/uplink-qos-phase1-phase2-breakdown.md`
> - `docs/uplink-qos-validation-and-livekit-compare.md`
> - `docs/uplink-qos-test-plan.md`
> 日期：2026-04-10

---

## 1. 当前结论

截至当前，`mediasoup-cpp` 的上行 QoS 已经从“只有底层原语”推进到了“具备可运行快环 + 慢环 + 双端验证”的状态。

已具备：

- 端上 QoS 快环
- 多 source planner
- peer/session 级协调基础
- probe/recheck 恢复路径
- 服务端 slow loop 聚合
- `qosPolicy` / `qosOverride`
- automatic override / automatic clear
- `qosConnectionQuality`
- `qosRoomState`
- room pressure override
- Node 双端 harness
- browser loopback 弱网 E2E
- browser -> real SFU signaling E2E

尚未完成：

- `dynacast`
- `subscriber-driven uplink`
- 更强的 server-side capacity / allocation 联动
- 更系统的 telemetry / timeline 输出

---

## 2. 已完成模块

## 2.1 客户端 QoS 子系统

目录：

```text
src/client/src/qos/
```

已实现模块：

- `types.ts`
- `constants.ts`
- `profiles.ts`
- `protocol.ts`
- `trace.ts`
- `clock.ts`
- `sampler.ts`
- `signals.ts`
- `stateMachine.ts`
- `planner.ts`
- `probe.ts`
- `executor.ts`
- `controller.ts`
- `coordinator.ts`
- `factory.ts`
- `adapters/producerAdapter.ts`
- `adapters/statsProvider.ts`
- `adapters/signalChannel.ts`

当前能力：

- 从 sender stats 提取信号并分类 `network/cpu/server_override/manual`
- 使用 `stable / early_warning / congested / recovering` 状态机
- 使用 source-specific planner：
  - `camera`
  - `screenShare`
  - `audio`
- 支持 `PeerQosCoordinator`
- 支持 `PublisherQosController`
- 支持 `createMediasoupProducerQosController()`
- 支持 `createMediasoupPeerQosSession()`
- 支持恢复阶段 probe/recheck

## 2.2 服务端 QoS 子系统

目录：

```text
src/qos/
```

已实现模块：

- `QosTypes.h`
- `QosSnapshot.h/cpp`
- `QosValidator.h/cpp`
- `QosRegistry.h/cpp`
- `QosAggregator.h/cpp`
- `QosRoomAggregator.h/cpp`
- `QosOverride.h/cpp`

当前能力：

- 校验 `clientStats` QoS schema
- 以 `seq` 进行去重 / 拒绝旧快照
- 聚合 peer 级 `stale / lost / quality`
- 聚合 room 级 `quality / poorPeers / lostPeers / stalePeers`
- 支持服务端自动保护性 override：
  - `server_auto_poor`
  - `server_auto_lost`
  - `server_auto_clear`
  - `server_room_pressure`
  - `server_room_pressure_clear`

---

## 3. 已完成接线

## 3.1 客户端接线

已将 QoS namespace 导出到：

- `src/client/src/index.ts`

导出能力：

- `qos.*`
- `createMediasoupProducerQosController`
- `createMediasoupPeerQosSession`

## 3.2 服务端接线

主要接线文件：

- `src/RoomService.cpp`
- `src/RoomService.h`
- `src/SignalingServer.cpp`
- `CMakeLists.txt`

当前接线结果：

- `clientStats` 现在走版本化 QoS snapshot 校验
- `getStats` / `statsReport` 会输出：
  - `clientStats`
  - `qos`
  - `qosStale`
  - `qosLastUpdatedMs`
- join 时会下发 `qosPolicy`
- 支持显式 `setQosPolicy`
- 支持显式 `setQosOverride`
- 支持 `qosConnectionQuality`
- 支持 `qosRoomState`
- 服务端会基于 `poor/lost` 自动下发保护性 `qosOverride`
- 服务端会在恢复后自动下发 `server_auto_clear`
- 服务端会在房间级压力下自动下发 `server_room_pressure`

---

## 4. 当前测试体系

## 4.1 客户端 Jest

目录：

```text
src/client/src/test/
```

当前已覆盖：

- `test.qos.protocol.ts`
- `test.qos.sampler.ts`
- `test.qos.signals.ts`
- `test.qos.stateMachine.ts`
- `test.qos.planner.ts`
- `test.qos.traceReplay.ts`
- `test.qos.executor.ts`
- `test.qos.statsProvider.ts`
- `test.qos.controller.ts`
- `test.qos.coordinator.ts`
- `test.qos.factory.ts`
- `test.qos.peerSession.ts`
- `test.qos.probe.ts`
- `test.qos.signalChannel.ts`

当前状态：

- `16` suites
- `169` tests
- 全绿

## 4.2 服务端 gtest

新增：

- `tests/test_qos_protocol.cpp`
- `tests/test_qos_validator.cpp`
- `tests/test_qos_registry.cpp`
- `tests/test_qos_aggregator.cpp`
- `tests/test_qos_override.cpp`
- `tests/test_qos_room_aggregator.cpp`

fixture：

```text
tests/fixtures/qos_protocol/
```

## 4.3 QoS 黑盒集成测试

文件：

- `tests/test_qos_integration.cpp`

已覆盖新增场景：

- QoS schema 存储与聚合
- `statsReport` 中的 QoS 聚合
- invalid QoS snapshot 拒绝
- 旧 `seq` 不覆盖新 `seq`
- join 后 `qosPolicy`
- 手动 `setQosOverride`
- 自动 `server_auto_poor`
- 自动 `server_auto_lost`
- 自动 `server_auto_clear`
- `qosConnectionQuality`
- `qosRoomState`
- room pressure override
- 手动 `setQosPolicy`

## 4.4 Node 双端 harness

目录：

```text
tests/qos_harness/
```

当前脚本：

- `run.mjs`
- `browser_loopback.mjs`
- `browser_server_signal.mjs`
- `browser_synthetic_sweep.mjs`

当前场景：

- `quick`
  - snapshot publish + aggregation
  - stale seq ignored
  - policy update
- `browser_loopback`
  - chromium headless
  - loopback RTCPeerConnection
  - `tc netem` UDP 弱网
- `browser_server_signal`
  - browser 原生 WebSocket -> 真实 SFU
  - `qosPolicy` update
  - automatic override
  - automatic clear
- `browser_synthetic_sweep`
  - chromium headless
  - synthetic sender stats sweep
  - 默认 camera profile 行为校准
  - 不等同于 `tc netem`

---

## 5. 已执行验证

已实际跑过：

### 客户端

```bash
cd src/client
npm run lint
npm test -- --runInBand
```

结果：

- 全绿

### 服务端 QoS 单元测试

```bash
./build/mediasoup_tests --gtest_filter='QosProtocolTest.*:QosValidatorTest.*:QosRegistryTest.*:QosAggregatorTest.*:QosRoomAggregatorTest.*:QosOverrideBuilderTest.*'
```

结果：

- 全绿

### 服务端 QoS 黑盒集成测试

```bash
./build/mediasoup_qos_integration_tests
```

结果：

- 全绿

### Node 双端 harness

```bash
node tests/qos_harness/run.mjs quick
node tests/qos_harness/browser_loopback.mjs
node tests/qos_harness/browser_server_signal.mjs
node tests/qos_harness/browser_synthetic_sweep.mjs --cases=P0
node --test tests/qos_harness/test.synthetic_sweep.mjs
```

结果：

- 全绿

---

## 6. 重要实现选择

### 6.1 recorder 暂不扩展

当前没有继续把 recorder/timeline 做成单独优先级事项。

原因：

- 现阶段更大的价值在于 QoS 决策与双端验证
- recorder 输出可以后续作为 telemetry/timeline 再补

### 6.2 automatic override 已做，但默认 quick harness 不包含所有场景

`quick` harness 当前保持保守，只保留稳定默认回归场景。

原因：

- 默认回归路径要优先稳定
- 某些更激进或时序更敏感的场景单独保留脚本更适合

### 6.3 browser loopback E2E 使用单独 tuned profile

`browser_loopback.mjs` 中使用了比默认 camera profile 更适合 loopback sender 的 profile。

原因：

- 单机场景的 sender/target bitrate 行为与真实生产环境不同
- 需要先保证它能形成可恢复的 baseline -> degrade -> recover 回归线

### 6.4 synthetic sweep 明确作为“设计校准”，不是弱网结论

新增 `browser_synthetic_sweep.mjs` 与 `synthetic_sweep_shared.mjs`，用于把文档 case 映射成可控的 synthetic sender stats，验证默认 camera profile 在受控输入下的状态机、ladder level、恢复路径和 verdict 逻辑。

边界：

- 它不是 `browser + tc netem`
- 它不会覆盖 Chromium CC、RTP/RTCP、真实 loopback 或真实 SFU 网络路径
- 它不能作为 B/BW/L/R/J/T/S 矩阵“已一对一执行”的依据

当前用途：

- 快速校准 case expectation 是否与当前控制器行为一致
- 快速发现“测试设计错了”还是“控制器行为变了”
- 为后续真实 `browser + netem` 参数化 runner 提供对照基线

---

## 7. 当前剩余差距

相对 LiveKit 风格的 QoS，当前主要剩余差距：

- `subscriber-driven uplink`
- `dynacast`
- 更强的服务端容量/分配联动
- 更系统的 QoS telemetry / timeline / 观测面

其中，前两项的优先级最高。

---

## 8. 下一步建议

后续优先级建议：

1. `subscriber-driven uplink / dynacast` 最小设计与实现
2. 服务端 capacity / allocation 联动
3. recorder/timeline/telemetry 作为后续观测增强

当前代码和测试地基已经足够支撑继续往这三项推进。
