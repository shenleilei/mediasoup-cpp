# WebRTC QoS Plain P3 Thread Model Smoke Report

- overall: `PASS`
- generatedAt: `2026-05-25T05:40:30.702984+00:00`
- scope: P3 thread-model dynamic smoke: slow_play_sink_injection push/play
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-smoke/20260525T054001Z`
- networkMode: `none`
- networkCondition: none
- skipReason: ``
- schemaSummary: threads=`6` tracks=`2` queues=`4`
- sdkProcessGapMaxMs: `24.994`
- feedbackGapMaxMs: `1006`

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
| `slow_play_sink_injection` | `PASS` | `8` | `2` | `2` | `86` | `164` | `1200000/2049528/2500000` | `None` |

## Checks

| check | status | evidence |
|---|---|---|
| `sfu-readyz-ok` | `PASS` | {"availableWorkerThreads": 1, "ok": true, "redisReady": true, "redisRequired": false, "registryEnabled": true, "shutdownRequested": false, "startupSucceeded": true, "workerThreads": 1, "workers": 1} |
| `no-harness-failure` | `PASS` | ok |
| `selected-consumers` | `PASS` | selectedConsumers=2 expected=2 |
| `selected-consumers-have-twcc` | `PASS` | [('53d997e1-49b9-4be9-bf14-853d3ea8df7a', '492917791', '5'), ('104db84f-320c-476e-a1a9-5e9a626ff5a9', '289516274', '5')] |
| `push-track-count` | `PASS` | trackCount=2 expected=2 |
| `play-track-count` | `PASS` | trackCount=2 expected=2 |
| `push-per-track-final` | `PASS` | tracks=2 expected=2 |
| `play-per-track-final` | `PASS` | tracks=2 expected=2 |
| `encoder-per-track-metrics` | `PASS` | tracks=2 expected=2 |
| `encoder-source-mode` | `PASS` | [{"accessUnits": 211, "bitrateChanges": 41, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedKeyframes": 0, "fpsChanges": 1, "framesEncoded"... |
| `push-each-track-pushed` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 239, "queueMaxDepth": 1, "queuedAu": 239, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `push-each-track-no-failures` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 239, "queueMaxDepth": 1, "queuedAu": 239, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `encoder-each-track-au-keyframe` | `PASS` | [{"accessUnits": 211, "bitrateChanges": 41, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedKeyframes": 0, "fpsChanges": 1, "framesEncoded"... |
| `slow-encoder-injection` | `PASS` | {"delayMs": 0, "mode": "slow-sink", "tracks": [{"accessUnits": 211, "bitrateChanges": 41, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedK... |
| `play-each-track-output` | `PASS` | [{"droppedFrames": 0, "enqueuedAu": 237, "lossQ8": 0, "nack": 0, "outputAu": 162, "pli": 0, "rttMs": 0, "senderSsrc": 492917791, "snapshotAvailable": true, "trackId": 1}, {"droppedFrames": 0, "enqueuedAu": 237, "lossQ... |
| `per-track-sink-workers` | `PASS` | [{"injectedSinkDelayCount": 162, "injectedSinkDelayTotalMs": 12960, "lastHeartbeatUs": 1543039869261, "loopGapMaxUs": 89912, "loopIterations": 163, "outputAu": 162, "qoeAccessUnitsIn": 162, "qoeDecodeErrors": 0, "qoeD... |
| `per-track-qoe-workers` | `PASS` | [{"accessUnitsIn": 162, "decodeErrors": 0, "decodedFrames": 129, "enabled": true, "firstFrameDelayUs": 0, "freezeCount": 2, "height": 180, "keyframesIn": 5, "maxFrameGapUs": 1047575, "outputFps": 9.85, "senderSsrc": 4... |
| `slow-sink-injection` | `PASS` | {"count": 324, "delayMs": 80, "mode": "slow-sink", "totalMs": 25920} |
| `rtcp-feedback-loop` | `PASS` | pushRtcpIn=86 playRtcpOut=164 |
| `feedback-gap` | `PASS` | feedbackGapMaxMs=1006 limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn |
| `rtcp-no-failures` | `PASS` | pushRtcpFailures=0 playRtcpFailures=0 playPacketFailures=0 |
| `sdk-loop-gap` | `PASS` | pushSdkGapUs=24994 playSdkGapUs=24302 limitUs=100000 |
| `sink-loop-gap` | `PASS` | sinkGapUs=91876 |
| `runtime-no-unexpected-alerts` | `PASS` | alerts=0 |

## Remaining P3 Acceptance

- `two_track_synthetic`
- `two_track_decode_loop`
- `slow_encoder_injection`
- `weak_network_two_track`
- `v4l2_single_camera`
- `v4l2_two_camera`
