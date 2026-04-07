# Monitoring Stack Scaffold

Path: `deploy/monitoring` (relative to repository root)

## Includes

- Prometheus + alert rules (`prometheus/`)
- Alertmanager routing (`alertmanager/`)
- Grafana provisioning + dashboards (`grafana/`)
- Loki + Promtail log pipeline (`loki/`, `promtail/`)
- Node Exporter + Process Exporter + Blackbox Exporter

## Start

```bash
cd deploy/monitoring
docker compose up -d
```

## Endpoints

- Grafana: `http://localhost:3001` (admin/admin)
- Prometheus: `http://localhost:9090`
- Alertmanager: `http://localhost:9093`

## Important

Edit probe targets in:

- `deploy/monitoring/prometheus/prometheus.yml`

and webhook URLs in:

- `deploy/monitoring/alertmanager/alertmanager.yml`

before production rollout.
