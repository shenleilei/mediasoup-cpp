# WebRTC QoS Plain P3 Thread Model Smoke Report

- overall: `PASS`
- generatedAt: `2026-05-25T05:39:12.183708+00:00`
- scope: P3 thread-model dynamic smoke: two_track_synthetic push/play
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-smoke/20260525T053847Z`
- networkMode: `none`
- networkCondition: none
- skipReason: ``
- schemaSummary: threads=`6` tracks=`2` queues=`4`
- sdkProcessGapMaxMs: `28.982`
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
| `two_track_synthetic` | `PASS` | `8` | `2` | `2` | `87` | `163` | `1200000/1990710/2500000` | `None` |

## Checks

| check | status | evidence |
|---|---|---|
| `sfu-readyz-ok` | `PASS` | {"availableWorkerThreads": 1, "ok": true, "redisReady": true, "redisRequired": false, "registryEnabled": true, "shutdownRequested": false, "startupSucceeded": true, "workerThreads": 1, "workers": 1} |
| `no-harness-failure` | `PASS` | ok |
| `selected-consumers` | `PASS` | selectedConsumers=2 expected=2 |
| `selected-consumers-have-twcc` | `PASS` | [('e06cfb7f-1b78-4cee-875e-fc6343528b5e', '220369183', '5'), ('bb8919c8-85a6-49ac-9354-a6d7ae9001df', '885711553', '5')] |
| `push-track-count` | `PASS` | trackCount=2 expected=2 |
| `play-track-count` | `PASS` | trackCount=2 expected=2 |
| `push-per-track-final` | `PASS` | tracks=2 expected=2 |
| `play-per-track-final` | `PASS` | tracks=2 expected=2 |
| `encoder-per-track-metrics` | `PASS` | tracks=2 expected=2 |
| `encoder-source-mode` | `PASS` | [{"accessUnits": 211, "bitrateChanges": 42, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedKeyframes": 0, "fpsChanges": 1, "framesEncoded"... |
| `push-each-track-pushed` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 239, "queueMaxDepth": 1, "queuedAu": 239, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `push-each-track-no-failures` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 239, "queueMaxDepth": 1, "queuedAu": 239, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `encoder-each-track-au-keyframe` | `PASS` | [{"accessUnits": 211, "bitrateChanges": 42, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedKeyframes": 0, "fpsChanges": 1, "framesEncoded"... |
| `slow-encoder-injection` | `PASS` | {"delayMs": 0, "mode": "none", "tracks": [{"accessUnits": 211, "bitrateChanges": 42, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 0, "forcedKeyfra... |
| `play-each-track-output` | `PASS` | [{"droppedFrames": 0, "enqueuedAu": 237, "lossQ8": 0, "nack": 0, "outputAu": 237, "pli": 0, "rttMs": 0, "senderSsrc": 220369183, "snapshotAvailable": true, "trackId": 1}, {"droppedFrames": 0, "enqueuedAu": 237, "lossQ... |
| `per-track-sink-workers` | `PASS` | [{"injectedSinkDelayCount": 0, "injectedSinkDelayTotalMs": 0, "lastHeartbeatUs": 1542961462566, "loopGapMaxUs": 100679, "loopIterations": 240, "outputAu": 237, "qoeAccessUnitsIn": 237, "qoeDecodeErrors": 0, "qoeDecode... |
| `per-track-qoe-workers` | `PASS` | [{"accessUnitsIn": 237, "decodeErrors": 0, "decodedFrames": 237, "enabled": true, "firstFrameDelayUs": 0, "freezeCount": 0, "height": 180, "keyframesIn": 8, "maxFrameGapUs": 122545, "outputFps": 29.92, "senderSsrc": 2... |
| `slow-sink-injection` | `PASS` | {"count": 0, "delayMs": 0, "mode": "none", "totalMs": 0} |
| `rtcp-feedback-loop` | `PASS` | pushRtcpIn=87 playRtcpOut=163 |
| `feedback-gap` | `PASS` | feedbackGapMaxMs=1010 limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn |
| `rtcp-no-failures` | `PASS` | pushRtcpFailures=0 playRtcpFailures=0 playPacketFailures=0 |
| `sdk-loop-gap` | `PASS` | pushSdkGapUs=28319 playSdkGapUs=28982 limitUs=50000 |
| `sink-loop-gap` | `PASS` | sinkGapUs=100679 |
| `runtime-no-unexpected-alerts` | `PASS` | alerts=0 |

## Remaining P3 Acceptance

- `two_track_decode_loop`
- `slow_encoder_injection`
- `slow_play_sink_injection`
- `weak_network_two_track`
- `v4l2_single_camera`
- `v4l2_two_camera`
