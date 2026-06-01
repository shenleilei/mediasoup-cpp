# 日志系统化审计

生成时间：`2026-06-01 CST`

## 1. 范围

本次审计覆盖：

- `src/` 下 SFU 主进程日志
- `src/mediasoup-worker-src/worker/` 下 worker 回灌日志
- 当前测试机落盘路径：
  - 容器内：`/var/log/agora/mediasoup`
  - 宿主机：`/var/log/agora/mediasoup/${docker-name}`

不覆盖：

- 第三方依赖未修改路径下的通用库日志设计
- 业务埋点统计本身的字段设计

## 2. 当前默认等级

- SFU 主进程默认：`info`
- worker 默认：`warn`
- 可通过环境变量覆盖：
  - `MEDIASOUP_LOG_LEVEL`
  - `MEDIASOUP_WORKER_LOG_LEVEL`
  - `MEDIASOUP_WORKER_LOG_TAGS`

## 3. 输出链路

### 3.1 当前结论

- SFU 主进程日志写入 `mediasoup-sfu_*.log`
- worker 日志通过 `Channel::processLog()` 回灌到主进程 logger，再写入同一个 `mediasoup-sfu_*.log`

### 3.2 证据

- `Channel.cpp`：
  - `case 'W': MS_WARN(workerLogger, "[pid:{}] {}", pid_, msg);`
  - `case 'E': MS_ERROR(workerLogger, "[pid:{}] {}", pid_, msg);`
- 测试机文件中已观察到：
  - `[Worker] [warning] [Channel.cpp:677] ...`
  - `[Worker] [error] [Channel.cpp:678] ...`

## 4. 日志点盘点

本次从 `src/` 中共扫描到约 `768` 处日志打点。

清单产物：

- [generated/logging-audit-inventory.json](./generated/logging-audit-inventory.json)

说明：

- 该清单记录了文件、行号、日志宏/函数类别、原始代码文本、所属模块
- 后续任何“降级/删减/补充”日志的改动，都应以该清单为底

### 4.1 高层模块分布

- `worker`: 570
- `Channel.cpp`: 27
- `WorkerThread.cpp`: 21
- `MainBootstrap.cpp`: 21
- `SignalingServerWs.cpp`: 20
- `Transport.cpp`: 17
- `Worker.cpp`: 12
- `RoomServiceMedia.cpp`: 12
- `Router.cpp`: 8
- `Logger.h`: 6
- `main.cpp`: 6

### 4.2 等级分布

- `MS_WARN_TAG`: 233
- `MS_DEBUG_TAG`: 187
- `MS_ERROR`: 98
- `MS_WARN_DEV`: 87
- `MS_WARN`: 80
- `SPDLOG_WARN`: 27
- `MS_SPDLOG_INFO`: 15
- `SPDLOG_ERROR`: 15
- `MS_SPDLOG_ERROR`: 10
- `MS_SPDLOG_WARN`: 7
- `SPDLOG_INFO`: 6
- `MS_INFO`: 3

## 5. 场景审计

### 5.1 启动与配置

保留为默认可见：

- `SFU starting`
- `SFU ready`
- `SignalingServer listening`
- `GeoRouter initialized`
- `Auto-detected public IP`
- `Auto-detected node geo`

保留为 `warn/error`：

- TLS 文件缺失
- 无法探测公网 IP
- `nodeId` 非法
- `webRtcServerPort` 非法
- `GeoRouter DB` 不存在且 fallback / init 失败

结论：

- 这一组整体合理，默认可保留。

### 5.2 连接与会话生命周期

保留为默认可见：

- `[join-ok]`
- `[ws-close]`
- `disconnected`
- `kicking old connection`

保留为 `warn/error`：

- stale request drop
- malformed websocket message
- leave rollback failed
- join reject

结论：

- 这一组是线上排障核心，默认保留合理。

### 5.3 worker 生命周期

保留为默认可见：

- `WorkerThread {} created`
- `worker died`
- `respawned worker`
- `respawn rate exceeded`
- `epoll_wait failed`
- `task exception`

结论：

- 这一组是线上稳定性核心，默认保留合理。

### 5.4 媒体建链

保留为默认可见：

- plain publish / subscribe connect failure
- createTransport validation failure
- auto-subscribe failure
- keyframe request failure

结论：

- 这一组是排查媒体不通、首帧不出、plain client 问题的核心，默认保留合理。

### 5.5 QoS / downlink / stats

保留为默认可见：

- automatic override / clear
- downlink planning failure
- stats collect failure

结论：

- 这一组和当前项目重点直接相关，默认保留合理。

### 5.6 高频噪音候选

以下日志不适合作为默认持续输出：

- `RTC::SimpleConsumer::SendRtpPacket() | simple consumer sending RTP ...`
- `RTC::NackGenerator::ReceivePacket() | late recovered packet not present in the NACK list ...`
- `RTC::RtpStreamRecv::OnNackGeneratorNackRequired() | producer recv large NACK burst ...`
- `webrtc::GoogCcNetworkController::OnRemoteBitrateReport() | Received REMB for packet feedback only GoogCC`

结论：

- 应作为开发级、实验级或显式打开的诊断日志，不适合长期默认刷盘。

## 6. 已执行的收敛

本轮已确认或完成：

- `mediasoup-sfu_*.log` 统一承载 SFU + worker 日志
- `Received REMB for packet feedback only GoogCC` 已从默认错误级降到开发级
- `simple consumer sending RTP ...` 已从更高噪音口径向开发级收敛

## 7. 缺口

本次还没有逐条完成的项目：

