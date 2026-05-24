#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BUILD_DIR="$ROOT_DIR/build-webrtc-qos-plain"
WORKER_BIN="$ROOT_DIR/mediasoup-worker"
REPORT_DIR="$ROOT_DIR/docs/generated"
REPORT_BASENAME="webrtc-qos-plain-p2-smoke-report"
ARTIFACT_ROOT="${TMPDIR:-/tmp}/webrtc-qos-plain-p2-smoke"
INPUT_FILE=""
SOURCE_MODE="copy"
SYNTHETIC_WIDTH=320
SYNTHETIC_HEIGHT=180
SYNTHETIC_FPS=15
V4L2_DEVICE="/dev/video0"
V4L2_WIDTH=640
V4L2_HEIGHT=360
V4L2_FPS=30
V4L2_INPUT_FORMAT=""
DECODE_QOE=0
CASES="baseline,delay_100ms,loss_2pct,loss_5pct,bandwidth_600k,drop_recover"
DURATION_SECONDS=10
BASE_PORT=33131
PLAY_BASE_PORT=43131
SERVER_IP="127.0.0.1"
MEDIA_IP="127.0.0.1"
NETEM_DEV="lo"
ENABLE_NETEM=0
STRICT=0

usage() {
	cat <<'EOF'
Usage:
  scripts/run_webrtc_qos_plain_p2_smoke.sh [options]

Options:
  --cases <csv>              Case list. Default: baseline,delay_100ms,loss_2pct,loss_5pct,bandwidth_600k,drop_recover
  --duration-seconds <n>     Per-case runtime. Default: 10. Formal P2 runs should use 60.
                             drop_recover uses at least 30s so recovery has a 15s observation window.
  --build-dir <path>         Build directory containing mediasoup-sfu and plain clients.
  --worker-bin <path>        mediasoup-worker binary. Default: ./mediasoup-worker.
  --report-dir <path>        Report output directory. Default: docs/generated.
  --report-name <name>       Report basename without extension. Default: webrtc-qos-plain-p2-smoke-report.
  --artifact-root <path>     Runtime logs/artifacts root. Default: /tmp/webrtc-qos-plain-p2-smoke.
  --input <path>             H264 MP4 input. If omitted, a short synthetic input is generated.
  --source <copy|synthetic|mp4-decode-loop|v4l2>
                             Push source mode. copy uses Annex-B remux; synthetic and mp4-decode-loop use x264 realtime encoder.
  --synthetic-width <px>      Synthetic source width. Default: 320.
  --synthetic-height <px>     Synthetic source height. Default: 180.
  --synthetic-fps <n>         Synthetic source fps. Default: 15.
  --input-v4l2 <path>          V4L2 device for --source v4l2. Default: /dev/video0.
  --v4l2-width <px>            V4L2 capture/encoder width. Default: 640.
  --v4l2-height <px>           V4L2 capture/encoder height. Default: 360.
  --v4l2-fps <n>               V4L2 capture/encoder fps. Default: 30.
  --v4l2-input-format <fmt>    Optional V4L2 input_format, for example mjpeg.
  --decode-qoe                Enable native FFmpeg decode/QoE sink in play client.
  --base-port <port>         First SFU signaling port. Default: 33131.
  --play-base-port <port>    First play UDP listen port. Default: 43131.
  --server-ip <ip>           SFU signaling IP. Default: 127.0.0.1.
  --media-ip <ip>            PlainTransport media remote IP. Default: 127.0.0.1.
  --netem-dev <dev>          tc netem device. Default: lo.
  --enable-netem             Allow tc netem mutation for weak-network cases.
  --strict                   Return non-zero when overall report status is not PASS.
  -h, --help                 Show this help.

Default mode is safe for local development: baseline runs, weak-network cases are SKIP
unless --enable-netem is set. SKIP is written to the report and is never counted as PASS.
EOF
}

require_arg() {
	local opt="$1"
	local value="${2:-}"
	if [[ -z "$value" ]]; then
		echo "missing value for $opt" >&2
		exit 2
	fi
}

while [[ $# -gt 0 ]]; do
	case "$1" in
		--cases) require_arg "$1" "${2:-}"; CASES="$2"; shift 2 ;;
		--cases=*) CASES="${1#*=}"; shift ;;
		--duration-seconds) require_arg "$1" "${2:-}"; DURATION_SECONDS="$2"; shift 2 ;;
		--duration-seconds=*) DURATION_SECONDS="${1#*=}"; shift ;;
		--build-dir) require_arg "$1" "${2:-}"; BUILD_DIR="$2"; shift 2 ;;
		--build-dir=*) BUILD_DIR="${1#*=}"; shift ;;
		--worker-bin) require_arg "$1" "${2:-}"; WORKER_BIN="$2"; shift 2 ;;
		--worker-bin=*) WORKER_BIN="${1#*=}"; shift ;;
		--report-dir) require_arg "$1" "${2:-}"; REPORT_DIR="$2"; shift 2 ;;
		--report-dir=*) REPORT_DIR="${1#*=}"; shift ;;
		--report-name) require_arg "$1" "${2:-}"; REPORT_BASENAME="$2"; shift 2 ;;
		--report-name=*) REPORT_BASENAME="${1#*=}"; shift ;;
		--artifact-root) require_arg "$1" "${2:-}"; ARTIFACT_ROOT="$2"; shift 2 ;;
		--artifact-root=*) ARTIFACT_ROOT="${1#*=}"; shift ;;
		--input) require_arg "$1" "${2:-}"; INPUT_FILE="$2"; shift 2 ;;
		--input=*) INPUT_FILE="${1#*=}"; shift ;;
		--source) require_arg "$1" "${2:-}"; SOURCE_MODE="$2"; shift 2 ;;
		--source=*) SOURCE_MODE="${1#*=}"; shift ;;
		--synthetic-width) require_arg "$1" "${2:-}"; SYNTHETIC_WIDTH="$2"; shift 2 ;;
		--synthetic-width=*) SYNTHETIC_WIDTH="${1#*=}"; shift ;;
		--synthetic-height) require_arg "$1" "${2:-}"; SYNTHETIC_HEIGHT="$2"; shift 2 ;;
		--synthetic-height=*) SYNTHETIC_HEIGHT="${1#*=}"; shift ;;
		--synthetic-fps) require_arg "$1" "${2:-}"; SYNTHETIC_FPS="$2"; shift 2 ;;
		--synthetic-fps=*) SYNTHETIC_FPS="${1#*=}"; shift ;;
		--input-v4l2) require_arg "$1" "${2:-}"; V4L2_DEVICE="$2"; shift 2 ;;
		--input-v4l2=*) V4L2_DEVICE="${1#*=}"; shift ;;
		--v4l2-width) require_arg "$1" "${2:-}"; V4L2_WIDTH="$2"; shift 2 ;;
		--v4l2-width=*) V4L2_WIDTH="${1#*=}"; shift ;;
		--v4l2-height) require_arg "$1" "${2:-}"; V4L2_HEIGHT="$2"; shift 2 ;;
		--v4l2-height=*) V4L2_HEIGHT="${1#*=}"; shift ;;
		--v4l2-fps) require_arg "$1" "${2:-}"; V4L2_FPS="$2"; shift 2 ;;
		--v4l2-fps=*) V4L2_FPS="${1#*=}"; shift ;;
		--v4l2-input-format) require_arg "$1" "${2:-}"; V4L2_INPUT_FORMAT="$2"; shift 2 ;;
		--v4l2-input-format=*) V4L2_INPUT_FORMAT="${1#*=}"; shift ;;
		--decode-qoe) DECODE_QOE=1; shift ;;
		--base-port) require_arg "$1" "${2:-}"; BASE_PORT="$2"; shift 2 ;;
		--base-port=*) BASE_PORT="${1#*=}"; shift ;;
		--play-base-port) require_arg "$1" "${2:-}"; PLAY_BASE_PORT="$2"; shift 2 ;;
		--play-base-port=*) PLAY_BASE_PORT="${1#*=}"; shift ;;
		--server-ip) require_arg "$1" "${2:-}"; SERVER_IP="$2"; shift 2 ;;
		--server-ip=*) SERVER_IP="${1#*=}"; shift ;;
		--media-ip) require_arg "$1" "${2:-}"; MEDIA_IP="$2"; shift 2 ;;
		--media-ip=*) MEDIA_IP="${1#*=}"; shift ;;
		--netem-dev) require_arg "$1" "${2:-}"; NETEM_DEV="$2"; shift 2 ;;
		--netem-dev=*) NETEM_DEV="${1#*=}"; shift ;;
		--enable-netem) ENABLE_NETEM=1; shift ;;
		--strict) STRICT=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
done

SFU_BIN="$BUILD_DIR/mediasoup-sfu"
PUSH_BIN="$BUILD_DIR/webrtc-qos-plain-push-client"
PLAY_BIN="$BUILD_DIR/webrtc-qos-plain-play-client"

for file in "$SFU_BIN" "$PUSH_BIN" "$PLAY_BIN" "$WORKER_BIN"; do
	if [[ ! -x "$file" ]]; then
		echo "required executable not found: $file" >&2
		exit 2
	fi
done

for tool in python3 curl setsid; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		echo "required tool not found: $tool" >&2
		exit 2
	fi
done

if ! [[ "$DURATION_SECONDS" =~ ^[0-9]+$ ]] || [[ "$DURATION_SECONDS" -lt 3 ]]; then
	echo "--duration-seconds must be an integer >= 3" >&2
	exit 2
fi

mkdir -p "$ARTIFACT_ROOT" "$REPORT_DIR"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_DIR="$ARTIFACT_ROOT/$RUN_ID"
mkdir -p "$RUN_DIR"

if [[ "$SOURCE_MODE" != "copy" && "$SOURCE_MODE" != "synthetic" && "$SOURCE_MODE" != "mp4-decode-loop" && "$SOURCE_MODE" != "v4l2" ]]; then
	echo "--source must be copy, synthetic, mp4-decode-loop, or v4l2" >&2
	exit 2
fi

