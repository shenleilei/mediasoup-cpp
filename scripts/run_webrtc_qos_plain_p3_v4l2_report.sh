#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BUILD_DIR="$ROOT_DIR/build-webrtc-qos-plain"
WORKER_BIN="$ROOT_DIR/mediasoup-worker"
REPORT_DIR="$ROOT_DIR/docs/generated"
REPORT_BASENAME="webrtc-qos-plain-p3-thread-model-v4l2-report"
ARTIFACT_ROOT="${TMPDIR:-/tmp}/webrtc-qos-plain-p3-thread-model-v4l2"
V4L2_DEVICE0="/dev/video0"
V4L2_DEVICE1="/dev/video1"
V4L2_WIDTH=640
V4L2_HEIGHT=360
V4L2_FPS=30
V4L2_INPUT_FORMAT=""
DURATION_SECONDS=8
BASE_PORT=34181
PLAY_PORT=44181
SERVER_IP="127.0.0.1"
MEDIA_IP="127.0.0.1"
STRICT=0

usage() {
	cat <<'EOF'
Usage:
  scripts/run_webrtc_qos_plain_p3_v4l2_report.sh [options]

Options:
  --input-v4l2 <path>       First V4L2 device. Default: /dev/video0.
  --input-v4l2-2 <path>     Second V4L2 device. Default: /dev/video1.
  --v4l2-width <px>         V4L2 capture/encoder width. Default: 640.
  --v4l2-height <px>        V4L2 capture/encoder height. Default: 360.
  --v4l2-fps <n>            V4L2 capture/encoder fps. Default: 30.
  --v4l2-input-format <fmt> Optional V4L2 input format, for example mjpeg.
  --duration-seconds <n>    Single-camera runtime when device exists. Default: 8.
  --build-dir <path>        Build directory containing mediasoup-sfu and plain clients.
  --worker-bin <path>       mediasoup-worker binary. Default: ./mediasoup-worker.
  --report-dir <path>       Report output directory. Default: docs/generated.
  --report-name <name>      Report basename without extension.
  --artifact-root <path>    Runtime logs/artifacts root.
  --base-port <port>        SFU signaling port for single-camera case. Default: 34181.
  --play-port <port>        Play UDP listen port for single-camera case. Default: 44181.
  --server-ip <ip>          SFU signaling IP. Default: 127.0.0.1.
  --media-ip <ip>           PlainTransport media remote IP. Default: 127.0.0.1.
  --strict                  Return non-zero when report status is not PASS.
  -h, --help                Show this help.

This P3 report never falls back to synthetic. Missing V4L2 devices are reported
as SKIP/PARTIAL with evidence. When both devices exist, the two-camera case runs
the same smoke with per-track V4L2 device configuration.
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
		--input-v4l2) require_arg "$1" "${2:-}"; V4L2_DEVICE0="$2"; shift 2 ;;
		--input-v4l2=*) V4L2_DEVICE0="${1#*=}"; shift ;;
		--input-v4l2-2) require_arg "$1" "${2:-}"; V4L2_DEVICE1="$2"; shift 2 ;;
		--input-v4l2-2=*) V4L2_DEVICE1="${1#*=}"; shift ;;
		--v4l2-width) require_arg "$1" "${2:-}"; V4L2_WIDTH="$2"; shift 2 ;;
		--v4l2-width=*) V4L2_WIDTH="${1#*=}"; shift ;;
		--v4l2-height) require_arg "$1" "${2:-}"; V4L2_HEIGHT="$2"; shift 2 ;;
		--v4l2-height=*) V4L2_HEIGHT="${1#*=}"; shift ;;
		--v4l2-fps) require_arg "$1" "${2:-}"; V4L2_FPS="$2"; shift 2 ;;
		--v4l2-fps=*) V4L2_FPS="${1#*=}"; shift ;;
		--v4l2-input-format) require_arg "$1" "${2:-}"; V4L2_INPUT_FORMAT="$2"; shift 2 ;;
		--v4l2-input-format=*) V4L2_INPUT_FORMAT="${1#*=}"; shift ;;
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
if ! [[ "$V4L2_WIDTH" =~ ^[0-9]+$ && "$V4L2_HEIGHT" =~ ^[0-9]+$ && "$V4L2_FPS" =~ ^[0-9]+$ ]]; then
	echo "--v4l2-width/height/fps must be positive integers" >&2
	exit 2
