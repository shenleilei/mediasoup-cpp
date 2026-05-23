# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-23T17:47:05Z` |
| Git Commit | `588c79bde1185f8022533b29383319f5056158ee` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-fullweak-final-cleantrace/20260523T174424Z` |
| Duration Seconds | `12` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |
| Failed Checks | `0` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 244, "baselinePushRtcpFeedbackPacketsIn": 130, "baselineSelectedTwccExtId": 5, "baselineSkipReason": null}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 11, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 215, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110, "baselineSkipReason": null}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2500000, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineForcedKeyframeRequests": 3, "baselineForcedKeyframes": 1, "baselineHeight": 180, "baselineKeyframes": 13, "baselineMaxForcedKeyframeDelayUs": 20540, "baselineSkipReason": null, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 358, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.07, "baselineSamples": 26, "baselineSkipReason": null, "baselineWidth": 320, "enabled": true}` |
| `recoveryFirstFrame` | `PASS` | `{"clearEpochMs": 1779558394367, "dropRecoverStatus": "PASS", "enabled": true, "postClearDecodedFramesDelta": 376, "postClearFirstDecodedDelayMs": 144, "postClearFirstDecodedEpochMs": 1779558394511, "postClearSamples": 32, "preClearDecodedFrames": 90}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 358 | 358/0 | 2579 | 130 | 244 | 5 | 332/13 | 0/4.58/18 | 0/0/0 | 1200000/2052376.92/2500000 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 360 | 385 | 294/0 | 4035 | 304 | 487 | 5 | 332/12 | 0/78.25/220 | 0/0/0 | 1200000/1791211.42/2160266 | 0 | 247/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 360 | 359 | 359/0 | 2608 | 157 | 387 | 5 | 332/13 | 0/7.25/23 | 0/0/0 | 1200000/2033744.50/2500000 | 0 | 143/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 258 | 156 | 156/0 | 1419 | 105 | 309 | 5 | 231/18 | 0/627.33/1489 | 0/0/0 | 300000/1158810.50/2500000 | 4 | 105/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 607 | 466 | 466/0 | 3922 | 309 | 1742 | 5 | 583/37 | 0/2186089.20/65535998 | 0/0/0 | 300000/1398037.20/2500000 | 5 | 1223/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-fullweak-final-cleantrace/20260523T174424Z`

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
