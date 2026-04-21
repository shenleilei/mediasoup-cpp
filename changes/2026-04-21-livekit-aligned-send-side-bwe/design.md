# Design

The implementation follows the structure of `/root/livekit/pkg/sfu/bwe/sendsidebwe` as directly as practical in C++.

This change supersedes the earlier local-only experiment under `changes/2026-04-21-plain-client-delay-based-twcc-bwe`. The repository now keeps only the livekit-aligned path as the supported send-side BWE direction.

Planned module mapping:

- `client/sendsidebwe/TwccFeedbackTracker.h`
- `client/sendsidebwe/PacketInfo.h`
- `client/sendsidebwe/PacketTracker.h`
- `client/sendsidebwe/TrafficStats.h`
- `client/sendsidebwe/PacketGroup.h`
- `client/sendsidebwe/ProbePacketGroup.h`
- `client/sendsidebwe/CongestionDetector.h`
- `client/sendsidebwe/SendSideBwe.h`
- `client/ccutils/TrendDetector.h`
- `client/ccutils/ProbeSignal.h`
- `client/ccutils/ProbeRegulator.h`

`NetworkThread` becomes glue only:

- ask send-side BWE for the next transport-wide sequence number
- rewrite RTP extension with that sequence number
- record successful sends in the BWE tracker
- feed parsed TWCC feedback into the BWE
- publish estimated available channel capacity into `SenderTransportController`
- start probe clusters when application demand exceeds estimated capacity and the BWE allows probing
- send transport-cc-enabled RTP padding probe packets on the network thread
- feed probe completion back into the BWE through the probe observer / finalize path

The previous local `DelayBasedBwe` has been replaced on the `NetworkThread` main path. Remaining cleanup work is limited to deleting legacy-only references/tests and keeping the livekit-aligned path as the only supported estimator implementation.
