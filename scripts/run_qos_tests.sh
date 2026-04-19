#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLIENT_DIR="$ROOT_DIR/src/client"
BUILD_DIR="$ROOT_DIR/build"
JEST_BIN="$CLIENT_DIR/node_modules/.bin/jest"
CASE_REPORT_SCRIPT="$ROOT_DIR/tests/qos_harness/render_case_report.mjs"
ARTIFACTS_DIR="$ROOT_DIR/tests/qos_harness/artifacts"
FAILURES_FILE="$ARTIFACTS_DIR/last-failures.txt"
DOWNLINK_SUMMARY_FILE="$ROOT_DIR/docs/downlink-qos-test-results-summary.md"
GENERATE_CASE_REPORT=0
GENERATE_DOWNLINK_CASE_REPORT=0
GENERATE_CPP_CLIENT_CASE_REPORT=0
MATRIX_INCLUDE_EXTENDED=0
MATRIX_CASES=""

ALL_GROUPS=(
  client-js
  cpp-unit
  cpp-integration
  cpp-accuracy
  cpp-recording
  cpp-client-matrix
  cpp-client-harness
  cpp-threaded
  node-harness
  browser-harness
  matrix
  downlink-matrix
)

SELECTED_GROUPS=()
SKIP_BROWSER=0
SKIP_MATRIX=0
FAILED_GROUPS=()
FAILED_TASKS=()
declare -A TASK_RESULTS=()
declare -A TASK_DURATIONS=()
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
  --matrix-include-extended
                    matrix 额外包含剩余 extended 场景（当前主要是高带宽 baseline）
  --matrix-cases=... 只跑指定 matrix case，例如 T9,T10,T11
  --resume          只重跑上次失败的精确任务
  --list            列出可用分组
  -h, --help        显示帮助

Available groups:
  client-js         客户端 QoS JS 单测（test.qos.*.js）
  cpp-unit          服务端 QoS 相关单测（包含 uplink/downlink QoS 单测）
  cpp-integration   服务端 QoS 集成测试（包含 uplink/downlink QoS 集成测试）
  cpp-accuracy      QoS accuracy 测试
  cpp-recording     QoS recording accuracy 测试
  cpp-client-matrix PlainTransport C++ client weak-network matrix（run_cpp_client_matrix.mjs）
  cpp-client-harness PlainTransport C++ client signaling / publish snapshot / override harness
  cpp-threaded      PlainTransport C++ client threaded gtest / threaded harness regression
  node-harness      Node QoS harness 场景
  browser-harness   browser_server_signal + browser_loopback + downlink browser harnesses
  matrix            browser loopback full matrix（run_matrix.mjs）

Notes:
  - 默认会顺序执行所有分组；单个任务失败后会继续执行其余选中项，最后统一汇总失败。
  - matrix 运行时间最长，并且依赖浏览器 / netem 环境。
  - 可用环境变量 QOS_MATRIX_SPEED 调整 matrix 用时。
  - 默认 matrix 已包含 `T9/T10/T11`；`--matrix-include-extended` 会额外加入剩余 extended 场景。
  - `--matrix-cases=...` 会把 matrix 切为 targeted 运行，并产出 targeted 报告。
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
    "tests/qos_harness/run_cpp_client_matrix.mjs"
    "tests/qos_harness/browser_server_signal.mjs"
    "tests/qos_harness/browser_loopback.mjs"
    "tests/qos_harness/browser_downlink_controls.mjs"
    "tests/qos_harness/browser_downlink_e2e.mjs"
    "tests/qos_harness/browser_downlink_priority.mjs"
    "tests/qos_harness/browser_downlink_v2.mjs"
    "tests/qos_harness/browser_downlink_v3.mjs"
    "tests/qos_harness/run_matrix.mjs"
    "client/build/plain-client 127.0.0.1 14"
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

