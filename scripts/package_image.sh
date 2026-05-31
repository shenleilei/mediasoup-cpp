#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/package_image.sh [--push] [--tag TAG] [--repository REPOSITORY] [--latest]

Build the Docker image while refusing to fall back to network downloads for
local prerequisites.

This script requires:
  - local Docker image `ubuntu:20.04`
  - executable `./mediasoup-worker` in the repository root

All arguments are forwarded to `./build_image.sh`.
EOF
}

for arg in "$@"; do
  case "$arg" in
    --help|-h)
      usage
      exit 0
      ;;
  esac
done

if ! docker images --format '{{.Repository}}:{{.Tag}}' | grep -qx 'ubuntu:20.04'; then
  >&2 printf 'ERROR: local Docker image ubuntu:20.04 not found. Load it first, then rerun.\n'
  exit 1
fi

if [[ ! -x ./mediasoup-worker ]]; then
  >&2 printf 'ERROR: ./mediasoup-worker is missing or not executable. This script will not download a worker binary.\n'
  exit 1
fi

if ! python3 ./scripts/ipc_contract_guard.py verify-release-readiness; then
  >&2 printf 'ERROR: IPC regression guard failed. Run: cd %s && ./script/run_all_tests.sh all\n' "$repo_root"
  exit 1
fi

exec ./build_image.sh "$@"
