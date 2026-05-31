# Monitoring & Alerting Runbook

本仓库提供了一个可直接启动的“分层监控 + 统一告警 + 分级升级”模板，位于：

- `deploy/monitoring`

## 1. 监控目标与分层

### 目标（SLI/SLO导向）
- 可用性：探活成功率、核心接口可达性
- 性能：延迟、吞吐、错误率、资源占用
- 业务：核心 KPI（需接入业务埋点）
- 响应：MTTA、MTTR、确认率

### 分层
1. 机器层：CPU/内存/磁盘/网络
2. 进程层：`mediasoup-sfu`、`mediasoup-worker` 进程存活与重启
3. 应用层：HTTP 可达性、接口探活（黑盒）
4. 业务层：预留 Grafana 业务看板模板
5. 日志与链路：Loki + Alloy（链路追踪建议后续补充 OTel + Jaeger/Tempo）

## 2. 组件与职责

| 组件 | 职责 | 数据模式 | 部署位置 |
|------|------|---------|---------|
| Prometheus | 采集数值指标（CPU、内存、磁盘等），评估阈值告警规则 | 拉模式：定时从各节点的 Exporter 拉取 `/metrics` | 监控中心 |
| Loki | 存储日志，评估日志关键字告警规则 | 推模式：接收 Alloy 推送的日志 | 监控中心 |
| Alertmanager | 接收 Prometheus 和 Loki 触发的告警，按严重级别路由到不同通知渠道 | — | 监控中心 |
| Grafana | 统一可视化面板，查询 Prometheus 指标和 Loki 日志 | — | 监控中心 |
| Alloy | 采集本机日志文件并推送到远程 Loki | 推模式：主动推送 | 每台 SFU 节点 |
| Node Exporter | 暴露本机系统指标（CPU/内存/磁盘/网络），等待 Prometheus 来拉 | 拉模式：被动暴露 `:9100/metrics` | 每台 SFU 节点 |
| Process Exporter | 暴露进程级指标（进程存活、重启检测） | 拉模式：被动暴露 `:9256/metrics` | 每台 SFU 节点 |
| Blackbox Exporter | HTTP/TCP 探活，检测服务可达性 | 拉模式 | 监控中心 |

### 数据流向

```
每台 SFU 节点                                               监控中心
┌───────────────────────────────────────┐                ┌──────────────────────────────────────┐
│ mediasoup-sfu                         │                │ Prometheus                           │
│   ├─ /healthz                         │──blackbox────→ │   ├─ 抓 node-exporter / /metrics     │
│   ├─ /readyz                          │                │   ├─ 评估 Prometheus 告警规则        │
│   ├─ /metrics                         │←────pull────── │   └─ 发告警到 Alertmanager           │
│   └─ /api/node-load                   │                │                                      │
│                                       │                │ Loki                                 │
│ Node Exporter :9100                   │←────pull────── │   ├─ 接收 Alloy 推送日志             │
│                                       │                │   ├─ 评估 Loki 日志告警规则          │
│ Alloy                                 │────push logs→  │   └─ 发告警到 Alertmanager           │
│   ├─ /var/log/mediasoup-cpp/*.log     │                │                                      │
│   └─ /var/lib/docker/containers/*     │                │ Alertmanager                         │
└───────────────────────────────────────┘                │   └─ 路由 → 企业微信/钉钉/电话      │
                                                         │                                      │
                                                         │ Grafana                              │
                                                         │   └─ 展示服务/基础设施/业务看板     │
                                                         └──────────────────────────────────────┘
```

## 3. 多节点部署

### 部署文件

| 文件 | 用途 | 部署位置 |
|------|------|---------|
| `docker-compose.center.yml` | Loki、Prometheus、Alertmanager、Grafana、Blackbox | 监控中心（1台） |
| `docker-compose.node.yml` | Alloy、Node Exporter | 每台 SFU 节点 |
| `docker-compose.yml` | 单机全量版（开发/测试用） | 单机 |
| `alloy/config.node.alloy` | 节点版 Alloy 配置，推送到远程 Loki | 每台 SFU 节点 |

### 监控中心启动

```bash
cd deploy/monitoring
docker compose -f docker-compose.center.yml up -d
```

### SFU 节点启动

将 `docker-compose.node.yml` 和 `alloy/config.node.alloy` 分发到每台 SFU 节点，替换监控中心 IP 后启动：

```bash
sed -i 's/<MONITOR_CENTER_IP>/10.x.x.x/g' alloy/config.node.alloy
docker compose -f docker-compose.node.yml up -d
```

### Prometheus 添加节点

在监控中心的 `prometheus/prometheus.yml` 中添加所有 SFU 节点的 Node Exporter 地址：

```yaml
- job_name: node
  static_configs:
    - targets:
        - 10.x.x.1:9100
        - 10.x.x.2:9100
        # ... 所有 SFU 节点
```

