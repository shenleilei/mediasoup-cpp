#!/usr/bin/env bash
set -u
set -o pipefail

REPO_ROOT="/root/mediasoup-cpp"
DOC_FILE="$REPO_ROOT/docs/uplink-qos-test-plan.md"
ARTIFACTS_ROOT="$REPO_ROOT/tests/qos_harness/artifacts"
RUN_ID="$(date '+%Y%m%d-%H%M%S')"
RUN_DIR="$ARTIFACTS_ROOT/uplink-qos-test-plan-$RUN_ID"
LATEST_LINK="$ARTIFACTS_ROOT/latest-uplink-qos-test-plan"
SESSION_LOG="$RUN_DIR/session.log"
RESULTS_TSV="$RUN_DIR/results.tsv"
REPORT_TMP="$RUN_DIR/generated-report.md"
REPORT_BEGIN='<!-- BEGIN GENERATED UPLINK QOS TEST REPORT -->'
REPORT_END='<!-- END GENERATED UPLINK QOS TEST REPORT -->'
TZ_NAME="${TZ:-Asia/Shanghai}"
RUN_STARTED_AT="$(date '+%Y-%m-%d %H:%M:%S %Z')"
RUN_STARTED_AT_UTC="$(date -u '+%Y-%m-%d %H:%M:%S UTC')"

mkdir -p "$RUN_DIR"
ln -sfn "$RUN_DIR" "$LATEST_LINK"
: > "$SESSION_LOG"
: > "$RESULTS_TSV"

PASS_COUNT=0
FAIL_COUNT=0
EXPECTED_STEP_COUNT=9
CURRENT_STEP_ID=""
CURRENT_STEP_DESC=""

log() {
  printf '[%s] %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*" | tee -a "$SESSION_LOG"
}

record_result() {
  local step_id="$1"
  local status="$2"
  local duration_s="$3"
  local doc_map="$4"
  local description="$5"
  local logfile="$6"
  printf '%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$step_id" "$status" "$duration_s" "$doc_map" "$description" "$logfile" >> "$RESULTS_TSV"
}

run_step() {
  local step_id="$1"
  local doc_map="$2"
  local description="$3"
  shift 3

  local logfile="$RUN_DIR/${step_id}.log"
  local start_s end_s duration_s rc status cmd_string
  printf -v cmd_string '%q ' "$@"
  CURRENT_STEP_ID="$step_id"
  CURRENT_STEP_DESC="$description"

  log "START $step_id :: $description"
  log "MAP   $doc_map"
  log "CMD   $cmd_string"

  start_s="$(date +%s)"
  "$@" >"$logfile" 2>&1
  rc=$?
  end_s="$(date +%s)"
  duration_s=$((end_s - start_s))

  if [[ $rc -eq 0 ]]; then
    status="PASS"
    PASS_COUNT=$((PASS_COUNT + 1))
  else
    status="FAIL($rc)"
    FAIL_COUNT=$((FAIL_COUNT + 1))
  fi

  record_result "$step_id" "$status" "$duration_s" "$doc_map" "$description" "$logfile"
  log "END   $step_id :: $status :: ${duration_s}s :: $logfile"
  CURRENT_STEP_ID=""
  CURRENT_STEP_DESC=""

  return 0
}

loopback_summary() {
  local logfile="$RUN_DIR/browser_loopback.log"
  if [[ ! -s "$logfile" ]]; then
    return
  fi

  node - "$logfile" <<'NODE' || true
const fs = require('fs');
const logPath = process.argv[2];
const text = fs.readFileSync(logPath, 'utf8');
const start = text.indexOf('{');
const end = text.lastIndexOf('}');
if (start === -1 || end === -1 || end <= start) {
  process.exit(1);
}
const data = JSON.parse(text.slice(start, end + 1));
const fmt = value => value ? `\`${value.state} / level ${value.level}\`` : '`n/a`';
const actions = (data.impaired?.trace || [])
  .map(entry => entry?.plannedAction?.type)
  .filter(type => type && type !== 'noop');

console.log(`- loopback baseline: ${fmt(data.baseline)}`);
console.log(`- loopback impaired: ${fmt(data.impaired)}`);
console.log(`- loopback recovered: ${fmt(data.recovered)}`);
console.log(`- impaired trace actions: \`${actions.length ? actions.join(', ') : 'none'}\``);
NODE
}

