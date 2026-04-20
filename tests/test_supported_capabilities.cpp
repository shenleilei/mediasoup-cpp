#include <gtest/gtest.h>
#include "RtpParametersFbs.h"
#include "supportedRtpCapabilities.h"
#include <flatbuffers/flatbuffers.h>
#include <set>

using namespace mediasoup;

TEST(SupportedCapabilitiesTest, HasCodecs) {
	auto caps = getSupportedRtpCapabilities();
	EXPECT_GT(caps.codecs.size(), 0u);

	// Should have opus
	bool hasOpus = false;
	for (auto& c : caps.codecs) {
		if (c.mimeType == "audio/opus") { hasOpus = true; break; }
	}
	EXPECT_TRUE(hasOpus);
}

TEST(SupportedCapabilitiesTest, HasVideoCodecs) {
	auto caps = getSupportedRtpCapabilities();
	bool hasVP8 = false, hasH264 = false;
	for (auto& c : caps.codecs) {
		if (c.mimeType == "video/VP8") hasVP8 = true;
		if (c.mimeType == "video/H264") hasH264 = true;
	}
	EXPECT_TRUE(hasVP8);
	EXPECT_TRUE(hasH264);
}

TEST(SupportedCapabilitiesTest, HasHeaderExtensions) {
	auto caps = getSupportedRtpCapabilities();
	EXPECT_GT(caps.headerExtensions.size(), 0u);
}

TEST(SupportedCapabilitiesTest, CodecsHaveValidFields) {
	auto caps = getSupportedRtpCapabilities();
	for (auto& c : caps.codecs) {
		EXPECT_FALSE(c.mimeType.empty());
		EXPECT_GT(c.clockRate, 0u);
		EXPECT_FALSE(c.kind.empty());
	}
}

TEST(SupportedCapabilitiesTest, NonZeroPayloadTypesAreUnique) {
	auto caps = getSupportedRtpCapabilities();
	std::set<uint32_t> payloadTypes;

	for (const auto& codec : caps.codecs) {
		if (codec.preferredPayloadType == 0) {
			continue;
		}
		EXPECT_TRUE(payloadTypes.insert(codec.preferredPayloadType).second)
			<< "duplicate payload type " << codec.preferredPayloadType
			<< " for " << codec.mimeType;
	}
}

TEST(SupportedCapabilitiesTest, RepairRtpStreamIdMapsToRepairUri) {
	RtpParameters params;
	params.headerExtensions.push_back({
		"urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id",
		3,
		false,
		json::object()
	});

	flatbuffers::FlatBufferBuilder builder;
	builder.Finish(BuildFbsRtpParameters(builder, params));

	auto* fbsParams =
		flatbuffers::GetRoot<FBS::RtpParameters::RtpParameters>(builder.GetBufferPointer());
	ASSERT_NE(fbsParams, nullptr);
	ASSERT_NE(fbsParams->header_extensions(), nullptr);
	ASSERT_EQ(fbsParams->header_extensions()->size(), 1u);
	EXPECT_EQ(
		fbsParams->header_extensions()->Get(0)->uri(),
		FBS::RtpParameters::RtpHeaderExtensionUri::RepairRtpStreamId);
}
