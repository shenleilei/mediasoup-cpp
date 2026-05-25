#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

BUILD_DIR="$ROOT_DIR/build-webrtc-qos-plain"
WORKER_BIN="$ROOT_DIR/mediasoup-worker"
REPORT_DIR="$ROOT_DIR/docs/generated"
ARTIFACT_ROOT="${TMPDIR:-/tmp}/webrtc-qos-plain-p2-acceptance"
CMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH:-}"
DEFAULT_SDK_PREFIX="$ROOT_DIR/../webrtc_qos_sdk/dist/linux-x86_64"
RUN_SMOKE=0
ENABLE_NETEM=0
RUN_BROWSER=0
RUN_V4L2=1
STRICT=1
PREFLIGHT_NETEM_ONLY=0
JOBS="${JOBS:-$(nproc)}"

usage() {
	cat <<'EOF'
Usage:
  scripts/run_webrtc_qos_plain_p2_acceptance.sh [options]

Options:
  --build-dir <path>          Build directory. Default: build-webrtc-qos-plain.
  --worker-bin <path>         mediasoup-worker binary. Default: ./mediasoup-worker.
  --report-dir <path>         Report output directory. Default: docs/generated.
  --artifact-root <path>      Runtime artifact root. Default: /tmp/webrtc-qos-plain-p2-acceptance.
  --cmake-prefix-path <path>  SDK dist prefix for cmake configure. Defaults to
                              CMAKE_PREFIX_PATH, then ../webrtc_qos_sdk/dist/linux-x86_64 if present.
  --jobs <n>                  Build parallelism. Default: nproc.
  --run-smoke                 Re-run P2 smoke reports instead of only verifying existing reports. Requires --enable-netem.
  --enable-netem              Allow tc netem mutation for weak-network smoke cases.
  --run-browser               Re-run browser receiver smoke. Without this, existing browser report is verified.
  --skip-v4l2                 Skip V4L2 smoke re-run when --run-smoke is used.
  --no-strict                 Allow smoke script PARTIAL exits where reports still explain SKIP.
  --preflight-netem-only      Only verify that formal weak-network refresh can use tc netem, then exit.
  -h, --help                  Show this help.

Default mode is safe and fast: configure/build required targets, run unit,
ORTC, PlainPublish integration and static gates, then verify the generated P2
reports already in docs/generated. Use --run-smoke --enable-netem for a formal
refresh of the main weak-network, recovery, MP4 decode-loop and V4L2 reports.
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
		--build-dir) require_arg "$1" "${2:-}"; BUILD_DIR="$2"; shift 2 ;;
		--build-dir=*) BUILD_DIR="${1#*=}"; shift ;;
		--worker-bin) require_arg "$1" "${2:-}"; WORKER_BIN="$2"; shift 2 ;;
		--worker-bin=*) WORKER_BIN="${1#*=}"; shift ;;
		--report-dir) require_arg "$1" "${2:-}"; REPORT_DIR="$2"; shift 2 ;;
		--report-dir=*) REPORT_DIR="${1#*=}"; shift ;;
		--artifact-root) require_arg "$1" "${2:-}"; ARTIFACT_ROOT="$2"; shift 2 ;;
		--artifact-root=*) ARTIFACT_ROOT="${1#*=}"; shift ;;
		--cmake-prefix-path) require_arg "$1" "${2:-}"; CMAKE_PREFIX_PATH="$2"; shift 2 ;;
		--cmake-prefix-path=*) CMAKE_PREFIX_PATH="${1#*=}"; shift ;;
		--jobs) require_arg "$1" "${2:-}"; JOBS="$2"; shift 2 ;;
		--jobs=*) JOBS="${1#*=}"; shift ;;
		--run-smoke) RUN_SMOKE=1; shift ;;
		--enable-netem) ENABLE_NETEM=1; shift ;;
		--run-browser) RUN_BROWSER=1; shift ;;
		--skip-v4l2) RUN_V4L2=0; shift ;;
		--no-strict) STRICT=0; shift ;;
		--preflight-netem-only) PREFLIGHT_NETEM_ONLY=1; shift ;;
		-h|--help) usage; exit 0 ;;
		*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
	esac
done

