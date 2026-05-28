# mediasoup-cpp 信令 HTTPS/WSS 改造设计

## 1. 背景

当前 mediasoup-cpp 的浏览器信令服务使用 `uWS::App` 监听 HTTP/WS。媒体面 WebRTC 已经通过 DTLS-SRTP 加密，但信令面如果仍走明文 WS，浏览器与 SFU 之间交换的 `dtlsParameters.fingerprints`、ICE 信息、房间操作等仍可能被中间人篡改或观察。

后续部署主要保持 `noredis` 模式。房间或任务应由 Hawkeye-server 选择具体 mediasoup-cpp 节点，客户端再直连该节点。因此本次不引入 Redis 房间路由，也不把 TLS 终止放到 SLB 上解决。

## 2. 术语边界

### 2.1 mediasoup-cpp 入站信令地址

这是浏览器或业务调用方连接 mediasoup-cpp 的地址：

```text
wss://<mediasoup-host>/ws
https://<mediasoup-host>/
```

本次 HTTPS/WSS 改造针对这一层。

### 2.2 Hawkeye 注册出站地址

这是 mediasoup-cpp 主动连接 Hawkeye-server 的地址：

```text
hawkeyeRegisterUrl = ws://<hawkeye-host>:<port>/register_ws
```

它是 mediasoup-cpp 到 Hawkeye-server 的出站注册通道。当前 `HawkeyeRegisterClient` 只支持 `ws://`，遇到 `wss://` 会拒绝。本次不改这个链路。

### 2.3 Hawkeye 注册 payload 中的 server 字段

当前 mediasoup-cpp 注册给 Hawkeye 的 `server` 字段由 `BuildRegisterServer()` 生成：

```text
announcedIp:signalingPort
```

本次不改这个格式，也不新增 `publicWsUrl` 或 `hawkeyeAdvertiseAddress`。后续如果 Hawkeye 分配协议需要返回完整 `wss://.../ws`，再作为独立变更处理。

### 2.4 nodeAddress

`nodeAddress` 是 Redis RoomRegistry 多节点模式下的房间 owner 地址。当前主路径是 `noredis`，本次不依赖也不改造 `nodeAddress`。

## 3. 目标

1. mediasoup-cpp 直接提供 HTTPS 和 WSS。
2. 进程启动时强制读取固定路径证书和私钥。
3. 证书或私钥不存在、不可读或加载失败时，进程启动失败。
4. 使用同一端口提供 HTTPS 静态资源/API 和 WSS `/ws`。
5. 证书随部署产物一起发布到镜像内固定路径。
6. 不改变 Hawkeye 注册出站链路和注册 payload 格式。
7. 不引入 SLB 粘性路由、Redis 房间路由或跨节点房间协调。

## 4. 非目标

1. 不改 `mediasoup-worker` DTLS 证书逻辑。
2. 不把媒体面 DTLS 证书改成公网 CA 证书。
3. 不实现 `HawkeyeRegisterClient` 的 `wss://` 出站注册。
4. 不修改 Hawkeye 分配 mediasoup server 的协议格式。
5. 不新增 Redis 依赖。
6. 不处理证书自动申请、自动续期或热更新。
7. 不保留 HTTP/WS fallback。
8. 不在本次实现 HTTP 到 HTTPS 的自动 301 跳转。

## 5. 目标部署形态

测试环境可以使用独立 mediasoup 域名和证书：

```text
https://mediasoup-test.welltransai.com/
wss://mediasoup-test.welltransai.com/ws
```

容器内证书路径示例：

```text
/opt/mediasoup-cpp/certs/tls.pem
/opt/mediasoup-cpp/certs/tls.key
```

证书随镜像或部署包发布，Dockerfile 或发布脚本负责把证书放到约定路径。mediasoup-cpp 只读取固定路径，不支持运行时改路径。

## 6. 配置设计

不新增 JSON、CLI 或环境变量配置项。固定路径：

```text
/opt/mediasoup-cpp/certs/tls.pem
/opt/mediasoup-cpp/certs/tls.key
```

语义：

- `tls.pem` 是站点证书和中间证书链，PEM 格式。
- `tls.key` 是未加密私钥，PEM 格式。
- 两个文件必须同时存在且可读。
- 文件缺失、权限不足、证书/私钥不匹配或 OpenSSL 加载失败时，进程启动失败。
- 本次不支持加密私钥 passphrase。

## 7. 代码改造方案

### 7.1 固定证书路径

新增固定常量：