- `Channel.cpp` 里 trace close / read loop 相关 `warn` 是否全部应降级
- `SignalingServerWs.cpp` 某些 `error` 是否应拆成 `warn + context`
- worker 侧 `RTCP / ICE / BWE / SRTP` 类 `WARN_TAG` 是否需要再按标签细分
- 是否需要为关键业务动作补统一 trace id / room id / peer id / transport id 上下文

## 8. 建议

### 保持默认可见

- 启动/关闭
- 监听成功/失败
- join/leave/session 替换
- worker died / respawn
- plain / WebRTC 建链失败
- QoS override / clear
- stats 失败

### 默认下沉到开发级

- 逐包发送日志
- NACK 恢复细节
- REMB 兼容性噪音
- 无损路径里的频繁 RTCP/BWE 观察项

### 建议下一轮继续审计

1. `Channel.cpp`
2. `SignalingServerWs.cpp`
3. `WorkerThread.cpp`
4. worker 侧 `ICE/DTLS/SRTP/BWE/RTCP`

## 9. 本轮结论

当前日志系统已经具备：

- 统一落盘
- 关键故障可见
- 部分高频噪音已开始收敛

但**还未完成“逐条日志 fully reviewed”的终态**。本文件是第一轮系统化审计基线，后续应继续按场景推进第二轮和第三轮精修。

## 10. 第二轮审计矩阵

### 10.1 启动与配置

| 场景 | 当前是否有日志 | 默认等级 | 结论 | 动作 |
|---|---|---|---|---|
| 进程启动 | 有 | `info` | 合理 | 保留 |
| TLS 文件校验失败 | 有 | `error` | 合理 | 保留 |
| 公网 IP 探测失败 | 有 | `error` | 合理 | 保留 |
| Geo DB fallback | 有 | `warn` | 合理 | 保留 |
| 节点 geo 自动识别 | 有 | `info` | 合理 | 保留 |
| 监听端口成功 | 有 | `info` | 合理 | 保留 |
| WorkerThread 创建成功 | 有 | `info` | 合理 | 保留 |

### 10.2 连接与会话

| 场景 | 当前是否有日志 | 默认等级 | 结论 | 动作 |
|---|---|---|---|---|
| join 成功 | 有 | `info` | 关键审计点 | 保留 |
| ws 关闭 | 有 | `info` | 关键审计点 | 保留 |
| 旧连接被踢 | 有 | `info` | 关键审计点 | 保留 |
| stale request 丢弃 | 有 | `warn` | 合理 | 保留 |
| malformed websocket message | 有 | `warn` | 合理 | 保留 |
| leave / rollback leave 失败 | 有 | `error` | 合理 | 保留 |
| join reject | 有 | `warn` | 合理 | 保留 |

### 10.3 worker 生命周期

| 场景 | 当前是否有日志 | 默认等级 | 结论 | 动作 |
|---|---|---|---|---|
| worker died | 有 | `error` | 核心故障 | 保留 |
| worker respawned | 有 | `warn/info` | 合理 | 可考虑降为 `info` |
| respawn rate exceeded | 有 | `error` | 核心故障 | 保留 |
| epoll / timer / task exception | 有 | `error` | 核心故障 | 保留 |
| channel invalid fd / pipe close | 有 | `warn/error` | 合理 | 保留 |

### 10.4 媒体建链

| 场景 | 当前是否有日志 | 默认等级 | 结论 | 动作 |
|---|---|---|---|---|
| createTransport 参数校验失败 | 有 | `warn` | 合理 | 保留 |
| plainPublish connect 失败 | 有 | `warn` | 合理 | 保留 |
| plainSubscribe connect 失败 | 有 | `warn` | 合理 | 保留 |
| auto-subscribe 失败 | 有 | `error` | 合理 | 保留 |
| keyframe 请求失败 | 有 | `warn` | 合理 | 保留 |

### 10.5 QoS / downlink / stats

| 场景 | 当前是否有日志 | 默认等级 | 结论 | 动作 |
|---|---|---|---|---|
| automatic override / clear | 有 | `info` | 关键业务链路 | 保留 |
| downlink planning failed | 有 | `warn` | 合理 | 保留 |
| send/recv transport getStats failed | 有 | `warn` | 合理 | 保留 |
| producer/consumer getStats failed | 有 | `warn` | 合理 | 保留 |

### 10.6 高频噪音

| 日志 | 当前状态 | 审计结论 | 动作 |
|---|---|---|---|
| `simple consumer sending RTP ...` | 已下沉 | 默认不该可见 | 已收敛 |
| `late recovered packet not present ...` | dev 级 | 默认不该可见 | 保持 |
| `producer recv large NACK burst ...` | dev 级 | 默认不该可见 | 保持 |
| `Received REMB for packet feedback only GoogCC` | 已下沉 | 默认不该可见 | 已收敛 |

## 11. 第二轮修正建议

### 建议立即处理

1. `WorkerThread {} respawned worker [pid:{}]`
当前为 `warn`
建议：降为 `info`
原因：成功恢复不是异常。

2. `Channel.cpp` trace close / readLoop start / exit
当前为 `warn`
建议：降为 `debug`
原因：属于内部跟踪，不适合作为默认日志。

3. `SignalingServerWs.cpp`
对 `clientStats error` / `downlinkClientStats error`
建议：保留 `error`
原因：业务关键控制面异常。

### 建议后续处理

1. worker 侧 `ICE/DTLS/SRTP/RTCP/BWE`
逐类过一遍 `WARN_TAG`
目标：只保留真正代表异常或兼容性问题的默认告警。

2. `Transport/Consumer/Producer close request failed`
增加更多上下文字段：
- roomId
- peerId
- transportId / consumerId / producerId

3. `RoomServiceMedia` / `RoomServiceStats`
对失败日志统一字段风格，避免同类故障不同格式。
