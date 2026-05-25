#!/usr/bin/env python3
"""Verify generated WebRTC QoS P2 report JSON files.

The smoke runner proves the runtime path by executing SFU/push/play. This
script is the offline acceptance gate for review: it re-checks the generated
reports and fails if SKIP/PARTIAL/PASS semantics drift from the P2 contract.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional


MAIN_REPORT = "webrtc-qos-plain-p2-smoke-report.json"
MP4_REPORT = "webrtc-qos-plain-p2-mp4-decode-loop-report.json"
V4L2_REPORT = "webrtc-qos-plain-p2-v4l2-report.json"
BROWSER_REPORT = "webrtc-qos-plain-p2-browser-receiver-report.json"
RECOVERY_REPORT = "webrtc-qos-plain-p2-recovery-first-frame-report.json"

MAIN_CASES = [
    "baseline",
    "delay_100ms",
    "loss_2pct",
    "loss_5pct",
    "bandwidth_600k",
    "drop_recover",
]


class ReportVerifier:
    def __init__(self) -> None:
        self.errors = []  # type: List[str]
        self.summaries = []  # type: List[str]

    def require(self, label: str, condition: bool, message: str) -> None:
        if not condition:
            self.errors.append(f"{label}: {message}")

    def load(self, path: Path, label: str) -> Dict[str, Any]:
        if not path.is_file():
            self.errors.append(f"{label}: missing report {path}")
            return {}
        try:
            with path.open("r", encoding="utf-8") as f:
                data = json.load(f)
        except Exception as exc:  # pragma: no cover - diagnostic path.
            self.errors.append(f"{label}: failed to parse {path}: {exc}")
            return {}
        self.require(label, data.get("schemaVersion") == 1, "schemaVersion must be 1")
        return data

    def verify_markdown_peer(self, json_path: Path, label: str) -> None:
        md_path = json_path.with_suffix(".md")
        self.require(label, md_path.is_file(), f"missing markdown peer {md_path}")

    def add_summary(self, label: str, report: Dict[str, Any]) -> None:
        status = report.get("overallStatus")
        summary = report.get("summary") or {}
        if summary:
            self.summaries.append(
                f"{label}: overall={status} "
                f"passed={summary.get('passedCases')} "
                f"skipped={summary.get('skippedCases')} "
                f"failedChecks={summary.get('failedChecks')}"
            )
        else:
            case = report.get("case") or {}
            self.summaries.append(f"{label}: overall={status} case={case.get('status')}")


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def positive(value: Any) -> bool:
    return is_number(value) and value > 0


def non_negative(value: Any) -> bool:
    return is_number(value) and value >= 0


def case_by_name(report: Dict[str, Any], name: str) -> Dict[str, Any]:
    for case in report.get("cases", []):
        if case.get("name") == name:
            return case
    return {}


def gate_status(report: Dict[str, Any], name: str) -> Optional[str]:
    gate = (report.get("gates") or {}).get(name) or {}
    return gate.get("status")


def gate_evidence(report: Dict[str, Any], name: str) -> Dict[str, Any]:
    gate = (report.get("gates") or {}).get(name) or {}
    evidence = gate.get("evidence") or {}
    return evidence if isinstance(evidence, dict) else {}


def check_status(case: Dict[str, Any], name: str) -> Optional[str]:
    for check in case.get("checks", []):
        if check.get("name") == name:
            return check.get("status")
    return None


def no_failed_checks(report: Dict[str, Any]) -> bool:
    if report.get("failedChecks"):
        return False
    summary = report.get("summary") or {}
    return summary.get("failedChecks", 0) == 0


def all_attempted_cases_have_no_fail(report: Dict[str, Any]) -> bool:
    for case in report.get("cases", []):
        if case.get("status") == "SKIP":
            continue
        for check in case.get("checks", []):
            if check.get("status") != "PASS":
                return False
    return True


def require_sdk_runtime(verifier: ReportVerifier, label: str, case: Dict[str, Any]) -> None:
    sdk = ((case.get("metrics") or {}).get("sdkRuntime") or {})
    for role in ("push", "play"):
        runtime = sdk.get(role) or {}
        verifier.require(label, runtime.get("enabled") is True, f"{role} SDK runtime must be enabled")
        verifier.require(label, positive(runtime.get("runtimeLogFiles")), f"{role} runtime log file must exist")
        verifier.require(label, positive(runtime.get("metricsFiles")), f"{role} metrics file must exist")
        verifier.require(label, positive(runtime.get("alertsFiles")), f"{role} alerts file must exist")
        verifier.require(label, positive(runtime.get("transportFeedbackCountMax")), f"{role} TWCC count must be > 0")
        verifier.require(label, positive(runtime.get("receiverReportCountMax")), f"{role} RR count must be > 0")
        verifier.require(label, runtime.get("transportFailureCountMax", 0) == 0, f"{role} transport failures must be 0")


def require_qoe_ok(verifier: ReportVerifier, label: str, case: Dict[str, Any]) -> None:
    qoe = ((case.get("metrics") or {}).get("qoe") or {})
    verifier.require(label, qoe.get("enabled") is True, "QoE decode must be enabled")
    verifier.require(label, positive(qoe.get("samples")), "QoE sample count must be > 0")
    verifier.require(label, positive(qoe.get("decodedFrames")), "decodedFrames must be > 0")
    verifier.require(label, qoe.get("decodeErrors") == 0, "decodeErrors must be 0")
    verifier.require(label, non_negative(qoe.get("firstFrameDelayUs")), "firstFrameDelayUs must be observable")


def verify_main_report(verifier: ReportVerifier, path: Path) -> None:
    label = "p2-main-smoke"
    report = verifier.load(path, label)
    if not report:
        return
    verifier.verify_markdown_peer(path, label)
    run_config = report.get("runConfig") or {}
    summary = report.get("summary") or {}
    verifier.require(label, report.get("overallStatus") == "PASS", "overallStatus must be PASS")
    verifier.require(label, no_failed_checks(report), "failedChecks must be 0")
    verifier.require(label, run_config.get("sourceMode") == "copy", "main report must use copy source")
    verifier.require(label, run_config.get("decodeQoe") is True, "main report must enable decode QoE")
    verifier.require(label, run_config.get("enableNetem") is True, "main weak-network report must enable netem")
    verifier.require(label, summary.get("passedCases") == len(MAIN_CASES), "all main cases must pass")
    verifier.require(label, summary.get("skippedCases") == 0, "main report must not skip cases")
    verifier.require(label, all_attempted_cases_have_no_fail(report), "attempted case checks must all PASS")

    expected_gates = {
        "qosMainline": "PASS",
        "sdkRuntimeObservability": "PASS",
        "nativeDecodeQoe": "PASS",
        "recoveryFirstFrame": "PASS",
        "weakNetworkCoverage": "PASS",
        "encoderRuntime": "SKIP",
    }
    for gate, status in expected_gates.items():
        verifier.require(label, gate_status(report, gate) == status, f"{gate} must be {status}")

    encoder_evidence = gate_evidence(report, "encoderRuntime")
    verifier.require(
        label,
        encoder_evidence.get("sourceMode") == "copy"
        and isinstance(encoder_evidence.get("skipReason"), str)
        and "x264" in encoder_evidence.get("skipReason", ""),
        "encoderRuntime SKIP must explain that copy source does not exercise x264",
    )

    seen_cases = [case.get("name") for case in report.get("cases", [])]
    verifier.require(label, seen_cases == MAIN_CASES, f"case order/content mismatch: {seen_cases}")
    for name in MAIN_CASES:
        case = case_by_name(report, name)
        metrics = case.get("metrics") or {}
        verifier.require(label, case.get("status") == "PASS", f"{name} must PASS")
        verifier.require(label, positive(metrics.get("pushedAu")), f"{name} pushedAu must be > 0")
        verifier.require(label, positive(metrics.get("playOutputAu")), f"{name} outputAu must be > 0")
        verifier.require(label, positive(metrics.get("playRtpPackets")), f"{name} RTP input must be > 0")
        verifier.require(label, positive(metrics.get("pushRtcpFeedbackPacketsIn")), f"{name} push RTCP in must be > 0")
        verifier.require(label, positive(metrics.get("playRtcpPacketsOut")), f"{name} play RTCP out must be > 0")
        verifier.require(label, positive(metrics.get("selectedTwccExtId")), f"{name} selected TWCC ext id must be > 0")
        require_qoe_ok(verifier, f"{label}:{name}", case)
        require_sdk_runtime(verifier, f"{label}:{name}", case)
        artifacts = case.get("artifacts") or {}
        verifier.require(label, bool(artifacts.get("caseDir")), f"{name} must record caseDir artifact")
        verifier.require(label, bool(artifacts.get("pushLog")), f"{name} must record push log artifact")
        verifier.require(label, bool(artifacts.get("playLog")), f"{name} must record play log artifact")

    delay_metrics = (case_by_name(report, "delay_100ms").get("metrics") or {})
    delay_rtt = delay_metrics.get("rttMs") or {}
    verifier.require(label, positive(delay_rtt.get("max")) and delay_rtt["max"] >= 50, "delay case RTT max must show netem")

    bandwidth_metrics = (case_by_name(report, "bandwidth_600k").get("metrics") or {})
    target = bandwidth_metrics.get("targetBps") or {}
    verifier.require(
        label,
        positive(target.get("min")) and positive(target.get("max")) and target["min"] < target["max"] * 0.9,
        "bandwidth case targetBps must drop",
    )

    recovery_case = case_by_name(report, "drop_recover")
    recovery_target = (recovery_case.get("metrics") or {}).get("targetBps") or {}
    verifier.require(
        label,
        check_status(recovery_case, "weak-recovery-target-up") == "PASS",
        "drop_recover must pass weak-recovery-target-up check",
    )
    verifier.require(
        label,
        positive(recovery_target.get("min"))
        and positive(recovery_target.get("max"))
        and recovery_target["min"] < recovery_target["max"],
        "drop_recover targetBps must vary instead of staying pinned at the minimum",
    )
    recovery_qoe = (((recovery_case.get("metrics") or {}).get("qoe") or {}).get("recovery") or {})
    verifier.require(
        label,
        check_status(recovery_case, "qoe-recovery-first-frame-after-clear") == "PASS"
        and
        non_negative(recovery_qoe.get("postClearFirstDecodedDelayMs"))
        and recovery_qoe["postClearFirstDecodedDelayMs"] <= 15000
        and positive(recovery_qoe.get("postClearDecodedFramesDelta")),
        "drop_recover must decode frames within 15s after netem clear",
    )
    verifier.add_summary(label, report)


def verify_mp4_decode_loop_report(verifier: ReportVerifier, path: Path) -> None:
    label = "p2-mp4-decode-loop"
    report = verifier.load(path, label)
    if not report:
        return
    verifier.verify_markdown_peer(path, label)
    run_config = report.get("runConfig") or {}
    baseline = case_by_name(report, "baseline")
    encoder = ((baseline.get("metrics") or {}).get("encoder") or {})
    verifier.require(label, report.get("overallStatus") == "PARTIAL", "baseline-only report should be PARTIAL")
    verifier.require(label, no_failed_checks(report), "failedChecks must be 0")
    verifier.require(label, run_config.get("sourceMode") == "mp4-decode-loop", "sourceMode must be mp4-decode-loop")
    verifier.require(label, run_config.get("decodeQoe") is True, "MP4 decode-loop report must enable decode QoE")
    verifier.require(label, run_config.get("enableNetem") is False, "MP4 decode-loop baseline report must not claim netem")
    verifier.require(label, run_config.get("cases") == ["baseline"], "MP4 decode-loop report must be baseline-only")
    verifier.require(label, baseline.get("status") == "PASS", "baseline must PASS")
    for gate in ("qosMainline", "sdkRuntimeObservability", "encoderRuntime", "nativeDecodeQoe"):
        verifier.require(label, gate_status(report, gate) == "PASS", f"{gate} must PASS")
    for gate in ("recoveryFirstFrame", "weakNetworkCoverage"):
        verifier.require(label, gate_status(report, gate) == "SKIP", f"{gate} must SKIP in baseline-only report")
    verifier.require(label, encoder.get("mode") == "mp4_decode_loop", "encoder mode must be mp4_decode_loop")
    verifier.require(label, encoder.get("name") == "x264", "encoder name must be x264")
    verifier.require(label, positive(encoder.get("accessUnits")), "encoder accessUnits must be > 0")
    verifier.require(label, positive(encoder.get("keyframes")), "encoder keyframes must be > 0")
    verifier.require(label, positive(encoder.get("forcedKeyframeRequests")), "forcedKeyframeRequests must be > 0")
    verifier.require(label, positive(encoder.get("forcedKeyframes")), "forcedKeyframes must be > 0")
    verifier.require(label, non_negative(encoder.get("maxForcedKeyframeDelayUs")) and encoder["maxForcedKeyframeDelayUs"] <= 1000000, "forced IDR delay must be <= 1s")
    require_qoe_ok(verifier, label, baseline)
    require_sdk_runtime(verifier, label, baseline)
    verifier.add_summary(label, report)


def verify_v4l2_report(verifier: ReportVerifier, path: Path) -> None:
    label = "p2-v4l2"
    report = verifier.load(path, label)
    if not report:
        return
    verifier.verify_markdown_peer(path, label)
    run_config = report.get("runConfig") or {}
    baseline = case_by_name(report, "baseline")
    verifier.require(label, no_failed_checks(report), "failedChecks must be 0")
    verifier.require(label, run_config.get("sourceMode") == "v4l2", "sourceMode must be v4l2")
    verifier.require(label, run_config.get("decodeQoe") is True, "V4L2 report must enable decode QoE")
    verifier.require(label, run_config.get("cases") == ["baseline"], "V4L2 report must be baseline-only")
    status = baseline.get("status")
    if status == "SKIP":
        reason = baseline.get("skipReason", "")
        verifier.require(label, report.get("overallStatus") == "PARTIAL", "V4L2 environment skip should be PARTIAL")
        verifier.require(label, isinstance(reason, str) and "v4l2" in reason.lower(), "V4L2 SKIP must include device reason")
        for gate in ("qosMainline", "sdkRuntimeObservability", "encoderRuntime", "nativeDecodeQoe"):
            verifier.require(label, gate_status(report, gate) == "SKIP", f"{gate} must SKIP when device is unavailable")
    elif status == "PASS":
        verifier.require(label, report.get("overallStatus") in ("PASS", "PARTIAL"), "V4L2 PASS report must be PASS/PARTIAL")
        for gate in ("qosMainline", "sdkRuntimeObservability", "encoderRuntime", "nativeDecodeQoe"):
            verifier.require(label, gate_status(report, gate) == "PASS", f"{gate} must PASS when V4L2 baseline runs")
        encoder = ((baseline.get("metrics") or {}).get("encoder") or {})
        verifier.require(label, encoder.get("mode") == "v4l2", "encoder mode must be v4l2")
        verifier.require(label, encoder.get("name") == "x264", "encoder name must be x264")
        verifier.require(label, positive(encoder.get("accessUnits")), "V4L2 encoder accessUnits must be > 0")
        require_qoe_ok(verifier, label, baseline)
        require_sdk_runtime(verifier, label, baseline)
    else:
        verifier.require(label, False, f"baseline must PASS or SKIP, got {status!r}")
    verifier.add_summary(label, report)


def verify_recovery_report(verifier: ReportVerifier, path: Path) -> None:
    label = "p2-recovery-first-frame"
    report = verifier.load(path, label)
    if not report:
        return
    verifier.verify_markdown_peer(path, label)
    run_config = report.get("runConfig") or {}
    summary = report.get("summary") or {}
    drop_recover = case_by_name(report, "drop_recover")
    recovery_qoe = (((drop_recover.get("metrics") or {}).get("qoe") or {}).get("recovery") or {})
    recovery_target = (drop_recover.get("metrics") or {}).get("targetBps") or {}
    encoder = ((drop_recover.get("metrics") or {}).get("encoder") or {})
    verifier.require(label, no_failed_checks(report), "failedChecks must be 0")
    verifier.require(label, run_config.get("sourceMode") == "synthetic", "recovery report must use synthetic source")
    verifier.require(label, run_config.get("decodeQoe") is True, "recovery report must enable decode QoE")
    verifier.require(label, run_config.get("enableNetem") is True, "recovery report must enable netem")
    verifier.require(label, run_config.get("cases") == ["drop_recover"], "recovery report must contain only drop_recover")
    verifier.require(label, summary.get("passedCases") == 1, "recovery report must pass one case")
    verifier.require(label, summary.get("skippedCases") == 0, "recovery report must not skip cases")
    verifier.require(label, drop_recover.get("status") == "PASS", "drop_recover must PASS")
    verifier.require(label, gate_status(report, "recoveryFirstFrame") == "PASS", "recoveryFirstFrame gate must PASS")
    verifier.require(label, gate_status(report, "weakNetworkCoverage") == "PASS", "weakNetworkCoverage must PASS")
    verifier.require(
        label,
        check_status(drop_recover, "weak-recovery-target-up") == "PASS",
        "recovery report must pass weak-recovery-target-up check",
    )
    verifier.require(
        label,
        positive(recovery_target.get("min"))
        and positive(recovery_target.get("max"))
        and recovery_target["min"] < recovery_target["max"],
        "recovery report targetBps must vary instead of staying pinned at the minimum",
    )
    verifier.require(
        label,
        check_status(drop_recover, "qoe-recovery-first-frame-after-clear") == "PASS"
        and
        non_negative(recovery_qoe.get("postClearFirstDecodedDelayMs"))
        and recovery_qoe["postClearFirstDecodedDelayMs"] <= 15000
        and positive(recovery_qoe.get("postClearDecodedFramesDelta")),
        "recovery report must decode frames within 15s after netem clear",
    )
    verifier.require(label, encoder.get("mode") == "synthetic", "recovery encoder mode must be synthetic")
    verifier.require(label, encoder.get("name") == "x264", "recovery encoder name must be x264")
    verifier.require(label, positive(encoder.get("accessUnits")), "recovery encoder accessUnits must be > 0")
    verifier.require(label, positive(encoder.get("forcedKeyframeRequests")), "recovery must observe forced keyframe requests")
    verifier.require(label, positive(encoder.get("forcedKeyframes")), "recovery must output forced keyframes")
    verifier.require(label, non_negative(encoder.get("maxForcedKeyframeDelayUs")) and encoder["maxForcedKeyframeDelayUs"] <= 1000000, "recovery forced IDR delay must be <= 1s")
    require_qoe_ok(verifier, label, drop_recover)
    require_sdk_runtime(verifier, label, drop_recover)
    verifier.add_summary(label, report)


def verify_browser_report(verifier: ReportVerifier, path: Path) -> None:
    label = "p2-browser-receiver"
    report = verifier.load(path, label)
    if not report:
        return
    verifier.verify_markdown_peer(path, label)
    case = report.get("case") or {}
    run_config = report.get("runConfig") or {}
    checks = case.get("checks") or []
    statuses = {check.get("name"): check.get("status") for check in checks}
    metrics = case.get("metrics") or {}
    push = metrics.get("push") or {}
    browser = metrics.get("browser") or {}
    diagnostics = browser.get("diagnostics") or {}
    device = diagnostics.get("device") or {}

    verifier.require(label, case.get("name") == "browser_receiver", "case name must be browser_receiver")
    verifier.require(label, run_config.get("sourceMode") == "synthetic", "browser receiver report must use synthetic source")
    verifier.require(label, positive(run_config.get("durationSeconds")), "browser receiver report must record duration")
    verifier.require(label, statuses.get("plain-push-alive") == "PASS", "plain push must start")
    verifier.require(label, statuses.get("plain-publish-ok") == "PASS", "plain publish must pass")
    verifier.require(label, positive(push.get("twccExtId")), "push twccExtId must be > 0")
    verifier.require(label, positive(push.get("pushedAu")), "push pushedAu must be > 0")
    verifier.require(label, positive(push.get("metricSamples")), "push metricSamples must be > 0")
    verifier.require(label, not any(status == "FAIL" for status in statuses.values()), "browser report must not contain FAIL checks")

    h264_status = statuses.get("browser-h264-capability")
    if h264_status == "SKIP":
        verifier.require(label, report.get("overallStatus") == "PARTIAL", "H264 capability skip should make browser report PARTIAL")
        verifier.require(label, case.get("browserErrorCode") == "BROWSER_H264_UNSUPPORTED", "browser skip must record BROWSER_H264_UNSUPPORTED")
        verifier.require(label, device.get("supportsH264Packetization1") is False, "browser diagnostics must show H264 unsupported")
        for check in ("browser-consumer-created", "browser-keyframe-requested", "browser-receiver-media-flow", "browser-track-live"):
            verifier.require(label, statuses.get(check) == "SKIP", f"{check} must SKIP when H264 is unavailable")
    else:
        verifier.require(label, h264_status == "PASS", "browser-h264-capability must PASS or SKIP")
        verifier.require(label, report.get("overallStatus") == "PASS", "browser capable environment must PASS")
        for check in ("browser-consumer-created", "browser-keyframe-requested", "browser-receiver-media-flow", "browser-track-live"):
            verifier.require(label, statuses.get(check) == "PASS", f"{check} must PASS when H264 is available")
    verifier.add_summary(label, report)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--generated-dir",
        default="docs/generated",
        help="Directory containing generated P2 report JSON files. Default: docs/generated",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    generated_dir = Path(args.generated_dir)
    verifier = ReportVerifier()
    verify_main_report(verifier, generated_dir / MAIN_REPORT)
    verify_mp4_decode_loop_report(verifier, generated_dir / MP4_REPORT)
    verify_v4l2_report(verifier, generated_dir / V4L2_REPORT)
    verify_browser_report(verifier, generated_dir / BROWSER_REPORT)
    verify_recovery_report(verifier, generated_dir / RECOVERY_REPORT)

    if verifier.errors:
        print("WebRTC QoS P2 report verification failed:", file=sys.stderr)
        for error in verifier.errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    for summary in verifier.summaries:
        print(summary)
    print("WebRTC QoS P2 reports verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
