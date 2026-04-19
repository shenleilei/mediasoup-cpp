#include <gtest/gtest.h>
#include "ortc.h"
#include <algorithm>
#include <set>

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

TEST(OrtcTest, MatchCodecsDifferentChannels) {
	RtpCodecCapability a{"audio", "audio/opus", 100, 48000, 1, {}, {}};
	RtpCodecCapability b{"audio", "audio/opus", 101, 48000, 2, {}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsSameChannels) {
	RtpCodecCapability a{"audio", "audio/opus", 100, 48000, 2, {}, {}};
	RtpCodecCapability b{"audio", "audio/opus", 101, 48000, 2, {}, {}};
	EXPECT_TRUE(ortc::matchCodecs(a, b));
}

TEST(OrtcTest, MatchCodecsVp9StrictProfileId) {
	RtpCodecCapability a{"video", "video/VP9", 0, 90000, 0, {{"profile-id", 0}}, {}};
	RtpCodecCapability b{"video", "video/VP9", 0, 90000, 0, {{"profile-id", 1}}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b, {true, false}));
	EXPECT_TRUE(ortc::matchCodecs(a, b, {false, false}));
}

TEST(OrtcTest, MatchCodecsH265StrictProfileId) {
	RtpCodecCapability a{"video", "video/H265", 0, 90000, 0, {{"profile-id", 1}}, {}};
	RtpCodecCapability b{"video", "video/H265", 0, 90000, 0, {{"profile-id", 2}}, {}};
	EXPECT_FALSE(ortc::matchCodecs(a, b, {true, false}));
	EXPECT_TRUE(ortc::matchCodecs(a, b, {false, false}));
}

TEST(OrtcTest, ProducerRtpParametersMappingIncludesRtxAndEncodings) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/opus", 0, 48000, 2, {}, {}},
		{"video", "video/VP8", 0, 90000, 0, {}, {}}
	};
	auto caps = ortc::generateRouterRtpCapabilities(mediaCodecs);

	RtpParameters params;
	params.codecs.push_back({"audio/opus", 111, 48000, 2, {}, {}});
	params.codecs.push_back({"video/VP8", 112, 90000, 0, {}, {}});
	params.codecs.push_back({"video/rtx", 113, 90000, 0, {{"apt", 112}}, {}});
	params.encodings.push_back(RtpEncodingParameters{123456u, "r0", std::nullopt, std::nullopt, false, "S1T3", 250000u});

	auto mapping = ortc::getProducerRtpParametersMapping(params, caps);
	EXPECT_EQ(mapping.encodings.size(), 1u);
	EXPECT_EQ(mapping.encodings[0].rid, "r0");
	EXPECT_EQ(mapping.encodings[0].ssrc.value_or(0), 123456u);
	EXPECT_GE(mapping.encodings[0].mappedSsrc, 100000000u);
	EXPECT_LE(mapping.encodings[0].mappedSsrc, 999999999u);

	bool hasAudio = false;
	bool hasVideo = false;
	bool hasVideoRtx = false;
	for (const auto& cm : mapping.codecs) {
		hasAudio = hasAudio || cm.payloadType == 111;
		hasVideo = hasVideo || cm.payloadType == 112;
		hasVideoRtx = hasVideoRtx || cm.payloadType == 113;
	}
	EXPECT_TRUE(hasAudio);
	EXPECT_TRUE(hasVideo);
	EXPECT_TRUE(hasVideoRtx);
}

TEST(OrtcTest, ProducerRtpParametersMappingRejectsUnsupportedCodec) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"audio", "audio/opus", 0, 48000, 2, {}, {}}
	};
	auto caps = ortc::generateRouterRtpCapabilities(mediaCodecs);

	RtpParameters params;
	params.codecs.push_back({"audio/FAKE", 111, 48000, 2, {}, {}});
	params.encodings.push_back(RtpEncodingParameters{1111u, "", std::nullopt, std::nullopt, false, "", std::nullopt});

	EXPECT_THROW(ortc::getProducerRtpParametersMapping(params, caps), std::runtime_error);
}

