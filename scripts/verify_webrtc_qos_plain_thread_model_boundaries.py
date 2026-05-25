#!/usr/bin/env python3
"""Verify the first WebRTC QoS plain client thread-model boundaries.

This static report is intentionally narrower than full P3 acceptance. It
checks the enforceable boundaries that already exist in the first runtime
threading slice: bounded queue semantics, decoded AU sink worker ownership,
play callback isolation, build/test registration and legacy logging/QoS bans.
The aggregate P3 report command also runs two-track synthetic, MP4 decode-loop,
slow worker injection, weak-network and V4L2 capability smoke tests.
"""

import argparse
import json
import os
import platform
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, Iterable, List, Pattern, Tuple


ROOT = Path(__file__).resolve().parents[1]
CLIENT_DIR = ROOT / "client" / "webrtc_qos_plain_client"
CMAKE_FILE = ROOT / "CMakeLists.txt"
QOS_TESTS_SCRIPT = ROOT / "scripts" / "run_qos_tests.sh"
DOC_FILE = ROOT / "docs" / "webrtc-qos-push-play-client-thread-model-design_cn.md"
DEFAULT_JSON = ROOT / "docs" / "generated" / "webrtc-qos-plain-thread-model-boundary-report.json"
DEFAULT_MARKDOWN = ROOT / "docs" / "generated" / "webrtc-qos-plain-thread-model-boundary-report.md"


