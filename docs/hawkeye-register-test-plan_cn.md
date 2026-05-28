# Hawkeye WebSocket 注册联调测试文档

> 说明：本文档中的 Redis 字段和观测步骤反映的是历史联调环境，当前 `mediasoup-cpp` 主运行路径已经切换为 local-only。

本文档用于验证 `mediasoup-cpp` 新增的 Hawkeye WebSocket 注册能力，以及 `hawkeye-server` 对服务注册、注销和选服的行为。

## 1. 目标

- 验证 `mediasoup-cpp` 启动后会自动连接 `hawkeye-server` 的 `/register_ws`
- 验证节点注册后会写入 Redis 的 `server:{ip:port}` 记录
- 验证断链后服务会从 Redis 中删除
- 验证正常分配、重复分配、负载均衡、异常重分配都能覆盖

## 2. 版本要求

需要同时使用新版本镜像：

- `hawkeye-server` 新增 `/register_ws`
- `mediasoup-cpp` 新增 WebSocket 注册客户端

不能用旧镜像做完整闭环验证。

## 3. 镜像构建

### 3.1 构建 `hawkeye-server` 镜像

`hawkeye-server` 需要基于一个新的合并分支来打包，这个分支以 `feature/ts-slice-integration` 为底，再叠加当前 WebSocket 注册补丁。该分支里已经包含 `conf/staging.json`，后续测试机请优先使用这个配置。

在 `hawkeye-server` 仓库根目录执行：

```bash
./docker/build_img.sh
```

说明：

- 该脚本会本地构建 `hawkeye-server` 镜像
- 默认还会打版本号并推送到仓库
- 如果只想确认构建流程，可根据你们的镜像发布方式调整推送步骤

### 3.2 构建 `mediasoup-cpp` 镜像

在 `mediasoup-cpp` 仓库根目录执行：

```bash
./build_image.sh
```

如果测试机要求尽量不走网络依赖，使用：

```bash
./scripts/package_image.sh
```

说明：

- 这条测试路径是本地-only，**不要执行 `git submodule update --init --recursive`**
- 仓库里需要已经存在这些本地目录，否则脚本会直接报错：
  - `third_party/flatbuffers`
  - `third_party/uWebSockets`
  - `third_party/nlohmann_json`
  - `third_party/spdlog`
  - `third_party/ip2region`
  - `src/mediasoup-worker-src`
- 仓库根目录需要有可执行的 `./mediasoup-worker`
- `package_image.sh` 会要求本地已有 `ubuntu:20.04`
- `build_image.sh` 和 `Dockerfile` 都只检查本地目录是否完整，不会自动拉取远程 submodule
- 适合测试机或离线环境

## 4. 最小启动配置

### 4.1 `mediasoup-cpp` 配置

建议至少准备 3 个实例，分别设置不同端口和 `nodeId`，例如：

```json
{
  "port": 3000,
  "workers": 1,
  "workerThreads": 1,
  "workerBin": "./mediasoup-worker",
  "redisHost": "127.0.0.1",
  "redisPort": 6379,
  "nodeId": "mediasoup-1",
  "nodeAddress": "ws://<ip-1>:3000/ws",
  "hawkeyeRegisterUrl": "ws://<hawkeye-host>:8080/register_ws",
  "hawkeyeRegisterType": "mediasoup"
}
```

第二、第三个实例只需要改：

- `port`
- `nodeId`
- `nodeAddress`

如果同机多实例，注意端口不要冲突。

### 4.2 `hawkeye-server` 配置

测试机上直接使用合并分支里的 `conf/staging.json`。至少要确保：

- `redis.addr` 指向测试机上的真实 Redis
- `websocketServer.port` 保持可被 `mediasoup-cpp` 访问
- `hawkeye-server` 新版本已启动
- 车辆上报和选服逻辑仍然可工作

如果要覆盖最小的跨实例分配场景，建议额外起 2 个 `hawkeye-server` 实例：

- `hawkeye1`
- `hawkeye2`

两个实例必须连同一个 Redis，并使用同一套 `staging.json` 里相同的选服规则。

## 5. 启动顺序

1. 先启动测试机上的 Redis
2. 启动基于合并分支的新 `hawkeye-server`
3. 启动 3 个 `mediasoup-cpp` 实例
4. 观察每个 `mediasoup-cpp` 是否成功连上 `/register_ws`

启动顺序对这条注册链路不是强约束：

