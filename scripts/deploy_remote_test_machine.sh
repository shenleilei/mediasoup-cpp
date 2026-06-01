#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

REMOTE_HOST="${REMOTE_HOST:-root@172.31.4.40}"
REMOTE_REPO_DIR="${REMOTE_REPO_DIR:-}"
LOCAL_IMAGE="${LOCAL_IMAGE:-mediasoup-cpp:sfu}"
REMOTE_IMAGE_REPO="${REMOTE_IMAGE_REPO:-mediasoup-cpp}"
REMOTE_IMAGE_TAG="${REMOTE_IMAGE_TAG:-remote-harness-$(date +%Y%m%d_%H%M%S)}"
REMOTE_IMAGE="${REMOTE_IMAGE_REPO}:${REMOTE_IMAGE_TAG}"
REMOTE_PLAIN_ROOM_ID="${REMOTE_PLAIN_ROOM_ID:-xxx}"
REMOTE_PLAIN_SIGNAL_PORT="${REMOTE_PLAIN_SIGNAL_PORT:-1770}"
REMOTE_PLAIN_PEER_ID="${REMOTE_PLAIN_PEER_ID:-plain-publisher}"
REMOTE_PLAIN_HOLD_MS="${REMOTE_PLAIN_HOLD_MS:-86400000}"
REMOTE_PLAIN_STARTUP_DELAY_MS="${REMOTE_PLAIN_STARTUP_DELAY_MS:-2000}"
REMOTE_PLAIN_WARMUP_KEYFRAMES="${REMOTE_PLAIN_WARMUP_KEYFRAMES:-3}"
REMOTE_LOG_ROOT="${REMOTE_LOG_ROOT:-/var/log/agora/mediasoup}"

shell_quote() {
  printf '%q' "$1"
}

cd "$ROOT_DIR"

SKIP_IPC_RELEASE_GUARD=1 "$ROOT_DIR/build_image.sh"
docker tag "$LOCAL_IMAGE" "$REMOTE_IMAGE"
docker save "$REMOTE_IMAGE" | ssh -T -o BatchMode=yes "$REMOTE_HOST" 'docker load >/dev/null'

remote_env_cmd="$(
  printf '%s ' \
    "REMOTE_REPO_DIR=$(shell_quote "$REMOTE_REPO_DIR")" \
    "REMOTE_IMAGE=$(shell_quote "$REMOTE_IMAGE")" \
    "REMOTE_PLAIN_ROOM_ID=$(shell_quote "$REMOTE_PLAIN_ROOM_ID")" \
    "REMOTE_PLAIN_SIGNAL_PORT=$(shell_quote "$REMOTE_PLAIN_SIGNAL_PORT")" \
    "REMOTE_PLAIN_PEER_ID=$(shell_quote "$REMOTE_PLAIN_PEER_ID")" \
    "REMOTE_PLAIN_HOLD_MS=$(shell_quote "$REMOTE_PLAIN_HOLD_MS")" \
    "REMOTE_PLAIN_STARTUP_DELAY_MS=$(shell_quote "$REMOTE_PLAIN_STARTUP_DELAY_MS")" \
    "REMOTE_PLAIN_WARMUP_KEYFRAMES=$(shell_quote "$REMOTE_PLAIN_WARMUP_KEYFRAMES")" \
    "REMOTE_LOG_ROOT=$(shell_quote "$REMOTE_LOG_ROOT")" \
    "bash -s"
)"

ssh -T -o BatchMode=yes "$REMOTE_HOST" "$remote_env_cmd" <<'REMOTE_SCRIPT'
set -euo pipefail

remote_repo_dir="${REMOTE_REPO_DIR:-/root/mediasoup-cpp}"
if [ ! -d "\$remote_repo_dir" ]; then
  for candidate in /root/mediasoup-cpp /root/workspace/mediasoup-cpp; do
    if [ -d "\$candidate" ]; then
      remote_repo_dir="\$candidate"
      break
    fi
  done
fi
[ -n "\$remote_repo_dir" ] || {
  echo "remote mediasoup-cpp repo directory not found" >&2
  exit 1
}

remote_image="${REMOTE_IMAGE}"
plain_room_id="${REMOTE_PLAIN_ROOM_ID}"
plain_signal_port="${REMOTE_PLAIN_SIGNAL_PORT}"
plain_peer_id="${REMOTE_PLAIN_PEER_ID}"
plain_hold_ms="${REMOTE_PLAIN_HOLD_MS}"
plain_startup_delay_ms="${REMOTE_PLAIN_STARTUP_DELAY_MS}"
plain_warmup_keyframes="${REMOTE_PLAIN_WARMUP_KEYFRAMES}"
remote_log_root="${REMOTE_LOG_ROOT}"

wait_health() {
  local url="$1"
  local mode="$2"
  local attempts="${3:-30}"
  local delay="${4:-1}"
  local i
  for ((i = 0; i < attempts; ++i)); do
    if [ "$mode" = "https" ]; then
      if curl -sk "$url" >/dev/null 2>&1; then
        return 0
      fi
    else
      if curl -sS "$url" >/dev/null 2>&1; then
        return 0
      fi
    fi
    sleep "$delay"
  done
  return 1
}

for name in mediasoup-h1 mediasoup-h2 mediasoup-h3 mediasoup-9000; do
  docker rm -f "$name" >/dev/null 2>&1 || true
done

