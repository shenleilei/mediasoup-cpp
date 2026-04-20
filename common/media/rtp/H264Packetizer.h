#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mediasoup::media::rtp {

class H264PacketSink {
public:
	virtual ~H264PacketSink() = default;

	// The packet memory is only valid for the duration of this callback.
	virtual void OnPacket(const uint8_t* packet, size_t packetLen) = 0;
};

class H264Packetizer {
public:
	static constexpr size_t kMaxPayloadBytes = 1200;

	static size_t PacketizeAnnexB(
		const uint8_t* annexB,
		size_t annexBSize,
		uint8_t payloadType,
		uint32_t rtpTimestamp,
		uint32_t ssrc,
		uint16_t* sequenceNumber,
		H264PacketSink* sink);

	static size_t PacketizeAnnexB(
		const std::vector<uint8_t>& annexB,
		uint8_t payloadType,
		uint32_t rtpTimestamp,
		uint32_t ssrc,
		uint16_t* sequenceNumber,
		H264PacketSink* sink)
	{
		return PacketizeAnnexB(
			annexB.data(),
			annexB.size(),
			payloadType,
			rtpTimestamp,
			ssrc,
			sequenceNumber,
			sink);
	}
};

} // namespace mediasoup::media::rtp
