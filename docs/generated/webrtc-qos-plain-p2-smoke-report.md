# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-23T16:21:53Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-fullweak-recovery-first-frame/20260523T161912Z` |
| Duration Seconds | `12` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 243, "baselinePushRtcpFeedbackPacketsIn": 130, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 11, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 232, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110, "baselineSkipReason": null}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2500000, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 1, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 12, "baselineMaxForcedKeyframeDelayUs": 0, "baselineSkipReason": null, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 358, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.05, "baselineSamples": 28, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `PASS` | `{"clearEpochMs": 1779553282840, "dropRecoverStatus": "PASS", "enabled": true, "postClearDecodedFramesDelta": 10, "postClearFirstDecodedDelayMs": 111, "postClearFirstDecodedEpochMs": 1779553282951, "postClearSamples": 32, "preClearDecodedFrames": 91}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 358 | 358/0 | 2660 | 130 | 243 | 5 | 332/12 | 0/7.08/12 | 0/0/0 | 1200000/2118357.58/2500000 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 360 | 382 | 346/0 | 3758 | 291 | 486 | 5 | 332/13 | 0/100.58/241 | 0/0/0 | 1200000/1619340.08/1875228 | 0 | 246/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 360 | 360 | 360/0 | 2679 | 146 | 291 | 5 | 332/12 | 0/8/13 | 0/0/0 | 1200000/2057783.58/2500000 | 0 | 47/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 226 | 103 | 103/0 | 768 | 90 | 269 | 5 | 217/20 | 0/761.58/2861 | 0/0/0 | 300000/933615.75/1863877 | 8 | 97/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 296 | 101 | 101/0 | 977 | 204 | 2068 | 5 | 292/38 | 0/1464.60/9391 | 0/0/0 | 300000/532418.57/1709291 | 9 | 1742/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-fullweak-recovery-first-frame/20260523T161912Z`

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
