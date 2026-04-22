#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
CLIENT_BUILD_DIR="$ROOT_DIR/client/build"
REPORT_FILE="$ROOT_DIR/docs/full-regression-test-results.md"
ROOT_README_FILE="$ROOT_DIR/README.md"
JOBS="${JOBS:-$(nproc)}"

ALL_GROUPS=(
  unit
  integration
  qos
  topology
  threaded
  worker
)
ALIAS_GROUPS=(
  non-qos
)

SELECTED_GROUPS=()
SKIP_BUILD=0
FAILED_GROUPS=()
TASK_ORDER=()
declare -A TASK_RESULTS=()
declare -A TASK_DURATIONS=()

relative_markdown_path() {
  local target_path="$1"
  python3 - "$REPORT_FILE" "$target_path" <<'PY'
import os, sys
report = os.path.abspath(sys.argv[1])
target = os.path.abspath(sys.argv[2])
print(os.path.relpath(target, os.path.dirname(report)))
PY
}

markdown_link_or_dash() {
  local path="$1"
  local label="$2"
  if [[ -f "$path" ]]; then
    local rel
    rel="$(relative_markdown_path "$path")"
    printf '[%s](%s)' "$label" "$rel"
  else
    printf '%s' '-'
  fi
}

file_mtime_or_dash() {
  local path="$1"
  if [[ -f "$path" ]]; then
    stat -c '%y' "$path" 2>/dev/null | cut -d'.' -f1
  else
    printf '%s' '-'
  fi
}

append_report_link_row() {
  local file="$1"
  local label="$2"
  local description="$3"
  printf '| %s | %s | %s | %s |\n' \
    "$label" \
    "$description" \
    "$(markdown_link_or_dash "$file" "$(basename "$file")")" \
    "$(file_mtime_or_dash "$file")"
}

append_latest_artifact_row() {
  local pattern="$1"
  local label="$2"
  local description="$3"
  local latest
  latest="$(find "$ROOT_DIR"/$pattern -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort | tail -1 || true)"
  if [[ -n "$latest" ]]; then
    local report_md="$latest/$(basename "$pattern").md"
    if [[ ! -f "$report_md" ]]; then
      report_md="$(find "$latest" -maxdepth 1 -type f -name '*.md' | sort | head -1 || true)"
    fi
    if [[ -n "$report_md" && -f "$report_md" ]]; then
      printf '| %s | %s | %s | %s |\n' \
        "$label" \
        "$description" \
        "$(markdown_link_or_dash "$report_md" "$(basename "$report_md")")" \
        "$(file_mtime_or_dash "$report_md")"
      return
    fi
  fi
  printf '| %s | %s | - | - |\n' "$label" "$description"
}

duration_seconds() {
  local duration="$1"
  if [[ "$duration" =~ ^([0-9]+)s$ ]]; then
    printf '%s' "${BASH_REMATCH[1]}"
  else
    printf '0'
  fi
}

duration_bar() {
  local seconds="$1"
  local max_seconds="$2"
  if (( max_seconds <= 0 || seconds <= 0 )); then
    printf '%s' '-'
    return
  fi
  local width=$(( seconds * 20 / max_seconds ))
  if (( width < 1 )); then
    width=1
  fi
  printf '%*s' "$width" '' | tr ' ' '#'
}

usage() {
  cat <<'EOF'
Usage:
  scripts/run_all_tests.sh
  scripts/run_all_tests.sh unit threaded
  scripts/run_all_tests.sh non-qos
  scripts/run_all_tests.sh --list

Options:
  --skip-build   assume binaries are already up to date
  --list         list available groups
  -h, --help     show help

Groups:
  unit         core unit suites inside mediasoup_tests
  integration  integration / e2e / stability / review-fix binaries
  qos          full QoS regression delegated to scripts/run_qos_tests.sh
  topology     topology + multinode binaries
  threaded     compatibility alias for threaded QoS regression only
  worker       vendored mediasoup-worker Catch2 regression suite
  non-qos      alias for unit + integration + topology + worker

Notes:
  - This script is the full repository regression entry.
  - Selected groups keep running after a test failure; the script exits non-zero at the end if any group failed.
  - It covers the native groups here and delegates the full QoS regression surface to scripts/run_qos_tests.sh.
  - If qos is selected, threaded is skipped automatically because qos already includes the threaded QoS slice.
  - threaded tests require client/build/plain-client.
  - mediasoup_review_fix_tests, mediasoup_multinode_tests, and mediasoup_topology_tests
    start an isolated Redis and require redis-server in PATH.
EOF
}

