#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BUILD_DIR="$ROOT_DIR/build-webrtc-qos-plain"
WORKER_BIN="$ROOT_DIR/mediasoup-worker"
REPORT_DIR="$ROOT_DIR/docs/generated"
REPORT_BASENAME="webrtc-qos-plain-p3-thread-model-smoke-report"
ARTIFACT_ROOT="${TMPDIR:-/tmp}/webrtc-qos-plain-p3-thread-model-smoke"
DURATION_SECONDS=8
SOURCE_MODE="synthetic"
INPUT_FILE="$ROOT_DIR/tests/fixtures/media/test_sweep.mp4"
V4L2_DEVICE="/dev/video0"
V4L2_DEVICE_2="/dev/video1"
V4L2_WIDTH=640
V4L2_HEIGHT=360
V4L2_FPS=30
V4L2_INPUT_FORMAT=""
TRACK_COUNT=2
INJECTION_MODE="none"
INJECT_ENCODER_DELAY_MS=0
INJECT_SINK_DELAY_MS=0
NETWORK_MODE="none"
NETEM_DEV="lo"
ENABLE_NETEM=0
DECODE_QOE=1
BASE_PORT=34131
PLAY_PORT=44131
SERVER_IP="127.0.0.1"
MEDIA_IP="127.0.0.1"
STRICT=0

usage() {
	cat <<'EOF'
Usage:
  scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh [options]

Options:
  --duration-seconds <n>  Runtime for the two-track case. Default: 8.
  --source <mode>         Source mode: synthetic, mp4-decode-loop, or v4l2. Default: synthetic.
  --input <path>          MP4 input for --source mp4-decode-loop.
  --input-v4l2 <path>     V4L2 device for --source v4l2. Default: /dev/video0.
  --input-v4l2-2 <path>   Second V4L2 device for --source v4l2 --track-count 2. Default: /dev/video1.
  --v4l2-width <px>       V4L2 capture/encoder width. Default: 640.
  --v4l2-height <px>      V4L2 capture/encoder height. Default: 360.
  --v4l2-fps <n>          V4L2 capture/encoder fps. Default: 30.
  --v4l2-input-format <fmt>
                          Optional V4L2 input format, for example mjpeg.
  --track-count <n>       Number of video tracks/consumers: 1 or 2. Default: 2.
                          v4l2 supports 1 or 2; track-count 2 requires --input-v4l2-2.
  --injection <mode>      Injection mode: none, slow-encoder, or slow-sink. Default: none.
  --inject-encoder-delay-ms <n>
                          Per-AU delay in push source worker. Default selected by --injection.
  --inject-sink-delay-ms <n>
                          Per-AU delay in play sink worker. Default selected by --injection.
  --network <mode>        Network mode: none or weak. Default: none.
                          weak uses 5% loss + 600kbps netem and needs >=18s runtime.
  --netem-dev <dev>       tc netem device for --network weak. Default: lo.
  --enable-netem          Allow tc netem mutation for --network weak.
  --decode-qoe <0|1>      Enable native FFmpeg decode QoE in play sink workers. Default: 1.
  --build-dir <path>      Build directory containing mediasoup-sfu and plain clients.
  --worker-bin <path>     mediasoup-worker binary. Default: ./mediasoup-worker.
  --report-dir <path>     Report output directory. Default: docs/generated.
  --report-name <name>    Report basename without extension.
  --artifact-root <path>  Runtime logs/artifacts root.
  --base-port <port>      SFU signaling port. Default: 34131.
  --play-port <port>      Play UDP listen port. Default: 44131.
  --server-ip <ip>        SFU signaling IP. Default: 127.0.0.1.
  --media-ip <ip>         PlainTransport media remote IP. Default: 127.0.0.1.
  --strict                Return non-zero when report status is not PASS.
  -h, --help              Show this help.

This dynamic P3 thread-model smoke runs one push process with two x264 tracks
and one play process selecting two video consumers. By default weak-network
mode is safe: it records SKIP/PARTIAL unless --enable-netem is explicitly set.
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
		--duration-seconds) require_arg "$1" "${2:-}"; DURATION_SECONDS="$2"; shift 2 ;;
		--duration-seconds=*) DURATION_SECONDS="${1#*=}"; shift ;;
		--source) require_arg "$1" "${2:-}"; SOURCE_MODE="$2"; shift 2 ;;
		--source=*) SOURCE_MODE="${1#*=}"; shift ;;
		--input) require_arg "$1" "${2:-}"; INPUT_FILE="$2"; shift 2 ;;
		--input=*) INPUT_FILE="${1#*=}"; shift ;;
		--input-v4l2) require_arg "$1" "${2:-}"; V4L2_DEVICE="$2"; shift 2 ;;
		--input-v4l2=*) V4L2_DEVICE="${1#*=}"; shift ;;
		--input-v4l2-2) require_arg "$1" "${2:-}"; V4L2_DEVICE_2="$2"; shift 2 ;;
		--input-v4l2-2=*) V4L2_DEVICE_2="${1#*=}"; shift ;;
		--v4l2-width) require_arg "$1" "${2:-}"; V4L2_WIDTH="$2"; shift 2 ;;
		--v4l2-width=*) V4L2_WIDTH="${1#*=}"; shift ;;
		--v4l2-height) require_arg "$1" "${2:-}"; V4L2_HEIGHT="$2"; shift 2 ;;
		--v4l2-height=*) V4L2_HEIGHT="${1#*=}"; shift ;;
		--v4l2-fps) require_arg "$1" "${2:-}"; V4L2_FPS="$2"; shift 2 ;;
		--v4l2-fps=*) V4L2_FPS="${1#*=}"; shift ;;
		--v4l2-input-format) require_arg "$1" "${2:-}"; V4L2_INPUT_FORMAT="$2"; shift 2 ;;
		--v4l2-input-format=*) V4L2_INPUT_FORMAT="${1#*=}"; shift ;;
		--track-count) require_arg "$1" "${2:-}"; TRACK_COUNT="$2"; shift 2 ;;
		--track-count=*) TRACK_COUNT="${1#*=}"; shift ;;
		--injection) require_arg "$1" "${2:-}"; INJECTION_MODE="$2"; shift 2 ;;
		--injection=*) INJECTION_MODE="${1#*=}"; shift ;;
		--inject-encoder-delay-ms) require_arg "$1" "${2:-}"; INJECT_ENCODER_DELAY_MS="$2"; shift 2 ;;
		--inject-encoder-delay-ms=*) INJECT_ENCODER_DELAY_MS="${1#*=}"; shift ;;
		--inject-sink-delay-ms) require_arg "$1" "${2:-}"; INJECT_SINK_DELAY_MS="$2"; shift 2 ;;
		--inject-sink-delay-ms=*) INJECT_SINK_DELAY_MS="${1#*=}"; shift ;;
		--network) require_arg "$1" "${2:-}"; NETWORK_MODE="$2"; shift 2 ;;
		--network=*) NETWORK_MODE="${1#*=}"; shift ;;
		--netem-dev) require_arg "$1" "${2:-}"; NETEM_DEV="$2"; shift 2 ;;
		--netem-dev=*) NETEM_DEV="${1#*=}"; shift ;;
		--enable-netem) ENABLE_NETEM=1; shift ;;
		--decode-qoe) require_arg "$1" "${2:-}"; DECODE_QOE="$2"; shift 2 ;;
		--decode-qoe=*) DECODE_QOE="${1#*=}"; shift ;;
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
		--base-port) require_arg "$1" "${2:-}"; BASE_PORT="$2"; shift 2 ;;
		--base-port=*) BASE_PORT="${1#*=}"; shift ;;
		--play-port) require_arg "$1" "${2:-}"; PLAY_PORT="$2"; shift 2 ;;
		--play-port=*) PLAY_PORT="${1#*=}"; shift ;;
		--server-ip) require_arg "$1" "${2:-}"; SERVER_IP="$2"; shift 2 ;;
		--server-ip=*) SERVER_IP="${1#*=}"; shift ;;
		--media-ip) require_arg "$1" "${2:-}"; MEDIA_IP="$2"; shift 2 ;;
		--media-ip=*) MEDIA_IP="${1#*=}"; shift ;;
		--strict) STRICT=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
done

if ! [[ "$DURATION_SECONDS" =~ ^[0-9]+$ ]] || [[ "$DURATION_SECONDS" -lt 5 ]]; then
	echo "--duration-seconds must be an integer >= 5" >&2
	exit 2
fi
if [[ "$SOURCE_MODE" != "synthetic" && "$SOURCE_MODE" != "mp4-decode-loop" && "$SOURCE_MODE" != "v4l2" ]]; then
	echo "--source must be synthetic, mp4-decode-loop, or v4l2" >&2
	exit 2
fi
if ! [[ "$TRACK_COUNT" =~ ^[0-9]+$ ]] || [[ "$TRACK_COUNT" -lt 1 || "$TRACK_COUNT" -gt 2 ]]; then
	echo "--track-count must be 1 or 2" >&2
	exit 2
fi
if [[ "$SOURCE_MODE" == "v4l2" ]] && ! [[ "$V4L2_WIDTH" =~ ^[0-9]+$ && "$V4L2_HEIGHT" =~ ^[0-9]+$ && "$V4L2_FPS" =~ ^[0-9]+$ ]]; then
	echo "--v4l2-width/height/fps must be positive integers" >&2
	exit 2
fi
if [[ "$INJECTION_MODE" != "none" && "$INJECTION_MODE" != "slow-encoder" && "$INJECTION_MODE" != "slow-sink" ]]; then
	echo "--injection must be none, slow-encoder, or slow-sink" >&2
	exit 2
