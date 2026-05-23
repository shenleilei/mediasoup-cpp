#include <gtest/gtest.h>

#include "play/FfmpegDecodeSink.h"
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

TEST(WebRtcQosDecodeSinkTest, DecodesSyntheticH264AccessUnits)
{
	std::string error;
	webrtc_qos_plain::RealtimeH264Source source(SmallSourceConfig());
	ASSERT_TRUE(source.Open(&error)) << error;

	webrtc_qos_plain::FfmpegDecodeSink decoder;
	ASSERT_TRUE(decoder.Open(&error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit au;
	uint64_t decodedInputs = 0;
	for (int i = 0; i < 20; ++i) {
		const int64_t nowUs = 1000000 + i * 70000;
		if (!source.NextAccessUnit(nowUs, &au, &error)) continue;
		webrtc_qos::AnnexBAccessUnitView view;
		view.bytes = au.bytes.data();
		view.size = au.bytes.size();
		view.capture_time_us = au.mediaTimeUs;
		view.keyframe = au.keyframe;
		ASSERT_TRUE(decoder.Decode(view, nowUs, &error)) << error;
		++decodedInputs;
		if (decoder.metrics().decodedFrames > 0) break;
	}

	EXPECT_GT(decodedInputs, 0u);
	const auto& metrics = decoder.metrics();
	EXPECT_TRUE(metrics.enabled);
	EXPECT_GT(metrics.accessUnitsIn, 0u);
	EXPECT_GT(metrics.keyframesIn, 0u);
	EXPECT_GT(metrics.decodedFrames, 0u);
	EXPECT_EQ(metrics.decodeErrors, 0u);
	EXPECT_EQ(metrics.width, 64);
	EXPECT_EQ(metrics.height, 48);
	EXPECT_GE(metrics.firstFrameDelayUs, 0);
}
