#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

REMOTE_HOST="${REMOTE_HOST:-root@172.31.4.40}"
REMOTE_REPO_DIR="${REMOTE_REPO_DIR:-}"
PRESSURE_WS_URL="${PRESSURE_WS_URL:-}"
PRESSURE_HTTP_URL="${PRESSURE_HTTP_URL:-}"
PRESSURE_CONTAINER="${PRESSURE_CONTAINER:-mediasoup-9000}"
PRESSURE_MAX_ROOMS="${PRESSURE_MAX_ROOMS:-1}"
PRESSURE_STEP="${PRESSURE_STEP:-1}"
PRESSURE_ROUND_MS="${PRESSURE_ROUND_MS:-2000}"
PRESSURE_STEADY_ROUND_MS="${PRESSURE_STEADY_ROUND_MS:-2000}"
PRESSURE_STEADY_ROUNDS="${PRESSURE_STEADY_ROUNDS:-1}"
PRESSURE_RECV_RATIO="${PRESSURE_RECV_RATIO:-0.70}"
PRESSURE_SAMPLE_HOST="${PRESSURE_SAMPLE_HOST:-local}"

run_tag="remote_pressure_smoke_$(date +%Y%m%d_%H%M%S)"

ssh -T -o BatchMode=yes "$REMOTE_HOST" "bash --noprofile --norc -lc '
set -euo pipefail
remote_repo_dir=\"$REMOTE_REPO_DIR\"
if [ -z \"\$remote_repo_dir\" ]; then
  for candidate in /root/mediasoup-cpp /root/workspace/mediasoup-cpp; do
    if [ -d \"\$candidate\" ]; then
      remote_repo_dir=\"\$candidate\"
      break
    fi
  done
fi
[ -n \"\$remote_repo_dir\" ] || {
  echo \"remote mediasoup-cpp repo directory not found\" >&2
  exit 1
}
cd \"\$remote_repo_dir\"
pressure_http_url=\"$PRESSURE_HTTP_URL\"
pressure_ws_url=\"$PRESSURE_WS_URL\"
if [ -z \"\$pressure_http_url\" ] || [ -z \"\$pressure_ws_url\" ]; then
  if curl -sk https://127.0.0.1:9000/healthz >/dev/null 2>&1; then
    pressure_http_url=\"https://127.0.0.1:9000\"
    pressure_ws_url=\"wss://127.0.0.1:9000/ws\"
  else
    curl -sS http://127.0.0.1:9000/healthz >/dev/null
    pressure_http_url=\"http://127.0.0.1:9000\"
    pressure_ws_url=\"ws://127.0.0.1:9000/ws\"
  fi
fi
stdbuf -oL -eL node tests/qos_harness/single_worker_pressure.mjs \
  --ws-url=\"\$pressure_ws_url\" \
  --http-url=\"\$pressure_http_url\" \
  --container=\"$PRESSURE_CONTAINER\" \
  --sample-host=\"$PRESSURE_SAMPLE_HOST\" \
  --max-rooms=\"$PRESSURE_MAX_ROOMS\" \
  --step=\"$PRESSURE_STEP\" \
  --round-ms=\"$PRESSURE_ROUND_MS\" \
  --steady-round-ms=\"$PRESSURE_STEADY_ROUND_MS\" \
  --hold-after-max \
  --steady-rounds=\"$PRESSURE_STEADY_ROUNDS\" \
  --recv-ratio=\"$PRESSURE_RECV_RATIO\" \
  --room-prefix=\"$run_tag\"
'"