fi

mkdir -p "$REPORT_DIR" "$ARTIFACT_ROOT"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_DIR="$ARTIFACT_ROOT/$RUN_ID"
SINGLE_REPORT="$RUN_DIR/single-camera.json"
SINGLE_MARKDOWN="$RUN_DIR/single-camera.md"
TWO_REPORT="$RUN_DIR/two-camera.json"
TWO_MARKDOWN="$RUN_DIR/two-camera.md"
mkdir -p "$RUN_DIR"

SMOKE_ARGS=(
	"$ROOT_DIR/scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh"
	--build-dir "$BUILD_DIR"
	--worker-bin "$WORKER_BIN"
	--source v4l2
	--track-count 1
	--input-v4l2 "$V4L2_DEVICE0"
	--v4l2-width "$V4L2_WIDTH"
	--v4l2-height "$V4L2_HEIGHT"
	--v4l2-fps "$V4L2_FPS"
	--duration-seconds "$DURATION_SECONDS"
	--base-port "$BASE_PORT"
	--play-port "$PLAY_PORT"
	--server-ip "$SERVER_IP"
	--media-ip "$MEDIA_IP"
	--report-dir "$RUN_DIR"
	--report-name single-camera
	--artifact-root "$RUN_DIR/artifacts"
)
if [[ -n "$V4L2_INPUT_FORMAT" ]]; then
	SMOKE_ARGS+=(--v4l2-input-format "$V4L2_INPUT_FORMAT")
fi

set +e
"${SMOKE_ARGS[@]}"
smoke_status=$?
set -e

two_smoke_status=0
if [[ -e "$V4L2_DEVICE0" && -e "$V4L2_DEVICE1" ]]; then
	TWO_SMOKE_ARGS=(
		"$ROOT_DIR/scripts/run_webrtc_qos_plain_p3_thread_model_smoke.sh"
		--build-dir "$BUILD_DIR"
		--worker-bin "$WORKER_BIN"
		--source v4l2
		--track-count 2
		--input-v4l2 "$V4L2_DEVICE0"
		--input-v4l2-2 "$V4L2_DEVICE1"
		--v4l2-width "$V4L2_WIDTH"
		--v4l2-height "$V4L2_HEIGHT"
		--v4l2-fps "$V4L2_FPS"
		--duration-seconds "$DURATION_SECONDS"
		--base-port "$((BASE_PORT + 10))"
		--play-port "$((PLAY_PORT + 10))"
		--server-ip "$SERVER_IP"
		--media-ip "$MEDIA_IP"
		--report-dir "$RUN_DIR"
		--report-name two-camera
		--artifact-root "$RUN_DIR/artifacts"
	)
	if [[ -n "$V4L2_INPUT_FORMAT" ]]; then
		TWO_SMOKE_ARGS+=(--v4l2-input-format "$V4L2_INPUT_FORMAT")
	fi
	set +e
	"${TWO_SMOKE_ARGS[@]}"
	two_smoke_status=$?
	set -e
fi

REPORT_JSON="$REPORT_DIR/$REPORT_BASENAME.json"
REPORT_MD="$REPORT_DIR/$REPORT_BASENAME.md"
python3 - "$ROOT_DIR" "$RUN_DIR" "$SINGLE_REPORT" "$TWO_REPORT" "$REPORT_JSON" "$REPORT_MD" "$REPORT_BASENAME" "$V4L2_DEVICE0" "$V4L2_DEVICE1" "$V4L2_WIDTH" "$V4L2_HEIGHT" "$V4L2_FPS" "$V4L2_INPUT_FORMAT" "$DURATION_SECONDS" "$smoke_status" "$two_smoke_status" "$STRICT" <<'PY'
import datetime
import json
import os
import platform
import sys
from pathlib import Path

