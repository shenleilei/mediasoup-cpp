#!/usr/bin/env python3
"""Verify WebRTC QoS plain client integration boundaries.

This is a static gate for the mediasoup-cpp adapter layer. The plain push/play
clients may keep signaling, UDP and logging glue here, but media QoS,
packetization, depacketization, feedback and pacing must stay inside
webrtc_qos_sdk public facades.
"""

import re
import sys
from pathlib import Path
from typing import Iterable, List, Pattern, Tuple


ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = ROOT / "client" / "webrtc_qos_plain_client"
CMAKE_FILE = ROOT / "CMakeLists.txt"

FORBIDDEN_CLIENT_PATTERNS = [
    (re.compile(r"\bH264Packetizer\b"), "legacy H264 packetizer"),
    (re.compile(r"\bPacketizeAnnexB\b"), "legacy Annex-B packetization"),
    (re.compile(r"\bPublisherQosController\b"), "legacy browser publisher QoS controller"),
    (re.compile(r"#\s*include\s*[<\"]common/media/rtp/"), "shared RTP packetizer headers"),
    (re.compile(r"\bmedia::rtp::"), "shared RTP packetizer namespace"),
    (re.compile(r"\bstd::cout\b"), "stdout logging; use spdlog/file logs"),
    (re.compile(r"\bprintf\s*\("), "printf logging; use spdlog/file logs"),
    (re.compile(r"\bfprintf\s*\("), "fprintf logging; use spdlog/file logs"),
    (re.compile(r"\bputs\s*\("), "puts logging; use spdlog/file logs"),
]

REQUIRED_FILES = [
    CLIENT_DIR / "push" / "WebRtcQosPushRuntime.cpp",
    CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.cpp",
    CLIENT_DIR / "common" / "RtpRtcpClassifier.cpp",
    CLIENT_DIR / "common" / "SdkRuntimeConfig.h",
    CLIENT_DIR / "common" / "RuntimeLogHelpers.cpp",
]

PUSH_REQUIRED_PATTERNS = [
    (re.compile(r"webrtc_qos::CreateVideoPushClient"), "CreateVideoPushClient"),
    (re.compile(r"PushAnnexBAccessUnit"), "PushAnnexBAccessUnit"),
    (re.compile(r"OnTransportFeedback"), "OnTransportFeedback"),
    (re.compile(r"GetEncoderAdaptation"), "GetEncoderAdaptation"),
    (re.compile(r"ConfigureSdkRuntimeFiles"), "SDK runtime file config"),
]

PLAY_REQUIRED_PATTERNS = [
    (re.compile(r"webrtc_qos::CreateVideoPlayClient"), "CreateVideoPlayClient"),
    (re.compile(r"OnRtpPacket"), "OnRtpPacket"),
    (re.compile(r"OnRtcpPacket"), "OnRtcpPacket"),
    (re.compile(r"decoded_access_unit_output"), "decoded_access_unit_output"),
    (re.compile(r"ConfigureSdkRuntimeFiles"), "SDK runtime file config"),
]

CMAKE_REQUIRED_PATTERNS = [
    (re.compile(r"find_package\(WebRtcQosSdk CONFIG QUIET\)"), "WebRtcQosSdk package lookup"),
    (re.compile(r"select_webrtc_qos_role\(WEBRTC_QOS_PUSH_TARGET push\)"), "push role target selection"),
    (re.compile(r"select_webrtc_qos_role\(WEBRTC_QOS_PLAY_TARGET play\)"), "play role target selection"),
    (re.compile(r"mediasoup_add_configured_executable\(webrtc-qos-plain-push-client"), "push executable target"),
    (re.compile(r"mediasoup_add_configured_executable\(webrtc-qos-plain-play-client"), "play executable target"),
    (re.compile(r"\$\{WEBRTC_QOS_PUSH_TARGET\}"), "push target links SDK push role"),
    (re.compile(r"\$\{WEBRTC_QOS_PLAY_TARGET\}"), "play target links SDK play role"),
]

