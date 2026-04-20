#include <gtest/gtest.h>

#include "media/h264/AnnexB.h"
#include "media/h264/Avcc.h"
#include "media/rtp/H264Packetizer.h"
#include "media/rtp/RtpHeader.h"

#include <vector>

using namespace mediasoup;

namespace {

class CollectingPacketSink final : public media::rtp::H264PacketSink {
public:
	void OnPacket(const uint8_t* packet, size_t packetLen) override
	{
		packets.emplace_back(packet, packet + packetLen);
	}

	std::vector<std::vector<uint8_t>> packets;
};

std::vector<uint8_t> ReconstructFuANalu(const std::vector<std::vector<uint8_t>>& packets)
{
	std::vector<uint8_t> reconstructed;
	for (size_t i = 0; i < packets.size(); ++i) {
		media::rtp::RtpHeader header;
		if (!media::rtp::RtpHeader::Parse(
				packets[i].data(), packets[i].size(), &header)) {
			ADD_FAILURE() << "failed to parse RTP packet " << i;
			return {};
		}
		if (header.payloadSize < 2u) {
			ADD_FAILURE() << "unexpected FU-A payload size for packet " << i;
			return {};
		}
		if (i == 0) {
			const uint8_t nalHeader = static_cast<uint8_t>(
				(header.payload[0] & 0xE0) | (header.payload[1] & 0x1F));
			reconstructed.push_back(nalHeader);
		}
		reconstructed.insert(
			reconstructed.end(),
			header.payload + 2,
			header.payload + header.payloadSize);
	}
	return reconstructed;
}

} // namespace

TEST(RtpHeaderSharedTest, MarkerBitParsed)
{
	uint8_t packet[20] = {0x80, static_cast<uint8_t>(0x80 | 100), 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0xAA};
	media::rtp::RtpHeader header;
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(packet, sizeof(packet), &header));
	EXPECT_TRUE(header.marker);
	EXPECT_EQ(header.payloadType, 100);

	packet[1] = 100;
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(packet, sizeof(packet), &header));
	EXPECT_FALSE(header.marker);
}

TEST(RtpHeaderSharedTest, TooShortFails)
{
	uint8_t packet[8] = {};
	media::rtp::RtpHeader header;
	EXPECT_FALSE(media::rtp::RtpHeader::Parse(packet, sizeof(packet), &header));
}

TEST(AnnexBToAvccSharedTest, SingleNal)
{
	std::vector<uint8_t> annexB = {0, 0, 0, 1, 0x65, 0xAA, 0xBB};
	std::vector<uint8_t> avcc;
	media::h264::AnnexBToAvcc(annexB, &avcc);
	ASSERT_EQ(avcc.size(), 7u);
	const uint32_t nalLen =
		(static_cast<uint32_t>(avcc[0]) << 24) |
		(static_cast<uint32_t>(avcc[1]) << 16) |
		(static_cast<uint32_t>(avcc[2]) << 8) |
		static_cast<uint32_t>(avcc[3]);
	EXPECT_EQ(nalLen, 3u);
	EXPECT_EQ(avcc[4], 0x65);
	EXPECT_EQ(avcc[5], 0xAA);
	EXPECT_EQ(avcc[6], 0xBB);
}

TEST(AnnexBToAvccSharedTest, TwoNals)
{
	std::vector<uint8_t> annexB = {0, 0, 0, 1, 0x67, 0x42, 0, 0, 0, 1, 0x68, 0x01};
	std::vector<uint8_t> avcc;
	media::h264::AnnexBToAvcc(annexB, &avcc);
	ASSERT_EQ(avcc.size(), 12u);
	const uint32_t firstNalLen =
		(static_cast<uint32_t>(avcc[0]) << 24) |
		(static_cast<uint32_t>(avcc[1]) << 16) |
		(static_cast<uint32_t>(avcc[2]) << 8) |
		static_cast<uint32_t>(avcc[3]);
	const uint32_t secondNalLen =
		(static_cast<uint32_t>(avcc[6]) << 24) |
		(static_cast<uint32_t>(avcc[7]) << 16) |
		(static_cast<uint32_t>(avcc[8]) << 8) |
		static_cast<uint32_t>(avcc[9]);
	EXPECT_EQ(firstNalLen, 2u);
	EXPECT_EQ(secondNalLen, 2u);
	EXPECT_EQ(avcc[4], 0x67);
	EXPECT_EQ(avcc[10], 0x68);
}

TEST(AnnexBToAvccSharedTest, EmptyInput)
{
	std::vector<uint8_t> avcc;
	media::h264::AnnexBToAvcc(nullptr, 0, &avcc);
	EXPECT_TRUE(avcc.empty());
}

TEST(AnnexBReaderSharedTest, MixedThreeAndFourByteStartCodes)
{
	std::vector<uint8_t> annexB = {
		0, 0, 0, 1, 0x67, 0x42,
		0, 0, 1, 0x68, 0x01,
		0, 0, 0, 1, 0x65, 0xAA, 0xBB
	};

	media::h264::AnnexBReader reader(annexB.data(), annexB.size());
	media::h264::NaluView nalu;
	ASSERT_TRUE(reader.Next(&nalu));
	ASSERT_EQ(nalu.size, 2u);
	EXPECT_EQ(nalu.data[0], 0x67);
	EXPECT_EQ(nalu.data[1], 0x42);

	ASSERT_TRUE(reader.Next(&nalu));
	ASSERT_EQ(nalu.size, 2u);
	EXPECT_EQ(nalu.data[0], 0x68);
	EXPECT_EQ(nalu.data[1], 0x01);

	ASSERT_TRUE(reader.Next(&nalu));
	ASSERT_EQ(nalu.size, 3u);
	EXPECT_EQ(nalu.data[0], 0x65);
	EXPECT_EQ(nalu.data[1], 0xAA);
	EXPECT_EQ(nalu.data[2], 0xBB);

	EXPECT_FALSE(reader.Next(&nalu));
}

