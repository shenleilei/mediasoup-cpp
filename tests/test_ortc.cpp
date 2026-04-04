#include <gtest/gtest.h>
#include "ortc.h"

using namespace mediasoup;

// --- matchCodecs ---

TEST(OrtcTest, MatchCodecsSameMimeType) {
	RtpCodecCapability a{"audio", "audio/opus", 100, 48000, 2, {}, {}};
	RtpCodecCapability b{"audio", "audio/opus", 111, 48000, 2, {}, {}};
	EXPECT_TRUE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsDifferentMimeType) {
	RtpCodecCapability a{"audio", "audio/opus", 100, 48000, 2, {}, {}};
	RtpCodecCapability b{"video", "video/VP8", 101, 90000, 0, {}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsDifferentClockRate) {
	RtpCodecCapability a{"audio", "audio/opus", 100, 48000, 2, {}, {}};
	RtpCodecCapability b{"audio", "audio/opus", 100, 16000, 2, {}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsCaseInsensitive) {
	RtpCodecCapability a{"video", "video/vp8", 100, 90000, 0, {}, {}};
	RtpCodecCapability b{"video", "video/VP8", 101, 90000, 0, {}, {}};
	EXPECT_TRUE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsH264ProfileMatch) {
	RtpCodecCapability a{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "4d0032"}, {"packetization-mode", 1}}, {}};
	RtpCodecCapability b{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "4d0033"}, {"packetization-mode", 1}}, {}};
	// Same profile (4d00), different level → should match (non-strict)
	EXPECT_TRUE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsH264ProfileMismatch) {
	RtpCodecCapability a{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "42e01f"}}, {}};
	RtpCodecCapability b{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "4d0032"}}, {}};
	// Different profile (42e0 vs 4d00)
	EXPECT_FALSE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsH264StrictPacketizationMode) {
	RtpCodecCapability a{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "4d0032"}, {"packetization-mode", 0}}, {}};
	RtpCodecCapability b{"video", "video/H264", 0, 90000, 0,
		{{"profile-level-id", "4d0032"}, {"packetization-mode", 1}}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b, {true, false}));
	EXPECT_TRUE(ortc::matchCodecs(a, b, {false, false}));
}

// --- isRtxCodec ---

TEST(OrtcTest, IsRtxCodec) {
	RtpCodecCapability rtx{"audio", "audio/rtx", 101, 48000, 0, {}, {}};
	RtpCodecCapability opus{"audio", "audio/opus", 100, 48000, 2, {}, {}};
	EXPECT_TRUE(ortc::isRtxCodec(rtx));
	EXPECT_FALSE(ortc::isRtxCodec(opus));
}

// --- generateRouterRtpCapabilities ---

TEST(OrtcTest, GenerateRouterRtpCapabilities_Basic) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/opus", 0, 48000, 2, {}, {}},
		{"video", "video/VP8", 0, 90000, 0, {}, {}},
	};
	auto caps = ortc::generateRouterRtpCapabilities(mediaCodecs);

	// Should have 2 media codecs + 2 RTX codecs = 4
	EXPECT_EQ(caps.codecs.size(), 4u);

	// First codec should be opus
	EXPECT_EQ(caps.codecs[0].mimeType, "audio/opus");
	EXPECT_EQ(caps.codecs[0].clockRate, 48000u);
	EXPECT_GT(caps.codecs[0].preferredPayloadType, 0);

	// Second should be opus RTX
	EXPECT_EQ(caps.codecs[1].mimeType, "audio/rtx");

	// Third should be VP8
	EXPECT_EQ(caps.codecs[2].mimeType, "video/VP8");

	// Fourth should be VP8 RTX
	EXPECT_EQ(caps.codecs[3].mimeType, "video/rtx");

	// Header extensions should be populated
	EXPECT_FALSE(caps.headerExtensions.empty());
}

TEST(OrtcTest, GenerateRouterRtpCapabilities_NoDuplicatePT) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/opus", 0, 48000, 2, {}, {}},
		{"video", "video/VP8", 0, 90000, 0, {}, {}},
		{"video", "video/VP9", 0, 90000, 0, {{"profile-id", 0}}, {}},
	};
	auto caps = ortc::generateRouterRtpCapabilities(mediaCodecs);

	// Collect all PTs, verify no duplicates
	std::set<uint8_t> pts;
	for (auto& c : caps.codecs) {
		EXPECT_TRUE(pts.insert(c.preferredPayloadType).second)
			<< "Duplicate PT: " << (int)c.preferredPayloadType
			<< " for " << c.mimeType;
	}
}

TEST(OrtcTest, GenerateRouterRtpCapabilities_RejectsRtxInput) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/rtx", 101, 48000, 0, {}, {}},
	};
	EXPECT_THROW(ortc::generateRouterRtpCapabilities(mediaCodecs), std::runtime_error);
}

TEST(OrtcTest, GenerateRouterRtpCapabilities_RejectsUnsupported) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/FAKE", 0, 48000, 0, {}, {}},
	};
	EXPECT_THROW(ortc::generateRouterRtpCapabilities(mediaCodecs), std::runtime_error);
}

// --- h264ProfilesMatch ---

TEST(OrtcTest, H264ProfilesMatch) {
	EXPECT_TRUE(ortc::h264ProfilesMatch(
		{{"profile-level-id", "42e01f"}}, {{"profile-level-id", "42e034"}}));
	EXPECT_FALSE(ortc::h264ProfilesMatch(
		{{"profile-level-id", "42e01f"}}, {{"profile-level-id", "4d0032"}}));
}

TEST(OrtcTest, H264ProfilesMatchDefault) {
	// Both default to 4d0032
	EXPECT_TRUE(ortc::h264ProfilesMatch(json::object(), json::object()));
}
