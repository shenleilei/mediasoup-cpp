#include <gtest/gtest.h>

#include "../client/PlainClientApp.h"
#include "../client/Vp8Packetizer.h"

#include <vector>

TEST(PlainClientVp8Test, PictureIdStateIsIndependentPerTrack)
{
	mediasoup::plainclient::Vp8PacketizerState trackA{};
	mediasoup::plainclient::Vp8PacketizerState trackB{};
	uint16_t seqA = 1;
	uint16_t seqB = 100;
	const uint8_t payload[8] = {0, 1, 2, 3, 4, 5, 6, 7};

	std::vector<uint16_t> trackAPictureIds;
	std::vector<uint16_t> trackBPictureIds;

	auto collectA = [&trackAPictureIds](const uint8_t* packet, size_t len) {
		trackAPictureIds.push_back(mediasoup::plainclient::ParseVp8PictureId(packet, len));
	};
	auto collectB = [&trackBPictureIds](const uint8_t* packet, size_t len) {
		trackBPictureIds.push_back(mediasoup::plainclient::ParseVp8PictureId(packet, len));
	};

	mediasoup::plainclient::PacketizeVp8Frame(payload, sizeof(payload), 120, 90000, 1111, &seqA, &trackA, collectA);
	mediasoup::plainclient::PacketizeVp8Frame(payload, sizeof(payload), 120, 90000, 2222, &seqB, &trackB, collectB);
	mediasoup::plainclient::PacketizeVp8Frame(payload, sizeof(payload), 120, 93600, 1111, &seqA, &trackA, collectA);

	ASSERT_FALSE(trackAPictureIds.empty());
	ASSERT_FALSE(trackBPictureIds.empty());
	EXPECT_EQ(trackAPictureIds.front(), 0);
	EXPECT_EQ(trackBPictureIds.front(), 0);
	EXPECT_EQ(trackAPictureIds.back(), 1);
	EXPECT_EQ(trackA.pictureId, 2);
	EXPECT_EQ(trackB.pictureId, 1);
}

TEST(PlainClientVp8Test, Vp8ModeDoesNotAllowCopyFallback)
{
	EXPECT_TRUE(PlainClientApp::ShouldFallbackToCopyMode(PlainClientApp::VideoCodecMode::H264));
	EXPECT_FALSE(PlainClientApp::ShouldFallbackToCopyMode(PlainClientApp::VideoCodecMode::VP8));
}
