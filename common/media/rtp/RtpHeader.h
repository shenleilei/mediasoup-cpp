#pragma once

#include <cstddef>
#include <cstdint>

namespace mediasoup::media::rtp {

struct RtpHeader {
	uint8_t payloadType{0};
	uint16_t sequenceNumber{0};
	uint32_t timestamp{0};
	uint32_t ssrc{0};
	bool marker{false};
	const uint8_t* payload{nullptr};
	size_t payloadSize{0};

	static bool Parse(const uint8_t* data, size_t len, RtpHeader* header)
	{
		if (!data || !header || len < 12) return false;

		header->marker = (data[1] & 0x80) != 0;
		header->payloadType = data[1] & 0x7F;
		header->sequenceNumber = (static_cast<uint16_t>(data[2]) << 8) | data[3];
		header->timestamp =
			(static_cast<uint32_t>(data[4]) << 24) |
			(static_cast<uint32_t>(data[5]) << 16) |
			(static_cast<uint32_t>(data[6]) << 8) |
			static_cast<uint32_t>(data[7]);
		header->ssrc =
			(static_cast<uint32_t>(data[8]) << 24) |
			(static_cast<uint32_t>(data[9]) << 16) |
			(static_cast<uint32_t>(data[10]) << 8) |
			static_cast<uint32_t>(data[11]);

		size_t headerLen = 12 + static_cast<size_t>(data[0] & 0x0F) * 4;
		if ((data[0] & 0x10) != 0) {
			if (len < headerLen + 4) return false;
			const size_t extLenWords =
				(static_cast<size_t>(data[headerLen + 2]) << 8) |
				static_cast<size_t>(data[headerLen + 3]);
			headerLen += 4 + extLenWords * 4;
		}
		if (headerLen > len) return false;

		header->payload = data + headerLen;
		header->payloadSize = len - headerLen;
		return true;
	}
};

} // namespace mediasoup::media::rtp