fi
if [[ "$NETWORK_MODE" != "none" && "$NETWORK_MODE" != "weak" ]]; then
	echo "--network must be none or weak" >&2
	exit 2
fi
if [[ "$NETWORK_MODE" == "weak" && "$INJECTION_MODE" != "none" ]]; then
	echo "--network weak cannot be combined with slow injection in the same case" >&2
	exit 2
fi
if [[ "$NETWORK_MODE" == "weak" && "$DURATION_SECONDS" -lt 18 ]]; then
	echo "--network weak requires --duration-seconds >= 18" >&2
	exit 2
fi
if ! [[ "$INJECT_ENCODER_DELAY_MS" =~ ^[0-9]+$ ]] || ! [[ "$INJECT_SINK_DELAY_MS" =~ ^[0-9]+$ ]]; then
	echo "--inject-*-delay-ms must be non-negative integers" >&2
	exit 2
fi
if [[ "$DECODE_QOE" != "0" && "$DECODE_QOE" != "1" ]]; then
	echo "--decode-qoe must be 0 or 1" >&2
	exit 2
fi
if [[ "$INJECTION_MODE" == "slow-encoder" && "$INJECT_ENCODER_DELAY_MS" == "0" ]]; then
	INJECT_ENCODER_DELAY_MS=80
fi
if [[ "$INJECTION_MODE" == "slow-sink" && "$INJECT_SINK_DELAY_MS" == "0" ]]; then
	INJECT_SINK_DELAY_MS=80
fi

SFU_BIN="$BUILD_DIR/mediasoup-sfu"
PUSH_BIN="$BUILD_DIR/webrtc-qos-plain-push-client"
PLAY_BIN="$BUILD_DIR/webrtc-qos-plain-play-client"

for file in "$SFU_BIN" "$PUSH_BIN" "$PLAY_BIN" "$WORKER_BIN"; do
	if [[ ! -x "$file" ]]; then
		echo "required executable not found: $file" >&2
		exit 2
	fi
done
if [[ "$SOURCE_MODE" == "mp4-decode-loop" && ! -f "$INPUT_FILE" ]]; then
	echo "MP4 input not found: $INPUT_FILE" >&2
	exit 2
fi

for tool in python3 curl setsid; do
	if ! command -v "$tool" >/dev/null 2>&1; then
		echo "required tool not found: $tool" >&2
		exit 2
	fi
done

RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_DIR="$ARTIFACT_ROOT/$RUN_ID"
CASE_NAME="two_track_synthetic"
SOURCE_MODE_REPORT="synthetic"
SOURCE_MODE_LOG="synthetic"
if [[ "$SOURCE_MODE" == "mp4-decode-loop" ]]; then
	CASE_NAME="two_track_decode_loop"
	SOURCE_MODE_REPORT="mp4_decode_loop"
	SOURCE_MODE_LOG="mp4_decode_loop"
elif [[ "$SOURCE_MODE" == "v4l2" ]]; then
	CASE_NAME="v4l2_single_camera"
	SOURCE_MODE_REPORT="v4l2"
	SOURCE_MODE_LOG="v4l2"
fi
if [[ "$INJECTION_MODE" == "slow-encoder" ]]; then
	CASE_NAME="slow_encoder_injection"
elif [[ "$INJECTION_MODE" == "slow-sink" ]]; then
	CASE_NAME="slow_play_sink_injection"
elif [[ "$NETWORK_MODE" == "weak" ]]; then
	CASE_NAME="weak_network_two_track"
fi
CASE_DIR="$RUN_DIR/$CASE_NAME"
mkdir -p "$CASE_DIR/push" "$CASE_DIR/play" "$REPORT_DIR"

now_ms() {
	date +%s%3N
}

json_escape() {
	local value="${1:-}"
	value="${value//\\/\\\\}"
	value="${value//\"/\\\"}"
	value="${value//$'\n'/ }"
	value="${value//$'\r'/ }"
	printf '%s' "$value"
}

network_condition() {
	if [[ "$NETWORK_MODE" == "weak" ]]; then
		printf '5%% loss + 600kbps rate limit on %s, then recovery' "$NETEM_DEV"
	else
		printf 'none'
	fi
}

write_environment_skip() {
	local reason="$1"
	printf '%s\n' "$reason" >"$CASE_DIR/SKIP_REASON"
	printf '%s\n' "$(network_condition)" >"$CASE_DIR/NETWORK_CONDITION"
	{
		printf '{\n'
		printf '  "case": "%s",\n' "$CASE_NAME"
		printf '  "sourceMode": "%s",\n' "$SOURCE_MODE_REPORT"
		printf '  "inputFile": "%s",\n' "$INPUT_FILE"
		printf '  "v4l2Device": "%s",\n' "$(json_escape "$V4L2_DEVICE")"
		printf '  "v4l2Device2": "%s",\n' "$(json_escape "$V4L2_DEVICE_2")"
		printf '  "v4l2Width": %s,\n' "$V4L2_WIDTH"
		printf '  "v4l2Height": %s,\n' "$V4L2_HEIGHT"
		printf '  "v4l2Fps": %s,\n' "$V4L2_FPS"
		printf '  "v4l2InputFormat": "%s",\n' "$(json_escape "$V4L2_INPUT_FORMAT")"
		printf '  "trackCount": %s,\n' "$TRACK_COUNT"
		printf '  "injectionMode": "%s",\n' "$INJECTION_MODE"
		printf '  "injectEncoderDelayMs": %s,\n' "$INJECT_ENCODER_DELAY_MS"
		printf '  "injectSinkDelayMs": %s,\n' "$INJECT_SINK_DELAY_MS"
		printf '  "networkMode": "%s",\n' "$NETWORK_MODE"
		printf '  "networkCondition": "%s",\n' "$(json_escape "$(network_condition)")"
		printf '  "netemDev": "%s",\n' "$NETEM_DEV"
		printf '  "netemEnabled": %s,\n' "$([[ "$ENABLE_NETEM" -eq 1 ]] && echo true || echo false)"
		printf '  "netemApplied": false,\n'
		printf '  "decodeQoe": %s,\n' "$([[ "$DECODE_QOE" -eq 1 ]] && echo true || echo false)"
		printf '  "skipReason": "%s",\n' "$(json_escape "$reason")"
		printf '  "durationSeconds": %s\n' "$DURATION_SECONDS"
		printf '}\n'
	} >"$CASE_DIR/case_timing.json"
}

cleanup_netem() {
	if [[ "$ENABLE_NETEM" -eq 1 ]] && command -v tc >/dev/null 2>&1; then
		tc qdisc del dev "$NETEM_DEV" root >/dev/null 2>&1 || true
	fi
}

