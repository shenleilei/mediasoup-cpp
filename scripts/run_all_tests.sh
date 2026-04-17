#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CLIENT_BUILD_DIR="$ROOT_DIR/client/build"
JOBS="${JOBS:-$(nproc)}"

ALL_GROUPS=(
  unit
  integration
  topology
  threaded
)

SELECTED_GROUPS=()
SKIP_BUILD=0

usage() {
  cat <<'EOF'
Usage:
  scripts/run_all_tests.sh
  scripts/run_all_tests.sh unit threaded
  scripts/run_all_tests.sh --list

Options:
  --skip-build   assume binaries are already up to date
  --list         list available groups
  -h, --help     show help

Groups:
  unit         non-QoS suites inside mediasoup_tests
  integration  integration / e2e / stability / review-fix binaries
  topology     topology + multinode binaries (requires Redis)
  threaded     threaded plain-client integration binary

Notes:
  - This script is the non-QoS full-test entry.
  - It intentionally excludes scripts/run_qos_tests.sh and dedicated QoS binaries:
    mediasoup_qos_integration_tests
    mediasoup_qos_accuracy_tests
    mediasoup_qos_recording_accuracy_tests
  - threaded tests require client/build/plain-client.
  - topology tests require a reachable Redis on 127.0.0.1:6379.
EOF
}

list_groups() {
  printf '%s\n' "${ALL_GROUPS[@]}"
}

require_file() {
  local path="$1"
  [[ -e "$path" ]] || {
    echo "error: required file not found: $path" >&2
    exit 1
  }
}

cleanup_test_ports() {
  local ports=(14000 14002 14003 14004 14005 14006 14010 14011 14012 14021)
  for port in "${ports[@]}"; do
    if command -v lsof >/dev/null 2>&1; then
      mapfile -t pids < <(lsof -tiTCP:"$port" -sTCP:LISTEN 2>/dev/null || true)
      if ((${#pids[@]} > 0)); then
        echo "info: killing listeners on 127.0.0.1:$port -> ${pids[*]}" >&2
        kill -9 "${pids[@]}" 2>/dev/null || true
      fi
    fi
  done
  pkill -9 -f 'mediasoup-sfu.*--port=140' 2>/dev/null || true
  sleep 1
}

build_targets() {
  echo "==> building non-QoS test targets"
  cmake --build "$BUILD_DIR" -j"$JOBS" --target \
    mediasoup_tests \
    mediasoup_integration_tests \
    mediasoup_e2e_tests \
    mediasoup_topology_tests \
    mediasoup_stability_integration_tests \
    mediasoup_multinode_tests \
    mediasoup_review_fix_tests \
    mediasoup_thread_integration_tests
  cmake --build "$CLIENT_BUILD_DIR" -j"$JOBS" --target plain-client
}

run_cmd() {
  local label="$1"
  shift
  echo
  echo "==> $label"
  "$@"
}

run_unit() {
  require_file "$BUILD_DIR/mediasoup_tests"
  run_cmd \
    "unit" \
    "$BUILD_DIR/mediasoup_tests" \
    "--gtest_filter=-ClientQos*:DownlinkAllocatorTest.*:DownlinkHealthMonitorTest.*:QosProtocolTest.*:QosValidatorTest.*:QosRegistryTest.*:QosAggregatorTest.*:QosRoomAggregatorTest.*:QosOverrideBuilderTest.*:QosAccuracyTest.*:QosRecordingAccuracyTest.*"
}

run_integration() {
  cleanup_test_ports
  require_file "$BUILD_DIR/mediasoup_integration_tests"
  require_file "$BUILD_DIR/mediasoup_e2e_tests"
  require_file "$BUILD_DIR/mediasoup_stability_integration_tests"
  require_file "$BUILD_DIR/mediasoup_review_fix_tests"
  run_cmd "integration:mediasoup_integration_tests" "$BUILD_DIR/mediasoup_integration_tests"
  cleanup_test_ports
  run_cmd "integration:mediasoup_e2e_tests" "$BUILD_DIR/mediasoup_e2e_tests"
  cleanup_test_ports
  run_cmd "integration:mediasoup_stability_integration_tests" "$BUILD_DIR/mediasoup_stability_integration_tests"
  cleanup_test_ports
  run_cmd "integration:mediasoup_review_fix_tests" "$BUILD_DIR/mediasoup_review_fix_tests"
}

run_topology() {
  cleanup_test_ports
  require_file "$BUILD_DIR/mediasoup_topology_tests"
  require_file "$BUILD_DIR/mediasoup_multinode_tests"
  if command -v redis-cli >/dev/null 2>&1; then
    if ! redis-cli -h 127.0.0.1 -p 6379 ping >/dev/null 2>&1; then
      echo "error: topology group requires Redis on 127.0.0.1:6379" >&2
      exit 1
    fi
  else
    echo "warning: redis-cli not found; continuing without preflight ping" >&2
  fi
  run_cmd "topology:mediasoup_topology_tests" "$BUILD_DIR/mediasoup_topology_tests"
  cleanup_test_ports
  run_cmd "topology:mediasoup_multinode_tests" "$BUILD_DIR/mediasoup_multinode_tests"
}

run_threaded() {
  cleanup_test_ports
  require_file "$BUILD_DIR/mediasoup_thread_integration_tests"
  require_file "$CLIENT_BUILD_DIR/plain-client"
  run_cmd \
    "threaded:mediasoup_thread_integration_tests" \
    env QOS_THREAD_INTEGRATION_PORT=14021 "$BUILD_DIR/mediasoup_thread_integration_tests"
}

run_group() {
  local group="$1"
  case "$group" in
    unit) run_unit ;;
    integration) run_integration ;;
    topology) run_topology ;;
    threaded) run_threaded ;;
    *) echo "error: unknown group '$group'" >&2; exit 1 ;;
  esac
}

main() {
  while (($# > 0)); do
    case "$1" in
      --skip-build) SKIP_BUILD=1 ;;
      --list) list_groups; exit 0 ;;
      -h|--help) usage; exit 0 ;;
      all) ;;
      *) SELECTED_GROUPS+=("$1") ;;
    esac
    shift
  done

  if ((${#SELECTED_GROUPS[@]} == 0)); then
    SELECTED_GROUPS=("${ALL_GROUPS[@]}")
  fi

  if ((SKIP_BUILD == 0)); then
    build_targets
  fi

  for group in "${SELECTED_GROUPS[@]}"; do
    run_group "$group"
  done

  echo
  echo "non-QoS test run passed"
}

main "$@"
