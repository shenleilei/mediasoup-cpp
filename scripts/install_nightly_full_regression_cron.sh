#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PYTHON_BIN="${PYTHON_BIN:-$(command -v python3)}"
CONFIG_PATH="${CONFIG_PATH:-$ROOT_DIR/.nightly-full-regression.env}"
CRON_LOG_PATH="${CRON_LOG_PATH:-/var/log/mediasoup-cpp-nightly-cron.log}"
CRON_SCHEDULE="${CRON_SCHEDULE:-0 3 * * *}"
MARKER="# mediasoup-cpp-nightly-full-regression"
PRINT_ONLY=0

usage() {
  cat <<EOF
Usage:
  scripts/install_nightly_full_regression_cron.sh
  scripts/install_nightly_full_regression_cron.sh --print-only

Options:
  --print-only   print the managed cron line without installing it
  -h, --help     show help

Environment overrides:
  PYTHON_BIN
  CONFIG_PATH
  CRON_LOG_PATH
  CRON_SCHEDULE
EOF
}

while (($# > 0)); do
  case "$1" in
    --print-only) PRINT_ONLY=1 ;;
    -h|--help) usage; exit 0 ;;
    *)
      echo "error: unknown option '$1'" >&2
      exit 1
      ;;
  esac
  shift
done

[[ -n "${PYTHON_BIN:-}" ]] || {
  echo "error: python3 not found" >&2
  exit 1
}

CRON_CMD="cd $ROOT_DIR && $PYTHON_BIN $ROOT_DIR/scripts/nightly_full_regression.py run --config $CONFIG_PATH >> $CRON_LOG_PATH 2>&1 $MARKER"
CRON_LINE="$CRON_SCHEDULE $CRON_CMD"

if ((PRINT_ONLY)); then
  printf '%s\n' "$CRON_LINE"
  exit 0
fi

TMP_FILE="$(mktemp)"
trap 'rm -f "$TMP_FILE"' EXIT

crontab -l 2>/dev/null | grep -F -v "$MARKER" > "$TMP_FILE" || true
printf '%s\n' "$CRON_LINE" >> "$TMP_FILE"
crontab "$TMP_FILE"

printf 'installed cron entry:\n%s\n' "$CRON_LINE"
