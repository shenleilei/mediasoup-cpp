#pragma once

#include <cstddef>
#include <cstdint>

namespace webrtc_qos_plain {

enum class PacketKind {
	Unknown,
	Rtp,
	Rtcp
};

PacketKind ClassifyRtpOrRtcp(const uint8_t* data, size_t size);
const char* PacketKindName(PacketKind kind);

} // namespace webrtc_qos_plain
