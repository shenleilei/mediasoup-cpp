#include "media/rtp/H264Packetizer.h"

#include "media/h264/AnnexB.h"

#include <algorithm>
#include <cstring>

namespace mediasoup::media::rtp {
namespace {

void WriteRtpHeader(
	uint8_t* buf,
	uint8_t payloadType,
	uint16_t sequenceNumber,
	uint32_t rtpTimestamp,
	uint32_t ssrc,
	bool marker)
{
	buf[0] = 0x80;
	buf[1] = static_cast<uint8_t>((marker ? 0x80 : 0x00) | (payloadType & 0x7F));
	buf[2] = static_cast<uint8_t>(sequenceNumber >> 8);
	buf[3] = static_cast<uint8_t>(sequenceNumber & 0xFF);
	buf[4] = static_cast<uint8_t>(rtpTimestamp >> 24);
	buf[5] = static_cast<uint8_t>(rtpTimestamp >> 16);
	buf[6] = static_cast<uint8_t>(rtpTimestamp >> 8);
	buf[7] = static_cast<uint8_t>(rtpTimestamp & 0xFF);
	buf[8] = static_cast<uint8_t>(ssrc >> 24);
	buf[9] = static_cast<uint8_t>(ssrc >> 16);
	buf[10] = static_cast<uint8_t>(ssrc >> 8);
	buf[11] = static_cast<uint8_t>(ssrc & 0xFF);
}

size_t EmitSingleNalu(
	const media::h264::NaluView& nalu,
	bool marker,
	uint8_t payloadType,
	uint32_t rtpTimestamp,
	uint32_t ssrc,
	uint16_t* sequenceNumber,
	H264PacketSink* sink)
{
	uint8_t packet[12 + H264Packetizer::kMaxPayloadBytes];
	WriteRtpHeader(packet, payloadType, (*sequenceNumber)++, rtpTimestamp, ssrc, marker);
	std::memcpy(packet + 12, nalu.data, nalu.size);
	sink->OnPacket(packet, 12 + nalu.size);
	return 1;
}

size_t EmitFragmentedNalu(
	const media::h264::NaluView& nalu,
	bool marker,
	uint8_t payloadType,
	uint32_t rtpTimestamp,
	uint32_t ssrc,
	uint16_t* sequenceNumber,
	H264PacketSink* sink)
{
	const uint8_t naluHeader = nalu.data[0];
	const uint8_t fuIndicator = static_cast<uint8_t>((naluHeader & 0x60) | 28);
	size_t packets = 0;
	size_t offset = 1;
	bool first = true;

	while (offset < nalu.size) {
		const size_t chunkSize = std::min(
			H264Packetizer::kMaxPayloadBytes - 2,
			nalu.size - offset);
		const bool end = (offset + chunkSize) >= nalu.size;

		uint8_t packet[12 + 2 + H264Packetizer::kMaxPayloadBytes];
		WriteRtpHeader(packet, payloadType, (*sequenceNumber)++, rtpTimestamp, ssrc, end && marker);
		packet[12] = fuIndicator;
		packet[13] = static_cast<uint8_t>(
			(naluHeader & 0x1F) |
			(first ? 0x80 : 0x00) |
			(end ? 0x40 : 0x00));
		std::memcpy(packet + 14, nalu.data + offset, chunkSize);
		sink->OnPacket(packet, 14 + chunkSize);
		offset += chunkSize;
		first = false;
		++packets;
	}

	return packets;
}

} // namespace

size_t H264Packetizer::PacketizeAnnexB(
	const uint8_t* annexB,
	size_t annexBSize,
	uint8_t payloadType,
	uint32_t rtpTimestamp,
	uint32_t ssrc,
	uint16_t* sequenceNumber,
	H264PacketSink* sink)
{
	if (!annexB || annexBSize == 0 || !sequenceNumber || !sink) return 0;

	media::h264::AnnexBReader reader(annexB, annexBSize);
	media::h264::NaluView current;
	if (!reader.Next(&current)) return 0;

	media::h264::NaluView next;
	bool hasNext = reader.Next(&next);
	size_t emittedPackets = 0;

	while (true) {
		const bool marker = !hasNext;
		if (current.size <= kMaxPayloadBytes) {
			emittedPackets += EmitSingleNalu(
				current, marker, payloadType, rtpTimestamp, ssrc, sequenceNumber, sink);
		} else {
			emittedPackets += EmitFragmentedNalu(
				current, marker, payloadType, rtpTimestamp, ssrc, sequenceNumber, sink);
		}

		if (!hasNext) break;
		current = next;
		hasNext = reader.Next(&next);
	}

	return emittedPackets;
}

} // namespace mediasoup::media::rtp
