# mediasoup-cpp 信令 HTTPS/WSS 实施计划

## 1. 执行结论

本次实现按固定证书路径、强制 HTTPS/WSS 的方案落地：

```text
/opt/mediasoup-cpp/certs/tls.pem
/opt/mediasoup-cpp/certs/tls.key
```

mediasoup-cpp 启动后只提供 HTTPS/WSS，不保留 HTTP/WS fallback。证书或私钥缺失、不可读、格式错误或不匹配时，进程启动失败。

本次不改：

- Hawkeye 注册出站链路：`hawkeyeRegisterUrl=ws://.../register_ws`
- Hawkeye 注册 payload 中的 `server=announcedIp:signalingPort`
- Redis RoomRegistry / `nodeAddress`
- mediasoup-worker 媒体面 DTLS 证书

## 2. 前置确认

实施前必须确认：

1. 已申请测试环境证书。
2. 证书 SAN 覆盖浏览器实际访问 mediasoup-cpp 的域名。
3. 证书文件交付为 PEM 证书链和未加密 PEM 私钥。
4. Hawkeye 或调用方最终给浏览器的 mediasoup 信令地址是：

```text
wss://<mediasoup-domain>/ws
```

如果 Hawkeye 当前把 mediasoup 注册的 `server=ip:port` 直接拼成 `ws://ip:port/ws`，本次 mediasoup-cpp 改造上线后该路径会失败，需要在调用链侧同步改成 WSS 地址。

## 3. 改动范围

### 3.1 代码文件

预计修改：

- `src/SignalingServer.h`
- `src/SignalingServer.cpp`
- `src/SignalingServerHttp.h`
- `src/SignalingServerHttp.cpp`
- `src/SignalingServerWs.h`
- `src/SignalingServerWs.cpp`
- `src/main.cpp`
- `public/qos-demo.js`

### 3.2 部署文件

预计修改：

- `Dockerfile`
- `docker/entrypoint.sh`，如需要启动前打印证书信息
- `build_image.sh`，如需要把证书路径作为构建前置检查
- `README.md`

不改 `config.example.json`，因为本方案不新增运行时配置项。

## 4. 任务拆分

### Task 1：定义固定 TLS 路径

新增固定常量，建议放在 `SignalingServer.h` 或独立小头文件：

```cpp
constexpr const char* kSignalingTlsCertFile = "/opt/mediasoup-cpp/certs/tls.pem";
constexpr const char* kSignalingTlsKeyFile  = "/opt/mediasoup-cpp/certs/tls.key";
```

验收：

- 常量只定义一处。
- 日志、文件检查、`uWS::SSLApp` 都复用同一组常量。

### Task 2：启动前证书文件检查

在主流程启动 `SignalingServer` 前检查：

- `tls.pem` 存在。
- `tls.key` 存在。
- 两者都是 regular file。
- 当前进程可读。

建议实现为小函数：

```cpp
bool ValidateSignalingTlsFiles(std::string& error);
```

证书和私钥是否匹配由 `uWS::SSLApp` / OpenSSL 加载验证。

验收：

- 缺 `tls.pem` 时进程返回非 0。
- 缺 `tls.key` 时进程返回非 0。
- 文件不可读时进程返回非 0。
- 日志明确打印缺失或不可读路径。

### Task 3：SignalingServer 改为强制 SSLApp

当前 `SignalingServer::run()` 创建 `uWS::App`。改为：

```cpp
uWS::SocketContextOptions opts;
opts.key_file_name = kSignalingTlsKeyFile;
opts.cert_file_name = kSignalingTlsCertFile;
uWS::SSLApp app(opts);
```

不再创建明文 `uWS::App`。

验收：

- 进程只接受 HTTPS/WSS。
- `http://host:port/healthz` 不能作为成功路径。
- `https://host:port/healthz` 成功。

### Task 4：HTTP route 适配 SSL

把 `SignalingServerHttp::RegisterHttpRoutes(uWS::App&, ...)` 改成接收 `uWS::SSLApp&`。

`StaticFileResponder` 已支持 `uWS::HttpResponse<SSL>`，HTTP 静态文件 route 需要确保调用 SSL 版本：

```cpp
ServeResolvedFile<true>(res, *resolved, ContentTypeForPath(url));
```

