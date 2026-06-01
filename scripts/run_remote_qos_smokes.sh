#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

REMOTE_DEMO_URL="${REMOTE_DEMO_URL:-https://14.103.165.183:1770/}"
REMOTE_PLAIN_URL="${REMOTE_PLAIN_URL:-https://volcvideo3.zelostech.com.cn:1770/}"
REMOTE_PLAIN_ROOM_ID="${REMOTE_PLAIN_ROOM_ID:-xxx}"

cd "$ROOT_DIR"

node tests/qos_harness/browser_demo_smoke.mjs "$REMOTE_DEMO_URL"
node tests/qos_harness/browser_plain_publish_abs_capture_smoke.mjs --observe-only "$REMOTE_PLAIN_URL" "$REMOTE_PLAIN_ROOM_ID"
