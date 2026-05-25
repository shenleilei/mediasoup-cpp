# WebRTC QoS Plain P3 V4L2 Report

- overall: `PARTIAL`
- generatedAt: `2026-05-25T05:41:56.412694+00:00`
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-v4l2/20260525T054156Z`
- devices: ``
- schemaSummary: threads=`0` tracks=`0` queues=`0`

## Gates

| gate | status |
|---|---|
| `v4l2SingleCamera` | `SKIP` |
| `v4l2TwoCamera` | `SKIP` |
| `cameraRuntime` | `SKIP` |
| `noSyntheticFallback` | `PASS` |

## Cases

| case | status | device(s) | skipReason |
|---|---|---|---|
| `v4l2_single_camera` | `SKIP` | `/dev/video0` | v4l2 device not found: /dev/video0 |
| `v4l2_two_camera` | `SKIP` | `/dev/video0, /dev/video1` | second v4l2 device not found: /dev/video1 |

## Acceptance Rules

- Missing camera devices are `SKIP/PARTIAL`, never PASS.
- This report never substitutes synthetic input for V4L2.
- Two-camera V4L2 uses per-track `--track source=v4l2,device=...` when both devices exist.

## Remaining P3 Acceptance

- `two_track_synthetic`
- `two_track_decode_loop`
- `slow_encoder_injection`
- `slow_play_sink_injection`
- `weak_network_two_track`
- `v4l2_single_camera`
- `v4l2_two_camera`