### 单机启动（开发/测试）

```bash
cd deploy/monitoring
docker compose up -d
```

默认入口：
- Grafana: `http://localhost:3001`（admin/admin）
- Prometheus: `http://localhost:9090`
- Alertmanager: `http://localhost:9093`

## 4. 告警规则详解

### 两类告警来源

| 来源 | 监控对象 | 规则文件 |
|------|---------|---------|
| Prometheus | 数值指标超阈值 | `prometheus/rules/mediasoup-alerts.yml` |
| Loki | 日志关键字匹配 | `loki/rules/fake/alerts.yml` |

两者触发的告警都发给同一个 Alertmanager，走同样的通知渠道。

### Prometheus 告警（指标阈值）

| 告警 | 触发条件 | 级别 | 数据来源 |
|------|---------|------|---------|
| `SfuHttpEndpointDown` | HTTP 探活失败 > 2分钟 | P1 | Blackbox Exporter |
| `SfuHealthEndpointDown` | `/healthz` 探活失败 > 1分钟 | P1 | Blackbox Exporter |
| `SfuReadyEndpointDown` | `mediasoup_sfu_ready == 0` 持续 1 分钟 | P1 | `/metrics` |
| `SfuNoAvailableWorkers` | `mediasoup_sfu_has_available_workers == 0` 持续 1 分钟 | P1 | `/metrics` |
| `SfuWorkerThreadCapacityLow` | `mediasoup_sfu_available_worker_threads < 1` 持续 1 分钟 | P1 | `/metrics` |
| `NodeCpuHigh` | CPU > 85% 持续 10分钟 | P2 | Node Exporter |
| `NodeMemoryPressure` | 内存 > 90% 持续 10分钟 | P2 | Node Exporter |
| `NodeDiskPressure` | 磁盘 > 85% 持续 15分钟 | P2 | Node Exporter |
| `MediasoupProcessDown` | SFU 进程消失 > 2分钟 | P1 | Process Exporter |
| `WorkerCrashSuspected` | worker 进程重启 | P2 | Process Exporter |
| `SfuJoinFailuresDetected` | 5分钟内 `join` 失败数增加 | P2 | `/metrics` |
| `SfuPlainPublishFailuresDetected` | 5分钟内 `plainPublish` 失败数增加 | P2 | `/metrics` |
| `SfuPlainSubscribeFailuresDetected` | 5分钟内 `plainSubscribe` 失败数增加 | P2 | `/metrics` |
| `SfuWorkerDeathsDetected` | 5分钟内 worker death 计数增加 | P1 | `/metrics` |
| `SfuWorkerRespawnsDetected` | 5分钟内 worker respawn 计数增加 | P2 | `/metrics` |
| `SfuMalformedWebSocketMessagesHigh` | 5分钟内 malformed websocket message > 10 | P2 | `/metrics` |
| `SfuWorkerThreadQueueDepthHigh` | worker thread queue depth > 100 持续 5 分钟 | P2 | `/metrics` |
| `SfuWorkerThreadTaskWaitHigh` | worker thread avg task wait > 50ms 持续 5 分钟 | P2 | `/metrics` |

### Loki 告警（日志关键字）

采集的日志文件：`/var/log/mediasoup-cpp/*.log`（Alloy job=`mediasoup-sfu`，包括 `mediasoup-sfu_*`、`container.stdout.log`、`container.stderr.log`）

#### 单机维度（按 host 分组，互不干扰）

| 告警 | 触发条件 | 级别 |
|------|---------|------|
| `LogErrorDetected` | 某台机器5分钟内出现 error | P2 |
| `LogErrorRateHigh` | 某台机器5分钟 error > 10 次 | P1 |
| `WorkerCrashInLog` | 某台机器出现 worker crash/killed/segfault | P1 |
| `JoinFailuresInLog` | 某台机器5分钟内出现 `[join-failed]` | P2 |
| `PlainPublishFailuresInLog` | 某台机器5分钟内出现 `plainPublish.*failed` | P2 |
| `PlainSubscribeFailuresInLog` | 某台机器5分钟内出现 `plainSubscribe.*failed` | P2 |
| `WebSocketCloseRateHigh` | 某台机器5分钟内 `[ws-close]` 数量过高 | P2 |
| `MalformedWebSocketMessagesInLog` | 某台机器5分钟内 malformed websocket message > 10 | P2 |

#### 集群维度（跨所有节点聚合）

| 告警 | 触发条件 | 级别 |
|------|---------|------|
| `ClusterErrorRateHigh` | 全集群5分钟 error 总量 > 50 | P1 |
| `ClusterMultiNodeErrors` | 同时 > 2 台节点报错 | P1 |
| `ClusterWorkerCrash` | 全集群 worker crash > 2 次 | P1 |