if [[ "$SOURCE_MODE" != "synthetic" && "$SOURCE_MODE" != "v4l2" && -z "$INPUT_FILE" ]]; then
	INPUT_FILE="$ARTIFACT_ROOT/input.mp4"
fi

generate_input_if_needed() {
	if [[ -s "$INPUT_FILE" ]]; then
		return
	fi
	if ! command -v ffmpeg >/dev/null 2>&1; then
		echo "ffmpeg is required to generate default H264 input; pass --input instead" >&2
		exit 2
	fi
	mkdir -p "$(dirname "$INPUT_FILE")"
	local input_seconds=$((DURATION_SECONDS + 6))
	if [[ "$input_seconds" -lt 12 ]]; then
		input_seconds=12
	fi
	ffmpeg -hide_banner -loglevel error \
		-f lavfi -i "testsrc=size=320x180:rate=15" \
		-t "$input_seconds" \
		-c:v libx264 -profile:v baseline -level 3.1 -pix_fmt yuv420p \
		-g 15 -keyint_min 15 -bf 0 -movflags +faststart \
		-y "$INPUT_FILE"
}

case_requires_netem() {
	[[ "$1" != "baseline" ]]
}

network_condition() {
	case "$1" in
		baseline) echo "none" ;;
		delay_100ms) echo "100ms delay + 20ms jitter" ;;
		loss_2pct) echo "2% random loss" ;;
		loss_5pct) echo "5% random loss" ;;
		bandwidth_600k) echo "600kbps rate limit" ;;
		drop_recover) echo "5% loss + 600kbps, then recovery" ;;
		*) echo "unknown" ;;
	esac
}

netem_args() {
	case "$1" in
		delay_100ms) echo "delay 100ms 20ms distribution normal" ;;
		loss_2pct) echo "loss 2%" ;;
		loss_5pct) echo "loss 5%" ;;
		bandwidth_600k) echo "rate 600kbit" ;;
		drop_recover) echo "loss 5% rate 600kbit" ;;
		*) echo "" ;;
	esac
}

cleanup_netem() {
	if [[ "$ENABLE_NETEM" -eq 1 ]] && command -v tc >/dev/null 2>&1; then
		tc qdisc del dev "$NETEM_DEV" root >/dev/null 2>&1 || true
	fi
}

now_ms() {
	date +%s%3N
}

json_number_or_null() {
	if [[ -n "${1:-}" ]]; then
		printf '%s' "$1"
	else
		printf 'null'
	fi
}

NETEM_SKIP_REASON=""
check_netem_ready() {
	if [[ "$ENABLE_NETEM" -ne 1 ]]; then
		NETEM_SKIP_REASON="netem disabled; pass --enable-netem to run weak-network cases"
		return 1
	fi
	if ! command -v tc >/dev/null 2>&1; then
		NETEM_SKIP_REASON="tc command not found"
		return 1
	fi
	if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
		NETEM_SKIP_REASON="CAP_NET_ADMIN/root is required for tc netem"
		return 1
	fi
	cleanup_netem
	local err="$RUN_DIR/netem-preflight.err"
	if ! tc qdisc add dev "$NETEM_DEV" root netem delay 1ms >"$err" 2>&1; then
		NETEM_SKIP_REASON="tc netem preflight failed: $(tr '\n' ' ' < "$err")"
		cleanup_netem
		return 1
	fi
	cleanup_netem
	NETEM_SKIP_REASON=""
	return 0
}

process_alive() {
	local pid="$1"
	if ! kill -0 "$pid" >/dev/null 2>&1; then
		return 1
	fi
	local state
	state="$(ps -o stat= -p "$pid" 2>/dev/null | awk '{print $1}')"
	[[ "$state" != Z* ]]
}

terminate_group() {
	local pid="${1:-}"
	if [[ -z "$pid" ]]; then
		return
	fi
	if ! kill -0 "$pid" >/dev/null 2>&1; then
		wait "$pid" 2>/dev/null || true
		return
	fi
	kill -TERM "$pid" >/dev/null 2>&1 || true
	for _ in $(seq 1 80); do
		if ! process_alive "$pid"; then
			wait "$pid" 2>/dev/null || true
			break
		fi
		sleep 0.1
	done
	if process_alive "$pid"; then
		kill -TERM "-$pid" >/dev/null 2>&1 || kill -TERM "$pid" >/dev/null 2>&1 || true
		for _ in $(seq 1 50); do
			if ! process_alive "$pid"; then
				wait "$pid" 2>/dev/null || true
				break
			fi
			sleep 0.1
		done
	fi
	if pgrep -g "$pid" >/dev/null 2>&1; then
		kill -KILL "-$pid" >/dev/null 2>&1 || kill -KILL "$pid" >/dev/null 2>&1 || true
	fi
	wait "$pid" 2>/dev/null || true
}

wait_ready() {
	local port="$1"
	local out="$2"
	for _ in $(seq 1 120); do
		if curl -fsS "http://$SERVER_IP:$port/readyz" >"$out.tmp" 2>/dev/null; then
			if python3 - "$out.tmp" <<'PY'
import json
import sys
try:
    data = json.load(open(sys.argv[1], 'r'))
    sys.exit(0 if data.get('ok') is True else 1)
except Exception:
    sys.exit(1)
PY
			then
				mv "$out.tmp" "$out"
				return 0
			fi
		fi
		sleep 0.25
	done
	rm -f "$out.tmp"
	return 1
}

write_skip() {
	local case_dir="$1"
	local case_name="$2"
	local reason="$3"
	mkdir -p "$case_dir"
	printf '%s\n' "$reason" >"$case_dir/SKIP_REASON"
	printf '%s\n' "$(network_condition "$case_name")" >"$case_dir/NETWORK_CONDITION"
}

source_skip_reason() {
	if [[ "$SOURCE_MODE" == "v4l2" && ! -e "$V4L2_DEVICE" ]]; then
		printf 'v4l2 device not found: %s' "$V4L2_DEVICE"
		return 0
	fi
	return 1
}

apply_netem() {
	local case_name="$1"
	local case_dir="$2"
	local args
	args="$(netem_args "$case_name")"
	if [[ -z "$args" ]]; then
		return 0
	fi
	cleanup_netem
	printf 'apply_start epochMs=%s args="%s" dev=%s\n' "$(now_ms)" "$args" "$NETEM_DEV" >>"$case_dir/netem.log"
	# shellcheck disable=SC2086
	if ! tc qdisc add dev "$NETEM_DEV" root netem $args >>"$case_dir/netem.log" 2>&1; then
		return 1
	fi
	printf 'apply_done epochMs=%s args="%s" dev=%s\n' "$(now_ms)" "$args" "$NETEM_DEV" >>"$case_dir/netem.log"
	return 0
}

