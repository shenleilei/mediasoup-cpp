# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T12:59:38Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-synthetic-smoke-v2/20260523T125915Z` |
| Duration Seconds | `6` |
| Netem | `disabled` on `lo` |
| Source Mode | `synthetic` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 121, "baselinePushRtcpFeedbackPacketsIn": 65, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 5, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 116, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 4, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 50}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 151, "baselineCurrentBitrateBps": 1959838, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 12, "baselineHeight": 180, "baselineKeyframes": 6, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 178 | 178 | 1261 | 65 | 121 | 5 | 151/6 | 0/8/14 | 0/0/0 | 1200000/1616812/1959838 | 0/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-synthetic-smoke-v2/20260523T125915Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic x264 mode produced encoded H264 access units and keyframes with observable encoder metrics.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