ensure_plain_client_built() {
  local build_dir="$ROOT_DIR/client/build"
  local binary="$build_dir/plain-client"

  require_file "$build_dir/Makefile"

  local rebuild=0
  if [[ ! -x "$binary" ]]; then
    rebuild=1
  elif find "$ROOT_DIR/client" -maxdepth 2 \
      \( -name '*.cpp' -o -name '*.h' -o -name 'CMakeLists.txt' \) \
      -newer "$binary" | grep -q .; then
    rebuild=1
  fi

  if ((rebuild)); then
    run_cmd \
      "build:plain-client" \
      --cwd "$build_dir" \
      cmake --build . --target plain-client
  fi

  require_file "$binary"
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
    TASK_RESULTS["$label"]="PASS"
    TASK_DURATIONS["$label"]="${elapsed}s"
    echo "<== [$label] PASS (${elapsed}s)"
  else
    TASK_RESULTS["$label"]="FAIL"
    TASK_DURATIONS["$label"]="${elapsed}s"
    echo "<== [$label] FAIL (${elapsed}s, rc=$rc)" >&2
    record_failed_task "$label"
    log_system_snapshot "after-fail:$label"
  fi
  return "$rc"
}

log_system_snapshot() {
  local label="$1"
  echo
  echo "==> [system:$label]"
  echo "timestamp_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
  awk '
    /MemTotal:|MemAvailable:|SwapTotal:|SwapFree:/ {
      printf "%s=%s%s ", $1, $2, $3
    }
    END { print "" }
  ' /proc/meminfo 2>/dev/null || true
  if command -v ps >/dev/null 2>&1; then
    echo "top_rss_processes:"
    ps -eo pid,ppid,rss,stat,comm,args --sort=-rss | head -n 12 || true
    echo "browser_and_runner_processes:"
    ps -eo pid,ppid,rss,stat,comm,args | awk '
      NR == 1 || /headless_shell|esbuild|cc1plus|mediasoup-sfu|run_matrix\.mjs|run_qos_tests\.sh|browser_downlink|browser_loopback/
    ' || true
  fi
}

join_targets_for_markdown() {
  local joined=""
  local item
  for item in "$@"; do
    if [[ -n "$joined" ]]; then
      joined+=", "
    fi
    joined+="\`$item\`"
  done
  printf '%s\n' "$joined"
}

downlink_report_labels() {
  printf '%s\n' \
    "cpp-unit" \
    "cpp-integration" \
    "browser-harness:downlink-controls" \
    "browser-harness:downlink-e2e" \
    "browser-harness:downlink-priority" \
    "browser-harness:downlink-v2" \
    "browser-harness:downlink-v3" \
    "downlink-matrix"
}

downlink_task_category() {
  local label="$1"
  case "$label" in
    cpp-*) printf 'server\n' ;;
    browser-harness:*) printf 'browser\n' ;;
    downlink-matrix) printf 'browser\n' ;;
    *) printf 'other\n' ;;
  esac
}

downlink_task_description() {
  local label="$1"
  case "$label" in
    cpp-unit)
      printf '服务端 downlink QoS 相关单测（allocator / planner / aggregator / publisher supply）\n'
      ;;
    cpp-integration)
      printf '服务端 downlink QoS 集成测试（consumer state、publisher clamp、stale snapshot 回归）\n'
      ;;
    browser-harness:downlink-controls)
      printf '浏览器信令控制验证：pause / resume / requestKeyFrame 基本控制链路\n'
      ;;
    browser-harness:downlink-e2e)
      printf '浏览器端到端验证：downlinkClientStats -> consumer pause/resume / priority\n'
      ;;
    browser-harness:downlink-priority)
      printf '浏览器弱网竞争验证：高优先级 subscriber 分配优于低优先级\n'
      ;;
    browser-harness:downlink-v2)
      printf '浏览器 v2 验证：subscriber demand -> track-scoped publisher clamp / clear / zero-demand hold\n'
      ;;
    browser-harness:downlink-v3)
      printf '浏览器 v3 验证：sustained zero-demand -> pauseUpstream / resumeUpstream / flicker 防抖\n'
      ;;
    downlink-matrix)
      printf 'downlink 弱网矩阵：baseline / bw / loss / rtt / jitter / transition / competition / zero-demand\n'
      ;;
    *)
      printf '%s\n' "$label"
      ;;
  esac
}