run_case() {
	local case_name="$1"
	local index="$2"
	local case_dir="$RUN_DIR/$case_name"
	local port=$((BASE_PORT + index))
	local play_port=$((PLAY_BASE_PORT + index))
	local room="p2-${case_name}-${RUN_ID}"
	local push_peer="p2-push-${index}"
	local play_peer="p2-play-${index}"
	local sfu_pid=""
	local play_pid=""
	local push_pid=""
	local pre=0
	local weak=0
	local recover=0
	local effective_duration=$DURATION_SECONDS
	local start_epoch_ms=""
	local apply_epoch_ms=""
	local clear_epoch_ms=""
	local end_epoch_ms=""
	local netem_applied=0

	mkdir -p "$case_dir/push" "$case_dir/play"
	printf '%s\n' "$(network_condition "$case_name")" >"$case_dir/NETWORK_CONDITION"

	local source_skip=""
	if source_skip="$(source_skip_reason)"; then
		write_skip "$case_dir" "$case_name" "$source_skip"
		return
	fi
	if case_requires_netem "$case_name" && [[ -n "$NETEM_SKIP_REASON" ]]; then
		write_skip "$case_dir" "$case_name" "$NETEM_SKIP_REASON"
		return
	fi

	start_epoch_ms="$(now_ms)"

	setsid "$SFU_BIN" \
		--nodaemon \
		--port="$port" \
		--workers=1 \
		--workerThreads=1 \
		--listenIp="$SERVER_IP" \
		--announcedIp="$SERVER_IP" \
		--workerBin="$WORKER_BIN" \
		--noRedisRequired \
		>"$case_dir/sfu.stdout.log" 2>&1 &
	sfu_pid=$!

	if ! wait_ready "$port" "$case_dir/readyz.json"; then
		printf 'readyz timeout\n' >"$case_dir/HARNESS_FAILURE"
		terminate_group "$sfu_pid"
		return
	fi

	local play_args=(
		"$PLAY_BIN"
		--server-ip="$SERVER_IP"
		--server-port="$port"
		--room="$room"
		--peer="$play_peer"
		--listen-ip="$SERVER_IP"
		--advertise-ip="$SERVER_IP"
		--listen-port="$play_port"
		--output-null=true
		--wait-consumer-timeout-ms=15000
		--media-remote-ip="$MEDIA_IP"
		--log-dir="$case_dir/play"
	)
	if [[ "$DECODE_QOE" == "1" ]]; then
		play_args+=(--decode-qoe=true)
	fi
	setsid "${play_args[@]}" >"$case_dir/play.stdout.log" 2>&1 &
	play_pid=$!

	sleep 0.7

	local push_args=(
		"$PUSH_BIN"
		--server-ip="$SERVER_IP"
		--server-port="$port"
		--room="$room"
		--peer="$push_peer"
		--video-ssrc="$((11111111 + index))"
		--media-remote-ip="$MEDIA_IP"
		--log-dir="$case_dir/push"
	)
	if [[ "$SOURCE_MODE" == "synthetic" ]]; then
		push_args+=(
			--input-synthetic=true
			--encoder=x264
			--synthetic-width="$SYNTHETIC_WIDTH"
			--synthetic-height="$SYNTHETIC_HEIGHT"
			--synthetic-fps="$SYNTHETIC_FPS"
		)
	elif [[ "$SOURCE_MODE" == "mp4-decode-loop" ]]; then
		push_args+=(
			--input="$INPUT_FILE"
			--input-decode-loop=true
			--loop-input=true
			--encoder=x264
		)
	elif [[ "$SOURCE_MODE" == "v4l2" ]]; then
		push_args+=(
			--input-v4l2="$V4L2_DEVICE"
			--encoder=x264
			--v4l2-width="$V4L2_WIDTH"
			--v4l2-height="$V4L2_HEIGHT"
			--v4l2-fps="$V4L2_FPS"
		)
		if [[ -n "$V4L2_INPUT_FORMAT" ]]; then
			push_args+=(--v4l2-input-format="$V4L2_INPUT_FORMAT")
		fi
	else
		push_args+=(--input="$INPUT_FILE" --loop-input=true)
	fi
	setsid "${push_args[@]}" >"$case_dir/push.stdout.log" 2>&1 &
	push_pid=$!

	if case_requires_netem "$case_name"; then
		if [[ "$case_name" == "drop_recover" && "$DURATION_SECONDS" -lt 30 ]]; then
			effective_duration=30
			pre=3
			weak=12
			recover=15
		elif [[ "$case_name" == "drop_recover" && "$DURATION_SECONDS" -lt 60 ]]; then
			effective_duration=$DURATION_SECONDS
			pre=$((DURATION_SECONDS / 10))
			recover=$((DURATION_SECONDS / 2))
			if [[ "$pre" -lt 3 ]]; then pre=3; fi
			if [[ "$recover" -lt 15 ]]; then recover=15; fi
			weak=$((effective_duration - pre - recover))
			if [[ "$weak" -lt 2 ]]; then
				weak=2
				effective_duration=$((pre + weak + recover))
			fi
		else
			pre=$((DURATION_SECONDS / 4))
			weak=$((DURATION_SECONDS / 2))
			recover=$((DURATION_SECONDS - pre - weak))
			if [[ "$pre" -lt 1 ]]; then pre=1; fi
			if [[ "$weak" -lt 1 ]]; then weak=1; fi
			if [[ "$recover" -lt 1 ]]; then recover=1; fi
		fi
		printf 'schedule preSeconds=%s weakSeconds=%s recoverSeconds=%s durationSeconds=%s effectiveDurationSeconds=%s\n' "$pre" "$weak" "$recover" "$DURATION_SECONDS" "$effective_duration" >"$case_dir/netem.log"
		sleep "$pre"
		apply_epoch_ms="$(now_ms)"
		if ! apply_netem "$case_name" "$case_dir"; then
			printf 'netem apply failed: %s\n' "$(tr '\n' ' ' < "$case_dir/netem.log" 2>/dev/null || true)" >"$case_dir/HARNESS_FAILURE"
		else
			netem_applied=1
			sleep "$weak"
			cleanup_netem
			clear_epoch_ms="$(now_ms)"
			printf 'clear_done epochMs=%s dev=%s\n' "$clear_epoch_ms" "$NETEM_DEV" >>"$case_dir/netem.log"
			sleep "$recover"
		fi
	else
		sleep "$DURATION_SECONDS"
	fi

	cleanup_netem
	end_epoch_ms="$(now_ms)"
	{
		printf '{\n'
		printf '  "case": "%s",\n' "$case_name"
		printf '  "durationSeconds": %s,\n' "$DURATION_SECONDS"
		printf '  "effectiveDurationSeconds": %s,\n' "$effective_duration"
		printf '  "preSeconds": %s,\n' "$pre"
		printf '  "weakSeconds": %s,\n' "$weak"
		printf '  "recoverSeconds": %s,\n' "$recover"
		printf '  "netemApplied": %s,\n' "$netem_applied"
		printf '  "startEpochMs": %s,\n' "$(json_number_or_null "$start_epoch_ms")"
		printf '  "applyEpochMs": %s,\n' "$(json_number_or_null "$apply_epoch_ms")"
		printf '  "clearEpochMs": %s,\n' "$(json_number_or_null "$clear_epoch_ms")"
		printf '  "endEpochMs": %s\n' "$(json_number_or_null "$end_epoch_ms")"
		printf '}\n'
	} >"$case_dir/case_timing.json"
	terminate_group "$push_pid"
	terminate_group "$play_pid"
	terminate_group "$sfu_pid"
}

