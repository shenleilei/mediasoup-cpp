# WebRTC QoS Plain P2 Smoke Report

| Item | Value |
|---|---|
| Overall | `PASS` |
| Generated At | `2026-05-23T14:18:18Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-fullweak-final3/20260523T141537Z` |
| Duration Seconds | `12` |
| Netem | `enabled` on `lo` |
| Source Mode | `synthetic` |
| Decode QoE | `enabled` |
| Input | `` |

## Gates

| Gate | Status | Evidence |
|---|---:|---|
| `qosMainline` | `PASS` | `{"baselinePlayRtcpPacketsOut": 244, "baselinePushRtcpFeedbackPacketsIn": 130, "baselineSelectedTwccExtId": 5}` |
| `sdkRuntimeObservability` | `PASS` | `{"baselinePlayAlertsFiles": 1, "baselinePlayMetricsFiles": 1, "baselinePlayReceiverReportCountMax": 12, "baselinePlayRuntimeEnabled": true, "baselinePlayRuntimeLogFiles": 1, "baselinePlayTransportFeedbackCountMax": 232, "baselinePushAlertsFiles": 1, "baselinePushMetricsFiles": 1, "baselinePushReceiverReportCountMax": 9, "baselinePushRuntimeEnabled": true, "baselinePushRuntimeLogFiles": 1, "baselinePushTransportFeedbackCountMax": 110}` |
| `encoderRuntime` | `PASS` | `{"baselineAccessUnits": 332, "baselineCurrentBitrateBps": 2500000, "baselineCurrentFps": 30, "baselineEncoderMode": "synthetic", "baselineEncoderName": "x264", "baselineEncoderSamples": 24, "baselineHeight": 180, "baselineKeyframes": 12, "baselineWidth": 320, "sourceMode": "synthetic"}` |
| `nativeDecodeQoe` | `PASS` | `{"baselineDecodeErrors": 0, "baselineDecodedFrames": 358, "baselineFirstFrameDelayUs": 0, "baselineFreezeCount": 0, "baselineHeight": 180, "baselineOutputFps": 30.08, "baselineSamples": 28, "baselineWidth": 320, "enabled": true}` |
| `weakNetworkCoverage` | `PASS` | `{"attemptedWeakCases": ["delay_100ms", "loss_2pct", "bandwidth_600k", "drop_recover"], "skippedCases": []}` |

## Cases

| Case | Status | Network | pushedAu | outputAu | decodedFrames/errors | RTP in | push RTCP in | play RTCP out | TWCC ext | Encoder AU/keyframes | RTT min/avg/max | Loss min/avg/max | targetBps min/avg/max | droppedFrames | NACK/PLI/RTX | Notes |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `baseline` | `PASS` | none | 359 | 358 | 358/0 | 2578 | 130 | 244 | 5 | 332/12 | 0/7.75/16 | 0/0/0 | 1200000/2005300.75/2500000 | 0 | 0/0/0 | - |
| `delay_100ms` | `PASS` | 100ms delay + 20ms jitter | 360 | 385 | 333/0 | 3964 | 303 | 490 | 5 | 332/13 | 0/109.58/242 | 0/0/0 | 1200000/1832228.50/2288765 | 0 | 249/0/0 | - |
| `loss_2pct` | `PASS` | 2% random loss | 359 | 359 | 359/0 | 2644 | 157 | 294 | 5 | 333/13 | 0/6.08/17 | 0/0/0 | 1200000/2129486/2500000 | 0 | 51/0/0 | - |
| `bandwidth_600k` | `PASS` | 600kbps rate limit | 206 | 104 | 104/0 | 787 | 73 | 233 | 5 | 202/16 | 0/718.17/2671 | 0/0/0 | 300000/880558.67/1693914 | 3 | 60/0/0 | - |
| `drop_recover` | `PASS` | 5% loss + 600kbps, then recovery | 307 | 103 | 103/0 | 1020 | 211 | 1767 | 5 | 302/37 | 0/1460.03/10551 | 0/0/0 | 300000/565958.63/2023706 | 7 | 1455/0/0 | - |

## Artifacts

- JSON report: `docs/generated/webrtc-qos-plain-p2-smoke-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-fullweak-final3/20260523T141537Z`

## Interpretation

- `PASS` case means SFU/push/play transport smoke met its checks.
- `SKIP` means the case was not verified and must not be counted as PASS.
- `qosMainline=PASS` means TWCC negotiation, push RTCP feedback input, and play RTCP feedback output are all observable.
- `sdkRuntimeObservability=PASS` means push/play SDK runtime log, metrics, alerts files exist and SDK RR/TWCC counters are non-zero.
- `encoderRuntime=PASS` means requested synthetic x264 mode produced encoded H264 access units and keyframes with observable encoder metrics.
- `nativeDecodeQoe=PASS` means requested native FFmpeg decode/QoE produced decoded frames and first-frame/decode-error metrics.
- `droppedFrames` is the SDK push-side pacer backpressure counter; non-zero values are acceptable in bandwidth/recovery cases when transport remains alive and QoE decode continues.
- `weakNetworkCoverage=PASS` means at least one tc netem weak-network case was actually attempted and passed; the generated report records which cases were covered.
- `weakNetworkCoverage=SKIP` means tc netem cases were intentionally not run; use `--enable-netem` when the host permits network emulation.