def rel(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def source_files(root: Path) -> Iterable[Path]:
    suffixes = {".cpp", ".cc", ".cxx", ".h", ".hpp"}
    for path in sorted(root.rglob("*")):
        if path.is_file() and path.suffix in suffixes:
            yield path


def line_for(text: str, offset: int) -> int:
    return text[:offset].count("\n") + 1


def require_file(errors: List[str], path: Path) -> None:
    if not path.is_file():
        errors.append(f"missing file: {rel(path)}")


def require_pattern(
    errors: List[str],
    path: Path,
    text: str,
    pattern: Pattern,
    description: str,
) -> None:
    if not pattern.search(text):
        errors.append(f"{rel(path)}: missing {description}")


def forbid_pattern(
    errors: List[str],
    path: Path,
    text: str,
    pattern: Pattern,
    description: str,
) -> None:
    match = pattern.search(text)
    if match:
        errors.append(f"{rel(path)}:{line_for(text, match.start())}: forbidden {description}: {match.group(0)}")


def make_gate(name: str, description: str, evidence: List[str], failures: List[str]) -> Dict:
    return {
        "name": name,
        "description": description,
        "status": "FAIL" if failures else "PASS",
        "evidence": evidence,
        "failures": failures,
    }


def verify_bounded_queue() -> Dict:
    path = CLIENT_DIR / "common" / "BoundedQueue.h"
    failures = []  # type: List[str]
    evidence = [rel(path)]
    require_file(failures, path)
    if not failures:
        text = read_text(path)
        required = [
            (re.compile(r"class\s+BoundedQueue\b"), "BoundedQueue class"),
            (re.compile(r"explicit\s+BoundedQueue\s*\(\s*size_t\s+capacity\s*\)"), "fixed capacity constructor"),
            (re.compile(r"PushDropOldest\s*\("), "drop-oldest push API"),
            (re.compile(r"Pop\s*\("), "blocking pop API"),
            (re.compile(r"PopFor\s*\("), "timed pop API for heartbeat loops"),
            (re.compile(r"TryPop\s*\("), "non-blocking pop API"),
            (re.compile(r"Close\s*\("), "close API"),
            (re.compile(r"std::condition_variable"), "condition_variable wakeup"),
            (re.compile(r"notify_all\s*\("), "close wakeup"),
            (re.compile(r"notify_one\s*\("), "producer wakeup"),
            (re.compile(r"capacity_"), "stored capacity"),
            (re.compile(r"dropped_"), "drop counter"),
            (re.compile(r"maxDepth_"), "max depth counter"),
        ]
        for pattern, description in required:
            require_pattern(failures, path, text, pattern, description)
    return make_gate(
        "boundedQueue",
        "Cross-thread queues have bounded capacity, close/wakeup semantics and drop counters.",
        evidence,
        failures,
    )


def verify_thread_primitives() -> Dict:
    mailbox = CLIENT_DIR / "common" / "ControlMailbox.h"
    latest = CLIENT_DIR / "common" / "LatestValue.h"
    failures = []  # type: List[str]
    evidence = [rel(mailbox), rel(latest)]
    for path in (mailbox, latest):
        require_file(failures, path)
    if not failures:
        mailbox_text = read_text(mailbox)
        latest_text = read_text(latest)
        mailbox_required = [
            (re.compile(r"class\s+ControlMailbox\b"), "ControlMailbox class"),
            (re.compile(r"eventfd\s*\("), "eventfd wakeup"),
            (re.compile(r"wakeFd\s*\("), "pollable wake fd"),
            (re.compile(r"Post\s*\("), "post API"),
            (re.compile(r"TryPop\s*\("), "owner drain API"),
            (re.compile(r"DrainWakeSignal\s*\("), "wake drain API"),
            (re.compile(r"Close\s*\("), "close API"),
            (re.compile(r"capacity_"), "bounded capacity"),
            (re.compile(r"dropped_"), "drop counter"),
        ]
        latest_required = [
            (re.compile(r"class\s+LatestValue\b"), "LatestValue class"),
            (re.compile(r"Store\s*\("), "store API"),
            (re.compile(r"Load\s*\("), "load API"),
            (re.compile(r"LoadIfNewer\s*\("), "versioned load API"),
            (re.compile(r"version_"), "version counter"),
        ]
        for pattern, description in mailbox_required:
            require_pattern(failures, mailbox, mailbox_text, pattern, description)
        for pattern, description in latest_required:
            require_pattern(failures, latest, latest_text, pattern, description)
    return make_gate(
        "threadPrimitives",
        "Control mailbox and latest-value snapshot primitives exist for owner-loop message passing.",
        evidence,
        failures,
    )


def verify_multi_track_config_boundary() -> Dict:
    args_h = CLIENT_DIR / "common" / "ClientArgs.h"
    args_cpp = CLIENT_DIR / "common" / "ClientArgs.cpp"
    ids_h = CLIENT_DIR / "common" / "ClientIds.h"
    ids_cpp = CLIENT_DIR / "common" / "ClientIds.cpp"
    signaling_cpp = CLIENT_DIR / "push" / "PushSignalingSession.cpp"
    runtime_cpp = CLIENT_DIR / "push" / "WebRtcQosPushRuntime.cpp"
    failures = []  # type: List[str]
    evidence = [rel(path) for path in (args_h, args_cpp, ids_h, ids_cpp, signaling_cpp, runtime_cpp)]
    for path in (args_h, args_cpp, ids_h, ids_cpp, signaling_cpp, runtime_cpp):
        require_file(failures, path)
    if not failures:
        checks = [
            (args_h, read_text(args_h), [
                (re.compile(r"struct\s+PushTrackOptions\b"), "PushTrackOptions"),
                (re.compile(r"std::vector<PushTrackOptions>\s+tracks"), "push track list"),
                (re.compile(r"v4l2Device"), "per-track V4L2 device option"),
            ]),
            (args_cpp, read_text(args_cpp), [
                (re.compile(r"ParseTrackOption"), "--track parser"),
                (re.compile(r"\"--track\""), "--track CLI option"),
                (re.compile(r"source=v4l2"), "--track V4L2 source usage"),
                (re.compile(r"device=<path>|device=/dev/videoN"), "--track V4L2 device usage"),
                (re.compile(r"duplicate --track ssrc"), "duplicate SSRC validation"),
            ]),
            (ids_h, read_text(ids_h), [
                (re.compile(r"struct\s+VideoTrackSessionParams\b"), "VideoTrackSessionParams"),
                (re.compile(r"struct\s+VideoSessionParams\b"), "VideoSessionParams"),
                (re.compile(r"MakeVideoSessionConfig"), "multi-track session builder declaration"),
            ]),
            (ids_cpp, read_text(ids_cpp), [
                (re.compile(r"MakeVideoSessionConfig"), "multi-track session builder"),
                (re.compile(r"for\s*\(\s*size_t\s+index\s*="), "track loop"),
                (re.compile(r"session\.video_tracks\.push_back"), "video track append"),
            ]),
            (signaling_cpp, read_text(signaling_cpp), [
                (re.compile(r"videoSsrcs"), "plainPublish videoSsrcs request"),
                (re.compile(r"parsed\.videoTracks"), "multi-track publish response"),
            ]),
            (runtime_cpp, read_text(runtime_cpp), [
                (re.compile(r"VideoSessionParams\s+sessionParams"), "push runtime multi-track session params"),
                (re.compile(r"publishInfo_\.videoTracks"), "push runtime published tracks"),
                (re.compile(r"MakeVideoSessionConfig"), "push runtime multi-track session builder"),
                (re.compile(r"tracksBySsrc"), "runtime maps per-track options by SSRC"),
                (re.compile(r"trackOptions->v4l2Device"), "runtime applies per-track V4L2 device"),
            ]),
        ]
        for path, text, required in checks:
            for pattern, description in required:
                require_pattern(failures, path, text, pattern, description)
    return make_gate(
        "multiTrackConfigBoundary",
        "Push CLI/signaling/session plumbing can represent multiple video tracks.",
        evidence,
        failures,
    )


def verify_play_multi_track_config_boundary() -> Dict:
    args_h = CLIENT_DIR / "common" / "ClientArgs.h"
    args_cpp = CLIENT_DIR / "common" / "ClientArgs.cpp"
    signaling_h = CLIENT_DIR / "play" / "PlaySignalingSession.h"
    signaling_cpp = CLIENT_DIR / "play" / "PlaySignalingSession.cpp"
    main_cpp = CLIENT_DIR / "play" / "main.cpp"
    runtime_h = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.h"
    runtime_cpp = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.cpp"
    failures = []  # type: List[str]
    evidence = [rel(path) for path in (args_h, args_cpp, signaling_h, signaling_cpp, main_cpp, runtime_h, runtime_cpp)]
    for path in (args_h, args_cpp, signaling_h, signaling_cpp, main_cpp, runtime_h, runtime_cpp):
        require_file(failures, path)
    if not failures:
        checks = [
            (args_h, read_text(args_h), [
                (re.compile(r"videoConsumerCount"), "play video consumer count option"),
            ]),
            (args_cpp, read_text(args_cpp), [
                (re.compile(r"\"--video-consumer-count\""), "play --video-consumer-count CLI option"),
                (re.compile(r"--video-consumer-count must be in \[1,16\]"), "play video consumer count validation"),
            ]),
            (signaling_h, read_text(signaling_h), [
                (re.compile(r"std::vector<ConsumerInfo>\s+TakeSelectedConsumers"), "multi-consumer selection declaration"),
            ]),
            (signaling_cpp, read_text(signaling_cpp), [
                (re.compile(r"TakeSelectedConsumers"), "multi-consumer selection implementation"),
                (re.compile(r"selected\.push_back"), "selected consumers vector"),
            ]),
            (main_cpp, read_text(main_cpp), [
                (re.compile(r"videoConsumerCount"), "play main requested consumer count"),
                (re.compile(r"TakeSelectedConsumers"), "play main selects multiple consumers"),
                (re.compile(r"std::vector<webrtc_qos_plain::ConsumerInfo>\s+consumers"), "play main stores multiple consumers"),
            ]),
            (runtime_h, read_text(runtime_h), [
                (re.compile(r"std::vector<ConsumerInfo>\s+consumerInfos_"), "play runtime stores multiple consumers"),
            ]),
            (runtime_cpp, read_text(runtime_cpp), [
                (re.compile(r"VideoSessionParams\s+sessionParams"), "play runtime multi-track session params"),
                (re.compile(r"consumerInfos_"), "play runtime uses consumer list"),
                (re.compile(r"MakeVideoSessionConfig"), "play runtime multi-track session builder"),
                (re.compile(r"play_track_metrics"), "play per-track metrics log"),
                (re.compile(r"play_track_final"), "play per-track final log"),
            ]),
        ]
        for path, text, required in checks:
            for pattern, description in required:
                require_pattern(failures, path, text, pattern, description)
    return make_gate(
        "playMultiTrackConfigBoundary",
        "Play CLI/signaling/runtime can select multiple video consumers and build a multi-track session.",
        evidence,
        failures,
    )


def verify_encoded_au_boundary() -> Dict:
    path = CLIENT_DIR / "push" / "EncodedAccessUnit.h"
    failures = []  # type: List[str]
    evidence = [rel(path)]
    require_file(failures, path)
    if not failures:
        text = read_text(path)
        required = [
            (re.compile(r"struct\s+EncodedAccessUnit\b"), "encoded AU item"),
            (re.compile(r"std::vector<uint8_t>\s+bytes"), "owned AU bytes"),
            (re.compile(r"CopyEncodedAccessUnit\s*\("), "copy helper"),
            (re.compile(r"ToAnnexBAccessUnitView\s*\("), "SDK view helper"),
            (re.compile(r"out->bytes\s*=\s*source\.bytes"), "source bytes copied into owned memory"),
            (re.compile(r"captureTimeUs"), "capture timestamp"),
            (re.compile(r"TransportIds"), "track transport ids"),
        ]
        for pattern, description in required:
            require_pattern(failures, path, text, pattern, description)
    return make_gate(
        "encodedAuBoundary",
        "Push-side cross-thread encoded AU items own bytes before being viewed by the SDK.",
        evidence,
        failures,
    )


def verify_push_sdk_transport_thread() -> Dict:
    header = CLIENT_DIR / "push" / "PushSdkTransportThread.h"
    source = CLIENT_DIR / "push" / "PushSdkTransportThread.cpp"
    source_worker_header = CLIENT_DIR / "push" / "PushTrackSourceWorker.h"
    source_worker = CLIENT_DIR / "push" / "PushTrackSourceWorker.cpp"
    raw_frame = CLIENT_DIR / "push" / "RawVideoFrame.h"
    capture_worker_header = CLIENT_DIR / "push" / "V4L2RawFrameCaptureWorker.h"
    capture_worker = CLIENT_DIR / "push" / "V4L2RawFrameCaptureWorker.cpp"
    encode_worker_header = CLIENT_DIR / "push" / "RawFrameEncodeWorker.h"
    encode_worker = CLIENT_DIR / "push" / "RawFrameEncodeWorker.cpp"
    runtime = CLIENT_DIR / "push" / "WebRtcQosPushRuntime.cpp"
    failures = []  # type: List[str]
    evidence = [
        rel(header),
        rel(source),
        rel(source_worker_header),
        rel(source_worker),
        rel(raw_frame),
        rel(capture_worker_header),
        rel(capture_worker),
        rel(encode_worker_header),
        rel(encode_worker),
        rel(runtime),
    ]
    for path in (header, source, source_worker_header, source_worker, raw_frame, capture_worker_header, capture_worker, encode_worker_header, encode_worker, runtime):
        require_file(failures, path)
    if not failures:
        h_text = read_text(header)
        cpp_text = read_text(source)
        worker_h_text = read_text(source_worker_header)
        worker_text = read_text(source_worker)
        raw_frame_text = read_text(raw_frame)
        capture_h_text = read_text(capture_worker_header)
        capture_text = read_text(capture_worker)
        encode_h_text = read_text(encode_worker_header)
        encode_text = read_text(encode_worker)
        runtime_text = read_text(runtime)
        header_required = [
            (re.compile(r"class\s+PushSdkTransportThread\b"), "PushSdkTransportThread class"),
            (re.compile(r"struct\s+PushSdkTrackQueueMetrics\b"), "per-track queue metrics"),
            (re.compile(r"struct\s+TrackQueueState"), "per-track queue state"),
            (re.compile(r"std::vector<std::unique_ptr<TrackQueueState>>\s+trackQueues_"), "per-track encoded AU queues"),
            (re.compile(r"LatestValue<webrtc_qos::EncoderAdaptation>\s+adaptation_"), "adaptation snapshot"),
            (re.compile(r"LatestValue<webrtc_qos::QosSnapshot>\s+snapshot_"), "QoS snapshot"),
            (re.compile(r"std::thread\s+thread_"), "owner thread"),
            (re.compile(r"Enqueue\s*\("), "enqueue API"),
            (re.compile(r"encoderAdaptation\s*\("), "adaptation API"),
            (re.compile(r"metrics\s*\(\)\s+const"), "metrics API"),
        ]
        source_required = [
            (re.compile(r"PlainUdpTransport\s+udp"), "UDP owned in SDK thread"),
            (re.compile(r"CreateVideoPushClient"), "SDK facade created in owner thread"),
            (re.compile(r"push->Start\s*\("), "SDK start in owner thread"),
            (re.compile(r"DrainEncodedQueues"), "round-robin per-track drain"),
            (re.compile(r"FindTrackQueue"), "track queue lookup"),
            (re.compile(r"push[.-]>?PushAnnexBAccessUnit"), "SDK push in owner thread"),
            (re.compile(r"push[.-]>?OnTransportFeedback|push\.OnTransportFeedback"), "SDK feedback in owner thread"),
            (re.compile(r"push->Process"), "SDK process in owner thread"),
            (re.compile(r"push->Stop\s*\("), "SDK stop in owner thread"),
            (re.compile(r"adaptation_\.Store"), "adaptation snapshot publish"),
            (re.compile(r"snapshot_\.Store"), "QoS snapshot publish"),
            (re.compile(r"GetTrackEncoderAdaptation"), "per-track adaptation snapshot publish"),
            (re.compile(r"GetTrackQosSnapshot"), "per-track QoS snapshot publish"),
            (re.compile(r"push_sdk_transport_thread_started"), "owner thread start log"),
            (re.compile(r"push_sdk_transport_thread_stopped"), "owner thread final log"),
        ]
        runtime_required = [
            (re.compile(r"PushSdkTransportThread\s+sdkThread"), "runtime uses SDK owner thread"),
            (re.compile(r"PushTrackSourceWorker"), "runtime starts per-track source workers"),
            (re.compile(r"std::vector<std::unique_ptr<PushTrackSourceWorker>>\s+sourceWorkers"), "runtime stores source workers"),
            (re.compile(r"sourceWorker->Stop\s*\("), "runtime stops source workers"),
            (re.compile(r"sdkMetricsForControl\.tracks"), "runtime reads per-track adaptation snapshots"),
            (re.compile(r"worker->StoreEncoderAdaptation"), "runtime posts adaptation snapshots to source workers"),
            (re.compile(r"encoder_track_metrics"), "runtime logs per-track encoder metrics"),
            (re.compile(r"push_track_metrics"), "runtime logs per-track push metrics"),
            (re.compile(r"push_track_final"), "runtime logs final per-track push metrics"),
            (re.compile(r"trackOptions->v4l2Device"), "runtime supports per-track V4L2 devices"),
            (re.compile(r"sdkThread\.Stop\s*\("), "runtime stops owner thread"),
        ]
        runtime_forbidden = [
            (re.compile(r"v4l2_multi_track_not_supported_yet"), "explicit V4L2 multi-track rejection"),
            (re.compile(r"\bCreateVideoPushClient\b"), "SDK facade creation in push runtime"),
            (re.compile(r"push->Start\s*\("), "SDK start in push runtime"),
            (re.compile(r"push->PushAnnexBAccessUnit"), "SDK push in push runtime"),
            (re.compile(r"push->OnTransportFeedback"), "SDK feedback in push runtime"),
            (re.compile(r"push->Process"), "SDK process in push runtime"),
            (re.compile(r"push->Stop\s*\("), "SDK stop in push runtime"),
            (re.compile(r"PlainUdpTransport\s+udp"), "UDP ownership in push runtime"),
            (re.compile(r"sdkThread\.Enqueue\s*\("), "direct encoded AU enqueue in push runtime"),
            (re.compile(r"session\.video_tracks\.front\(\)\.ids"), "single-track-only enqueue"),
        ]
        worker_required = [
            (re.compile(r"class\s+PushTrackSourceWorker\b"), "source worker class"),
            (re.compile(r"std::thread\s+thread_"), "source worker thread"),
            (re.compile(r"PushSdkTransportThread\*\s+sdkThread_"), "source worker posts to SDK owner"),
            (re.compile(r"LatestValue<webrtc_qos::EncoderAdaptation>\s+adaptation_"), "adaptation snapshot mailbox"),
            (re.compile(r"LatestValue<RealtimeH264SourceMetrics>\s+sourceMetrics_"), "source metrics snapshot"),
            (re.compile(r"BoundedQueue<RawVideoFrame>"), "raw frame queue in source worker"),
            (re.compile(r"V4L2RawFrameCaptureWorker"), "source worker owns V4L2 capture worker"),
            (re.compile(r"RawFrameEncodeWorker"), "source worker owns raw frame encode worker"),
            (re.compile(r"CopyEncodedAccessUnit"), "worker copies owned AU bytes"),
            (re.compile(r"sdkThread_->Enqueue"), "worker queues AU to SDK owner"),
            (re.compile(r"push_track_source_worker_started"), "source worker start log"),
            (re.compile(r"push_track_source_worker_stopped"), "source worker stop log"),
        ]
        raw_split_required = [
            (raw_frame, raw_frame_text, [
                (re.compile(r"struct\s+RawVideoFrame\b"), "owned raw video frame item"),
                (re.compile(r"std::vector<uint8_t>\s+yuv420p"), "raw frame owns YUV bytes"),
                (re.compile(r"captureTimeUs"), "raw frame capture timestamp"),
                (re.compile(r"mediaTimeUs"), "raw frame media timestamp"),
            ]),
		(capture_worker_header, capture_h_text, [
			(re.compile(r"class\s+V4L2RawFrameCaptureWorker\b"), "V4L2 capture worker class"),
			(re.compile(r"BoundedQueue<RawVideoFrame>"), "capture worker outputs raw frame queue"),
			(re.compile(r"InputFormat"), "capture worker owns V4L2 input"),
			(re.compile(r"Decoder"), "capture worker owns V4L2 decoder"),
			(re.compile(r"std::thread\s+thread_"), "capture worker thread"),
			(re.compile(r"openTimeoutMs"), "capture worker open deadline config"),
			(re.compile(r"readTimeoutMs"), "capture worker read deadline config"),
			(re.compile(r"interruptDeadlineUs_"), "capture worker FFmpeg interrupt deadline"),
		]),
		(capture_worker, capture_text, [
			(re.compile(r"av_find_input_format\(\"v4l2\"\)"), "capture worker opens V4L2"),
			(re.compile(r"OpenWithFormatInterruptible"), "capture worker uses interruptible FFmpeg input"),
			(re.compile(r"interruptDeadlineUs_\.store"), "capture worker sets FFmpeg interrupt deadlines"),
			(re.compile(r"rawQueue_->PushDropOldest"), "capture worker bounded raw queue push"),
			(re.compile(r"v4l2_capture_worker_started"), "capture worker start log"),
			(re.compile(r"v4l2_capture_worker_stopped"), "capture worker final log"),
            ]),
            (encode_worker_header, encode_h_text, [
                (re.compile(r"class\s+RawFrameEncodeWorker\b"), "raw frame encode worker class"),
                (re.compile(r"BoundedQueue<RawVideoFrame>"), "encode worker consumes raw frame queue"),
                (re.compile(r"PushSdkTransportThread\*\s+sdkThread_"), "encode worker posts to SDK owner"),
                (re.compile(r"LatestValue<webrtc_qos::EncoderAdaptation>\s+adaptation_"), "encode worker adaptation snapshot"),
                (re.compile(r"std::thread\s+thread_"), "encode worker thread"),
            ]),
            (encode_worker, encode_text, [
                (re.compile(r"Encoder::Create\(AV_CODEC_ID_H264"), "encode worker owns H264 encoder"),
                (re.compile(r"rawQueue_->PopFor"), "encode worker consumes raw queue"),
                (re.compile(r"sdkThread_->Enqueue"), "encode worker enqueues encoded AU to SDK"),
                (re.compile(r"raw_frame_encode_worker_started"), "encode worker start log"),
                (re.compile(r"raw_frame_encode_worker_stopped"), "encode worker final log"),
            ]),
        ]
        for pattern, description in header_required:
            require_pattern(failures, header, h_text, pattern, description)
        for pattern, description in source_required:
            require_pattern(failures, source, cpp_text, pattern, description)
        worker_header_descriptions = {
            "source worker class",
            "source worker thread",
            "source worker posts to SDK owner",
            "adaptation snapshot mailbox",
            "source metrics snapshot",
        }
        for pattern, description in worker_required:
            path = source_worker_header if description in worker_header_descriptions else source_worker
            text = worker_h_text if path == source_worker_header else worker_text
            require_pattern(failures, path, text, pattern, description)
        for path, text, required in raw_split_required:
            for pattern, description in required:
                require_pattern(failures, path, text, pattern, description)
        for pattern, description in runtime_required:
            require_pattern(failures, runtime, runtime_text, pattern, description)
        for pattern, description in runtime_forbidden:
            forbid_pattern(failures, runtime, runtime_text, pattern, description)
        forbid_pattern(
            failures,
            source_worker,
            worker_text,
            re.compile(r"v4l2Source_|V4L2H264Source"),
            "fused V4L2 source in PushTrackSourceWorker runtime path")
        forbid_pattern(
            failures,
            source_worker_header,
            worker_h_text,
            re.compile(r"V4L2H264Source"),
            "fused V4L2 source in PushTrackSourceWorker header")
    return make_gate(
        "pushSdkTransportOwnerThread",
        "Push-side VideoPushClient and UDP socket are owned by PushSdkTransportThread.",
        evidence,
        failures,
    )


def verify_play_sdk_transport_thread() -> Dict:
    header = CLIENT_DIR / "play" / "PlaySdkTransportThread.h"
    source = CLIENT_DIR / "play" / "PlaySdkTransportThread.cpp"
    runtime = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.cpp"
    failures = []  # type: List[str]
    evidence = [rel(header), rel(source), rel(runtime)]
    for path in (header, source, runtime):
        require_file(failures, path)
    if not failures:
        h_text = read_text(header)
        cpp_text = read_text(source)
        runtime_text = read_text(runtime)
        header_required = [
            (re.compile(r"class\s+PlaySdkTransportThread\b"), "PlaySdkTransportThread class"),
            (re.compile(r"struct\s+PlaySdkTrackMetrics\b"), "per-track play metrics"),
            (re.compile(r"PlainUdpTransport\s+udp"), "bound UDP socket moved into owner thread config"),
            (re.compile(r"LatestValue<webrtc_qos::QosSnapshot>\s+snapshot_"), "QoS snapshot"),
            (re.compile(r"trackSnapshots_"), "per-track QoS snapshots"),
            (re.compile(r"std::thread\s+thread_"), "owner thread"),
            (re.compile(r"metrics\s*\(\)\s+const"), "metrics API"),
        ]
        source_required = [
            (re.compile(r"PlainUdpTransport\s+udp\s*=\s*std::move"), "UDP owned in SDK thread"),
            (re.compile(r"CreateVideoPlayClient"), "SDK facade created in owner thread"),
            (re.compile(r"play->Start\s*\("), "SDK start in owner thread"),
            (re.compile(r"play[.-]>?OnRtpPacket|play\.OnRtpPacket"), "SDK RTP input in owner thread"),
            (re.compile(r"play[.-]>?OnRtcpPacket|play\.OnRtcpPacket"), "SDK RTCP input in owner thread"),
            (re.compile(r"play->Process"), "SDK process in owner thread"),
            (re.compile(r"play->Stop\s*\("), "SDK stop in owner thread"),
            (re.compile(r"snapshot_\.Store"), "QoS snapshot publish"),
            (re.compile(r"GetTrackQosSnapshot"), "per-track QoS snapshot publish"),
            (re.compile(r"play_sdk_transport_thread_started"), "owner thread start log"),
            (re.compile(r"play_sdk_transport_thread_stopped"), "owner thread final log"),
        ]
        runtime_required = [
            (re.compile(r"PlaySdkTransportThread\s+sdkThread"), "runtime uses SDK owner thread"),
            (re.compile(r"sdkThread\.metrics\s*\("), "runtime reads SDK metrics"),
            (re.compile(r"sdkMetrics\.tracks"), "runtime reads per-track SDK metrics"),
            (re.compile(r"sdkThread\.Stop\s*\("), "runtime stops owner thread"),
        ]
        runtime_forbidden = [
            (re.compile(r"\bCreateVideoPlayClient\b"), "SDK facade creation in play runtime"),
            (re.compile(r"play->Start\s*\("), "SDK start in play runtime"),
            (re.compile(r"play->OnRtpPacket"), "SDK RTP input in play runtime"),
            (re.compile(r"play->OnRtcpPacket"), "SDK RTCP input in play runtime"),
            (re.compile(r"play->Process"), "SDK process in play runtime"),
            (re.compile(r"play->Stop\s*\("), "SDK stop in play runtime"),
        ]
        for pattern, description in header_required:
            require_pattern(failures, header, h_text, pattern, description)
        for pattern, description in source_required:
            require_pattern(failures, source, cpp_text, pattern, description)
        for pattern, description in runtime_required:
            require_pattern(failures, runtime, runtime_text, pattern, description)
        for pattern, description in runtime_forbidden:
            forbid_pattern(failures, runtime, runtime_text, pattern, description)
    return make_gate(
        "playSdkTransportOwnerThread",
        "Play-side VideoPlayClient and UDP socket are owned by PlaySdkTransportThread.",
        evidence,
        failures,
    )


def verify_decoded_sink_worker() -> Dict:
    header = CLIENT_DIR / "play" / "DecodedAuSinkWorker.h"
    source = CLIENT_DIR / "play" / "DecodedAuSinkWorker.cpp"
    failures = []  # type: List[str]
    evidence = [rel(header), rel(source)]
    for path in (header, source):
        require_file(failures, path)
    if not failures:
        h_text = read_text(header)
        cpp_text = read_text(source)
        header_required = [
            (re.compile(r"struct\s+DecodedAccessUnit\b"), "owned decoded AU item"),
            (re.compile(r"std::vector<uint8_t>\s+bytes"), "owned AU bytes"),
            (re.compile(r"BoundedQueue<DecodedAccessUnit>\s+queue_"), "bounded worker queue"),
            (re.compile(r"std::thread\s+thread_"), "worker thread"),
            (re.compile(r"std::atomic<"), "atomic counters"),
            (re.compile(r"lastHeartbeatUs_"), "heartbeat counter"),
            (re.compile(r"loopGapMaxUs_"), "loop gap counter"),
            (re.compile(r"stopReason_"), "stop reason"),
            (re.compile(r"trackId_"), "per-track sink identity"),
            (re.compile(r"senderSsrc_"), "per-track sender SSRC"),
            (re.compile(r"trackName_"), "per-track sink name"),
            (re.compile(r"writtenAccessUnitsByTrack_"), "per-track written AU metrics"),
            (re.compile(r"enqueuedAccessUnitsByTrack_"), "per-track enqueued AU metrics"),
            (re.compile(r"Enqueue\s*\("), "enqueue API"),
            (re.compile(r"metrics\s*\(\)\s+const"), "metrics snapshot API"),
        ]
        source_required = [
            (re.compile(r"thread_\s*=\s*std::thread"), "worker thread startup"),
            (re.compile(r"queue_\.Close\s*\("), "stop closes queue"),
            (re.compile(r"accessUnit\.ids\.track_id\s*!=\s*trackId_"), "wrong-track reject"),
            (re.compile(r"item\.bytes\.assign\s*\(\s*accessUnit\.bytes"), "callback copies SDK view bytes"),
            (re.compile(r"queue_\.PushDropOldest\s*\("), "enqueue uses bounded queue policy"),
            (re.compile(r"while\s*\(\s*true\s*\)"), "worker drain loop"),
            (re.compile(r"PopFor\s*\("), "timed queue wait"),
            (re.compile(r"lastHeartbeatUs_\.store"), "heartbeat update"),
            (re.compile(r"loopGapMaxUs_"), "loop gap update"),
            (re.compile(r"StoreStopReason"), "stop reason update"),
            (re.compile(r"AnnexBSink\s+sink"), "Annex-B sink owned by worker"),
            (re.compile(r"FfmpegDecodeSink\s+decodeSink"), "FFmpeg decode sink owned by worker"),
            (re.compile(r"decoded_sink_worker_started"), "worker start log"),
            (re.compile(r"decoded_sink_worker_stopped"), "worker final log"),
            (re.compile(r"decoded_sink_track_stopped"), "per-track sink final log"),
        ]
        for pattern, description in header_required:
            require_pattern(failures, header, h_text, pattern, description)
        for pattern, description in source_required:
            require_pattern(failures, source, cpp_text, pattern, description)
    return make_gate(
        "decodedSinkWorkerOwnership",
        "Play sink and decode work run in a dedicated worker and own copied AU memory.",
        evidence,
        failures,
    )


def verify_play_callback_isolation() -> Dict:
    runtime_cpp = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.cpp"
    runtime_h = CLIENT_DIR / "play" / "WebRtcQosPlayRuntime.h"
    owner_cpp = CLIENT_DIR / "play" / "PlaySdkTransportThread.cpp"
    failures = []  # type: List[str]
    evidence = [rel(runtime_cpp), rel(runtime_h), rel(owner_cpp)]
    for path in (runtime_cpp, runtime_h, owner_cpp):
        require_file(failures, path)
    if not failures:
        cpp_text = read_text(runtime_cpp)
        h_text = read_text(runtime_h)
        owner_text = read_text(owner_cpp)
        required = [
            (re.compile(r"std::vector<SinkWorkerEntry>\s+sinkWorkers"), "runtime per-track sink worker vector"),
            (re.compile(r"std::unordered_map<uint32_t,\s*DecodedAuSinkWorker\*>\s+sinkWorkersByTrack"), "runtime track-to-sink map"),
            (re.compile(r"std::make_unique<DecodedAuSinkWorker>"), "runtime creates sink worker per track"),
            (re.compile(r"decodedAccessUnitOutput\s*="), "runtime decoded callback config"),
            (re.compile(r"sinkWorkersByTrack\.find\s*\(\s*accessUnit\.ids\.track_id\s*\)"), "callback dispatches by track id"),
            (re.compile(r"it->second->Enqueue\s*\(\s*accessUnit\s*\)"), "callback only enqueues AU to matching track"),
            (re.compile(r"AggregateSinkMetrics\s*\("), "aggregate metrics built from per-track sinks"),
            (re.compile(r"sinkQueueDepth"), "sink queue depth metric"),
            (re.compile(r"sinkQueueDroppedAu"), "sink queue drop metric"),
            (re.compile(r"sinkHeartbeatAgeMs"), "sink heartbeat metric"),
            (re.compile(r"sinkLoopGapMaxUs"), "sink loop gap metric"),
            (re.compile(r"sinkStopReason"), "sink stop reason metric"),
            (re.compile(r"StopSinkWorkers\s*\("), "explicit workers stop"),
            (re.compile(r"play_track_qoe_metrics"), "per-track QoE periodic metrics"),
            (re.compile(r"play_track_qoe_final"), "per-track QoE final metrics"),
        ]
        owner_required = [
            (re.compile(r"decoded_access_unit_output\s*=\s*config_\.decodedAccessUnitOutput"), "SDK decoded callback bound in play owner thread"),
        ]
        for pattern, description in required:
            require_pattern(failures, runtime_cpp, cpp_text, pattern, description)
        for pattern, description in owner_required:
            require_pattern(failures, owner_cpp, owner_text, pattern, description)
        forbidden_cpp = [
            (re.compile(r"\bAnnexBSink\b"), "direct AnnexBSink use in play runtime"),
            (re.compile(r"\bFfmpegDecodeSink\b"), "direct FfmpegDecodeSink use in play runtime"),
            (re.compile(r"\.Write\s*\("), "direct sink write in play runtime"),
            (re.compile(r"\.Decode\s*\("), "direct decode in play runtime"),
        ]
        forbidden_h = [
            (re.compile(r"play/AnnexBSink\.h"), "AnnexBSink include in runtime header"),
            (re.compile(r"play/FfmpegDecodeSink\.h"), "FfmpegDecodeSink include in runtime header"),
        ]
        for pattern, description in forbidden_cpp:
            forbid_pattern(failures, runtime_cpp, cpp_text, pattern, description)
        for pattern, description in forbidden_h:
            forbid_pattern(failures, runtime_h, h_text, pattern, description)
    return make_gate(
        "playCallbackIsolation",
        "SDK decoded callback does not perform file IO, FFmpeg decode or QoE work inline.",
        evidence,
        failures,
    )


def verify_worker_facade_boundaries() -> Dict:
    worker_files = [
        CLIENT_DIR / "common" / "BoundedQueue.h",
        CLIENT_DIR / "common" / "ControlMailbox.h",
        CLIENT_DIR / "common" / "LatestValue.h",
        CLIENT_DIR / "push" / "EncodedAccessUnit.h",
        CLIENT_DIR / "play" / "DecodedAuSinkWorker.h",
        CLIENT_DIR / "play" / "DecodedAuSinkWorker.cpp",
    ]
    failures = []  # type: List[str]
    evidence = [rel(path) for path in worker_files]
    forbidden = [
        (re.compile(r"\bCreateVideoPushClient\b"), "SDK push facade creation in worker"),
        (re.compile(r"\bCreateVideoPlayClient\b"), "SDK play facade creation in worker"),
        (re.compile(r"\bVideoPushClient\b"), "SDK push facade type in worker"),
        (re.compile(r"\bVideoPlayClient\b"), "SDK play facade type in worker"),
        (re.compile(r"\bPlainUdpTransport\b"), "UDP transport ownership in worker"),
        (re.compile(r"\bWsClient\b"), "signaling ownership in worker"),
    ]
    for path in worker_files:
        require_file(failures, path)
        if not path.is_file():
            continue
        text = read_text(path)
        for pattern, description in forbidden:
            forbid_pattern(failures, path, text, pattern, description)
    return make_gate(
        "workerFacadeBoundary",
        "Worker files do not own SDK facade, UDP transport or signaling objects.",
        evidence,
        failures,
    )


def verify_logging_and_legacy_boundaries() -> Dict:
    failures = []  # type: List[str]
    evidence = [rel(CLIENT_DIR)]
    forbidden = [
        (re.compile(r"\bH264Packetizer\b"), "legacy H264 packetizer"),
        (re.compile(r"\bPacketizeAnnexB\b"), "legacy Annex-B packetizer"),
        (re.compile(r"\bPublisherQosController\b"), "legacy browser publisher QoS controller"),
        (re.compile(r"#\s*include\s*[<\"]common/media/rtp/"), "shared RTP packetizer headers"),
        (re.compile(r"\bmedia::rtp::"), "shared RTP packetizer namespace"),
        (re.compile(r"\bstd::cout\b"), "stdout logging"),
        (re.compile(r"\bprintf\s*\("), "printf logging"),
        (re.compile(r"\bfprintf\s*\("), "fprintf logging"),
        (re.compile(r"\bputs\s*\("), "puts logging"),
    ]
    if not CLIENT_DIR.is_dir():
        failures.append(f"missing client directory: {rel(CLIENT_DIR)}")
    else:
        for path in source_files(CLIENT_DIR):
            text = read_text(path)
            for pattern, description in forbidden:
                forbid_pattern(failures, path, text, pattern, description)
    return make_gate(
        "loggingAndLegacyBoundaries",
        "Thread-model client paths keep legacy QoS/packetizer code and stdout logging out.",
        evidence,
        failures,
    )


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


def verify_build_and_tests() -> Dict:
    test_file = ROOT / "tests" / "test_webrtc_qos_decode_sink.cpp"
    failures = []  # type: List[str]
    evidence = [rel(CMAKE_FILE), rel(test_file)]
    for path in (CMAKE_FILE, test_file):
        require_file(failures, path)
    if not failures:
        cmake_text = read_text(CMAKE_FILE)
        test_text = read_text(test_file)
        primitive_test = ROOT / "tests" / "test_webrtc_qos_thread_model_primitives.cpp"
        require_file(failures, primitive_test)
        play_block = target_block(cmake_text, "webrtc-qos-plain-play-client")
        if not play_block:
            failures.append("CMakeLists.txt: missing webrtc-qos-plain-play-client target block")
        elif "client/webrtc_qos_plain_client/play/DecodedAuSinkWorker.cpp" not in play_block:
            failures.append("CMakeLists.txt: play target missing DecodedAuSinkWorker.cpp")
        if "mediasoup_webrtc_qos_plain_unit_tests" not in cmake_text:
            failures.append("CMakeLists.txt: missing mediasoup_webrtc_qos_plain_unit_tests target")
        if "client/webrtc_qos_plain_client/play/DecodedAuSinkWorker.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing DecodedAuSinkWorker.cpp")
        if "client/webrtc_qos_plain_client/push/PushSdkTransportThread.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing PushSdkTransportThread.cpp")
        if "client/webrtc_qos_plain_client/push/PushTrackSourceWorker.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing PushTrackSourceWorker.cpp")
        if "client/webrtc_qos_plain_client/push/RawFrameEncodeWorker.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing RawFrameEncodeWorker.cpp")
        if "client/webrtc_qos_plain_client/push/V4L2RawFrameCaptureWorker.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing V4L2RawFrameCaptureWorker.cpp")
        if "client/webrtc_qos_plain_client/push/V4L2H264Source.cpp" in cmake_text:
            failures.append("CMakeLists.txt: fused V4L2H264Source.cpp must not be registered in current runtime/test targets")
        if "client/webrtc_qos_plain_client/play/PlaySdkTransportThread.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit/build targets missing PlaySdkTransportThread.cpp")
        if "tests/test_webrtc_qos_thread_model_primitives.cpp" not in cmake_text:
            failures.append("CMakeLists.txt: unit target missing thread model primitives tests")
        required_tests = [
            (re.compile(r"BoundedQueueDropsOldestAndCloses"), "BoundedQueue close/drop test"),
            (re.compile(r"BoundedQueuePopForTimesOutWithoutClosing"), "BoundedQueue timed pop test"),
            (re.compile(r"DecodedAuSinkWorkerWritesAndDecodesAsynchronously"), "decoded sink worker async test"),
            (re.compile(r"DecodedAuSinkWorkerCopiesAccessUnitBytesBeforeReturning"), "decoded AU owned-copy test"),
            (re.compile(r"DecodedAuSinkWorkerRejectsWrongTrackAndReportsIdentity"), "per-track sink routing test"),
        ]
        for pattern, description in required_tests:
            require_pattern(failures, test_file, test_text, pattern, description)
        if primitive_test.is_file():
            primitive_text = read_text(primitive_test)
            primitive_required = [
                (re.compile(r"ControlMailboxSignalsFdAndTracksCapacity"), "ControlMailbox fd/capacity test"),
                (re.compile(r"ControlMailboxCloseRejectsPostsAndWakesOwner"), "ControlMailbox close wake test"),
                (re.compile(r"LatestValueTracksVersions"), "LatestValue version test"),
                (re.compile(r"VideoSessionConfigBuildsMultipleTracks"), "multi-track session config test"),
                (re.compile(r"EncodedAccessUnitOwnsCopiedBytes"), "push encoded AU owned-copy test"),
                (re.compile(r"RawVideoFrameOwnsCopiedYuvBytes"), "push raw frame owned-copy test"),
                (re.compile(r"PushSdkTransportThreadOwnsSdkAndUdpLoop"), "push SDK owner thread test"),
                (re.compile(r"PushSdkTransportThreadDrainsPerTrackQueues"), "push SDK per-track queue test"),
                (re.compile(r"PushTrackSourceWorkerEncodesAndQueuesOnWorkerThread"), "push source worker dynamic test"),
                (re.compile(r"PushTrackSourceWorkersEncodeTwoTracksIndependently"), "push source workers two-track dynamic test"),
                (re.compile(r"RawFrameEncodeWorkerConsumesRawQueueAndPushesSdk"), "raw frame encode worker dynamic test"),
                (re.compile(r"PlaySdkTransportThreadOwnsSdkAndUdpLoop"), "play SDK owner thread test"),
                (re.compile(r"PlaySdkTransportThreadExposesPerTrackSnapshots"), "play SDK per-track snapshot test"),
            ]
            for pattern, description in primitive_required:
                require_pattern(failures, primitive_test, primitive_text, pattern, description)
    return make_gate(
        "buildAndUnitCoverage",
        "The first thread-model slice is registered in build targets and unit tests.",
        evidence,
        failures,
    )


