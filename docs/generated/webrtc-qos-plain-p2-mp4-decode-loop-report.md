# WebRTC QoS P2 Push/Play Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T17:16:18Z` |
| Git Commit | `8279e23d70f63d2dd0f29eef8d850ca5a0d9d2fb` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-mp4-decode-loop-p2m9-final/20260523T171549Z` |
| Duration Seconds | `12` |
| Netem | `disabled` on `lo` |
| Source Mode | `mp4-decode-loop` |
| Decode QoE | `enabled` |
| Input | `/tmp/webrtc-qos-plain-p2-mp4-decode-loop-p2m9-final/input.mp4` |
| Failed Checks | `0` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 243, "baselinePushRtcpFeedbackPacketsIn": 130, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 10, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 214, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110, "baselineSkipReason": null}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 333, "baselineCurrentBitrateBps": 1861454, "baselineCurrentFps": 30, "baselineEncoderMode": "mp4_decode_loop", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 1, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 12, "baselineMaxForcedKeyframeDelayUs": 0, "baselineSkipReason": null, "baselineWidth": 320, "sourceMode": "mp4-decode-loop"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 359, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.09, "baselineSamples": 26, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `SKIP` | `{"clearEpochMs": null, "dropRecoverStatus": null, "enabled": true, "postClearDecodedFramesDelta": null, "postClearFirstDecodedDelayMs": null, "postClearFirstDecodedEpochMs": null, "postClearSamples": null, "preClearDecodedFrames": null}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 359 | 359/0 | 1645 | 130 | 243 | 5 | 333/12 | 0/6.75/14 | 0/0/0 | 1200000/1651125.75/1861454 | 0 | 0/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-mp4-decode-loop-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-mp4-decode-loop-p2m9-final/20260523T171549Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic, MP4 decode-loop, or V4L2 x264 mode produced encoded H264 access units/keyframes, and SDK keyframe requests produced an IDR within 1 second.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `recoveryFirstFrame=PASS` means `drop_recover` observed decoded frame growth after netem clear within 15 seconds.
- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
