# WebRtcServer 共享端口改造评审

## 1. 结论

新版 mediasoup 官方已经支持我们要的核心能力，但升级 worker 版本本身不会让当前项目自动进入共享端口模式。

当前仓库的问题不是 `mediasoup-worker 3.14.6` 完全没有能力，而是 C++ 封装层还在走 `listenInfos` 独立端口池路径：

- `Worker` 现在已改成单端口参数。
- `Router::createWebRtcTransport()` 固定构造 `ListenIndividual`。
- `RoomService` 给 WebRTC 和 PlainTransport 都传同一份 `listenInfos`。
- FBS/generated 已经有 `WebRtcServer`、`ListenServer` 和 `ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER`，但业务代码没有用。

建议分两步走：

1. 先在当前 `3.14.6` worker 协议上接入 `WebRtcServer`，解决浏览器 `WebRtcTransport` 的端口复用问题。
2. 再把 worker 升级到更新的 3.19.x 作为独立变更处理，避免把协议升级、编译要求、运行行为变化和共享端口改造混在一起。

`WebRtcServer` 只覆盖 `WebRtcTransport`。项目里的录制、plain publish/subscribe 和部分内部链路仍会创建 `PlainTransport`，这部分不会因为 `WebRtcServer` 自动变成单端口。

## 2. 项目约束状态

`AGENTS.md` 要求先读取：

- `docs/aicoding/PROJECT_STANDARD.md`
- `docs/aicoding/PLANS.md`
- `docs/aicoding/REVIEW.md`

但当前仓库没有 `docs/aicoding/` 目录，也没有现成的 `changes/` 目录。本文档按 `AGENTS.md` 的 Structured Change 意图新建在：

```text
changes/webrtcserver-shared-port/README.md
```

本文档是改造评审和实施方案，不是已接受行为规范；稳定行为落地后再同步到 `specs/current/`。

## 3. 官方能力与版本判断

### 3.1 mediasoup 已支持的能力

官方 v3 API 已提供：

- `worker.createWebRtcServer()`
- `router.createWebRtcTransport({ webRtcServer })`
- 多个 `WebRtcTransport` 复用同一个 worker 级 `WebRtcServer` 监听入口

官方文档还明确要求：如果应用创建 N 个 mediasoup worker，一般也要创建 N 个 `WebRtcServer`，且每个 server 使用不同端口，避免多个 worker 绑定同一个本地端口。

### 3.2 版本线索

从官方 changelog 看：

- `3.10.0` 引入 `WebRtcServer`。
- `3.10.8` 允许 `WebRtcServer` 的 `listenInfos.port` 省略，从 worker RTC 端口范围内自动分配。
- `3.14.0` 引入 `TransportListenInfo.portRange`，并开始弱化 worker 级端口范围配置的推荐地位。
- `3.14.5` 修复过 `WebRtcServer` 相关 TCP 内存泄露和 close crash。
- `3.17.1` 移除了 `listenInfos` 最多 8 个的限制。
- `3.19.18` 把 `WORKER_CLOSE` 从 request 改成 notification，并要求 `mediasoup-worker` 用 C++20 构建。

当前项目锁定的是 `3.14.6`，已经晚于 `WebRtcServer` 引入和 3.14.5 修复，因此第一阶段不必为了“是否存在 WebRtcServer”升级到 3.19。

### 3.3 升级不能替代本地改造

即使把根目录 `mediasoup-worker` 二进制换成 3.19.x，当前 C++ 代码仍然会继续发送：

```text
ROUTER_CREATE_WEBRTCTRANSPORT + ListenIndividual
```

也就是说 transport 仍会按独立端口池创建，不会自动共享监听端口。

如果升级到 3.19.x，还必须单独处理：

- 重新同步 `worker/fbs` 与 `generated/*_generated.h`。
- 核对所有 `FBS::Request::Method` enum 顺序和含义。
- 调整 `Worker::close()`，因为 3.19.18 起 `WORKER_CLOSE` 行为有变化。
- 确认 Docker/本机构建链路满足 C++20 worker 构建要求。
- 回归 Channel、Router、Transport、Producer、Consumer、PlainTransport 的请求响应体兼容性。

## 4. 当前代码基线

### 4.1 已有协议能力

当前仓库已有这些 FBS 定义：

- `fbs/worker.fbs`
  - `CreateWebRtcServerRequest`
  - `CloseWebRtcServerRequest`
- `fbs/webRtcTransport.fbs`
  - `ListenIndividual`
  - `ListenServer`
