#!/usr/bin/env bash
set -euo pipefail

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