- 如果 `mediasoup-cpp` 先启动，`hawkeye-server` 后启动，`mediasoup-cpp` 会按固定间隔重连
- 当前注册客户端默认每 `5s` 重试一次
- 只要 `mediasoup-cpp` 进程还在，`hawkeye-server` 起来后就会自动补注册

注册成功后，Redis 中应能看到类似：

- `server:<ip-1>:3000`
- `server:<ip-2>:3000`
- `server:<ip-3>:3000`

## 6. 验证点

### 6.1 注册验证

检查：

- `mediasoup-cpp` 日志里是否出现注册成功
- `hawkeye-server` 日志里是否出现 `/register_ws` 收到请求
- Redis 中是否存在对应 `server:*` key

Redis 里至少应包含：

- `heartbeat`
- `server_type=mediasoup`

### 6.2 注销验证

停止任意一个 `mediasoup-cpp` 实例后，检查：

- `hawkeye-server` 是否检测到 websocket 关闭或 ping 失败
- 对应 `server:{ip:port}` 是否被删除
- 挂在该 server 上的车辆 `video_server` 是否被清理

补充说明：

- `register_ws` 不是一次性注册
- 服务端必须持续刷新 Redis 里的 `heartbeat`
- 否则 `GetAllServer` 会在 `ServerHeartBeatTimeout` 后把该节点当成过期节点剔除
- 公网 IP 采用启动时自动探测；如果探测失败，程序会直接启动失败
- 本次 staging 验证已确认：服务端在 websocket ping 成功后刷新 heartbeat，节点能在超时窗口之外继续保持可用

## 7. 分配测试用例

下面这些用例用于覆盖正常和异常分配。

### 用例 1: 正常注册和正常分配

步骤：

1. 启动 3 个 `mediasoup-cpp` 实例
2. 让车辆上报进入 `hawkeye-server`
3. 观察 `ChooseVideoServer` 分配结果

预期：

- 车辆能被分配到某个在线 `mediasoup` 节点
- 车辆的 `video_server` 会写回 Redis

### 用例 1A: 最小双 Hawkeye 一致性分配

这个用例是最少必须覆盖的场景。

步骤：

1. 启动 `hawkeye1` 和 `hawkeye2`
2. 启动 3 个 `mediasoup-cpp` 实例并完成注册
3. 通过 `hawkeye1` 让同一辆车完成第一次上报和分配
4. 等几秒后，通过 `hawkeye2` 再次让同一辆车上报

预期：

- `hawkeye2` 仍然能从 Redis 读到这辆车之前的 `video_server`
- 如果原 `mediasoup-sfu` 仍在线，`hawkeye2` 应复用同一个 `video_server`
- 不应该因为切到另一个 `hawkeye-server` 实例就给同一辆车换掉已存在的可用服务

### 用例 2: 均匀分配

步骤：

1. 保持 3 个 `mediasoup-cpp` 实例在线
2. 连续上报多个不同车辆

预期：

- `SelectServer` 会优先选择挂载车辆数较少的节点
- 分配结果应尽量分散，而不是始终落在同一个节点

### 用例 3: 同一车辆复用原节点

步骤：

1. 让同一车辆先分配到某个节点
2. 再次上报同一车辆

预期：

- 如果节点仍然可用，`hawkeye-server` 优先复用原 `video_server`
- 不应无故切到别的节点

### 用例 4: 节点断开后的重新分配

步骤：

1. 让车辆分配到某个 `mediasoup-cpp`
2. 停掉该 `mediasoup-cpp`
3. 让同一车辆再次上报

预期：

- `hawkeye-server` 已删除失效 `server:*`
- 车辆会重新分配到剩余在线节点

### 用例 5: 服务器类型不匹配

步骤：

1. 让车辆原本绑定到某个 `mediasoup` 节点
2. 模拟请求的 `serverType` 与缓存类型不一致

预期：

- 旧绑定会被清理
- 车辆会重新选服

### 用例 6: Reset 重分配

步骤：

1. 给车辆设置 `reset=true`
2. 让 `hawkeye-server` 重新处理上报

预期：

- 旧节点的重置次数会增加
- 车辆可能被重新分配到其他节点

### 用例 7: 全部节点满载时兜底

步骤：

1. 人为让多个节点车辆数接近上限
2. 再上报新车辆

预期：

- `SelectServer` 先找最小负载节点
- 如果都满了，会走随机兜底

## 8. Staging 实测结果