def verify_design_doc_traceability() -> Dict:
    failures = []  # type: List[str]
    smoke_script = ROOT / "scripts" / "run_webrtc_qos_plain_p3_thread_model_smoke.sh"
    v4l2_script = ROOT / "scripts" / "run_webrtc_qos_plain_p3_v4l2_report.sh"
    evidence = [rel(DOC_FILE), rel(QOS_TESTS_SCRIPT), rel(smoke_script), rel(v4l2_script)]
    for path in (DOC_FILE, QOS_TESTS_SCRIPT, smoke_script, v4l2_script):
        require_file(failures, path)
    if not failures:
        text = read_text(DOC_FILE)
        required = [
            (re.compile(r"## 16\. 验收方案"), "acceptance section"),
            (re.compile(r"### 16\.0 验收标准一页版"), "one-page acceptance criteria"),
            (re.compile(r"### 16\.0\.1 Definition of Done"), "definition of done checklist"),
            (re.compile(r"合入验收"), "merge acceptance layer"),
            (re.compile(r"P3 自动化验收"), "P3 automated acceptance layer"),
            (re.compile(r"生产签收"), "production signoff layer"),
            (re.compile(r"V4L2 split \+ 双 camera"), "real-camera production signoff layer"),
            (re.compile(r"硬性 PASS 标准"), "hard pass criteria"),
            (re.compile(r"硬性 FAIL 条件"), "hard fail criteria"),
            (re.compile(r"### 16\.9 里程碑签收标准"), "milestone signoff matrix"),
            (re.compile(r"### 16\.10 Go / No-Go 判定"), "go/no-go criteria"),
            (re.compile(r"threadSafetyBoundary"), "thread safety gate"),
            (re.compile(r"p3-thread-model-report"), "P3 report command"),
            (re.compile(r"p3-thread-model-acceptance"), "P3 production acceptance command"),
            (re.compile(r"feedbackGapSource"), "feedback gap source requirement"),
            (re.compile(r"tidSource"), "thread tid source requirement"),
        ]
        for pattern, description in required:
            require_pattern(failures, DOC_FILE, text, pattern, description)
        script_text = read_text(QOS_TESTS_SCRIPT)
        script_required = [
            (re.compile(r"p3-thread-model-acceptance"), "registered P3 production acceptance group"),
            (re.compile(r"run_p3_thread_model_acceptance"), "P3 production acceptance runner"),
            (re.compile(r"--enable-netem"), "strict weak-network netem execution"),
            (re.compile(r"run_webrtc_qos_plain_p3_v4l2_report\.sh[\s\S]+--strict"), "strict V4L2 production acceptance execution"),
        ]
        for pattern, description in script_required:
            require_pattern(failures, QOS_TESTS_SCRIPT, script_text, pattern, description)
        for script_path in (smoke_script, v4l2_script):
            report_text = read_text(script_path)
            report_required = [
                (re.compile(r'"sourceMode"'), "report sourceMode field"),
                (re.compile(r'"trackCount"'), "report trackCount field"),
                (re.compile(r'"threads"'), "report threads field"),
                (re.compile(r'"tracks"'), "report tracks field"),
                (re.compile(r'"queues"'), "report queues field"),
                (re.compile(r'"sdk"'), "report sdk field"),
                (re.compile(r'"threadSafety"'), "report threadSafety field"),
                (re.compile(r'"skipReasons"'), "report skipReasons field"),
            ]
            for pattern, description in report_required:
                require_pattern(failures, script_path, report_text, pattern, description)
        smoke_text = read_text(smoke_script)
        require_pattern(failures, smoke_script, smoke_text, re.compile(r"feedback-gap"), "feedback-gap dynamic check")
        require_pattern(failures, smoke_script, smoke_text, re.compile(r"feedbackGapSource"), "feedback gap source in sdk report")
    return make_gate(
        "designDocTraceability",
        "The design document exposes executable acceptance and milestone criteria.",
        evidence,
        failures,
    )


