# Directory Compare

LiveKit source:

- `/root/livekit/pkg/sfu/bwe/sendsidebwe/twcc_feedback.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/packet_info.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/packet_tracker.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/traffic_stats.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/packet_group.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/probe_packet_group.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/congestion_detector.go`
- `/root/livekit/pkg/sfu/bwe/sendsidebwe/send_side_bwe.go`
- `/root/livekit/pkg/sfu/ccutils/probe_signal.go`
- `/root/livekit/pkg/sfu/ccutils/probe_regulator.go`
- `/root/livekit/pkg/sfu/ccutils/trenddetector.go`
- `/root/livekit/pkg/sfu/ccutils/prober.go` (types only for `ProbeCluster*`)

C++ mapping in this repo:

- `client/sendsidebwe/TwccFeedbackTracker.h`
- `client/sendsidebwe/PacketInfo.h`
- `client/sendsidebwe/PacketTracker.h`
- `client/sendsidebwe/TrafficStats.h`
- `client/sendsidebwe/PacketGroup.h`
- `client/sendsidebwe/ProbePacketGroup.h`
- `client/sendsidebwe/CongestionDetector.h`
- `client/sendsidebwe/SendSideBwe.h`
- `client/ccutils/ProbeSignal.h`
- `client/ccutils/ProbeRegulator.h`
- `client/ccutils/TrendDetector.h`
- `client/ccutils/ProbeTypes.h`

Integration status:

- low-level support utilities: ported
- TWCC feedback time-order logic: ported
- packet tracker: ported
- packet group / traffic stats: ported
- congestion detector shell/core: ported into C++ module
- `NetworkThread` glue migration: pending
- old `DelayBasedBwe` removal/replacement: completed
