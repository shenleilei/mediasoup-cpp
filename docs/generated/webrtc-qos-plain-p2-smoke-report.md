# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-23T13:39:46Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-netem-smoke-gated/20260523T133901Z` |
| Duration Seconds | `6` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 121, "baselinePushRtcpFeedbackPacketsIn": 65, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 5, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 116, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 4, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 50}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 151, "baselineCurrentBitrateBps": 2326605, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 12, "baselineHeight": 180, "baselineKeyframes": 6, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 178, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.12, "baselineSamples": 16, "baselineWidth": 320, "enabled": true}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 178 | 178 | 178/0 | 1342 | 65 | 121 | 5 | 151/6 | 0/7.83/14 | 0/0/0 | 1200000/1894794.50/2326605 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 179 | 190 | 167/0 | 1663 | 146 | 283 | 5 | 151/6 | 0/82.83/209 | 0/0/0 | 1128876/1285647.33/1674497 | 161/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-netem-smoke-gated/20260523T133901Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic x264 mode produced encoded H264 access units and keyframes with observable encoder metrics.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
