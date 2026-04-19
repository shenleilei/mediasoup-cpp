#!/bin/bash
# run_sweep_test.sh — Run uplink QoS sweep cases on Linux C++ client
# Uses tc netem to simulate network conditions, runs plain-client, collects QoS trace.
#
# Usage: ./run_sweep_test.sh [CASE_ID] [--all]
# Examples:
#   ./run_sweep_test.sh L5          # run single case
#   ./run_sweep_test.sh --all       # run all P0 cases
#   ./run_sweep_test.sh --all-p1    # run all cases including P1

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CLIENT="$REPO_ROOT/client/build/plain-client"
CASES_FILE="$REPO_ROOT/tests/qos_harness/scenarios/sweep_cases.json"
SERVER_IP="${SERVER_IP:-127.0.0.1}"
SERVER_PORT="${SERVER_PORT:-3000}"
IFACE="${IFACE:-lo}"
MP4="$REPO_ROOT/tests/fixtures/media/test_sweep.mp4"
RESULTS_DIR="$REPO_ROOT/client/test-results"
RUNNER_NAME="${RUNNER_NAME:-}"

if [ -z "$RUNNER_NAME" ]; then
    if [ "$IFACE" = "lo" ]; then
        RUNNER_NAME="loopback"
    else
        RUNNER_NAME="default"
    fi
fi

# Colors
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'

mkdir -p "$RESULTS_DIR"

# Generate long test video if not exists (60s, keyframe every 1s)
if [ ! -f "$MP4" ]; then
    echo "Generating test video..."
    ffmpeg -y -f lavfi -i "testsrc=d=120:s=640x480:r=25" \
        -pix_fmt yuv420p -c:v libx264 -profile:v baseline -g 25 \
        "$MP4" 2>/dev/null
fi

cleanup_tc() {
    tc qdisc del dev "$IFACE" root 2>/dev/null || true
}
trap cleanup_tc EXIT

apply_netem() {
    local bw="$1" rtt="$2" loss="$3" jitter="$4"
    cleanup_tc
    # Match the browser loopback runner more closely:
    # - keep TCP signaling off the impaired path
    # - use netem's built-in rate instead of a separate TBF child
    # - apply a bandwidth haircut so scenario kbps better matches observed
    #   payload capacity once RTP/audio/queueing overhead is included
    local delay_ms=$((rtt / 2))
    tc qdisc add dev "$IFACE" root handle 1: prio bands 3 \
        priomap 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 2>/dev/null || true

    local netem_args=(
        qdisc add dev "$IFACE" parent 1:3 handle 30: netem
        delay "${delay_ms}ms"
    )

    if [ "${jitter}" != "0" ]; then
        netem_args+=("${jitter}ms" distribution normal)
    fi

    if [ "${loss}" != "0" ]; then
        netem_args+=(loss "${loss}%")
    fi

    if [ "$bw" -gt 0 ] && [ "$bw" -lt 100000 ]; then
        local effective_bw
        if [ "$bw" -ge 3000 ]; then
            effective_bw=$(( (bw * 85 + 99) / 100 ))
        else
            effective_bw=$(( (bw * 70 + 99) / 100 ))
        fi
        [ "$effective_bw" -lt 1 ] && effective_bw=1
        netem_args+=(rate "${effective_bw}kbit")
    fi

    tc "${netem_args[@]}" 2>/dev/null || true
    tc filter add dev "$IFACE" parent 1:0 protocol ip u32 \
        match ip protocol 17 0xff flowid 1:3 2>/dev/null || true
}

state_rank() {
    case "$1" in
        stable) echo 0 ;;
        early_warning) echo 1 ;;
        recovering) echo 2 ;;
        congested) echo 3 ;;
        *) echo 99 ;;
    esac
}