render_report() {
	local report_json="$REPORT_DIR/$REPORT_BASENAME.json"
	local report_md="$REPORT_DIR/$REPORT_BASENAME.md"
	python3 - "$ROOT_DIR" "$RUN_DIR" "$REPORT_DIR" "$report_json" "$report_md" "$CASES" "$DURATION_SECONDS" "$BUILD_DIR" "$WORKER_BIN" "$INPUT_FILE" "$ENABLE_NETEM" "$NETEM_DEV" "$STRICT" "$SOURCE_MODE" "$SYNTHETIC_WIDTH" "$SYNTHETIC_HEIGHT" "$SYNTHETIC_FPS" "$DECODE_QOE" "$V4L2_DEVICE" "$V4L2_WIDTH" "$V4L2_HEIGHT" "$V4L2_FPS" "$V4L2_INPUT_FORMAT" <<'PY'
import datetime
import glob
import json
import os
import platform
import re
import statistics
import subprocess
import sys
import time

root_dir, run_dir, report_dir, report_json, report_md = sys.argv[1:6]
case_names = [c.strip() for c in sys.argv[6].split(',') if c.strip()]
duration_seconds = int(sys.argv[7])
build_dir = sys.argv[8]
worker_bin = sys.argv[9]
input_file = sys.argv[10]
enable_netem = sys.argv[11] == '1'
netem_dev = sys.argv[12]
strict = sys.argv[13] == '1'
source_mode = sys.argv[14]
synthetic_width = int(sys.argv[15])
synthetic_height = int(sys.argv[16])
synthetic_fps = int(sys.argv[17])
decode_qoe = sys.argv[18] == '1'
v4l2_device = sys.argv[19]
v4l2_width = int(sys.argv[20])
v4l2_height = int(sys.argv[21])
v4l2_fps = int(sys.argv[22])
v4l2_input_format = sys.argv[23]

ANSI_RE = re.compile(r'\x1b\[[0-9;]*m')

def read_file(path):
    try:
        with open(path, 'r', encoding='utf-8', errors='replace') as f:
            return ANSI_RE.sub('', f.read())
    except Exception:
        return ''

def read_first(path):
    text = read_file(path).strip()
    return text.splitlines()[0] if text else ''

def read_json(path):
    try:
        with open(path, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception:
        return {}

def path_from_root(path):
    if not path:
        return ''
    return path if os.path.isabs(path) else os.path.abspath(os.path.join(root_dir, path))

def command_output(argv):
    try:
        return subprocess.check_output(argv, stderr=subprocess.STDOUT).decode('utf-8', 'replace').strip()
    except Exception as exc:
        return str(exc)

def parse_cmake_cache(path):
    result = {}
    for line in read_file(path).splitlines():
        line = line.strip()
        if not line or line.startswith('//') or line.startswith('#') or ':' not in line or '=' not in line:
            continue
        key_type, value = line.split('=', 1)
        key = key_type.split(':', 1)[0]
        result[key] = value
    return result

def sdk_dist_from_cmake_dir(cmake_dir):
    marker = os.path.join('lib', 'cmake', 'WebRtcQosSdk')
    if cmake_dir and cmake_dir.endswith(marker):
        return cmake_dir[:-len(marker)].rstrip(os.sep)
    return ''

def collect_failed_checks(cases):
    failed = []
    for case in cases:
        for check in case.get('checks', []):
            if check.get('status') != 'PASS':
                failed.append({
                    'case': case.get('name'),
                    'check': check.get('name'),
                    'status': check.get('status'),
                    'evidence': check.get('evidence'),
                })
    return failed

def numbers(values):
    values = [float(v) for v in values if v is not None]
    if not values:
        return {'min': None, 'avg': None, 'max': None}
    return {
        'min': min(values),
        'avg': sum(values) / float(len(values)),
        'max': max(values),
    }

def last_int(regex, text, group=1):
    matches = list(re.finditer(regex, text))
    if not matches:
        return None
    return int(matches[-1].group(group))

def first_int(regex, text, group=1):
    match = re.search(regex, text)
    return int(match.group(group)) if match else None

def parse_log_epoch_ms(line):
    match = re.match(r'(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.(\d{3})', line)
    if not match:
        return None
    try:
        dt = datetime.datetime.strptime(match.group(1), '%Y-%m-%d %H:%M:%S')
        return int(time.mktime(dt.timetuple()) * 1000) + int(match.group(2))
    except Exception:
        return None

def parse_push_metrics(text):
    rows = []
    pattern = re.compile(
        r'push_metrics pushedAu=(\d+) targetBps=(\d+) pacingBps=(\d+) '
        r'finalTargetBps=(\d+) rttMs=([0-9.]+) loss=([0-9.]+) '
        r'rtcpFeedbackPacketsIn=(\d+) rtcpFeedbackBytesIn=(\d+) '
        r'rtcpFeedbackFailures=(\d+) maxFps=([0-9.]+) requestKeyframe=(\w+)'
        r'(?: droppedFrames=(\d+))?')
    for line in text.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        rows.append({
            'epochMs': parse_log_epoch_ms(line),
            'pushedAu': int(match.group(1)),
            'targetBps': int(match.group(2)),
            'pacingBps': int(match.group(3)),
            'finalTargetBps': int(match.group(4)),
            'rttMs': float(match.group(5)),
            'loss': float(match.group(6)),
            'rtcpFeedbackPacketsIn': int(match.group(7)),
            'rtcpFeedbackBytesIn': int(match.group(8)),
            'rtcpFeedbackFailures': int(match.group(9)),
            'maxFps': float(match.group(10)),
            'requestKeyframe': match.group(11),
            'droppedFrames': int(match.group(12) or 0),
        })
    return rows

def parse_play_metrics(text):
    rows = []
    pattern = re.compile(
        r'play_metrics rtpPackets=(\d+) rtcpPackets=(\d+) rtcpPacketsOut=(\d+) '
        r'rtcpBytesOut=(\d+) rtcpSendFailures=(\d+) outputAu=(\d+) '
        r'nack=(\d+) pli=(\d+) retransmission=(\d+) droppedRetransmission=(\d+) '
        r'rttMs=([0-9.]+) lossQ8=(\d+)')
    for line in text.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        rows.append({
            'epochMs': parse_log_epoch_ms(line),
            'rtpPackets': int(match.group(1)),
            'rtcpPackets': int(match.group(2)),
            'rtcpPacketsOut': int(match.group(3)),
            'rtcpBytesOut': int(match.group(4)),
            'rtcpSendFailures': int(match.group(5)),
            'outputAu': int(match.group(6)),
            'nack': int(match.group(7)),
            'pli': int(match.group(8)),
            'retransmission': int(match.group(9)),
            'droppedRetransmission': int(match.group(10)),
            'rttMs': float(match.group(11)),
            'lossQ8': int(match.group(12)),
        })
    return rows

def parse_encoder_metrics(text):
    rows = []
    pattern = re.compile(
        r'encoder_metrics mode=(\w+) encoder=(\w+) currentBitrateBps=(\d+) '
        r'currentFps=(\d+) width=(\d+) height=(\d+) framesGenerated=(\d+) '
        r'framesEncoded=(\d+) accessUnits=(\d+) keyframes=(\d+) '
        r'encoderRecreates=(\d+) bitrateChanges=(\d+) fpsChanges=(\d+) '
        r'forcedKeyframeRequests=(\d+)'
        r'(?: forcedKeyframes=(\d+) maxForcedKeyframeDelayUs=(-?\d+))? '
        r'lastKeyframe=(\w+)')
    for match in pattern.finditer(text):
        rows.append({
            'mode': match.group(1),
            'encoder': match.group(2),
            'currentBitrateBps': int(match.group(3)),
            'currentFps': int(match.group(4)),
            'width': int(match.group(5)),
            'height': int(match.group(6)),
            'framesGenerated': int(match.group(7)),
            'framesEncoded': int(match.group(8)),
            'accessUnits': int(match.group(9)),
            'keyframes': int(match.group(10)),
            'encoderRecreates': int(match.group(11)),
            'bitrateChanges': int(match.group(12)),
            'fpsChanges': int(match.group(13)),
            'forcedKeyframeRequests': int(match.group(14)),
            'forcedKeyframes': int(match.group(15) or 0),
            'maxForcedKeyframeDelayUs': int(match.group(16) or -1),
            'lastKeyframe': match.group(17),
        })
    return rows

def parse_qoe_metrics(text):
    rows = []
    pattern = re.compile(
        r'qoe_metrics enabled=(\w+) accessUnitsIn=(\d+) keyframesIn=(\d+) '
        r'decodedFrames=(\d+) decodeErrors=(\d+) freezeCount=(\d+) '
        r'firstFrameDelayUs=(-?\d+) maxFrameGapUs=(\d+) outputFps=([0-9.]+) '
        r'width=(\d+) height=(\d+)')
    for line in text.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        rows.append({
            'epochMs': parse_log_epoch_ms(line),
            'enabled': match.group(1),
            'accessUnitsIn': int(match.group(2)),
            'keyframesIn': int(match.group(3)),
            'decodedFrames': int(match.group(4)),
            'decodeErrors': int(match.group(5)),
            'freezeCount': int(match.group(6)),
            'firstFrameDelayUs': int(match.group(7)),
            'maxFrameGapUs': int(match.group(8)),
            'outputFps': float(match.group(9)),
            'width': int(match.group(10)),
            'height': int(match.group(11)),
        })
    stop_pattern = re.compile(
        r'qoe_runtime_stopped enabled=(\w+) accessUnitsIn=(\d+) keyframesIn=(\d+) '
        r'decodedFrames=(\d+) decodeErrors=(\d+) freezeCount=(\d+) '
        r'firstFrameDelayUs=(-?\d+) maxFrameGapUs=(\d+) outputFps=([0-9.]+) '
        r'width=(\d+) height=(\d+)')
    for line in text.splitlines():
        match = stop_pattern.search(line)
        if not match:
            continue
        rows.append({
            'epochMs': parse_log_epoch_ms(line),
            'enabled': match.group(1),
            'accessUnitsIn': int(match.group(2)),
            'keyframesIn': int(match.group(3)),
            'decodedFrames': int(match.group(4)),
            'decodeErrors': int(match.group(5)),
            'freezeCount': int(match.group(6)),
            'firstFrameDelayUs': int(match.group(7)),
            'maxFrameGapUs': int(match.group(8)),
            'outputFps': float(match.group(9)),
            'width': int(match.group(10)),
            'height': int(match.group(11)),
        })
    return rows

def recovery_qoe_metrics(qoe_metrics, clear_ms):
    result = {
        'enabled': bool(qoe_metrics) and isinstance(clear_ms, int),
        'clearEpochMs': clear_ms,
        'preClearDecodedFrames': None,
        'postClearFirstDecodedEpochMs': None,
        'postClearFirstDecodedDelayMs': None,
        'postClearDecodedFramesDelta': None,
        'postClearSamples': 0,
    }
    if not qoe_metrics or not isinstance(clear_ms, int):
        return result
    pre_clear = [
        m for m in qoe_metrics
        if isinstance(m.get('epochMs'), int) and m['epochMs'] < clear_ms
    ]
    post_clear = [
        m for m in qoe_metrics
        if isinstance(m.get('epochMs'), int) and m['epochMs'] >= clear_ms
    ]
    result['postClearSamples'] = len(post_clear)
    baseline_decoded = (
        pre_clear[-1].get('decodedFrames')
        if pre_clear else
        (qoe_metrics[0].get('decodedFrames') if qoe_metrics else None))
    if baseline_decoded is None:
        return result
    result['preClearDecodedFrames'] = baseline_decoded
    last_post_decoded = post_clear[-1].get('decodedFrames') if post_clear else baseline_decoded
    result['postClearDecodedFramesDelta'] = max(0, last_post_decoded - baseline_decoded)
    for row in post_clear:
        if row.get('decodedFrames', 0) > baseline_decoded:
            result['postClearFirstDecodedEpochMs'] = row.get('epochMs')
            result['postClearFirstDecodedDelayMs'] = row.get('epochMs') - clear_ms
            break
    return result

def parse_jsonl_file(path):
    rows = []
    for line in read_file(path).splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            rows.append(json.loads(line))
        except Exception:
            continue
    return rows

def max_json_int(rows, key):
    values = []
    for row in rows:
        value = row.get(key)
        if isinstance(value, bool):
            continue
        if isinstance(value, (int, float)):
            values.append(int(value))
    return max(values) if values else 0

def parse_sdk_runtime(case_dir, role):
    role_dir = os.path.join(case_dir, role)
    runtime_log_files = sorted(glob.glob(os.path.join(role_dir, '{}.{}.*.log'.format(role, role))))
    metrics_files = sorted(glob.glob(os.path.join(role_dir, '{}_metrics.{}.*.jsonl'.format(role, role))))
    alerts_files = sorted(glob.glob(os.path.join(role_dir, '{}_alerts.{}.*.jsonl'.format(role, role))))
    metric_rows = []
    for path in metrics_files:
        metric_rows.extend(parse_jsonl_file(path))
    session_rows = [row for row in metric_rows if row.get('scope') == 'session']
    return {
        'runtimeLogFiles': len(runtime_log_files),
        'metricsFiles': len(metrics_files),
        'alertsFiles': len(alerts_files),
        'sessionMetricSamples': len(session_rows),
        'receiverReportCountMax': max_json_int(session_rows, 'receiver_report_count'),
        'transportFeedbackCountMax': max_json_int(session_rows, 'transport_feedback_count'),
        'processTickCountMax': max_json_int(session_rows, 'process_tick_count'),
        'maxProcessTickGapUs': max_json_int(session_rows, 'max_process_tick_gap_us'),
        'transportFailureCountMax': max_json_int(session_rows, 'transport_failure_count'),
    }

IGNORED_ALERT_PATTERNS = [
    re.compile(r'GeoRouter DB .* not found, falling back'),
    re.compile(r'worker pipe closed .* delegating to detached reaper'),
    re.compile(r'worker died .* worker process pipe closed'),
    re.compile(r'WorkerThread \d+ worker died .* attempting respawn'),
    re.compile(r'WorkerThread \d+ respawned worker'),
    re.compile(r'worker process \[pid:\d+\] exited with code 0'),
]

def is_ignored_alert(line):
    return any(pattern.search(line) for pattern in IGNORED_ALERT_PATTERNS)

def make_check(name, passed, evidence):
    return {'name': name, 'status': 'PASS' if passed else 'FAIL', 'evidence': evidence}

def is_number(value):
    return isinstance(value, (int, float)) and not isinstance(value, bool)

def positive(value):
    return is_number(value) and value > 0

def non_negative(value):
    return is_number(value) and value >= 0

def status_from_checks(checks):
    return 'PASS' if all(c['status'] == 'PASS' for c in checks) else 'FAIL'

def parse_case(case_name):
    case_dir = os.path.join(run_dir, case_name)
    condition = read_first(os.path.join(case_dir, 'NETWORK_CONDITION')) or 'unknown'
    timing = read_json(os.path.join(case_dir, 'case_timing.json'))
    skip_reason = read_first(os.path.join(case_dir, 'SKIP_REASON'))
    if skip_reason:
        return {
            'name': case_name,
            'status': 'SKIP',
            'networkCondition': condition,
            'skipReason': skip_reason,
            'artifacts': {'caseDir': case_dir},
            'checks': [],
            'metrics': {'timing': timing},
        }

    push_text = '\n'.join([
        read_file(os.path.join(case_dir, 'push', 'push.log')),
        read_file(os.path.join(case_dir, 'push.stdout.log')),
    ])
    play_text = '\n'.join([
        read_file(os.path.join(case_dir, 'play', 'play.log')),
        read_file(os.path.join(case_dir, 'play.stdout.log')),
    ])
    sfu_text = read_file(os.path.join(case_dir, 'sfu.stdout.log'))
    all_text = '\n'.join([push_text, play_text, sfu_text])

    ready_ok = False
    try:
        with open(os.path.join(case_dir, 'readyz.json'), 'r', encoding='utf-8') as f:
            ready_ok = json.load(f).get('ok') is True
    except Exception:
        ready_ok = False

    publish_twcc = first_int(r'plain_publish_ok .*twccExtId=(\d+)', push_text)
    selected_twcc = first_int(r'selected_consumer .*twccExtId=(\d+)', play_text)
    audio_enabled_false = re.search(r'plain_publish_ok .*audioEnabled=false', push_text) is not None
    has_audio_consumer = re.search(r'new_consumer_notification .*"kind"\s*:\s*"audio"', play_text) is not None

    push_stop = re.search(
        r'push_runtime_stopped pushedAu=(\d+) rtcpFeedbackPacketsIn=(\d+) '
        r'rtcpFeedbackBytesIn=(\d+) rtcpFeedbackFailures=(\d+)', push_text)
    play_stop = re.search(
        r'play_runtime_stopped rtpPackets=(\d+) rtcpPackets=(\d+) rtcpPacketsOut=(\d+) '
        r'rtcpBytesOut=(\d+) rtcpSendFailures=(\d+) outputAu=(\d+)', play_text)

    push_metrics = parse_push_metrics(push_text)
    play_metrics = parse_play_metrics(play_text)
    encoder_metrics = parse_encoder_metrics(push_text)
    qoe_metrics = parse_qoe_metrics(play_text)
    push_sdk_runtime_enabled = re.search(r'sdk_runtime_files role=push enabled=true', push_text) is not None
    play_sdk_runtime_enabled = re.search(r'sdk_runtime_files role=play enabled=true', play_text) is not None
    push_sdk_runtime = parse_sdk_runtime(case_dir, 'push')
    play_sdk_runtime = parse_sdk_runtime(case_dir, 'play')

    pushed_au = int(push_stop.group(1)) if push_stop else last_int(r'push_metrics pushedAu=(\d+)', push_text)
    push_rtcp_in = int(push_stop.group(2)) if push_stop else last_int(r'rtcpFeedbackPacketsIn=(\d+)', push_text)
    push_rtcp_bytes_in = int(push_stop.group(3)) if push_stop else last_int(r'rtcpFeedbackBytesIn=(\d+)', push_text)
    push_rtcp_failures = int(push_stop.group(4)) if push_stop else last_int(r'rtcpFeedbackFailures=(\d+)', push_text)
    play_rtp = int(play_stop.group(1)) if play_stop else last_int(r'play_metrics rtpPackets=(\d+)', play_text)
    play_rtcp_in = int(play_stop.group(2)) if play_stop else last_int(r'play_metrics rtpPackets=\d+ rtcpPackets=(\d+)', play_text)
    play_rtcp_out = int(play_stop.group(3)) if play_stop else last_int(r'rtcpPacketsOut=(\d+)', play_text)
    play_rtcp_bytes_out = int(play_stop.group(4)) if play_stop else last_int(r'rtcpBytesOut=(\d+)', play_text)
    play_rtcp_send_failures = int(play_stop.group(5)) if play_stop else last_int(r'rtcpSendFailures=(\d+)', play_text)
    output_au = int(play_stop.group(6)) if play_stop else last_int(r'outputAu=(\d+)', play_text)

    max_nack = max([m['nack'] for m in play_metrics] or [0])
    max_pli = max([m['pli'] for m in play_metrics] or [0])
    max_retransmission = max([m['retransmission'] for m in play_metrics] or [0])
    max_dropped_frames = max([m['droppedFrames'] for m in push_metrics] or [0])
    last_encoder = encoder_metrics[-1] if encoder_metrics else {}
    last_qoe = qoe_metrics[-1] if qoe_metrics else {}
    recovery_qoe = recovery_qoe_metrics(qoe_metrics, timing.get('clearEpochMs'))

    warning_lines = []
    ignored_warning_lines = []
    for line in all_text.splitlines():
        lowered = line.lower()
        if any(token in lowered for token in ['[error]', '[warning]', ' failed', '_failed', 'timeout', 'malformed']):
            clean_line = line.strip()
            if is_ignored_alert(clean_line):
                ignored_warning_lines.append(clean_line)
            else:
                warning_lines.append(clean_line)
    warning_lines = warning_lines[:20]

    harness_failure = read_first(os.path.join(case_dir, 'HARNESS_FAILURE'))

    checks = [
        make_check('sfu-readyz-ok', ready_ok, 'readyz ok=true' if ready_ok else 'missing or non-ok readyz'),
        make_check('push-video-only-publish', bool(audio_enabled_false), 'audioEnabled=false' if audio_enabled_false else 'plain_publish_ok audioEnabled=false not found'),
        make_check('publish-twcc-ext', positive(publish_twcc), 'twccExtId={}'.format(publish_twcc)),
        make_check('play-selected-twcc-ext', positive(selected_twcc), 'twccExtId={}'.format(selected_twcc)),
        make_check('no-audio-consumer', not has_audio_consumer, 'audio consumer absent' if not has_audio_consumer else 'audio consumer notification found'),
        make_check('push-au-output', positive(pushed_au), 'pushedAu={}'.format(pushed_au)),
        make_check('play-rtp-input', positive(play_rtp), 'rtpPackets={}'.format(play_rtp)),
        make_check('play-au-output', positive(output_au), 'outputAu={}'.format(output_au)),
        make_check('push-rtcp-feedback-input', positive(push_rtcp_in), 'rtcpFeedbackPacketsIn={}'.format(push_rtcp_in)),
        make_check('push-rtcp-feedback-no-failures', push_rtcp_failures == 0, 'rtcpFeedbackFailures={}'.format(push_rtcp_failures)),
        make_check('play-rtcp-send-no-failures', play_rtcp_send_failures == 0, 'rtcpSendFailures={}'.format(play_rtcp_send_failures)),
        make_check('sdk-push-runtime-enabled', push_sdk_runtime_enabled, 'enabled={}'.format(push_sdk_runtime_enabled)),
        make_check('sdk-play-runtime-enabled', play_sdk_runtime_enabled, 'enabled={}'.format(play_sdk_runtime_enabled)),
        make_check('sdk-push-runtime-files', positive(push_sdk_runtime.get('runtimeLogFiles')) and positive(push_sdk_runtime.get('metricsFiles')) and positive(push_sdk_runtime.get('alertsFiles')), json.dumps(push_sdk_runtime, sort_keys=True)),
        make_check('sdk-play-runtime-files', positive(play_sdk_runtime.get('runtimeLogFiles')) and positive(play_sdk_runtime.get('metricsFiles')) and positive(play_sdk_runtime.get('alertsFiles')), json.dumps(play_sdk_runtime, sort_keys=True)),
        make_check('sdk-play-rtcp-generated', positive(play_sdk_runtime.get('transportFeedbackCountMax')) and positive(play_sdk_runtime.get('receiverReportCountMax')), 'twcc={} rr={}'.format(play_sdk_runtime.get('transportFeedbackCountMax'), play_sdk_runtime.get('receiverReportCountMax'))),
        make_check('sdk-push-rtcp-counted', positive(push_sdk_runtime.get('transportFeedbackCountMax')) and positive(push_sdk_runtime.get('receiverReportCountMax')), 'twcc={} rr={}'.format(push_sdk_runtime.get('transportFeedbackCountMax'), push_sdk_runtime.get('receiverReportCountMax'))),
        make_check('harness-no-failure', not bool(harness_failure), harness_failure or 'ok'),
        make_check('runtime-no-unexpected-alerts', not bool(warning_lines), 'alerts={}'.format(len(warning_lines)) if warning_lines else 'ok'),
    ]

    if source_mode in ('synthetic', 'mp4-decode-loop', 'v4l2'):
        expected_encoder_mode = source_mode.replace('-', '_')
        if source_mode == 'synthetic':
            source_shape_ok = (
                last_encoder.get('width') == synthetic_width and
                last_encoder.get('height') == synthetic_height)
        elif source_mode == 'v4l2':
            source_shape_ok = (
                last_encoder.get('width') == v4l2_width and
                last_encoder.get('height') == v4l2_height)
        else:
            source_shape_ok = (
                positive(last_encoder.get('width')) and
                positive(last_encoder.get('height')))
        checks.extend([
            make_check('encoder-metrics-present', bool(encoder_metrics), 'samples={}'.format(len(encoder_metrics))),
            make_check('encoder-mode-selected', last_encoder.get('mode') == expected_encoder_mode and last_encoder.get('encoder') == 'x264', json.dumps(last_encoder, sort_keys=True)),
            make_check('encoder-au-output', positive(last_encoder.get('accessUnits')), 'accessUnits={}'.format(last_encoder.get('accessUnits'))),
            make_check('encoder-keyframe-output', positive(last_encoder.get('keyframes')), 'keyframes={}'.format(last_encoder.get('keyframes'))),
            make_check(
                'encoder-forced-keyframe-response',
                bool(
                    positive(last_encoder.get('forcedKeyframeRequests')) and
                    positive(last_encoder.get('forcedKeyframes')) and
                    is_number(last_encoder.get('maxForcedKeyframeDelayUs')) and
                    0 <= last_encoder.get('maxForcedKeyframeDelayUs') <= 1000000),
                'forcedKeyframeRequests={} forcedKeyframes={} maxForcedKeyframeDelayUs={}'.format(
                    last_encoder.get('forcedKeyframeRequests'),
                    last_encoder.get('forcedKeyframes'),
                    last_encoder.get('maxForcedKeyframeDelayUs'))),
            make_check('encoder-source-shape', source_shape_ok and positive(last_encoder.get('currentFps')) and positive(last_encoder.get('currentBitrateBps')), json.dumps(last_encoder, sort_keys=True)),
        ])
    if decode_qoe:
        checks.extend([
            make_check('qoe-metrics-present', bool(qoe_metrics), 'samples={}'.format(len(qoe_metrics))),
            make_check('qoe-decodes-frames', positive(last_qoe.get('decodedFrames')), 'decodedFrames={}'.format(last_qoe.get('decodedFrames'))),
            make_check('qoe-decode-errors-zero', last_qoe.get('decodeErrors', 0) == 0, 'decodeErrors={}'.format(last_qoe.get('decodeErrors'))),
            make_check('qoe-first-frame-observable', non_negative(last_qoe.get('firstFrameDelayUs')), 'firstFrameDelayUs={}'.format(last_qoe.get('firstFrameDelayUs'))),
        ])

    if case_name != 'baseline':
        if case_name == 'delay_100ms':
            rtt = [m['rttMs'] for m in push_metrics if m.get('rttMs') is not None]
            checks.append(make_check('weak-delay-rtt-observable', bool(rtt and max(rtt) >= 50), 'rttMax={}'.format(max(rtt) if rtt else None)))
        elif case_name.startswith('loss_'):
            checks.append(make_check('weak-loss-feedback-observable', bool(max_nack > 0 or max_retransmission > 0 or positive(push_rtcp_in)), 'nack={} retransmission={} pushRtcpIn={}'.format(max_nack, max_retransmission, push_rtcp_in)))
        elif case_name == 'bandwidth_600k':
            targets = [m['targetBps'] for m in push_metrics]
            dropped = bool(targets and min(targets) < max(targets) * 0.9)
            checks.append(make_check('weak-bandwidth-target-down', dropped, 'targetMin={} targetMax={}'.format(min(targets) if targets else None, max(targets) if targets else None)))
        elif case_name == 'drop_recover':
            targets = [m['targetBps'] for m in push_metrics]
            clear_ms = timing.get('clearEpochMs')
            post_clear = [
                m for m in push_metrics
                if isinstance(m.get('epochMs'), int) and isinstance(clear_ms, int) and m['epochMs'] >= clear_ms
            ]
            post_targets = [m['targetBps'] for m in post_clear]
            global_min = min(targets) if targets else None
            post_max = max(post_targets) if post_targets else None
            post_last = post_targets[-1] if post_targets else None
            recovered = bool(
                len(targets) >= 3 and
                global_min is not None and
                post_targets and
                (post_max > global_min or post_last > global_min)
            )
            checks.append(make_check(
                'weak-recovery-target-up',
                recovered,
                'targetMin={} postClearMax={} postClearLast={} postClearSamples={} recoverSeconds={}'.format(
                    global_min,
                    post_max,
                    post_last,
                    len(post_targets),
                    timing.get('recoverSeconds'))))
            if decode_qoe:
                recovery_delay_ms = recovery_qoe.get('postClearFirstDecodedDelayMs')
                recovery_decoded_delta = recovery_qoe.get('postClearDecodedFramesDelta')
                recovery_first_frame_ok = bool(
                    isinstance(recovery_delay_ms, (int, float)) and
                    0 <= recovery_delay_ms <= 15000 and
                    isinstance(recovery_decoded_delta, (int, float)) and
                    recovery_decoded_delta > 0)
                checks.append(make_check(
                    'qoe-recovery-first-frame-after-clear',
                    recovery_first_frame_ok,
                    'delayMs={} decodedDelta={} postClearSamples={} clearEpochMs={}'.format(
                        recovery_delay_ms,
                        recovery_decoded_delta,
                        recovery_qoe.get('postClearSamples'),
                        recovery_qoe.get('clearEpochMs'))))

    metrics = {
        'publishTwccExtId': publish_twcc,
        'selectedTwccExtId': selected_twcc,
        'pushedAu': pushed_au,
        'playOutputAu': output_au,
        'playRtpPackets': play_rtp,
        'playRtcpPacketsIn': play_rtcp_in,
        'pushRtcpFeedbackPacketsIn': push_rtcp_in,
        'pushRtcpFeedbackBytesIn': push_rtcp_bytes_in,
        'pushRtcpFeedbackFailures': push_rtcp_failures,
        'playRtcpPacketsOut': play_rtcp_out,
        'playRtcpBytesOut': play_rtcp_bytes_out,
        'playRtcpSendFailures': play_rtcp_send_failures,
        'rttMs': numbers([m['rttMs'] for m in push_metrics]),
        'senderLossFraction': numbers([m['loss'] for m in push_metrics]),
        'playLossQ8': numbers([m['lossQ8'] for m in play_metrics]),
        'targetBps': numbers([m['targetBps'] for m in push_metrics]),
        'finalTargetBps': numbers([m['finalTargetBps'] for m in push_metrics]),
        'droppedFrames': max_dropped_frames,
        'nack': max_nack,
        'pli': max_pli,
        'retransmission': max_retransmission,
        'pushMetricSamples': len(push_metrics),
        'playMetricSamples': len(play_metrics),
        'encoder': {
            'mode': last_encoder.get('mode'),
            'name': last_encoder.get('encoder'),
            'samples': len(encoder_metrics),
            'currentBitrateBps': last_encoder.get('currentBitrateBps'),
            'currentFps': last_encoder.get('currentFps'),
            'width': last_encoder.get('width'),
            'height': last_encoder.get('height'),
            'framesGenerated': last_encoder.get('framesGenerated'),
            'framesEncoded': last_encoder.get('framesEncoded'),
            'accessUnits': last_encoder.get('accessUnits'),
            'keyframes': last_encoder.get('keyframes'),
            'encoderRecreates': last_encoder.get('encoderRecreates'),
            'bitrateChanges': last_encoder.get('bitrateChanges'),
            'fpsChanges': last_encoder.get('fpsChanges'),
            'forcedKeyframeRequests': last_encoder.get('forcedKeyframeRequests'),
            'forcedKeyframes': last_encoder.get('forcedKeyframes'),
            'maxForcedKeyframeDelayUs': last_encoder.get('maxForcedKeyframeDelayUs'),
        },
        'qoe': {
            'enabled': decode_qoe,
            'samples': len(qoe_metrics),
            'accessUnitsIn': last_qoe.get('accessUnitsIn'),
            'keyframesIn': last_qoe.get('keyframesIn'),
            'decodedFrames': last_qoe.get('decodedFrames'),
            'decodeErrors': last_qoe.get('decodeErrors'),
            'freezeCount': last_qoe.get('freezeCount'),
            'firstFrameDelayUs': last_qoe.get('firstFrameDelayUs'),
            'maxFrameGapUs': last_qoe.get('maxFrameGapUs'),
            'outputFps': last_qoe.get('outputFps'),
            'width': last_qoe.get('width'),
            'height': last_qoe.get('height'),
            'recovery': recovery_qoe if case_name == 'drop_recover' else None,
        },
        'sdkRuntime': {
            'push': {
                'enabled': push_sdk_runtime_enabled,
                **push_sdk_runtime,
            },
            'play': {
                'enabled': play_sdk_runtime_enabled,
                **play_sdk_runtime,
            },
        },
        'timing': timing,
    }

    status = status_from_checks(checks)
    return {
        'name': case_name,
        'status': status,
        'networkCondition': condition,
        'checks': checks,
        'metrics': metrics,
        'alerts': {
            'count': len(warning_lines),
            'samples': warning_lines,
            'ignoredHarnessNoiseCount': len(ignored_warning_lines),
        },
        'artifacts': {
            'caseDir': case_dir,
            'sfuLog': os.path.join(case_dir, 'sfu.stdout.log'),
            'pushLog': os.path.join(case_dir, 'push', 'push.log'),
            'playLog': os.path.join(case_dir, 'play', 'play.log'),
            'readyz': os.path.join(case_dir, 'readyz.json'),
        },
    }

cases = [parse_case(case_name) for case_name in case_names]

attempted = [c for c in cases if c['status'] != 'SKIP']
failed_cases = [c for c in attempted if c['status'] == 'FAIL']
skipped_cases = [c for c in cases if c['status'] == 'SKIP']
baseline = next((c for c in cases if c['name'] == 'baseline'), None)
baseline_missing = baseline is None
baseline_skipped = bool(baseline and baseline.get('status') == 'SKIP')
baseline_skip_reason = (
    baseline.get('skipReason') if baseline_skipped else
    ('baseline case not requested' if baseline_missing else None))
baseline_unavailable = baseline_skipped or baseline_missing

qos_gate_evidence = {
    'baselineSelectedTwccExtId': baseline and baseline.get('metrics', {}).get('selectedTwccExtId'),
    'baselinePushRtcpFeedbackPacketsIn': baseline and baseline.get('metrics', {}).get('pushRtcpFeedbackPacketsIn'),
    'baselinePlayRtcpPacketsOut': baseline and baseline.get('metrics', {}).get('playRtcpPacketsOut'),
    'baselineSkipReason': baseline_skip_reason,
}
qos_gate_pass = bool(
    baseline and
    positive(baseline.get('metrics', {}).get('selectedTwccExtId')) and
    positive(baseline.get('metrics', {}).get('pushRtcpFeedbackPacketsIn')) and
    positive(baseline.get('metrics', {}).get('playRtcpPacketsOut')))

baseline_sdk = baseline.get('metrics', {}).get('sdkRuntime', {}) if baseline else {}
baseline_push_sdk = baseline_sdk.get('push', {})
baseline_play_sdk = baseline_sdk.get('play', {})
sdk_observability_evidence = {
    'baselinePushRuntimeEnabled': baseline_push_sdk.get('enabled'),
    'baselinePlayRuntimeEnabled': baseline_play_sdk.get('enabled'),
    'baselinePushRuntimeLogFiles': baseline_push_sdk.get('runtimeLogFiles'),
    'baselinePushMetricsFiles': baseline_push_sdk.get('metricsFiles'),
    'baselinePushAlertsFiles': baseline_push_sdk.get('alertsFiles'),
    'baselinePlayRuntimeLogFiles': baseline_play_sdk.get('runtimeLogFiles'),
    'baselinePlayMetricsFiles': baseline_play_sdk.get('metricsFiles'),
    'baselinePlayAlertsFiles': baseline_play_sdk.get('alertsFiles'),
    'baselinePushTransportFeedbackCountMax': baseline_push_sdk.get('transportFeedbackCountMax'),
    'baselinePushReceiverReportCountMax': baseline_push_sdk.get('receiverReportCountMax'),
    'baselinePlayTransportFeedbackCountMax': baseline_play_sdk.get('transportFeedbackCountMax'),
    'baselinePlayReceiverReportCountMax': baseline_play_sdk.get('receiverReportCountMax'),
    'baselineSkipReason': baseline_skip_reason,
}
sdk_observability_pass = bool(
    baseline and
    baseline_push_sdk.get('enabled') is True and
    baseline_play_sdk.get('enabled') is True and
    positive(baseline_push_sdk.get('runtimeLogFiles')) and
    positive(baseline_push_sdk.get('metricsFiles')) and
    positive(baseline_push_sdk.get('alertsFiles')) and
    positive(baseline_play_sdk.get('runtimeLogFiles')) and
    positive(baseline_play_sdk.get('metricsFiles')) and
    positive(baseline_play_sdk.get('alertsFiles')) and
    positive(baseline_push_sdk.get('transportFeedbackCountMax')) and
    positive(baseline_push_sdk.get('receiverReportCountMax')) and
    positive(baseline_play_sdk.get('transportFeedbackCountMax')) and
    positive(baseline_play_sdk.get('receiverReportCountMax')))

baseline_encoder = baseline.get('metrics', {}).get('encoder', {}) if baseline else {}
encoder_source_modes = ('synthetic', 'mp4-decode-loop', 'v4l2')
encoder_runtime_applicable = source_mode in encoder_source_modes
expected_encoder_mode = source_mode.replace('-', '_')
if source_mode == 'synthetic':
    baseline_encoder_shape_ok = (
        baseline_encoder.get('width') == synthetic_width and
        baseline_encoder.get('height') == synthetic_height)
elif source_mode == 'v4l2':
    baseline_encoder_shape_ok = (
        baseline_encoder.get('width') == v4l2_width and
        baseline_encoder.get('height') == v4l2_height)
else:
    baseline_encoder_shape_ok = (
        positive(baseline_encoder.get('width')) and
        positive(baseline_encoder.get('height')))
encoder_gate_evidence = {
    'sourceMode': source_mode,
    'skipReason': None if encoder_runtime_applicable else 'source mode does not exercise the x264 runtime encoder',
    'baselineEncoderMode': baseline_encoder.get('mode'),
    'baselineEncoderName': baseline_encoder.get('name'),
    'baselineEncoderSamples': baseline_encoder.get('samples'),
    'baselineAccessUnits': baseline_encoder.get('accessUnits'),
    'baselineKeyframes': baseline_encoder.get('keyframes'),
    'baselineForcedKeyframeRequests': baseline_encoder.get('forcedKeyframeRequests'),
    'baselineForcedKeyframes': baseline_encoder.get('forcedKeyframes'),
    'baselineMaxForcedKeyframeDelayUs': baseline_encoder.get('maxForcedKeyframeDelayUs'),
    'baselineCurrentBitrateBps': baseline_encoder.get('currentBitrateBps'),
    'baselineCurrentFps': baseline_encoder.get('currentFps'),
    'baselineWidth': baseline_encoder.get('width'),
    'baselineHeight': baseline_encoder.get('height'),
    'baselineSkipReason': baseline_skip_reason,
}
encoder_gate_pass = (
    False if not encoder_runtime_applicable else bool(
        baseline and
        baseline_encoder.get('mode') == expected_encoder_mode and
        baseline_encoder.get('name') == 'x264' and
        positive(baseline_encoder.get('samples')) and
        positive(baseline_encoder.get('accessUnits')) and
        positive(baseline_encoder.get('keyframes')) and
        positive(baseline_encoder.get('forcedKeyframeRequests')) and
        positive(baseline_encoder.get('forcedKeyframes')) and
        is_number(baseline_encoder.get('maxForcedKeyframeDelayUs')) and
        0 <= baseline_encoder.get('maxForcedKeyframeDelayUs') <= 1000000 and
        baseline_encoder_shape_ok and
        positive(baseline_encoder.get('currentFps')) and
        positive(baseline_encoder.get('currentBitrateBps'))))

baseline_qoe = baseline.get('metrics', {}).get('qoe', {}) if baseline else {}
qoe_gate_evidence = {
    'enabled': decode_qoe,
    'baselineSamples': baseline_qoe.get('samples'),
    'baselineDecodedFrames': baseline_qoe.get('decodedFrames'),
    'baselineDecodeErrors': baseline_qoe.get('decodeErrors'),
    'baselineFirstFrameDelayUs': baseline_qoe.get('firstFrameDelayUs'),
    'baselineFreezeCount': baseline_qoe.get('freezeCount'),
    'baselineOutputFps': baseline_qoe.get('outputFps'),
    'baselineWidth': baseline_qoe.get('width'),
    'baselineHeight': baseline_qoe.get('height'),
    'baselineSkipReason': baseline_skip_reason,
}
drop_recover_case = next((c for c in cases if c['name'] == 'drop_recover'), None)
drop_recover_qoe = drop_recover_case.get('metrics', {}).get('qoe', {}) if drop_recover_case else {}
drop_recover_recovery_qoe = drop_recover_qoe.get('recovery') or {}
drop_recover_recovery_delay_ms = drop_recover_recovery_qoe.get('postClearFirstDecodedDelayMs')
drop_recover_recovery_decoded_delta = drop_recover_recovery_qoe.get('postClearDecodedFramesDelta')
recovery_first_frame_evidence = {
    'enabled': decode_qoe,
    'dropRecoverStatus': drop_recover_case and drop_recover_case.get('status'),
    'clearEpochMs': drop_recover_recovery_qoe.get('clearEpochMs'),
    'preClearDecodedFrames': drop_recover_recovery_qoe.get('preClearDecodedFrames'),
    'postClearFirstDecodedEpochMs': drop_recover_recovery_qoe.get('postClearFirstDecodedEpochMs'),
    'postClearFirstDecodedDelayMs': drop_recover_recovery_delay_ms,
    'postClearDecodedFramesDelta': drop_recover_recovery_decoded_delta,
    'postClearSamples': drop_recover_recovery_qoe.get('postClearSamples'),
}
recovery_first_frame_pass = bool(
    decode_qoe and
    drop_recover_case and
    drop_recover_case.get('status') == 'PASS' and
    isinstance(drop_recover_recovery_delay_ms, (int, float)) and
    0 <= drop_recover_recovery_delay_ms <= 15000 and
    isinstance(drop_recover_recovery_decoded_delta, (int, float)) and
    drop_recover_recovery_decoded_delta > 0)
qoe_gate_pass = (
    True if not decode_qoe else bool(
        baseline and
        positive(baseline_qoe.get('samples')) and
        positive(baseline_qoe.get('decodedFrames')) and
        baseline_qoe.get('decodeErrors', 0) == 0 and
        non_negative(baseline_qoe.get('firstFrameDelayUs'))))

gates = {
    'qosMainline': {
        'status': 'SKIP' if baseline_unavailable else ('PASS' if qos_gate_pass else 'FAIL'),
        'requirements': [
            'consumer TWCC ext id > 0',
            'push RTCP feedback input > 0',
            'play RTCP feedback output > 0',
        ],
        'evidence': qos_gate_evidence,
    },
    'sdkRuntimeObservability': {
        'status': 'SKIP' if baseline_unavailable else ('PASS' if sdk_observability_pass else 'FAIL'),
        'requirements': [
            'SDK runtime log/metrics/alerts files enabled for push and play',
            'SDK push metrics count received TWCC and RR',
            'SDK play metrics count generated TWCC and RR',
        ],
        'evidence': sdk_observability_evidence,
    },
    'encoderRuntime': {
        'status': (
            'SKIP' if baseline_unavailable or not encoder_runtime_applicable
            else ('PASS' if encoder_gate_pass else 'FAIL')),
        'requirements': [
            'synthetic, MP4 decode-loop, or V4L2 source uses x264 realtime encoder when requested',
            'encoder metrics expose source shape, fps, bitrate, AU count, keyframe count',
            'SDK keyframe requests produce a forced IDR within 1 second',
        ],
        'evidence': encoder_gate_evidence,
    },
    'nativeDecodeQoe': {
        'status': 'SKIP' if baseline_unavailable else ('PASS' if qoe_gate_pass else 'FAIL'),
        'requirements': [
            'native play decode/QoE is enabled when requested',
            'FFmpeg decode produces frames and exposes first-frame/freeze/decode-error metrics',
        ],
        'evidence': qoe_gate_evidence,
    },
    'recoveryFirstFrame': {
        'status': (
            'SKIP' if not decode_qoe or not drop_recover_case or drop_recover_case.get('status') == 'SKIP'
            else ('PASS' if recovery_first_frame_pass else 'FAIL')),
        'requirements': [
            'drop_recover case is attempted with QoE decode enabled',
            'first decoded frame after netem clear is observed within 15 seconds',
        ],
        'evidence': recovery_first_frame_evidence,
    },
    'weakNetworkCoverage': {
        'status': 'PASS' if not skipped_cases and any(c['name'] != 'baseline' for c in attempted) else 'SKIP',
        'requirements': ['delay/loss/bandwidth/recovery cases attempted with tc netem'],
        'evidence': {
            'attemptedWeakCases': [c['name'] for c in attempted if c['name'] != 'baseline'],
            'skippedCases': [{'name': c['name'], 'reason': c.get('skipReason', '')} for c in skipped_cases],
        },
    },
}

blocking_gates = {
    name: gate for name, gate in gates.items()
    if not (name == 'encoderRuntime' and not encoder_runtime_applicable)
}

if failed_cases:
    overall = 'FAIL'
elif all(g['status'] == 'PASS' for g in blocking_gates.values()):
    overall = 'PASS'
else:
    overall = 'PARTIAL'

build_dir_abs = path_from_root(build_dir)
worker_bin_abs = path_from_root(worker_bin)
cmake_cache = parse_cmake_cache(os.path.join(build_dir_abs, 'CMakeCache.txt'))
webrtc_qos_sdk_cmake_dir = cmake_cache.get('WebRtcQosSdk_DIR', '')
webrtc_qos_sdk_dist = sdk_dist_from_cmake_dir(webrtc_qos_sdk_cmake_dir)
webrtc_qos_sdk_repo = os.path.abspath(os.path.join(webrtc_qos_sdk_dist, '..', '..')) if webrtc_qos_sdk_dist else ''
webrtc_qos_sdk_git_commit = ''
webrtc_qos_sdk_git_branch = ''
webrtc_qos_sdk_git_status = ''
if webrtc_qos_sdk_repo and os.path.isdir(os.path.join(webrtc_qos_sdk_repo, '.git')):
    webrtc_qos_sdk_git_commit = command_output(['git', '-C', webrtc_qos_sdk_repo, 'rev-parse', 'HEAD'])
    webrtc_qos_sdk_git_branch = command_output(['git', '-C', webrtc_qos_sdk_repo, 'branch', '--show-current'])
    webrtc_qos_sdk_git_status = command_output(['git', '-C', webrtc_qos_sdk_repo, 'status', '--short', '--untracked-files=no'])
git_commit = command_output(['git', '-C', root_dir, 'rev-parse', 'HEAD'])
git_branch = command_output(['git', '-C', root_dir, 'branch', '--show-current'])
failed_checks = collect_failed_checks(cases)

report = {
    'schemaVersion': 1,
    'generatedAt': datetime.datetime.utcnow().replace(microsecond=0).isoformat() + 'Z',
    'overallStatus': overall,
    'runDir': run_dir,
    'sourceRevision': {
        'gitCommit': git_commit,
        'gitBranch': git_branch,
    },
    'runConfig': {
        'durationSeconds': duration_seconds,
        'cases': case_names,
        'buildDir': build_dir,
        'buildDirAbs': build_dir_abs,
        'workerBin': worker_bin,
        'workerBinAbs': worker_bin_abs,
        'inputFile': input_file,
        'sourceMode': source_mode,
        'synthetic': {
            'width': synthetic_width,
            'height': synthetic_height,
            'fps': synthetic_fps,
        },
        'v4l2': {
            'device': v4l2_device,
            'width': v4l2_width,
            'height': v4l2_height,
            'fps': v4l2_fps,
            'inputFormat': v4l2_input_format or None,
        },
        'decodeQoe': decode_qoe,
        'enableNetem': enable_netem,
        'netemDev': netem_dev,
        'strict': strict,
    },
    'environment': {
        'platform': platform.platform(),
        'python': platform.python_version(),
        'kernel': command_output(['uname', '-a']),
        'tc': command_output(['bash', '-lc', 'command -v tc || true']),
        'ffmpeg': command_output(['bash', '-lc', 'ffmpeg -version 2>/dev/null | head -1 || true']),
        'cmakePrefixPath': cmake_cache.get('CMAKE_PREFIX_PATH', ''),
        'webrtcQosSdkDir': webrtc_qos_sdk_cmake_dir,
        'webrtcQosSdkDist': webrtc_qos_sdk_dist,
        'webrtcQosSdkRepo': webrtc_qos_sdk_repo,
        'webrtcQosSdkGitCommit': webrtc_qos_sdk_git_commit,
        'webrtcQosSdkGitBranch': webrtc_qos_sdk_git_branch,
        'webrtcQosSdkGitStatus': webrtc_qos_sdk_git_status,
    },
    'gates': gates,
    'summary': {
        'attemptedCases': len(attempted),
        'passedCases': len([c for c in attempted if c['status'] == 'PASS']),
        'failedCases': len(failed_cases),
        'skippedCases': len(skipped_cases),
        'failedChecks': len(failed_checks),
    },
    'failedChecks': failed_checks,
    'cases': cases,
}

os.makedirs(report_dir, exist_ok=True)
with open(report_json, 'w', encoding='utf-8') as f:
    json.dump(report, f, indent=2, sort_keys=True)
    f.write('\n')

def fmt(value):
    if value is None:
        return '-'
    if isinstance(value, float):
        if value.is_integer():
            return str(int(value))
        return '{:.2f}'.format(value)
    return str(value)

def fmt_triplet(metric):
    if not metric:
        return '-'
    return '{}/{}/{}'.format(fmt(metric.get('min')), fmt(metric.get('avg')), fmt(metric.get('max')))

lines = []
lines.append('# WebRTC QoS P2 Push/Play Smoke Report')
lines.append('')
lines.append('| Item | Value |')
lines.append('|---|---|')
lines.append('| Overall | `{}` |'.format(overall))
lines.append('| Generated At | `{}` |'.format(report['generatedAt']))
lines.append('| Git Commit | `{}` |'.format(git_commit))
lines.append('| SDK Dist | `{}` |'.format(webrtc_qos_sdk_dist or '-'))
lines.append('| Run Dir | `{}` |'.format(run_dir))
lines.append('| Duration Seconds | `{}` |'.format(duration_seconds))
lines.append('| Netem | `{}` on `{}` |'.format('enabled' if enable_netem else 'disabled', netem_dev))
lines.append('| Source Mode | `{}` |'.format(source_mode))
lines.append('| Decode QoE | `{}` |'.format('enabled' if decode_qoe else 'disabled'))
lines.append('| Input | `{}` |'.format(input_file))
lines.append('| Failed Checks | `{}` |'.format(len(failed_checks)))
if source_mode == 'v4l2':
    lines.append('| V4L2 Device | `{}` |'.format(v4l2_device))
lines.append('')
lines.append('## Gates')
lines.append('')
lines.append('| Gate | Status | Evidence |')
lines.append('|---|---:|---|')
for name, gate in gates.items():
    lines.append('| `{}` | `{}` | `{}` |'.format(name, gate['status'], json.dumps(gate['evidence'], ensure_ascii=False, sort_keys=True)))
lines.append('')
lines.append('## Cases')
lines.append('')
lines.append('| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |')
lines.append('|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|')
for case in cases:
    metrics = case.get('metrics', {})
    if case['status'] == 'SKIP':
        lines.append('| `{}` | `{}` | {} | - | - | - | - | - | - | - | - | - | - | - | - | - | {} |'.format(
            case['name'], case['status'], case.get('networkCondition', '-'), case.get('skipReason', '-')))
        continue
    twcc = metrics.get('selectedTwccExtId') or metrics.get('publishTwccExtId')
    notes = []
    for check in case.get('checks', []):
        if check.get('status') != 'PASS':
            notes.append('{}: {}'.format(check.get('name'), check.get('evidence')))
    if case.get('alerts', {}).get('count'):
        notes.append('alerts={}'.format(case['alerts']['count']))
    encoder = metrics.get('encoder') or {}
    encoder_summary = '-'
    if encoder.get('accessUnits') is not None:
        encoder_summary = '{}/{}'.format(fmt(encoder.get('accessUnits')), fmt(encoder.get('keyframes')))
    qoe = metrics.get('qoe') or {}
    qoe_summary = '-'
    if qoe.get('decodedFrames') is not None:
        qoe_summary = '{}/{}'.format(fmt(qoe.get('decodedFrames')), fmt(qoe.get('decodeErrors')))
    lines.append('| `{}` | `{}` | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {} | {}/{}/{} | {} |'.format(
        case['name'],
        case['status'],
        case.get('networkCondition', '-'),
        fmt(metrics.get('pushedAu')),
        fmt(metrics.get('playOutputAu')),
        qoe_summary,
        fmt(metrics.get('playRtpPackets')),
        fmt(metrics.get('pushRtcpFeedbackPacketsIn')),
        fmt(metrics.get('playRtcpPacketsOut')),
        fmt(twcc),
        encoder_summary,
        fmt_triplet(metrics.get('rttMs')),
        fmt_triplet(metrics.get('senderLossFraction')),
        fmt_triplet(metrics.get('targetBps')),
        fmt(metrics.get('droppedFrames')),
        fmt(metrics.get('nack')),
        fmt(metrics.get('pli')),
        fmt(metrics.get('retransmission')),
        '<br>'.join(notes) if notes else '-'))
lines.append('')
lines.append('## Artifacts')
lines.append('')
lines.append('- JSON report: `{}`'.format(report_json))
lines.append('- Runtime logs: `{}`'.format(run_dir))
lines.append('')
lines.append('## Interpretation')
lines.append('')
lines.append('- `PASS` case means SFU/push/play transport smoke met its checks.')
lines.append('- `SKIP` means the case was not verified and must not be counted as PASS.')
lines.append('- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.')
lines.append('- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.')
lines.append('- `encoderRuntime=PASS` means requested synthetic, MP4 decode-loop, or V4L2 x264 mode produced encoded H264 access units/keyframes, and SDK keyframe requests produced an IDR within 1 second.')
lines.append('- `encoderRuntime=SKIP` with `Source Mode=copy` means the report used MP4 H264 copy input and did not exercise the realtime x264 encoder; use synthetic, MP4 decode-loop, or V4L2 reports for encoder runtime evidence.')
lines.append('- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.')
lines.append('- `recoveryFirstFrame=PASS` means `drop_recover` observed decoded frame growth after netem clear within 15 seconds.')
lines.append('- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.')
lines.append('- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.')
lines.append('- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.')

with open(report_md, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines))
    f.write('\n')

print(report_json)
print(report_md)
if overall == 'FAIL':
    sys.exit(1)
if strict and overall != 'PASS':
    sys.exit(1)
PY
}

if [[ "$SOURCE_MODE" != "synthetic" && "$SOURCE_MODE" != "v4l2" ]]; then
	generate_input_if_needed
fi

IFS=',' read -r -a CASE_LIST <<<"$CASES"
if [[ "${#CASE_LIST[@]}" -eq 0 ]]; then
	echo "--cases must include at least one case" >&2
	exit 2
fi

if check_netem_ready; then
	NETEM_SKIP_REASON=""
fi

cleanup() {
	cleanup_netem
}
trap cleanup EXIT

index=0
for raw_case in "${CASE_LIST[@]}"; do
	case_name="$(printf '%s' "$raw_case" | xargs)"
	if [[ -z "$case_name" ]]; then
		continue
	fi
	case "$case_name" in
		baseline|delay_100ms|loss_2pct|loss_5pct|bandwidth_600k|drop_recover) ;;
		*) echo "unknown case: $case_name" >&2; exit 2 ;;
	esac
	echo "==> [webrtc-qos-p2] case=$case_name"
	run_case "$case_name" "$index"
	index=$((index + 1))
done

echo "==> [webrtc-qos-p2] render report"
render_report
