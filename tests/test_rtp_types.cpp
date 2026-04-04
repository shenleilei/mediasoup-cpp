#include <gtest/gtest.h>
#include "RtpTypes.h"

using namespace mediasoup;
using json = nlohmann::json;

// --- RtpCodecCapability ---

TEST(RtpTypesTest, CodecCapabilityRoundTrip) {
	RtpCodecCapability orig{"audio", "audio/opus", 100, 48000, 2,
		{{"minptime", 10}}, {{"nack", ""}}};
	json j = orig;
	auto parsed = j.get<RtpCodecCapability>();
	EXPECT_EQ(parsed.mimeType, "audio/opus");
	EXPECT_EQ(parsed.clockRate, 48000u);
	EXPECT_EQ(parsed.channels, 2);
	EXPECT_EQ(parsed.preferredPayloadType, 100);
}

TEST(RtpTypesTest, CodecCapabilityOptionalFields) {
	json j = {{"mimeType", "video/VP8"}, {"clockRate", 90000}};
	auto c = j.get<RtpCodecCapability>();
	EXPECT_EQ(c.channels, 0);
	EXPECT_EQ(c.preferredPayloadType, 0);
	EXPECT_TRUE(c.parameters.empty());
}

// --- RtpCodecParameters ---

TEST(RtpTypesTest, CodecParametersRoundTrip) {
	RtpCodecParameters orig;
	orig.mimeType = "video/VP8";
	orig.payloadType = 96;
	orig.clockRate = 90000;
	json j = orig;
	auto parsed = j.get<RtpCodecParameters>();
	EXPECT_EQ(parsed.mimeType, "video/VP8");
	EXPECT_EQ(parsed.payloadType, 96);
	EXPECT_EQ(parsed.clockRate, 90000u);
}

// --- RtpEncodingParameters ---

TEST(RtpTypesTest, EncodingWithSsrc) {
	RtpEncodingParameters enc;
	enc.ssrc = 12345;
	enc.rtxSsrc = 67890;
	json j = enc;
	EXPECT_EQ(j["ssrc"], 12345);
	EXPECT_EQ(j["rtx"]["ssrc"], 67890);

	auto parsed = j.get<RtpEncodingParameters>();
	EXPECT_EQ(*parsed.ssrc, 12345u);
	EXPECT_EQ(*parsed.rtxSsrc, 67890u);
}

TEST(RtpTypesTest, EncodingWithRid) {
	json j = {{"rid", "r0"}, {"scalabilityMode", "L1T3"}, {"mappedSsrc", 0}};
	auto enc = j.get<RtpEncodingParameters>();
	EXPECT_EQ(enc.rid, "r0");
	EXPECT_EQ(enc.scalabilityMode, "L1T3");
	EXPECT_FALSE(enc.ssrc.has_value());
}

// --- RtpParameters ---

TEST(RtpTypesTest, RtpParametersRoundTrip) {
	RtpParameters params;
	params.mid = "0";
	params.codecs.push_back({"audio/opus", 111, 48000, 2, {}, {}});
	params.rtcp.cname = "test-cname";

	json j = params;
	auto parsed = j.get<RtpParameters>();
	EXPECT_EQ(parsed.mid, "0");
	EXPECT_EQ(parsed.codecs.size(), 1u);
	EXPECT_EQ(parsed.codecs[0].mimeType, "audio/opus");
	EXPECT_EQ(parsed.rtcp.cname, "test-cname");
}

// --- RtpCapabilities ---

TEST(RtpTypesTest, RtpCapabilitiesFromJson) {
	json j = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000},
			{"channels", 2}, {"preferredPayloadType", 100}
		}}},
		{"headerExtensions", {{
			{"uri", "urn:ietf:params:rtp-hdrext:ssrc-audio-level"},
			{"preferredId", 1}, {"kind", "audio"}
		}}}
	};
	auto caps = j.get<RtpCapabilities>();
	EXPECT_EQ(caps.codecs.size(), 1u);
	EXPECT_EQ(caps.headerExtensions.size(), 1u);
	EXPECT_EQ(caps.headerExtensions[0].uri, "urn:ietf:params:rtp-hdrext:ssrc-audio-level");
}

// --- RtpMapping ---

TEST(RtpTypesTest, RtpMappingRoundTrip) {
	RtpMapping mapping;
	mapping.codecs.push_back({111, 100});
	EncodingMapping em;
	em.ssrc = 1234;
	em.mappedSsrc = 5678;
	mapping.encodings.push_back(em);

	json j = mapping;
	auto parsed = j.get<RtpMapping>();
	EXPECT_EQ(parsed.codecs[0].payloadType, 111);
	EXPECT_EQ(parsed.codecs[0].mappedPayloadType, 100);
	EXPECT_EQ(*parsed.encodings[0].ssrc, 1234u);
	EXPECT_EQ(parsed.encodings[0].mappedSsrc, 5678u);
}