def render_markdown(report: Dict) -> str:
    lines = [
        "# WebRTC QoS Plain Thread Model Boundary Report",
        "",
        f"- overall: `{report['overall']}`",
        f"- generatedAt: `{report['generatedAt']}`",
        f"- scope: {report['scope']}",
        f"- note: {report['note']}",
        "",
        "## Gates",
        "",
        "| gate | status | evidence | failures |",
        "|---|---|---|---|",
    ]
    for gate in report["gates"]:
        evidence = "<br>".join(f"`{item}`" for item in gate["evidence"])
        failures = "<br>".join(gate["failures"]) if gate["failures"] else "-"
        lines.append(f"| `{gate['name']}` | `{gate['status']}` | {evidence} | {failures} |")
    lines.extend([
        "",
        "## Coverage",
        "",
        "- This report verifies static thread-model boundaries, per-track push queues, per-track push source contexts, play-side multi-consumer session wiring and per-track sink/QoE worker ownership.",
        "- `scripts/run_qos_tests.sh p3-thread-model-report` also runs dynamic `two_track_synthetic`, `two_track_decode_loop`, `slow_encoder_injection`, `slow_play_sink_injection`, `weak_network_two_track` and V4L2 capability reports.",
        "- V4L2 capture/raw/encode split is statically verified here; production signoff still requires true two-camera V4L2 runtime on hardware.",
    ])
    return "\n".join(lines) + "\n"


