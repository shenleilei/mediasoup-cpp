#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLIENT_DIR="$ROOT_DIR/src/client"
BUILD_DIR="$ROOT_DIR/build"
JEST_BIN="$CLIENT_DIR/node_modules/.bin/jest"
CASE_REPORT_SCRIPT="$ROOT_DIR/tests/qos_harness/render_case_report.mjs"
CASE_REPORT_JSON="$ROOT_DIR/docs/generated/uplink-qos-matrix-report.json"
ARTIFACTS_DIR="$ROOT_DIR/tests/qos_harness/artifacts"
FAILURES_FILE="$ARTIFACTS_DIR/last-failures.txt"
GENERATE_CASE_REPORT=0

ALL_GROUPS=(
  client-js
  cpp-unit
  cpp-integration
  cpp-accuracy
  cpp-recording
  node-harness
  browser-harness
  matrix
)

SELECTED_GROUPS=()
SKIP_BROWSER=0
SKIP_MATRIX=0
FAILED_GROUPS=()
FAILED_TASKS=()
RESUME_MODE=0

mkdir -p "$ARTIFACTS_DIR"

usage() {
  cat <<'EOF'
Usage:
  scripts/run_qos_tests.sh                # 默认全量跑 QoS 相关测试
  scripts/run_qos_tests.sh all
  scripts/run_qos_tests.sh client-js cpp-unit
  scripts/run_qos_tests.sh --resume

Options:
  --skip-browser    跳过 browser harness 和 matrix
  --skip-matrix     跳过 matrix
  --resume          只重跑上次失败的精确任务
  --list            列出可用分组
  -h, --help        显示帮助

Available groups:
  client-js         客户端 QoS JS 单测（test.qos.*.js）
  cpp-unit          服务端 QoS 相关单测（mediasoup_tests 中 *Qos*）
  cpp-integration   服务端 QoS 集成测试（mediasoup_qos_integration_tests）
  cpp-accuracy      QoS accuracy 测试
  cpp-recording     QoS recording accuracy 测试
  node-harness      Node QoS harness 场景
  browser-harness   browser_server_signal + browser_loopback
  matrix            browser loopback full matrix（run_matrix.mjs）

Notes:
  - 默认会顺序执行所有分组。
  - matrix 运行时间最长，并且依赖浏览器 / netem 环境。
  - 可用环境变量 QOS_MATRIX_SPEED 调整 matrix 用时。
  - 失败任务会记录到 tests/qos_harness/artifacts/last-failures.txt
  - full matrix 当前主报告写入 docs/generated/ 和 docs/；每次新结果都会按生成时间归档到 docs/archive/uplink-qos-runs/
EOF
}

list_groups() {
  printf '%s\n' "${ALL_GROUPS[@]}"
}

fail() {
  echo "error: $*" >&2
  exit 1
}

record_failed_task() {
  local label="$1"
  [[ "$label" == build:* ]] && return 0
  FAILED_TASKS+=("$label")
}

port_is_available() {
  local port="$1"

  command -v node >/dev/null 2>&1 || fail "node is required to probe port availability"

  node -e "
    const net = require('node:net');
    const server = net.createServer();
    server.once('error', () => process.exit(1));
    server.listen(${port}, '127.0.0.1', () => server.close(() => process.exit(0)));
  "
}

