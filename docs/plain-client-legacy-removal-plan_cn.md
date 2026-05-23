# 旧 plain-client 下线与删除方案

状态：设计文档，尚未删除源码。

日期：2026-05-24

## 1. 目标

把旧 `plain-client` 从当前主线里安全下线，后续再删除旧实现，避免它继续影响 WebRTC QoS Plain push/play 客户端的构建、测试和文档判断。

这里的“旧 `plain-client`”特指原先自研 RTP/RTCP/QoS 的 Linux `PlainTransport C++ client`，包括 `client/build/plain-client`、`client/PlainClient*`、`client/qos`、`client/sendsidebwe`、`client/ccutils` 以及围绕它写的旧 harness、旧矩阵报告和旧状态页。

这里不包含新的 `webrtc-qos-plain-push-client` / `webrtc-qos-plain-play-client`。新的 push/play 客户端继续保留，并且仍然复用 mediasoup-cpp 里的信令和 PlainTransport 服务端能力。

## 2. 为什么不能直接删除

旧实现和新实现在命名上都带 `plain`，但职责不同。直接按字符串删除容易误伤三个仍在使用的边界。

| 边界 | 当前用途 | 处理原则 |
|---|---|---|
| `client/WsClient.{h,cpp}` | 新 push/play 客户端仍用它发 `join`、`plainPublish`、`plainSubscribe`。 | 保留。 |
| `client/webrtc_qos_plain_client/**` | 新 WebRTC QoS SDK 推拉流客户端。 | 保留。 |
| `src/RoomServiceMedia.cpp`、`src/RoomService.h`、`src/SignalingRequestDispatcher.h` 里的 `plainPublish` / `plainSubscribe` | 新客户端和测试仍通过这些信令打通 PlainTransport。 | 保留。 |
| `tests/test_qos_integration.cpp`、服务端 PlainTransport 相关测试 | 验证 SFU 侧 plain publish / subscribe 能力。 | 按 server 能力保留或迁移，不能因为旧 client 删除。 |
| P2 smoke / recovery / MP4 / browser / V4L2 报告 | 当前 WebRTC QoS Plain P2 验收依据。 | 保留。 |

## 3. 当前引用审计

### 3.1 旧客户端实现

这些文件属于旧 `plain-client`，第二阶段才删除。第一阶段只从构建、脚本和文档入口下线。

| 路径 | 说明 | 建议处理 |
|---|---|---|
| `client/CMakeLists.txt` | 定义独立 `plain-client` 可执行文件。 | 第一阶段移除或改成显式 legacy 选项，默认不构建。 |
| `client/main.cpp` | 旧可执行入口。 | 第二阶段删除。 |
| `client/PlainClientApp.{h,cpp}` | 旧 client 主流程，包含 signaling、FFmpeg、RTP、QoS 控制。 | 第二阶段删除。 |
| `client/PlainClientLegacy.cpp` | 旧 legacy 运行模式。 | 第二阶段删除。 |
| `client/PlainClientThreaded.cpp` | 旧 threaded 运行模式。 | 第二阶段删除。 |
| `client/PlainClientSupport.{h,cpp}` | 旧 QoS 信号、协议、策略辅助函数。 | 第二阶段删除；若仍有通用价值，先迁移到独立 server/test helper。 |
| `client/NetworkThread.h`、`client/SourceWorker.h`、`client/ThreadTypes.h` | 旧多线程媒体发送模型。 | 第二阶段删除。 |
| `client/SenderTransportController.h`、`client/RtcpHandler.h`、`client/TransportCcHelpers.h`、`client/UdpSendHelpers.h` | 旧 RTP/RTCP/TWCC/UDP 控制面。 | 第二阶段删除。 |
| `client/Vp8Packetizer.h` | 旧 VP8 RTP packetizer。 | 第二阶段删除。 |
| `client/QosTrace.h`、`client/TestHooks.h`、`client/ThreadedControlHelpers.h` | 旧观测和测试 hook。 | 第二阶段删除。 |
| `client/qos/**` | 旧 client 自研 QoS controller。 | 第二阶段删除。 |
| `client/sendsidebwe/**` | 旧 sender-side BWE / TWCC 估算。 | 第二阶段删除。 |
| `client/ccutils/**` | 旧 congestion-control helper。 | 第二阶段删除。 |
| `client/run_sweep_test.sh`、`client/test-results/**` | 旧 sweep 测试入口和结果。 | 第二阶段删除或移入 archive。 |