downlink_task_command() {
  local label="$1"
  case "$label" in
    cpp-unit)
      printf './build/mediasoup_qos_unit_tests\n'
      ;;
    cpp-integration)
      printf './build/mediasoup_qos_integration_tests\n'
      ;;
    browser-harness:downlink-controls)
      printf 'node tests/qos_harness/browser_downlink_controls.mjs\n'
      ;;
    browser-harness:downlink-e2e)
      printf 'node tests/qos_harness/browser_downlink_e2e.mjs\n'
      ;;
    browser-harness:downlink-priority)
      printf 'node tests/qos_harness/browser_downlink_priority.mjs\n'
      ;;
    browser-harness:downlink-v2)
      printf 'node tests/qos_harness/browser_downlink_v2.mjs\n'
      ;;
    browser-harness:downlink-v3)
      printf 'node tests/qos_harness/browser_downlink_v3.mjs\n'
      ;;
    downlink-matrix)
      printf 'node tests/qos_harness/run_downlink_matrix.mjs\n'
      ;;
    *)
      printf '-\n'
      ;;
  esac
}

markdown_anchor() {
  printf '%s' "$1" | tr '[:upper:]' '[:lower:]' | sed -E 's/[^a-z0-9]+/-/g; s/^-+//; s/-+$//'
}

join_markdown_links() {
  local joined=""
  local item
  for item in "$@"; do
    [[ -n "$item" ]] || continue
    local anchor
    anchor="$(markdown_anchor "$item")"
    local link="[$item](#$anchor)"
    if [[ -n "$joined" ]]; then
      joined+="、"
    fi
    joined+="$link"
  done
  printf '%s\n' "${joined:-无}"
}