if ! [[ "$JOBS" =~ ^[0-9]+$ ]] || [[ "$JOBS" -lt 1 ]]; then
	echo "--jobs must be an integer >= 1" >&2
	exit 2
fi
if [[ "$RUN_SMOKE" -eq 1 && "$ENABLE_NETEM" -ne 1 ]]; then
	echo "--run-smoke requires --enable-netem so weak-network and recovery reports cannot be refreshed as SKIP" >&2
	exit 2
fi
if [[ -z "$CMAKE_PREFIX_PATH" && -f "$DEFAULT_SDK_PREFIX/lib/cmake/WebRtcQosSdk/WebRtcQosSdkConfig.cmake" ]]; then
	CMAKE_PREFIX_PATH="$DEFAULT_SDK_PREFIX"
fi

BUILD_DIR="$(python3 - "$ROOT_DIR" "$BUILD_DIR" <<'PY'
import os
import sys
root, path = sys.argv[1:3]
print(path if os.path.isabs(path) else os.path.abspath(os.path.join(root, path)))
PY
)"
WORKER_BIN="$(python3 - "$ROOT_DIR" "$WORKER_BIN" <<'PY'
import os
import sys
root, path = sys.argv[1:3]
print(path if os.path.isabs(path) else os.path.abspath(os.path.join(root, path)))
PY
)"
REPORT_DIR="$(python3 - "$ROOT_DIR" "$REPORT_DIR" <<'PY'
import os
import sys
root, path = sys.argv[1:3]
print(path if os.path.isabs(path) else os.path.abspath(os.path.join(root, path)))
PY
)"

run_step() {
	local label="$1"
	shift
	echo
	echo "==> [p2-acceptance:$label]"
	echo "    $*"
	"$@"
	echo "<== [p2-acceptance:$label] PASS"
}

preflight_netem() {
	local err
	err="$(mktemp)"
	if ! command -v tc >/dev/null 2>&1; then
		echo "tc command not found; cannot refresh weak-network reports" >&2
		rm -f "$err"
		exit 2
	fi
	if [[ "${EUID:-$(id -u)}" -ne 0 ]]; then
		echo "CAP_NET_ADMIN/root is required to refresh weak-network reports" >&2
		rm -f "$err"
		exit 2
	fi
	tc qdisc del dev lo root >/dev/null 2>&1 || true
	if ! tc qdisc add dev lo root netem delay 1ms >"$err" 2>&1; then
		echo "tc netem preflight failed on lo: $(tr '\n' ' ' < "$err")" >&2
		rm -f "$err"
		tc qdisc del dev lo root >/dev/null 2>&1 || true
		exit 2
	fi
	tc qdisc del dev lo root >/dev/null 2>&1 || true
	rm -f "$err"
}

if [[ "$PREFLIGHT_NETEM_ONLY" -eq 1 ]]; then
	preflight_netem
	echo "tc netem preflight passed on lo"
	exit 0
fi
if [[ "$RUN_SMOKE" -eq 1 ]]; then
	preflight_netem
fi

cmake_args=(
	-S "$ROOT_DIR"
	-B "$BUILD_DIR"
	-DBUILD_TESTS=ON
)
if [[ -n "$CMAKE_PREFIX_PATH" ]]; then
	cmake_args+=("-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH")
fi

run_step configure cmake "${cmake_args[@]}"
run_step build cmake --build "$BUILD_DIR" \
	--target mediasoup-sfu webrtc-qos-plain-push-client webrtc-qos-plain-play-client mediasoup_tests mediasoup_webrtc_qos_plain_unit_tests mediasoup_qos_integration_tests \
	-j "$JOBS"
run_step plain-unit "$BUILD_DIR/mediasoup_webrtc_qos_plain_unit_tests" \
	--gtest_filter='WebRtcQosRealtimeSourceTest.*:WebRtcQosDecodeSinkTest.*'
run_step ortc-twcc "$BUILD_DIR/mediasoup_tests" \
	--gtest_filter='OrtcTest.ConsumableAndConsumerRtpParametersPreserveTransportCcExtension:OrtcTest.ConsumableRtpParametersMapsCodecsAndDefaultsScalability'