write_report() {
  local script_rc="$1"
  local finished_at finished_at_utc overall_status
  local recorded_steps expected_steps incomplete_steps

  finished_at="$(date '+%Y-%m-%d %H:%M:%S %Z')"
  finished_at_utc="$(date -u '+%Y-%m-%d %H:%M:%S UTC')"
  recorded_steps="$(awk 'END { print NR + 0 }' "$RESULTS_TSV" 2>/dev/null)"
  expected_steps="$EXPECTED_STEP_COUNT"
  incomplete_steps=$((expected_steps - recorded_steps))
  if (( incomplete_steps < 0 )); then
    incomplete_steps=0
  fi

  if (( recorded_steps < expected_steps )); then
    overall_status="INCOMPLETE"
  elif [[ $FAIL_COUNT -eq 0 && $script_rc -eq 0 ]]; then
    overall_status="PASS"
  else
    overall_status="FAIL"
  fi

  {
    printf '\n%s\n' "$REPORT_BEGIN"
    printf '\n## 16. 测试执行报告（自动追加）\n\n'
    printf '- 执行时间（%s）：`%s`\n' "$TZ_NAME" "$RUN_STARTED_AT"
    printf '- 执行时间（UTC）：`%s`\n' "$RUN_STARTED_AT_UTC"
    printf '- 完成时间（%s）：`%s`\n' "$TZ_NAME" "$finished_at"
    printf '- 完成时间（UTC）：`%s`\n' "$finished_at_utc"
    printf '- 后台运行方式：`tmux` detached session\n'
    if [[ "$overall_status" == "INCOMPLETE" ]]; then
      printf '- 结果汇总：`%s`，已记录 `%s/%s` 个 step，通过 `%s` 项，失败 `%s` 项，未完成 `%s` 项\n' \
        "$overall_status" "$recorded_steps" "$expected_steps" "$PASS_COUNT" "$FAIL_COUNT" "$incomplete_steps"
    else
      printf '- 结果汇总：`%s`，通过 `%s` 项，失败 `%s` 项\n' "$overall_status" "$PASS_COUNT" "$FAIL_COUNT"
    fi
    printf '- 原始日志目录：`%s`\n' "$RUN_DIR"
    printf '- 最新日志软链：`%s`\n\n' "$LATEST_LINK"

    printf '### 16.1 实际执行项\n\n'
    printf '| Step | 状态 | 耗时 | 文档映射 | 说明 | 日志 |\n'
    printf '|---|---|---:|---|---|---|\n'
    while IFS=$'\t' read -r step_id status duration_s doc_map description logfile; do
      [[ -n "$step_id" ]] || continue
      printf '| `%s` | `%s` | `%ss` | %s | %s | `%s` |\n' \
        "$step_id" "$status" "$duration_s" "$doc_map" "$description" "$logfile"
    done < "$RESULTS_TSV"

    printf '\n### 16.2 关键观察\n\n'
    if (( recorded_steps < expected_steps )); then
      printf '- 本次后台执行未完整跑完计划中的全部自动化 step；当前 artifact 只能证明上表里已经落盘的条目。\n'
      if [[ -n "$CURRENT_STEP_ID" ]]; then
        printf -- '- 中断时最后一个已开始但未完成记录的 step：`%s`（%s）。\n' "$CURRENT_STEP_ID" "$CURRENT_STEP_DESC"
      fi
    fi
    printf '- 本次后台执行的是仓库当前已有自动化入口能够直接落地的上行 QoS 子集：gtest、Node harness、browser loopback、browser -> real SFU signaling。\n'
    printf '- 这些执行项覆盖了文档里最关键的策略链路：`clientStats` 聚合、`qosPolicy` 下发/热更新、`qosOverride` 下发、automatic poor/lost/clear、`qosConnectionQuality`、`qosRoomState`、room pressure。\n'
    if [[ -s "$RUN_DIR/browser_loopback.log" ]]; then
      loopback_summary
    else
      printf '%s\n' '- `browser_loopback` 未产出可解析 JSON，无法提取 baseline/impaired/recovered 摘要。'
    fi

    printf '\n### 16.3 与测试计划 Case 的对应关系\n\n'
    printf '| 文档 Case | 执行结论 | 证据 |\n'
    printf '|---|---|---|\n'
    printf '| `P2` | 部分直接覆盖 | `QosIntegrationTest.SetQosPolicyNotifiesTargetPeer` 直接覆盖服务端通知链路；若本次有 `policy_update` step 日志，才可进一步认定 client runtime 热更新已执行 |\n'
    printf '| `P3` | 部分直接覆盖 | `QosIntegrationTest.SetQosOverrideNotifiesTargetPeer` 直接覆盖服务端 override 下发；client 端是否实际应用仍依赖 `override_force_audio_only` harness |\n'
    printf '| `P4` | 依赖单独 harness | 需要 `run.mjs override_force_audio_only` 或等价 browser harness 才能证明 `forceAudioOnly -> enterAudioOnly` 已直接执行 |\n'
    printf '| `P5` | 部分覆盖 | `AutomaticQosOverrideClearsWhenQualityRecovers` 覆盖 automatic clear；manual clear 仍需单独 harness/日志 |\n'
    printf '| `RM2` | 已直接执行 | `QosIntegrationTest.RoomQosStateAndRoomPressureOverride` |\n'
    printf '| `RM3` | 本次无直接覆盖 | 当前 gtest 直接覆盖的是 `poorPeers=2 -> room pressure`；`lostPeers>=1` 仍需单独 room-lost case |\n'
    printf '| `RM4` | 依赖单独 harness | 需要 `browser_server_signal.mjs` 或等价 clear case 日志；当前 gtest 不直接证明 room pressure clear |\n'
    printf '| `L5` / `L8` | 相关覆盖 | `AutomaticQosOverrideOnPoorQuality`、`AutomaticQosOverrideOnLostQuality` 基于合成 `clientStats` 验证 severe poor/lost 下自动保护，但不是文档表格的一对一参数扫描 |\n'
    printf '| `R4` / `J3` / `T5` | 相关覆盖 | 只有在 `browser_loopback.mjs` / `browser_server_signal.mjs` 产出完成日志时，才能说明固定弱网场景下退化/恢复链路可工作；即使通过也不是矩阵一对一执行 |\n'

    printf '\n### 16.4 当前未一对一执行的 Case\n\n'
    printf '- `B1-B3`、`BW1-BW7`、`L1-L8`、`R1-R6`、`J1-J5`、`T1-T8`、`S1-S4` 里的大多数扫描/转场 case，当前仓库还没有与文档矩阵一一对应的参数化自动化 runner。\n'
    printf '- 现有 `browser_loopback.mjs` 只覆盖一个固定弱网剖面，现有 `browser_server_signal.mjs` 只覆盖 policy/override/auto-clear 的固定联动场景，因此不能把这些命令直接等同于整张测试矩阵已经全部跑完。\n'
    printf '- 如果后续要把文档矩阵完全落地，下一步需要补一个可参数化的 browser/netem runner，至少支持按 Case 注入带宽、RTT、loss、jitter 以及 baseline/impairment/recovery 时长。\n'

    printf '\n### 16.5 结论\n\n'
    if (( recorded_steps < expected_steps )); then
      printf '- 当前 artifact 不是一次完整的 uplink QoS 自动化重跑结果；它只能证明上表里已经记录完成的 step。\n'
      printf '- 若要宣称“现有自动化入口全部通过”或“整份测试计划已执行完成”，需要先补齐剩余 Node/browser harness 的完成日志，尤其是 `browser_loopback` 的 JSON 摘要和弱网时序数据。\n'
    elif [[ $FAIL_COUNT -eq 0 ]]; then
      printf '- 当前已有自动化入口全部通过，说明上行 QoS 的基础协议、服务端聚合、policy/override 下发、room pressure 以及 browser 侧弱网退化/恢复链路都可工作。\n'
      printf '- 但这不等于文档中的完整 case matrix 已全部验证；本次结果更准确的表述是：“已把仓库现有可执行的 uplink QoS 自动化子集在后台完整重跑，并把结果回填到计划文档”。\n'
    else
      printf '- 本次后台执行存在失败项，需先看上面的 Step 表和原始日志定位后，再判断是否能够认定当前 QoS 链路稳定。\n'
      printf '- 即使失败项定位完成，文档矩阵中的大多数扫描 case 仍需要补参数化 harness 才能宣称“整份测试计划已执行完成”。\n'
    fi

    printf '\n%s\n' "$REPORT_END"
  } > "$REPORT_TMP"

  perl -0pi -e 's/\n<!-- BEGIN GENERATED UPLINK QOS TEST REPORT -->.*?<!-- END GENERATED UPLINK QOS TEST REPORT -->\n?//sg' "$DOC_FILE"
  cat "$REPORT_TMP" >> "$DOC_FILE"
}

