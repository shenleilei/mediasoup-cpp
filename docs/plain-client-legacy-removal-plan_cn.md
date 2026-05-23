# 旧 plain-client 直接删除方案

状态：设计文档，尚未删除源码。

日期：2026-05-24

## 1. 目标

直接从主线删除旧 `plain-client`，不再先做“下线但保留源码”的过渡阶段。

这里的“旧 `plain-client`”特指原先自研 RTP/RTCP/QoS 的 Linux `PlainTransport C++ client`，包括：

- `client/build/plain-client` 这个旧可执行文件。
- `client/` 下旧 client 根目录源码，例如 `PlainClient*`、`NetworkThread`、`SourceWorker`、`ThreadTypes`、`RtcpHandler`、`SenderTransportController`、`Vp8Packetizer`。
- `client/qos/**`、`client/sendsidebwe/**`、`client/ccutils/**` 这套自研 QoS / BWE / congestion-control 实现。
- 围绕旧 client 写的 CMake target、gtest、Node harness、matrix、nightly 附件、报告渲染器和历史状态页。

这里不包含新的 `webrtc-qos-plain-push-client` / `webrtc-qos-plain-play-client`。新的 push/play 客户端继续保留，并且继续复用 mediasoup-cpp 的 WebSocket 信令和 PlainTransport 服务端能力。

## 2. 删除原则

| 原则 | 说明 |
|---|---|
| 一次做到最终状态 | 同一个变更里删除源码、测试、脚本入口、报告入口和文档入口，不做长期 legacy 开关。 |
| 保留新客户端 | `client/webrtc_qos_plain_client/**` 是当前主线，不删除。 |
| 保留信令 glue | `client/WsClient.{h,cpp}` 被新 push/play 客户端复用，不删除。 |
| 保留服务端 PlainTransport 能力 | `plainPublish` / `plainSubscribe` 是新客户端仍在使用的 SFU 协议能力，不删除。 |
| 不保留旧结果作为当前证明 | 主 README 和 docs README 不再把旧 plain-client matrix 当作当前 QoS 结果。 |
| 不保留旧 active runner | `run_qos_tests.sh`、`run_all_tests.sh`、nightly 不再构建或运行旧 client。 |

## 3. 必须保留的边界

这些文件或能力不能因为带 `plain` 字样而删除。

| 边界 | 保留原因 | 验收方式 |
|---|---|---|
| `client/WsClient.{h,cpp}` | 新 push/play 客户端仍用它发 `join`、`plainPublish`、`plainSubscribe`。 | 新 push/play target source 仍包含 `client/WsClient.cpp`。 |
| `client/webrtc_qos_plain_client/**` | 当前要继续演进的 WebRTC QoS SDK 推拉流客户端。 | `webrtc-qos-plain-push-client` / `webrtc-qos-plain-play-client` 编译通过。 |
| `scripts/run_webrtc_qos_plain_p2_smoke.sh` | 当前 P2 主验证入口。 | P2 smoke 报告仍为 `PASS`。 |
| `tests/qos_harness/browser_plain_receiver.mjs` | 当前 P2 browser receiver 自动化入口，使用新 push client。 | 不引用 `client/build/plain-client`。 |
| `src/RoomServiceMedia.cpp` | `plainPublish` / `plainSubscribe` 服务端实现。 | `mediasoup_qos_integration_tests` 中 server plain publish / clientStats 测试通过。 |
| `src/SignalingRequestDispatcher.h` | 信令分发仍要支持 `plainPublish` / `plainSubscribe`。 | 新客户端可完成 publish / subscribe。 |
| `src/RoomService.h` | 服务端 API 声明仍要保留。 | 编译通过。 |
| `tests/test_qos_integration.cpp` | SFU 侧 `plainPublish`、`clientStats`、QoS 聚合验证仍属于服务端能力。 | 保留并跑通。 |
| `docs/webrtc-qos-push-play-client-*.md` | 新客户端设计、P2 验收和实现清单。 | README 继续链接这些文档。 |
| `docs/generated/webrtc-qos-plain-p2-*.{md,json}` | 当前 WebRTC QoS Plain P2 验收报告。 | README 继续以这些报告作为当前结果。 |