write_downlink_report() {
  mapfile -t labels < <(downlink_report_labels)
  local generated_at
  generated_at="$(date -u '+%Y-%m-%dT%H:%M:%S.%3NZ')"
  local selected_targets
  selected_targets="$(join_targets_for_markdown "${GROUPS_TO_RUN[@]}")"
  local total=${#labels[@]}
  local executed=0 passed=0 failed=0 not_run=0
  local failed_labels=()
  local server_labels=()
  local browser_labels=()
  local label status duration category

  for label in "${labels[@]}"; do
    status="${TASK_RESULTS[$label]:-NOT_RUN}"
    case "$status" in
      PASS) ((executed += 1)); ((passed += 1)) ;;
      FAIL) ((executed += 1)); ((failed += 1)); failed_labels+=("$label") ;;
      *) ((not_run += 1)) ;;
    esac

    category="$(downlink_task_category "$label")"
    case "$category" in
      server) server_labels+=("$label") ;;
      browser) browser_labels+=("$label") ;;
    esac
  done

  mkdir -p "$(dirname "$DOWNLINK_SUMMARY_FILE")"
  {
    echo "# 下行 QoS 测试结果汇总"
    echo
    echo "生成时间：\`$generated_at\`"
    echo
    echo "## 1. 汇总"
    echo
    echo "- 总任务：\`$total\`"
    echo "- 已执行：\`$executed\`"
    echo "- 通过：\`$passed\`"
    echo "- 失败：\`$failed\`"
    echo "- 未执行：\`$not_run\`"
    echo "- 执行脚本：\`scripts/run_qos_tests.sh\`"
    echo "- 本次选择目标：${selected_targets:-无}"
    echo
    if ((${#failed_labels[@]} > 0)); then
      echo "### 1.1 失败任务"
      echo
      echo "| 任务 | 结果 | 耗时 |"
      echo "|---|---|---|"
      for label in "${failed_labels[@]}"; do
        duration="${TASK_DURATIONS[$label]:--}"
        echo "| [\`$label\`](#$(markdown_anchor "$label")) | \`${TASK_RESULTS[$label]}\` | \`${duration}\` |"
      done
      echo
    else
      echo "### 1.1 失败任务"
      echo
      echo "- 无"
      echo
    fi

    echo "## 2. 快速跳转"
    echo
    echo "- 失败任务：$(join_markdown_links "${failed_labels[@]}")"
    echo "- server：$(join_markdown_links "${server_labels[@]}")"
    echo "- browser：$(join_markdown_links "${browser_labels[@]}")"
    echo
    echo "## 3. 逐项结果"
    echo

    for label in "${labels[@]}"; do
      status="${TASK_RESULTS[$label]:-NOT_RUN}"
      duration="${TASK_DURATIONS[$label]:--}"
      category="$(downlink_task_category "$label")"
      echo "### $label"
      echo
      echo "| 字段 | 内容 |"
      echo "|---|---|"
      echo "| 任务 ID | \`$label\` |"
      echo "| 类别 | \`$category\` |"
      echo "| 说明 | $(downlink_task_description "$label") |"
      echo "| 状态 | \`$status\` |"
      echo "| 耗时 | \`$duration\` |"
      echo "| 对应命令 | \`$(downlink_task_command "$label")\` |"
      echo
    done
  } > "$DOWNLINK_SUMMARY_FILE"
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
      if [[ "$group" == node-harness:* || "$group" == browser-harness:* || "$group" == cpp-client-harness:* ]]; then
        requested+=("$group")
        continue
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
      [[ "$group" == "browser-harness" || "$group" == browser-harness:* || "$group" == "matrix" || "$group" == "downlink-matrix" ]] && continue
      filtered+=("$group")
    done
    requested=("${filtered[@]}")
  elif ((SKIP_MATRIX)); then
    local filtered=()
    for group in "${requested[@]}"; do
      [[ "$group" == "matrix" || "$group" == "downlink-matrix" ]] && continue
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
  require_file "$BUILD_DIR/mediasoup_qos_unit_tests"
  ensure_target_built \
    mediasoup_qos_unit_tests \
    "$BUILD_DIR/mediasoup_qos_unit_tests" \
    "$ROOT_DIR/CMakeLists.txt" \
    "$ROOT_DIR/tests/test_client_qos.cpp" \
    "$ROOT_DIR/client/qos/QosConstants.h" \
    "$ROOT_DIR/client/qos/QosController.h" \
    "$ROOT_DIR/client/qos/QosCoordinator.h" \
    "$ROOT_DIR/client/qos/QosExecutor.h" \
    "$ROOT_DIR/client/qos/QosPlanner.h" \
    "$ROOT_DIR/client/qos/QosProbe.h" \
    "$ROOT_DIR/client/qos/QosProfiles.h" \
    "$ROOT_DIR/client/qos/QosProtocol.h" \
    "$ROOT_DIR/client/qos/QosSignals.h" \
    "$ROOT_DIR/client/qos/QosStateMachine.h" \
    "$ROOT_DIR/client/qos/QosTypes.h" \
    "$ROOT_DIR/tests/test_downlink_allocator.cpp" \
    "$ROOT_DIR/tests/test_downlink_health.cpp" \
    "$ROOT_DIR/tests/test_downlink_v2.cpp" \
    "$ROOT_DIR/tests/test_qos_unit.cpp" \
    "$ROOT_DIR/tests/test_qos_protocol.cpp" \
    "$ROOT_DIR/tests/test_qos_validator.cpp" \
    "$ROOT_DIR/tests/test_qos_registry.cpp" \
    "$ROOT_DIR/tests/test_qos_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_room_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_override.cpp" \
    "$ROOT_DIR/tests/test_thread_model.cpp" \
    "$ROOT_DIR/client/ThreadedControlHelpers.h"
  run_cmd \
    "cpp-unit" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_qos_unit_tests"
}

run_cpp_integration() {
  require_file "$BUILD_DIR/mediasoup_qos_integration_tests"
  ensure_target_built \
    mediasoup_qos_integration_tests \
    "$BUILD_DIR/mediasoup_qos_integration_tests" \
    "$ROOT_DIR/CMakeLists.txt" \
    "$ROOT_DIR/tests/test_qos_integration.cpp"
  prepare_test_port 14011 "QoS integration test SFU port 14011"
  run_cmd \
    "cpp-integration" \
    --cwd "$ROOT_DIR" \
    "$BUILD_DIR/mediasoup_qos_integration_tests"
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

run_cpp_client_matrix() {
  require_file "$BUILD_DIR/mediasoup-sfu"
  ensure_target_built \
    mediasoup-sfu \
    "$BUILD_DIR/mediasoup-sfu" \
    "$ROOT_DIR/src/main.cpp" \
    "$ROOT_DIR/src/SignalingServer.cpp" \
    "$ROOT_DIR/src/RoomService.cpp"
  ensure_plain_client_built
  prepare_test_port 14019 "QoS cpp-client matrix SFU port 14019"
  local matrix_args=()
  if ((MATRIX_INCLUDE_EXTENDED)); then
    matrix_args+=("--include-extended")
  fi
  if [[ -n "$MATRIX_CASES" ]]; then
    matrix_args+=("--cases=$MATRIX_CASES")
  fi
  run_cmd \
    "cpp-client-matrix" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/run_cpp_client_matrix.mjs" "${matrix_args[@]}"
}

run_cpp_client_harness() {
  require_file "$BUILD_DIR/mediasoup-sfu"
  ensure_target_built \
    mediasoup-sfu \
    "$BUILD_DIR/mediasoup-sfu" \
    "$ROOT_DIR/src/main.cpp" \
    "$ROOT_DIR/src/SignalingServer.cpp" \
    "$ROOT_DIR/src/RoomService.cpp"
  ensure_plain_client_built
  prepare_test_port 14020 "QoS cpp-client harness SFU port 14020"
  local scenarios=(
    publish_snapshot
    stale_seq
    policy_update
    auto_override_poor
    override_force_audio_only
    manual_clear
    multi_video_budget
    multi_track_snapshot
    threaded_quick
  )

  local failed=0
  for scenario in "${scenarios[@]}"; do
    if ! run_cmd \
      "cpp-client-harness:$scenario" \
      --cwd "$ROOT_DIR" \
      env QOS_CPP_CLIENT_HARNESS_PORT=14020 \
      node "$ROOT_DIR/tests/qos_harness/run_cpp_client_harness.mjs" "$scenario"; then
      failed=1
    fi
  done
  return "$failed"
}

run_cpp_threaded() {
  require_file "$BUILD_DIR/mediasoup_thread_integration_tests"
  ensure_target_built \
    mediasoup_thread_integration_tests \
    "$BUILD_DIR/mediasoup_thread_integration_tests" \
    "$ROOT_DIR/tests/test_thread_integration.cpp" \
    "$ROOT_DIR/tests/TestWsClient.h" \
    "$ROOT_DIR/tests/TestProcessUtils.h" \
    "$ROOT_DIR/client/ThreadTypes.h" \
    "$ROOT_DIR/client/ThreadedControlHelpers.h" \
    "$ROOT_DIR/client/NetworkThread.h" \
    "$ROOT_DIR/client/SourceWorker.h" \
    "$ROOT_DIR/client/main.cpp"
  ensure_plain_client_built
  prepare_test_port 14021 "threaded integration SFU port 14021"
  run_cmd \
    "cpp-threaded:gtest" \
    --cwd "$ROOT_DIR" \
    env QOS_THREAD_INTEGRATION_PORT=14021 \
    "$BUILD_DIR/mediasoup_thread_integration_tests"
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
  prepare_test_port 14013 "Downlink control harness SFU port 14013"
  prepare_test_port 14014 "Downlink E2E harness SFU port 14014"
  prepare_test_port 14015 "Downlink priority harness SFU port 14015"
  prepare_test_port 14016 "Downlink v2 harness SFU port 14016"
  prepare_test_port 14017 "Downlink v3 harness SFU port 14017"
  log_system_snapshot "pre-browser-harness"
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

  if ! run_cmd \
    "browser-harness:downlink-controls" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_downlink_controls.mjs"; then
    failed=1
  fi

  if ! run_cmd \
    "browser-harness:downlink-e2e" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_downlink_e2e.mjs"; then
    failed=1
  fi

  if ! run_cmd \
    "browser-harness:downlink-priority" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_downlink_priority.mjs"; then
    failed=1
  fi

  if ! run_cmd \
    "browser-harness:downlink-v2" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_downlink_v2.mjs"; then
    failed=1
  fi

  if ! run_cmd \
    "browser-harness:downlink-v3" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_downlink_v3.mjs"; then
    failed=1
  fi
  return "$failed"
}

run_matrix() {
  prepare_test_port 14011 "QoS matrix loopback port 14011"
  local matrix_args=()
  if ((MATRIX_INCLUDE_EXTENDED)); then
    matrix_args+=("--include-extended")
  fi
  if [[ -n "$MATRIX_CASES" ]]; then
    matrix_args+=("--cases=$MATRIX_CASES")
  fi
  log_system_snapshot "pre-matrix"
  run_cmd \
    "matrix" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/run_matrix.mjs" "${matrix_args[@]}"
}

run_downlink_matrix() {
  prepare_test_port 14018 "Downlink matrix SFU port 14018"
  local dl_args=()
  if [[ -n "$MATRIX_CASES" ]]; then
    dl_args+=("--cases=$MATRIX_CASES")
  fi
  log_system_snapshot "pre-downlink-matrix"
  run_cmd \
    "downlink-matrix" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/run_downlink_matrix.mjs" "${dl_args[@]}"
}

run_group() {
  local group="$1"
  case "$group" in
    client-js) run_client_js ;;
    cpp-unit) run_cpp_unit ;;
    cpp-integration) run_cpp_integration ;;
    cpp-accuracy) run_cpp_accuracy ;;
    cpp-recording) run_cpp_recording ;;
    cpp-client-matrix) run_cpp_client_matrix ;;
    cpp-client-harness) run_cpp_client_harness ;;
    cpp-threaded) run_cpp_threaded ;;
    node-harness) run_node_harness ;;
    browser-harness) run_browser_harness ;;
    matrix) run_matrix ;;
    downlink-matrix) run_downlink_matrix ;;
    *) fail "internal error: unsupported group '$group'" ;;
  esac
}