run_step plain-publish-integration env \
	MEDIASOUP_TEST_SFU_BIN="$BUILD_DIR/mediasoup-sfu" \
	MEDIASOUP_TEST_WORKER_BIN="$WORKER_BIN" \
	"$BUILD_DIR/mediasoup_qos_integration_tests" \
	--gtest_filter='QosIntegrationTest.PlainPublishSupportsVideoOnlyAndKeepsLegacyAudioDefault:QosIntegrationTest.PlainPublishReplacesOldTransportAndUsesBaselineCodec:QosIntegrationTest.PlainPublishRejectsDuplicateVideoSsrcs'
run_step p2-boundaries python3 "$ROOT_DIR/scripts/verify_webrtc_qos_plain_client_boundaries.py"

if [[ "$RUN_SMOKE" -eq 1 ]]; then
	smoke_args=(
		"$ROOT_DIR/scripts/run_webrtc_qos_plain_p2_smoke.sh"
		--build-dir "$BUILD_DIR"
		--worker-bin "$WORKER_BIN"
		--source copy
		--decode-qoe
		--cases baseline,delay_100ms,loss_2pct,loss_5pct,bandwidth_600k,drop_recover
		--duration-seconds 10
		--artifact-root "$ARTIFACT_ROOT/copy"
		--report-dir "$REPORT_DIR"
	)
	if [[ "$ENABLE_NETEM" -eq 1 ]]; then
		smoke_args+=(--enable-netem)
	fi
	if [[ "$STRICT" -eq 1 ]]; then
		smoke_args+=(--strict)
	fi
	run_step copy-smoke "${smoke_args[@]}"

	recovery_args=(
		"$ROOT_DIR/scripts/run_webrtc_qos_plain_p2_smoke.sh"
		--build-dir "$BUILD_DIR"
		--worker-bin "$WORKER_BIN"
		--source synthetic
		--decode-qoe
		--cases drop_recover
		--duration-seconds 30
		--artifact-root "$ARTIFACT_ROOT/recovery-first-frame"
		--report-dir "$REPORT_DIR"
		--report-name webrtc-qos-plain-p2-recovery-first-frame-report
	)
	if [[ "$ENABLE_NETEM" -eq 1 ]]; then
		recovery_args+=(--enable-netem)
	fi
	run_step recovery-first-frame-smoke "${recovery_args[@]}"

	mp4_args=(
		"$ROOT_DIR/scripts/run_webrtc_qos_plain_p2_smoke.sh"
		--build-dir "$BUILD_DIR"
		--worker-bin "$WORKER_BIN"
		--source mp4-decode-loop
		--decode-qoe
		--cases baseline
		--duration-seconds 12
		--artifact-root "$ARTIFACT_ROOT/mp4-decode-loop"
		--report-dir "$REPORT_DIR"
		--report-name webrtc-qos-plain-p2-mp4-decode-loop-report
	)
	run_step mp4-decode-loop-smoke "${mp4_args[@]}"

	if [[ "$RUN_V4L2" -eq 1 ]]; then
		v4l2_args=(
			"$ROOT_DIR/scripts/run_webrtc_qos_plain_p2_smoke.sh"
			--build-dir "$BUILD_DIR"
			--worker-bin "$WORKER_BIN"
			--source v4l2
			--input-v4l2 /dev/video0
			--decode-qoe
			--cases baseline
			--duration-seconds 6
			--artifact-root "$ARTIFACT_ROOT/v4l2"
			--report-dir "$REPORT_DIR"
			--report-name webrtc-qos-plain-p2-v4l2-report
		)
		run_step v4l2-smoke "${v4l2_args[@]}"
	fi

	if [[ "$RUN_BROWSER" -eq 1 ]]; then
		run_step browser-receiver-smoke node "$ROOT_DIR/tests/qos_harness/browser_plain_receiver.mjs" \
			--build-dir "$BUILD_DIR" \
			--worker-bin "$WORKER_BIN" \
			--source synthetic \
			--duration-seconds 10 \
			--artifact-root "$ARTIFACT_ROOT/browser-receiver" \
			--report-dir "$REPORT_DIR" \
			--report-name webrtc-qos-plain-p2-browser-receiver-report
	fi
fi

run_step p2-reports python3 "$ROOT_DIR/scripts/verify_webrtc_qos_plain_p2_reports.py" --generated-dir "$REPORT_DIR"

echo
echo "WebRTC QoS P2 acceptance verified"
