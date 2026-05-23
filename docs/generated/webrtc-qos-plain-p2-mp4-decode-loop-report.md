# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T14:52:48Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-mp4-decode-loop/20260523T145219Z` |
| Duration Seconds | `12` |
| Netem | `disabled` on `lo` |
| Source Mode | `mp4-decode-loop` |
| Decode QoE | `enabled` |
| Input | `/tmp/webrtc-qos-plain-p2-mp4-decode-loop/input.mp4` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 241, "baselinePushRtcpFeedbackPacketsIn": 128, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 12, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 229, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 108}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2061354, "baselineCurrentFps": 30, "baselineEncoderMode": "mp4_decode_loop", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 2, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 13, "baselineMaxForcedKeyframeDelayUs": 10195, "baselineWidth": 320, "sourceMode": "mp4-decode-loop"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 357, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.01, "baselineSamples": 28, "baselineWidth": 320, "enabled": true}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 357 | 357 | 357/0 | 1631 | 128 | 241 | 5 | 332/13 | 0/5.50/16 | 0/0/0 | 1200000/1686090.33/2061354 | 0 | 0/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-mp4-decode-loop-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-mp4-decode-loop/20260523T145219Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic or MP4 decode-loop x264 mode produced encoded H264 access units/keyframes, and SDK keyframe requests produced an IDR within 1 second.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