NETEM_SKIP_REASON=""
check_netem_ready() {
	if [[ "$NETWORK_MODE" != "weak" ]]; then
		return 0
	fi
	if [[ "$ENABLE_NETEM" -ne 1 ]]; then
		NETEM_SKIP_REASON="netem disabled; set WEBRTC_QOS_P3_ENABLE_NETEM=1 or pass --enable-netem to run weak-network case"
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

apply_weak_netem() {
	local case_dir="$1"
	cleanup_netem
	printf 'apply_start epochMs=%s args="loss 5%% rate 600kbit" dev=%s\n' "$(now_ms)" "$NETEM_DEV" >>"$case_dir/netem.log"
	if ! tc qdisc add dev "$NETEM_DEV" root netem loss 5% rate 600kbit >>"$case_dir/netem.log" 2>&1; then
		return 1
	fi
	printf 'apply_done epochMs=%s args="loss 5%% rate 600kbit" dev=%s\n' "$(now_ms)" "$NETEM_DEV" >>"$case_dir/netem.log"
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

render_report() {
	local report_json="$REPORT_DIR/$REPORT_BASENAME.json"
	local report_md="$REPORT_DIR/$REPORT_BASENAME.md"
	P3_THREAD_MODEL_TRACK_COUNT="$TRACK_COUNT" P3_THREAD_MODEL_DECODE_QOE="$DECODE_QOE" python3 - "$ROOT_DIR" "$RUN_DIR" "$CASE_DIR" "$report_json" "$report_md" "$DURATION_SECONDS" "$STRICT" "$CASE_NAME" "$SOURCE_MODE_REPORT" "$SOURCE_MODE_LOG" "$INPUT_FILE" "$INJECTION_MODE" "$INJECT_ENCODER_DELAY_MS" "$INJECT_SINK_DELAY_MS" "$NETWORK_MODE" "$NETEM_DEV" "$ENABLE_NETEM" <<'PY'
import datetime
import json
import os
import platform
import re
import sys
import time

root_dir, run_dir, case_dir, report_json, report_md = sys.argv[1:6]
duration_seconds = int(sys.argv[6])
strict = sys.argv[7] == "1"
case_name = sys.argv[8]
source_mode = sys.argv[9]
source_mode_log = sys.argv[10]
input_file = sys.argv[11]
injection_mode = sys.argv[12]
inject_encoder_delay_ms = int(sys.argv[13])
inject_sink_delay_ms = int(sys.argv[14])
network_mode = sys.argv[15]
netem_dev = sys.argv[16]
enable_netem = sys.argv[17] == "1"
expected_track_count = int(os.environ.get("P3_THREAD_MODEL_TRACK_COUNT", "2"))
decode_qoe_enabled = bool(int(os.environ.get("P3_THREAD_MODEL_DECODE_QOE", "1")))

def read_file(path):
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except Exception:
        return ""

def read_json(path):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}

def read_primary_log(primary, fallback):
    text = read_file(primary)
    return text if text.strip() else read_file(fallback)

def last_match(pattern, text):
    matches = list(re.finditer(pattern, text))
    return matches[-1] if matches else None

def parse_bool(value):
    return str(value).lower() == "true"

def parse_log_epoch_ms(line):
    match = re.match(r"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.(\d{3})", line)
    if not match:
        return None
    try:
        dt = datetime.datetime.strptime(match.group(1), "%Y-%m-%d %H:%M:%S")
        return int(time.mktime(dt.timetuple()) * 1000) + int(match.group(2))
    except Exception:
        return None

def parse_push_metrics(text):
    rows = []
    pattern = re.compile(
        r"push_metrics pushedAu=(\d+) targetBps=(\d+) pacingBps=(\d+) "
        r"finalTargetBps=(\d+) rttMs=([0-9.]+) loss=([0-9.]+) "
        r"rtcpFeedbackPacketsIn=(\d+) rtcpFeedbackBytesIn=(\d+) "
        r"rtcpFeedbackFailures=(\d+) maxFps=([0-9.]+) requestKeyframe=(\w+) "
        r"droppedFrames=(\d+)"
    )
    for line in text.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        rows.append({
            "epochMs": parse_log_epoch_ms(line),
            "pushedAu": int(match.group(1)),
            "targetBps": int(match.group(2)),
            "pacingBps": int(match.group(3)),
            "finalTargetBps": int(match.group(4)),
            "rttMs": float(match.group(5)),
            "loss": float(match.group(6)),
            "rtcpFeedbackPacketsIn": int(match.group(7)),
            "rtcpFeedbackBytesIn": int(match.group(8)),
            "rtcpFeedbackFailures": int(match.group(9)),
            "maxFps": float(match.group(10)),
            "requestKeyframe": parse_bool(match.group(11)),
            "droppedFrames": int(match.group(12)),
        })
    return rows

def numbers(values):
    clean = [value for value in values if isinstance(value, (int, float))]
    if not clean:
        return {"count": 0, "min": None, "avg": None, "max": None, "last": None}
    return {
        "count": len(clean),
        "min": min(clean),
        "avg": sum(clean) / len(clean),
        "max": max(clean),
        "last": clean[-1],
    }

def make_status_check(name, status, evidence):
    return {"name": name, "status": status, "evidence": evidence}

def make_check(name, passed, evidence):
    return {"name": name, "status": "PASS" if passed else "FAIL", "evidence": evidence}

def ms_from_us(value):
    if value is None:
        return None
    return value / 1000.0

def max_present(values):
    clean = [value for value in values if isinstance(value, (int, float))]
    return max(clean) if clean else None

def max_counter_increment_gap_ms(rows, key):
    last_epoch = None
    last_value = None
    max_gap = None
    for row in rows:
        epoch = row.get("epochMs")
        value = row.get(key)
        if not isinstance(epoch, int) or not isinstance(value, int):
            continue
        if last_value is None:
            last_epoch = epoch
            last_value = value
            continue
        if value > last_value:
            if last_epoch is not None:
                gap = max(0, epoch - last_epoch)
                max_gap = gap if max_gap is None else max(max_gap, gap)
            last_epoch = epoch
            last_value = value
    return max_gap

def by_track(items):
    out = {}
    for item in items:
        track_id = item.get("trackId")
        if isinstance(track_id, int):
            out[track_id] = item
    return out

def make_thread(role, started, stopped, stop_reason, loop_gap_us, loop_iterations=None, track_id=None, sender_ssrc=None, heartbeat_us=None):
    loop_gap_ms = ms_from_us(loop_gap_us)
    return {
        "role": role,
        "tid": None,
        "trackId": track_id,
        "senderSsrc": sender_ssrc,
        "started": bool(started),
        "stopped": bool(stopped),
        "stopReason": stop_reason,
        "heartbeatGapMaxMs": loop_gap_ms,
        "heartbeatLastUs": heartbeat_us,
        "loopGapMaxMs": loop_gap_ms,
        "loopIterations": loop_iterations,
        "tidSource": "not logged yet",
    }

def make_queue(name, capacity, max_depth, max_age_ms, push_count, pop_count, drop_count, close_reason, track_id=None):
    return {
        "name": name,
        "trackId": track_id,
        "capacity": capacity,
        "maxDepth": max_depth,
        "maxAgeMs": max_age_ms,
        "pushCount": push_count,
        "popCount": pop_count,
        "dropCount": drop_count,
        "closeReason": close_reason,
    }

push_text = read_primary_log(
    os.path.join(case_dir, "push", "push.log"),
    os.path.join(case_dir, "push.stdout.log"),
)
play_text = read_primary_log(
    os.path.join(case_dir, "play", "play.log"),
    os.path.join(case_dir, "play.stdout.log"),
)
sfu_text = read_file(os.path.join(case_dir, "sfu.stdout.log"))
timing = read_json(os.path.join(case_dir, "case_timing.json"))
ready = read_json(os.path.join(case_dir, "readyz.json"))
harness_failure = read_file(os.path.join(case_dir, "HARNESS_FAILURE")).strip()
skip_reason = read_file(os.path.join(case_dir, "SKIP_REASON")).strip()
network_condition = read_file(os.path.join(case_dir, "NETWORK_CONDITION")).strip() or "none"

selected_consumers = re.findall(
    r"selected_consumer .*consumerId=([^ ]+) .*ssrc=(\d+) .*twccExtId=(\d+)",
    play_text,
)
push_start = last_match(r"push_runtime_started .*trackCount=(\d+)", push_text)
play_start = last_match(r"play_runtime_started .*trackCount=(\d+)", play_text)
push_stop = last_match(
    r"push_runtime_stopped pushedAu=(\d+) rtcpFeedbackPacketsIn=(\d+) "
    r"rtcpFeedbackBytesIn=(\d+) rtcpFeedbackFailures=(\d+) trackCount=(\d+) "
    r"queuedAu=(\d+) sdkQueueDroppedAu=(\d+) sdkQueueMaxDepth=(\d+) "
    r"sdkStarted=(\w+) sdkStopped=(\w+) sdkStopReason=([^ ]+) sdkFatalError=([^ ]*)",
    push_text,
)
play_stop = last_match(
    r"play_runtime_stopped rtpPackets=(\d+) rtcpPackets=(\d+) rtcpPacketsOut=(\d+) "
    r"rtcpBytesOut=(\d+) rtcpSendFailures=(\d+) outputAu=(\d+) packetInputFailures=(\d+) "
    r"sdkStarted=(\w+) sdkStopped=(\w+) sdkStopReason=([^ ]+) sdkFatalError=([^ ]*) "
    r"sdkLoopIterations=(\d+) sdkLoopGapMaxUs=(\d+) trackCount=(\d+) "
    r"sinkQueueDroppedAu=(\d+) sinkQueueMaxDepth=(\d+) sinkStarted=(\w+) sinkStopped=(\w+) "
    r"sinkStopReason=([^ ]+) sinkLoopIterations=(\d+) sinkLoopGapMaxUs=(\d+)"
    r"(?: sinkInjectedDelayCount=(\d+) sinkInjectedDelayTotalMs=(\d+))?",
    play_text,
)
push_sdk_stop = last_match(
    r"push_sdk_transport_thread_stopped stopReason=([^ ]+) tracks=(\d+) queuedAu=(\d+) "
    r"pushedAu=(\d+) droppedAu=(\d+) pushFailures=(\d+) rtcpFeedbackPacketsIn=(\d+) "
    r"rtcpFeedbackBytesIn=(\d+) rtcpFeedbackFailures=(\d+) loopIterations=(\d+) loopGapMaxUs=(\d+)",
    push_text,
)
play_sdk_stop = last_match(
    r"play_sdk_transport_thread_stopped stopReason=([^ ]+) tracks=(\d+) rtpPackets=(\d+) "
    r"rtcpPackets=(\d+) rtcpPacketsOut=(\d+) rtcpBytesOut=(\d+) rtcpSendFailures=(\d+) "
    r"packetInputFailures=(\d+) loopIterations=(\d+) loopGapMaxUs=(\d+)",
    play_text,
)
push_sdk_started = "push_sdk_transport_thread_started" in push_text
play_sdk_started = "play_sdk_transport_thread_started" in play_text
push_source_started = {int(match.group(1)) for match in re.finditer(r"push_track_source_worker_started trackId=(\d+)", push_text)}
raw_encode_started = {int(match.group(1)) for match in re.finditer(r"raw_frame_encode_worker_started trackId=(\d+)", push_text)}
v4l2_capture_started = {int(match.group(1)) for match in re.finditer(r"v4l2_capture_worker_started trackId=(\d+)", push_text)}
decoded_sink_started = {int(match.group(1)) for match in re.finditer(r"decoded_sink_worker_started trackId=(\d+)", play_text)}

push_tracks = []
for match in re.finditer(
    r"push_track_final trackId=(\d+) senderSsrc=(\d+) queuedAu=(\d+) pushedAu=(\d+) "
    r"droppedAu=(\d+) queueMaxDepth=(\d+) pushFailures=(\d+) "
    r"adaptationAvailable=(\w+) snapshotAvailable=(\w+)",
    push_text,
):
    push_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "queuedAu": int(match.group(3)),
        "pushedAu": int(match.group(4)),
        "droppedAu": int(match.group(5)),
        "queueMaxDepth": int(match.group(6)),
        "pushFailures": int(match.group(7)),
        "adaptationAvailable": parse_bool(match.group(8)),
        "snapshotAvailable": parse_bool(match.group(9)),
    })

