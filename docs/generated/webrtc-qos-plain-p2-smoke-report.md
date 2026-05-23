# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T12:10:13Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-smoke-final-verify/20260523T120950Z` |
| Duration Seconds | `6` |
| Netem | `disabled` on `lo` |
| Input | `/tmp/webrtc-qos-plain-p2-smoke-final-verify/input.mp4` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `FAIL` | `{"baselinePlayRtcpPacketsOut": 0, "baselinePushRtcpFeedbackPacketsIn": 67, "baselineSelectedTwccExtId": 5}` |
| `weakNetworkCoverage` | `SKIP` | `{"attemptedWeakCases": [], "skippedCases": [{"name": "delay_100ms", "reason": "netem disabled; pass --enable-netem to run weak-network cases"}]}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | RTP in | push RTCP in | play RTCP out | TWCC ext | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 90 | 90 | 195 | 67 | 0 | 5 | 0/2.83/7 | 0/0/0 | 1200000/1894887.50/2033865 | 0/0/0 | - |
| `delay_100ms` | `SKIP` | 100ms delay + 20ms jitter | - | - | - | - | - | - | - | - | - | - | netem disabled; pass --enable-netem to run weak-network cases |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-smoke-final-verify/20260523T120950Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=FAIL` means P2-M1d is still open: play did not output periodic RR/TWCC feedback through the SDK facade.