> 集群告警依赖所有节点的 Alloy 都推送到同一个 Loki，由 Loki ruler 在中心端跨节点聚合计算。

### 告警级别与通知渠道

| 级别 | 通知渠道 | 说明 |
|------|---------|------|
| P1 | 企业微信 + 钉钉 + 电话升级 | 立即响应 |
| P2 | 企业微信 + 钉钉 | 重要异常 |
| P3 | 企业微信 | 趋势风险 |

通知通过 Alertmanager webhook 发送到中继服务（`localhost:18080`），需部署 relay 服务对接实际的企业微信/钉钉机器人 webhook。

## 5. 告警分级与升级策略

已内置 P1/P2/P3 示例：
- P1：服务不可用、核心进程退出（立即升级）
- P2：高资源占用、worker 重启等重要异常
- P3：趋势风险（如磁盘未来将满）

Alertmanager 已内置：
- 分级路由
- P1 抑制低级别噪声
- 企业微信/钉钉 webhook 中继占位
- 电话告警升级占位（`/notify/phone/p1`）

> 生产环境建议使用统一“告警中继服务”对接企业微信、钉钉、工单系统与电话平台，避免在 Alertmanager 中直接暴露密钥。

## 6. 面板体系（Grafana）

Grafana 是整个监控栈的可视化层，将 Prometheus 指标和 Loki 日志以图表形式展示。访问地址：`http://<监控中心IP>:3001`（admin/admin）。

已预置四类看板：

| 看板 | 用途 | 典型场景 |
|------|------|---------|
| Global Overview | 全局概览：可用性、节点健康、房间数、失败率、告警分布 | 日常巡检，一眼判断集群是否正常 |
| Infrastructure | 机器指标 + 进程/worker/room 水位 | 收到 `NodeCpuHigh` 或 worker 可用性告警后定位资源层问题 |
| Service SLI/SLO | 服务质量：healthy/ready、失败率、worker death/respawn burn | 评估 SFU 服务是否正在退化 |
| Business KPI | 当前运行 KPI：rooms/workers/available threads/请求失败/断连 | 值班排障时快速看业务层信号 |

各组件分工：
- Prometheus / Loki — 收数据、算告警
- Alertmanager — 发通知
- Grafana — 可视化展示，用于趋势分析、历史对比、故障排查

## 7. 实施节奏建议

1. 第 1 阶段：机器层 + 进程层 + IM 告警
2. 第 2 阶段：应用层探活 + 统一服务大盘 + 日志告警
3. 第 3 阶段：业务 KPI 告警 + 链路追踪 + 降噪优化
4. 第 4 阶段：电话升级 + 值班体系 + SLO 治理

## 8. 部署步骤

### 8.1 监控中心部署

在监控中心机器执行：

```bash
cd /root/workspace/mediasoup-cpp/deploy/monitoring
docker compose -f docker-compose.center.yml up -d
```

默认入口：

- Grafana: `http://<监控中心IP>:3001`
- Prometheus: `http://<监控中心IP>:9090`
- Alertmanager: `http://<监控中心IP>:9093`

### 8.2 SFU 节点部署

在每台 SFU 节点执行：

```bash
cd /root/workspace/mediasoup-cpp/deploy/monitoring
sed -i 's/<MONITOR_CENTER_IP>/你的监控中心IP/g' alloy/config.node.alloy
docker compose -f docker-compose.node.yml up -d
```

这一步会启动：

- `node-exporter`
- `alloy`

### 8.3 需要修改的配置

Prometheus：

- [deploy/monitoring/prometheus/prometheus.yml](../deploy/monitoring/prometheus/prometheus.yml)

需要确认：

- `mediasoup-sfu` 的 `/metrics` 地址正确
- `blackbox-http` 的首页探活地址正确
- `blackbox-sfu-health` 的 `/healthz` 地址正确
- `node-exporter` 节点列表正确

Alertmanager：

- [deploy/monitoring/alertmanager/alertmanager.yml](../deploy/monitoring/alertmanager/alertmanager.yml)

需要修改：

- 企业微信 webhook
- 钉钉 webhook
- 电话升级中继地址

Alloy：

- [deploy/monitoring/alloy/config.node.alloy](../deploy/monitoring/alloy/config.node.alloy)

需要修改：

- Loki 地址为监控中心实际 IP

## 9. 部署后验收步骤

### 9.1 检查服务接口

在 SFU 节点执行：

```bash
curl -k https://<sfu-host>:3000/healthz
curl -k https://<sfu-host>:3000/readyz
curl -k https://<sfu-host>:3000/metrics
curl -k https://<sfu-host>:3000/api/node-load
```

确认：

- `/healthz` 返回 `200`
- `/readyz` 返回 `200`
- `/metrics` 有 `mediasoup_sfu_*` 指标
- `/api/node-load` 有 `workerQueueStats`

