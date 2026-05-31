#!/usr/bin/env python3
import argparse
import hashlib
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[1]
DOCS_GENERATED_DIR = ROOT_DIR / "docs" / "generated"
MANIFEST_PATH = DOCS_GENERATED_DIR / "ipc-contract-manifest.json"
STAMP_PATH = DOCS_GENERATED_DIR / "ipc-full-regression-stamp.json"

MAIN_RUNTIME_HEADER = ROOT_DIR / "third_party" / "flatbuffers" / "include" / "flatbuffers" / "base.h"
MAIN_MESSAGE_GENERATED = ROOT_DIR / "generated" / "message_generated.h"
MAIN_NOTIFICATION_GENERATED = ROOT_DIR / "generated" / "notification_generated.h"
MAIN_SETUP = ROOT_DIR / "setup.sh"
MAIN_CMAKE = ROOT_DIR / "CMakeLists.txt"
WORKER_WRAP = ROOT_DIR / "src" / "mediasoup-worker-src" / "worker" / "subprojects" / "flatbuffers.wrap"
WORKER_MESON = ROOT_DIR / "src" / "mediasoup-worker-src" / "worker" / "fbs" / "meson.build"
WORKER_GENERATED_MESSAGE = (
    ROOT_DIR
    / "src"
    / "mediasoup-worker-src"
    / "worker"
    / "out"
    / "Release"
    / "build"
    / "fbs"
    / "FBS"
    / "message.h"
)
WORKER_GENERATED_NOTIFICATION = (
    ROOT_DIR
    / "src"
    / "mediasoup-worker-src"
    / "worker"
    / "out"
    / "Release"
    / "build"
    / "fbs"
    / "FBS"
    / "notification.h"
)

