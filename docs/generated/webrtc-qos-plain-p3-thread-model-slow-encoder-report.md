# WebRTC QoS Plain P3 Thread Model Smoke Report

- overall: `PASS`
- generatedAt: `2026-05-25T05:40:01.178961+00:00`
- scope: P3 thread-model dynamic smoke: slow_encoder_injection push/play
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-smoke/20260525T053936Z`
- networkMode: `none`
- networkCondition: none
- skipReason: ``
- schemaSummary: threads=`6` tracks=`2` queues=`4`
- sdkProcessGapMaxMs: `26.615`
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
| `slow_encoder_injection` | `PASS` | `8` | `2` | `2` | `88` | `163` | `1200000/1535942/1625286` | `None` |

## Checks

| check | status | evidence |
|---|---|---|
| `sfu-readyz-ok` | `PASS` | {"availableWorkerThreads": 1, "ok": true, "redisReady": true, "redisRequired": false, "registryEnabled": true, "shutdownRequested": false, "startupSucceeded": true, "workerThreads": 1, "workers": 1} |
| `no-harness-failure` | `PASS` | ok |
| `selected-consumers` | `PASS` | selectedConsumers=2 expected=2 |
| `selected-consumers-have-twcc` | `PASS` | [('ee714de1-87e0-4a8d-b8f7-7c8c61f3d5ff', '201366914', '5'), ('02b5cc6c-817c-4921-b707-9f2975f38817', '949960013', '5')] |
| `push-track-count` | `PASS` | trackCount=2 expected=2 |
| `play-track-count` | `PASS` | trackCount=2 expected=2 |
| `push-per-track-final` | `PASS` | tracks=2 expected=2 |
| `play-per-track-final` | `PASS` | tracks=2 expected=2 |
| `encoder-per-track-metrics` | `PASS` | tracks=2 expected=2 |
| `encoder-source-mode` | `PASS` | [{"accessUnits": 58, "bitrateChanges": 3, "currentBitrateBps": 962643, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forcedKeyframes": 1, "fpsChanges": 1, "framesEncoded": 5... |
| `push-each-track-pushed` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 98, "queueMaxDepth": 1, "queuedAu": 98, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "dr... |
| `push-each-track-no-failures` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 98, "queueMaxDepth": 1, "queuedAu": 98, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "dr... |
| `encoder-each-track-au-keyframe` | `PASS` | [{"accessUnits": 58, "bitrateChanges": 3, "currentBitrateBps": 962643, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forcedKeyframes": 1, "fpsChanges": 1, "framesEncoded": 5... |
| `slow-encoder-injection` | `PASS` | {"delayMs": 80, "mode": "slow-encoder", "tracks": [{"accessUnits": 58, "bitrateChanges": 3, "currentBitrateBps": 962643, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forced... |
| `play-each-track-output` | `PASS` | [{"droppedFrames": 0, "enqueuedAu": 98, "lossQ8": 0, "nack": 0, "outputAu": 98, "pli": 0, "rttMs": 0, "senderSsrc": 201366914, "snapshotAvailable": true, "trackId": 1}, {"droppedFrames": 0, "enqueuedAu": 98, "lossQ8":... |
| `per-track-sink-workers` | `PASS` | [{"injectedSinkDelayCount": 0, "injectedSinkDelayTotalMs": 0, "lastHeartbeatUs": 1543010368196, "loopGapMaxUs": 101087, "loopIterations": 105, "outputAu": 98, "qoeAccessUnitsIn": 98, "qoeDecodeErrors": 0, "qoeDecodedF... |
| `per-track-qoe-workers` | `PASS` | [{"accessUnitsIn": 98, "decodeErrors": 0, "decodedFrames": 98, "enabled": true, "firstFrameDelayUs": 0, "freezeCount": 0, "height": 180, "keyframesIn": 4, "maxFrameGapUs": 112981, "outputFps": 12.27, "senderSsrc": 201... |
| `slow-sink-injection` | `PASS` | {"count": 0, "delayMs": 0, "mode": "slow-encoder", "totalMs": 0} |
| `rtcp-feedback-loop` | `PASS` | pushRtcpIn=88 playRtcpOut=163 |
| `feedback-gap` | `PASS` | feedbackGapMaxMs=1010 limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn |
| `rtcp-no-failures` | `PASS` | pushRtcpFailures=0 playRtcpFailures=0 playPacketFailures=0 |
| `sdk-loop-gap` | `PASS` | pushSdkGapUs=26615 playSdkGapUs=20806 limitUs=100000 |
| `sink-loop-gap` | `PASS` | sinkGapUs=101499 |
| `runtime-no-unexpected-alerts` | `PASS` | alerts=0 |

## Remaining P3 Acceptance

- `two_track_synthetic`
- `two_track_decode_loop`
- `slow_play_sink_injection`
- `weak_network_two_track`
- `v4l2_single_camera`
- `v4l2_two_camera`
