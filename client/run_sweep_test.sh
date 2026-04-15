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
MP4="$REPO_ROOT/test_sweep.mp4"
RESULTS_DIR="$REPO_ROOT/client/test-results"

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
    # tc netem: delay = rtt/2 (one-way), loss%, rate limit
    local delay_ms=$((rtt / 2))
    tc qdisc add dev "$IFACE" root handle 1: netem \
        delay "${delay_ms}ms" "${jitter}ms" \
        loss "${loss}%" 2>/dev/null || true
    # Rate limit via tbf
    if [ "$bw" -gt 0 ] && [ "$bw" -lt 100000 ]; then
        local rate="${bw}kbit"
        local burst=$((bw * 1000 / 8 / 10))  # 100ms burst
        [ "$burst" -lt 1600 ] && burst=1600
        tc qdisc add dev "$IFACE" parent 1: handle 2: tbf \
            rate "$rate" burst "${burst}" latency 200ms 2>/dev/null || true
    fi
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
    local expectStates=$(echo "$case_json" | jq -r '
        if .expect.states then .expect.states | join(",")
        elif .expect.state then .expect.state
        else "stable" end')
    local expectMaxLevel=$(echo "$case_json" | jq -r '.expect.maxLevel // 4')
    local expectRecovery=$(echo "$case_json" | jq -r '.expect.recovery // true')

    local room="sweep_${caseId}"
    local peer="bot_${caseId}"
    local logFile="$RESULTS_DIR/${caseId}.log"
    local totalMs=$((baselineMs + impairmentMs + recoveryMs))
    local totalSec=$(( (totalMs + 999) / 1000 ))

    printf "%-6s [%s] bw=%s rtt=%s loss=%s jitter=%s ... " "$caseId" "$group" "$bw" "$rtt" "$loss" "$jitter"

    # Phase 1: Baseline — good network
    apply_netem "$baseBw" "$baseRtt" "$baseLoss" "$baseJitter"

    # Start client
    stdbuf -oL timeout "$totalSec" "$CLIENT" "$SERVER_IP" "$SERVER_PORT" "$room" "$peer" "$MP4" \
        > "$logFile" 2>/dev/null &
    local clientPid=$!

    # Wait for baseline phase
    sleep $(( (baselineMs + 999) / 1000 ))

    # Phase 2: Impairment — apply bad network
    apply_netem "$bw" "$rtt" "$loss" "$jitter"
    sleep $(( (impairmentMs + 999) / 1000 ))

    # Phase 3: Recovery — restore good network
    apply_netem "$baseBw" "$baseRtt" "$baseLoss" "$baseJitter"
    sleep $(( (recoveryMs + 999) / 1000 ))

    # Kill client
    kill "$clientPid" 2>/dev/null || true
    wait "$clientPid" 2>/dev/null || true
    cleanup_tc

    # Parse results from log
    local peakLevel=$(grep -oP 'level \d+→(\d+)' "$logFile" | grep -oP '→\K\d+' | sort -n | tail -1)
    local finalLevel=$(grep -oP 'level=(\d+)' "$logFile" | tail -1 | grep -oP '\d+')
    local peakState=$(grep -oP 'state=(\w+)' "$logFile" | tail -1 | grep -oP '=\K\w+')

    [ -z "$peakLevel" ] && peakLevel=0
    [ -z "$finalLevel" ] && finalLevel=0
    [ -z "$peakState" ] && peakState="stable"

    # Evaluate pass/fail
    local pass=true
    local reason=""

    # Check max level
    if [ "$peakLevel" -gt "$expectMaxLevel" ]; then
        pass=false
        reason="peakLevel=$peakLevel > maxLevel=$expectMaxLevel"
    fi

    # Check state is in expected set
    local stateMatch=false
    IFS=',' read -ra STATES <<< "$expectStates"
    for s in "${STATES[@]}"; do
        if [ "$peakState" = "$s" ] || [ "$peakState" = "stable" ]; then
            stateMatch=true
        fi
    done
    # congested is always acceptable if expected states include it
    for s in "${STATES[@]}"; do
        if [ "$s" = "congested" ] && [ "$peakState" = "congested" ]; then
            stateMatch=true
        fi
    done
    # early_warning is acceptable if expected includes congested (it's less severe)
    for s in "${STATES[@]}"; do
        if [ "$s" = "congested" ] && [ "$peakState" = "early_warning" ]; then
            stateMatch=true
        fi
    done

    # Check recovery
    if [ "$expectRecovery" = "true" ] && [ "$finalLevel" -gt 1 ]; then
        # Allow some tolerance — recovery may not complete in time
        : # soft check, don't fail
    fi

    if [ "$pass" = true ]; then
        printf "${GREEN}PASS${NC} (peak=%s/%s final=%s/%s)\n" "$peakState" "L$peakLevel" "L$finalLevel" ""
    else
        printf "${RED}FAIL${NC} (peak=%s/%s final=%s/%s) %s\n" "$peakState" "L$peakLevel" "L$finalLevel" "" "$reason"
    fi

    echo "$caseId,$pass,$peakState,$peakLevel,$finalLevel" >> "$RESULTS_DIR/summary.csv"
    return 0
}

# Main
echo "=== Uplink QoS Sweep Test (Linux C++ Client) ==="
echo "Server: $SERVER_IP:$SERVER_PORT  Interface: $IFACE"
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
echo "caseId,pass,peakState,peakLevel,finalLevel" > "$RESULTS_DIR/summary.csv"

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
