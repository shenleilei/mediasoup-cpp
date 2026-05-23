#include <gtest/gtest.h>

#include "push/RealtimeH264Source.h"

namespace {

webrtc_qos_plain::RealtimeH264SourceConfig SmallSourceConfig()
{
	webrtc_qos_plain::RealtimeH264SourceConfig config;
	config.width = 64;
	config.height = 48;
	config.fps = 15;
	config.bitrateBps = 500000;
	config.minBitrateBps = 120000;
	config.maxBitrateBps = 800000;
	return config;
}

} // namespace

TEST(WebRtcQosRealtimeSourceTest, ProducesAnnexBAccessUnits)
{
	std::string error;
	webrtc_qos_plain::RealtimeH264Source source(SmallSourceConfig());
	ASSERT_TRUE(source.Open(&error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit au;
	ASSERT_TRUE(source.NextAccessUnit(1000000, &au, &error)) << error;
	EXPECT_FALSE(au.bytes.empty());
	EXPECT_TRUE(au.keyframe);
	EXPECT_EQ(au.mediaTimeUs, 0);

	const auto& metrics = source.metrics();
	EXPECT_EQ(metrics.accessUnits, 1u);
	EXPECT_EQ(metrics.keyframes, 1u);
	EXPECT_EQ(metrics.currentBitrateBps, 500000u);
	EXPECT_EQ(metrics.currentFps, 15u);
}

TEST(WebRtcQosRealtimeSourceTest, AppliesEncoderAdaptation)
{
	std::string error;
	webrtc_qos_plain::RealtimeH264Source source(SmallSourceConfig());
	ASSERT_TRUE(source.Open(&error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit first;
	ASSERT_TRUE(source.NextAccessUnit(1000000, &first, &error)) << error;

	webrtc_qos::EncoderAdaptation adaptation;
	adaptation.target_bitrate_bps = 180000;
	adaptation.max_fps = 7;
	adaptation.request_keyframe = true;
	ASSERT_TRUE(source.ApplyEncoderAdaptation(adaptation, &error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit adapted;
	bool produced = false;
	for (int i = 0; i < 10; ++i) {
		if (source.NextAccessUnit(1000000 + 200000 + i * 20000, &adapted, &error)) {
			produced = true;
			break;
		}
	}
	ASSERT_TRUE(produced) << error;
	EXPECT_TRUE(adapted.keyframe);

	const auto& metrics = source.metrics();
	EXPECT_EQ(metrics.currentBitrateBps, 180000u);
	EXPECT_EQ(metrics.currentFps, 7u);
	EXPECT_GE(metrics.bitrateChanges, 1u);
	EXPECT_GE(metrics.fpsChanges, 1u);
	EXPECT_GE(metrics.forcedKeyframeRequests, 1u);
	EXPECT_GE(metrics.encoderRecreates, 2u);
}