如果编译器可以从 `res` 推导模板参数，可以保留当前调用；否则显式写 `<true>`。

验收：

- `/` 和 `/index.html` 可通过 HTTPS 访问。
- `/api/resolve` 可通过 HTTPS 访问。
- `/api/node-load`、`/healthz`、`/readyz`、`/metrics` 可通过 HTTPS 访问。

### Task 5：WebSocket route 适配 SSL

把 `SignalingServerWs::RegisterWebSocketRoutes(uWS::App&, ...)` 改成接收 `uWS::SSLApp&`。

业务 lambda 使用 `auto* ws`，大部分逻辑不需要改。重点确认这些 helper 能接受 `uWS::WebSocket<true, ...>*`：

- `PostNotify`
- `PostBroadcast`
- `HasMappedSession`
- `WsMap`
- `PerSocketData`

验收：

- 浏览器可连接 `wss://<host>:<port>/ws`。
- 加入房间、创建 transport、produce、consume 正常。

### Task 6：启动日志与失败日志

成功日志改为明确 HTTPS/WSS：

```text
SignalingServer listening on port 9000 [scheme:https/wss cert:/opt/mediasoup-cpp/certs/tls.pem]
```

失败日志需要覆盖：

- 缺证书。
- 缺私钥。
- 证书不可读。
- 私钥不可读。
- `SSLApp.listen()` 失败。

验收：

- 运维仅看日志能判断当前是 HTTPS/WSS。
- 缺证书失败时日志包含固定路径。

### Task 7：前端 fallback URL 修正

当前 `public/qos-demo.js` 的 fallback 使用：

```js
`${proto}://${location.hostname}:${location.port || 3000}/ws`
```

改为：

```js
`${proto}://${location.host}/ws`
```

避免 HTTPS 默认 443 场景下错误拼出 `:3000`。

验收：

- `https://domain/` 打开 demo 时，WebSocket URL 为 `wss://domain/ws`。
- `https://domain:9000/` 打开 demo 时，WebSocket URL 为 `wss://domain:9000/ws`。

### Task 8：Docker 镜像固定证书路径

Dockerfile runtime stage 创建证书目录：

```dockerfile
RUN chmod +x /usr/local/bin/mediasoup-sfu-entrypoint \
  && mkdir -p /var/log/mediasoup \
  && mkdir -p /opt/mediasoup-cpp/certs
```

证书发布方式按实际交付决定：

1. 若证书文件进入构建上下文，则 Dockerfile 复制到固定路径。
2. 若证书由发布流水线注入，则发布脚本在构建前放到固定路径，再构建镜像。

计划中的固定目标路径必须是：

```text
/opt/mediasoup-cpp/certs/tls.pem
/opt/mediasoup-cpp/certs/tls.key
```

验收：

- 镜像内存在两个文件。
- `openssl x509 -in /opt/mediasoup-cpp/certs/tls.pem -noout -dates` 可执行。
- 文件权限不阻止运行用户读取。

### Task 9：发布前证书检查

在发布脚本或人工 checklist 中执行：

```bash
openssl x509 -in certs/tls.pem -noout -subject -issuer -dates -ext subjectAltName
openssl x509 -in certs/tls.pem -noout -modulus | openssl md5
openssl rsa -in certs/tls.key -noout -modulus | openssl md5
```

第二、三条输出必须一致。

验收：

- 发布日志包含证书 `notAfter`。
- 证书和私钥 modulus 校验一致。
- 过期证书不得进入镜像。

### Task 10：README 更新

更新内容：

- 信令端口现在是 HTTPS/WSS。
- 固定证书路径。
- 缺证书启动失败。
- Hawkeye 注册出站仍是 `ws://.../register_ws`。
- Hawkeye/调用方给浏览器的 mediasoup 地址必须是 `wss://.../ws`。

验收：

- README 没有继续暗示信令服务默认 HTTP/WS 可用。
- 本地开发启动说明包含如何放测试证书。

## 5. 验证计划

### 5.1 构建验证

```bash
cmake --build build --target mediasoup-sfu -j"$(nproc)"
```