run_target() {
  local target="$1"
  case "$target" in
    client-js|cpp-unit|cpp-integration|cpp-accuracy|cpp-recording|cpp-client-matrix|cpp-client-harness|cpp-threaded|node-harness|browser-harness|matrix|downlink-matrix)
      run_group "$target"
      ;;
    cpp-client-harness:*)
      prepare_test_port 14020 "QoS cpp-client harness SFU port 14020"
      local scenario="${target#cpp-client-harness:}"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        env QOS_CPP_CLIENT_HARNESS_PORT=14020 \
        node "$ROOT_DIR/tests/qos_harness/run_cpp_client_harness.mjs" "$scenario"
      ;;
    cpp-threaded:*)
      prepare_test_port 14021 "threaded integration SFU port 14021"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        env QOS_THREAD_INTEGRATION_PORT=14021 \
        "$BUILD_DIR/mediasoup_thread_integration_tests"
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
    browser-harness:downlink-controls)
      prepare_test_port 14013 "Downlink control harness SFU port 14013"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_controls.mjs"
      ;;
    browser-harness:downlink-e2e)
      prepare_test_port 14014 "Downlink E2E harness SFU port 14014"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_e2e.mjs"
      ;;
    browser-harness:downlink-priority)
      prepare_test_port 14015 "Downlink priority harness SFU port 14015"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_priority.mjs"
      ;;
    browser-harness:downlink-v2)
      prepare_test_port 14016 "Downlink v2 harness SFU port 14016"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_v2.mjs"
      ;;
    browser-harness:downlink-v3)
      prepare_test_port 14017 "Downlink v3 harness SFU port 14017"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_v3.mjs"
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
    --matrix-include-extended)
      MATRIX_INCLUDE_EXTENDED=1
      ;;
    --matrix-cases=*)
      MATRIX_CASES="${1#--matrix-cases=}"
      [[ -n "$MATRIX_CASES" ]] || fail "matrix cases cannot be empty"
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
  fi
  if [[ "$group" == "downlink-matrix" ]]; then
    GENERATE_DOWNLINK_CASE_REPORT=1
  fi
  if [[ "$group" == "cpp-client-matrix" ]]; then
    GENERATE_CPP_CLIENT_CASE_REPORT=1
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

