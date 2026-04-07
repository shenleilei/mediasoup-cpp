# Monitoring Stack Scaffold

Path: `/home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring`

## Includes

- Prometheus + alert rules (`prometheus/`)
- Alertmanager routing (`alertmanager/`)
- Grafana provisioning + dashboards (`grafana/`)
- Loki + Promtail log pipeline (`loki/`, `promtail/`)
- Node Exporter + Process Exporter + Blackbox Exporter

## Start

```bash
cd /home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring
docker compose up -d
```

## Endpoints

- Grafana: `http://localhost:3001` (admin/admin)
- Prometheus: `http://localhost:9090`
- Alertmanager: `http://localhost:9093`

## Important

Edit probe targets in:

- `/home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring/prometheus/prometheus.yml`

and webhook URLs in:

- `/home/runner/work/mediasoup-cpp/mediasoup-cpp/deploy/monitoring/alertmanager/alertmanager.yml`

before production rollout.
