# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-23T14:30:30Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-fullweak-idr/20260523T142749Z` |
| Duration Seconds | `12` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 244, "baselinePushRtcpFeedbackPacketsIn": 129, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 11, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 233, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2500000, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 1, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 12, "baselineMaxForcedKeyframeDelayUs": 0, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 358, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.06, "baselineSamples": 28, "baselineWidth": 320, "enabled": true}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 358 | 358/0 | 2663 | 129 | 244 | 5 | 332/12 | 0/5.92/14 | 0/0/0 | 1200000/2117690.83/2500000 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 360 | 383 | 311/0 | 3991 | 315 | 499 | 5 | 332/13 | 0/106.42/232 | 0/0/0 | 1200000/1853280.58/2227342 | 0 | 259/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 361 | 360 | 360/0 | 2690 | 162 | 304 | 5 | 333/12 | 0/5.67/16 | 0/0/0 | 1200000/2086280.92/2500000 | 0 | 59/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 218 | 101 | 101/0 | 809 | 83 | 249 | 5 | 209/19 | 0/870.83/2600 | 0/0/0 | 300000/985280.83/1994746 | 5 | 78/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 367 | 96 | 96/0 | 1038 | 261 | 1894 | 5 | 359/36 | 0/1544.97/9594 | 0/0/0 | 300000/551369.27/1695765 | 6 | 1558/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-fullweak-idr/20260523T142749Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic x264 mode produced encoded H264 access units/keyframes, and SDK keyframe requests produced an IDR within 1 second.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