如果当前 build 目录不存在：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build --target mediasoup-sfu -j"$(nproc)"
```

### 5.2 缺证书失败验证

在没有固定路径证书的环境执行：

```bash
./build/mediasoup-sfu --nodaemon --port=9000 --noRedisRequired
```

预期：

- 进程退出非 0。
- 日志包含缺失的 `/opt/mediasoup-cpp/certs/tls.pem` 或 `tls.key`。

### 5.3 自签名测试证书验证

仅用于本地功能验证：

```bash
mkdir -p /opt/mediasoup-cpp/certs
openssl req -x509 -newkey rsa:2048 -nodes \
  -keyout /opt/mediasoup-cpp/certs/tls.key \
  -out /opt/mediasoup-cpp/certs/tls.pem \
  -days 1 \
  -subj "/CN=localhost"
```

启动：

```bash
./build/mediasoup-sfu --nodaemon --port=9000 --noRedisRequired
```

验证：

```bash
curl -k https://127.0.0.1:9000/healthz
curl -k https://127.0.0.1:9000/readyz
curl -k https://127.0.0.1:9000/api/node-load
```

### 5.4 正式测试证书验证

使用申请到的测试域名证书构建镜像或放入固定路径后验证：

```bash
openssl x509 -in /opt/mediasoup-cpp/certs/tls.pem -noout -subject -issuer -dates -ext subjectAltName
curl https://<mediasoup-domain>:<port>/healthz
```

浏览器验证：

- 打开 `https://<mediasoup-domain>:<port>/`。
- DevTools Network 中 WebSocket 是 `wss://<mediasoup-domain>:<port>/ws`。
- 无证书警告。

### 5.5 WebRTC 功能回归

至少覆盖：

- 单人加入房间。
- 双人加入同一房间。
- 发布摄像头/屏幕流。
- 订阅远端流。
- `connectWebRtcTransport` 成功。
- worker stats 中 `dtlsState` 到 `connected`。
- 断开重连不回退到 `ws://`。

### 5.6 Hawkeye 回归

保持原注册链路：

```text
hawkeyeRegisterUrl=ws://<hawkeye-host>:<port>/register_ws
```

验证：

- mediasoup-cpp 仍能注册到 Hawkeye。
- Hawkeye 能看到 `type=mediasoup` 的 server。
- Hawkeye/调用方最终给浏览器的 mediasoup 信令 URL 是 `wss://.../ws`。
- 车端/业务分配流程不因为 mediasoup-cpp 强制 WSS 回归。

## 6. 上线检查

上线前 checklist：

- [ ] 证书 SAN 覆盖访问域名。
- [ ] 证书未过期，发布日志记录 `notAfter`。
- [ ] 证书和私钥匹配。
- [ ] 镜像内存在 `/opt/mediasoup-cpp/certs/tls.pem`。
- [ ] 镜像内存在 `/opt/mediasoup-cpp/certs/tls.key`。
- [ ] 缺证书场景启动失败已验证。
- [ ] HTTPS `/healthz` 成功。
- [ ] WSS `/ws` 成功。
- [ ] 双人 WebRTC 互通成功。
- [ ] Hawkeye 注册仍成功。
- [ ] Hawkeye/调用方不会下发 `ws://.../ws`。

## 7. 回滚方案

由于本次不保留 HTTP/WS fallback，回滚方式是回滚镜像版本。

回滚前提：

- 保留上一版 HTTP/WS 镜像 tag。
- Hawkeye/调用方如果已经切到 `wss://.../ws`，回滚旧镜像前需要同步切回旧访问方式，或者旧镜像前面有 TLS 终止层。

推荐灰度方式：

1. 新证书镜像以新 tag 发布。
2. 先单节点运行。
3. 用测试域名访问该节点。
4. Hawkeye 只给测试车辆/测试房间分配该节点。
5. 验证通过后再扩大范围。

## 8. 完成标准

本变更完成需要同时满足：

1. 代码只启动 HTTPS/WSS。
2. 固定路径证书缺失时启动失败。
3. Docker 镜像包含固定路径证书或发布流程能保证该路径存在。
4. 浏览器 demo 通过 HTTPS/WSS 完成双人 WebRTC 互通。
5. Hawkeye 注册和分配链路完成回归。
6. README 说明与实际行为一致。