if ((GENERATE_CASE_REPORT)) && [[ -f "$CASE_REPORT_SCRIPT" ]]; then
  if [[ -n "$MATRIX_CASES" ]]; then
    CASE_REPORT_JSON="$ROOT_DIR/docs/generated/uplink-qos-matrix-report.targeted.json"
    CASE_REPORT_OUTPUT="$ROOT_DIR/docs/generated/uplink-qos-case-results.targeted.md"
  else
    CASE_REPORT_JSON="$ROOT_DIR/docs/generated/uplink-qos-matrix-report.json"
    CASE_REPORT_OUTPUT="$ROOT_DIR/docs/uplink-qos-case-results.md"
  fi

  if [[ ! -f "$CASE_REPORT_JSON" ]]; then
    echo
    echo "<== [case-report] WARN (matrix json not found: $CASE_REPORT_JSON)" >&2
  else
  echo
  echo "==> [case-report]"
  if node \
    "$CASE_REPORT_SCRIPT" \
    "--input=$CASE_REPORT_JSON" \
    "--output=$CASE_REPORT_OUTPUT"; then
    echo "<== [case-report] PASS"
  else
    echo "<== [case-report] WARN (generation failed)" >&2
  fi
  fi
fi

CPP_CLIENT_CASE_REPORT_SCRIPT="$ROOT_DIR/tests/qos_harness/render_cpp_client_case_report.mjs"
if ((GENERATE_CPP_CLIENT_CASE_REPORT)) && [[ -f "$CPP_CLIENT_CASE_REPORT_SCRIPT" ]]; then
  if [[ -n "$MATRIX_CASES" ]]; then
    CPP_CLIENT_CASE_REPORT_JSON="$ROOT_DIR/docs/generated/uplink-qos-cpp-client-matrix-report.targeted.json"
    CPP_CLIENT_CASE_REPORT_OUTPUT="$ROOT_DIR/docs/generated/uplink-qos-cpp-client-case-results.targeted.md"
  else
    CPP_CLIENT_CASE_REPORT_JSON="$ROOT_DIR/docs/generated/uplink-qos-cpp-client-matrix-report.json"
    CPP_CLIENT_CASE_REPORT_OUTPUT="$ROOT_DIR/docs/plain-client-qos-case-results.md"
  fi

  if [[ ! -f "$CPP_CLIENT_CASE_REPORT_JSON" ]]; then
    echo
    echo "<== [cpp-client-case-report] WARN (matrix json not found: $CPP_CLIENT_CASE_REPORT_JSON)" >&2
  else
    echo
    echo "==> [cpp-client-case-report]"
    if node \
      "$CPP_CLIENT_CASE_REPORT_SCRIPT" \
      "--input=$CPP_CLIENT_CASE_REPORT_JSON" \
      "--output=$CPP_CLIENT_CASE_REPORT_OUTPUT"; then
      echo "<== [cpp-client-case-report] PASS"
    else
      echo "<== [cpp-client-case-report] WARN (generation failed)" >&2
    fi
  fi