## 4. 删除范围

### 4.1 旧 client 源码

最终目标是 `client/` 根目录只保留新客户端需要的文件。

| 路径 | 处理 |
|---|---|
| `client/CMakeLists.txt` | 删除。它只定义旧 `plain-client` 独立构建入口。 |
| `client/main.cpp` | 删除。 |
| `client/PlainClientApp.{h,cpp}` | 删除。 |
| `client/PlainClientLegacy.cpp` | 删除。 |
| `client/PlainClientThreaded.cpp` | 删除。 |
| `client/PlainClientSupport.{h,cpp}` | 删除。 |
| `client/NetworkThread.h` | 删除。 |
| `client/SourceWorker.h` | 删除。 |
| `client/ThreadTypes.h` | 删除。 |
| `client/ThreadedControlHelpers.h` | 删除。 |
| `client/RtcpHandler.h` | 删除。 |
| `client/SenderTransportController.h` | 删除。 |
| `client/TransportCcHelpers.h` | 删除。 |
| `client/UdpSendHelpers.h` | 删除。 |
| `client/Vp8Packetizer.h` | 删除。 |
| `client/QosTrace.h` | 删除。 |
| `client/TestHooks.h` | 删除。 |
| `client/qos/**` | 删除。 |
| `client/sendsidebwe/**` | 删除。 |
| `client/ccutils/**` | 删除。 |
| `client/run_sweep_test.sh` | 删除。 |
| `client/test-results/**` | 删除。 |

保留：

| 路径 | 处理 |
|---|---|
| `client/WsClient.{h,cpp}` | 保留。 |
| `client/webrtc_qos_plain_client/**` | 保留。 |

### 4.2 CMake 和 C++ 测试

| 位置 | 删除/修改 |
|---|---|
| 根 `CMakeLists.txt` 的 `MEDIASOUP_QOS_UNIT_TEST_SOURCES` | 删除 `client/PlainClientSupport.cpp`、`tests/test_client_qos.cpp`、`tests/test_thread_model.cpp`。 |
| 根 `CMakeLists.txt` 的 `MEDIASOUP_UNIT_TEST_SOURCES` | 删除 `tests/test_plain_client_vp8.cpp`。 |
| 根 `CMakeLists.txt` | 删除 `mediasoup_thread_integration_tests` target。 |
| 根 `CMakeLists.txt` | 删除 `mediasoup_source_worker_failure_tests` target。 |
| 根 `CMakeLists.txt` | 删除 `mediasoup_source_worker_integration_tests` target。 |
| `tests/test_client_qos.cpp` | 删除。 |
| `tests/test_thread_model.cpp` | 删除。 |
| `tests/test_plain_client_vp8.cpp` | 删除。 |
| `tests/test_thread_integration.cpp` | 删除旧 client / `NetworkThread` / `SourceWorker` 内容。若其中有必须保留的 SFU `PlainTransportDirect` 断言，同一个变更内迁移到 server-only 测试文件后再删除原文件。 |
| `tests/test_source_worker_failure.cpp` | 删除。 |
| `tests/test_source_worker_integration.cpp` | 删除。 |

注意：`tests/test_qos_integration.cpp` 不删除。它验证的是服务端 `plainPublish`、`clientStats` 和 QoS 聚合，不是旧 client 实现。

### 4.3 主动脚本入口

这部分是 review 后补充的关键项。只删源码不够，主动入口也必须一起清理，否则旧 client 仍会被构建、运行或出现在报告里。