summarize_phase_state() {
    local logFile="$1"
    local phaseStartMs="$2"
    local phaseEndMs="$3"
    local fallbackState="$4"
    local fallbackLevel="$5"
    local mode="$6"

    awk -v phaseStartMs="$phaseStartMs" \
        -v phaseEndMs="$phaseEndMs" \
        -v fallbackState="$fallbackState" \
        -v fallbackLevel="$fallbackLevel" \
        -v mode="$mode" '
        function state_rank(state) {
            if (state == "stable") return 0;
            if (state == "early_warning") return 1;
            if (state == "recovering") return 2;
            if (state == "congested") return 3;
            return 99;
        }
        /^\[QOS_TRACE\]/ {
            ts = "";
            state = "";
            level = "";
            for (i = 1; i <= NF; ++i) {
                split($i, kv, "=");
                key = kv[1];
                value = kv[2];
                if (key == "tsMs") ts = value + 0;
                else if (key == "state") state = value;
                else if (key == "level") level = value + 0;
            }
            if (ts == "" || state == "" || level == "") next;
            if (ts <= phaseStartMs) next;
            if (phaseEndMs > 0 && ts > phaseEndMs) next;

            if (!selected) {
                selectedState = state;
                selectedLevel = level;
                selected = 1;
            } else if (mode == "current") {
                selectedState = state;
                selectedLevel = level;
            } else if (mode == "peak") {
                if (state_rank(state) > state_rank(selectedState) ||
                    (state_rank(state) == state_rank(selectedState) && level > selectedLevel)) {
                    selectedState = state;
                    selectedLevel = level;
                }
            } else if (mode == "best") {
                if (state_rank(state) < state_rank(selectedState) ||
                    (state_rank(state) == state_rank(selectedState) && level < selectedLevel)) {
                    selectedState = state;
                    selectedLevel = level;
                }
            }
        }
        END {
            if (!selected) {
                selectedState = fallbackState;
                selectedLevel = fallbackLevel;
            }
            print selectedState "," selectedLevel;
        }
    ' "$logFile"
}

expects_recovery() {
    local group="$1"
    local expectRecovery="$2"
    if [ "$expectRecovery" = "false" ]; then
        return 1
    fi
    if [ "$group" = "transition" ] || [ "$group" = "burst" ] || [ "$expectRecovery" = "true" ]; then
        return 0
    fi
    return 1
}