(
    root_dir,
    run_dir,
    single_report_path,
    two_report_path,
    report_json,
    report_md,
    report_basename,
    device0,
    device1,
    width,
    height,
    fps,
    input_format,
    duration_seconds,
    smoke_status,
    two_smoke_status,
    strict,
) = sys.argv[1:18]
duration_seconds = int(duration_seconds)
smoke_status = int(smoke_status)
two_smoke_status = int(two_smoke_status)
strict = strict == "1"

def read_json(path):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}

def make_check(name, status, evidence):
    return {"name": name, "status": status, "evidence": evidence}

def collect_child_list(key, *reports):
    out = []
    for report in reports:
        for item in report.get(key) or []:
            out.append(item)
    return out

def collect_child_sdk(*reports):
    out = {}
    for name, report in reports:
        sdk = report.get("sdk")
        if sdk:
            out[name] = sdk
    return out

def v4l2_devices():
    dev = Path("/dev")
    if not dev.is_dir():
        return []
    return sorted(str(dev / name) for name in os.listdir(dev) if name.startswith("video"))

single_report = read_json(single_report_path)
single_case = (single_report.get("cases") or [{}])[0]
single_status = single_case.get("status") or ("FAIL" if smoke_status else "SKIP")
if smoke_status != 0 and single_status != "PASS":
    single_status = "FAIL"
single_metrics = single_case.get("metrics") or {}
single_skip = single_metrics.get("skipReason") or (single_report.get("environment") or {}).get("skipReasons", [None])[0]

single_case_out = {
    "name": "v4l2_single_camera",
    "status": single_status,
    "device": device0,
    "durationSeconds": duration_seconds,
    "sourceReport": single_report_path,
    "checks": single_case.get("checks") or [],
    "metrics": single_metrics,
    "skipReason": single_skip,
}

devices = v4l2_devices()
two_report = read_json(two_report_path)
two_report_case = (two_report.get("cases") or [{}])[0]
if Path(device0).exists() and Path(device1).exists() and two_report:
    two_status = two_report_case.get("status") or ("FAIL" if two_smoke_status else "PASS")
    two_skip = (two_report_case.get("metrics") or {}).get("skipReason")
    if two_smoke_status != 0 and two_status != "PASS":
        two_status = "FAIL"
elif not Path(device1).exists():
    two_status = "SKIP"
    two_skip = "second v4l2 device not found: {}".format(device1)
elif not Path(device0).exists():
    two_status = "SKIP"
    two_skip = "first v4l2 device not found: {}".format(device0)
else:
    two_status = "FAIL"
    two_skip = "two-camera report missing despite both V4L2 devices existing"

two_case = {
    "name": "v4l2_two_camera",
    "status": two_status,
    "devices": [device0, device1],
    "durationSeconds": duration_seconds if Path(device0).exists() and Path(device1).exists() else 0,
    "sourceReport": two_report_path,
    "checks": two_report_case.get("checks") or [
        make_check("v4l2-two-camera-environment", two_status, two_skip or "ok")
    ],
    "metrics": two_report_case.get("metrics") or {
        "deviceCount": len([item for item in [device0, device1] if Path(item).exists()]),
        "runtimeSupported": Path(device0).exists() and Path(device1).exists(),
    },
    "skipReason": two_skip,
}

has_fail = single_status == "FAIL" or two_status == "FAIL"
has_skip = single_status == "SKIP" or two_status == "SKIP"
if has_fail:
    overall = "FAIL"
elif has_skip:
    overall = "PARTIAL"
else:
    overall = "PASS"

gates = {
    "v4l2SingleCamera": single_status,
    "v4l2TwoCamera": two_status,
    "cameraRuntime": "FAIL" if has_fail else ("SKIP" if has_skip else "PASS"),
    "noSyntheticFallback": "PASS",
}

skip_reasons = []
for reason in [single_skip, two_skip]:
    if reason:
        skip_reasons.append(reason)
