# PlainTransport C++ Client QoS Parity Checklist

> 当前状态说明
>
> 如果只想快速看“现在是否已进入可回归状态、应该看哪些结果”，
> 先看 [plain-client-qos-status.md](README.md)。

生成时间：`2026-04-16`

## 1. 目标

这份清单只回答一个问题：

`当前仓库里，JS QoS 已有测试资产，哪些已经有 C++ PlainTransport client 的对位覆盖？`

这里的“对位”分 3 类：

1. 纯逻辑 parity
2. signal / server-path parity
3. weak-network matrix parity

## 2. 总体结论

当前已完成的对齐范围：

- JS/client 纯逻辑测试
  已有 C++ `ClientQos*` 对位测试
- JS node/browser signaling harness
  运行时真实 threaded plain-client 已有 `cpp-client-harness` 对位场景
  server-side synthetic signaling / `clientStats` 语义仍主要由 `cpp-integration` + `node-harness` 覆盖
- JS uplink browser matrix 主 gate `43 case`
  已有 `cpp-client-matrix` 对位结果
- JS `BW2` extended sentinel
  已有 C++ client targeted 结果

## 3. 对位清单

| JS 资产 | JS 入口 | C++ 对位 | 当前状态 |
|---|---|---|---|
| `signals / stateMachine / planner / controller / protocol` | `client-js` | [tests/test_client_qos.cpp](../../../tests/test_client_qos.cpp) | `PASS` |
| `publish_snapshot` | [tests/qos_harness/run.mjs](../../../tests/qos_harness/run.mjs) | [tests/qos_harness/run_cpp_client_harness.mjs](../../../tests/qos_harness/run_cpp_client_harness.mjs) | `PASS` |
| `threaded_multi_video_budget` | runtime-specific threaded harness | [tests/qos_harness/run_cpp_client_harness.mjs](../../../tests/qos_harness/run_cpp_client_harness.mjs) | `PASS` |
| `threaded_generation_switch` | runtime-specific threaded harness | [tests/qos_harness/run_cpp_client_harness.mjs](../../../tests/qos_harness/run_cpp_client_harness.mjs) | `PASS` |
| uplink browser matrix 主 gate | [tests/qos_harness/run_matrix.mjs](../../../tests/qos_harness/run_matrix.mjs) | [tests/qos_harness/run_cpp_client_matrix.mjs](../../../tests/qos_harness/run_cpp_client_matrix.mjs) | `43 / 43 PASS` |
| uplink `BW2` extended sentinel | [tests/qos_harness/run_matrix.mjs](../../../tests/qos_harness/run_matrix.mjs) targeted | [tests/qos_harness/run_cpp_client_matrix.mjs](../../../tests/qos_harness/run_cpp_client_matrix.mjs) targeted | `PASS` |

## 4. 当前机器结果

### 4.1 C++ client matrix

- full:
  - [docs/plain-client-qos-case-results.md](../../plain-client-qos-case-results.md)
  - [docs/generated/uplink-qos-cpp-client-matrix-report.json](../../generated/uplink-qos-cpp-client-matrix-report.json)

- targeted:
  - [docs/generated/uplink-qos-cpp-client-case-results.targeted.md](../../generated/uplink-qos-cpp-client-case-results.targeted.md)
  - [docs/generated/uplink-qos-cpp-client-matrix-report.targeted.json](../../generated/uplink-qos-cpp-client-matrix-report.targeted.json)

### 4.2 C++ client signaling harness

入口：

- [tests/qos_harness/run_cpp_client_harness.mjs](../../../tests/qos_harness/run_cpp_client_harness.mjs)

统一脚本分组：

- `cpp-client-harness`

### 4.3 C++ unit parity

入口：

- [tests/test_client_qos.cpp](../../../tests/test_client_qos.cpp)

统一脚本分组：

- `cpp-unit`

## 5. 说明

这份清单的“全部对齐”口径，当前指的是：

- JS uplink QoS 已有的主测试资产
- 在 C++ PlainTransport client 上已经存在可执行、可回归的对位入口

这里刻意不再把下面这些 server-side synthetic signaling case 记为 plain-client runtime 对位：

- `stale_seq`
- `policy_update`
- `auto_override_poor`
- `override_force_audio_only`
- `manual_clear`

这些语义仍有自动化覆盖，但入口属于 `cpp-integration` / `node-harness`，不再由 plain-client 生产二进制内置测试注入承担。

不包括：

- downlink browser matrix 的 C++ client 对位
- 非 uplink QoS 范围外的 browser UI / demo 层验证

## 6. 建议维护方式

后续如果新增新的 JS QoS 测试资产，更新顺序固定为：

1. 先在这份清单里加一行
2. 再补对应 C++ 对位入口
3. 最后再把状态从 `TODO` 改为 `PASS`