```cpp
constexpr const char* kTlsCertFile = "/opt/mediasoup-cpp/certs/tls.pem";
constexpr const char* kTlsKeyFile  = "/opt/mediasoup-cpp/certs/tls.key";
```

`FinalizeRuntimeOptions()` 或 `main()` 启动前检查：

- 两个文件存在。
- 两个文件是 regular file。
- 当前进程可读。

最终证书/私钥匹配性由 `uWS::SSLApp` / OpenSSL 加载阶段验证；加载失败直接启动失败。

### 7.2 SignalingServer 构造

新增轻量配置结构：

```cpp
struct SignalingTlsOptions {
    std::string certFile = kTlsCertFile;
    std::string keyFile = kTlsKeyFile;
};
```

`SignalingServer` 构造函数增加该配置：

```cpp
SignalingServer(
    int port,
    std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
    RoomRegistry* registry,
    bool redisRequired,
    SignalingTlsOptions tlsOptions);
```

### 7.3 uWebSockets App 分支

当前代码：

```cpp
uWS::App app;
SignalingServerWs::RegisterWebSocketRoutes(app, ...);
SignalingServerHttp::RegisterHttpRoutes(app, ...);
app.listen(...);
app.run();
```

目标结构：

```cpp
uWS::SocketContextOptions opts;
opts.key_file_name = tlsOptions_.keyFile.c_str();
opts.cert_file_name = tlsOptions_.certFile.c_str();

uWS::SSLApp app(opts);
runApp(app, "https/wss");
```

`runApp()` 可用模板函数实现，避免复制路由注册和 timer 管理逻辑。不再创建 `uWS::App` 明文服务。

### 7.4 路由注册改为 SSL 版本

将现有接口从只接受 `uWS::App&` 改成 SSL route 注册：

```cpp
static void RegisterWebSocketRoutes(
    uWS::SSLApp& app,
    SignalingServer& server,
    ...);

static void RegisterHttpRoutes(
    uWS::SSLApp& app,
    SignalingServer& server,
    uWS::Loop* loop);
```

`StaticFileResponder` 已经是 `template <bool SSL>`，HTTP route 内部改为调用 SSL 版本：

```cpp
SignalingServerHttp::RegisterHttpRoutes<true>(sslApp, ...);
```

WebSocket route 的 lambda 使用 `auto* ws`，业务逻辑可基本保持不变，只需要函数入参从 `uWS::App&` 改成 `uWS::SSLApp&`。

### 7.5 日志

启动成功日志应明确协议：

```text
SignalingServer listening on port 3000 [scheme:https/wss cert:/path/cert.pem]
```

失败日志：

```text
SignalingServer failed: TLS certificate or private key missing
```

## 8. 前端兼容性

当前 `public/qos-demo.js` 已按页面协议选择 WebSocket 协议：

```js
const proto = location.protocol === 'https:' ? 'wss' : 'ws';
```

因此如果 demo 页面通过 HTTPS 打开，WebSocket 会自动使用 WSS。

需要检查 `/api/resolve` fallback 的端口拼接逻辑，避免 HTTPS 默认 443 时错误回落到 `:3000`。在 `noredis` 主路径下，推荐 fallback 直接使用当前 origin：

```js
const fallback = `${proto}://${location.host}/ws`;
```

这项属于本次 HTTPS/WSS 兼容修正。

## 9. 与 Hawkeye 的关系

本次不改变 mediasoup-cpp 向 Hawkeye-server 的注册方式：

```text
hawkeyeRegisterUrl 仍然是 ws://.../register_ws
server 字段仍然保持当前 announcedIp:signalingPort
```

Hawkeye-server 后续如果需要分配浏览器可直连的 WSS 地址，有两种后续方案：

1. 在 Hawkeye-server 侧根据 `server=ip:port` 自行拼接业务域名。
2. mediasoup-cpp 后续新增独立配置项注册完整公网地址。

这两项都不放入本次范围。

部署前必须确认 Hawkeye 分配链路不会把 `server=announcedIp:signalingPort` 直接拼成 `ws://announcedIp:signalingPort/ws` 给浏览器使用。改造后 mediasoup-cpp 只接受 HTTPS/WSS，继续使用明文 `ws://` 连接会失败。

如果 Hawkeye 当前确实依赖 `server=ip:port` 生成浏览器信令地址，本次上线需要同步在 Hawkeye 或调用方侧把该地址映射为：

```text
wss://<mediasoup-domain>/ws
```

这属于部署/调用链配置前提，不在 mediasoup-cpp 本次代码范围内。

