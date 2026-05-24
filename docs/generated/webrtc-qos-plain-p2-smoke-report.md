# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-24T00:40:55Z` |
| Git Commit | `4a15a91a48e333fbaf07fbdbe6a223b38ab8b0bf` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-smoke/20260524T003756Z` |
| Duration Seconds | `10` |
| Netem | `enabled` on `lo` |
| Source Mode | `copy` |
| Decode QoE | `enabled` |
| Input | `/tmp/webrtc-qos-plain-p2-smoke/input.mp4` |
| Failed Checks | `0` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 162, "baselinePushRtcpFeedbackPacketsIn": 110, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 10, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 152, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 8, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 90, "baselineSkipReason": null}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": null, "baselineCurrentBitrateBps": null, "baselineCurrentFps": null, "baselineEncoderMode": null, "baselineEncoderName": null, "baselineEncoderSamples": 0, "baselineForcedKeyframeRequests": null, "baselineForcedKeyframes": null, "baselineHeight": null, "baselineKeyframes": null, "baselineMaxForcedKeyframeDelayUs": null, "baselineSkipReason": null, "baselineWidth": null, "sourceMode": "copy"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 150, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 15.05, "baselineSamples": 24, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `PASS` | `{"clearEpochMs": 1779583224776, "dropRecoverStatus": "PASS", "enabled": true, "postClearDecodedFramesDelta": 241, "postClearFirstDecodedDelayMs": 105, "postClearFirstDecodedEpochMs": 1779583224881, "postClearSamples": 32, "preClearDecodedFrames": 178}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "loss_5pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 150 | 150 | 150/0 | 275 | 110 | 162 | 5 | - | 0/4/8 | 0/0/0 | 1200000/1903009/1994666 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 151 | 155 | 151/0 | 295 | 114 | 184 | 5 | - | 0/80.30/217 | 0/0/0 | 300000/734385.20/2021926 | 0 | 15/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 151 | 151 | 151/0 | 279 | 124 | 180 | 5 | - | 0/5.90/9 | 0/0/0 | 1200000/1846777.40/1994666 | 0 | 17/0/0 | - |
| `loss_5pct` | `PASS` | 5% random loss | 151 | 139 | 139/0 | 282 | 112 | 173 | 5 | - | 0/3/9 | 0/0/0 | 1200000/1952714.40/2036350 | 0 | 9/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 151 | 136 | 136/0 | 278 | 110 | 200 | 5 | - | 0/34.60/80 | 0/0/0 | 300000/1237333/1994666 | 0 | 31/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 451 | 447 | 419/0 | 751 | 328 | 526 | 5 | - | 0/31.40/145 | 0/0/0 | 300000/620077.53/2040436 | 0 | 22/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-smoke/20260524T003756Z`

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