encoder_tracks_by_id = {}
for match in re.finditer(
    r"encoder_track_metrics mode=(\w+) encoder=(\w+) trackId=(\d+) senderSsrc=(\d+) "
    r"queuedAu=(\d+) currentBitrateBps=(\d+) currentFps=(\d+) width=(\d+) height=(\d+) "
    r"framesGenerated=(\d+) framesEncoded=(\d+) accessUnits=(\d+) keyframes=(\d+) "
    r"encoderRecreates=(\d+) bitrateChanges=(\d+) fpsChanges=(\d+) forcedKeyframeRequests=(\d+) "
    r"forcedKeyframes=(\d+) maxForcedKeyframeDelayUs=(-?\d+) lastKeyframe=(\w+) "
    r"(?:injectedEncoderDelayCount=(\d+) injectedEncoderDelayTotalMs=(\d+) )?"
    r"workerLoopGapMaxUs=(\d+)",
    push_text,
):
    track_id = int(match.group(3))
    encoder_tracks_by_id[track_id] = {
        "mode": match.group(1),
        "encoder": match.group(2),
        "trackId": track_id,
        "senderSsrc": int(match.group(4)),
        "queuedAu": int(match.group(5)),
        "currentBitrateBps": int(match.group(6)),
        "currentFps": int(match.group(7)),
        "width": int(match.group(8)),
        "height": int(match.group(9)),
        "framesGenerated": int(match.group(10)),
        "framesEncoded": int(match.group(11)),
        "accessUnits": int(match.group(12)),
        "keyframes": int(match.group(13)),
        "encoderRecreates": int(match.group(14)),
        "bitrateChanges": int(match.group(15)),
        "fpsChanges": int(match.group(16)),
        "forcedKeyframeRequests": int(match.group(17)),
        "forcedKeyframes": int(match.group(18)),
        "maxForcedKeyframeDelayUs": int(match.group(19)),
        "lastKeyframe": parse_bool(match.group(20)),
        "injectedEncoderDelayCount": int(match.group(21) or 0),
        "injectedEncoderDelayTotalMs": int(match.group(22) or 0),
        "workerLoopGapMaxUs": int(match.group(23)),
    }
encoder_tracks = [encoder_tracks_by_id[key] for key in sorted(encoder_tracks_by_id)]

source_worker_tracks = []
for match in re.finditer(
    r"push_track_source_worker_stopped trackId=(\d+) senderSsrc=(\d+) mode=([^ ]+) "
    r"queuedAu=(\d+) enqueueFailures=(\d+) injectedEncoderDelayCount=(\d+) "
    r"injectedEncoderDelayTotalMs=(\d+) eof=(\w+) loopIterations=(\d+) loopGapMaxUs=(\d+) "
    r"rawQueueDroppedFrames=(\d+) rawQueueMaxDepth=(\d+) rawQueuePushedFrames=(\d+) "
    r"rawQueuePoppedFrames=(\d+) stopReason=([^ ]+) fatalError=([^ ]*)",
    push_text,
):
    source_worker_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "mode": match.group(3),
        "queuedAu": int(match.group(4)),
        "enqueueFailures": int(match.group(5)),
        "injectedEncoderDelayCount": int(match.group(6)),
        "injectedEncoderDelayTotalMs": int(match.group(7)),
        "eof": parse_bool(match.group(8)),
        "loopIterations": int(match.group(9)),
        "loopGapMaxUs": int(match.group(10)),
        "rawQueueDroppedFrames": int(match.group(11)),
        "rawQueueMaxDepth": int(match.group(12)),
        "rawQueuePushedFrames": int(match.group(13)),
        "rawQueuePoppedFrames": int(match.group(14)),
        "stopReason": match.group(15),
        "fatalError": match.group(16),
    })

raw_encode_tracks = []
for match in re.finditer(
    r"raw_frame_encode_worker_stopped trackId=(\d+) senderSsrc=(\d+) queuedAu=(\d+) "
    r"enqueueFailures=(\d+) rawQueueDroppedFrames=(\d+) rawQueueMaxDepth=(\d+) "
    r"injectedEncoderDelayCount=(\d+) injectedEncoderDelayTotalMs=(\d+) framesGenerated=(\d+) "
    r"framesEncoded=(\d+) accessUnits=(\d+) keyframes=(\d+) loopIterations=(\d+) "
    r"loopGapMaxUs=(\d+) stopReason=([^ ]+) fatalError=([^ ]*)",
    push_text,
):
    raw_encode_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "queuedAu": int(match.group(3)),
        "enqueueFailures": int(match.group(4)),
        "rawQueueDroppedFrames": int(match.group(5)),
        "rawQueueMaxDepth": int(match.group(6)),
        "injectedEncoderDelayCount": int(match.group(7)),
        "injectedEncoderDelayTotalMs": int(match.group(8)),
        "framesGenerated": int(match.group(9)),
        "framesEncoded": int(match.group(10)),
        "accessUnits": int(match.group(11)),
        "keyframes": int(match.group(12)),
        "loopIterations": int(match.group(13)),
        "loopGapMaxUs": int(match.group(14)),
        "stopReason": match.group(15),
        "fatalError": match.group(16),
    })

v4l2_capture_tracks = []
for match in re.finditer(
    r"v4l2_capture_worker_stopped trackId=(\d+) senderSsrc=(\d+) device=([^ ]+) "
    r"framesDecoded=(\d+) queuedFrames=(\d+) rawQueueDroppedFrames=(\d+) rawQueueMaxDepth=(\d+) "
    r"loopIterations=(\d+) loopGapMaxUs=(\d+) stopReason=([^ ]+) fatalError=([^ ]*)",
    push_text,
):
    v4l2_capture_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "device": match.group(3),
        "framesDecoded": int(match.group(4)),
        "queuedFrames": int(match.group(5)),
        "rawQueueDroppedFrames": int(match.group(6)),
        "rawQueueMaxDepth": int(match.group(7)),
        "loopIterations": int(match.group(8)),
        "loopGapMaxUs": int(match.group(9)),
        "stopReason": match.group(10),
        "fatalError": match.group(11),
    })

play_tracks = []
for match in re.finditer(
    r"play_track_final trackId=(\d+) senderSsrc=(\d+) snapshotAvailable=(\w+) "
    r"enqueuedAu=(\d+) outputAu=(\d+) nack=(\d+) pli=(\d+) droppedFrames=(\d+) "
    r"rttMs=(\d+) lossQ8=(\d+)",
    play_text,
):
    play_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "snapshotAvailable": parse_bool(match.group(3)),
        "enqueuedAu": int(match.group(4)),
        "outputAu": int(match.group(5)),
        "nack": int(match.group(6)),
        "pli": int(match.group(7)),
        "droppedFrames": int(match.group(8)),
        "rttMs": int(match.group(9)),
        "lossQ8": int(match.group(10)),
    })

play_track_qoe = []
for match in re.finditer(
    r"play_track_qoe_final trackId=(\d+) senderSsrc=(\d+) enabled=(\w+) "
    r"accessUnitsIn=(\d+) keyframesIn=(\d+) decodedFrames=(\d+) decodeErrors=(\d+) "
    r"freezeCount=(\d+) firstFrameDelayUs=(-?\d+) maxFrameGapUs=(\d+) "
    r"outputFps=([0-9.]+) width=(\d+) height=(\d+)",
    play_text,
):
    play_track_qoe.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "enabled": parse_bool(match.group(3)),
        "accessUnitsIn": int(match.group(4)),
        "keyframesIn": int(match.group(5)),
        "decodedFrames": int(match.group(6)),
        "decodeErrors": int(match.group(7)),
        "freezeCount": int(match.group(8)),
        "firstFrameDelayUs": int(match.group(9)),
        "maxFrameGapUs": int(match.group(10)),
        "outputFps": float(match.group(11)),
        "width": int(match.group(12)),
        "height": int(match.group(13)),
    })

decoded_sink_tracks = []
for match in re.finditer(
    r"decoded_sink_track_stopped trackId=(\d+) senderSsrc=(\d+) trackName=([^ ]+) "
    r"stopReason=([^ ]+) outputAu=(\d+) queuePushed=(\d+) queuePopped=(\d+) "
    r"queueDropped=(\d+) queueMaxDepth=(\d+) injectedSinkDelayCount=(\d+) "
    r"injectedSinkDelayTotalMs=(\d+) loopIterations=(\d+) loopGapMaxUs=(\d+) "
    r"lastHeartbeatUs=(\d+) qoeEnabled=(\w+) qoeAccessUnitsIn=(\d+) "
    r"qoeKeyframesIn=(\d+) qoeDecodedFrames=(\d+) qoeDecodeErrors=(\d+) "
    r"qoeFreezeCount=(\d+) qoeFirstFrameDelayUs=(-?\d+) qoeMaxFrameGapUs=(\d+) "
    r"qoeOutputFps=([0-9.]+) qoeWidth=(\d+) qoeHeight=(\d+)",
    play_text,
):
    decoded_sink_tracks.append({
        "trackId": int(match.group(1)),
        "senderSsrc": int(match.group(2)),
        "trackName": match.group(3),
        "stopReason": match.group(4),
        "outputAu": int(match.group(5)),
        "queuePushed": int(match.group(6)),
        "queuePopped": int(match.group(7)),
        "queueDropped": int(match.group(8)),
        "queueMaxDepth": int(match.group(9)),
        "injectedSinkDelayCount": int(match.group(10)),
        "injectedSinkDelayTotalMs": int(match.group(11)),
        "loopIterations": int(match.group(12)),
        "loopGapMaxUs": int(match.group(13)),
        "lastHeartbeatUs": int(match.group(14)),
        "qoeEnabled": parse_bool(match.group(15)),
        "qoeAccessUnitsIn": int(match.group(16)),
        "qoeKeyframesIn": int(match.group(17)),
        "qoeDecodedFrames": int(match.group(18)),
        "qoeDecodeErrors": int(match.group(19)),
        "qoeFreezeCount": int(match.group(20)),
        "qoeFirstFrameDelayUs": int(match.group(21)),
        "qoeMaxFrameGapUs": int(match.group(22)),
        "qoeOutputFps": float(match.group(23)),
        "qoeWidth": int(match.group(24)),
        "qoeHeight": int(match.group(25)),
    })

