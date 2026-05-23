#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <unistd.h>

#include "ffmpeg/AvError.h"
#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/OutputFormat.h"
#include "push/Mp4DecodeH264Source.h"
#include "push/RealtimeH264Source.h"
#include "push/V4L2H264Source.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

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

mediasoup::ffmpeg::FramePtr MakeFrame(int width, int height, int64_t pts)
{
	auto frame = mediasoup::ffmpeg::MakeFrame();
	frame->format = AV_PIX_FMT_YUV420P;
	frame->width = width;
	frame->height = height;
	mediasoup::ffmpeg::FrameGetBuffer(frame.get(), 32);
	mediasoup::ffmpeg::FrameMakeWritable(frame.get());
	frame->pts = pts;
	for (int y = 0; y < height; ++y) {
		std::memset(frame->data[0] + y * frame->linesize[0], static_cast<int>(16 + pts * 3), width);
	}
	for (int y = 0; y < height / 2; ++y) {
		std::memset(frame->data[1] + y * frame->linesize[1], 96, width / 2);
		std::memset(frame->data[2] + y * frame->linesize[2], 128, width / 2);
	}
	return frame;
}

std::filesystem::path WriteH264Mp4Fixture()
{
	const auto path = std::filesystem::temp_directory_path() /
		("webrtc-qos-mp4-decode-loop-" + std::to_string(::getpid()) + ".mp4");
	std::filesystem::remove(path);
	auto encoder = mediasoup::ffmpeg::Encoder::Create(AV_CODEC_ID_H264, [](AVCodecContext* ctx) {
		ctx->width = 64;
		ctx->height = 48;
		ctx->pix_fmt = AV_PIX_FMT_YUV420P;
		ctx->time_base = AVRational{1, 15};
		ctx->framerate = AVRational{15, 1};
		ctx->bit_rate = 300000;
		ctx->rc_max_rate = 300000;
		ctx->rc_buffer_size = 300000;
		ctx->gop_size = 15;
		ctx->max_b_frames = 0;
		av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
		av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
		av_opt_set(ctx->priv_data, "profile", "baseline", 0);
	});
	auto output = mediasoup::ffmpeg::OutputFormat::Create("mp4", path.string());
	auto* stream = output.NewStream();
	stream->time_base = AVRational{1, 15};
	mediasoup::ffmpeg::CheckError(
		avcodec_parameters_from_context(stream->codecpar, encoder.get()),
		"avcodec_parameters_from_context");
	output.OpenIo();
	output.WriteHeader();
	for (int i = 0; i < 20; ++i) {
		auto frame = MakeFrame(64, 48, i);
		if (!encoder.SendFrame(frame.get())) throw std::runtime_error("encoder SendFrame returned EAGAIN");
		auto packet = mediasoup::ffmpeg::MakePacket();
		if (!packet) throw std::runtime_error("av_packet_alloc failed");
		if (!encoder.ReceivePacket(packet.get())) throw std::runtime_error("encoder ReceivePacket returned EAGAIN");
		packet->stream_index = stream->index;
		av_packet_rescale_ts(packet.get(), encoder.get()->time_base, stream->time_base);
		output.WriteInterleavedFrame(packet.get());
	}
	output.Close();
	return path;
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

TEST(WebRtcQosRealtimeSourceTest, Mp4DecodeLoopProducesAdaptedAnnexBAccessUnits)
{
	std::string error;
	const auto input = WriteH264Mp4Fixture();
	webrtc_qos_plain::Mp4DecodeH264SourceConfig config;
	config.path = input.string();
	config.loopInput = true;
	config.bitrateBps = 500000;
	config.minBitrateBps = 120000;
	config.maxBitrateBps = 800000;
	webrtc_qos_plain::Mp4DecodeH264Source source(config);
	ASSERT_TRUE(source.Open(&error)) << error;

	webrtc_qos::EncoderAdaptation adaptation;
	adaptation.target_bitrate_bps = 180000;
	adaptation.max_fps = 7;
	adaptation.request_keyframe = true;
	const int64_t requestUs = 1000000;
	ASSERT_TRUE(source.ApplyEncoderAdaptation(adaptation, requestUs, &error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit au;
	bool produced = false;
	for (int i = 0; i < 20; ++i) {
		if (source.NextAccessUnit(requestUs + i * 20000, &au, &error)) {
			produced = true;
			break;
		}
	}
	ASSERT_TRUE(produced) << error;
	EXPECT_FALSE(au.bytes.empty());
	EXPECT_TRUE(au.keyframe);

	const auto& metrics = source.metrics();
	EXPECT_EQ(metrics.currentBitrateBps, 180000u);
	EXPECT_EQ(metrics.currentFps, 7u);
	EXPECT_GE(metrics.accessUnits, 1u);
	EXPECT_GE(metrics.keyframes, 1u);
	EXPECT_GE(metrics.forcedKeyframeRequests, 1u);
	EXPECT_GE(metrics.forcedKeyframes, 1u);
	EXPECT_GE(metrics.maxForcedKeyframeDelayUs, 0);
	EXPECT_LE(metrics.maxForcedKeyframeDelayUs, 1000000);
	std::filesystem::remove(input);
}

TEST(WebRtcQosRealtimeSourceTest, V4L2MissingDeviceReportsOpenFailure)
{
	std::string error;
	webrtc_qos_plain::V4L2H264SourceConfig config;
	config.device = "/dev/mediasoup-cpp-missing-video-device";
	config.width = 64;
	config.height = 48;
	config.fps = 15;
	config.bitrateBps = 500000;
	config.minBitrateBps = 120000;
	config.maxBitrateBps = 800000;
	webrtc_qos_plain::V4L2H264Source source(config);
	EXPECT_FALSE(source.Open(&error));
	EXPECT_FALSE(error.empty());
	EXPECT_EQ(source.metrics().accessUnits, 0u);
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
	const int64_t requestUs = 1200000;
	ASSERT_TRUE(source.ApplyEncoderAdaptation(adaptation, requestUs, &error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit adapted;
	bool produced = false;
	for (int i = 0; i < 10; ++i) {
		if (source.NextAccessUnit(requestUs + i * 20000, &adapted, &error)) {
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
	EXPECT_GE(metrics.forcedKeyframes, 1u);
	EXPECT_GE(metrics.maxForcedKeyframeDelayUs, 0);
	EXPECT_LE(metrics.maxForcedKeyframeDelayUs, 1000000);
	EXPECT_GE(metrics.encoderRecreates, 2u);
}
