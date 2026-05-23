# WebRTC QoS Plain Client

This directory contains the new mediasoup PlainTransport push/play clients.

- `push/` publishes one H264 video track through `webrtc_qos::VideoPushClient`.
- `play/` subscribes one H264 video consumer through `webrtc_qos::VideoPlayClient`.
- `common/` contains only CLI, ID, UDP, packet classification, and logging glue.

The clients intentionally do not link the legacy `client/qos`, `sendsidebwe`,
`ccutils`, `RtcpHandler`, `NetworkThread`, `SenderTransportController`, or local
H264 packetizer code. RTP/RTCP and QoS behavior belongs to `webrtc_qos_sdk`.