| 文件 | 删除/修改 |
|---|---|
| `scripts/run_qos_tests.sh` | 删除 `cpp-client-matrix`、`cpp-client-harness`、`cpp-threaded` group。 |
| `scripts/run_qos_tests.sh` | 删除 `ensure_plain_client_built()`。 |
| `scripts/run_qos_tests.sh` | 删除 `client/build/plain-client` cleanup pattern。 |
| `scripts/run_qos_tests.sh` | 删除 `run_cpp_client_matrix()`、`run_cpp_client_harness()`、`run_cpp_threaded()`。 |
| `scripts/run_qos_tests.sh` | 删除 `cpp-client-harness:*`、`cpp-threaded:*` 精确 target 分支。 |
| `scripts/run_qos_tests.sh` | 删除 `GENERATE_CPP_CLIENT_CASE_REPORT` 和 `render_cpp_client_case_report.mjs` 调用。 |
| `scripts/run_all_tests.sh` | 删除 `threaded` 兼容 alias 中对旧 client 的要求。 |
| `scripts/run_all_tests.sh` | 删除 `cmake --build "$CLIENT_BUILD_DIR" --target plain-client`。 |
| `scripts/run_all_tests.sh` | 删除 `mediasoup_thread_integration_tests`、`mediasoup_source_worker_failure_tests`、`mediasoup_source_worker_integration_tests` 构建项。 |
| `scripts/run_all_tests.sh` | 删除 full regression 报告里的 Plain Client Cases / Matrix JSON / TWCC A/B Eval 行。 |
| `scripts/nightly_full_regression.py` | 删除 `docs/plain-client-qos-case-results.md` 默认附件。 |
| `scripts/nightly_full_regression.py` | 删除旧 cpp-client matrix summary 的特殊解析，除非还有非旧 client runner 复用。 |

### 4.4 Node harness、matrix 和报告渲染

| 路径 | 处理 |
|---|---|
| `tests/qos_harness/cpp_client_runner.mjs` | 删除。 |
| `tests/qos_harness/run_cpp_client_harness.mjs` | 删除。 |
| `tests/qos_harness/run_cpp_client_matrix.mjs` | 删除。 |
| `tests/qos_harness/render_cpp_client_case_report.mjs` | 删除。 |
| `tests/qos_harness/cpp_client_report_artifacts.mjs` | 删除。 |
| `tests/qos_harness/test.cpp_client_runner_trace.mjs` | 删除。 |
| `tests/qos_harness/test.report_artifacts.mjs` 中 plain-client / cpp-client case | 删除对应 case；保留 browser/downlink 等非旧 client case。 |
| `tests/qos_harness/test.case_report_renderers.mjs` 中 plain-client renderer case | 删除对应 case。 |
| `tests/qos_harness/browser_public_interop.mjs` | 旧 plain-client 到 browser interop 删除；如果还要 browser interop，另用新 `webrtc-qos-plain-push-client` 新建 case。 |
| `tests/qos_harness/run_twcc_ab_eval.mjs` | 删除。 |
| `tests/qos_harness/render_twcc_ab_report.mjs` | 删除。 |
| `tests/qos_harness/scenarios/sweep_cases.json` | 删除 `expectByRunner.cpp_client` 相关覆写。 |
| `changes/2026-04-21-plain-client-sender-transport-control/**` | 删除。 |

保留：

| 路径 | 处理 |
|---|---|
| `tests/qos_harness/browser_plain_receiver.mjs` | 保留，它使用新 push client。 |
| browser uplink/downlink harness | 保留。 |
| 新 P2 smoke 相关脚本和报告 | 保留。 |

### 4.5 文档和报告

| 路径 | 处理 |
|---|---|
| `docs/plain-client-qos-status.md` | 删除。 |
| `docs/plain-client-qos-case-results.md` | 删除。 |
| `docs/plain-client-qos-parity-checklist.md` | 删除。 |
| `docs/generated/plain-client-qos-case-results.targeted.md` | 删除。 |
| `docs/generated/uplink-qos-cpp-client-matrix-report*.json` | 删除。 |
| `docs/archive/uplink-qos-cpp-client-runs/**` | 删除。git 历史足够追溯旧结果。 |
| `docs/linux-client-architecture_cn.md` | 删除或改成指向新 WebRTC QoS Plain client；如果内容仍是旧 client 架构，则删除。 |
| `docs/linux-client-multi-source-thread-model_cn.md` | 删除。 |
| `docs/linux-client-threaded-implementation-checklist_cn.md` | 删除。 |
| `docs/linux-client-threaded-test-gap-checklist_cn.md` | 删除。 |
| `docs/qos-status.md` | 删除 Linux plain-client 当前状态入口，改为只保留 browser uplink、downlink、WebRTC QoS Plain P2。 |
| `docs/README.md` | 删除旧 plain-client 链接，保留本删除方案和新 P2 入口。 |
| 根 `README.md` | 删除旧 plain-client 状态、矩阵和架构入口，保留本删除方案和新 P2 报告入口。 |
| `docs/DEVELOPMENT.md`、`docs/dependencies_cn.md`、`docs/architecture_cn.md` | 删除旧 `plain-client`、`SourceWorker`、`NetworkThread`、`client/qos` 相关说明。 |
| `docs/nightly-full-regression.md` | 删除旧 plain-client 附件和报告说明。 |