finish() {
  local script_rc="$1"
  local recorded_steps
  recorded_steps="$(awk 'END { print NR + 0 }' "$RESULTS_TSV" 2>/dev/null)"
  write_report "$script_rc"
  if (( recorded_steps < EXPECTED_STEP_COUNT )); then
    log "RUN COMPLETE :: INCOMPLETE :: pass=$PASS_COUNT fail=$FAIL_COUNT recorded=$recorded_steps/$EXPECTED_STEP_COUNT"
  elif [[ $FAIL_COUNT -ne 0 || $script_rc -ne 0 ]]; then
    log "RUN COMPLETE :: FAIL :: pass=$PASS_COUNT fail=$FAIL_COUNT"
  else
    log "RUN COMPLETE :: PASS :: pass=$PASS_COUNT fail=$FAIL_COUNT"
  fi
}

trap 'rc=$?; finish "$rc"; exit "$rc"' EXIT

cd "$REPO_ROOT" || exit 1

log "Artifacts: $RUN_DIR"
log "Doc file:   $DOC_FILE"

run_step \
  "qos_unit" \
  "基础能力支撑（协议/校验/聚合/override builder）" \
  "服务端 QoS 单元测试" \
  ./build/mediasoup_tests \
  "--gtest_filter=QosProtocolTest.*:QosValidatorTest.*:QosRegistryTest.*:QosAggregatorTest.*:QosRoomAggregatorTest.*:QosOverrideBuilderTest.*"