push_track_count = int(push_stop.group(5)) if push_stop else None
play_track_count = int(play_stop.group(14)) if play_stop else None
push_sdk_gap_us = None
play_sdk_gap_us = int(play_stop.group(13)) if play_stop else None
play_sink_gap_us = int(play_stop.group(21)) if play_stop else None
play_sink_injected_count = int(play_stop.group(22) or 0) if play_stop else 0
play_sink_injected_total_ms = int(play_stop.group(23) or 0) if play_stop else 0
push_rtcp_in = int(push_stop.group(2)) if push_stop else 0
push_rtcp_bytes_in = int(push_stop.group(3)) if push_stop else 0
push_rtcp_failures = int(push_stop.group(4)) if push_stop else None
play_rtp_in = int(play_stop.group(1)) if play_stop else 0
play_rtcp_in = int(play_stop.group(2)) if play_stop else 0
play_rtcp_out = int(play_stop.group(3)) if play_stop else 0
play_rtcp_bytes_out = int(play_stop.group(4)) if play_stop else 0
play_rtcp_failures = int(play_stop.group(5)) if play_stop else None
play_packet_failures = int(play_stop.group(7)) if play_stop else None

if push_stop:
    gap_match = last_match(r"push_metrics .*sdkLoopGapMaxUs=(\d+)", push_text)
    push_sdk_gap_us = int(gap_match.group(1)) if gap_match else None
push_metric_rows = parse_push_metrics(push_text)
target_bps_stats = numbers([row["targetBps"] for row in push_metric_rows])
final_target_bps_stats = numbers([row["finalTargetBps"] for row in push_metric_rows])
feedback_gap_max_ms = max_counter_increment_gap_ms(push_metric_rows, "rtcpFeedbackPacketsIn")
clear_epoch_ms = timing.get("clearEpochMs")
post_clear_target_bps = [
    row["targetBps"] for row in push_metric_rows
    if isinstance(row.get("epochMs"), int) and isinstance(clear_epoch_ms, int) and row["epochMs"] >= clear_epoch_ms
]
post_clear_target_stats = numbers(post_clear_target_bps)

warnings = []
ignored_warning_patterns = [
    re.compile(r"GeoRouter DB .* not found, falling back"),
    re.compile(r"worker pipe closed .* delegating to detached reaper"),
    re.compile(r"worker died .* worker process pipe closed"),
    re.compile(r"WorkerThread \d+ worker died .* attempting respawn"),
    re.compile(r"WorkerThread \d+ respawned worker"),
    re.compile(r"worker process \[pid:\d+\] exited with code 0"),
]
for line in "\n".join([push_text, play_text, sfu_text]).splitlines():
    lowered = line.lower()
    if any(token in lowered for token in ["[error]", "[warning]", " failed", "_failed", "timeout", "malformed"]):
        if any(pattern.search(line) for pattern in ignored_warning_patterns):
            continue
        warnings.append(line.strip())
warnings = warnings[:20]

encoder_injection_ok = (
    injection_mode != "slow-encoder" or
    (
        inject_encoder_delay_ms > 0 and
        len(encoder_tracks) == expected_track_count and
        all(item["injectedEncoderDelayCount"] > 0 for item in encoder_tracks) and
        all(item["injectedEncoderDelayTotalMs"] >= inject_encoder_delay_ms for item in encoder_tracks)
    )
)
sink_injection_ok = (
    injection_mode != "slow-sink" or
    (
        inject_sink_delay_ms > 0 and
        play_sink_injected_count > 0 and
        play_sink_injected_total_ms >= inject_sink_delay_ms
    )
)
per_track_sink_ok = (
    len(decoded_sink_tracks) == expected_track_count and
    all(item["stopReason"] == "queue_closed" for item in decoded_sink_tracks) and
    all(item["outputAu"] > 0 for item in decoded_sink_tracks) and
    all(item["loopIterations"] > 0 for item in decoded_sink_tracks)
)
per_track_qoe_ok = (
    not decode_qoe_enabled or
    (
        len(play_track_qoe) == expected_track_count and
        all(item["enabled"] for item in play_track_qoe) and
        all(item["decodedFrames"] > 0 for item in play_track_qoe) and
        all(item["decodeErrors"] == 0 for item in play_track_qoe)
    )
)
weak_network_ok = (
    network_mode != "weak" or
    (
        timing.get("netemApplied") is True and
        target_bps_stats["count"] >= 6 and
        target_bps_stats["min"] is not None and
        target_bps_stats["max"] is not None and
        target_bps_stats["min"] < target_bps_stats["max"] * 0.9 and
        post_clear_target_stats["count"] >= 2 and
        post_clear_target_stats["max"] is not None and
        post_clear_target_stats["max"] > target_bps_stats["min"] and
        push_rtcp_in > 0 and
        play_rtcp_out > 0
    )
)
sdk_gap_limit_us = 100000 if network_mode == "weak" or injection_mode != "none" else 50000
sdk_loop_gap_ok = (
    (push_sdk_gap_us is None or push_sdk_gap_us <= sdk_gap_limit_us) and
    (play_sdk_gap_us is not None and play_sdk_gap_us <= sdk_gap_limit_us)
)
feedback_gap_ok = (
    push_rtcp_in > 0 and
    (feedback_gap_max_ms is None or feedback_gap_max_ms <= 2000)
)

if skip_reason:
    skip_check_name = "v4l2-environment" if source_mode == "v4l2" else "weak-network-environment"
    checks = [
        make_status_check(
            skip_check_name,
            "SKIP",
            json.dumps({
                "reason": skip_reason,
                "sourceMode": source_mode,
                "trackCount": expected_track_count,
                "networkMode": network_mode,
                "enableNetem": enable_netem,
                "netemDev": netem_dev,
            }, sort_keys=True),
        )
    ]
    case_status = "SKIP"
else:
    checks = [
        make_check("sfu-readyz-ok", ready.get("ok") is True, json.dumps(ready, sort_keys=True)),
        make_check("no-harness-failure", not harness_failure, harness_failure or "ok"),
        make_check("selected-consumers", len(selected_consumers) == expected_track_count, "selectedConsumers={} expected={}".format(len(selected_consumers), expected_track_count)),
        make_check("selected-consumers-have-twcc", len(selected_consumers) == expected_track_count and all(int(item[2]) > 0 for item in selected_consumers), str(selected_consumers)),
        make_check("push-track-count", push_track_count == expected_track_count, "trackCount={} expected={}".format(push_track_count, expected_track_count)),
        make_check("play-track-count", play_track_count == expected_track_count, "trackCount={} expected={}".format(play_track_count, expected_track_count)),
        make_check("push-per-track-final", len(push_tracks) == expected_track_count, "tracks={} expected={}".format(len(push_tracks), expected_track_count)),
        make_check("play-per-track-final", len(play_tracks) == expected_track_count, "tracks={} expected={}".format(len(play_tracks), expected_track_count)),
        make_check("encoder-per-track-metrics", len(encoder_tracks) == expected_track_count, "tracks={} expected={}".format(len(encoder_tracks), expected_track_count)),
        make_check("encoder-source-mode", len(encoder_tracks) == expected_track_count and all(item["mode"] == source_mode_log for item in encoder_tracks), json.dumps(encoder_tracks, sort_keys=True)),
        make_check("push-each-track-pushed", len(push_tracks) == expected_track_count and all(item["pushedAu"] > 0 for item in push_tracks), json.dumps(push_tracks, sort_keys=True)),
        make_check("push-each-track-no-failures", len(push_tracks) == expected_track_count and all(item["pushFailures"] == 0 for item in push_tracks), json.dumps(push_tracks, sort_keys=True)),
        make_check("encoder-each-track-au-keyframe", len(encoder_tracks) == expected_track_count and all(item["accessUnits"] > 0 and item["keyframes"] > 0 for item in encoder_tracks), json.dumps(encoder_tracks, sort_keys=True)),
        make_check("slow-encoder-injection", encoder_injection_ok, json.dumps({"mode": injection_mode, "delayMs": inject_encoder_delay_ms, "tracks": encoder_tracks}, sort_keys=True)),
        make_check("play-each-track-output", len(play_tracks) == expected_track_count and all(item["enqueuedAu"] > 0 and item["outputAu"] > 0 for item in play_tracks), json.dumps(play_tracks, sort_keys=True)),
        make_check("per-track-sink-workers", per_track_sink_ok, json.dumps(decoded_sink_tracks, sort_keys=True)),
        make_check("per-track-qoe-workers", per_track_qoe_ok, json.dumps(play_track_qoe, sort_keys=True)),
        make_check("slow-sink-injection", sink_injection_ok, json.dumps({"mode": injection_mode, "delayMs": inject_sink_delay_ms, "count": play_sink_injected_count, "totalMs": play_sink_injected_total_ms}, sort_keys=True)),
        make_check("rtcp-feedback-loop", push_rtcp_in > 0 and play_rtcp_out > 0, "pushRtcpIn={} playRtcpOut={}".format(push_rtcp_in, play_rtcp_out)),
        make_check("feedback-gap", feedback_gap_ok, "feedbackGapMaxMs={} limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn".format(feedback_gap_max_ms)),
        make_check("rtcp-no-failures", push_rtcp_failures == 0 and play_rtcp_failures == 0 and play_packet_failures == 0, "pushRtcpFailures={} playRtcpFailures={} playPacketFailures={}".format(push_rtcp_failures, play_rtcp_failures, play_packet_failures)),
        make_check("sdk-loop-gap", sdk_loop_gap_ok, "pushSdkGapUs={} playSdkGapUs={} limitUs={}".format(push_sdk_gap_us, play_sdk_gap_us, sdk_gap_limit_us)),
        make_check("sink-loop-gap", play_sink_gap_us is not None and play_sink_gap_us <= 150000, "sinkGapUs={}".format(play_sink_gap_us)),
        make_check("runtime-no-unexpected-alerts", not warnings, "alerts={}".format(len(warnings))),
    ]
    if network_mode == "weak":
        checks.append(make_check("weak-network-target-down-and-recover", weak_network_ok, json.dumps({
            "networkMode": network_mode,
            "networkCondition": network_condition,
            "netemApplied": timing.get("netemApplied"),
            "targetBps": target_bps_stats,
            "postClearTargetBps": post_clear_target_stats,
            "samples": len(push_metric_rows),
            "pushRtcpIn": push_rtcp_in,
            "playRtcpOut": play_rtcp_out,
        }, sort_keys=True)))
    case_status = "PASS" if all(item["status"] == "PASS" for item in checks) else "FAIL"
