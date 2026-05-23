#include "common/RtpRtcpClassifier.h"

namespace webrtc_qos_plain {

PacketKind ClassifyRtpOrRtcp(const uint8_t* data, size_t size)
{
	if (!data || size < 2) return PacketKind::Unknown;
	const uint8_t version = data[0] >> 6;
	if (version != 2) return PacketKind::Unknown;
	const uint8_t packetType = data[1];
	if (packetType >= 192 && packetType <= 223) return PacketKind::Rtcp;
	return PacketKind::Rtp;
}

const char* PacketKindName(PacketKind kind)
{
	switch (kind) {
		case PacketKind::Rtp:
			return "rtp";
		case PacketKind::Rtcp:
			return "rtcp";
		case PacketKind::Unknown:
			return "unknown";
	}
	return "unknown";
}

} // namespace webrtc_qos_plain