### 3.2 旧测试入口

这些测试现在仍会把旧实现拉进构建。第一阶段要先下线或拆分，确保新客户端和 server 单测不依赖旧代码。

| 路径 | 当前问题 | 第一阶段处理 |
|---|---|---|
| 根 `CMakeLists.txt` 的 `MEDIASOUP_QOS_UNIT_TEST_SOURCES` | 仍编译 `client/PlainClientSupport.cpp`、`tests/test_client_qos.cpp`、`tests/test_thread_model.cpp`。 | 把旧 client 专属测试从默认 target 移出。 |
| 根 `CMakeLists.txt` 的 `MEDIASOUP_UNIT_TEST_SOURCES` | 仍编译 `tests/test_plain_client_vp8.cpp`。 | 从默认 target 移出。 |
| `tests/test_client_qos.cpp` | 直接 include `client/qos/QosController.h` 和 `PlainClientSupport.h`。 | 下线，或只保留能迁移到 server QoS 的纯协议 fixture。 |
| `tests/test_thread_model.cpp` | 直接覆盖旧队列、旧 TWCC、旧 BWE、旧 sender controller。 | 下线。 |
| `tests/test_thread_integration.cpp` | 混有旧 client binary 检查、旧线程模型测试和部分 server PlainTransport 行为。 | 拆分：旧 client 部分下线，server PlainTransport 部分保留。 |
| `tests/test_plain_client_vp8.cpp` | 直接覆盖旧 `PlainClientApp` 和 `Vp8Packetizer`。 | 下线。 |

### 3.3 旧 harness 和旧报告

这些脚本或报告以 `client/build/plain-client` 为中心，属于旧实现资产。第一阶段先从常用脚本、README 和验收入口移除，第二阶段删除或归档。

| 路径 | 当前问题 | 建议处理 |
|---|---|---|
| `tests/qos_harness/cpp_client_runner.mjs` | 直接启动 `client/build/plain-client`。 | 第一阶段从主动测试入口移除，第二阶段删除。 |
| `tests/qos_harness/run_cpp_client_harness.mjs` | 旧 plain-client QoS harness。 | 第一阶段从主动测试入口移除，第二阶段删除。 |
| `tests/qos_harness/browser_public_interop.mjs` | 旧 plain-client 到 browser 的 interop。 | 若还要 browser interop，改用新 `webrtc-qos-plain-push-client`；旧入口第二阶段删除。 |
| `tests/qos_harness/cpp_client_report_artifacts.mjs` 和相关 renderer tests | 生成旧 plain-client 矩阵报告路径。 | 第一阶段不再作为当前验收入口，第二阶段删除或归档。 |
| `docs/plain-client-qos-status.md`、`docs/plain-client-qos-case-results.md`、`docs/plain-client-qos-parity-checklist.md` | 旧 plain-client 状态和矩阵结果。 | 第一阶段标记 legacy，第二阶段移入 archive 或删除。 |
| `docs/generated/uplink-qos-cpp-client-matrix-report*.json` | 旧 plain-client 矩阵结果。 | 第二阶段归档或删除。 |

### 3.4 必须保留的文件

这些文件不能因为包含 `plain` 字样而删除。

| 路径 | 保留原因 |
|---|---|
| `client/WsClient.{h,cpp}` | 新 push/play 客户端复用 WebSocket request/notification 能力。 |
| `client/webrtc_qos_plain_client/**` | 当前要继续演进的 WebRTC QoS Plain push/play 客户端。 |
| `scripts/run_webrtc_qos_plain_p2_smoke.sh` | 当前 P2 主验证入口。 |
| `tests/qos_harness/browser_plain_receiver.mjs` | 当前 P2 browser receiver 自动化入口，使用新 push client。 |
| `docs/webrtc-qos-push-play-client-design_cn.md` | 新客户端总体设计。 |
| `docs/webrtc-qos-push-play-client-p2-design_cn.md` | P2 设计与验收门禁。 |
| `docs/webrtc-qos-push-play-client-implementation-checklist_cn.md` | 新客户端实现对照清单。 |
| `docs/generated/webrtc-qos-plain-p2-*.{md,json}` | 当前 WebRTC QoS Plain P2 验收报告。 |
| `src/RoomServiceMedia.cpp` | `plainPublish` / `plainSubscribe` 服务端实现仍被新客户端使用。 |
| `src/SignalingRequestDispatcher.h` | 信令分发仍要支持 `plainPublish` / `plainSubscribe`。 |
| `src/RoomService.h` | 服务端 API 声明仍要保留。 |

