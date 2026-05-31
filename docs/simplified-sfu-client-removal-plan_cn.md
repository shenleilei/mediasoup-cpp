# 简化版 mediasoup-cpp 服务方案：移除 native client 功能

## 背景

当前项目同时包含 `mediasoup-sfu` 服务、浏览器推拉流 demo、服务端 QoS/PlainTransport 能力，以及根目录 `client/` 下的 native WebRTC QoS plain push/play client。若目标是构建一个更简化的 mediasoup-cpp 服务，并至少保留浏览器之间的互通能力，主要收敛点应是删除 native client 功能，而不是删除浏览器 demo 或服务端媒体转发能力。

本方案的目标是让仓库收敛为：

- 保留 `mediasoup-sfu` 服务端。
- 保留浏览器推流、拉流、房间加入和 WebSocket 信令 demo。
- 保留浏览器互通所需的静态资源、前端代码和服务端媒体转发路径。
- 保留服务端必要的 QoS、PlainTransport 协议、测试基础设施。
- 移除根目录 `client/` 下 native WebRTC QoS plain client 的构建、源码、脚本、测试 harness 和文档入口。

## 非目标

本次简化不建议顺手删除以下内容：

- `src/client/lib/`：该目录名称容易误导，但它属于浏览器端 client 代码路径，不是根目录 `client/` 下的 native client。
- `src/qos/`：这是服务端 QoS 能力，不等同于 native client。除非后续目标变成完全无 QoS，否则应保留。
- 浏览器 demo、静态资源、WebSocket 信令、房间加入逻辑、推拉流互通链路。
- 与 native client 无关的测试 fixture，例如 `tests/fixtures/qos_protocol/valid_client_v1.json`。
- 服务端 PlainTransport 协议能力，例如 `RoomService::plainPublish`、`RoomService::plainSubscribe` 和 `QosIntegrationTest.PlainPublish*`。如果后续目标升级为 browser-only SFU，再把这部分作为独立范围评审。

## 当前耦合点

删除 `client/` 前需要先处理以下耦合点。

### CMake 构建耦合

顶层 `CMakeLists.txt` 中 native client 相关入口包括：

- `find_package(WebRtcQosSdk CONFIG QUIET)`。
- `select_webrtc_qos_role(...)`。
- `webrtc-qos-plain-push-client` 可执行目标。
- `webrtc-qos-plain-play-client` 可执行目标。
- `mediasoup_webrtc_qos_plain_unit_tests` 测试目标。
- `WEBRTC_QOS_PUSH_TARGET` 和 `WEBRTC_QOS_PLAY_TARGET` 相关判断。

其中 `mediasoup_webrtc_qos_plain_unit_tests` 也引用 `client/webrtc_qos_plain_client/` 下的源码，因此不能只删除两个可执行目标。

### `WsClient` 归属

`client/WsClient.cpp` 和 `client/WsClient.h` 主要服务 native push/play client。当前集成测试主路径已经有独立的 `tests/TestWsClient.h`，而不是依赖 `client/WsClient.*`。

当前 `client/WsClient.*` 的测试耦合主要是：

- `tests/test_ws_client.cpp` 专门测试 `client/WsClient.h`。
- 顶层 `CMakeLists.txt` 把 `client/WsClient.cpp` 加入 `mediasoup_tests`。

默认建议是随 native client 一起删除 `client/WsClient.*` 和 `tests/test_ws_client.cpp`，并从 `mediasoup_tests` source 列表中移除 `client/WsClient.cpp`。只有在明确需要一个生产无关、通用的 WebSocket 测试 helper 时，才将它改名并迁移到 `tests/support/`，同时在文档中说明它不再是 native client helper。

### native client 专属测试耦合

移除 `mediasoup_webrtc_qos_plain_unit_tests` 后，以下测试文件虽然未必继续编译，但已经没有有效目标，应删除或归档，避免留下 dead tests：

- `tests/test_webrtc_qos_realtime_source.cpp`
- `tests/test_webrtc_qos_decode_sink.cpp`
- `tests/test_webrtc_qos_thread_model_primitives.cpp`

### 测试 harness 耦合

`tests/qos_harness/browser_plain_receiver.mjs` 直接要求 `webrtc-qos-plain-push-client` 存在。删除 native push client 后，它会变成必然失败的测试入口，应明确删除、归档，或从所有聚合入口摘除。

同时检查：

