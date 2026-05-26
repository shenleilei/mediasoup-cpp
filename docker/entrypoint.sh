#!/usr/bin/env bash
set -euo pipefail

rtc_min_port="${MEDIASOUP_RTC_MIN_PORT:-8000}"
rtc_max_port="${MEDIASOUP_RTC_MAX_PORT:-8002}"

args=(
  --nodaemon
  "--port=${MEDIASOUP_PORT:-1770}"
  "--workerBin=${MEDIASOUP_WORKER_BIN:-./mediasoup-worker}"
  "--listenIp=${MEDIASOUP_LISTEN_IP:-0.0.0.0}"
  "--rtcMinPort=${rtc_min_port}"
  "--rtcMaxPort=${rtc_max_port}"
  "--recordDir=${MEDIASOUP_RECORD_DIR:-/var/lib/mediasoup/recordings}"
)

auto_workers=""
auto_worker_threads=""
if [[ -z "${MEDIASOUP_WORKERS:-}" || -z "${MEDIASOUP_WORKER_THREADS:-}" ]]; then
  if [[ "$rtc_min_port" =~ ^[0-9]+$ && "$rtc_max_port" =~ ^[0-9]+$ && "$rtc_max_port" -ge "$rtc_min_port" ]]; then
    rtc_port_span=$((rtc_max_port - rtc_min_port + 1))
    if (( rtc_port_span <= 8 )); then
      auto_workers=1
      auto_worker_threads=1
    fi
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

if [[ -n "${MEDIASOUP_ANNOUNCED_IP:-}" ]]; then
  args+=("--announcedIp=${MEDIASOUP_ANNOUNCED_IP}")
fi

if [[ -n "${MEDIASOUP_REDIS_HOST:-}" ]]; then
  args+=("--redisHost=${MEDIASOUP_REDIS_HOST}")
else
  args+=("--redisHost=0.0.0.0")
fi

if [[ -n "${MEDIASOUP_REDIS_PORT:-}" ]]; then
  args+=("--redisPort=${MEDIASOUP_REDIS_PORT}")
else
  args+=(--redisPort=1)
fi

if [[ "${MEDIASOUP_REDIS_REQUIRED:-0}" == "1" ]]; then
  args+=(--redisRequired)
else
  args+=(--noRedisRequired)
fi

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

exec ./mediasoup-sfu "${args[@]}" "$@"