- `fbs/request.fbs`
  - `WORKER_CREATE_WEBRTCSERVER`
  - `WORKER_WEBRTCSERVER_CLOSE`
  - `WEBRTCSERVER_DUMP`
  - `ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER`
- `generated/worker_generated.h`
- `generated/webRtcTransport_generated.h`
- `generated/webRtcServer_generated.h`

这说明当前协议层已经有接入点。

### 4.2 当前缺失的业务封装

当前 C++ 层缺失：

- `WebRtcServer` C++ wrapper 或轻量 handle。
- `Worker::createWebRtcServer(...)`。
- `Worker` 对默认 `webRtcServerId` 的持有。
- worker respawn 后重新创建 `WebRtcServer` 的逻辑。
- `Router` 构造时继承父 worker 的默认 `webRtcServerId`。
- `WebRtcTransportOptions` 选择 `ListenServer` 的字段。
- `Router::createWebRtcTransport()` 使用 `ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER` 的请求路径。
- 运行时配置项，例如是否启用 `WebRtcServer`、每个 worker 使用哪个共享监听端口。

### 4.3 当前独立端口路径

当前路径是：

```text
main
  -> BuildListenInfos(options)
  -> CreateWorkerThreadPool(..., listenInfos)
  -> WorkerThread
  -> RoomManager(..., listenInfos)
  -> RoomService::createTransport()
  -> Router::createWebRtcTransport()
  -> ListenIndividual(listen_infos)
```

`Worker::spawn()` 现在给 worker 进程传：

```text
--rtcPort=<port>
```

所以 WebRTC transport 和 PlainTransport 都会消耗 worker RTC 端口池。

### 4.4 PlainTransport 仍然是单独问题

当前会创建 `PlainTransport` 的路径包括：

- `RoomService::createPlainTransport()`
- `RoomService::plainPublish()`
- `RoomService::plainSubscribe()`
- `RoomRecordingHelpers.cpp` 里的录制链路

这些不是 `WebRtcServer` 的覆盖范围。切换 `WebRtcServer` 只能降低浏览器 WebRTC transport 的端口压力，不能消除 PlainTransport 的端口占用。

## 5. 目标模型

### 5.1 对象关系

目标结构：

```text
Worker
  -> WebRtcServer(id, listenInfos)
  -> Router(defaultWebRtcServerId)
     -> WebRtcTransport(ListenServer(webRtcServerId))
     -> PlainTransport(ListenInfo)
```

核心原则：

- 每个 mediasoup worker 持有自己的 `WebRtcServer`。
- 每个 Router 继承创建它的 worker 的默认 `webRtcServerId`。
- 浏览器 `WebRtcTransport` 默认挂到 Router 的 `webRtcServerId`。
- PlainTransport 继续走 `listenInfos`，后续单独做端口策略。
- 保留 `ListenIndividual` fallback，便于灰度和回滚。

### 5.2 端口模型

单 worker 测试环境：

```text
worker-0 WebRtcServer UDP port 8000
PlainTransport 继续使用显式端口或 `portRange`
```

多 worker 环境：

```text
worker-0 WebRtcServer UDP port 8000
worker-1 WebRtcServer UDP port 8001
worker-2 WebRtcServer UDP port 8002
...
```

不能让多个 worker 默认绑定同一个本地 UDP 端口，除非明确设计进程间 socket 共享或 reuse-port 行为，并完成内核、worker、负载分配侧验证。本次改造不把多 worker 同端口作为目标。

### 5.3 建议新增配置

建议新增运行时配置：

```json
{
  "webRtcServerEnableUdp": true,
  "webRtcServerEnableTcp": false
}
```

语义：

- 所有 worker 都走 WebRtcServer 单端口路径，不再保留旧路径。
- 每个 worker 的监听端口由 `webRtcServerPort` 统一配置并按 worker 序号递增。
- 当 worker 数量大于可用 WebRtcServer 端口数量时，启动失败并输出明确错误。

Docker/env 建议对应：

```text
MEDIASOUP_WEBRTC_SERVER_PORT=8000
```

## 6. 推荐实施阶段

### Phase 0：测试机单端口默认值

当前版本已经切到 WebRtcServer 单端口路径。测试机不再使用 `MEDIASOUP_RTC_MIN_PORT` / `MEDIASOUP_RTC_MAX_PORT` 表达 WebRTC 端口，而是按节点名默认推导：

```text
mediasoup-h1 -> MEDIASOUP_WEBRTC_SERVER_PORT=8000
mediasoup-h2 -> MEDIASOUP_WEBRTC_SERVER_PORT=8001
mediasoup-h3 -> MEDIASOUP_WEBRTC_SERVER_PORT=8002
```

验收：