- `tests/qos_harness/browser_plain_receiver.mjs`
- `tests/qos_harness/browser/plain-receiver-entry.js`，如果只被 browser plain receiver harness 使用。
- `tests/TestProcessUtils.h` 中指向 `build-webrtc-qos-plain` 的默认路径，避免继续暗示 native client 构建目录是主路径。

### 脚本耦合

以下脚本属于 native WebRTC QoS plain client 路径，应删除或从总测试入口中摘除：

- `scripts/run_webrtc_qos_plain_p2_acceptance.sh`
- `scripts/run_webrtc_qos_plain_p2_smoke.sh`
- `scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh`
- `scripts/run_webrtc_qos_plain_p3_v4l2_report.sh`
- `scripts/verify_webrtc_qos_plain_client_boundaries.py`
- `scripts/verify_webrtc_qos_plain_p2_reports.py`
- `scripts/verify_webrtc_qos_plain_thread_model_boundaries.py`

同时需要更新：

- `scripts/run_qos_tests.sh`
- `scripts/run_all_tests.sh`
- `scripts/nightly_full_regression.py`

这些入口不应再调用已删除的 native client 测试、smoke、browser receiver、boundary 或 report 生成逻辑。

### 文档、setup 和生成报告耦合

需要清理或归档的文档包括：

- `docs/webrtc-qos-push-play-client-design_cn.md`
- `docs/webrtc-qos-push-play-client-implementation-checklist_cn.md`
- `docs/webrtc-qos-push-play-client-p2-design_cn.md`
- `docs/webrtc-qos-push-play-client-thread-model-design_cn.md`
- `docs/generated/webrtc-qos-plain-*`
- `docs/generated/README.md`
- `README.md`
- `README_en.md`
- `docs/dependencies_cn.md`
- `docs/architecture_cn.md`
- `docs/DEVELOPMENT.md`
- `docs/README.md`
- `docs/full-architecture-flow_cn.md`
- `docs/qos-status.md`
- `docs/troubleshooting_cn.md`
- `docs/nightly-full-regression.md`
- `setup.sh`

`docs/generated/` 下的历史报告删除噪音较大，建议单独提交处理，或移动到明确的 archive 区域。不要把大量历史报告删除和 CMake 行为变更混在一个提交中。

`setup.sh` 也需要清理，不应继续提示 `CMAKE_PREFIX_PATH=<webrtc_qos_sdk>` 或 native plain client 构建方式。

## 推荐执行阶段

### Phase 0：建立基线

在正确目录操作：

```bash
cd /root/workspace/mediasoup-cpp
git status --short --branch
```

确认当前分支、未提交修改和最近提交。不要再使用已废弃的 `/root/mediasoup-cpp-src`。

建议先跑一次最小构建基线：

```bash
cmake -S . -B build-slim -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build-slim --target mediasoup-sfu -j$(nproc)
```

### Phase 1：先断开 native client 构建入口

先修改顶层 `CMakeLists.txt`，移除 native client 构建入口，但暂时不删除源码文件。

应移除或调整：

- `WebRtcQosSdk` 查找逻辑。
- `select_webrtc_qos_role(...)`。
- `webrtc-qos-plain-push-client`。
- `webrtc-qos-plain-play-client`。
- `mediasoup_webrtc_qos_plain_unit_tests`。
- 与 `WEBRTC_QOS_PUSH_TARGET`、`WEBRTC_QOS_PLAY_TARGET` 相关的条件分支。

这一阶段的验收目标是：没有 native client 目标参与构建，`mediasoup-sfu` 仍可成功构建。

### Phase 2：处理 `WsClient` 和 native client 专属测试

默认执行删除路径：

```text
client/WsClient.cpp
client/WsClient.h
tests/test_ws_client.cpp
tests/test_webrtc_qos_realtime_source.cpp
tests/test_webrtc_qos_decode_sink.cpp
tests/test_webrtc_qos_thread_model_primitives.cpp
```

同时更新 `CMakeLists.txt`：

- 从 `MEDIASOUP_UNIT_TEST_SOURCES` 中移除 `client/WsClient.cpp` 和 `tests/test_ws_client.cpp`。
- 移除 `mediasoup_webrtc_qos_plain_unit_tests` 及其 source 列表。

可选保留路径：如果确认 `client/WsClient.*` 仍有通用测试价值，而不是 native client 遗留代码，则将其改名并移动到 `tests/support/`，并在文档中解释新的用途。

这一阶段的验收目标是：`mediasoup_tests` 不再依赖根目录 `client/`，且不留下 native client 专属 dead tests。

### Phase 3：删除 native client 源码