cleanup_test_port() {
  local port="$1"
  local pids=()

  [[ "$port" =~ ^14[0-9]{3}$ ]] || fail "refusing to auto-kill non-test port: $port"

  if command -v lsof >/dev/null 2>&1; then
    mapfile -t pids < <(lsof -tiTCP:"$port" -sTCP:LISTEN 2>/dev/null || true)
  elif command -v fuser >/dev/null 2>&1; then
    mapfile -t pids < <(fuser -n tcp "$port" 2>/dev/null | tr ' ' '\n' | sed '/^$/d' || true)
  else
    fail "neither lsof nor fuser is available for test port cleanup"
  fi

  ((${#pids[@]} > 0)) || return 0

  echo "warning: killing test listener(s) on 127.0.0.1:$port -> ${pids[*]}" >&2
  kill -9 "${pids[@]}" 2>/dev/null || true
  sleep 1
}

cleanup_test_processes_fallback() {
  local port="$1"

  [[ "$port" =~ ^14[0-9]{3}$ ]] || fail "refusing to auto-kill non-test port: $port"

  local patterns=(
    "mediasoup-sfu.*--port=${port}"
    "mediasoup_qos_integration_tests"
    "tests/qos_harness/run.mjs"
    "tests/qos_harness/browser_server_signal.mjs"
    "tests/qos_harness/browser_loopback.mjs"
    "tests/qos_harness/run_matrix.mjs"
  )

  for pattern in "${patterns[@]}"; do
    pkill -9 -f "$pattern" 2>/dev/null || true
  done

  sleep 1
}

prepare_test_port() {
  local port="$1"
  local label="${2:-port $port}"

  [[ "$port" =~ ^14[0-9]{3}$ ]] || fail "refusing to auto-clean non-test port: $port"

  echo "info: cleaning test processes for $label" >&2
  cleanup_test_processes_fallback "$port"
  cleanup_test_port "$port"

  if ! port_is_available "$port"; then
    fail "$label is still in use on 127.0.0.1 after proactive cleanup"
  fi
}

require_port_available() {
  local port="$1"
  local label="${2:-port $port}"

  if port_is_available "$port"; then
    return 0
  fi

  cleanup_test_port "$port"

  if port_is_available "$port"; then
    return 0
  fi

  cleanup_test_processes_fallback "$port"

  if ! port_is_available "$port"; then
    fail "$label is still in use on 127.0.0.1 after auto-cleanup"
  fi
}

require_file() {
  local path="$1"
  [[ -e "$path" ]] || fail "required file not found: $path"
}

ensure_target_built() {
  local target="$1"
  local binary="$2"
  shift 2
  local sources=("$@")

  require_file "$binary"

  local rebuild=0
  local binary_mtime
  binary_mtime="$(stat -c '%Y' "$binary")"

  for source in "${sources[@]}"; do
    require_file "$source"
    local source_mtime
    source_mtime="$(stat -c '%Y' "$source")"
    if ((source_mtime > binary_mtime)); then
      rebuild=1
      break
    fi
  done

  if ((rebuild)); then
    run_cmd \
      "build:$target" \
      --cwd "$ROOT_DIR" \
      cmake --build "$BUILD_DIR" --target "$target"
  fi
}

run_cmd() {
  local label="$1"
  shift
  local cwd="${ROOT_DIR}"
  if (($# >= 2)) && [[ "$1" == "--cwd" ]]; then
    cwd="$2"
    shift 2
  fi

  local start end elapsed
  local rc
  start="$(date +%s)"
  echo
  echo "==> [$label]"
  echo "    (cd $cwd && $*)"
  set +e
  (
    cd "$cwd"
    "$@"
  )
  rc=$?
  set -e
  end="$(date +%s)"
  elapsed=$((end - start))
  if ((rc == 0)); then
    echo "<== [$label] PASS (${elapsed}s)"
  else
    echo "<== [$label] FAIL (${elapsed}s, rc=$rc)" >&2
    record_failed_task "$label"
  fi
  return "$rc"
}

normalize_groups() {
  local requested=()

  if ((RESUME_MODE)); then
    [[ -f "$FAILURES_FILE" ]] || fail "resume requested but failure file not found: $FAILURES_FILE"
    mapfile -t requested < <(grep -v '^\s*#' "$FAILURES_FILE" | sed '/^\s*$/d')
    ((${#requested[@]} > 0)) || fail "resume requested but failure file is empty: $FAILURES_FILE"
  elif ((${#SELECTED_GROUPS[@]} == 0)); then
    requested=("${ALL_GROUPS[@]}")
  else
    for group in "${SELECTED_GROUPS[@]}"; do
      if [[ "$group" == "all" ]]; then
        requested=("${ALL_GROUPS[@]}")
        break
      fi
      case " ${ALL_GROUPS[*]} " in
        *" $group "*) requested+=("$group") ;;
        *) fail "unknown group: $group" ;;
      esac
    done
  fi

  if ((RESUME_MODE)); then
    printf '%s\n' "${requested[@]}"
    return
  fi

  if ((SKIP_BROWSER)); then
    local filtered=()
    for group in "${requested[@]}"; do
      [[ "$group" == "browser-harness" || "$group" == "matrix" ]] && continue
      filtered+=("$group")
    done
    requested=("${filtered[@]}")
  elif ((SKIP_MATRIX)); then
    local filtered=()
    for group in "${requested[@]}"; do
      [[ "$group" == "matrix" ]] && continue
      filtered+=("$group")
    done
    requested=("${filtered[@]}")
  fi

  printf '%s\n' "${requested[@]}"
}

run_client_js() {
  require_file "$JEST_BIN"

  local tests=(
    "$CLIENT_DIR/lib/test/test.qos.controller.js"
    "$CLIENT_DIR/lib/test/test.qos.coordinator.js"
    "$CLIENT_DIR/lib/test/test.qos.executor.js"
    "$CLIENT_DIR/lib/test/test.qos.factory.js"
    "$CLIENT_DIR/lib/test/test.qos.peerSession.js"
    "$CLIENT_DIR/lib/test/test.qos.planner.js"
    "$CLIENT_DIR/lib/test/test.qos.probe.js"
    "$CLIENT_DIR/lib/test/test.qos.sampler.js"
    "$CLIENT_DIR/lib/test/test.qos.signalChannel.js"
    "$CLIENT_DIR/lib/test/test.qos.signals.js"
    "$CLIENT_DIR/lib/test/test.qos.stateMachine.js"
    "$CLIENT_DIR/lib/test/test.qos.statsProvider.js"
  )
  local missing=()
  for test_path in "${tests[@]}"; do
    [[ -f "$test_path" ]] || missing+=("$test_path")
  done
  ((${#missing[@]} == 0)) || fail "missing client QoS JS tests: ${missing[*]}"

  # Keep default full run stable: protocol/traceReplay fixtures are not checked
  # into src/client/lib/test/fixtures in this repo snapshot, so those suites are
  # intentionally omitted from the default entrypoint.
  ((${#tests[@]} > 0)) || fail "no client QoS JS tests found under $CLIENT_DIR/lib/test"

  run_cmd \
    "client-js" \
    --cwd "$ROOT_DIR" \
    bash \
    -lc \
    "cd '$CLIENT_DIR' && '$JEST_BIN' --runTestsByPath ${tests[*]@Q} --runInBand --testRegex '.*'"
}

run_cpp_unit() {
  require_file "$BUILD_DIR/mediasoup_tests"
  ensure_target_built \
    mediasoup_tests \
    "$BUILD_DIR/mediasoup_tests" \
    "$ROOT_DIR/tests/test_qos_protocol.cpp" \
    "$ROOT_DIR/tests/test_qos_validator.cpp" \
    "$ROOT_DIR/tests/test_qos_registry.cpp" \
    "$ROOT_DIR/tests/test_qos_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_room_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_override.cpp"
  run_cmd \
    "cpp-unit" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_tests" \
    "--gtest_filter=QosProtocolTest.*:QosValidatorTest.*:QosRegistryTest.*:QosAggregatorTest.*:QosRoomAggregatorTest.*:QosOverrideBuilderTest.*"
}

run_cpp_integration() {
  require_file "$BUILD_DIR/mediasoup_qos_integration_tests"
  ensure_target_built \
    mediasoup_qos_integration_tests \
    "$BUILD_DIR/mediasoup_qos_integration_tests" \
    "$ROOT_DIR/tests/test_qos_integration.cpp"
  prepare_test_port 14011 "QoS integration test SFU port 14011"
  run_cmd \
    "cpp-integration" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_qos_integration_tests" \
    "--gtest_filter=QosIntegrationTest.ClientStatsQosStoredAndAggregated:QosIntegrationTest.ClientStatsQosInBroadcast:QosIntegrationTest.InvalidClientStatsRejected:QosIntegrationTest.OlderClientStatsSeqIsIgnored:QosIntegrationTest.JoinReceivesQosPolicyNotification:QosIntegrationTest.SetQosOverrideNotifiesTargetPeer:QosIntegrationTest.ManualQosOverrideClear:QosIntegrationTest.SetQosPolicyNotifiesTargetPeer:QosIntegrationTest.AutomaticQosOverrideOnPoorQuality:QosIntegrationTest.AutomaticQosOverrideOnLostQuality:QosIntegrationTest.AutomaticQosOverrideClearsWhenQualityRecovers:QosIntegrationTest.ConnectionQualityNotificationDelivered:QosIntegrationTest.RoomQosStateAndRoomPressureOverride:QosIntegrationTest.RoomLostPeerPressureOverride"
}

run_cpp_accuracy() {
  require_file "$BUILD_DIR/mediasoup_qos_accuracy_tests"
  ensure_target_built \
    mediasoup_qos_accuracy_tests \
    "$BUILD_DIR/mediasoup_qos_accuracy_tests" \
    "$ROOT_DIR/tests/test_qos_accuracy.cpp"
  run_cmd \
    "cpp-accuracy" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_qos_accuracy_tests"
}

run_cpp_recording() {
  require_file "$BUILD_DIR/mediasoup_qos_recording_accuracy_tests"
  ensure_target_built \
    mediasoup_qos_recording_accuracy_tests \
    "$BUILD_DIR/mediasoup_qos_recording_accuracy_tests" \
    "$ROOT_DIR/tests/test_qos_recording_accuracy.cpp"
  run_cmd \
    "cpp-recording" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_qos_recording_accuracy_tests"
}

run_node_harness() {
  prepare_test_port 14011 "QoS node harness SFU port 14011"
  local scenarios=(
    publish_snapshot
    stale_seq
    policy_update
    auto_override_poor
    override_force_audio_only
    manual_clear
  )

  local failed=0
  for scenario in "${scenarios[@]}"; do
    if ! run_cmd \
      "node-harness:$scenario" \
      --cwd "$ROOT_DIR" \
      node "$ROOT_DIR/tests/qos_harness/run.mjs" "$scenario"; then
      failed=1
    fi
  done
  return "$failed"
}

run_browser_harness() {
  prepare_test_port 14012 "QoS browser harness SFU port 14012"
  local failed=0
  if ! run_cmd \
    "browser-harness:server-signal" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_server_signal.mjs"; then
    failed=1
  fi

  if ! run_cmd \
    "browser-harness:loopback" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_loopback.mjs"; then
    failed=1
  fi
  return "$failed"
}

run_matrix() {
  prepare_test_port 14011 "QoS matrix loopback port 14011"
  run_cmd \
    "matrix" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/run_matrix.mjs"
}

run_group() {
  local group="$1"
  case "$group" in
    client-js) run_client_js ;;
    cpp-unit) run_cpp_unit ;;
    cpp-integration) run_cpp_integration ;;
    cpp-accuracy) run_cpp_accuracy ;;
    cpp-recording) run_cpp_recording ;;
    node-harness) run_node_harness ;;
    browser-harness) run_browser_harness ;;
    matrix) run_matrix ;;
    *) fail "internal error: unsupported group '$group'" ;;
  esac
}

run_target() {
  local target="$1"
  case "$target" in
    client-js|cpp-unit|cpp-integration|cpp-accuracy|cpp-recording|node-harness|browser-harness|matrix)
      run_group "$target"
      ;;
    node-harness:*)
      prepare_test_port 14011 "QoS node harness SFU port 14011"
      local scenario="${target#node-harness:}"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/run.mjs" "$scenario"
      ;;
    browser-harness:server-signal)
      prepare_test_port 14012 "QoS browser harness SFU port 14012"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_server_signal.mjs"
      ;;
    browser-harness:loopback)
      prepare_test_port 14012 "QoS browser harness SFU port 14012"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_loopback.mjs"
      ;;
    *)
      fail "unknown target: $target"
      ;;
  esac
}

while (($# > 0)); do
  case "$1" in
    --skip-browser)
      SKIP_BROWSER=1
      ;;
    --skip-matrix)
      SKIP_MATRIX=1
      ;;
    --resume)
      RESUME_MODE=1
      ;;
    --list)
      list_groups
      exit 0
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      SELECTED_GROUPS+=("$1")
      ;;
  esac
  shift