fi

DOWNLINK_CASE_REPORT_SCRIPT="$ROOT_DIR/tests/qos_harness/render_downlink_case_report.mjs"
if ((GENERATE_DOWNLINK_CASE_REPORT)) && [[ -f "$DOWNLINK_CASE_REPORT_SCRIPT" ]]; then
  if [[ -n "$MATRIX_CASES" ]]; then
    DL_CASE_REPORT_JSON="$ROOT_DIR/docs/generated/downlink-qos-matrix-report.targeted.json"
    DL_CASE_REPORT_OUTPUT="$ROOT_DIR/docs/generated/downlink-qos-case-results.targeted.md"
  else
    DL_CASE_REPORT_JSON="$ROOT_DIR/docs/generated/downlink-qos-matrix-report.json"
    DL_CASE_REPORT_OUTPUT="$ROOT_DIR/docs/downlink-qos-case-results.md"
  fi

  if [[ ! -f "$DL_CASE_REPORT_JSON" ]]; then
    echo
    echo "<== [downlink-case-report] WARN (matrix json not found: $DL_CASE_REPORT_JSON)" >&2
  else
    echo
    echo "==> [downlink-case-report]"
    if node \
      "$DOWNLINK_CASE_REPORT_SCRIPT" \
      "--input=$DL_CASE_REPORT_JSON" \
      "--output=$DL_CASE_REPORT_OUTPUT"; then
      echo "<== [downlink-case-report] PASS"
    else
      echo "<== [downlink-case-report] WARN (generation failed)" >&2
    fi
  fi
fi

echo
echo "==> [downlink-report]"
if write_downlink_report; then
  echo "<== [downlink-report] PASS ($DOWNLINK_SUMMARY_FILE)"
else
  echo "<== [downlink-report] WARN (generation failed)" >&2
fi

echo
if ((${#FAILED_GROUPS[@]} == 0)); then
  echo "All selected QoS test groups passed."
else
  echo "Completed with failures in group(s): ${FAILED_GROUPS[*]}" >&2
  exit 1
fi
