# WebRTC QoS Push/Play Client

This directory contains the mediasoup push/play clients backed by `webrtc_qos_sdk`.

- `push/` publishes one H264 video track through `webrtc_qos::VideoPushClient`.
- `play/` subscribes one H264 video consumer through `webrtc_qos::VideoPlayClient`.
- `common/` contains only CLI, ID, UDP, packet classification, and spdlog file
  logging glue.

RTP/RTCP, congestion control, pacing, feedback handling, packetization, and QoS
behavior belong to `webrtc_qos_sdk`; this directory only keeps the mediasoup
signaling and UDP integration layer.

Formal runtime observability must go through `--log-dir` files and SDK
logs/metrics/alerts. Do not add `std::cout` / `printf` style logging here.
