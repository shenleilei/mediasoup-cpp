#include <gtest/gtest.h>
#include "supportedRtpCapabilities.h"

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