本次已在 staging 机器上完成实测，结论如下。

### 8.1 镜像与启动

- `hawkeye-server` 已使用合并后的新镜像启动，并通过 `bash /hawkeye-server/start_premise.sh -m staging` 正常加载 `conf/staging.json`
- `mediasoup-cpp` 已使用新镜像启动 3 个实例
- 旧的 `mediasoup-cpp hawk` 进程已停掉，避免和新版本混跑

### 8.2 注册链路

- 3 个 `mediasoup-cpp` 都成功连上 `hawkeye-server` 的 `/register_ws`
- Redis 中可见：
  - `server:14.103.165.183:1770`
  - `server:14.103.165.183:1771`
  - `server:14.103.165.183:1772`
- 每个 key 都包含：
  - `heartbeat`
  - `server_type=mediasoup`
- 注册上去的地址是 `pub_ip:port`，不是 `127.0.0.1`
- 本次 staging 里 `mediasoup-cpp` 的公网 IP 自动探测结果为 `14.103.165.183`

### 8.3 心跳刷新

- 之前只做首包注册会导致节点在 `ServerHeartBeatTimeout` 后被误判过期
- 已补充为：服务端在 websocket ping 成功后刷新 Redis 的 `heartbeat`
- 实测中，节点在超时窗口之外仍能保持可用，不会被 `GetAllServer` 错删
- 该项在 staging 上已验证通过，3 个节点在 65 秒后仍保持可用

### 8.4 注销与清理

- 停掉任意一个 `mediasoup-cpp` 实例后，`hawkeye-server` 能检测到 websocket 关闭
- 对应 `server:{ip:port}` 会从 Redis 中删除
- 该 server 下挂着的车辆 `video_server` 反向引用也会被清理
- staging 实测：
  - 停掉 `mediasoup-h1` 后，`server:14.103.165.183:1770` 从 Redis 删除
  - `hawkeye-server` 日志记录到 `register websocket closed`
  - `CAR-001` 的旧 `video_server` 反向引用被清理
- 重新拉起该节点后会重新注册回来

### 8.5 车辆选服与复用

- 同一辆车先通过 `hawkeye1` 上报，再通过 `hawkeye2` 重复上报
- 两次都命中了同一个 `mediasoup-sfu`
- 本次实际分配结果为：
  - `CAR-001` 初次分配到 `14.103.165.183:1770`
  - `CAR-002` 分配到 `14.103.165.183:1772`
  - `CAR-003` 分配到 `14.103.165.183:1771`
- 双 Hawkeye 一致性验证：
  - `CAR-001` 再通过 `hawkeye2` 上报时仍复用 `14.103.165.183:1770`
- Redis 回写结果：
  - 车辆 hash 的 `video_server = server:14.103.165.183:1770`
  - `server:14.103.165.183:1770` 下挂有对应车辆字段
- 异常重分配验证：
  - `mediasoup-h1` 停掉后，`CAR-001` 再次上报
  - 返回的 `video_server` 变为 `14.103.165.183:1772`
  - Redis 中 `CAR-001` 的 `video_server` 同步更新为 `server:14.103.165.183:1772`

### 8.6 结论

- 注册通过
- 注销通过
- heartbeat 刷新通过
- 车辆选服通过
- 双 `hawkeye-server` 一致性通过
- 异常节点下线后的重新分配通过

当前实现满足本次新增的 WebSocket 注册/注销需求，且不影响原有选服逻辑。

## 8. Redis 检查项

重点看这些 key：

- `server:{ip:port}`
- `vehicle` 对应的 `video_server`
- `server:{ip:port}` hash 下的车辆字段

需要确认：

- 注册时写入 `heartbeat`
- 注销时删除 `server:{ip:port}`
- 注销时清掉车辆侧的 `video_server`

## 9. 建议的最小测试矩阵

建议至少按下面规模做一次联调：

- `hawkeye-server` 1 个
- `mediasoup-cpp` 3 个普通节点

这样能覆盖：

- 注册
- 注销
- 均匀分配
- 复用原节点
- 异常重分配
  - 负载均衡与异常回收

## 10. 结论判定

满足以下条件即可认为链路通过：

- 3 个 `mediasoup-cpp` 都能注册到 `hawkeye-server`
- 断开任意一个节点后，Hawkeye 能及时删除对应注册
- 车辆正常请求能分配到在线节点
- 重复请求、reset、断链重分配都符合现有选服逻辑