IPC_SENSITIVE_PATTERNS = [
    "fbs/*.fbs",
    "generated/*_generated.h",
    "src/Channel.cpp",
    "src/Channel.h",
    "src/TransportConnectResponseUtils.h",
    "src/mediasoup-worker-src/worker/fbs/*.fbs",
    "src/mediasoup-worker-src/worker/fbs/meson.build",
    "src/mediasoup-worker-src/worker/subprojects/flatbuffers.wrap",
    "src/mediasoup-worker-src/worker/include/Channel/**/*",
    "src/mediasoup-worker-src/worker/src/Channel/**/*",
    "src/mediasoup-worker-src/worker/include/RTC/**/*",
    "src/mediasoup-worker-src/worker/src/RTC/**/*",
    "CMakeLists.txt",
    "setup.sh",
]


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def sha256_file(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def parse_flatbuffers_version(path: Path):
    if not path.exists():
        return None

    text = load_text(path)
    patterns = {
        "major": re.compile(r"FLATBUFFERS_VERSION_MAJOR\s*(?:==)?\s*(\d+)"),
        "minor": re.compile(r"FLATBUFFERS_VERSION_MINOR\s*(?:==)?\s*(\d+)"),
        "revision": re.compile(r"FLATBUFFERS_VERSION_REVISION\s*(?:==)?\s*(\d+)"),
    }

    parts = {}
    for key, pattern in patterns.items():
        match = pattern.search(text)
        if not match:
            return None
        parts[key] = match.group(1)

    return "{major}.{minor}.{revision}".format(**parts)


def parse_worker_wrap_version():
    if not WORKER_WRAP.exists():
        return None

    text = load_text(WORKER_WRAP)
    match = re.search(r"directory\s*=\s*flatbuffers-([0-9.]+)", text)
    return match.group(1) if match else None


def parse_generation_args(path: Path, style: str):
    if not path.exists():
        return []

    text = load_text(path)

    if style == "main":
        args = []
        for flag in ("--cpp", "--gen-object-api", "--scoped-enums"):
            if flag in text:
                args.append(flag)
        return args

    args = []
    if "--cpp" in text:
        args.append("--cpp")
    if "--cpp-field-case-style" in text and "lower" in text:
        args.append("--cpp-field-case-style lower")
    if "--reflect-names" in text:
        args.append("--reflect-names")
    if "--scoped-enums" in text:
        args.append("--scoped-enums")
    if "--filename-suffix" in text:
        args.append("--filename-suffix ''")
    return args


def collect_sensitive_files():
    files = set()
    for pattern in IPC_SENSITIVE_PATTERNS:
        for candidate in ROOT_DIR.glob(pattern):
            if candidate.is_file():
                files.add(candidate.resolve())
    return sorted(files)


def git_changed_files():
    import subprocess

    try:
        result = subprocess.run(
            ["git", "-C", str(ROOT_DIR), "status", "--porcelain"],
            check=True,
            capture_output=True,
            text=True,
        )
    except Exception:
        return []

    changed = []
    for line in result.stdout.splitlines():
        if not line:
            continue
        payload = line[3:] if len(line) > 3 else ""
        if " -> " in payload:
            payload = payload.split(" -> ", 1)[1]
        if payload:
            changed.append(payload)
    return changed


def changed_sensitive_paths():
    changed = []
    changed_files = git_changed_files()
    for rel_path in changed_files:
        candidate = ROOT_DIR / rel_path
        try:
            resolved = candidate.resolve()
        except FileNotFoundError:
            resolved = candidate
        if any(entry.resolve() == resolved for entry in collect_sensitive_files()):
            changed.append(rel_path)
    return sorted(set(changed))


def build_state():
    files = collect_sensitive_files()

    main_runtime_version = parse_flatbuffers_version(MAIN_RUNTIME_HEADER)
    main_message_version = parse_flatbuffers_version(MAIN_MESSAGE_GENERATED)
    main_notification_version = parse_flatbuffers_version(MAIN_NOTIFICATION_GENERATED)
    worker_wrap_version = parse_worker_wrap_version()
    worker_message_version = parse_flatbuffers_version(WORKER_GENERATED_MESSAGE)
    worker_notification_version = parse_flatbuffers_version(WORKER_GENERATED_NOTIFICATION)

    file_entries = []
    for path in files:
        file_entries.append(
            {
                "path": path.relative_to(ROOT_DIR).as_posix(),
                "sha256": sha256_file(path),
            }
        )

    state = {
        "generatedAtUtc": utc_now(),
        "main": {
            "runtimeInclude": "third_party/flatbuffers/include",
            "runtimeVersion": main_runtime_version,
            "generatedMessageVersion": main_message_version,
            "generatedNotificationVersion": main_notification_version,
            "generationArgs": parse_generation_args(MAIN_SETUP, "main"),
        },
        "worker": {
            "wrapVersion": worker_wrap_version,
            "generatedMessageVersion": worker_message_version,
            "generatedNotificationVersion": worker_notification_version,
            "generationArgs": parse_generation_args(WORKER_MESON, "worker"),
        },
        "ipcSensitiveFiles": file_entries,
    }

    fingerprint_payload = {
        "main": state["main"],
        "worker": {
            "wrapVersion": state["worker"]["wrapVersion"],
            "generationArgs": state["worker"]["generationArgs"],
        },
        "ipcSensitiveFiles": state["ipcSensitiveFiles"],
    }
    state["ipcContractFingerprint"] = sha256_bytes(
        json.dumps(fingerprint_payload, sort_keys=True, separators=(",", ":")).encode("utf-8")
    )

    return state


def collect_consistency_errors(state, require_worker_generated: bool):
    errors = []

    main_runtime = state["main"]["runtimeVersion"]
    main_msg = state["main"]["generatedMessageVersion"]
    main_notif = state["main"]["generatedNotificationVersion"]

    if not main_runtime:
        errors.append(f"missing main FlatBuffers runtime header version: {MAIN_RUNTIME_HEADER}")
    if not main_msg or not main_notif:
        errors.append("missing main generated FlatBuffers header version(s)")
    if main_msg and main_notif and main_msg != main_notif:
        errors.append(
            f"main generated header versions differ: message={main_msg} notification={main_notif}"
        )
    if main_runtime and main_msg and main_runtime != main_msg:
        errors.append(
            f"main runtime/generated mismatch: runtime={main_runtime} generated={main_msg}"
        )

    worker_wrap = state["worker"]["wrapVersion"]
    worker_msg = state["worker"]["generatedMessageVersion"]
    worker_notif = state["worker"]["generatedNotificationVersion"]

    if not worker_wrap:
        errors.append(f"missing worker flatbuffers wrap version: {WORKER_WRAP}")
    if require_worker_generated and (not worker_msg or not worker_notif):
        errors.append(
            "worker generated FlatBuffers headers are missing; build the worker before recording IPC regression success"
        )
    if worker_msg and worker_notif and worker_msg != worker_notif:
        errors.append(
            f"worker generated header versions differ: message={worker_msg} notification={worker_notif}"
        )
    if worker_wrap and worker_msg and worker_wrap != worker_msg:
        errors.append(
            f"worker wrap/generated mismatch: wrap={worker_wrap} generated={worker_msg}"
        )

    if not state["ipcSensitiveFiles"]:
        errors.append("no IPC-sensitive files were discovered for fingerprinting")

    return errors


def write_json(path: Path, payload):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def read_report_generated_at():
    report_path = ROOT_DIR / "docs" / "full-regression-test-results.md"
    if not report_path.exists():
        return None

    text = load_text(report_path)
    match = re.search(r"Generated at:\s*`([^`]+)`", text)
    return match.group(1) if match else None


def print_errors(errors):
    for error in errors:
        print(f"error: {error}", file=sys.stderr)


def command_check_consistency(args):
    state = build_state()
    errors = collect_consistency_errors(state, args.require_worker_generated)
    if errors:
        print_errors(errors)
        return 1
    if args.warn_changed:
        changed = changed_sensitive_paths()
        if changed:
            print("warning: IPC-sensitive paths currently modified:", file=sys.stderr)
            for path in changed[:20]:
                print(f"  - {path}", file=sys.stderr)
            if len(changed) > 20:
                print(f"  - ... and {len(changed) - 20} more", file=sys.stderr)
            print(
                "warning: finish by rerunning `cd /root/workspace/mediasoup-cpp && ./script/run_all_tests.sh all` before release",
                file=sys.stderr,
            )
    print("ipc contract metadata is internally consistent")
    return 0


def command_record_success(args):
    state = build_state()
    errors = collect_consistency_errors(state, require_worker_generated=True)
    if errors:
        print_errors(errors)
        return 1

    manifest = {
        "generatedAtUtc": utc_now(),
        "ipcContractFingerprint": state["ipcContractFingerprint"],
        "main": state["main"],
        "worker": state["worker"],
        "ipcSensitiveFiles": state["ipcSensitiveFiles"],
    }
    write_json(MANIFEST_PATH, manifest)

    stamp = {
        "generatedAtUtc": utc_now(),
        "requiredEntryPoint": "./script/run_all_tests.sh all",
        "command": args.command,
        "manifestPath": MANIFEST_PATH.relative_to(ROOT_DIR).as_posix(),
        "ipcContractFingerprint": state["ipcContractFingerprint"],
        "ipcSensitiveFileCount": len(state["ipcSensitiveFiles"]),
        "fullRegressionReportGeneratedAt": read_report_generated_at(),
    }
    write_json(STAMP_PATH, stamp)

    print(f"wrote IPC contract manifest: {MANIFEST_PATH}")
    print(f"wrote IPC regression stamp: {STAMP_PATH}")
    return 0


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def diff_files(previous_entries, current_entries):
    previous = {entry["path"]: entry["sha256"] for entry in previous_entries}
    current = {entry["path"]: entry["sha256"] for entry in current_entries}

    changed = []
    for path in sorted(set(previous) | set(current)):
        if path not in previous:
            changed.append(("added", path))
        elif path not in current:
            changed.append(("removed", path))
        elif previous[path] != current[path]:
            changed.append(("changed", path))
    return changed


def command_verify_release(_args):
    state = build_state()
    errors = collect_consistency_errors(state, require_worker_generated=False)
    if errors:
        print_errors(errors)
        return 1

    if not MANIFEST_PATH.exists() or not STAMP_PATH.exists():
        print(
            "error: missing IPC regression stamp/manifest; rerun `cd /root/workspace/mediasoup-cpp && ./script/run_all_tests.sh all`",
            file=sys.stderr,
        )
        return 1

    manifest = load_json(MANIFEST_PATH)
    stamp = load_json(STAMP_PATH)
    current_fp = state["ipcContractFingerprint"]
    recorded_fp = stamp.get("ipcContractFingerprint")

    if recorded_fp != manifest.get("ipcContractFingerprint"):
        print(
            "error: IPC regression stamp does not match the recorded manifest; rerun `./script/run_all_tests.sh all`",
            file=sys.stderr,
        )
        return 1

    if current_fp != recorded_fp:
        changed = diff_files(manifest.get("ipcSensitiveFiles", []), state["ipcSensitiveFiles"])
        print(
            "error: IPC-sensitive sources changed since the last successful full regression.",
            file=sys.stderr,
        )
        print(
            "error: rerun `cd /root/workspace/mediasoup-cpp && ./script/run_all_tests.sh all` before building or releasing.",
            file=sys.stderr,
        )
        if changed:
            print("error: changed IPC-sensitive paths:", file=sys.stderr)
            for change_type, path in changed[:20]:
                print(f"  - {change_type}: {path}", file=sys.stderr)
            if len(changed) > 20:
                print(f"  - ... and {len(changed) - 20} more", file=sys.stderr)
        return 1

    print(
        f"IPC release guard passed using stamp from {stamp.get('generatedAtUtc', 'unknown time')}"
    )
    return 0


def build_parser():
    parser = argparse.ArgumentParser(
        description="Guard the mediasoup IPC contract with manifests and full-regression stamps."
    )
    subparsers = parser.add_subparsers(dest="command_name", required=True)

    check_parser = subparsers.add_parser("check-consistency")
    check_parser.add_argument("--require-worker-generated", action="store_true")
    check_parser.add_argument("--warn-changed", action="store_true")
    check_parser.set_defaults(handler=command_check_consistency)

    record_parser = subparsers.add_parser("record-success")
    record_parser.add_argument(
        "--command",
        default="./script/run_all_tests.sh all",
        help="Entry point that produced the successful regression stamp.",
    )
    record_parser.set_defaults(handler=command_record_success)

    verify_parser = subparsers.add_parser("verify-release-readiness")
    verify_parser.set_defaults(handler=command_verify_release)

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    return args.handler(args)


if __name__ == "__main__":
    sys.exit(main())