def write_report(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def build_report() -> Dict:
    gates = [
        verify_bounded_queue(),
        verify_thread_primitives(),
        verify_multi_track_config_boundary(),
        verify_play_multi_track_config_boundary(),
        verify_encoded_au_boundary(),
        verify_push_sdk_transport_thread(),
        verify_play_sdk_transport_thread(),
        verify_decoded_sink_worker(),
        verify_play_callback_isolation(),
        verify_worker_facade_boundaries(),
        verify_logging_and_legacy_boundaries(),
        verify_build_and_tests(),
        verify_design_doc_traceability(),
    ]
    overall = "PASS" if all(gate["status"] == "PASS" for gate in gates) else "FAIL"
    thread_safety = {
        "ownerViolations": sum(1 for gate in gates if gate["name"] in {
            "pushSdkTransportThread",
            "playSdkTransportThread",
            "decodedSinkWorker",
            "playCallbackIsolation",
            "workerFacadeBoundaries",
            "legacyLoggingAndQos",
        } and gate["status"] != "PASS"),
        "lockViolations": sum(1 for gate in gates if gate["name"] in {
            "threadPrimitives",
            "workerFacadeBoundaries",
        } and gate["status"] != "PASS"),
        "lifetimeViolations": sum(1 for gate in gates if gate["name"] in {
            "boundedQueue",
            "threadPrimitives",
        } and gate["status"] != "PASS"),
        "dataViolations": sum(1 for gate in gates if gate["name"] in {
            "encodedAuBoundary",
            "multiTrackConfigBoundary",
            "playMultiTrackConfigBoundary",
        } and gate["status"] != "PASS"),
        "status": "PASS" if overall == "PASS" else "FAIL",
    }
    return {
        "overall": overall,
        "generatedAt": datetime.now(timezone.utc).isoformat(),
        "scope": "static boundary report for the first WebRTC QoS plain thread-model slice",
        "sourceMode": "static",
        "trackCount": 0,
        "environment": {
            "cpuCount": os.cpu_count(),
            "platform": platform.platform(),
            "hasNetem": None,
            "hasV4L2Devices": Path("/dev/video0").exists(),
            "v4l2Devices": sorted(str(path) for path in Path("/dev").glob("video*")) if Path("/dev").is_dir() else [],
            "browserH264Supported": None,
            "skipReasons": [],
        },
        "note": "This static boundary report is paired with two-track synthetic, MP4 decode-loop, slow worker injection, weak-network and V4L2 capability smoke by p3-thread-model-report; V4L2 capture/raw/encode split is statically verified, while production signoff still requires true two-camera V4L2 runtime on hardware.",
        "gates": gates,
        "cases": [],
        "threads": [],
        "tracks": [],
        "queues": [],
        "sdk": {},
        "threadSafety": thread_safety,
        "artifacts": {
            "reportJson": str(DEFAULT_JSON),
            "reportMarkdown": str(DEFAULT_MARKDOWN),
        },
        "skipReasons": [],
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--json", default=str(DEFAULT_JSON), help="JSON report output path")
    parser.add_argument("--markdown", default=str(DEFAULT_MARKDOWN), help="Markdown report output path")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    report = build_report()
    write_report(Path(args.json), json.dumps(report, ensure_ascii=False, indent=2) + "\n")
    write_report(Path(args.markdown), render_markdown(report))

    if report["overall"] != "PASS":
        print("WebRTC QoS plain thread-model boundary verification failed:", file=sys.stderr)
        for gate in report["gates"]:
            if gate["status"] != "PASS":
                print(f"- {gate['name']}", file=sys.stderr)
                for failure in gate["failures"]:
                    print(f"  - {failure}", file=sys.stderr)
        return 1

    print(f"WebRTC QoS plain thread-model boundaries verified: {rel(Path(args.markdown))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