### 9.2 检查 Prometheus Targets

在 Prometheus 页面确认以下 target 是 `UP`：

- `prometheus`
- `node`
- `mediasoup-sfu`
- `blackbox-http`
- `blackbox-sfu-health`

### 9.3 检查 Grafana

打开 Grafana，确认四张看板都能看到数据：

- `Global Overview`
- `Infrastructure`
- `Service SLI/SLO`
- `Business KPI`

### 9.4 人工演练告警

建议最少演练以下几项：

1. 停掉 `mediasoup-sfu`
2. 杀掉一个 `mediasoup-worker`
3. 发送错误请求，制造 `join-failed`
4. 发送 malformed websocket 消息

确认：

- Prometheus 指标变化正确
- Loki 收到日志
- Alertmanager 发出通知
- Grafana 看板出现对应变化

## 10. 值班与复盘 SOP（最小要求）

- 告警必须支持：确认、转派、关闭
- P1/P2 故障必须复盘并跟踪改进项闭环
- 每月回顾以下指标：
  - 误报率
  - 漏报率
  - MTTA（平均发现/确认时间）
  - MTTR（平均恢复时间）

## 11. 本项目适配说明

当前模板已经接入两类应用层信号：

- `GET /healthz`
  返回 `200` 表示 SFU 已完成启动且仍有可用 worker；返回 `503` 表示应用已降级。
- `GET /metrics`
  暴露 Prometheus 指标，例如：
  - `mediasoup_sfu_healthy`
  - `mediasoup_sfu_ready`
  - `mediasoup_sfu_has_available_workers`
  - `mediasoup_sfu_available_worker_threads`
  - `mediasoup_sfu_workers`
  - `mediasoup_sfu_rooms`
  - `mediasoup_sfu_join_failures_total`
  - `mediasoup_sfu_plain_publish_failures_total`
  - `mediasoup_sfu_plain_subscribe_failures_total`
  - `mediasoup_sfu_worker_deaths_total`
  - `mediasoup_sfu_worker_respawns_total`
  - `mediasoup_sfu_ws_disconnects_total`
  - `mediasoup_sfu_malformed_ws_messages_total`
  - `mediasoup_sfu_worker_thread_queue_depth{thread_id=...}`
  - `mediasoup_sfu_worker_thread_queue_peak_depth{thread_id=...}`
  - `mediasoup_sfu_worker_thread_avg_task_wait_us{thread_id=...}`

- `GET /api/node-load`
  暴露补充排障信号，例如：
  - `availableWorkerThreads`
  - `workerQueueStats[].queueDepth`
  - `workerQueueStats[].queuePeakDepth`
  - `workerQueueStats[].avgTaskWaitUs`
  - `roomOwnership`

说明：
- 当前 `workerQueueStats` 既能通过 `/api/node-load` 手工排障，也已经同步到 `/metrics` 的逐 `thread_id` gauge，适合做 Grafana 面板和 Prometheus 告警。

`deploy/monitoring/prometheus/prometheus.yml`
默认已启用 `mediasoup-sfu` scrape job 和 `blackbox-sfu-health` 探活。

## 12. 当前最值得关注的告警

P1：

- `SfuHttpEndpointDown`
- `SfuHealthEndpointDown`
- `SfuReadyEndpointDown`
- `SfuNoAvailableWorkers`
- `SfuWorkerThreadCapacityLow`
- `SfuWorkerDeathsDetected`
- `WorkerCrashInLog`

P2：

- `SfuJoinFailuresDetected`
- `SfuPlainPublishFailuresDetected`
- `SfuPlainSubscribeFailuresDetected`
- `SfuWorkerRespawnsDetected`
- `SfuMalformedWebSocketMessagesHigh`
- `SfuWorkerThreadQueueDepthHigh`
- `SfuWorkerThreadTaskWaitHigh`
- `JoinFailuresInLog`
- `PlainPublishFailuresInLog`
- `PlainSubscribeFailuresInLog`
- `WebSocketCloseRateHigh`

## 13. 当前方案的设计依据

当前这套监控体系的选择依据来自代码审视结论：

- 服务端已有 `/healthz`、`/readyz`、`/metrics`、`/api/node-load`
- 关键失败路径已补：
  - `join failed`
  - `plainPublish/plainSubscribe failed`
  - `worker death/respawn`
  - `ws disconnect`
  - malformed websocket
- 控制面排队退化信号已补成 Prometheus 指标：
  - `mediasoup_sfu_worker_thread_queue_depth`
  - `mediasoup_sfu_worker_thread_queue_peak_depth`
  - `mediasoup_sfu_worker_thread_avg_task_wait_us`

因此当前体系的重点是：

1. 先发现服务不可用
2. 再发现 worker / queue / 请求失败率是否开始退化
3. 再通过 Loki 日志定位原因
