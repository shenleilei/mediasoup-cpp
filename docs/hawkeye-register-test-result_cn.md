# Hawkeye WebSocket 注册测试结果

## 测试目标

验证 `mediasoup-cpp` 新增的 Hawkeye WebSocket 注册链路是否可用，并确认：

- 容器镜像可以正常构建
- 启动时可以自动探测公网 IP
- `HawkeyeRegisterClient` 能主动连接 Hawkeye 的 `/register_ws`
- 注册帧里会携带自动探测到的公网地址

## 测试环境

- 代码目录：`/root/mediasoup-cpp`
- 镜像构建：`./build_image.sh`
- 本地 worker：使用仓库内置的 `/root/mediasoup-cpp/mediasoup-worker`
- Hawkeye 注册临时服务：`python3 + websockets`
- staging 联调环境：`172.31.4.40`
- staging Hawkeye：
  - `hawkeye1`
  - `hawkeye2`
- staging mediasoup：
  - `mediasoup-h1`
  - `mediasoup-h2`
  - `mediasoup-h3`

## 已完成测试

### 1. 本地镜像构建

执行：

```bash
cd /root/mediasoup-cpp
./build_image.sh
```

结果：

- 本地镜像 `mediasoup-cpp:sfu` 构建成功
- Docker 构建过程里，`mediasoup-sfu` 目标编译成功
- 构建产物标签：
  - `mediasoup-cpp:sfu`

### 2. 容器启动烟雾测试

执行：

```bash
docker run -d --rm --name mediasoup-smoke \
  -e MEDIASOUP_PORT=3003 \
  -e MEDIASOUP_RTC_MIN_PORT=8000 \
  -e MEDIASOUP_RTC_MAX_PORT=8002 \
  -e MEDIASOUP_REDIS_REQUIRED=0 \
  mediasoup-cpp:sfu
```

结果：

- 容器可以正常启动
- 日志显示：
  - 自动探测到公网 IP
  - Redis 不可用时会按 `redisRequired=false` 进入 local-only mode
  - `SignalingServer` 成功监听端口

### 3. Hawkeye WebSocket 注册烟雾测试

执行方式：

- 启动一个临时 WebSocket 服务器监听 `127.0.0.1:8765/register_ws`
- 启动 `mediasoup-cpp` 容器并设置：
  - `HAWKEYE_REGISTER_URL=ws://127.0.0.1:8765/register_ws`
  - `HAWKEYE_REGISTER_TYPE=mediasoup`

结果：

- 容器启动后成功连接临时 WebSocket 服务器
- 服务端收到注册帧：

```json
{"server":"14.103.157.236:3005","type":"mediasoup"}
```

- 说明：
  - `server` 字段使用的是自动探测到的公网 IP
  - 注册类型为 `mediasoup`

### 4. Staging 注册与分配联调

执行方式：

- 将本地镜像传输到 staging 机器后加载
- 在 staging 上重建并替换：
  - `hawkeye1`
  - `hawkeye2`
  - `mediasoup-h1`
  - `mediasoup-h2`
  - `mediasoup-h3`
- 使用 `HAWKEYE_REGISTER_URL=ws://127.0.0.1:30000/register_ws`
- `mediasoup-cpp` 使用自动探测到的公网 IP 作为注册地址
- 通过 `ws://172.31.4.40:30000/vehicle` 和 `ws://172.31.4.40:30001/vehicle` 发送车辆上报

结果：

- 3 个 `mediasoup-cpp` 都成功注册到 `hawkeye-server`
- Redis 中可见：
  - `server:14.103.165.183:1770`
  - `server:14.103.165.183:1771`
  - `server:14.103.165.183:1772`
- 车辆分配结果满足一致性和复用预期：
  - `CAR-001` 初次通过 `hawkeye1` 分配到 `14.103.165.183:1770`
  - `CAR-002` 分配到 `14.103.165.183:1772`
  - `CAR-003` 分配到 `14.103.165.183:1771`
  - `CAR-001` 再通过 `hawkeye2` 上报时仍复用 `14.103.165.183:1770`
- 停掉 `mediasoup-h1` 后：
  - `server:14.103.165.183:1770` 从 Redis 中删除
  - `hawkeye-server` 日志记录到 `register websocket closed`
  - `CAR-001` 的旧 `video_server` 反向引用被清理
- 重新上报 `CAR-001` 后：
  - 返回的 `video_server` 变为 `14.103.165.183:1772`
  - Redis 中 `CAR-001` 的 `video_server = server:14.103.165.183:1772`
  - `server:14.103.165.183:1772` 下新增了 `CAR-001` 字段

## 结论

- `mediasoup-cpp` 镜像构建通过
- 容器启动通过
- 公网 IP 自动探测通过
- Hawkeye WebSocket 注册通过
- 注册 payload 格式正确
- staging 上的车辆分配、复用、断链清理、异常重分配也已通过

## 注意事项

- 本轮测试中，`mediasoup-cpp` 读取的注册入口环境变量是 `HAWKEYE_REGISTER_URL`，不是 `MEDIASOUP_HAWKEYE_REGISTER_URL`
- 本轮已经覆盖 staging 车辆分配/复用/断链清理/异常重分配测试
- `listenIp` 和手工 `announcedIp` 外部入口已清理，运行时由程序自动探测公网 IP