LOG_HELPER_REQUIRED_PATTERNS = [
    (re.compile(r"spdlog::sinks::basic_file_sink_mt"), "spdlog file sink"),
    (re.compile(r"std::filesystem::create_directories"), "log directory creation"),
]


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def source_files(root: Path) -> Iterable[Path]:
    suffixes = {".cpp", ".cc", ".cxx", ".h", ".hpp"}
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix in suffixes:
            yield path


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def target_block(cmake_text: str, target_name: str) -> str:
    marker = f"mediasoup_add_configured_executable({target_name}"
    start = cmake_text.find(marker)
    if start < 0:
        return ""
    next_target = cmake_text.find("mediasoup_add_configured_executable(", start + len(marker))
    next_else = cmake_text.find("    else()", start + len(marker))
    candidates = [pos for pos in (next_target, next_else) if pos >= 0]
    end = min(candidates) if candidates else len(cmake_text)
    return cmake_text[start:end]


def check_required_file(errors: List[str], path: Path) -> None:
    if not path.is_file():
        errors.append(f"missing required file: {rel(path)}")


def check_patterns(errors: List[str], label: str, text: str, required: List[Tuple[Pattern, str]]) -> None:
    for pattern, description in required:
        if not pattern.search(text):
            errors.append(f"{label}: missing {description}")


def verify_no_forbidden_client_symbols(errors: List[str]) -> None:
    for path in source_files(CLIENT_DIR):
        text = read_text(path)
        for pattern, description in FORBIDDEN_CLIENT_PATTERNS:
            match = pattern.search(text)
            if match:
                line_no = text[:match.start()].count("\n") + 1
                errors.append(f"{rel(path)}:{line_no}: forbidden {description}: {match.group(0)}")


def verify_runtime_facades(errors: List[str]) -> None:
    push_path = CLIENT_DIR / "push" / "WebRtcQosPushRuntime.cpp"
    play_path = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.cpp"
    log_helper_path = CLIENT_DIR / "common" / "RuntimeLogHelpers.cpp"
    check_patterns(errors, rel(push_path), read_text(push_path), PUSH_REQUIRED_PATTERNS)
    check_patterns(errors, rel(play_path), read_text(play_path), PLAY_REQUIRED_PATTERNS)
    check_patterns(errors, rel(log_helper_path), read_text(log_helper_path), LOG_HELPER_REQUIRED_PATTERNS)


def verify_cmake_boundaries(errors: List[str]) -> None:
    cmake_text = read_text(CMAKE_FILE)
    check_patterns(errors, rel(CMAKE_FILE), cmake_text, CMAKE_REQUIRED_PATTERNS)

    push_block = target_block(cmake_text, "webrtc-qos-plain-push-client")
    play_block = target_block(cmake_text, "webrtc-qos-plain-play-client")
    if not push_block:
        errors.append("CMakeLists.txt: missing webrtc-qos-plain-push-client target block")
    if not play_block:
        errors.append("CMakeLists.txt: missing webrtc-qos-plain-play-client target block")

    target_checks = [
        ("webrtc-qos-plain-push-client", push_block, "${WEBRTC_QOS_PUSH_TARGET}"),
        ("webrtc-qos-plain-play-client", play_block, "${WEBRTC_QOS_PLAY_TARGET}"),
    ]
    for target, block, sdk_target in target_checks:
        if not block:
            continue
        if sdk_target not in block:
            errors.append(f"CMakeLists.txt:{target}: must link {sdk_target}")
        if "mediasoup_common_media" in block:
            errors.append(f"CMakeLists.txt:{target}: must not link mediasoup_common_media")
        for forbidden, description in FORBIDDEN_CLIENT_PATTERNS[:3]:
            if forbidden.search(block):
                errors.append(f"CMakeLists.txt:{target}: forbidden {description}")


def main() -> int:
    errors = []  # type: List[str]
    if not CLIENT_DIR.is_dir():
        errors.append(f"missing client directory: {rel(CLIENT_DIR)}")
    if not CMAKE_FILE.is_file():
        errors.append(f"missing CMake file: {rel(CMAKE_FILE)}")
    for path in REQUIRED_FILES:
        check_required_file(errors, path)

    if not errors:
        verify_no_forbidden_client_symbols(errors)
        verify_runtime_facades(errors)
        verify_cmake_boundaries(errors)

    if errors:
        print("WebRTC QoS plain client boundary verification failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("WebRTC QoS plain client boundaries verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
