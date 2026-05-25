# WebRTC QoS P2 Push/Play Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T17:16:30Z` |
| Git Commit | `8279e23d70f63d2dd0f29eef8d850ca5a0d9d2fb` |
| SDK Dist | `/root/webrtc_qos_sdk/dist/linux-x86_64` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-v4l2-p2m9-final/20260523T171630Z` |
| Duration Seconds | `6` |
| Netem | `disabled` on `lo` |
| Source Mode | `v4l2` |
| Decode QoE | `enabled` |
| Input | `` |
| Failed Checks | `0` |
| V4L2 Device | `/dev/video0` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `SKIP` | `{"baselinePlayRtcpPacketsOut": null, "baselinePushRtcpFeedbackPacketsIn": null, "baselineSelectedTwccExtId": null, "baselineSkipReason": "v4l2 device not found: /dev/video0"}` |
| `sdkRuntimeObservability` | `SKIP` | `{"baselinePlayAlertsFiles": null, "baselinePlayMetricsFiles": null, "baselinePlayReceiverReportCountMax": null, "baselinePlayRuntimeEnabled": null, "baselinePlayRuntimeLogFiles": null, "baselinePlayTransportFeedbackCountMax": null, "baselinePushAlertsFiles": null, "baselinePushMetricsFiles": null, "baselinePushReceiverReportCountMax": null, "baselinePushRuntimeEnabled": null, "baselinePushRuntimeLogFiles": null, "baselinePushTransportFeedbackCountMax": null, "baselineSkipReason": "v4l2 device not found: /dev/video0"}` |
| `encoderRuntime` | `SKIP` | `{"baselineAccessUnits": null, "baselineCurrentBitrateBps": null, "baselineCurrentFps": null, "baselineEncoderMode": null, "baselineEncoderName": null, "baselineEncoderSamples": null, "baselineForcedKeyframeRequests": null, "baselineForcedKeyframes": null, "baselineHeight": null, "baselineKeyframes": null, "baselineMaxForcedKeyframeDelayUs": null, "baselineSkipReason": "v4l2 device not found: /dev/video0", "baselineWidth": null, "sourceMode": "v4l2"}` |
| `nativeDecodeQoe` | `SKIP` | `{"baselineDecodeErrors": null, "baselineDecodedFrames": null, "baselineFirstFrameDelayUs": null, "baselineFreezeCount": null, "baselineHeight": null, "baselineOutputFps": null, "baselineSamples": null, "baselineSkipReason": "v4l2 device not found: /dev/video0", "baselineWidth": null, "enabled": true}` |
| `recoveryFirstFrame` | `SKIP` | `{"clearEpochMs": null, "dropRecoverStatus": null, "enabled": true, "postClearDecodedFramesDelta": null, "postClearFirstDecodedDelayMs": null, "postClearFirstDecodedEpochMs": null, "postClearSamples": null, "preClearDecodedFrames": null}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": [{"name": "baseline", "reason": "v4l2 device not found: /dev/video0"}]}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/retransmission | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `SKIP` | none | - | - | - | - | - | - | - | - | - | - | - | - | - | v4l2 device not found: /dev/video0 |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-v4l2-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-v4l2-p2m9-final/20260523T171630Z`

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
