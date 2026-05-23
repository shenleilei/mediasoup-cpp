# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T12:28:33Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-sdk-smoke-v2/20260523T122810Z` |
| Duration Seconds | `6` |
| Netem | `disabled` on `lo` |
| Input | `/tmp/webrtc-qos-plain-p2-sdk-smoke-v2/input.mp4` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 98, "baselinePushRtcpFeedbackPacketsIn": 66, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 6, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 92, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 4, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 50}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": [{"name": "delay_100ms", "reason": "netem disabled; pass --enable-netem to run weak-network cases"}]}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | RTP in | push RTCP in | play RTCP out | TWCC ext | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 90 | 90 | 196 | 66 | 98 | 5 | 0/6/9 | 0/0/0 | 1200000/1897205.83/2036647 | 0/0/0 | - |
| `delay_100ms` | `SKIP` | 100ms delay + 20ms jitter | - | - | - | - | - | - | - | - | - | - | netem disabled; pass --enable-netem to run weak-network cases |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-sdk-smoke-v2/20260523T122810Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
