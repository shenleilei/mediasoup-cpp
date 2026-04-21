#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace mediasoup::plainclient {

static constexpr uint8_t RTCP_RTPFB_FMT_TRANSPORT_CC = 15;

struct TransportCcFeedbackSummary {
	uint32_t senderSsrc{ 0 };
	uint32_t mediaSsrc{ 0 };
	uint16_t baseSequenceNumber{ 0 };
	uint16_t packetStatusCount{ 0 };
	uint32_t referenceTime{ 0 };
	uint8_t feedbackPacketCount{ 0 };
	uint32_t receivedPacketCount{ 0 };
	uint32_t lostPacketCount{ 0 };
};

inline bool ParseTransportCcFeedbackSummary(
	const uint8_t* packet,
	size_t packetLen,
	TransportCcFeedbackSummary* summary)
{
	if (!packet || !summary || packetLen < 20) {
		return false;
	}

	const uint8_t version = static_cast<uint8_t>((packet[0] >> 6) & 0x03);
	const uint8_t fmt = static_cast<uint8_t>(packet[0] & 0x1F);
	const uint8_t pt = packet[1];
	if (version != 2 || pt != 205 || fmt != RTCP_RTPFB_FMT_TRANSPORT_CC) {
		return false;
	}

	const uint16_t lengthWords = static_cast<uint16_t>((packet[2] << 8) | packet[3]);
	const size_t expectedLen = static_cast<size_t>(lengthWords + 1) * 4;
	if (expectedLen != packetLen) {
		return false;
	}

	TransportCcFeedbackSummary parsed;
	parsed.senderSsrc =
		(static_cast<uint32_t>(packet[4]) << 24) |
		(static_cast<uint32_t>(packet[5]) << 16) |
		(static_cast<uint32_t>(packet[6]) << 8) |
		static_cast<uint32_t>(packet[7]);
	parsed.mediaSsrc =
		(static_cast<uint32_t>(packet[8]) << 24) |
		(static_cast<uint32_t>(packet[9]) << 16) |
		(static_cast<uint32_t>(packet[10]) << 8) |
		static_cast<uint32_t>(packet[11]);
	parsed.baseSequenceNumber = static_cast<uint16_t>((packet[12] << 8) | packet[13]);
	parsed.packetStatusCount = static_cast<uint16_t>((packet[14] << 8) | packet[15]);
	parsed.referenceTime =
		(static_cast<uint32_t>(packet[16]) << 16) |
		(static_cast<uint32_t>(packet[17]) << 8) |
		static_cast<uint32_t>(packet[18]);
	parsed.feedbackPacketCount = packet[19];

	std::vector<uint8_t> symbols;
	symbols.reserve(parsed.packetStatusCount);

	size_t offset = 20;
	while (symbols.size() < parsed.packetStatusCount) {
		if (offset + 2 > packetLen) {
			return false;
		}

		const uint16_t chunk = static_cast<uint16_t>((packet[offset] << 8) | packet[offset + 1]);
		offset += 2;

		if ((chunk & 0x8000u) == 0) {
			// Run Length Chunk.
			const uint8_t symbol = static_cast<uint8_t>((chunk >> 13) & 0x03);
			uint16_t runLength = static_cast<uint16_t>(chunk & 0x1FFFu);
			while (runLength > 0 && symbols.size() < parsed.packetStatusCount) {
				symbols.push_back(symbol);
				runLength--;
			}
			continue;
		}

		if ((chunk & 0x4000u) == 0) {
			// Status Vector Chunk (1-bit symbols, 14 entries).
			for (int i = 0; i < 14 && symbols.size() < parsed.packetStatusCount; ++i) {
				const int shift = 13 - i;
				const uint8_t symbol = static_cast<uint8_t>((chunk >> shift) & 0x01);
				symbols.push_back(symbol);
			}
			continue;
		}

		// Status Vector Chunk (2-bit symbols, 7 entries).
		for (int i = 0; i < 7 && symbols.size() < parsed.packetStatusCount; ++i) {
			const int shift = 12 - (i * 2);
			const uint8_t symbol = static_cast<uint8_t>((chunk >> shift) & 0x03);
			symbols.push_back(symbol);
		}
	}

	size_t deltaOffset = offset;
	for (const uint8_t symbol : symbols) {
		switch (symbol) {
			case 0:
				parsed.lostPacketCount++;
				break;
			case 1:
				if (deltaOffset + 1 > packetLen) {
					return false;
				}
				deltaOffset += 1;
				parsed.receivedPacketCount++;
				break;
			case 2:
				if (deltaOffset + 2 > packetLen) {
					return false;
				}
				deltaOffset += 2;
				parsed.receivedPacketCount++;
				break;
			default:
				// Reserved/unknown symbol. Skip delta parsing and do not count.
				break;
		}
	}

	*summary = parsed;
	return true;
}