source_workers_by_id = by_track(source_worker_tracks)
raw_encode_by_id = by_track(raw_encode_tracks)
v4l2_capture_by_id = by_track(v4l2_capture_tracks)
push_by_id = by_track(push_tracks)
encoder_by_id = by_track(encoder_tracks)
play_by_id = by_track(play_tracks)
sink_by_id = by_track(decoded_sink_tracks)
qoe_by_id = by_track(play_track_qoe)

threads = [
    make_thread(
        "push_sdk_transport",
        push_sdk_started or (push_stop and parse_bool(push_stop.group(9))),
        bool(push_sdk_stop) or (push_stop and parse_bool(push_stop.group(10))),
        push_sdk_stop.group(1) if push_sdk_stop else (push_stop.group(11) if push_stop else None),
        int(push_sdk_stop.group(11)) if push_sdk_stop else push_sdk_gap_us,
        int(push_sdk_stop.group(10)) if push_sdk_stop else None,
    ),
    make_thread(
        "play_sdk_transport",
        play_sdk_started or (play_stop and parse_bool(play_stop.group(9))),
        bool(play_sdk_stop) or (play_stop and parse_bool(play_stop.group(10))),
        play_sdk_stop.group(1) if play_sdk_stop else (play_stop.group(11) if play_stop else None),
        int(play_sdk_stop.group(10)) if play_sdk_stop else play_sdk_gap_us,
        int(play_sdk_stop.group(9)) if play_sdk_stop else (int(play_stop.group(12)) if play_stop else None),
    ),
]
for item in source_worker_tracks:
    threads.append(make_thread(
        "push_track_source_worker",
        item["trackId"] in push_source_started or item["queuedAu"] > 0,
        True,
        item["stopReason"],
        item["loopGapMaxUs"],
        item["loopIterations"],
        item["trackId"],
        item["senderSsrc"],
    ))
if not source_worker_tracks:
    for item in encoder_tracks:
        threads.append(make_thread(
            "push_track_source_worker",
            item["trackId"] in push_source_started,
            False,
            None,
            item.get("workerLoopGapMaxUs"),
            None,
            item["trackId"],
            item["senderSsrc"],
        ))
for item in raw_encode_tracks:
    threads.append(make_thread(
        "raw_frame_encode_worker",
        item["trackId"] in raw_encode_started,
        True,
        item["stopReason"],
        item["loopGapMaxUs"],
        item["loopIterations"],
        item["trackId"],
        item["senderSsrc"],
    ))
for item in v4l2_capture_tracks:
    threads.append(make_thread(
        "v4l2_capture_worker",
        item["trackId"] in v4l2_capture_started,
        True,
        item["stopReason"],
        item["loopGapMaxUs"],
        item["loopIterations"],
        item["trackId"],
        item["senderSsrc"],
    ))
for item in decoded_sink_tracks:
    threads.append(make_thread(
        "decoded_au_sink_worker",
        item["trackId"] in decoded_sink_started,
        True,
        item["stopReason"],
        item["loopGapMaxUs"],
        item["loopIterations"],
        item["trackId"],
        item["senderSsrc"],
        item["lastHeartbeatUs"],
    ))

queues = []
for item in push_tracks:
    queues.append(make_queue(
        "encoded-au-track-{}".format(item["trackId"]),
        128,
        item["queueMaxDepth"],
        None,
        item["queuedAu"],
        item["pushedAu"],
        item["droppedAu"],
        push_sdk_stop.group(1) if push_sdk_stop else (push_stop.group(11) if push_stop else None),
        item["trackId"],
    ))
for item in source_worker_tracks:
    if source_mode == "v4l2" or item["rawQueuePushedFrames"] > 0 or item["rawQueueMaxDepth"] > 0:
        queues.append(make_queue(
            "raw-frame-track-{}".format(item["trackId"]),
            3,
            item["rawQueueMaxDepth"],
            None,
            item["rawQueuePushedFrames"],
            item["rawQueuePoppedFrames"],
            item["rawQueueDroppedFrames"],
            item["stopReason"],
            item["trackId"],
        ))
for item in decoded_sink_tracks:
    queues.append(make_queue(
        "decoded-au-sink-track-{}".format(item["trackId"]),
        64,
        item["queueMaxDepth"],
        None,
        item["queuePushed"],
        item["queuePopped"],
        item["queueDropped"],
        item["stopReason"],
        item["trackId"],
    ))

tracks = []
track_ids = sorted(set(push_by_id) | set(encoder_by_id) | set(play_by_id) | set(sink_by_id) | set(qoe_by_id) | set(source_workers_by_id) | set(raw_encode_by_id) | set(v4l2_capture_by_id))
for track_id in track_ids:
    push_track = push_by_id.get(track_id, {})
    encoder_track = encoder_by_id.get(track_id, {})
    source_worker = source_workers_by_id.get(track_id, {})
    raw_encode = raw_encode_by_id.get(track_id, {})
    v4l2_capture = v4l2_capture_by_id.get(track_id, {})
    play_track = play_by_id.get(track_id, {})
    sink_track = sink_by_id.get(track_id, {})
    qoe_track = qoe_by_id.get(track_id, {})
    tracks.append({
        "trackId": track_id,
        "source": source_mode,
        "ssrc": push_track.get("senderSsrc") or encoder_track.get("senderSsrc") or source_worker.get("senderSsrc") or play_track.get("senderSsrc"),
        "capture": {
            "mode": source_worker.get("mode") or encoder_track.get("mode") or source_mode,
            "framesGenerated": encoder_track.get("framesGenerated") or raw_encode.get("framesGenerated"),
            "framesDecoded": v4l2_capture.get("framesDecoded"),
            "device": v4l2_capture.get("device"),
        },
        "encode": {
            "encoder": encoder_track.get("encoder"),
            "framesEncoded": encoder_track.get("framesEncoded") or raw_encode.get("framesEncoded"),
            "accessUnits": encoder_track.get("accessUnits") or raw_encode.get("accessUnits"),
            "keyframes": encoder_track.get("keyframes") or raw_encode.get("keyframes"),
            "bitrateChanges": encoder_track.get("bitrateChanges"),
            "forcedKeyframes": encoder_track.get("forcedKeyframes"),
            "loopGapMaxMs": ms_from_us(encoder_track.get("workerLoopGapMaxUs") or raw_encode.get("loopGapMaxUs")),
        },
        "queue": {
            "encodedMaxDepth": push_track.get("queueMaxDepth"),
            "encodedDroppedAu": push_track.get("droppedAu"),
            "rawMaxDepth": source_worker.get("rawQueueMaxDepth"),
            "rawDroppedFrames": source_worker.get("rawQueueDroppedFrames"),
            "sinkMaxDepth": sink_track.get("queueMaxDepth"),
            "sinkDroppedAu": sink_track.get("queueDropped"),
        },
        "sdk": {
            "queuedAu": push_track.get("queuedAu"),
            "pushedAu": push_track.get("pushedAu"),
            "pushFailures": push_track.get("pushFailures"),
            "adaptationAvailable": push_track.get("adaptationAvailable"),
            "snapshotAvailable": push_track.get("snapshotAvailable") or play_track.get("snapshotAvailable"),
            "playOutputAu": play_track.get("outputAu"),
            "playEnqueuedAu": play_track.get("enqueuedAu"),
        },
        "qoe": {
            "enabled": qoe_track.get("enabled"),
            "accessUnitsIn": qoe_track.get("accessUnitsIn"),
            "decodedFrames": qoe_track.get("decodedFrames"),
            "decodeErrors": qoe_track.get("decodeErrors"),
            "freezeCount": qoe_track.get("freezeCount"),
            "maxFrameGapMs": ms_from_us(qoe_track.get("maxFrameGapUs")),
            "outputFps": qoe_track.get("outputFps"),
        },
    })