- 双浏览器 join/publish/subscribe 成功。
- `ss -lunp` 显示每个 `mediasoup-worker` 只监听一个 WebRtcServer UDP 端口。

### Phase 1：C++ 封装层接出 WebRtcServer

新增一个轻量 wrapper：

```text
src/WebRtcServer.h
src/WebRtcServer.cpp
```

建议最小字段：

```cpp
class WebRtcServer {
public:
  WebRtcServer(std::string id, Channel* channel);
  const std::string& id() const;
  void close();
  json dump();
};
```

`Worker` 增加：

```cpp
std::shared_ptr<WebRtcServer> createWebRtcServer(
  const std::vector<nlohmann::json>& listenInfos);

std::string defaultWebRtcServerId() const;
```

请求构造：

```text
Method: WORKER_CREATE_WEBRTCSERVER
Body:   Worker_CreateWebRtcServerRequest
```

close 请求：

```text
Method: WORKER_WEBRTCSERVER_CLOSE
Body:   Worker_CloseWebRtcServerRequest
```

验收：

- worker 启动后能创建 `WebRtcServer`。
- `WEBRTCSERVER_DUMP` 能返回 UDP socket / TCP server 信息。
- worker close 和 worker died 不遗留 wrapper 状态。

### Phase 2：Router 支持 ListenServer

调整 `Router` 和 `WebRtcTransportOptions`：

```cpp
struct WebRtcTransportOptions {
  std::vector<json> listenInfos;
  std::string webRtcServerId;
  bool useWebRtcServer = false;
  ...
};
```

`Router` 构造函数增加默认 `webRtcServerId`：

```cpp
Router(id, channel, mediaCodecs, defaultWebRtcServerId)
```

`Router::createWebRtcTransport()` 分支：

```text
if useWebRtcServer/defaultWebRtcServerId:
  Method: ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER
  Listen: ListenServer(webRtcServerId)
else:
  Method: ROUTER_CREATE_WEBRTCTRANSPORT
  Listen: ListenIndividual(listenInfos)
```

注意：当前 FBS 里已经有独立的 `ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER`，不要只替换 union body 但仍发送 `ROUTER_CREATE_WEBRTCTRANSPORT`。

验收：

- WebRTC transport 创建响应仍能解析 ICE parameters/candidates/DTLS parameters。
- ICE candidate 暴露的端口为 WebRtcServer 监听端口。
- fallback 路径保持可用。

### Phase 3：WorkerThread 启动和 respawn 串起来

在 `WorkerThread::createWorkers()` 中：

1. 创建 worker。
2. 计算该 worker 的 `WebRtcServer` listenInfo。
3. 调 `worker->createWebRtcServer(...)`。
4. 再加入 `WorkerManager`。
5. 创建 `RoomManager`。

`WorkerThread::onWorkerDied()` respawn 路径也必须做同样初始化，否则新 worker 创建的 Router 会丢失默认 `webRtcServerId`，回退或失败。

验收：

- worker crash 后 respawn，新的房间仍走 WebRtcServer。
- 旧房间按现有 runtime safety 规范被通知/拆除。
- `/healthz` 或日志能体现 WebRtcServer 初始化失败。

### Phase 4：运行时配置与 Docker

新增配置解析：

- `RuntimeOptions::webRtcServerPort`
- 可选：`webRtcServerEnableTcp`

校验：

- 端口范围合法。
- 启用时可用端口数 >= mediasoup worker 数。
- 与 `listenIp/announcedIp` 组合生成 listenInfo。

Docker：

- entrypoint 增加 env 到 CLI/config 的映射。
- README 更新单 worker 和多 worker 端口暴露示例。
- WebRtcServer 单端口模式下，不再需要把 WebRTC browser transport 数量和 RTC 端口数绑定。

验收：

- `docker run` 单 worker 使用 `8000/udp` 可跑浏览器 demo。
- 多 worker 时要求暴露对应的多个 UDP 端口。
- 配置错误时启动失败，不静默回退。

### Phase 5：PlainTransport 端口策略

这是独立范围，不建议塞进第一版。

候选方案：

1. 给 PlainTransport 独立端口范围，例如 `plainMinPort/plainMaxPort`。
2. 录制链路固定走 `127.0.0.1`，评估是否可用本地专用范围。
3. 对 browser-only demo 增加关闭录制/PlainTransport 链路的配置。
4. 后续再评估 plain RTP 入口是否需要自己的复用模型。

验收：

- 只开浏览器 demo 时，WebRtcTransport 不再消耗多个公网 RTC 端口。
- 打开录制或 plain publish/subscribe 时，PlainTransport 的端口消耗有明确容量模型。