inline bool RewriteRtpWithTransportCcSequence(
	const uint8_t* packet,
	size_t packetLen,
	uint8_t extensionId,
	uint16_t transportWideSequence,
	uint8_t* outPacket,
	size_t* outPacketLen)
{
	if (!packet || !outPacket || !outPacketLen || packetLen < 12 || extensionId == 0 || extensionId > 14) {
		return false;
	}

	const size_t outCapacity = *outPacketLen;
	if (outCapacity < packetLen) {
		return false;
	}

	const uint8_t version = static_cast<uint8_t>((packet[0] >> 6) & 0x03);
	if (version != 2) {
		return false;
	}

	const size_t csrcCount = static_cast<size_t>(packet[0] & 0x0F);
	const bool hasExtension = (packet[0] & 0x10) != 0;
	const size_t baseHeaderLen = 12 + csrcCount * 4;
	if (packetLen < baseHeaderLen) {
		return false;
	}

	if (!hasExtension) {
		// Add one-byte RTP header extension with one 2-byte transport-wide sequence element.
		static constexpr size_t kTransportCcExtensionBytes = 8; // 4-byte ext header + 4-byte padded payload.
		const size_t requiredLen = packetLen + kTransportCcExtensionBytes;
		if (requiredLen > outCapacity) {
			return false;
		}

		std::memcpy(outPacket, packet, baseHeaderLen);
		outPacket[0] = static_cast<uint8_t>(outPacket[0] | 0x10); // Set X bit.

		const size_t extOffset = baseHeaderLen;
		outPacket[extOffset + 0] = 0xBE;
		outPacket[extOffset + 1] = 0xDE;
		outPacket[extOffset + 2] = 0x00;
		outPacket[extOffset + 3] = 0x01; // 1 word == 4 bytes of extension payload.
		outPacket[extOffset + 4] = static_cast<uint8_t>((extensionId << 4) | 0x01); // length=2 bytes => encoded as 1.
		outPacket[extOffset + 5] = static_cast<uint8_t>(transportWideSequence >> 8);
		outPacket[extOffset + 6] = static_cast<uint8_t>(transportWideSequence & 0xFF);
		outPacket[extOffset + 7] = 0x00; // padding.

		std::memcpy(
			outPacket + extOffset + kTransportCcExtensionBytes,
			packet + baseHeaderLen,
			packetLen - baseHeaderLen);

		*outPacketLen = requiredLen;
		return true;
	}

	// Update existing one-byte extension in-place (copied to outPacket).
	if (packetLen < baseHeaderLen + 4) {
		return false;
	}

	const uint16_t extensionProfile =
		static_cast<uint16_t>((packet[baseHeaderLen] << 8) | packet[baseHeaderLen + 1]);
	const size_t extensionWords =
		static_cast<size_t>((packet[baseHeaderLen + 2] << 8) | packet[baseHeaderLen + 3]);
	const size_t extensionDataLen = extensionWords * 4;
	const size_t extensionDataOffset = baseHeaderLen + 4;
	if (packetLen < extensionDataOffset + extensionDataLen) {
		return false;
	}

	if (extensionProfile != 0xBEDE) {
		return false;
	}

	std::memcpy(outPacket, packet, packetLen);
	*outPacketLen = packetLen;

	size_t elementOffset = 0;
	while (elementOffset < extensionDataLen) {
		const uint8_t headerByte = outPacket[extensionDataOffset + elementOffset];
		if (headerByte == 0) {
			elementOffset++;
			continue;
		}

		const uint8_t id = static_cast<uint8_t>(headerByte >> 4);
		if (id == 15) {
			break;
		}

		const size_t valueLen = static_cast<size_t>(headerByte & 0x0F) + 1;
		if (elementOffset + 1 + valueLen > extensionDataLen) {
			return false;
		}

		if (id == extensionId && valueLen == 2) {
			outPacket[extensionDataOffset + elementOffset + 1] =
				static_cast<uint8_t>(transportWideSequence >> 8);
			outPacket[extensionDataOffset + elementOffset + 2] =
				static_cast<uint8_t>(transportWideSequence & 0xFF);
			return true;
		}

		elementOffset += 1 + valueLen;
	}

	return false;
}

} // namespace mediasoup::plainclient
