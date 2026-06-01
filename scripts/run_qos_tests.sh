#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CLIENT_DIR="$ROOT_DIR/src/client"
BUILD_DIR="$ROOT_DIR/build"
HARNESS_DIR="$ROOT_DIR/tests/qos_harness"
JEST_BIN=""
ARTIFACTS_DIR="$ROOT_DIR/tests/qos_harness/artifacts"
FAILURES_FILE="$ARTIFACTS_DIR/last-failures.txt"
DOWNLINK_SUMMARY_FILE="$ROOT_DIR/docs/downlink-qos-test-results-summary.md"
GENERATE_DOWNLINK_CASE_REPORT=0
GENERATE_DOWNLINK_SUMMARY=1

DEFAULT_GROUPS=(
  client-js
  cpp-unit
  cpp-integration
  cpp-accuracy
  node-harness
  browser-harness
  downlink-matrix
  remote-harness
)
OPTIONAL_GROUPS=()
ALL_GROUPS=("${DEFAULT_GROUPS[@]}" "${OPTIONAL_GROUPS[@]}")

SELECTED_GROUPS=()
SKIP_BROWSER=0
FAILED_GROUPS=()
FAILED_TASKS=()
declare -A TASK_RESULTS=()
declare -A TASK_DURATIONS=()
RESUME_MODE=0
MATRIX_CASES=""

mkdir -p "$ARTIFACTS_DIR"

usage() {
  cat <<'EOF'
Usage:
  scripts/run_qos_tests.sh                # 默认全量跑 QoS 相关测试
  scripts/run_qos_tests.sh all
  scripts/run_qos_tests.sh client-js cpp-unit
  scripts/run_qos_tests.sh --resume

Options:
  --skip-browser    跳过 browser harness
  --resume          只重跑上次失败的精确任务
  --list            列出可用分组
  -h, --help        显示帮助

Available groups:
  client-js         客户端 QoS JS 单测（test.qos.*.js）
  cpp-unit          服务端 QoS 相关单测（包含 uplink/downlink QoS 单测）
  cpp-integration   服务端 QoS 集成测试（包含 uplink/downlink QoS 集成测试）
  cpp-accuracy      QoS accuracy 测试
  node-harness      Node QoS harness 场景
  browser-harness   browser_server_signal + downlink browser harnesses
  downlink-matrix   browser downlink weak-network matrix（run_downlink_matrix.mjs）
  remote-harness    先构建并部署测试机镜像，再跑远端 smoke + mediasoup-9000 有限压力 smoke

Notes:
  - 默认会顺序执行所有分组；单个任务失败后会继续执行其余选中项，最后统一汇总失败。
  - 失败任务会记录到 tests/qos_harness/artifacts/last-failures.txt
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
  elif command -v netstat >/dev/null 2>&1; then
    mapfile -t pids < <(
      netstat -ltnp 2>/dev/null |
        awk -v port=":$port" '
          $4 ~ port "$" {
            slash = index($7, "/")
            if (slash > 1) {
              print substr($7, 1, slash - 1)
            }
          }
        ' |
        sort -u
    )
  else
    fail "none of lsof, fuser, or netstat is available for test port cleanup"
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
    "tests/qos_harness/browser_downlink_controls.mjs"
    "tests/qos_harness/browser_downlink_e2e.mjs"
    "tests/qos_harness/browser_downlink_priority.mjs"
    "tests/qos_harness/browser_downlink_v2.mjs"
    "tests/qos_harness/browser_downlink_v3.mjs"
    "headless_shell .*puppeteer_dev_chrome_profile-"
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

ensure_harness_node_modules() {
  local required_bins=(
    "$HARNESS_DIR/node_modules/.bin/esbuild"
  )
  local required_modules=(
    "$HARNESS_DIR/node_modules/awaitqueue"
    "$HARNESS_DIR/node_modules/debug"
    "$HARNESS_DIR/node_modules/h264-profile-level-id"
    "$HARNESS_DIR/node_modules/npm-events-package"
    "$HARNESS_DIR/node_modules/puppeteer-core"
    "$HARNESS_DIR/node_modules/queue-microtask"
    "$HARNESS_DIR/node_modules/sdp-transform"
    "$HARNESS_DIR/node_modules/ua-parser-js"
  )
  local path

  for path in "${required_bins[@]}" "${required_modules[@]}"; do
    if [[ ! -e "$path" ]]; then
      command -v npm >/dev/null 2>&1 || fail "npm is required to install qos_harness dependencies"
      require_file "$HARNESS_DIR/package.json"
      echo "info: restoring qos_harness npm dependencies" >&2
      (
        cd "$HARNESS_DIR"
        npm install
      )
      return 0
    fi
  done
}

require_browser_runtime() {
  ensure_harness_node_modules
  command -v node >/dev/null 2>&1 || fail "node is required for browser QoS harnesses"

  local helper_path="$ROOT_DIR/tests/qos_harness/browser_runtime_helpers.mjs"
  require_file "$helper_path"

  local output
  if ! output="$(
    node --input-type=module -e "
      import { resolveChromiumExecutable } from '${helper_path}';
      console.log(resolveChromiumExecutable());
    " 2>&1
  )"; then
    fail "$output"
  fi

  [[ -n "$output" ]] || fail "browser runtime resolver returned empty executable path"
}