## 7. 升级策略

### 7.1 第一阶段不建议直接升 3.19

原因：

- 当前 `3.14.6` 已经具备 WebRtcServer 所需 FBS 和 worker 能力。
- 当前项目的缺口在 C++ 业务封装，不在 worker 是否支持。
- 3.19.x 会带来协议、构建和行为变化，尤其 `WORKER_CLOSE` 行为变化会直接影响 `Worker::close()`。
- 同时做升级和架构切换，问题定位会变差。

### 7.2 后续升级到 3.19.x 的独立清单

独立升级时再做：

- 更新 `MEDIASOUP_VERSION`。
- 更新 `setup.sh`、`Dockerfile` 的 worker 下载版本。
- 同步 upstream `worker/fbs` 到本仓库 `fbs/`。
- 重新生成 `generated/*_generated.h`。
- 修正 `Worker::close()` 和所有 request/notification 差异。
- 完整回归 unit/integration/browser harness。

## 8. 风险与边界

### 8.1 端口容量误判

`WebRtcServer` 会显著降低 browser WebRTC transport 对端口池的压力，但不会让所有媒体链路都单端口。PlainTransport 仍需容量规划。

### 8.2 多 worker 单端口误用

单 worker 可以只暴露一个 WebRtcServer UDP 端口。多 worker 不应默认绑定同一个端口。本方案按“每 worker 一个共享端口”设计。

### 8.3 FBS 方法选错

当前 FBS 有专门的：

```text
ROUTER_CREATE_WEBRTCTRANSPORT_WITH_SERVER
```

实现时要使用这个 method。只把 options 改成 `ListenServer` 但继续发 `ROUTER_CREATE_WEBRTCTRANSPORT` 有兼容风险。

### 8.4 回滚路径

WebRtcServer 初始化失败时建议启动失败，不建议运行中静默降级。

## 9. 测试计划

### 9.1 单元测试

建议新增或扩展：

- `Worker` 创建 WebRtcServer 的请求体构造测试。
- `Router::createWebRtcTransport()` 选择 `ListenServer` 时使用正确 method 和 body。
- 配置校验：worker 数量超过 WebRtcServer 端口数量时失败。

### 9.2 集成测试

建议新增：

- 单 worker、单 UDP WebRtcServer 端口、两个浏览器互通。
- 单 worker、两个 peer，各自 send/recv transport，不出现 `no more available ports`。
- worker crash/respawn 后新房间仍可创建 WebRtcTransport。

### 9.3 运行测试

测试机验证：

```text
curl http://127.0.0.1:9000/healthz
curl "http://127.0.0.1:9000/api/resolve?roomId=test-room"
```

浏览器验证：

- 两个浏览器加入同一房间。
- A 发布音视频。
- B 订阅成功。
- B 发布音视频。
- A 订阅成功。
- 观察日志无 `no more available ports`。

网络验证：

```text
ss -lunp | grep mediasoup-worker
```

期望 WebRTC browser path 主要落在 WebRtcServer 端口；PlainTransport 另行统计。

## 10. 推荐下一步

推荐下一步只做 Phase 1 到 Phase 2 的最小代码改造：

- 不升级 worker。
- 增加 WebRtcServer wrapper。
- 每个 worker 创建一个 WebRtcServer。
- Router 默认使用 `ListenServer` 创建 WebRtcTransport。
- PlainTransport 保持原状。
- 保留配置开关和旧路径 fallback。

这能用最小风险验证核心假设：浏览器 demo 的 WebRTC transport 是否可以在单 worker 单 UDP 端口下稳定复用。

## 11. 参考

官方资料：

- mediasoup v3 API: `worker.createWebRtcServer()` 与 `router.createWebRtcTransport({ webRtcServer })`
  - https://mediasoup.org/documentation/v3/mediasoup/api/
- mediasoup v3 changelog
  - https://github.com/versatica/mediasoup/blob/v3/CHANGELOG.md

本仓库证据：

- `src/Worker.cpp`
- `src/Worker.h`
- `src/Router.cpp`
- `src/Router.h`
- `src/WorkerThread.cpp`
- `src/MainBootstrap.cpp`
- `src/RoomServiceMedia.cpp`
- `src/RoomRecordingHelpers.cpp`
- `fbs/request.fbs`
- `fbs/worker.fbs`
- `fbs/webRtcTransport.fbs`
- `fbs/webRtcServer.fbs`
- `generated/request_generated.h`
- `generated/worker_generated.h`
- `generated/webRtcTransport_generated.h`
- `docs/mediasoup-worker-architecture-analysis_cn.md`