## 4. 两阶段落地

### 4.1 第一阶段：下线，不删除

目标是让旧 `plain-client` 不再进入默认构建、默认测试、默认文档入口，同时保留源码便于回查。

建议提交名：`Disable legacy plain-client entrypoints`

实施内容：

| 项 | 具体动作 | 验证 |
|---|---|---|
| 构建入口 | `client/CMakeLists.txt` 的 `plain-client` target 不再默认构建。可以删除独立构建入口，也可以先改成 `BUILD_LEGACY_PLAIN_CLIENT=OFF`。 | `cmake --build ... --target help` 默认不再出现 `plain-client`。 |
| 根单测 | 从默认 gtest target 移出旧 client 专属测试。 | `mediasoup_tests`、`mediasoup_qos_unit_tests` 不再编译 `PlainClientSupport.cpp`、`test_plain_client_vp8.cpp`、`test_thread_model.cpp`。 |
| 混合测试拆分 | `tests/test_thread_integration.cpp` 里旧 client binary 和旧线程模型 case 下线；server PlainTransport 能力测试保留。 | `plainPublish` / `plainSubscribe` server 测试仍存在。 |
| harness | 常用脚本不再调用 `client/build/plain-client`。 | `rg -n "client/build/plain-client" scripts tests docs README.md` 只允许命中 legacy 删除方案。 |
| 文档入口 | README 不再把旧 plain-client 矩阵作为当前价值证明；保留新 P2 报告作为主入口。 | README source-of-truth 指向 WebRTC QoS Plain P2。 |

第一阶段不删除旧源码，原因是这样可以把“是否还有活跃引用”单独验证清楚。如果第一阶段通过，第二阶段删除的风险会明显降低。

### 4.2 第二阶段：删除旧实现

目标是在确认没有活跃引用后删除旧代码和旧资产。

建议提交名：`Remove legacy plain-client implementation`

实施内容：

| 项 | 删除范围 | 前置条件 |
|---|---|---|
| 旧 client 源码 | `client/PlainClient*`、`client/main.cpp`、`client/qos/**`、`client/sendsidebwe/**`、`client/ccutils/**`、旧 RTP/RTCP helper。 | 第一阶段默认构建、测试、脚本均不再引用。 |
| 旧 client 测试 | `test_client_qos.cpp`、`test_plain_client_vp8.cpp`、旧 thread model/integration 片段。 | server PlainTransport 测试已拆出并通过。 |
| 旧 harness | 以 `client/build/plain-client` 为入口的 JS harness。 | 新 P2 harness 可覆盖当前 push/play 验收。 |
| 旧报告文档 | 旧 plain-client 状态页、旧矩阵结果、旧对齐清单。 | README 和 docs README 已不再把它们列为当前入口。 |

如果需要保留历史报告，只移入 `docs/archive/legacy-plain-client/`，不要继续挂在主 README 里。

## 5. 验收门禁

### 5.1 静态门禁

第一阶段完成后运行：

```bash
cd /root/mediasoup-cpp
rg -n "client/build/plain-client|PlainClientApp|PlainClientLegacy|PlainClientThreaded|client/qos|client/sendsidebwe|client/ccutils|Vp8Packetizer|SenderTransportController" \
  CMakeLists.txt client tests scripts docs README.md
```

允许命中：

| 命中位置 | 是否允许 |
|---|---|
| 本文档 | 允许。 |
| 明确标记为 legacy/archive 的文档 | 第一阶段允许，第二阶段不建议挂主入口。 |
| 默认 CMake target、默认测试、常用脚本 | 不允许。 |
| `client/webrtc_qos_plain_client/**` | 不应命中旧实现类型名；如果命中说明发生反向依赖。 |

第二阶段完成后运行：

