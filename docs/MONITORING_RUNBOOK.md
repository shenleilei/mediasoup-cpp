# Monitoring & Alerting Runbook

本仓库提供了一个可直接启动的“分层监控 + 统一告警 + 分级升级”模板，位于：

- `/home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring`

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
5. 日志与链路：Loki + Promtail（链路追踪建议后续补充 OTel + Jaeger/Tempo）

## 2. 组件与职责

- Prometheus：指标抓取与规则计算
- Alertmanager：告警路由、抑制、分组
- Grafana：统一可视化面板
- Loki + Promtail：日志聚合与检索
- Node Exporter：主机指标
- Process Exporter：进程级指标
- Blackbox Exporter：接口可用性探测

## 3. 快速启动

```bash
cd /home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring
docker compose up -d
```

默认入口：
- Grafana: `http://localhost:3001`（admin/admin）
- Prometheus: `http://localhost:9090`
- Alertmanager: `http://localhost:9093`

## 4. 告警分级与升级策略

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

## 5. 面板体系

已预置四类看板：
- Global Overview
- Service SLI/SLO
- Infrastructure
- Business KPI（占位模板，需接入业务指标）

## 6. 实施节奏建议

1. 第 1 阶段：机器层 + 进程层 + IM 告警
2. 第 2 阶段：应用层探活 + 统一服务大盘 + 日志告警
3. 第 3 阶段：业务 KPI 告警 + 链路追踪 + 降噪优化
4. 第 4 阶段：电话升级 + 值班体系 + SLO 治理

## 7. 值班与复盘 SOP（最小要求）

- 告警必须支持：确认、转派、关闭
- P1/P2 故障必须复盘并跟踪改进项闭环
- 每月回顾以下指标：
  - 误报率
  - 漏报率
  - MTTA（平均发现/确认时间）
  - MTTR（平均恢复时间）

## 8. 本项目适配说明

当前模板默认通过黑盒探活和系统/进程指标提供可观测性，不依赖应用内 `/metrics`。

如果后续在 `mediasoup-sfu` 增加 Prometheus 原生指标，请在
`/home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring/prometheus/prometheus.yml`
中启用 `mediasoup-sfu` scrape job，并将业务指标接入 `Business KPI` 看板。