历史 review 文档如果只是记录历史问题，可以保留，但必须避免被 docs README 当成当前实现入口。

## 5. 单次变更内的实施顺序

这是单次变更，同一个变更内部建议按这个顺序做，避免遗漏引用。

1. 修改 CMake，移除旧 target 和旧 test source。
2. 修改 `scripts/run_qos_tests.sh`、`scripts/run_all_tests.sh`、nightly，移除主动入口。
3. 删除旧 C++ 源码和旧 C++ 测试。
4. 删除旧 Node harness、matrix runner、report renderer 和 changes 目录。
5. 更新 README、docs README、qos-status、dependencies、architecture、DEVELOPMENT。
6. 删除旧报告和旧 archive。
7. 跑静态门禁、构建门禁、运行门禁。
8. 用一个提交完成，例如 `Remove legacy plain-client implementation`。

## 6. 验收门禁

### 6.1 静态门禁

删除后运行：

```bash
cd /root/mediasoup-cpp
SEARCH_PATHS=(CMakeLists.txt client tests scripts docs README.md)
[[ -d changes ]] && SEARCH_PATHS+=(changes)
if rg -n "client/build/plain-client|plain-client-qos|PlainClient(App|Support|Legacy|Threaded)?|cpp-client|cpp_client|client/qos|client/sendsidebwe|client/ccutils|NetworkThread|SourceWorker|Vp8Packetizer|SenderTransportController|RtcpHandler" \
  "${SEARCH_PATHS[@]}" | \
  rg -v "docs/plain-client-legacy-removal-plan_cn.md|docs/webrtc-qos-push-play-client|client/webrtc_qos_plain_client|webrtc-qos-plain|plainPublish|plainSubscribe"; then
  echo "unexpected legacy plain-client references remain" >&2
  exit 1
fi
```

该命令应退出 `0`。过滤项只用于排除本删除方案、新 WebRTC QoS Plain 客户端文档/源码和服务端 PlainTransport 协议名。

静态检查的判定：

| 命中位置 | 判定 |
|---|---|
| 本删除方案 | 允许。 |
| 新客户端文档里说明“不依赖旧实现”的文字 | 允许，但不能链接到已删除源码。 |
| `webrtc-qos-plain-*` | 允许，这不是旧 `plain-client`。 |
| `plainPublish` / `plainSubscribe` | 允许，这是服务端协议能力。 |
| 主动构建、主动测试、旧源码、旧 harness、旧报告入口 | 不允许。 |

还要单独确认：

```bash
cd /root/mediasoup-cpp
test -e client/WsClient.cpp
test -d client/webrtc_qos_plain_client
test ! -e client/CMakeLists.txt
test ! -e client/PlainClientApp.cpp
test ! -e client/qos
test ! -e client/sendsidebwe
test ! -e client/ccutils
test ! -e tests/qos_harness/cpp_client_runner.mjs
test ! -e docs/plain-client-qos-status.md
```

### 6.2 构建门禁

```bash
cd /root/mediasoup-cpp
cmake -S . -B build-webrtc-qos-plain \
  -DCMAKE_PREFIX_PATH=/root/webrtc_qos_sdk/dist/linux-x86_64
cmake --build build-webrtc-qos-plain \
  --target mediasoup-sfu mediasoup_tests mediasoup_qos_unit_tests mediasoup_qos_integration_tests \
  webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_webrtc_qos_plain_unit_tests \
  -j"$(nproc)"
```

