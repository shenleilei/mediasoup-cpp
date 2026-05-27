# Hawkeye WebSocket 注册联调最终报告

## 结论

本次 `mediasoup-cpp` 对 Hawkeye 的 WebSocket 注册改造已完成 staging 联调，结果通过。

已验证通过的能力：

- `mediasoup-cpp` 启动后可自动探测公网 IP
- `mediasoup-cpp` 可通过 `HAWKEYE_REGISTER_URL` 自动连接 Hawkeye 的 `/register_ws`
- Hawkeye 可在连接建立后完成注册，并在断链后执行注销清理
- 注册节点会写入 Redis 的 `server:{ip:port}` 记录
- 车辆上报可正确分配到在线 `mediasoup` 节点
- 同一车辆切换到另一个 Hawkeye 实例后仍可复用原 `video_server`
- 节点断开后，Redis 中对应服务记录会被删除，车辆会在再次上报时重新分配

## 部署方式

本次不是 `push` 到仓库后再拉取，而是：

1. 在本地构建镜像
2. 通过 `docker save | ssh | docker load` 传到 staging 机
3. 在 staging 机上用 `docker run` 直接替换运行中的容器

staging 机器：

- `172.31.4.40`

## 测试组件

### Hawkeye

- `hawkeye1`
- `hawkeye2`

### mediasoup

- `mediasoup-h1`
- `mediasoup-h2`
- `mediasoup-h3`

## 关键结果

### 1. 注册通过

3 个 `mediasoup-cpp` 实例都成功注册到 Hawkeye，Redis 中可见：

- `server:14.103.165.183:1770`
- `server:14.103.165.183:1771`
- `server:14.103.165.183:1772`

每个节点都带有：

- `heartbeat`
- `server_type=mediasoup`

### 2. 车辆选服通过

车辆上报后可以正确分配到在线 `mediasoup` 节点。

实际结果：

- `CAR-001` 首次分配到 `14.103.165.183:1770`
- `CAR-002` 分配到 `14.103.165.183:1772`
- `CAR-003` 分配到 `14.103.165.183:1771`

### 3. 双 Hawkeye 一致性通过

同一辆车先通过 `hawkeye1`，几秒后再通过 `hawkeye2` 上报，仍然命中同一个 `video_server`。

实际结果：

- `CAR-001` 在 `hawkeye1` 和 `hawkeye2` 上都复用了 `14.103.165.183:1770`

### 4. 断链注销通过

停止 `mediasoup-h1` 后：

- Hawkeye 日志中出现 `register websocket closed`
- Redis 中 `server:14.103.165.183:1770` 被删除
- 车辆侧旧的 `video_server` 反向引用被清理

### 5. 异常重分配通过

`mediasoup-h1` 下线后，重新上报 `CAR-001`：

- 返回的 `video_server` 变为 `14.103.165.183:1772`
- Redis 中 `CAR-001` 的 `video_server` 同步更新为 `server:14.103.165.183:1772`

## 代码约束确认

- `listenIp` 已固定为内部默认 `0.0.0.0`
- 不再对外暴露手工 `announcedIp`
- 注册地址由程序启动时自动探测公网 IP 得到
- 公网 IP 探测失败会直接启动失败

## 最终判定

本次新增的 Hawkeye WebSocket 注册能力已经完成端到端验证，覆盖：

- 注册
- 注销
- heartbeat 刷新
- 车辆分配
- 同车复用
- 双 Hawkeye 一致性
- 节点下线后的重新分配

可以进入后续发布准备。