threads = collect_child_list("threads", single_report, two_report)
tracks = collect_child_list("tracks", single_report, two_report)
queues = collect_child_list("queues", single_report, two_report)
sdk_summary = collect_child_sdk(("singleCamera", single_report), ("twoCamera", two_report))
thread_safety = {
    "ownerViolations": None,
    "lockViolations": None,
    "lifetimeViolations": None,
    "dataViolations": None,
    "status": "covered-by-static-boundary-report",
    "requiredReport": "webrtc-qos-plain-thread-model-boundary-report",
}

report = {
    "overall": overall,
    "generatedAt": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "scope": "P3 thread-model V4L2 camera acceptance",
    "sourceMode": "v4l2",
    "trackCount": 2,
    "environment": {
        "cpuCount": os.cpu_count(),
        "platform": platform.platform(),
        "hasNetem": None,
        "hasV4L2Devices": bool(devices),
        "v4l2Devices": devices,
        "browserH264Supported": None,
        "skipReasons": skip_reasons,
    },
    "runConfig": {
        "sourceMode": "v4l2",
        "durationSeconds": duration_seconds,
        "v4l2": {
            "device0": device0,
            "device1": device1,
            "width": int(width),
            "height": int(height),
            "fps": int(fps),
            "inputFormat": input_format or None,
        },
    },
    "cases": [single_case_out, two_case],
    "gates": gates,
    "threads": threads,
    "tracks": tracks,
    "queues": queues,
    "sdk": sdk_summary,
    "threadSafety": thread_safety,
    "skipReasons": skip_reasons,
    "artifacts": {
        "runDir": run_dir,
        "singleCameraReport": single_report_path,
        "singleCameraMarkdown": str(Path(single_report_path).with_suffix(".md")),
        "twoCameraReport": two_report_path,
        "twoCameraMarkdown": str(Path(two_report_path).with_suffix(".md")),
        "reportJson": report_json,
        "reportMarkdown": report_md,
    },
    "remainingP3Acceptance": [
        item for item in [
            "two_track_synthetic",
            "two_track_decode_loop",
            "slow_encoder_injection",
            "slow_play_sink_injection",
            "weak_network_two_track",
            "v4l2_single_camera",
            "v4l2_two_camera",
        ]
        if not (
            (item == "v4l2_single_camera" and single_status == "PASS") or
            (item == "v4l2_two_camera" and two_status == "PASS")
        )
    ],
}

Path(report_json).parent.mkdir(parents=True, exist_ok=True)
with open(report_json, "w", encoding="utf-8") as f:
    json.dump(report, f, ensure_ascii=False, indent=2)
    f.write("\n")

lines = [
    "# WebRTC QoS Plain P3 V4L2 Report",
    "",
    "- overall: `{}`".format(overall),
    "- generatedAt: `{}`".format(report["generatedAt"]),
    "- artifactRoot: `{}`".format(run_dir),
    "- devices: `{}`".format(", ".join(devices) if devices else ""),
    "- schemaSummary: threads=`{}` tracks=`{}` queues=`{}`".format(len(threads), len(tracks), len(queues)),
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
    "## Cases",
    "",
    "| case | status | device(s) | skipReason |",
    "|---|---|---|---|",
])
for case in report["cases"]:
    device_text = ", ".join(case.get("devices") or [case.get("device", "")])
    reason = str(case.get("skipReason") or "").replace("|", "\\|")
    lines.append("| `{}` | `{}` | `{}` | {} |".format(case["name"], case["status"], device_text, reason))
lines.extend([
    "",
    "## Acceptance Rules",
    "",
    "- Missing camera devices are `SKIP/PARTIAL`, never PASS.",
    "- This report never substitutes synthetic input for V4L2.",
    "- Two-camera V4L2 uses per-track `--track source=v4l2,device=...` when both devices exist.",
    "",
    "## Remaining P3 Acceptance",
    "",
])
for item in report["remainingP3Acceptance"]:
    lines.append("- `{}`".format(item))
with open(report_md, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")

print("P3 V4L2 report: {}".format(report_md))
if overall == "FAIL":
    sys.exit(1)
if strict and overall != "PASS":
    sys.exit(1)
sys.exit(0)
PY