TEST(OrtcTest, ConsumableRtpParametersMapsCodecsAndDefaultsScalability) {
	std::vector<RtpCodecCapability> mediaCodecs = {
		{"video", "video/VP8", 0, 90000, 0, {}, {}}
	};
	auto caps = ortc::generateRouterRtpCapabilities(mediaCodecs);

	RtpParameters params;
	params.codecs.push_back({"video/VP8", 112, 90000, 0, {}, {}});
	params.codecs.push_back({"video/rtx", 113, 90000, 0, {{"apt", 112}}, {}});
	params.encodings.push_back(RtpEncodingParameters{1234u, "r0", std::nullopt, std::nullopt, false, "S1T1", 500000u});
	params.rtcp.cname = "stream-cname";

	auto mapping = ortc::getProducerRtpParametersMapping(params, caps);
	auto consumable = ortc::getConsumableRtpParameters("video", params, caps, mapping);

	EXPECT_EQ(consumable.codecs.size(), mapping.codecs.size());
	ASSERT_EQ(consumable.encodings.size(), 1u);
	EXPECT_EQ(consumable.encodings[0].ssrc.value_or(0), mapping.encodings[0].mappedSsrc);
	EXPECT_EQ(consumable.encodings[0].scalabilityMode, "S1T1");
	EXPECT_EQ(consumable.encodings[0].maxBitrate.value_or(0), 500000u);
	EXPECT_EQ(consumable.rtcp.cname, "stream-cname");
	EXPECT_TRUE(consumable.rtcp.reducedSize);
}

TEST(OrtcTest, ConsumerRtpParametersMatchesRemoteCodecWithoutUnnegotiatedRtx) {
	RtpParameters consumable;
	consumable.codecs.push_back({"video/VP8", 120, 90000, 0, {}, {}});
	consumable.codecs.push_back({"video/rtx", 121, 90000, 0, {{"apt", 120}}, {}});
	consumable.headerExtensions.push_back({"urn:ietf:params:rtp-hdrext:sdes:mid", 1, false, {}});
	consumable.headerExtensions.push_back({"urn:ietf:params:rtp-hdrext:ssrc-audio-level", 10, false, {}});
	consumable.encodings.push_back(RtpEncodingParameters{5555u, "", std::nullopt, std::nullopt, false, "", std::nullopt});
	consumable.rtcp.cname = "cname";

	RtpCapabilities remote;
	remote.codecs.push_back({"video", "video/VP8", 96, 90000, 0, {}, {}});
	remote.headerExtensions.push_back({"video", "urn:ietf:params:rtp-hdrext:sdes:mid", 1, false, "sendrecv"});

	auto consumer = ortc::getConsumerRtpParameters(consumable, remote, false);
	EXPECT_EQ(consumer.codecs.size(), 1u);
	EXPECT_EQ(consumer.codecs[0].mimeType, "video/VP8");
	ASSERT_EQ(consumer.headerExtensions.size(), 1u);
	EXPECT_EQ(consumer.headerExtensions[0].uri, "urn:ietf:params:rtp-hdrext:sdes:mid");
	ASSERT_EQ(consumer.encodings.size(), 1u);
	EXPECT_TRUE(consumer.encodings[0].ssrc.has_value());
	EXPECT_FALSE(consumer.encodings[0].rtxSsrc.has_value());
	EXPECT_TRUE(consumer.rtcp.reducedSize);
}