sdk_process_gap_max_ms = ms_from_us(max_present([push_sdk_gap_us, play_sdk_gap_us]))
sdk_summary = {
    "processGapMaxMs": sdk_process_gap_max_ms,
    "pushProcessGapMaxMs": ms_from_us(push_sdk_gap_us),
    "playProcessGapMaxMs": ms_from_us(play_sdk_gap_us),
    "feedbackGapMaxMs": feedback_gap_max_ms,
    "feedbackGapSource": "push_metrics.rtcpFeedbackPacketsIn",
    "rtpCounters": {
        "playRtpPacketsIn": play_rtp_in,
    },
    "rtcpCounters": {
        "pushFeedbackPacketsIn": push_rtcp_in,
        "pushFeedbackBytesIn": push_rtcp_bytes_in,
        "pushFeedbackFailures": push_rtcp_failures,
        "playRtcpPacketsIn": play_rtcp_in,
        "playRtcpPacketsOut": play_rtcp_out,
        "playRtcpBytesOut": play_rtcp_bytes_out,
        "playRtcpSendFailures": play_rtcp_failures,
    },
    "targetBitrateBps": target_bps_stats,
    "finalTargetBitrateBps": final_target_bps_stats,
    "postClearTargetBitrateBps": post_clear_target_stats,
}
thread_safety = {
    "ownerViolations": None,
    "lockViolations": None,
    "lifetimeViolations": None,
    "dataViolations": None,
    "status": "covered-by-static-boundary-report",
    "requiredReport": "webrtc-qos-plain-thread-model-boundary-report",
}
if skip_reason:
    threads = []
    tracks = []
    queues = []
    sdk_summary["status"] = "skipped"
    sdk_summary["skipReason"] = skip_reason
case_report = {
    "name": case_name,
    "status": case_status,
    "sourceMode": source_mode,
    "trackCount": expected_track_count,
    "durationSeconds": duration_seconds,
    "checks": checks,
    "metrics": {
        "selectedConsumers": selected_consumers,
        "pushTrackCount": push_track_count,
        "playTrackCount": play_track_count,
        "pushRtcpFeedbackPacketsIn": push_rtcp_in,
        "playRtcpPacketsOut": play_rtcp_out,
        "feedbackGapMaxMs": feedback_gap_max_ms,
        "pushSdkLoopGapMaxUs": push_sdk_gap_us,
        "playSdkLoopGapMaxUs": play_sdk_gap_us,
        "playSinkLoopGapMaxUs": play_sink_gap_us,
        "injectionMode": injection_mode,
        "injectEncoderDelayMs": inject_encoder_delay_ms,
        "injectSinkDelayMs": inject_sink_delay_ms,
        "playSinkInjectedDelayCount": play_sink_injected_count,
        "playSinkInjectedDelayTotalMs": play_sink_injected_total_ms,
        "networkMode": network_mode,
        "networkCondition": network_condition,
        "netemDev": netem_dev,
        "enableNetem": enable_netem,
        "decodeQoe": decode_qoe_enabled,
        "skipReason": skip_reason or None,
        "pushMetricSamples": len(push_metric_rows),
        "targetBps": target_bps_stats,
        "finalTargetBps": final_target_bps_stats,
        "postClearTargetBps": post_clear_target_stats,
        "v4l2": timing.get("v4l2") or {
            "device": timing.get("v4l2Device"),
            "width": timing.get("v4l2Width"),
            "height": timing.get("v4l2Height"),
            "fps": timing.get("v4l2Fps"),
            "inputFormat": timing.get("v4l2InputFormat") or None,
        },
        "pushTracks": push_tracks,
        "encoderTracks": encoder_tracks,
        "sourceWorkerTracks": source_worker_tracks,
        "rawEncodeTracks": raw_encode_tracks,
        "v4l2CaptureTracks": v4l2_capture_tracks,
        "playTracks": play_tracks,
        "decodedSinkTracks": decoded_sink_tracks,
        "playTrackQoe": play_track_qoe,
        "decodeQoe": decode_qoe_enabled,
        "timing": timing,
        "inputFile": input_file if source_mode == "mp4_decode_loop" else None,
    },
    "alerts": warnings,
    "artifacts": {
        "caseDir": case_dir,
        "pushLog": os.path.join(case_dir, "push", "push.log"),
        "playLog": os.path.join(case_dir, "play", "play.log"),
        "sfuLog": os.path.join(case_dir, "sfu.stdout.log"),
    },
}

gates = {
    "multiTrackCoverage": case_status if case_status == "SKIP" else ("PASS" if case_status == "PASS" else "FAIL"),
    "playTrackCoverage": "SKIP" if case_status == "SKIP" else ("PASS" if any(item["name"] == "play-each-track-output" and item["status"] == "PASS" for item in checks) else "FAIL"),
    "sdkThreadHealth": "SKIP" if case_status == "SKIP" else ("PASS" if any(item["name"] == "sdk-loop-gap" and item["status"] == "PASS" for item in checks) else "FAIL"),
    "feedbackLoop": "SKIP" if case_status == "SKIP" else ("PASS" if any(item["name"] == "rtcp-feedback-loop" and item["status"] == "PASS" for item in checks) else "FAIL"),
    "slowEncoderIsolation": "SKIP" if case_status == "SKIP" else ("PASS" if any(item["name"] == "slow-encoder-injection" and item["status"] == "PASS" for item in checks) else "FAIL"),
    "slowSinkIsolation": "SKIP" if case_status == "SKIP" else ("PASS" if any(item["name"] == "slow-sink-injection" and item["status"] == "PASS" for item in checks) else "FAIL"),
    "perTrackPlaySinkQoe": "SKIP" if case_status == "SKIP" else ("PASS" if all(any(item["name"] == name and item["status"] == "PASS" for item in checks) for name in ("per-track-sink-workers", "per-track-qoe-workers")) else "FAIL"),
    "cameraRuntime": (
        "SKIP" if source_mode == "v4l2" and case_status == "SKIP" else
        "PASS" if source_mode != "v4l2" else
        "PASS" if case_status == "PASS" else
        "FAIL"
    ),
    "weakNetworkTwoTrack": (
        "SKIP" if case_status == "SKIP" and network_mode == "weak" else
        "PASS" if network_mode != "weak" else
        "PASS" if any(item["name"] == "weak-network-target-down-and-recover" and item["status"] == "PASS" for item in checks) else
        "FAIL"
    ),
}
if case_status == "SKIP":
    overall = "PARTIAL"
else:
    overall = "PASS" if case_status == "PASS" and all(value == "PASS" for value in gates.values()) else "FAIL"

report = {
    "overall": overall,
    "generatedAt": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "scope": "P3 thread-model dynamic smoke: {} push/play".format(case_name),
    "sourceMode": source_mode,
    "trackCount": expected_track_count,
    "environment": {
        "cpuCount": os.cpu_count(),
        "platform": platform.platform(),
        "hasNetem": timing.get("netemApplied") is True or (enable_netem and not skip_reason),
        "netemDev": netem_dev,
        "hasV4L2Devices": os.path.exists("/dev/video0"),
        "v4l2Devices": sorted([
            os.path.join("/dev", name) for name in os.listdir("/dev") if name.startswith("video")
        ]) if os.path.isdir("/dev") else [],
        "browserH264Supported": None,
        "skipReasons": [skip_reason] if skip_reason else [],
    },
    "cases": [case_report],
    "gates": gates,
    "threads": threads,
    "tracks": tracks,
    "queues": queues,
    "sdk": sdk_summary,
    "threadSafety": thread_safety,
    "skipReasons": [skip_reason] if skip_reason else [],
    "artifacts": {
        "runDir": run_dir,
        "reportJson": report_json,
        "reportMarkdown": report_md,
    },
    "remainingP3Acceptance": [
        "two_track_synthetic",
        "two_track_decode_loop",
        "slow_encoder_injection",
        "slow_play_sink_injection",
        "weak_network_two_track",
        "v4l2_single_camera",
        "v4l2_two_camera",
    ],
}
report["remainingP3Acceptance"] = [
    item for item in report["remainingP3Acceptance"] if item != case_name
] if case_status != "SKIP" else report["remainingP3Acceptance"]

