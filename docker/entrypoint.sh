#!/usr/bin/env bash
set -euo pipefail

args=(
  --nodaemon
  "--port=${MEDIASOUP_PORT:-1770}"
  "--workerBin=${MEDIASOUP_WORKER_BIN:-./mediasoup-worker}"
  "--listenIp=${MEDIASOUP_LISTEN_IP:-0.0.0.0}"
  "--rtcMinPort=${MEDIASOUP_RTC_MIN_PORT:-8000}"
  "--rtcMaxPort=${MEDIASOUP_RTC_MAX_PORT:-8002}"
  "--recordDir=${MEDIASOUP_RECORD_DIR:-/var/lib/mediasoup/recordings}"
)

if [[ -n "${MEDIASOUP_WORKERS:-}" ]]; then
  args+=("--workers=${MEDIASOUP_WORKERS}")
fi

if [[ -n "${MEDIASOUP_WORKER_THREADS:-}" ]]; then
  args+=("--workerThreads=${MEDIASOUP_WORKER_THREADS}")
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