ensure_client_js_runtime() {
  ensure_harness_node_modules
  local candidates=(
    "$CLIENT_DIR/node_modules/.bin/jest"
    "$HARNESS_DIR/node_modules/.bin/jest"
  )
  local candidate
  local have_jest=0
  local have_fake_track=0

  for candidate in "${candidates[@]}"; do
    if [[ -x "$candidate" ]]; then
      JEST_BIN="$candidate"
      have_jest=1
      break
    fi
  done

  if [[ -d "$HARNESS_DIR/node_modules/fake-mediastreamtrack" ]]; then
    have_fake_track=1
  fi

  if ((have_jest && have_fake_track)); then
    return 0
  fi

  command -v npm >/dev/null 2>&1 || fail "npm is required to install client QoS JS test dependencies"
  require_file "$HARNESS_DIR/package.json"

  echo "info: installing temporary client QoS JS test dependencies into tests/qos_harness" >&2
  (
    cd "$HARNESS_DIR"
    npm install --no-save jest fake-mediastreamtrack
  )

  JEST_BIN="$HARNESS_DIR/node_modules/.bin/jest"
  [[ -x "$JEST_BIN" ]] || fail "unable to provision Jest for client QoS JS tests"
  [[ -d "$HARNESS_DIR/node_modules/fake-mediastreamtrack" ]] || \
    fail "unable to provision fake-mediastreamtrack for client QoS JS tests"
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

run_loopback_netem_preflight() {
  local label="$1"
  local args=(
    "$ROOT_DIR/tests/qos_harness/preflight_netem_guards.mjs"
    "--iface=lo"
  )
  if [[ "${QOS_FORCE_CLEAR_NETEM_GUARDS:-0}" == "1" ]]; then
    args+=("--force-clear-live")
  fi

  run_cmd \
    "$label" \
    --cwd "$ROOT_DIR" \
    node "${args[@]}"
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
      NR == 1 || /headless_shell|esbuild|cc1plus|mediasoup-sfu|run_qos_tests\.sh|browser_downlink/
    ' || true
  fi
}

clear_loopback_root_qdisc() {
  if [[ -x /usr/sbin/tc ]]; then
    /usr/sbin/tc qdisc del dev lo root 2>/dev/null || true
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
    requested=("${DEFAULT_GROUPS[@]}")
  else
    for group in "${SELECTED_GROUPS[@]}"; do
      if [[ "$group" == "all" ]]; then
        requested=("${DEFAULT_GROUPS[@]}")
        break
      fi
      if [[ "$group" == node-harness:* || "$group" == browser-harness:* || "$group" == remote-harness:* ]]; then
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
      [[ "$group" == "browser-harness" || "$group" == browser-harness:* || "$group" == "downlink-matrix" || "$group" == "remote-harness" || "$group" == remote-harness:* ]] && continue
      filtered+=("$group")
    done
    requested=("${filtered[@]}")
  fi

  printf '%s\n' "${requested[@]}"
}

run_client_js() {
  ensure_client_js_runtime

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
    "cd '$ROOT_DIR' && NODE_PATH='$HARNESS_DIR/node_modules' '$JEST_BIN' --config '{\"rootDir\":\"$ROOT_DIR\",\"testEnvironment\":\"node\",\"moduleDirectories\":[\"node_modules\",\"$HARNESS_DIR/node_modules\"],\"modulePathIgnorePatterns\":[\"<rootDir>/third_party/flatbuffers\",\"<rootDir>/src/mediasoup-worker-src/worker/subprojects/flatbuffers-24.3.6\"]}' --runTestsByPath ${tests[*]@Q} --runInBand --testRegex '.*'"
}

run_cpp_unit() {
  require_file "$BUILD_DIR/mediasoup_qos_unit_tests"
  ensure_target_built \
    mediasoup_qos_unit_tests \
    "$BUILD_DIR/mediasoup_qos_unit_tests" \
    "$ROOT_DIR/CMakeLists.txt" \
    "$ROOT_DIR/tests/test_downlink_allocator.cpp" \
    "$ROOT_DIR/tests/test_downlink_health.cpp" \
    "$ROOT_DIR/tests/test_downlink_v2.cpp" \
    "$ROOT_DIR/tests/test_qos_unit.cpp" \
    "$ROOT_DIR/tests/test_qos_protocol.cpp" \
    "$ROOT_DIR/tests/test_qos_validator.cpp" \
    "$ROOT_DIR/tests/test_qos_registry.cpp" \
    "$ROOT_DIR/tests/test_qos_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_room_aggregator.cpp" \
    "$ROOT_DIR/tests/test_qos_override.cpp"
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

run_node_harness() {
  prepare_test_port 14011 "QoS node harness SFU port 14011"
  local failed=0
  if ! run_cmd \
    "node-harness:netem-guard" \
    --cwd "$ROOT_DIR" \
    node --test "$ROOT_DIR/tests/qos_harness/test.netem_guard.mjs"; then
    failed=1
  fi
  local scenarios=(
    publish_snapshot
    stale_seq
    policy_update
    auto_override_poor
    override_force_audio_only
    manual_clear
  )

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
  require_browser_runtime
  prepare_test_port 14012 "QoS browser harness SFU port 14012"
  prepare_test_port 14013 "Downlink control harness SFU port 14013"
  prepare_test_port 14014 "Downlink E2E harness SFU port 14014"
  prepare_test_port 14015 "Downlink priority harness SFU port 14015"
  prepare_test_port 14016 "Downlink v2 harness SFU port 14016"
  prepare_test_port 14017 "Downlink v3 harness SFU port 14017"
  clear_loopback_root_qdisc
  if ! run_loopback_netem_preflight "browser-harness:netem-preflight"; then
    clear_loopback_root_qdisc
    return 1
  fi
  log_system_snapshot "pre-browser-harness"
  local failed=0
  if ! run_cmd \
    "browser-harness:server-signal" \
    --cwd "$ROOT_DIR" \
    node "$ROOT_DIR/tests/qos_harness/browser_server_signal.mjs"; then
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
  clear_loopback_root_qdisc
  return "$failed"
}

run_downlink_matrix() {
  require_browser_runtime
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

run_remote_harness() {
  require_browser_runtime
  local failed=0

  if ! run_cmd \
    "remote-harness:deploy" \
    --cwd "$ROOT_DIR" \
    "$ROOT_DIR/scripts/deploy_remote_test_machine.sh"; then
    return 1
  fi

  if ! run_cmd \
    "remote-harness:browser-smokes" \
    --cwd "$ROOT_DIR" \
    "$ROOT_DIR/scripts/run_remote_qos_smokes.sh"; then
    return 1
  fi

  if ! run_cmd \
    "remote-harness:pressure-smoke" \
    --cwd "$ROOT_DIR" \
    "$ROOT_DIR/scripts/run_remote_pressure_smoke.sh"; then
    failed=1
  fi

  return "$failed"
}

run_group() {
  local group="$1"
  case "$group" in
    client-js) run_client_js ;;
    cpp-unit) run_cpp_unit ;;
    cpp-integration) run_cpp_integration ;;
    cpp-accuracy) run_cpp_accuracy ;;
    node-harness) run_node_harness ;;
    browser-harness) run_browser_harness ;;
    downlink-matrix) run_downlink_matrix ;;
    remote-harness) run_remote_harness ;;
    *) fail "internal error: unsupported group '$group'" ;;
  esac
}

