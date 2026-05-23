# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `FAIL` |
| Generated At | `2026-05-23T16:55:28Z` |
| Git Commit | `8279e23d70f63d2dd0f29eef8d850ca5a0d9d2fb` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-fullweak-p2m9-final/20260523T165248Z` |
| Duration Seconds | `12` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |
| Failed Checks | `1` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 243, "baselinePushRtcpFeedbackPacketsIn": 129, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 11, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 232, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 8, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110, "baselineSkipReason": null}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2500000, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 3, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 13, "baselineMaxForcedKeyframeDelayUs": 20491, "baselineSkipReason": null, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 359, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.06, "baselineSamples": 28, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `FAIL` | `{"clearEpochMs": 1779555298331, "dropRecoverStatus": "FAIL", "enabled": true, "postClearDecodedFramesDelta": 3, "postClearFirstDecodedDelayMs": 130, "postClearFirstDecodedEpochMs": 1779555298461, "postClearSamples": 32, "preClearDecodedFrames": 93}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 359 | 359/0 | 2627 | 129 | 243 | 5 | 332/13 | 0/7.42/17 | 0/0/0 | 1200000/2113877/2500000 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 360 | 388 | 319/0 | 4003 | 313 | 496 | 5 | 332/13 | 0/104.42/222 | 0/0/0 | 1200000/1824620.83/2243528 | 0 | 254/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 359 | 359 | 359/0 | 2627 | 154 | 294 | 5 | 333/13 | 0/4.75/21 | 0/0/0 | 1200000/2080997.67/2500000 | 0 | 51/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 227 | 102 | 102/0 | 853 | 83 | 258 | 5 | 217/17 | 0/733.50/2801 | 0/0/0 | 300000/986637.50/1986686 | 4 | 86/0/0 | - |
| `drop_recover` | `FAIL` | 5% loss + 600kbps, then recovery | 298 | 96 | 96/0 | 954 | 214 | 2083 | 5 | 293/37 | 0/1407.10/10636 | 0/0/0 | 300000/570019.93/1984529 | 8 | 1774/0/0 | weak-recovery-target-up: targetMin=300000 postClearMax=300000 postClearLast=300000 postClearSamples=30 recoverSeconds=15 |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-fullweak-p2m9-final/20260523T165248Z`

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