```bash
cd /root/mediasoup-cpp
test ! -e client/PlainClientApp.cpp
test ! -e client/qos
test ! -e client/sendsidebwe
test ! -e client/ccutils
rg -n "client/build/plain-client|PlainClientApp|PlainClientLegacy|PlainClientThreaded|PlainClientSupport|Vp8Packetizer|SenderTransportController" \
  CMakeLists.txt client tests scripts docs README.md
```

第二阶段只允许命中 git 历史外不可避免的 legacy/archive 说明；主线代码和主动测试不允许命中。

### 5.2 构建门禁

```bash
cd /root/mediasoup-cpp
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/root/webrtc_qos_sdk/dist/linux-x86_64
cmake --build build-webrtc-qos-plain \
  --target mediasoup-sfu webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_webrtc_qos_plain_unit_tests \
  -j"$(nproc)"
```

通过标准：

| 检查 | 标准 |
|---|---|
| `mediasoup-sfu` | 编译通过。 |
| `webrtc-qos-plain-push-client` | 编译通过。 |
| `webrtc-qos-plain-play-client` | 编译通过。 |
| `mediasoup_webrtc_qos_plain_unit_tests` | 编译通过。 |
| `client/build/plain-client` | 默认流程不再生成。 |

### 5.3 运行门禁

```bash
cd /root/mediasoup-cpp
./build-webrtc-qos-plain/mediasoup_webrtc_qos_plain_unit_tests
./scripts/run_webrtc_qos_plain_p2_smoke.sh \
  --build-dir build-webrtc-qos-plain \
  --sdk-dir /root/webrtc_qos_sdk \
  --report-dir docs/generated
```

通过标准：

| 报告 | 标准 |
|---|---|
| `docs/generated/webrtc-qos-plain-p2-smoke-report.json` | `overall` 为 `PASS`。 |
| `qosMainline` | `PASS`。 |
| `sdkRuntimeObservability` | `PASS`。 |
| `encoderRuntime` | `PASS`。 |
| `nativeDecodeQoe` | `PASS`。 |
| `recoveryFirstFrame` | `PASS`。 |
| `weakNetworkCoverage` | `PASS`。 |

## 6. 风险和规避

| 风险 | 影响 | 规避 |
|---|---|---|
| 误删 `client/WsClient.*` | 新 push/play 信令不可用。 | 删除前静态检查新 target source，确认仍包含 `client/WsClient.cpp`。 |
| 误删 `plainPublish` / `plainSubscribe` | 新客户端不能与 SFU 建 PlainTransport。 | server 信令和测试按 PlainTransport 能力保留。 |
| 旧 harness 仍跑 stale binary | 测试可能假通过。 | 第一阶段强制移除 `client/build/plain-client` 主动入口。 |
| 把旧 client QoS 结果当成新 P2 结果 | README 价值表达混乱。 | 主 README 只把 WebRTC QoS Plain P2 报告作为当前结果入口。 |
| 删除混合测试时误删 server 行为验证 | `plainPublish` server 回归风险上升。 | 先拆分混合测试，再删旧 client 片段。 |
| 第二阶段删除后才发现通用 helper 被复用 | 回滚成本上升。 | 第一阶段至少保留一个完整回归周期，静态门禁确认无活跃引用后再删。 |

## 7. 建议提交顺序

| 顺序 | 提交 | 内容 |
|---|---|---|
| 1 | `Document legacy plain-client removal plan` | 本文档和 README 链接。 |
| 2 | `Disable legacy plain-client entrypoints` | 下线默认构建、默认测试、主动 harness，不删源码。 |
| 3 | `Remove legacy plain-client implementation` | 删除旧源码、旧测试、旧 harness。 |
| 4 | `Archive legacy plain-client reports` | 如需保留历史结果，把旧状态页和旧报告移入 archive；否则随第 3 个提交删除。 |

## 8. 最终完成标准

| 标准 | 说明 |
|---|---|
| 新 push/play 客户端仍可构建运行 | `webrtc-qos-plain-push-client` 和 `webrtc-qos-plain-play-client` 通过 P2 smoke。 |
| 服务端 PlainTransport 信令仍保留 | `plainPublish` / `plainSubscribe` 不删除，相关 server 测试保留。 |
| 默认构建不再包含旧 client | 不再默认生成 `client/build/plain-client`。 |
| 主 README 不再依赖旧结果证明价值 | 当前价值用 WebRTC QoS Plain P2 报告证明。 |
| 旧源码无活跃引用 | `rg` 静态门禁通过。 |
