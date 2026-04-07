# 上线 Checklist

## P0 — 上线前必须完成

### 基础设施
- [ ] 换多队列网卡 VM 或物理机（当前 virtio 单队列，80 rooms 就触发内核 softirq 瓶颈）
- [ ] 配置 HTTPS + WSS（当前是 HTTP 明文，浏览器会限制 getUserMedia）
  - 方案 A：Nginx 反向代理 + Let's Encrypt 证书
  - 方案 B：uWS 直接加载 TLS 证书
- [ ] 域名绑定（替代裸 IP 访问）
- [ ] 防火墙开放 UDP 端口范围 10000-59999（WebRTC 媒体端口）
- [ ] 防火墙开放 TCP 443（WSS 信令）

### 安全
- [ ] WebSocket 信令加认证（当前任何人都能 join 任何房间）
  - 方案：JWT token 验证，join 时校验
- [ ] 限制单 IP 连接数 / 请求频率（防 DDoS）
- [ ] DTLS 证书：当前用 mediasoup-worker 自动生成的，生产环境考虑固定证书
- [ ] 录制文件访问控制（当前 /recordings/* 无鉴权）
- [ ] 去掉 config.example.json 中的默认值，强制用户配置

### 稳定性
- [ ] 进程守护：systemd service 文件（替代手动启动）
- [ ] 日志轮转：logrotate 配置（/var/log/mediasoup-sfu.log）
- [ ] 录制磁盘监控告警（当前有自动清理，但需要监控）
- [ ] Worker 崩溃告警（当前只有日志，需要接入监控系统）
- [ ] OOM 保护：设置 cgroup 内存限制

### 测试
- [ ] 全量测试通过：`mediasoup_tests` + 所有集成测试
- [ ] 压测验证：在目标机器上跑 `mediasoup_bench`，确认容量
- [ ] 长时间运行测试：跑 24 小时，检查内存泄漏 / 文件描述符泄漏

## P1 — 上线后尽快完成

### 监控
- [ ] Prometheus metrics 导出（房间数、peer 数、Worker CPU、丢包率）
- [ ] Grafana 面板
- [ ] 告警规则：Worker 崩溃、丢包率 > 5%、CPU > 80%

### 运维
- [ ] 优雅重启脚本（不中断现有通话）
- [ ] 配置热更新（当前改配置需要重启）
- [ ] 录制文件定期归档到对象存储（S3/OSS）

### 功能
- [ ] 信令协议文档化（当前是 ad-hoc JSON，没有 schema）
- [ ] 房间人数上限控制
- [ ] 踢人 / 静音 API

## P2 — 后续迭代

- [ ] Dynacast（按需转发，节省带宽）
- [ ] Simulcast 支持（多分辨率编码）
- [ ] TURN 服务器部署（NAT 穿透失败时的 fallback）
- [ ] 多节点 Redis 集群（当前单 Redis 实例）
- [ ] 录制转码（WebM → MP4，兼容更多播放器）
- [ ] SDK 封装（iOS/Android/Web SDK，隐藏信令细节）
