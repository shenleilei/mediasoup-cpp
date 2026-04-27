#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace mediasoup::plainclient {

struct Vp8PacketizerState {
	uint16_t pictureId{0};
};

inline void WriteRtpHeader(
	uint8_t* buf,
	uint8_t payloadType,
	uint16_t seq,
	uint32_t ts,
	uint32_t ssrc,
	bool marker)
{
	buf[0] = 0x80;
	buf[1] = static_cast<uint8_t>((marker ? 0x80 : 0) | (payloadType & 0x7F));
	buf[2] = static_cast<uint8_t>(seq >> 8);
	buf[3] = static_cast<uint8_t>(seq & 0xFF);
	buf[4] = static_cast<uint8_t>(ts >> 24);
	buf[5] = static_cast<uint8_t>(ts >> 16);
	buf[6] = static_cast<uint8_t>(ts >> 8);
	buf[7] = static_cast<uint8_t>(ts);
	buf[8] = static_cast<uint8_t>(ssrc >> 24);
	buf[9] = static_cast<uint8_t>(ssrc >> 16);
	buf[10] = static_cast<uint8_t>(ssrc >> 8);
	buf[11] = static_cast<uint8_t>(ssrc);
}

template<typename PacketSinkFn>
inline void PacketizeVp8Frame(
	const uint8_t* data,
	size_t size,
	uint8_t payloadType,
	uint32_t ts,
	uint32_t ssrc,
	uint16_t* seq,
	Vp8PacketizerState* state,
	PacketSinkFn&& packetSink)
{
	if (!data || size == 0 || !seq || !state) return;

	static constexpr size_t kMaxRtpPacketBytes = 1200;
	static constexpr size_t kRtpHeaderBytes = 12;
	static constexpr size_t kVp8DescriptorBytes = 4;
	static constexpr size_t kMaxChunkBytes =
		kMaxRtpPacketBytes - kRtpHeaderBytes - kVp8DescriptorBytes;

	const uint16_t currentPictureId = state->pictureId++;

	size_t offset = 0;
	while (offset < size) {
		const size_t chunkSize = std::min(kMaxChunkBytes, size - offset);
		const bool firstChunk = offset == 0;
		const bool lastChunk = offset + chunkSize >= size;

		uint8_t packet[kMaxRtpPacketBytes];
		WriteRtpHeader(packet, payloadType, (*seq)++, ts, ssrc, lastChunk);
		packet[12] = static_cast<uint8_t>(0x80 | (firstChunk ? 0x10 : 0x00));
		packet[13] = 0x80;
		packet[14] = static_cast<uint8_t>(0x80 | ((currentPictureId >> 8) & 0x7f));
		packet[15] = static_cast<uint8_t>(currentPictureId & 0xff);
		std::memcpy(packet + 16, data + offset, chunkSize);

		packetSink(packet, 16 + chunkSize);
		offset += chunkSize;
	}
}

inline uint16_t ParseVp8PictureId(const uint8_t* packet, size_t len)
{
	if (!packet || len < 16) return 0;
	return static_cast<uint16_t>(((packet[14] & 0x7f) << 8) | packet[15]);
}

} // namespace mediasoup::plainclient