os.makedirs(os.path.dirname(report_json), exist_ok=True)
with open(report_json, "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
    f.write("\n")

lines = [
    "# WebRTC QoS Plain P3 Thread Model Smoke Report",
    "",
    "- overall: `{}`".format(overall),
    "- generatedAt: `{}`".format(report["generatedAt"]),
    "- scope: {}".format(report["scope"]),
    "- artifactRoot: `{}`".format(run_dir),
    "- networkMode: `{}`".format(network_mode),
    "- networkCondition: {}".format(network_condition),
    "- skipReason: `{}`".format(skip_reason or ""),
    "- schemaSummary: threads=`{}` tracks=`{}` queues=`{}`".format(len(threads), len(tracks), len(queues)),
    "- sdkProcessGapMaxMs: `{}`".format(sdk_summary["processGapMaxMs"]),
    "- feedbackGapMaxMs: `{}`".format(sdk_summary["feedbackGapMaxMs"]),
    "",
    "## Gates",
    "",
    "| gate | status |",
    "|---|---|",
]
for key, value in gates.items():
    lines.append("| `{}` | `{}` |".format(key, value))
lines.extend([
    "",
    "## Case",
    "",
    "| case | status | duration | push tracks | play tracks | push RTCP in | play RTCP out | targetBps min/avg/max | post-clear target max |",
    "|---|---|---:|---:|---:|---:|---:|---:|---:|",
    "| `{}` | `{}` | `{}` | `{}` | `{}` | `{}` | `{}` | `{}/{:.0f}/{}` | `{}` |".format(
        case_name,
        case_status,
        duration_seconds,
        case_report["metrics"]["pushTrackCount"],
        case_report["metrics"]["playTrackCount"],
        case_report["metrics"]["pushRtcpFeedbackPacketsIn"],
        case_report["metrics"]["playRtcpPacketsOut"],
        target_bps_stats["min"],
        target_bps_stats["avg"] or 0,
        target_bps_stats["max"],
        post_clear_target_stats["max"],
    ),
    "",
    "## Checks",
    "",
    "| check | status | evidence |",
    "|---|---|---|",
])
for check in checks:
    evidence = str(check["evidence"]).replace("|", "\\|")
    if len(evidence) > 220:
        evidence = evidence[:217] + "..."
    lines.append("| `{}` | `{}` | {} |".format(check["name"], check["status"], evidence))
lines.extend([
    "",
    "## Remaining P3 Acceptance",
    "",
])
for item in report["remainingP3Acceptance"]:
    lines.append("- `{}`".format(item))
with open(report_md, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

print("P3 thread-model smoke report: {}".format(report_md))
if overall == "FAIL":
    sys.exit(1)
if strict and overall != "PASS":
    sys.exit(1)
sys.exit(0)
PY
}

sfu_pid=""
play_pid=""
push_pid=""
cleanup() {
	terminate_group "$push_pid"
	terminate_group "$play_pid"
	terminate_group "$sfu_pid"
	cleanup_netem
}
trap cleanup EXIT

room="p3-thread-model-${RUN_ID}"
push_peer="p3-push"
play_peer="p3-play"

printf '%s\n' "$(network_condition)" >"$CASE_DIR/NETWORK_CONDITION"
if [[ "$NETWORK_MODE" == "weak" ]] && ! check_netem_ready; then
	write_environment_skip "$NETEM_SKIP_REASON"
	render_report
	exit 0
fi
if [[ "$SOURCE_MODE" == "v4l2" && ! -e "$V4L2_DEVICE" ]]; then
	write_environment_skip "v4l2 device not found: $V4L2_DEVICE"
	render_report
	exit 0
fi
if [[ "$SOURCE_MODE" == "v4l2" && "$TRACK_COUNT" -ge 2 && ! -e "$V4L2_DEVICE_2" ]]; then
	write_environment_skip "second v4l2 device not found: $V4L2_DEVICE_2"
	render_report
	exit 0
fi

start_epoch_ms="$(now_ms)"
setsid "$SFU_BIN" \
	--nodaemon \
	--port="$BASE_PORT" \
	--workers=1 \
	--workerThreads=1 \
	--listenIp="$SERVER_IP" \
	--announcedIp="$SERVER_IP" \
	--workerBin="$WORKER_BIN" \
	--noRedisRequired \
	>"$CASE_DIR/sfu.stdout.log" 2>&1 &
sfu_pid=$!

if ! wait_ready "$BASE_PORT" "$CASE_DIR/readyz.json"; then
	printf 'readyz timeout\n' >"$CASE_DIR/HARNESS_FAILURE"
	render_report
	exit 1
fi

setsid "$PLAY_BIN" \
	--server-ip="$SERVER_IP" \
	--server-port="$BASE_PORT" \
	--room="$room" \
	--peer="$play_peer" \
	--listen-ip="$SERVER_IP" \
	--advertise-ip="$SERVER_IP" \
	--listen-port="$PLAY_PORT" \
	--output-null=true \
	--decode-qoe="$DECODE_QOE" \
	--wait-consumer-timeout-ms=15000 \
	--video-consumer-count="$TRACK_COUNT" \
	--media-remote-ip="$MEDIA_IP" \
	--inject-sink-delay-ms="$INJECT_SINK_DELAY_MS" \
	--log-dir="$CASE_DIR/play" \
	>"$CASE_DIR/play.stdout.log" 2>&1 &
play_pid=$!

sleep 0.7

push_args=(
	"$PUSH_BIN"
	--server-ip="$SERVER_IP"
	--server-port="$BASE_PORT"
	--room="$room"
	--peer="$push_peer"
	--media-remote-ip="$MEDIA_IP"
	--inject-encoder-delay-ms="$INJECT_ENCODER_DELAY_MS"
	--log-dir="$CASE_DIR/push"
)
if [[ "$SOURCE_MODE" == "v4l2" ]]; then
	push_args+=(--track="id=cam0,ssrc=11111111,weight=100,source=v4l2,device=$V4L2_DEVICE,width=$V4L2_WIDTH,height=$V4L2_HEIGHT,fps=$V4L2_FPS")
	if [[ "$TRACK_COUNT" -ge 2 ]]; then
		push_args+=(--track="id=cam1,ssrc=22222222,weight=100,source=v4l2,device=$V4L2_DEVICE_2,width=$V4L2_WIDTH,height=$V4L2_HEIGHT,fps=$V4L2_FPS")
	fi
else
	push_args+=(--track=id=cam0,ssrc=11111111,weight=100)
	if [[ "$TRACK_COUNT" -ge 2 ]]; then
		push_args+=(--track=id=cam1,ssrc=22222222,weight=100)
	fi
fi
if [[ "$SOURCE_MODE" == "synthetic" ]]; then
	push_args+=(
		--input-synthetic=true
		--encoder=x264
		--synthetic-width=320
		--synthetic-height=180
		--synthetic-fps=15
	)
elif [[ "$SOURCE_MODE" == "mp4-decode-loop" ]]; then
	push_args+=(
		--input="$INPUT_FILE"
		--input-decode-loop=true
		--loop-input=true
		--encoder=x264
	)
else
	push_args+=(
		--encoder=x264
		--v4l2-width="$V4L2_WIDTH"
		--v4l2-height="$V4L2_HEIGHT"
		--v4l2-fps="$V4L2_FPS"
	)
	if [[ -n "$V4L2_INPUT_FORMAT" ]]; then
		push_args+=(--v4l2-input-format="$V4L2_INPUT_FORMAT")
	fi
fi
setsid "${push_args[@]}" >"$CASE_DIR/push.stdout.log" 2>&1 &
push_pid=$!

apply_epoch_ms=""
clear_epoch_ms=""
netem_applied=0
pre_seconds=0
weak_seconds=0
recover_seconds=0
if [[ "$NETWORK_MODE" == "weak" ]]; then
	pre_seconds=4
	weak_seconds=8
	recover_seconds=$((DURATION_SECONDS - pre_seconds - weak_seconds))
	if [[ "$recover_seconds" -lt 6 ]]; then
		recover_seconds=6
	fi
	sleep "$pre_seconds"
	if apply_weak_netem "$CASE_DIR"; then
		netem_applied=1
		apply_epoch_ms="$(now_ms)"
	else
		printf 'netem apply failed\n' >"$CASE_DIR/HARNESS_FAILURE"
	fi
	sleep "$weak_seconds"
	cleanup_netem
	clear_epoch_ms="$(now_ms)"
	printf 'clear epochMs=%s dev=%s\n' "$clear_epoch_ms" "$NETEM_DEV" >>"$CASE_DIR/netem.log"
	sleep "$recover_seconds"
else
	sleep "$DURATION_SECONDS"
fi
end_epoch_ms="$(now_ms)"

{
	printf '{\n'
	printf '  "case": "%s",\n' "$CASE_NAME"
	printf '  "sourceMode": "%s",\n' "$SOURCE_MODE_REPORT"
	printf '  "inputFile": "%s",\n' "$INPUT_FILE"
	printf '  "v4l2Device": "%s",\n' "$V4L2_DEVICE"
	printf '  "v4l2Device2": "%s",\n' "$V4L2_DEVICE_2"
	printf '  "v4l2Width": %s,\n' "$V4L2_WIDTH"
	printf '  "v4l2Height": %s,\n' "$V4L2_HEIGHT"
	printf '  "v4l2Fps": %s,\n' "$V4L2_FPS"
	printf '  "v4l2InputFormat": "%s",\n' "$(json_escape "$V4L2_INPUT_FORMAT")"
	printf '  "trackCount": %s,\n' "$TRACK_COUNT"
	printf '  "injectionMode": "%s",\n' "$INJECTION_MODE"
	printf '  "injectEncoderDelayMs": %s,\n' "$INJECT_ENCODER_DELAY_MS"
	printf '  "injectSinkDelayMs": %s,\n' "$INJECT_SINK_DELAY_MS"
	printf '  "networkMode": "%s",\n' "$NETWORK_MODE"
	printf '  "networkCondition": "%s",\n' "$(json_escape "$(network_condition)")"
	printf '  "netemDev": "%s",\n' "$NETEM_DEV"
	printf '  "netemEnabled": %s,\n' "$([[ "$ENABLE_NETEM" -eq 1 ]] && echo true || echo false)"
	printf '  "netemApplied": %s,\n' "$([[ "$netem_applied" -eq 1 ]] && echo true || echo false)"
	printf '  "decodeQoe": %s,\n' "$([[ "$DECODE_QOE" -eq 1 ]] && echo true || echo false)"
	printf '  "preSeconds": %s,\n' "$pre_seconds"
	printf '  "weakSeconds": %s,\n' "$weak_seconds"
	printf '  "recoverSeconds": %s,\n' "$recover_seconds"
	printf '  "durationSeconds": %s,\n' "$DURATION_SECONDS"
	printf '  "startEpochMs": %s,\n' "$start_epoch_ms"
	if [[ -n "$apply_epoch_ms" ]]; then
		printf '  "applyEpochMs": %s,\n' "$apply_epoch_ms"
	else
		printf '  "applyEpochMs": null,\n'
	fi
	if [[ -n "$clear_epoch_ms" ]]; then
		printf '  "clearEpochMs": %s,\n' "$clear_epoch_ms"
	else
		printf '  "clearEpochMs": null,\n'
	fi
	printf '  "endEpochMs": %s\n' "$end_epoch_ms"
	printf '}\n'
} >"$CASE_DIR/case_timing.json"

cleanup
trap - EXIT
render_report
