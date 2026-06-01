#!/usr/bin/env bash
set -euo pipefail

tls_cert=/opt/mediasoup-cpp/certs/tls.pem
tls_key=/opt/mediasoup-cpp/certs/tls.key
if [[ ! -r "$tls_cert" ]]; then
  >&2 printf 'ERROR: signaling TLS certificate is missing or unreadable: %s\n' "$tls_cert"
  exit 1
fi
if [[ ! -r "$tls_key" ]]; then
  >&2 printf 'ERROR: signaling TLS private key is missing or unreadable: %s\n' "$tls_key"
  exit 1
fi

default_web_rtc_server_port=9000
case "${MEDIASOUP_NODE_ID:-}" in
  mediasoup-h1) default_web_rtc_server_port=8000 ;;
  mediasoup-h2) default_web_rtc_server_port=8001 ;;
  mediasoup-h3) default_web_rtc_server_port=8002 ;;
esac
web_rtc_server_port="${MEDIASOUP_WEBRTC_SERVER_PORT:-${default_web_rtc_server_port}}"

args=(
  --nodaemon
  "--port=${MEDIASOUP_PORT:-9000}"
  "--workerBin=${MEDIASOUP_WORKER_BIN:-./mediasoup-worker}"
)

auto_workers=""
auto_worker_threads=""
if [[ -z "${MEDIASOUP_WORKERS:-}" || -z "${MEDIASOUP_WORKER_THREADS:-}" ]]; then
  if [[ "$web_rtc_server_port" =~ ^[0-9]+$ ]]; then
    auto_workers=1
    auto_worker_threads=1
  fi
fi

if [[ -n "${MEDIASOUP_WORKERS:-}" ]]; then
  args+=("--workers=${MEDIASOUP_WORKERS}")
elif [[ -n "$auto_workers" ]]; then
  args+=("--workers=${auto_workers}")
fi

if [[ -n "${MEDIASOUP_WORKER_THREADS:-}" ]]; then
  args+=("--workerThreads=${MEDIASOUP_WORKER_THREADS}")
elif [[ -n "$auto_worker_threads" ]]; then
  args+=("--workerThreads=${auto_worker_threads}")
fi

args+=("--webRtcServerPort=${web_rtc_server_port}")

if [[ -n "${MEDIASOUP_NODE_ID:-}" ]]; then
  args+=("--nodeId=${MEDIASOUP_NODE_ID}")
fi

if [[ -n "${MEDIASOUP_NODE_ADDRESS:-}" ]]; then
  args+=("--nodeAddress=${MEDIASOUP_NODE_ADDRESS}")
fi

if [[ -n "${MEDIASOUP_GEO_DB:-}" ]]; then
  args+=("--geoDb=${MEDIASOUP_GEO_DB}")
fi

if [[ -n "${MEDIASOUP_LOG_DIR:-}" ]]; then
  args+=("--logDir=${MEDIASOUP_LOG_DIR}")
fi

if [[ -n "${MEDIASOUP_LOG_LEVEL:-}" ]]; then
  args+=("--logLevel=${MEDIASOUP_LOG_LEVEL}")
fi

if [[ -n "${MEDIASOUP_LOG_DIR:-}" ]]; then
  mkdir -p "${MEDIASOUP_LOG_DIR}"

  rotate_hours="${MEDIASOUP_LOG_ROTATE_HOURS:-3}"
  if [[ "$rotate_hours" =~ ^[0-9]+$ ]] && (( rotate_hours > 0 )); then
    current_hour="$(date +%H)"
    bucket_hour=$((10#$current_hour / rotate_hours * rotate_hours))
  else
    bucket_hour="$(date +%H)"
  fi
  bucket_day="$(date +%Y%m%d)"
  log_prefix="${MEDIASOUP_LOG_PREFIX:-mediasoup-sfu}"
  log_file="${MEDIASOUP_LOG_DIR}/${log_prefix}_${bucket_day}$(printf '%02d' "${bucket_hour}")_1.log"
  touch "${log_file}"
  exec >>"${log_file}" 2>&1
fi

exec ./mediasoup-sfu "${args[@]}" "$@"
