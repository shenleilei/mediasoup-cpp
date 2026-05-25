# WebRTC QoS Plain P3 Thread Model Smoke Report

- overall: `PASS`
- generatedAt: `2026-05-25T05:39:36.620376+00:00`
- scope: P3 thread-model dynamic smoke: two_track_decode_loop push/play
- artifactRoot: `/tmp/webrtc-qos-plain-p3-thread-model-smoke/20260525T053912Z`
- networkMode: `none`
- networkCondition: none
- skipReason: ``
- schemaSummary: threads=`6` tracks=`2` queues=`4`
- sdkProcessGapMaxMs: `21.901`
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
| `two_track_decode_loop` | `PASS` | `8` | `2` | `2` | `88` | `161` | `1200000/2173541/2500000` | `None` |

## Checks

| check | status | evidence |
|---|---|---|
| `sfu-readyz-ok` | `PASS` | {"availableWorkerThreads": 1, "ok": true, "redisReady": true, "redisRequired": false, "registryEnabled": true, "shutdownRequested": false, "startupSucceeded": true, "workerThreads": 1, "workers": 1} |
| `no-harness-failure` | `PASS` | ok |
| `selected-consumers` | `PASS` | selectedConsumers=2 expected=2 |
| `selected-consumers-have-twcc` | `PASS` | [('9dc55abe-8d43-42f4-bdba-5171a926088e', '458806950', '5'), ('c0d7c065-658b-4ee1-9837-e3719d858337', '639049180', '5')] |
| `push-track-count` | `PASS` | trackCount=2 expected=2 |
| `play-track-count` | `PASS` | trackCount=2 expected=2 |
| `push-per-track-final` | `PASS` | tracks=2 expected=2 |
| `play-per-track-final` | `PASS` | tracks=2 expected=2 |
| `encoder-per-track-metrics` | `PASS` | tracks=2 expected=2 |
| `encoder-source-mode` | `PASS` | [{"accessUnits": 212, "bitrateChanges": 21, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forcedKeyframes": 1, "fpsChanges": 1, "framesEncoded"... |
| `push-each-track-pushed` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 238, "queueMaxDepth": 1, "queuedAu": 238, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `push-each-track-no-failures` | `PASS` | [{"adaptationAvailable": true, "droppedAu": 0, "pushFailures": 0, "pushedAu": 238, "queueMaxDepth": 1, "queuedAu": 238, "senderSsrc": 11111111, "snapshotAvailable": true, "trackId": 1}, {"adaptationAvailable": true, "... |
| `encoder-each-track-au-keyframe` | `PASS` | [{"accessUnits": 212, "bitrateChanges": 21, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forcedKeyframes": 1, "fpsChanges": 1, "framesEncoded"... |
| `slow-encoder-injection` | `PASS` | {"delayMs": 0, "mode": "none", "tracks": [{"accessUnits": 212, "bitrateChanges": 21, "currentBitrateBps": 1400000, "currentFps": 30, "encoder": "x264", "encoderRecreates": 2, "forcedKeyframeRequests": 1, "forcedKeyfra... |
| `play-each-track-output` | `PASS` | [{"droppedFrames": 0, "enqueuedAu": 238, "lossQ8": 0, "nack": 0, "outputAu": 238, "pli": 0, "rttMs": 0, "senderSsrc": 458806950, "snapshotAvailable": true, "trackId": 1}, {"droppedFrames": 0, "enqueuedAu": 238, "lossQ... |
| `per-track-sink-workers` | `PASS` | [{"injectedSinkDelayCount": 0, "injectedSinkDelayTotalMs": 0, "lastHeartbeatUs": 1542985815255, "loopGapMaxUs": 101549, "loopIterations": 247, "outputAu": 238, "qoeAccessUnitsIn": 238, "qoeDecodeErrors": 0, "qoeDecode... |
| `per-track-qoe-workers` | `PASS` | [{"accessUnitsIn": 238, "decodeErrors": 0, "decodedFrames": 238, "enabled": true, "firstFrameDelayUs": 0, "freezeCount": 0, "height": 480, "keyframesIn": 9, "maxFrameGapUs": 191107, "outputFps": 30.15, "senderSsrc": 4... |
| `slow-sink-injection` | `PASS` | {"count": 0, "delayMs": 0, "mode": "none", "totalMs": 0} |
| `rtcp-feedback-loop` | `PASS` | pushRtcpIn=88 playRtcpOut=161 |
| `feedback-gap` | `PASS` | feedbackGapMaxMs=1010 limitMs=2000 source=push_metrics.rtcpFeedbackPacketsIn |
| `rtcp-no-failures` | `PASS` | pushRtcpFailures=0 playRtcpFailures=0 playPacketFailures=0 |
| `sdk-loop-gap` | `PASS` | pushSdkGapUs=20036 playSdkGapUs=21901 limitUs=50000 |
| `sink-loop-gap` | `PASS` | sinkGapUs=101934 |
| `runtime-no-unexpected-alerts` | `PASS` | alerts=0 |

## Remaining P3 Acceptance

- `two_track_synthetic`
- `slow_encoder_injection`
- `slow_play_sink_injection`
- `weak_network_two_track`
- `v4l2_single_camera`
- `v4l2_two_camera`