run_case() {
    local case_json="$1"
    local caseId=$(echo "$case_json" | jq -r '.caseId')
    local group=$(echo "$case_json" | jq -r '.group')
    local bw=$(echo "$case_json" | jq -r '.bandwidth')
    local rtt=$(echo "$case_json" | jq -r '.rtt')
    local loss=$(echo "$case_json" | jq -r '.loss')
    local jitter=$(echo "$case_json" | jq -r '.jitter')
    local baselineMs=$(echo "$case_json" | jq -r '.baselineMs')
    local impairmentMs=$(echo "$case_json" | jq -r '.impairmentMs')
    local recoveryMs=$(echo "$case_json" | jq -r '.recoveryMs')

    # Baseline network params (defaults to good network)
    local baseBw=$(echo "$case_json" | jq -r '.baselineBw // 4000')
    local baseRtt=$(echo "$case_json" | jq -r '.baselineRtt // 25')
    local baseLoss=$(echo "$case_json" | jq -r '.baselineLoss // 0.1')
    local baseJitter=$(echo "$case_json" | jq -r '.baselineJitter // 5')

    # Expected results
    local expectJson
    expectJson=$(echo "$case_json" | jq -c --arg runner "$RUNNER_NAME" '.expectByRunner[$runner] // .expect // {}')
    local expectStates
    expectStates=$(echo "$expectJson" | jq -r '
        if (.states | type) == "array" and (.states | length) > 0 then .states | join(",")
        elif (.state | type) == "string" and (.state | length) > 0 then .state
        else "stable" end')
    local expectMinLevel
    expectMinLevel=$(echo "$expectJson" | jq -r 'if .minLevel != null then .minLevel else 0 end')
    local expectMaxLevel
    expectMaxLevel=$(echo "$expectJson" | jq -r 'if .maxLevel != null then .maxLevel else 999 end')
    local expectRecovery
    expectRecovery=$(echo "$expectJson" | jq -r 'if has("recovery") then (.recovery | tostring) else "auto" end')

    local room="sweep_${caseId}"
    local peer="bot_${caseId}"
    local logFile="$RESULTS_DIR/${caseId}.log"
    local totalMs=$((baselineMs + impairmentMs + recoveryMs))
    local totalSec=$(( (totalMs + 999) / 1000 ))
    local clientStartMs
    local impairmentStartMs
    local recoveryStartMs
    local runEndMs

    printf "%-6s [%s/%s] bw=%s rtt=%s loss=%s jitter=%s ... " "$caseId" "$group" "$RUNNER_NAME" "$bw" "$rtt" "$loss" "$jitter"

    # Phase 1: Baseline — good network
    apply_netem "$baseBw" "$baseRtt" "$baseLoss" "$baseJitter"

    # Start client
    clientStartMs=$(date +%s%3N)
    stdbuf -oL timeout "$totalSec" "$CLIENT" "$SERVER_IP" "$SERVER_PORT" "$room" "$peer" "$MP4" \
        > "$logFile" 2>/dev/null &
    local clientPid=$!

    # Wait for baseline phase
    sleep $(( (baselineMs + 999) / 1000 ))

    # Phase 2: Impairment — apply bad network
    apply_netem "$bw" "$rtt" "$loss" "$jitter"
    impairmentStartMs=$(date +%s%3N)
    sleep $(( (impairmentMs + 999) / 1000 ))

    # Phase 3: Recovery — restore good network
    apply_netem "$baseBw" "$baseRtt" "$baseLoss" "$baseJitter"
    recoveryStartMs=$(date +%s%3N)
    sleep $(( (recoveryMs + 999) / 1000 ))
    runEndMs=$(date +%s%3N)

    # Kill client
    kill "$clientPid" 2>/dev/null || true
    wait "$clientPid" 2>/dev/null || true
    cleanup_tc

    # Evaluate pass/fail
    local pass=true
    local reason=""
    local baselineCurrent baselineState baselineLevel
    local impairmentCurrent impairmentPeak recoveryCurrent recoveryBest
    local impairmentState impairmentLevel recoveredState recoveredLevel

    if ! grep -q '^\[QOS_TRACE\]' "$logFile"; then
        pass=false
        reason="no_qos_trace"
        baselineState="stable"
        baselineLevel=0
        impairmentState="stable"
        impairmentLevel=0
        recoveredState="stable"
        recoveredLevel=0
    else
        baselineCurrent=$(summarize_phase_state "$logFile" "$clientStartMs" "$impairmentStartMs" "stable" 0 "current")
        IFS=',' read -r baselineState baselineLevel <<< "$baselineCurrent"

        impairmentCurrent=$(summarize_phase_state "$logFile" "$impairmentStartMs" "$recoveryStartMs" "$baselineState" "$baselineLevel" "current")
        impairmentPeak=$(summarize_phase_state "$logFile" "$impairmentStartMs" "$recoveryStartMs" "${impairmentCurrent%%,*}" "${impairmentCurrent##*,}" "peak")
        recoveryCurrent=$(summarize_phase_state "$logFile" "$recoveryStartMs" "$runEndMs" "${impairmentCurrent%%,*}" "${impairmentCurrent##*,}" "current")
        recoveryBest=$(summarize_phase_state "$logFile" "$recoveryStartMs" "$runEndMs" "${recoveryCurrent%%,*}" "${recoveryCurrent##*,}" "best")

        if [ "$group" = "baseline" ]; then
            impairmentState="$baselineState"
            impairmentLevel="$baselineLevel"
        else
            IFS=',' read -r impairmentState impairmentLevel <<< "$impairmentPeak"
        fi
        IFS=',' read -r recoveredState recoveredLevel <<< "$recoveryBest"

        local stateMatch=false
        IFS=',' read -ra STATES <<< "$expectStates"
        for s in "${STATES[@]}"; do
            if [ "$impairmentState" = "$s" ]; then
                stateMatch=true
                break
            fi
        done

        local levelMatch=true
        if [ "$impairmentLevel" -lt "$expectMinLevel" ] || [ "$impairmentLevel" -gt "$expectMaxLevel" ]; then
            levelMatch=false
        fi

        local recoveryPassed=true
        if expects_recovery "$group" "$expectRecovery"; then
            if [ "$(state_rank "$recoveredState")" -gt "$(state_rank "$baselineState")" ] || \
               [ "$recoveredLevel" -gt "$baselineLevel" ]; then
                recoveryPassed=false
            fi
        fi

        if [ "$stateMatch" != true ] || [ "$levelMatch" != true ] || [ "$recoveryPassed" != true ]; then
            pass=false
            reason="stateMatch=$stateMatch,levelMatch=$levelMatch,recoveryPassed=$recoveryPassed"
        fi
    fi

    if [ "$pass" = true ]; then
        printf "${GREEN}PASS${NC} baseline=%s/L%s impaired=%s/L%s recovered=%s/L%s\n" \
            "$baselineState" "$baselineLevel" "$impairmentState" "$impairmentLevel" "$recoveredState" "$recoveredLevel"
    else
        printf "${RED}FAIL${NC} baseline=%s/L%s impaired=%s/L%s recovered=%s/L%s %s\n" \
            "$baselineState" "$baselineLevel" "$impairmentState" "$impairmentLevel" "$recoveredState" "$recoveredLevel" "$reason"
    fi

    echo "$caseId,$pass,$RUNNER_NAME,$baselineState,$baselineLevel,$impairmentState,$impairmentLevel,$recoveredState,$recoveredLevel,$reason" >> "$RESULTS_DIR/summary.csv"
    return 0
}

# Main
echo "=== Uplink QoS Sweep Test (Linux C++ Client) ==="
echo "Server: $SERVER_IP:$SERVER_PORT  Interface: $IFACE  Runner: $RUNNER_NAME"
echo ""

# Parse args
FILTER="${1:-}"
RUN_ALL=false
RUN_ALL_P1=false

if [ "$FILTER" = "--all" ]; then
    RUN_ALL=true; FILTER=""
elif [ "$FILTER" = "--all-p1" ]; then
    RUN_ALL_P1=true; FILTER=""
fi

# Clear previous results
echo "caseId,pass,runner,baselineState,baselineLevel,impairedState,impairedLevel,recoveredState,recoveredLevel,reason" > "$RESULTS_DIR/summary.csv"

# Read cases
TOTAL=0; PASSED=0; FAILED=0

while IFS= read -r case_json; do
    caseId=$(echo "$case_json" | jq -r '.caseId')
    priority=$(echo "$case_json" | jq -r '.priority // "P1"')
    extended=$(echo "$case_json" | jq -r '.extended // false')

    # Filter
    if [ -n "$FILTER" ] && [ "$caseId" != "$FILTER" ]; then continue; fi
    if [ "$RUN_ALL" = true ] && [ "$priority" != "P0" ]; then continue; fi
    if [ "$RUN_ALL_P1" = false ] && [ "$RUN_ALL" = false ] && [ -z "$FILTER" ]; then
        # Default: run P0 only
        if [ "$priority" != "P0" ]; then continue; fi
    fi
    if [ "$extended" = "true" ] && [ -z "$FILTER" ]; then continue; fi

    run_case "$case_json"
    TOTAL=$((TOTAL + 1))
    result=$(tail -1 "$RESULTS_DIR/summary.csv" | cut -d, -f2)
    if [ "$result" = "true" ]; then PASSED=$((PASSED + 1)); else FAILED=$((FAILED + 1)); fi
done < <(jq -c '.[]' "$CASES_FILE")

echo ""
echo "=== Results: $PASSED/$TOTAL passed, $FAILED failed ==="
[ "$FAILED" -gt 0 ] && exit 1 || exit 0
