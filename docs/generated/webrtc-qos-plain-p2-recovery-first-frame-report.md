# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T16:16:30Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-recovery-first-frame/20260523T161543Z` |
| Duration Seconds | `30` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `SKIP` | `{"baselinePlayRtcpPacketsOut": null, "baselinePushRtcpFeedbackPacketsIn": null, "baselineSelectedTwccExtId": null, "baselineSkipReason": "baseline case not requested"}` |
| `sdkRuntimeObservability` | `SKIP` | `{"baselinePlayAlertsFiles": null, "baselinePlayMetricsFiles": null, "baselinePlayReceiverReportCountMax": null, "baselinePlayRuntimeEnabled": null, "baselinePlayRuntimeLogFiles": null, "baselinePlayTransportFeedbackCountMax": null, "baselinePushAlertsFiles": null, "baselinePushMetricsFiles": null, "baselinePushReceiverReportCountMax": null, "baselinePushRuntimeEnabled": null, "baselinePushRuntimeLogFiles": null, "baselinePushTransportFeedbackCountMax": null, "baselineSkipReason": "baseline case not requested"}` |
| `encoderRuntime` | `SKIP` | `{"baselineAccessUnits": null, "baselineCurrentBitrateBps": null, "baselineCurrentFps": null, "baselineEncoderMode": null, "baselineEncoderName": null, "baselineEncoderSamples": null, "baselineForcedKeyframeRequests": null, "baselineForcedKeyframes": null, "baselineHeight": null, "baselineKeyframes": null, "baselineMaxForcedKeyframeDelayUs": null, "baselineSkipReason": "baseline case not requested", "baselineWidth": null, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `SKIP` | `{"baselineDecodeErrors": null, "baselineDecodedFrames": null, "baselineFirstFrameDelayUs": null, "baselineFreezeCount": null, "baselineHeight": null, "baselineOutputFps": null, "baselineSamples": null, "baselineSkipReason": "baseline case not requested", "baselineWidth": null, "enabled": true}` |
| `recoveryFirstFrame` | `PASS` | `{"clearEpochMs": 1779552959587, "dropRecoverStatus": "PASS", "enabled": true, "postClearDecodedFramesDelta": 2, "postClearFirstDecodedDelayMs": 1141, "postClearFirstDecodedEpochMs": 1779552960728, "postClearSamples": 32, "preClearDecodedFrames": 91}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 328 | 93 | 93/0 | 1010 | 181 | 1795 | 5 | 319/37 | 0/1367.90/8253 | 0/0/0 | 300000/566669/1953152 | 6 | 1480/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-recovery-first-frame-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-recovery-first-frame/20260523T161543Z`

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
