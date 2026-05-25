# WebRTC QoS P2 Push/Play Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-24T03:03:27Z` |
| Git Commit | `8d88c71504ea136b5a9e522cfd3697fb8f1fa612` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-smoke/20260524T030028Z` |
| Duration Seconds | `10` |
| Netem | `enabled` on `lo` |
| Source Mode | `copy` |
| Decode QoE | `enabled` |
| Input | `/tmp/webrtc-qos-plain-p2-smoke/input.mp4` |
| Failed Checks | `0` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 161, "baselinePushRtcpFeedbackPacketsIn": 109, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 9, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 152, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 7, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 90, "baselineSkipReason": null}` |
| `encoderRuntime` | `SKIP` | `{"baselineAccessUnits": null, "baselineCurrentBitrateBps": null, "baselineCurrentFps": null, "baselineEncoderMode": null, "baselineEncoderName": null, "baselineEncoderSamples": 0, "baselineForcedKeyframeRequests": null, "baselineForcedKeyframes": null, "baselineHeight": null, "baselineKeyframes": null, "baselineMaxForcedKeyframeDelayUs": null, "baselineSkipReason": null, "baselineWidth": null, "skipReason": "source mode does not exercise the x264 runtime encoder", "sourceMode": "copy"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 150, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 15.06, "baselineSamples": 24, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `PASS` | `{"clearEpochMs": 1779591776960, "dropRecoverStatus": "PASS", "enabled": true, "postClearDecodedFramesDelta": 241, "postClearFirstDecodedDelayMs": 120, "postClearFirstDecodedEpochMs": 1779591777080, "postClearSamples": 32, "preClearDecodedFrames": 190}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "loss_5pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/retransmission | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 150 | 150 | 150/0 | 276 | 109 | 161 | 5 | - | 0/6.50/9 | 0/0/0 | 1200000/1915199.40/1994666 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 151 | 158 | 140/0 | 292 | 116 | 186 | 5 | - | 0/86.30/247 | 0/0/0 | 300000/737648.20/2038241 | 0 | 18/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 150 | 135 | 135/0 | 273 | 108 | 163 | 5 | - | 0/5.20/8 | 0/0/0 | 1200000/2117333/2500000 | 0 | 3/0/0 | - |
| `loss_5pct` | `PASS` | 5% random loss | 152 | 145 | 132/0 | 281 | 112 | 218 | 5 | - | 0/5.60/11 | 0/0/0 | 1200000/1915199.70/1994667 | 0 | 50/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 151 | 150 | 150/0 | 281 | 110 | 169 | 5 | - | 0/52.80/183 | 0/0/0 | 300000/1237333/1994666 | 0 | 1/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 451 | 431 | 431/0 | 726 | 332 | 549 | 5 | - | 0/24.20/115 | 0/0/0 | 300000/632260.23/1994666 | 0 | 38/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-smoke/20260524T030028Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic, MP4 decode-loop, or V4L2 x264 mode produced encoded H264 access units/keyframes, and SDK keyframe requests produced an IDR within 1 second.
- `encoderRuntime=SKIP` with `Source Mode=copy` means the report used MP4 H264 copy input and did not exercise the realtime x264 encoder; use synthetic, MP4 decode-loop, or V4L2 reports for encoder runtime evidence.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `recoveryFirstFrame=PASS` means `drop_recover` observed decoded frame growth after netem clear within 15 seconds.
- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
