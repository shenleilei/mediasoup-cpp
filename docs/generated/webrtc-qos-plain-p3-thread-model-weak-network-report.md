# WebRTC QoS Plain P3 Thread Model Smoke Report

- overall: `PASS`
- generatedAt: `2026-05-25T05:41:49.191609+00:00`
- scope: P3 thread-model dynamic smoke: weak_network_two_track push/play
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-smoke/20260525T054114Z`
- networkMode: `weak`
- networkCondition: 5% loss + 600kbps rate limit on lo, then recovery
- skipReason: ``
- schemaSummary: threads=`6` tracks=`2` queues=`4`
- sdkProcessGapMaxMs: `31.284`
- feedbackGapMaxMs: `1010`

## Gates

| gate | status |
|---|---|
| `multiTrackCoverage` | `PASS` |
| `playTrackCoverage` | `PASS` |
| `sdkThreadHealth` | `PASS` |
| `feedbackLoop` | `PASS` |
| `slowEncoderIsolation` | `PASS` |
| `slowSinkIsolation` | `PASS` |
| `perTrackPlaySinkQoe` | `PASS` |
| `cameraRuntime` | `PASS` |
| `weakNetworkTwoTrack` | `PASS` |

## Case

| case | status | duration | push tracks | play tracks | push RTCP in | play RTCP out | targetBps min/avg/max | post-clear target max |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `weak_network_two_track` | `PASS` | `18` | `2` | `2` | `186` | `1938` | `300000/905092/2151973` | `396093` |

## Checks

| check | status | evidence |
|---|---|---|
| `sfu-readyz-ok` | `PASS` | {"availableWorkerThreads": 1, "ok": true, "redisReady": true, "redisRequired": false, "registryEnabled": true, "shutdownRequested": false, "startupSucceeded": true, "workerThreads": 1, "workers": 1} |
| `no-harness-failure` | `PASS` | ok |
| `selected-consumers` | `PASS` | selectedConsumers=2 expected=2 |
| `selected-consumers-have-twcc` | `PASS` | [('731b9468-3295-4ec7-92ad-b5a21c38396e', '369321992', '5'), ('b9a429fb-8647-4fc8-ad73-92052af9d825', '393168665', '5')] |
| `push-track-count` | `PASS` | trackCount=2 expected=2 |
| `play-track-count` | `PASS` | trackCount=2 expected=2 |
| `push-per-track-final` | `PASS` | tracks=2 expected=2 |
| `play-per-track-final` | `PASS` | tracks=2 expected=2 |
| `encoder-per-track-metrics` | `PASS` | tracks=2 expected=2 |
| `encoder-source-mode` | `PASS` | [{"accessUnits": 295, "bitrateChanges": 59, "currentBitrateBps": 348046, "currentFps": 10, "encoder": "x264", "encoderRecreates": 6, "forcedKeyframeRequests": 48, "forcedKeyframes": 9, "fpsChanges": 5, "framesEncoded"... |
| `push-each-track-pushed` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 304, "queueMaxDepth": 1, "queuedAu": 304, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `push-each-track-no-failures` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 304, "queueMaxDepth": 1, "queuedAu": 304, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `encoder-each-track-au-keyframe` | `PASS` | [{"accessUnits": 295, "bitrateChanges": 59, "currentBitrateBps": 348046, "currentFps": 10, "encoder": "x264", "encoderRecreates": 6, "forcedKeyframeRequests": 48, "forcedKeyframes": 9, "fpsChanges": 5, "framesEncoded"... |
| `slow-encoder-injection` | `PASS` | {"delayMs": 0, "mode": "none", "tracks": [{"accessUnits": 295, "bitrateChanges": 59, "currentBitrateBps": 348046, "currentFps": 10, "encoder": "x264", "encoderRecreates": 6, "forcedKeyframeRequests": 48, "forcedKeyfra... |
| `play-each-track-output` | `PASS` | [{"droppedFrames": 0, "enqueuedAu": 133, "lossQ8": 0, "nack": 786, "outputAu": 133, "pli": 0, "rttMs": 0, "senderSsrc": 369321992, "snapshotAvailable": true, "trackId": 1}, {"droppedFrames": 0, "enqueuedAu": 122, "los... |
| `per-track-sink-workers` | `PASS` | [{"injectedSinkDelayCount": 0, "injectedSinkDelayTotalMs": 0, "lastHeartbeatUs": 1543118263399, "loopGapMaxUs": 112562, "loopIterations": 270, "outputAu": 133, "qoeAccessUnitsIn": 133, "qoeDecodeErrors": 0, "qoeDecode... |
| `per-track-qoe-workers` | `PASS` | [{"accessUnitsIn": 133, "decodeErrors": 0, "decodedFrames": 133, "enabled": true, "firstFrameDelayUs": 0, "freezeCount": 3, "height": 180, "keyframesIn": 9, "maxFrameGapUs": 5651272, "outputFps": 10.6, "senderSsrc": 3... |
| `slow-sink-injection` | `PASS` | {"count": 0, "delayMs": 0, "mode": "none", "totalMs": 0} |
| `rtcp-feedback-loop` | `PASS` | pushRtcpIn=186 playRtcpOut=1938 |
| `feedback-gap` | `PASS` | feedbackGapMaxMs=1010 limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn |
| `rtcp-no-failures` | `PASS` | pushRtcpFailures=0 playRtcpFailures=0 playPacketFailures=0 |
| `sdk-loop-gap` | `PASS` | pushSdkGapUs=31284 playSdkGapUs=28506 limitUs=100000 |
| `sink-loop-gap` | `PASS` | sinkGapUs=112562 |
| `runtime-no-unexpected-alerts` | `PASS` | alerts=0 |
| `weak-network-target-down-and-recover` | `PASS` | {"netemApplied": true, "networkCondition": "5% loss + 600kbps rate limit on lo, then recovery", "networkMode": "weak", "playRtcpOut": 1938, "postClearTargetBps": {"avg": 389181.3333333333, "count": 6, "last": 396093, ... |

## Remaining P3 Acceptance

- `two_track_synthetic`
- `two_track_decode_loop`
- `slow_encoder_injection`
- `slow_play_sink_injection`
- `v4l2_single_camera`
- `v4l2_two_camera`