done

mapfile -t GROUPS_TO_RUN < <(normalize_groups)
((${#GROUPS_TO_RUN[@]} > 0)) || fail "no groups selected after applying options"

for group in "${GROUPS_TO_RUN[@]}"; do
  if [[ "$group" == "matrix" ]]; then
    GENERATE_CASE_REPORT=1
    break
  fi
done

FAILED_TASKS=()

echo "QoS test root: $ROOT_DIR"
if ((RESUME_MODE)); then
  echo "Resume targets:"
else
  echo "Groups:"
fi
for group in "${GROUPS_TO_RUN[@]}"; do
  echo "  - $group"
done

for group in "${GROUPS_TO_RUN[@]}"; do
  failed_count_before=${#FAILED_TASKS[@]}
  if ! run_target "$group"; then
    if ((${#FAILED_TASKS[@]} == failed_count_before)); then
      record_failed_task "$group"
    fi
    FAILED_GROUPS+=("$group")
  fi
done

{
  printf '# last updated: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %Z')"
  if ((${#FAILED_TASKS[@]} == 0)); then
    printf '# no failed tasks\n'
  else
    printf '%s\n' "${FAILED_TASKS[@]}" | awk '!seen[$0]++'
  fi
} > "$FAILURES_FILE"

if ((GENERATE_CASE_REPORT)) && [[ -f "$CASE_REPORT_JSON" && -f "$CASE_REPORT_SCRIPT" ]]; then
  echo
  echo "==> [case-report]"
  if node \
    "$CASE_REPORT_SCRIPT" \
    "--input=$CASE_REPORT_JSON" \
    "--output=$ROOT_DIR/docs/uplink-qos-case-results.md"; then
    echo "<== [case-report] PASS"
  else
    echo "<== [case-report] WARN (generation failed)" >&2
  fi
fi

echo
if ((${#FAILED_GROUPS[@]} == 0)); then
  echo "All selected QoS test groups passed."
else
  echo "Completed with failures in group(s): ${FAILED_GROUPS[*]}" >&2
  exit 1
fi