run_step \
  "qos_integration" \
  "P2 / P3 / P5 / RM2-RM4 + clientStats 聚合链路" \
  "服务端 uplink QoS 黑盒集成测试子集" \
  ./build/mediasoup_qos_integration_tests \
  "--gtest_filter=QosIntegrationTest.ClientStatsQosStoredAndAggregated:QosIntegrationTest.ClientStatsQosInBroadcast:QosIntegrationTest.InvalidClientStatsRejected:QosIntegrationTest.OlderClientStatsSeqIsIgnored:QosIntegrationTest.JoinReceivesQosPolicyNotification:QosIntegrationTest.SetQosOverrideNotifiesTargetPeer:QosIntegrationTest.SetQosPolicyNotifiesTargetPeer:QosIntegrationTest.AutomaticQosOverrideOnPoorQuality:QosIntegrationTest.AutomaticQosOverrideOnLostQuality:QosIntegrationTest.AutomaticQosOverrideClearsWhenQualityRecovers:QosIntegrationTest.ConnectionQualityNotificationDelivered:QosIntegrationTest.RoomQosStateAndRoomPressureOverride"

run_step \
  "publish_snapshot" \
  "clientStats 聚合链路" \
  "Node harness: publish snapshot + aggregation" \
  node tests/qos_harness/run.mjs publish_snapshot

run_step \
  "stale_seq" \
  "旧 seq 不应覆盖新快照" \
  "Node harness: stale seq ignored" \
  node tests/qos_harness/run.mjs stale_seq

run_step \
  "policy_update" \
  "P2" \
  "Node harness: qosPolicy hot update" \
  node tests/qos_harness/run.mjs policy_update

run_step \
  "auto_override_poor" \
  "L5 / L8 相关覆盖" \
  "Node harness: automatic poor override" \
  node tests/qos_harness/run.mjs auto_override_poor

run_step \
  "override_force_audio_only" \
  "P3 / P4" \
  "Node harness: manual override force audio-only" \
  node tests/qos_harness/run.mjs override_force_audio_only

run_step \
  "browser_server_signal" \
  "P2 / P5 / RM4 / severe degrade-recover 联动" \
  "Browser harness: policy update + automatic override + automatic clear" \
  node tests/qos_harness/browser_server_signal.mjs

run_step \
  "browser_loopback" \
  "R4 / J3 / T5 相关覆盖" \
  "Browser loopback + tc netem 弱网退化/恢复" \
  env QOS_PRINT_JSON=1 node tests/qos_harness/browser_loopback.mjs