TEST(AnnexBReaderSharedTest, ConsecutiveAndTrailingStartCodesIgnored)
{
	std::vector<uint8_t> annexB = {
		0, 0, 0, 1,
		0, 0, 1,
		0x65, 0xAA,
		0, 0, 0, 1,
		0, 0, 1,
		0x41, 0xBB,
		0, 0, 0, 1
	};

	media::h264::AnnexBReader reader(annexB.data(), annexB.size());
	media::h264::NaluView nalu;
	ASSERT_TRUE(reader.Next(&nalu));
	ASSERT_EQ(nalu.size, 2u);
	EXPECT_EQ(nalu.data[0], 0x65);
	EXPECT_EQ(nalu.data[1], 0xAA);

	ASSERT_TRUE(reader.Next(&nalu));
	ASSERT_EQ(nalu.size, 2u);
	EXPECT_EQ(nalu.data[0], 0x41);
	EXPECT_EQ(nalu.data[1], 0xBB);

	EXPECT_FALSE(reader.Next(&nalu));
}

TEST(AnnexBReaderSharedTest, NoStartCodeReturnsFalse)
{
	std::vector<uint8_t> payload = {0x65, 0xAA, 0xBB};
	media::h264::AnnexBReader reader(payload.data(), payload.size());
	media::h264::NaluView nalu;
	EXPECT_FALSE(reader.Next(&nalu));
}

TEST(H264PacketizerSharedTest, SingleNalUnitUsesOnePacket)
{
	std::vector<uint8_t> annexB = {0, 0, 0, 1, 0x65, 0xAA, 0xBB};
	uint16_t sequenceNumber = 100;
	CollectingPacketSink sink;

	const size_t emitted = media::rtp::H264Packetizer::PacketizeAnnexB(
		annexB, 96, 90000, 1234, &sequenceNumber, &sink);

	ASSERT_EQ(emitted, 1u);
	ASSERT_EQ(sink.packets.size(), 1u);
	EXPECT_EQ(sequenceNumber, 101);

	media::rtp::RtpHeader header;
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(
		sink.packets.front().data(), sink.packets.front().size(), &header));
	EXPECT_TRUE(header.marker);
	EXPECT_EQ(header.payloadType, 96);
	EXPECT_EQ(header.sequenceNumber, 100);
	ASSERT_EQ(header.payloadSize, 3u);
	EXPECT_EQ(header.payload[0], 0x65);
	EXPECT_EQ(header.payload[1], 0xAA);
	EXPECT_EQ(header.payload[2], 0xBB);
}

TEST(H264PacketizerSharedTest, MarkerBitOnlyOnLastNal)
{
	std::vector<uint8_t> annexB = {
		0, 0, 0, 1, 0x61, 0x01,
		0, 0, 0, 1, 0x65, 0x02
	};
	uint16_t sequenceNumber = 7;
	CollectingPacketSink sink;

	const size_t emitted = media::rtp::H264Packetizer::PacketizeAnnexB(
		annexB, 97, 12345, 5678, &sequenceNumber, &sink);

	ASSERT_EQ(emitted, 2u);
	ASSERT_EQ(sink.packets.size(), 2u);
	EXPECT_EQ(sequenceNumber, 9);

	media::rtp::RtpHeader first;
	media::rtp::RtpHeader second;
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(sink.packets[0].data(), sink.packets[0].size(), &first));
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(sink.packets[1].data(), sink.packets[1].size(), &second));
	EXPECT_FALSE(first.marker);
	EXPECT_TRUE(second.marker);
	EXPECT_EQ(first.sequenceNumber, 7);
	EXPECT_EQ(second.sequenceNumber, 8);
}

TEST(H264PacketizerSharedTest, LargeNalFragmentsAsFuA)
{
	std::vector<uint8_t> annexB = {0, 0, 0, 1, 0x65};
	for (size_t i = 0; i < 2500; ++i)
		annexB.push_back(static_cast<uint8_t>(i & 0xFF));

	uint16_t sequenceNumber = 42;
	CollectingPacketSink sink;

	const size_t emitted = media::rtp::H264Packetizer::PacketizeAnnexB(
		annexB, 102, 43210, 9876, &sequenceNumber, &sink);

	ASSERT_GT(emitted, 1u);
	ASSERT_EQ(sink.packets.size(), emitted);
	EXPECT_EQ(sequenceNumber, static_cast<uint16_t>(42 + emitted));

	media::rtp::RtpHeader first;
	media::rtp::RtpHeader last;
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(sink.packets.front().data(), sink.packets.front().size(), &first));
	ASSERT_TRUE(media::rtp::RtpHeader::Parse(sink.packets.back().data(), sink.packets.back().size(), &last));
	ASSERT_GE(first.payloadSize, 2u);
	ASSERT_GE(last.payloadSize, 2u);
	EXPECT_FALSE(first.marker);
	EXPECT_TRUE(last.marker);
	EXPECT_EQ(first.payload[0] & 0x1F, 28);
	EXPECT_TRUE((first.payload[1] & 0x80) != 0);
	EXPECT_FALSE((first.payload[1] & 0x40) != 0);
	EXPECT_EQ(last.payload[0] & 0x1F, 28);
	EXPECT_TRUE((last.payload[1] & 0x40) != 0);

	const auto reconstructed = ReconstructFuANalu(sink.packets);
	ASSERT_EQ(reconstructed.size(), annexB.size() - 4);
	EXPECT_EQ(reconstructed[0], 0x65);
	EXPECT_EQ(reconstructed.back(), annexB.back());
}