mkdir -p "$remote_log_root"/mediasoup-h1 \
         "$remote_log_root"/mediasoup-h2 \
         "$remote_log_root"/mediasoup-h3 \
         "$remote_log_root"/mediasoup-9000
rm -f "$remote_log_root"/mediasoup-h1/container.stdout.log \
      "$remote_log_root"/mediasoup-h1/container.stderr.log \
      "$remote_log_root"/mediasoup-h2/container.stdout.log \
      "$remote_log_root"/mediasoup-h2/container.stderr.log \
      "$remote_log_root"/mediasoup-h3/container.stdout.log \
      "$remote_log_root"/mediasoup-h3/container.stderr.log \
      "$remote_log_root"/mediasoup-9000/container.stdout.log \
      "$remote_log_root"/mediasoup-9000/container.stderr.log

docker run -d --name mediasoup-h1 --network host \
  -v "$remote_log_root/mediasoup-h1:/var/log/agora/mediasoup" \
  -e MEDIASOUP_NODE_ID=mediasoup-h1 \
  -e MEDIASOUP_NODE_NAME=mediasoup-h1 \
  -e MEDIASOUP_PORT=1770 \
  -e MEDIASOUP_WEBRTC_SERVER_PORT=8000 \
  -e MEDIASOUP_LISTEN_IP=0.0.0.0 \
  -e MEDIASOUP_ANNOUNCED_IP=14.103.165.183 \
  -e MEDIASOUP_NODE_ADDRESS=ws://14.103.165.183:1770/ws \
  -e HAWKEYE_REGISTER_URL=ws://127.0.0.1:30000/register_ws \
  -e MEDIASOUP_LOG_DIR=/var/log/agora/mediasoup \
  "$remote_image" >/dev/null

docker run -d --name mediasoup-h2 --network host \
  -v "$remote_log_root/mediasoup-h2:/var/log/agora/mediasoup" \
  -e MEDIASOUP_NODE_ID=mediasoup-h2 \
  -e MEDIASOUP_NODE_NAME=mediasoup-h2 \
  -e MEDIASOUP_PORT=1771 \
  -e MEDIASOUP_WEBRTC_SERVER_PORT=8001 \
  -e MEDIASOUP_LISTEN_IP=0.0.0.0 \
  -e MEDIASOUP_ANNOUNCED_IP=14.103.165.183 \
  -e MEDIASOUP_NODE_ADDRESS=ws://14.103.165.183:1771/ws \
  -e HAWKEYE_REGISTER_URL=ws://127.0.0.1:30000/register_ws \
  -e MEDIASOUP_LOG_DIR=/var/log/agora/mediasoup \
  "$remote_image" >/dev/null

docker run -d --name mediasoup-h3 --network host \
  -v "$remote_log_root/mediasoup-h3:/var/log/agora/mediasoup" \
  -e MEDIASOUP_NODE_ID=mediasoup-h3 \
  -e MEDIASOUP_NODE_NAME=mediasoup-h3 \
  -e MEDIASOUP_PORT=1772 \
  -e MEDIASOUP_WEBRTC_SERVER_PORT=8002 \
  -e MEDIASOUP_LISTEN_IP=0.0.0.0 \
  -e MEDIASOUP_ANNOUNCED_IP=14.103.165.183 \
  -e MEDIASOUP_NODE_ADDRESS=ws://14.103.165.183:1772/ws \
  -e HAWKEYE_REGISTER_URL=ws://127.0.0.1:30000/register_ws \
  -e MEDIASOUP_LOG_DIR=/var/log/agora/mediasoup \
  "$remote_image" >/dev/null

docker run -d --name mediasoup-9000 --network host \
  -v "$remote_log_root/mediasoup-9000:/var/log/agora/mediasoup" \
  -e MEDIASOUP_PORT=9000 \
  -e MEDIASOUP_WEBRTC_SERVER_PORT=9000 \
  -e MEDIASOUP_WORKERS=1 \
  -e MEDIASOUP_WORKER_THREADS=1 \
  -e MEDIASOUP_REDIS_REQUIRED=0 \
  -e MEDIASOUP_LOG_DIR=/var/log/agora/mediasoup \
  "$remote_image" >/dev/null

wait_health https://127.0.0.1:1770/healthz https
wait_health https://127.0.0.1:1771/healthz https
wait_health https://127.0.0.1:1772/healthz https
if ! wait_health https://127.0.0.1:9000/healthz https 5 1; then
  wait_health http://127.0.0.1:9000/healthz http
fi

pkill -f "remote_plain_publish_abs_capture_sender.mjs .* ${plain_room_id} ${plain_peer_id}" >/dev/null 2>&1 || true
rm -f /tmp/remote_plain_smoke.log /tmp/remote_plain_smoke.pid
cd "$remote_repo_dir"
nohup node tests/qos_harness/remote_plain_publish_abs_capture_sender.mjs \
  127.0.0.1 \
  "$plain_signal_port" \
  "$plain_room_id" \
  "$plain_peer_id" \
  "$plain_hold_ms" \
  "$plain_startup_delay_ms" \
  "$plain_warmup_keyframes" \
  >/tmp/remote_plain_smoke.log 2>&1 < /dev/null &
echo $! >/tmp/remote_plain_smoke.pid
sleep 1
ps -p "$(cat /tmp/remote_plain_smoke.pid)" -o pid=,etimes=,cmd=
tail -n 20 /tmp/remote_plain_smoke.log
REMOTE_SCRIPT

printf '%s\n' "$REMOTE_IMAGE"