TEST(OrtcTest, ConsumerRtpParametersAddsRtxWhenRemoteNegotiatesIt) {
	RtpParameters consumable;
	consumable.codecs.push_back({"video/VP8", 120, 90000, 0, {}, {}});
	consumable.codecs.push_back({"video/rtx", 121, 90000, 0, {{"apt", 120}}, {}});
	consumable.encodings.push_back(RtpEncodingParameters{5555u, "", std::nullopt, std::nullopt, false, "", std::nullopt});

	RtpCapabilities remote;
	remote.codecs.push_back({"video", "video/VP8", 96, 90000, 0, {}, {}});
	remote.codecs.push_back({"video", "video/rtx", 97, 90000, 0, {{"apt", 96}}, {}});

	auto consumer = ortc::getConsumerRtpParameters(consumable, remote, false);
	ASSERT_EQ(consumer.codecs.size(), 2u);
	EXPECT_EQ(consumer.codecs[0].mimeType, "video/VP8");
	EXPECT_EQ(consumer.codecs[1].mimeType, "video/rtx");
	ASSERT_EQ(consumer.encodings.size(), 1u);
	EXPECT_TRUE(consumer.encodings[0].rtxSsrc.has_value());
}

TEST(OrtcTest, ConsumerRtpParametersRejectsWhenRemoteHasNoMatchingCodec) {
	RtpParameters consumable;
	consumable.codecs.push_back({"audio/opus", 111, 48000, 2, {}, {}});
	consumable.encodings.push_back(RtpEncodingParameters{9999u, "", std::nullopt, std::nullopt, false, "", std::nullopt});

	RtpCapabilities remote;
	remote.codecs.push_back({"video", "video/VP8", 96, 90000, 0, {}, {}});

	EXPECT_THROW(ortc::getConsumerRtpParameters(consumable, remote, false), std::runtime_error);
}

TEST(OrtcTest, ConsumerRtpParametersRejectsH264ProfileMismatch) {
	RtpParameters consumable;
	consumable.codecs.push_back({"video/H264", 120, 90000, 0,
		{{"profile-level-id", "42e01f"}, {"packetization-mode", 1}}, {}});
	consumable.encodings.push_back(RtpEncodingParameters{9999u, "", std::nullopt, std::nullopt, false, "", std::nullopt});

	RtpCapabilities remote;
	remote.codecs.push_back({"video", "video/H264", 96, 90000, 0,
		{{"profile-level-id", "4d0032"}, {"packetization-mode", 1}}, {}});

	EXPECT_THROW(ortc::getConsumerRtpParameters(consumable, remote, false), std::runtime_error);
}

TEST(OrtcTest, ConsumerRtpParametersRejectsEmptyConsumableCodecs) {
	RtpParameters consumable;
	consumable.encodings.push_back(RtpEncodingParameters{123u, "", std::nullopt, std::nullopt, false, "", std::nullopt});
	RtpCapabilities remote;
	EXPECT_THROW(ortc::getConsumerRtpParameters(consumable, remote, false), std::runtime_error);
}

TEST(OrtcTest, PipeConsumerRtpParametersAssignsSsrcsPerEncoding) {
	RtpParameters consumable;
	consumable.codecs.push_back({"video/VP8", 120, 90000, 0, {}, {}});
	consumable.codecs.push_back({"video/rtx", 121, 90000, 0, {{"apt", 120}}, {}});
	consumable.encodings.push_back(RtpEncodingParameters{111u, "r0", std::nullopt, std::nullopt, false, "", std::nullopt});
	consumable.encodings.push_back(RtpEncodingParameters{222u, "r1", std::nullopt, std::nullopt, false, "", std::nullopt});

	auto pipeConsumer = ortc::getPipeConsumerRtpParameters(consumable);
	EXPECT_EQ(pipeConsumer.codecs.size(), consumable.codecs.size());
	ASSERT_EQ(pipeConsumer.encodings.size(), 2u);
	for (const auto& enc : pipeConsumer.encodings) {
		EXPECT_TRUE(enc.ssrc.has_value());
		EXPECT_TRUE(enc.rtxSsrc.has_value());
		EXPECT_GE(enc.ssrc.value_or(0), 100000000u);
		EXPECT_LE(enc.ssrc.value_or(0), 999999999u);
	}
}
