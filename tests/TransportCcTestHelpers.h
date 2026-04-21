#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace transportcc_test {

inline std::vector<uint8_t> BuildTransportCcFeedbackPacketFromPacketDeltas(
	const std::vector<std::optional<int16_t>>& deltas250us,
	uint16_t baseSequenceNumber = 1000,
	uint32_t senderSsrc = 0x11223344u,
	uint32_t mediaSsrc = 0x55667788u,
	uint32_t referenceTime = 0x000110u,
	uint8_t feedbackPacketCount = 0x10)
{
	std::vector<uint8_t> packet(20, 0);
	packet[0] = 0x8F;
	packet[1] = 205;
	packet[4] = static_cast<uint8_t>(senderSsrc >> 24);
	packet[5] = static_cast<uint8_t>(senderSsrc >> 16);
	packet[6] = static_cast<uint8_t>(senderSsrc >> 8);
	packet[7] = static_cast<uint8_t>(senderSsrc & 0xFF);
	packet[8] = static_cast<uint8_t>(mediaSsrc >> 24);
	packet[9] = static_cast<uint8_t>(mediaSsrc >> 16);
	packet[10] = static_cast<uint8_t>(mediaSsrc >> 8);
	packet[11] = static_cast<uint8_t>(mediaSsrc & 0xFF);
	packet[12] = static_cast<uint8_t>(baseSequenceNumber >> 8);
	packet[13] = static_cast<uint8_t>(baseSequenceNumber & 0xFF);
	packet[14] = static_cast<uint8_t>((deltas250us.size() >> 8) & 0xFF);
	packet[15] = static_cast<uint8_t>(deltas250us.size() & 0xFF);
	packet[16] = static_cast<uint8_t>((referenceTime >> 16) & 0xFF);
	packet[17] = static_cast<uint8_t>((referenceTime >> 8) & 0xFF);
	packet[18] = static_cast<uint8_t>(referenceTime & 0xFF);
	packet[19] = feedbackPacketCount;

	std::vector<uint8_t> chunks;
	std::vector<uint8_t> deltas;
	for (size_t chunkBase = 0; chunkBase < deltas250us.size(); chunkBase += 7) {
		uint16_t chunk = 0xC000u;
		for (size_t i = 0; i < 7; ++i) {
			const size_t index = chunkBase + i;
			uint8_t symbol = 0;
			if (index < deltas250us.size() && deltas250us[index].has_value()) {
				const int16_t delta = *deltas250us[index];
				symbol = (delta >= -128 && delta <= 127) ? 1u : 2u;
			}
			const int shift = 12 - static_cast<int>(i * 2);
			chunk = static_cast<uint16_t>(chunk | ((static_cast<uint16_t>(symbol) & 0x03u) << shift));
		}
		chunks.push_back(static_cast<uint8_t>(chunk >> 8));
		chunks.push_back(static_cast<uint8_t>(chunk & 0xFF));
	}

	for (const auto& delta250us : deltas250us) {
		if (!delta250us.has_value()) {
			continue;
		}
		const int16_t delta = *delta250us;
		if (delta >= -128 && delta <= 127) {
			deltas.push_back(static_cast<uint8_t>(static_cast<int8_t>(delta)));
		} else {
			deltas.push_back(static_cast<uint8_t>((static_cast<uint16_t>(delta) >> 8) & 0xFF));
			deltas.push_back(static_cast<uint8_t>(static_cast<uint16_t>(delta) & 0xFF));
		}
	}

	packet.insert(packet.end(), chunks.begin(), chunks.end());
	packet.insert(packet.end(), deltas.begin(), deltas.end());
	while ((packet.size() % 4) != 0) {
		packet.push_back(0x00);
	}

	const uint16_t lengthWords = static_cast<uint16_t>((packet.size() / 4) - 1);
	packet[2] = static_cast<uint8_t>(lengthWords >> 8);
	packet[3] = static_cast<uint8_t>(lengthWords & 0xFF);
	return packet;
}

inline std::vector<uint8_t> BuildTransportCcFeedbackPacketFromRuns(
	const std::vector<std::pair<uint8_t, uint16_t>>& runs,
	uint16_t baseSequenceNumber = 1000,
	uint32_t senderSsrc = 0x11223344u,
	uint32_t mediaSsrc = 0x55667788u,
	uint32_t referenceTime = 0x000110u,
	uint8_t feedbackPacketCount = 0x10)
{
	std::vector<std::optional<int16_t>> deltas250us;
	for (const auto& run : runs) {
		for (uint16_t i = 0; i < run.second; ++i) {
			switch (run.first) {
				case 0:
					deltas250us.push_back(std::nullopt);
					break;
				case 1:
					deltas250us.emplace_back(static_cast<int16_t>(1));
					break;
				case 2:
					deltas250us.emplace_back(static_cast<int16_t>(256));
					break;
				default:
					deltas250us.push_back(std::nullopt);
					break;
			}
		}
	}

	return BuildTransportCcFeedbackPacketFromPacketDeltas(
		deltas250us,
		baseSequenceNumber,
		senderSsrc,
		mediaSsrc,
		referenceTime,
		feedbackPacketCount);
}

} // namespace transportcc_test