run_target() {
  local target="$1"
  case "$target" in
    client-js|cpp-unit|cpp-integration|cpp-accuracy|node-harness|browser-harness|downlink-matrix|remote-harness)
      run_group "$target"
      ;;
    node-harness:*)
      prepare_test_port 14011 "QoS node harness SFU port 14011"
      local scenario="${target#node-harness:}"
      if [[ "$scenario" == "netem-guard" ]]; then
        run_cmd \
          "$target" \
          --cwd "$ROOT_DIR" \
          node --test "$ROOT_DIR/tests/qos_harness/test.netem_guard.mjs"
      else
        run_cmd \
          "$target" \
          --cwd "$ROOT_DIR" \
          node "$ROOT_DIR/tests/qos_harness/run.mjs" "$scenario"
      fi
      ;;
    browser-harness:server-signal)
      require_browser_runtime
      prepare_test_port 14012 "QoS browser harness SFU port 14012"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_server_signal.mjs"
      ;;
    browser-harness:downlink-controls)
      require_browser_runtime
      prepare_test_port 14013 "Downlink control harness SFU port 14013"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_controls.mjs"
      ;;
    browser-harness:downlink-e2e)
      require_browser_runtime
      prepare_test_port 14014 "Downlink E2E harness SFU port 14014"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_e2e.mjs"
      ;;
    browser-harness:downlink-priority)
      require_browser_runtime
      prepare_test_port 14015 "Downlink priority harness SFU port 14015"
      clear_loopback_root_qdisc
      local rc=0
      if ! run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_priority.mjs"; then
        rc=1
      fi
      clear_loopback_root_qdisc
      return "$rc"
      ;;
    browser-harness:downlink-v2)
      require_browser_runtime
      prepare_test_port 14016 "Downlink v2 harness SFU port 14016"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_v2.mjs"
      ;;
    browser-harness:downlink-v3)
      require_browser_runtime
      prepare_test_port 14017 "Downlink v3 harness SFU port 14017"
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        node "$ROOT_DIR/tests/qos_harness/browser_downlink_v3.mjs"
      ;;
    remote-harness:browser-smokes)
      require_browser_runtime
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        "$ROOT_DIR/scripts/run_remote_qos_smokes.sh"
      ;;
    remote-harness:deploy)
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        "$ROOT_DIR/scripts/deploy_remote_test_machine.sh"
      ;;
    remote-harness:pressure-smoke)
      run_cmd \
        "$target" \
        --cwd "$ROOT_DIR" \
        "$ROOT_DIR/scripts/run_remote_pressure_smoke.sh"
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
  if [[ "$group" == "downlink-matrix" ]]; then
    GENERATE_DOWNLINK_CASE_REPORT=1
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

DOWNLINK_CASE_REPORT_SCRIPT="$ROOT_DIR/tests/qos_harness/render_downlink_case_report.mjs"
if ((GENERATE_DOWNLINK_CASE_REPORT)) && [[ -f "$DOWNLINK_CASE_REPORT_SCRIPT" ]]; then
  DL_CASE_REPORT_JSON="$ROOT_DIR/docs/generated/downlink-qos-matrix-report.json"
  DL_CASE_REPORT_OUTPUT="$ROOT_DIR/docs/downlink-qos-case-results.md"

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

if ((GENERATE_DOWNLINK_SUMMARY)); then
  echo
  echo "==> [downlink-report]"
  if write_downlink_report; then
    echo "<== [downlink-report] PASS ($DOWNLINK_SUMMARY_FILE)"
  else
    echo "<== [downlink-report] WARN (generation failed)" >&2
  fi
fi

echo
if ((${#FAILED_GROUPS[@]} == 0)); then
  echo "All selected QoS test groups passed."
else
  echo "Completed with failures in group(s): ${FAILED_GROUPS[*]}" >&2
  exit 1
fi