## 10. 证书策略

### 10.1 文件格式

需要：

```text
cert.pem: 站点证书 + 中间证书链，PEM 格式
key: 私钥，PEM 格式
```

证书 SAN 必须覆盖浏览器实际访问 mediasoup-cpp 的域名。

### 10.2 发布方式

本次采用固定路径随部署产物发布：

```text
/opt/mediasoup-cpp/certs/tls.pem
/opt/mediasoup-cpp/certs/tls.key
```

代码只依赖上述两个文件存在且可读。

### 10.3 过期检查

本次服务启动不强制解析证书过期时间，但发布流程应执行：

```bash
openssl x509 -in cert.pem -noout -subject -issuer -dates -ext subjectAltName
```

测试机曾检查到仓库内置 `hawkeye-server/resource/server.pem` 已在 `2025-02-19 23:59:59 GMT` 过期，不能复用。

## 11. 验证计划

### 11.1 配置解析

覆盖：

- 固定路径存在 cert/key，服务以 HTTPS/WSS 启动。
- 缺少 cert，启动失败。
- 缺少 key，启动失败。
- cert/key 不可读，启动失败。
- cert/key 不匹配，启动失败。

### 11.2 HTTP/WSS 功能

手工验证：

```bash
curl -k https://127.0.0.1:3000/healthz
curl -k https://127.0.0.1:3000/readyz
curl -k https://127.0.0.1:3000/api/node-load
```

WebSocket 验证：

```text
wss://127.0.0.1:3000/ws
```

如果使用正式域名证书，浏览器应无证书警告。

### 11.3 WebRTC 回归

至少验证：

- 打开 HTTPS demo 页面。
- 加入房间。
- 创建 send/recv WebRtcTransport。
- `connectWebRtcTransport` 成功。
- `dtlsState` 到 `connected`。
- 双人同房间音视频互通。

### 11.4 Hawkeye 回归

由于本次不改 Hawkeye 注册链路，验证：

- mediasoup-cpp 仍能通过原 `hawkeyeRegisterUrl=ws://.../register_ws` 注册。
- Hawkeye-server 仍能看到 mediasoup 类型 server。
- Hawkeye 或调用方最终给浏览器的 mediasoup 信令地址必须是 `wss://.../ws`。
- 车端/业务分配逻辑不因 HTTPS/WSS 改造回归。

## 12. 风险与处理

### 12.1 uWS SSL 模板改造影响面

HTTP、WS route 当前接收 `uWS::App&`，改成 `uWS::SSLApp&` 后会暴露 `uWS::HttpResponse<true>` / `uWS::WebSocket<true>` 类型差异。

处理：

- 保持业务逻辑函数不改签名，只在路由绑定层适配。
- 优先把改动限制在 `SignalingServerHttp`、`SignalingServerWs`、`SignalingServer.cpp`。

### 12.2 证书域名不匹配

WSS 连接要求证书 SAN 覆盖访问域名。用 IP 访问正式域名证书会失败。

处理：

- 测试环境为 mediasoup 准备明确域名。
- 浏览器访问和 WSS 地址都使用该域名。

### 12.3 证书随镜像发布的轮换成本

证书放进镜像时，每次证书轮换都需要重新构建和发布镜像。

处理：

- 证书过期前必须重新构建和发布镜像。
- 发布流程必须打印证书 `notAfter`，避免过期证书进入镜像。

## 13. 实施顺序

1. 在镜像/部署产物中加入固定路径证书文件。
2. 新增固定证书路径常量和启动前文件检查。
3. 新增 `SignalingTlsOptions` 并传入 `SignalingServer`。
4. 将 HTTP/WS route 注册改为 SSL route。
5. `SignalingServer::run()` 改为只启动 `uWS::SSLApp`。
6. 修正 demo fallback WebSocket URL 使用 `location.host`。
7. 更新 Dockerfile、发布脚本和 README 固定证书路径说明。
8. 构建并验证缺证书时启动失败。
9. 用测试证书运行 HTTPS/WSS 回归。

## 14. 当前结论

本次改造应聚焦 mediasoup-cpp 入站信令加密：

```text
HTTP/WS  ->  HTTPS/WSS
```

不改变：

```text
mediasoup-cpp -> Hawkeye-server register_ws
Hawkeye 注册 payload
RoomRegistry/nodeAddress
media DTLS certificate
```

这样符合当前 `noredis` 常态，也不会把 Hawkeye 分配协议、Redis 房间路由和 TLS 改造混成一个大变更。