删除：

```text
client/webrtc_qos_plain_client/
```

确认 `client/WsClient.*` 已删除或迁移后，继续删除根目录：

```text
client/
```

这一阶段应避免改动 `src/client/lib/`。

### Phase 4：清理脚本、测试 harness 和聚合入口

删除 native plain client 专用脚本和 harness，或至少从聚合入口中摘除。

需要特别处理：

- `tests/qos_harness/browser_plain_receiver.mjs`：删除 native push client 后会必然失败。
- `tests/qos_harness/browser/plain-receiver-entry.js`：如果没有其他消费者，应随 browser plain receiver harness 删除或归档。
- `scripts/run_qos_tests.sh` 中所有 P2/P3 native plain client、browser receiver 和 report 复核分支。
- `scripts/run_all_tests.sh` 和 `scripts/nightly_full_regression.py` 中的 generated plain report 引用。

重点检查：

```bash
git grep -n -E "client/webrtc_qos_plain_client|webrtc_qos_plain|webrtc-qos-plain|WebRtcQosSdk|webrtc_qos_sdk|build-webrtc-qos-plain|plain-push-client|plain-play-client|browser_plain_receiver|plain-receiver-entry"
```

其中构建、脚本和活跃文档不应再出现 native client 入口。若 archive 文档保留历史引用，应明确放在归档路径中。

### Phase 5：清理文档和 setup 输出

更新根 README、docs 入口、`docs/generated/README.md` 索引和 setup 输出，说明当前简化版服务的边界：

- 支持浏览器通过 WebSocket 信令加入房间。
- 支持浏览器推流和拉流互通。
- 保留服务端 PlainTransport 能力，但不再提供 native plain push/play client。
- 不再要求 WebRtcQosSdk 作为构建依赖。
- `setup.sh` 不再提示用户设置 `CMAKE_PREFIX_PATH=<webrtc_qos_sdk>`。

历史设计文档和 generated report 建议单独提交删除或归档。

## 验收清单

### 构建验收

```bash
cmake -S . -B build-slim -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build-slim --target mediasoup-sfu -j$(nproc)
```

若保留单元测试：

```bash
cmake -S . -B build-test -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build-test -j$(nproc)
```

### 引用验收

```bash
git grep -n -E "client/webrtc_qos_plain_client|webrtc_qos_plain|webrtc-qos-plain|WebRtcQosSdk|webrtc_qos_sdk|build-webrtc-qos-plain|plain-push-client|plain-play-client|browser_plain_receiver|plain-receiver-entry"
```

期望结果：

- 活跃构建、脚本、setup、README 中没有命中。
- 如果仍有命中，只应位于明确的 archive 或历史说明文档中。

### 功能验收

启动 `mediasoup-sfu` 后，用浏览器 demo 验证：

- 浏览器 A 加入房间并推流成功。
- 浏览器 B 加入同一房间并拉到浏览器 A 的流。
- 浏览器 B 推流时，浏览器 A 也能拉到对应流。
- 断开、重连、离开房间不会导致服务崩溃。

如果本次不删除服务端 PlainTransport 协议能力，还应保留并验证 `QosIntegrationTest.PlainPublish*` 相关测试，不要把它们和 native client 一起误删。

## 建议提交拆分

建议拆成四个提交，降低回滚和 review 成本：

1. `build: remove native plain client targets`
2. `test: remove native client helper tests`
3. `chore: remove native plain client sources, scripts, and harnesses`
4. `docs: document simplified sfu-only service`

## 最小可行替代方案

如果只想快速得到一个简化构建，而不是立刻删除文件，可以只执行 Phase 1：从 CMake 中移除 native client 和 WebRtcQosSdk 入口，保留源码和脚本文档不动。

这种方式改动最小，但仓库中仍会存在已经不可构建或不再推荐使用的 native client 代码，后续维护者容易误解。因此它适合短期验证，不适合作为最终清理状态。

## 最终推荐状态

最终仓库应满足：

- 根目录不再存在 native client 功能入口。
- `mediasoup-sfu` 构建不依赖 WebRtcQosSdk。
- 浏览器 demo 是保留的端到端推拉流验证入口。
- `tests/TestWsClient.h` 继续服务集成测试，`client/WsClient.*` 不再作为遗留 helper 混在生产 client 目录。
- native plain client 专属脚本、harness、unit tests 和 generated report 不再出现在活跃路径。
- 文档清楚描述“简化版服务”的能力边界和不再支持的 native client 能力。