list_groups() {
  printf '%s\n' "${ALL_GROUPS[@]}"
  printf '%s\n' "${ALIAS_GROUPS[@]}"
}

normalize_selected_groups() {
  local group
  local has_qos=0
  local -a normalized=()

  for group in "${SELECTED_GROUPS[@]}"; do
    if [[ "$group" == "non-qos" ]]; then
      for expanded_group in unit integration topology worker; do
        case " ${normalized[*]} " in
          *" $expanded_group "*) continue ;;
        esac
        normalized+=("$expanded_group")
      done
      continue
    fi

    case " ${ALL_GROUPS[*]} " in
      *" $group "*) ;;
      *)
        echo "error: unknown group '$group'" >&2
        return 1
        ;;
    esac

    case " ${normalized[*]} " in
      *" $group "*) continue ;;
    esac

    normalized+=("$group")
    if [[ "$group" == "qos" ]]; then
      has_qos=1
    fi
  done

  if ((has_qos)); then
    local -a filtered=()
    for group in "${normalized[@]}"; do
      [[ "$group" == "threaded" ]] && continue
      filtered+=("$group")
    done
    normalized=("${filtered[@]}")
  fi

  SELECTED_GROUPS=("${normalized[@]}")
}

record_task_result() {
  local label="$1"
  local status="$2"
  local duration="${3:--}"
  TASK_ORDER+=("$label")
  TASK_RESULTS["$label"]="$status"
  TASK_DURATIONS["$label"]="$duration"
}

join_markdown_codes() {
  local result=""
  local item

  for item in "$@"; do
    if [[ -n "$result" ]]; then
      result+=", "
    fi
    result+="\`$item\`"
  done

  printf '%s' "${result:-none}"
}

task_group() {
  local label="$1"
  if [[ "$label" == *:* ]]; then
    printf '%s' "${label%%:*}"
  else
    printf '%s' "$label"
  fi
}

