# WebRTC QoS Plain P2 Browser Receiver Report

| Item | Value |
|---|---|
| Overall | `PARTIAL` |
| Generated At | `2026-05-23T15:21:14Z` |
| Run Dir | `/tmp/webrtc-qos-plain-p2-browser-receiver/20260523T152110Z` |
| Source Mode | `synthetic` |
| Duration Seconds | `10` |

## Checks

| Check | Status | Evidence |
|---|---:|---|
| `plain-push-alive` | `PASS` | `pid=1038224` |
| `plain-publish-ok` | `PASS` | `{"logPath":"/tmp/webrtc-qos-plain-p2-browser-receiver/20260523T152110Z/browser_receiver/push/push.log","producerId":"826e3db0-f31a-4554-befb-662fb90a1f14","ssrc":22334455,"payloadType":127,"twccExtId":5,"pushedAu":91,"metricSamples":4}` |
| `browser-h264-capability` | `SKIP` | `Error: browser does not expose H264 packetization-mode=1 receive capability<br>    at Object.init (http://127.0.0.1:43659/:184:21)<br>    at async http://127.0.0.1:43659/:348:18` |
| `browser-harness-ok` | `PASS` | `skipped after codec diagnostics` |
| `browser-consumer-created` | `SKIP` | `consumerCount=0` |
| `browser-keyframe-requested` | `SKIP` | `keyframeRequests=0` |
| `browser-receiver-media-flow` | `SKIP` | `{}` |
| `browser-track-live` | `SKIP` | `[]` |

## Browser Metrics

| Metric | Value |
|---|---:|
| consumerCount | 0 |
| packetsReceivedDelta | 0 |
| bytesReceivedDelta | 0 |
| framesDecodedDelta | 0 |
| framesReceivedDelta | 0 |
| currentTimeDeltaMs | 0 |
| finalFrameSize | 0x0 |
| keyframeRequests | 0 |

## Browser Diagnostics

| Item | Value |
|---|---|
| handlerName | `Chrome111` |
| supportsH264Packetization1 | `false` |
| deviceVideoCodecs | `[{"mimeType":"video/VP8","payloadType":104,"parameters":{}},{"mimeType":"video/rtx","payloadType":105,"parameters":{"apt":104}},{"mimeType":"video/VP9","payloadType":106,"parameters":{"profile-id":0}},{"mimeType":"video/rtx","payloadType":108,"parameters":{"apt":106}}]` |
| routerVideoCodecs | `[{"mimeType":"video/H264","payloadType":107,"parameters":{"level-asymmetry-allowed":1,"packetization-mode":1,"profile-level-id":"4d0032"}},{"mimeType":"video/rtx","payloadType":102,"parameters":{"apt":107}},{"mimeType":"video/H264","payloadType":127,"parameters":{"level-asymmetry-allowed":1,"packetization-mode":1,"profile-level-id":"42e01f"}},{"mimeType":"video/rtx","payloadType":103,"parameters":{"apt":127}},{"mimeType":"video/VP8","payloadType":104,"parameters":{}},{"mimeType":"video/rtx","payloadType":105,"parameters":{"apt":104}},{"mimeType":"video/VP9","payloadType":106,"parameters":{"profile-id":0}},{"mimeType":"video/rtx","payloadType":108,"parameters":{"apt":106}}]` |

## Artifacts

- JSON report: `/root/mediasoup-cpp/docs/generated/webrtc-qos-plain-p2-browser-receiver-report.json`
- Runtime logs: `/tmp/webrtc-qos-plain-p2-browser-receiver/20260523T152110Z`

## Interpretation

- `browser-receiver-media-flow=PASS` means Chromium consumed the plain push video through a real WebRTC recv transport and inbound RTP stats increased.
- `browser-h264-capability=SKIP` means the local browser binary does not expose H264 packetization-mode=1; rerun the same command with a Chromium build that includes H264 to turn this into a blocking PASS/FAIL gate.
- This browser report does not replace native weak-network QoS smoke; it covers P2-M7 browser receiver compatibility.