通过标准：

| 检查 | 标准 |
|---|---|
| `mediasoup-sfu` | 编译通过。 |
| `mediasoup_tests` | 编译通过，且不再包含 `test_plain_client_vp8.cpp`。 |
| `mediasoup_qos_unit_tests` | 编译通过，且不再包含旧 client QoS 单测。 |
| `mediasoup_qos_integration_tests` | 编译通过，保留 server QoS / `plainPublish` 验证。 |
| `webrtc-qos-plain-push-client` | 编译通过。 |
| `webrtc-qos-plain-play-client` | 编译通过。 |
| `mediasoup_webrtc_qos_plain_unit_tests` | 编译通过。 |
| `plain-client` target | 不存在。 |
| `mediasoup_thread_integration_tests` / `mediasoup_source_worker_*` target | 不存在。 |

target help 也要确认：

```bash
if cmake --build build-webrtc-qos-plain --target help | \
  rg -n "plain-client|thread_integration|source_worker|cpp-client|cpp_client" | \
  rg -v "webrtc-qos-plain-(push|play)-client"; then
  echo "unexpected legacy plain-client target remains" >&2
  exit 1
fi
```

该命令应退出 `0`，不应有旧 target 命中。

### 6.3 运行门禁

```bash
cd /root/mediasoup-cpp
./build-webrtc-qos-plain/mediasoup_tests
./build-webrtc-qos-plain/mediasoup_qos_unit_tests
./build-webrtc-qos-plain/mediasoup_qos_integration_tests
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

脚本入口也要确认：

```bash
cd /root/mediasoup-cpp
if ./scripts/run_qos_tests.sh --list | rg -n "cpp-client|cpp_threaded|plain-client"; then
  echo "unexpected legacy QoS group remains" >&2
  exit 1
fi
```

该命令应退出 `0`，不应有旧分组命中。

## 7. 主要风险

| 风险 | 影响 | 规避 |
|---|---|---|
| 误删 `client/WsClient.*` | 新 push/play 信令不可用。 | 静态门禁和新 target 编译确认。 |
| 误删 `plainPublish` / `plainSubscribe` | 新客户端不能与 SFU 建 PlainTransport。 | server 侧 API 和 integration tests 保留。 |
| 直接删除 `test_thread_integration.cpp` 导致 SFU PlainTransportDirect 覆盖丢失 | server TWCC / PlainTransport 行为回归风险升高。 | 同一变更内把必须保留的 server-only 断言迁移到 `tests/test_qos_integration.cpp` 或新 server-only 测试。 |
| 旧 runner 留在 `run_qos_tests.sh` / `run_all_tests.sh` | 删除源码后 CI 或本地全量回归失败。 | 主动入口纳入删除范围和静态门禁。 |
| 旧报告还挂在 README | README 继续用已删除能力证明价值。 | README 只保留 WebRTC QoS Plain P2 报告。 |
| nightly 继续附加旧 plain-client 报告 | 自动邮件缺附件或误报。 | nightly 附件列表同步删除。 |

## 8. 最终完成标准

| 标准 | 说明 |
|---|---|
| 旧 `plain-client` 源码不存在 | `client/` 根目录只保留 `WsClient.*` 和 `webrtc_qos_plain_client/**` 相关内容。 |
| 旧 target 不存在 | `plain-client`、`mediasoup_thread_integration_tests`、`mediasoup_source_worker_*` 不再出现在 target help。 |
| 旧主动入口不存在 | `run_qos_tests.sh --list` 不再列出 `cpp-client-*` / `cpp-threaded`。 |
| 旧 harness 和旧报告不存在 | `cpp_client_runner`、`run_cpp_client_matrix`、旧 plain-client docs/generated/archive 不再存在。 |
| 新 push/play 客户端仍可构建运行 | WebRTC QoS Plain P2 smoke 仍为 `PASS`。 |
| 服务端 PlainTransport 信令仍保留 | `plainPublish` / `plainSubscribe` 相关 server integration tests 通过。 |