write_report() {
  local generated_at overall_status
  local attempted=0
  local passed=0
  local failed=0
  local label status duration group duration_seconds_value max_duration_seconds=0
  local -a failed_tasks=()

  generated_at="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  overall_status="PASS"
  if ((${#FAILED_GROUPS[@]} > 0)); then
    overall_status="FAIL"
  fi

  for label in "${TASK_ORDER[@]}"; do
    status="${TASK_RESULTS[$label]:-NOT_RUN}"
    duration="${TASK_DURATIONS[$label]:--}"
    duration_seconds_value="$(duration_seconds "$duration")"
    if (( duration_seconds_value > max_duration_seconds )); then
      max_duration_seconds=$duration_seconds_value
    fi
    case "$status" in
      PASS)
        ((attempted += 1))
        ((passed += 1))
        ;;
      FAIL)
        ((attempted += 1))
        ((failed += 1))
        failed_tasks+=("$label")
        ;;
    esac
  done

  mkdir -p "$(dirname "$REPORT_FILE")"
  {
    echo "# Full Regression Test Results"
    echo
    echo "Generated at: \`$generated_at\`"
    echo
    echo "## Summary"
    echo
    echo "- Script: \`scripts/run_all_tests.sh\`"
    echo "- Selected groups: $(join_markdown_codes "${SELECTED_GROUPS[@]}")"
    echo "- Overall status: \`$overall_status\`"
    echo "- Attempted tasks: \`$attempted\`"
    echo "- Passed tasks: \`$passed\`"
    echo "- Failed tasks: \`$failed\`"
    echo "- Failed groups: $(join_markdown_codes "${FAILED_GROUPS[@]}")"
    echo
    echo "## Failed Task Summary"
    echo
    if ((${#failed_tasks[@]} == 0)); then
      echo "- none"
    else
      echo "| Task | Group | Duration |"
      echo "|---|---|---|"
      for label in "${failed_tasks[@]}"; do
        group="$(task_group "$label")"
        duration="${TASK_DURATIONS[$label]:--}"
        echo "| \`$label\` | \`$group\` | \`$duration\` |"
      done
    fi
    echo
    echo "## Task Results"
    echo
    if ((${#TASK_ORDER[@]} == 0)); then
      echo "- No tasks were executed."
    else
      echo "| Task | Group | Status | Duration |"
      echo "|---|---|---|---|"
      for label in "${TASK_ORDER[@]}"; do
        group="$(task_group "$label")"
        status="${TASK_RESULTS[$label]:-NOT_RUN}"
        duration="${TASK_DURATIONS[$label]:--}"
        echo "| \`$label\` | \`$group\` | \`$status\` | \`$duration\` |"
      done
    fi
    echo
    echo "## Task Duration View"
    echo
    if ((${#TASK_ORDER[@]} == 0)); then
      echo "- No duration data."
    else
      echo "| Task | Duration | Visual |"
      echo "|---|---:|---|"
      for label in "${TASK_ORDER[@]}"; do
        duration="${TASK_DURATIONS[$label]:--}"
        duration_seconds_value="$(duration_seconds "$duration")"
        echo "| \`$label\` | \`$duration\` | $(duration_bar "$duration_seconds_value" "$max_duration_seconds") |"
      done
    fi
    echo
    echo "## Detailed Reports"
    echo
    echo "| Report | Scope | Link | Updated |"
    echo "|---|---|---|---|"
    append_report_link_row "$ROOT_DIR/docs/qos/uplink/test-results-summary.md" "Uplink Summary" "Uplink QoS summary"
    append_report_link_row "$ROOT_DIR/docs/uplink-qos-case-results.md" "Uplink Cases" "Browser uplink per-case report"
    append_report_link_row "$ROOT_DIR/docs/plain-client-qos-case-results.md" "Plain Client Cases" "PlainTransport C++ client per-case report"
    append_report_link_row "$ROOT_DIR/docs/downlink-qos-test-results-summary.md" "Downlink Summary" "Downlink QoS summary"
    append_report_link_row "$ROOT_DIR/docs/downlink-qos-case-results.md" "Downlink Cases" "Downlink per-case report"
    append_report_link_row "$ROOT_DIR/docs/generated/uplink-qos-matrix-report.json" "Uplink Matrix JSON" "Latest browser uplink matrix artifact"
    append_report_link_row "$ROOT_DIR/docs/generated/uplink-qos-cpp-client-matrix-report.json" "Plain Client Matrix JSON" "Latest C++ client matrix artifact"
    append_report_link_row "$ROOT_DIR/docs/generated/downlink-qos-matrix-report.json" "Downlink Matrix JSON" "Latest downlink matrix artifact"
    append_report_link_row "$ROOT_DIR/docs/generated/twcc-ab-report.md" "TWCC A/B Summary" "Latest TWCC A/B effectiveness summary"
    append_latest_artifact_row "changes/2026-04-21-livekit-aligned-send-side-bwe/artifacts/full43-compare" "LiveKit 43-Case Compare" "Latest livekit-aligned 43-case comparison"
    append_latest_artifact_row "changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval" "TWCC A/B Eval Artifact" "Latest timestamped TWCC A/B artifact bundle"
  } > "$REPORT_FILE"
}

refresh_root_readme_qos_status() {
  python3 - "$ROOT_DIR" "$ROOT_README_FILE" <<'PY'
import json
import os
import re
import sys

root_dir = os.path.abspath(sys.argv[1])
readme_path = os.path.abspath(sys.argv[2])

if not os.path.isfile(readme_path):
    sys.exit(0)

with open(readme_path, "r", encoding="utf-8") as handle:
    content = handle.read()

marker_re = re.compile(
    r"(<!-- BEGIN AUTO QOS STATUS -->\n)(.*?)(<!-- END AUTO QOS STATUS -->)",
    re.DOTALL,
)

if not marker_re.search(content):
    sys.exit(0)


def load_matrix_summary(rel_path):
    json_path = os.path.join(root_dir, rel_path)
    if not os.path.isfile(json_path):
        return None
    with open(json_path, "r", encoding="utf-8") as handle:
        payload = json.load(handle)
    summary = payload.get("summary") or {}
    total = summary.get("total")
    passed = summary.get("passed")
    failed = summary.get("failed") or 0
    errors = summary.get("errors") or 0
    generated_at = payload.get("generatedAt") or ""
    date_label = generated_at[:10] if len(generated_at) >= 10 else "-"
    if total is None or passed is None:
        return None
    status_parts = [f"{passed} / {total} PASS"]
    if failed:
        status_parts.append(f"{failed} FAIL")
    if errors:
        status_parts.append(f"{errors} ERROR")
    return f"`{', '.join(status_parts)}` (`{date_label}`)"


browser_status = load_matrix_summary("docs/generated/uplink-qos-matrix-report.json")
plain_client_status = load_matrix_summary("docs/generated/uplink-qos-cpp-client-matrix-report.json")

if not browser_status or not plain_client_status:
    sys.exit(0)

replacement = (
    "<!-- BEGIN AUTO QOS STATUS -->\n"
    f"- browser uplink matrix main gate: {browser_status}\n"
    f"- PlainTransport C++ client matrix: {plain_client_status}\n"
    "<!-- END AUTO QOS STATUS -->"
)

updated = marker_re.sub(replacement, content, count=1)
if updated != content:
    with open(readme_path, "w", encoding="utf-8") as handle:
        handle.write(updated)
PY
}

require_file() {
  local path="$1"
  local label="${2:-}"
  [[ -e "$path" ]] || {
    echo "error: required file not found: $path" >&2
    if [[ -n "$label" ]]; then
      record_task_result "$label" "FAIL" "-"
    fi
    return 1
  }
}

require_command() {
  local cmd="$1"
  local label="${2:-}"
  command -v "$cmd" >/dev/null 2>&1 || {
    echo "error: required command not found: $cmd" >&2
    if [[ -n "$label" ]]; then
      record_task_result "$label" "FAIL" "-"
    fi
    return 1
  }
}

log_system_snapshot() {
  local label="$1"
  echo
  echo "==> system:$label"
  echo "timestamp_utc=$(date -u '+%Y-%m-%dT%H:%M:%SZ') jobs=$JOBS"
  awk '
    /MemTotal:|MemAvailable:|SwapTotal:|SwapFree:/ {
      printf "%s=%s%s ", $1, $2, $3
    }
    END { print "" }
  ' /proc/meminfo 2>/dev/null || true
  if command -v ps >/dev/null 2>&1; then
    echo "top_rss_processes:"
    ps -eo pid,ppid,rss,stat,comm,args --sort=-rss | head -n 12 || true
    echo "browser_and_build_processes:"
    ps -eo pid,ppid,rss,stat,comm,args | awk '
      NR == 1 || /headless_shell|esbuild|cc1plus|mediasoup-sfu|run_matrix\.mjs|run_qos_tests\.sh|cmake|gmake/
    ' || true
  fi
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

append_unique_target() {
  local target="$1"
  local -n target_list_ref="$2"
  local existing

  for existing in "${target_list_ref[@]}"; do
    [[ "$existing" == "$target" ]] && return 0
  done

  target_list_ref+=("$target")
}

build_targets() {
  local -a build_targets=()

  for group in "${SELECTED_GROUPS[@]}"; do
    case "$group" in
      unit)
        append_unique_target mediasoup_tests build_targets
        ;;
      integration)
        append_unique_target mediasoup-sfu build_targets
        append_unique_target mediasoup_integration_tests build_targets
        append_unique_target mediasoup_e2e_tests build_targets
        append_unique_target mediasoup_stability_integration_tests build_targets
        append_unique_target mediasoup_review_fix_tests build_targets
        ;;
      topology)
        append_unique_target mediasoup-sfu build_targets
        append_unique_target mediasoup_topology_tests build_targets
        append_unique_target mediasoup_multinode_tests build_targets
        ;;
      qos|threaded|worker)
        ;;
      *)
        echo "error: unsupported group in build_targets(): $group" >&2
        return 1
        ;;
    esac
  done

  if ((${#build_targets[@]} == 0)); then
    echo "==> no native prebuild targets required for selected groups: ${SELECTED_GROUPS[*]}"
    return 0
  fi

  log_system_snapshot "pre-build"
  echo "==> building full regression test targets"
  echo "==> selected native build targets: ${build_targets[*]}"
  cmake --build "$BUILD_DIR" -j"$JOBS" --target "${build_targets[@]}"
  log_system_snapshot "post-build"
}

pick_worker_python() {
  local candidates=()

  if [[ -n "${MEDIASOUP_WORKER_PYTHON:-}" ]]; then
    candidates+=("${MEDIASOUP_WORKER_PYTHON}")
  fi

  candidates+=(python3.12 python3.11 python3.10 python3.9 python3.8 python3)

  local candidate candidate_path version major minor
  for candidate in "${candidates[@]}"; do
    candidate_path=""
    if [[ -x "$candidate" ]]; then
      candidate_path="$candidate"
    elif command -v "$candidate" >/dev/null 2>&1; then
      candidate_path="$(command -v "$candidate")"
    else
      continue
    fi

    version="$("$candidate_path" -c 'import sys; print(f"{sys.version_info[0]} {sys.version_info[1]}")')" || continue
    read -r major minor <<< "$version"

    if (( major > 3 || (major == 3 && minor >= 8) )); then
      printf '%s\n' "$candidate_path"
      return 0
    fi
  done

  return 1
}

install_worker_invoke() {
  local worker_python="$1"
  local invoke_target_dir="$2"
  local pythonpath="${invoke_target_dir}${PYTHONPATH:+:${PYTHONPATH}}"

  if PYTHONPATH="$pythonpath" "$worker_python" -c 'import invoke' >/dev/null 2>&1; then
    return 0
  fi

  mkdir -p "$invoke_target_dir"

  if ! "$worker_python" -m pip install --upgrade --no-user --target "$invoke_target_dir" invoke -i https://mirrors.aliyun.com/pypi/simple; then
    echo "warning: Aliyun PyPI mirror missing invoke or failed; falling back to upstream PyPI" >&2
    "$worker_python" -m pip install --upgrade --no-user --target "$invoke_target_dir" invoke -i https://pypi.org/simple
  fi
}

run_cmd() {
  local label="$1"
  shift
  local rc
  local elapsed
  local start_time
  echo
  echo "==> $label"
  start_time=$SECONDS
  set +e
  "$@"
  rc=$?
  set -e
  elapsed="$((SECONDS - start_time))s"
  if ((rc == 0)); then
    echo "<== $label PASS"
    record_task_result "$label" "PASS" "$elapsed"
  else
    echo "<== $label FAIL (rc=$rc)" >&2
    record_task_result "$label" "FAIL" "$elapsed"
    log_system_snapshot "after-fail:$label"
  fi
  return "$rc"
}

run_unit() {
  require_file "$BUILD_DIR/mediasoup_tests" "unit:preflight:mediasoup_tests" || return 1
  run_cmd \
    "unit" \
    "$BUILD_DIR/mediasoup_tests"
}

run_integration() {
  local failed=0
  cleanup_test_ports
  require_file "$BUILD_DIR/mediasoup_integration_tests" "integration:preflight:mediasoup_integration_tests" || return 1
  require_file "$BUILD_DIR/mediasoup_e2e_tests" "integration:preflight:mediasoup_e2e_tests" || return 1
  require_file "$BUILD_DIR/mediasoup_stability_integration_tests" "integration:preflight:mediasoup_stability_integration_tests" || return 1
  require_file "$BUILD_DIR/mediasoup_review_fix_tests" "integration:preflight:mediasoup_review_fix_tests" || return 1
  if ! run_cmd "integration:mediasoup_integration_tests" "$BUILD_DIR/mediasoup_integration_tests"; then
    failed=1
  fi
  cleanup_test_ports
  if ! run_cmd "integration:mediasoup_e2e_tests" "$BUILD_DIR/mediasoup_e2e_tests"; then
    failed=1
  fi
  cleanup_test_ports
  if ! run_cmd "integration:mediasoup_stability_integration_tests" "$BUILD_DIR/mediasoup_stability_integration_tests"; then
    failed=1
  fi
  cleanup_test_ports
  require_command redis-server "integration:preflight:redis-server" || return 1
  if ! run_cmd "integration:mediasoup_review_fix_tests" "$BUILD_DIR/mediasoup_review_fix_tests"; then
    failed=1
  fi
  return "$failed"
}

run_qos() {
  require_file "$ROOT_DIR/scripts/run_qos_tests.sh" "qos:preflight:run_qos_tests.sh" || return 1
  cleanup_test_ports
  log_system_snapshot "pre-qos"
  run_cmd \
    "qos:qos-regression" \
    "$ROOT_DIR/scripts/run_qos_tests.sh" all
}

run_topology() {
  local failed=0
  cleanup_test_ports
  require_file "$BUILD_DIR/mediasoup_topology_tests" "topology:preflight:mediasoup_topology_tests" || return 1
  require_file "$BUILD_DIR/mediasoup_multinode_tests" "topology:preflight:mediasoup_multinode_tests" || return 1
  require_command redis-server "topology:preflight:redis-server" || return 1
  if ! run_cmd "topology:mediasoup_topology_tests" "$BUILD_DIR/mediasoup_topology_tests"; then
    failed=1
  fi
  cleanup_test_ports
  if ! run_cmd "topology:mediasoup_multinode_tests" "$BUILD_DIR/mediasoup_multinode_tests"; then
    failed=1
  fi
  return "$failed"
}

run_threaded() {
  require_file "$ROOT_DIR/scripts/run_qos_tests.sh" "threaded:preflight:run_qos_tests.sh" || return 1
  cleanup_test_ports
  run_cmd \
    "threaded:qos-threaded-regression" \
    "$ROOT_DIR/scripts/run_qos_tests.sh" cpp-threaded
}

run_worker() {
  local worker_dir="$ROOT_DIR/src/mediasoup-worker-src/worker"
  local invoke_dir="$worker_dir/out/pip_run_all_invoke"
  local worker_python
  local pythonpath
  local worker_test_tags='[parser][rtcp][rr],[rtp][rtcp][nack]'

  require_file "$worker_dir/meson.build" "worker:preflight:meson.build" || return 1

  if ! worker_python="$(pick_worker_python)"; then
    echo "error: no Python 3.8+ interpreter found for vendored worker tests" >&2
    record_task_result "worker:preflight:python" "FAIL" "-"
    return 1
  fi

  if ! "$worker_python" -m pip --version >/dev/null 2>&1; then
    echo "error: pip is not available for $worker_python" >&2
    record_task_result "worker:preflight:pip" "FAIL" "-"
    return 1
  fi

  install_worker_invoke "$worker_python" "$invoke_dir" || {
    record_task_result "worker:preflight:invoke" "FAIL" "-"
    return 1
  }

  pythonpath="${invoke_dir}${PYTHONPATH:+:${PYTHONPATH}}"

  run_cmd \
    "worker:mediasoup-worker-test" \
    env \
      MEDIASOUP_TEST_TAGS="$worker_test_tags" \
      PYTHONPATH="$pythonpath" \
      PYTHON="$worker_python" \
      taskset -c 0 \
      "$worker_python" -m invoke -r "$worker_dir" test
}

run_group() {
  local group="$1"
  case "$group" in
    unit) run_unit ;;
    integration) run_integration ;;
    qos) run_qos ;;
    topology) run_topology ;;
    threaded) run_threaded ;;
    worker) run_worker ;;
    *) echo "error: unknown group '$group'" >&2; return 1 ;;
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

  normalize_selected_groups

  if ((SKIP_BUILD == 0)); then
    build_targets
  fi

  for group in "${SELECTED_GROUPS[@]}"; do
    if ! run_group "$group"; then
      FAILED_GROUPS+=("$group")
    fi
  done

  write_report
  refresh_root_readme_qos_status

  echo
  if ((${#FAILED_GROUPS[@]} == 0)); then
    echo "full regression test run passed"
  else
    echo "full regression test run completed with failures in group(s): ${FAILED_GROUPS[*]}" >&2
    exit 1
  fi
}

main "$@"
