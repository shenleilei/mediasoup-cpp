#include <gtest/gtest.h>

#include "ffmpeg/AvPtr.h"
#include "ffmpeg/BitstreamFilter.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/InputFormat.h"
#include "ffmpeg/OutputFormat.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

using namespace mediasoup;

namespace {

struct CodecParametersDeleter {
	void operator()(AVCodecParameters* parameters) const
	{
		if (parameters) {
			avcodec_parameters_free(&parameters);
		}
	}
};

bool HasMpeg4Encoder()
{
	return avcodec_find_encoder(AV_CODEC_ID_MPEG4) != nullptr;
}

ffmpeg::FramePtr MakeYuv420Frame(int width, int height, int64_t pts, uint8_t fill)
{
	auto frame = ffmpeg::MakeFrame();
	frame->format = AV_PIX_FMT_YUV420P;
	frame->width = width;
	frame->height = height;
	ffmpeg::FrameGetBuffer(frame.get(), 32);
	ffmpeg::FrameMakeWritable(frame.get());
	frame->pts = pts;

	for (int y = 0; y < height; ++y)
		std::memset(frame->data[0] + y * frame->linesize[0], fill, width);
	for (int y = 0; y < height / 2; ++y) {
		std::memset(frame->data[1] + y * frame->linesize[1], fill / 2, width / 2);
		std::memset(frame->data[2] + y * frame->linesize[2], fill / 3, width / 2);
	}

	return frame;
}

ffmpeg::Encoder MakeMpeg4Encoder()
{
	const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
	if (!codec) {
		throw std::runtime_error("mpeg4 encoder unavailable");
	}

	return ffmpeg::Encoder::Create(AV_CODEC_ID_MPEG4, [](AVCodecContext* context) {
		context->width = 16;
		context->height = 16;
		context->time_base = AVRational{1, 25};
		context->framerate = AVRational{25, 1};
		context->pix_fmt = AV_PIX_FMT_YUV420P;
	});
}

ffmpeg::PacketPtr ClonePacket(const AVPacket* source)
{
	auto packet = ffmpeg::MakePacket();
	if (!packet) {
		throw std::runtime_error("av_packet_alloc failed");
	}
	ffmpeg::PacketRef(packet.get(), source);
	return packet;
}

std::vector<ffmpeg::PacketPtr> EncodePackets(size_t count, AVCodecParameters* outputParameters)
{
	auto encoder = MakeMpeg4Encoder();
	std::vector<ffmpeg::PacketPtr> packets;
	packets.reserve(count);

	for (size_t i = 0; i < count; ++i) {
		auto frame = MakeYuv420Frame(16, 16, static_cast<int64_t>(i), static_cast<uint8_t>(10 + i));
		if (!encoder.SendFrame(frame.get())) {
			throw std::runtime_error("encoder send unexpectedly returned EAGAIN");
		}

		auto packet = ffmpeg::MakePacket();
		if (!packet) {
			throw std::runtime_error("av_packet_alloc failed");
		}
		if (!encoder.ReceivePacket(packet.get())) {
			throw std::runtime_error("encoder failed to produce packet");
		}
		packets.push_back(ClonePacket(packet.get()));
	}

	if (avcodec_parameters_from_context(outputParameters, encoder.get()) < 0) {
		throw std::runtime_error("avcodec_parameters_from_context failed");
	}
	return packets;
}

std::string ReadFile(const std::filesystem::path& path)
{
	std::ifstream input(path, std::ios::binary);
	return std::string(
		std::istreambuf_iterator<char>(input),
		std::istreambuf_iterator<char>());
}

} // namespace

TEST(OutputFormatSharedTest, CloseWritesTrailerAfterHeader)
{
	const auto tempDir = std::filesystem::temp_directory_path();
	const auto outputPath =
		tempDir / ("mediasoup-output-format-" + std::to_string(::getpid()) + ".mp4");
	std::filesystem::remove(outputPath);

	{
		auto output = ffmpeg::OutputFormat::Create("mp4", outputPath.string());
		auto* stream = output.NewStream();
		ASSERT_NE(stream, nullptr);
		stream->id = 0;
		stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		stream->codecpar->codec_id = AV_CODEC_ID_MPEG4;
		stream->codecpar->width = 16;
		stream->codecpar->height = 16;
		stream->time_base = {1, 25};

		output.OpenIo();
		output.WriteHeader();
		output.Close();
	}

	const auto bytes = ReadFile(outputPath);
	EXPECT_NE(bytes.find("moov"), std::string::npos);
	std::filesystem::remove(outputPath);
}

TEST(EncoderSharedTest, SendFrameReturnsFalseOnEagain)
{
	if (!HasMpeg4Encoder()) {
		GTEST_SKIP() << "mpeg4 encoder unavailable";
	}
	auto encoder = MakeMpeg4Encoder();
	auto firstFrame = MakeYuv420Frame(16, 16, 0, 10);
	auto secondFrame = MakeYuv420Frame(16, 16, 1, 20);

	ASSERT_TRUE(encoder.SendFrame(firstFrame.get()));
	EXPECT_FALSE(encoder.SendFrame(secondFrame.get()));

	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);
	EXPECT_TRUE(encoder.ReceivePacket(packet.get()));
}

TEST(DecoderSharedTest, SendPacketReturnsFalseOnEagain)
{
	if (!HasMpeg4Encoder()) {
		GTEST_SKIP() << "mpeg4 encoder unavailable";
	}
	auto parameters = std::unique_ptr<AVCodecParameters, CodecParametersDeleter>(
		avcodec_parameters_alloc(),
		CodecParametersDeleter{});
	ASSERT_NE(parameters, nullptr);

	const auto packets = EncodePackets(3, parameters.get());
	auto decoder = ffmpeg::Decoder::OpenFromParameters(parameters.get());

	ASSERT_TRUE(decoder.SendPacket(packets[0].get()));
	ASSERT_TRUE(decoder.SendPacket(packets[1].get()));
	EXPECT_FALSE(decoder.SendPacket(packets[2].get()));

	auto frame = ffmpeg::MakeFrame();
	ASSERT_NE(frame, nullptr);
	EXPECT_TRUE(decoder.ReceiveFrame(frame.get()));
}

TEST(BitstreamFilterSharedTest, SendPacketReturnsFalseOnEagain)
{
	auto params = std::unique_ptr<AVCodecParameters, CodecParametersDeleter>(
		avcodec_parameters_alloc(),
		CodecParametersDeleter{});
	ASSERT_NE(params, nullptr);
	params->codec_type = AVMEDIA_TYPE_VIDEO;
	params->codec_id = AV_CODEC_ID_H264;

	auto filter = ffmpeg::BitstreamFilter::Create("null", params.get(), AVRational{1, 90000});

	auto first = ffmpeg::MakePacket();
	auto second = ffmpeg::MakePacket();
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	ASSERT_EQ(av_new_packet(first.get(), 4), 0);
	ASSERT_EQ(av_new_packet(second.get(), 4), 0);

	ASSERT_TRUE(filter.SendPacket(first.get()));
	EXPECT_FALSE(filter.SendPacket(second.get()));

	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);
	EXPECT_TRUE(filter.ReceivePacket(packet.get()));
}

TEST(InputFormatSharedTest, EmptyQueriesKeepSafeSentinelBehavior)
{
	ffmpeg::InputFormat input;
	EXPECT_EQ(input.FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO), -1);
	EXPECT_EQ(input.StreamAt(0), nullptr);
}

TEST(InputFormatSharedTest, EmptyOperationalMethodsThrow)
{
	ffmpeg::InputFormat input;
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(input.FindStreamInfo(), std::runtime_error);
	EXPECT_THROW(input.ReadPacket(packet.get()), std::runtime_error);
}

TEST(InputFormatSharedTest, MovedFromOperationalMethodsThrow)
{
	ffmpeg::InputFormat original;
	ffmpeg::InputFormat moved(std::move(original));
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(original.FindStreamInfo(), std::runtime_error);
	EXPECT_THROW(original.ReadPacket(packet.get()), std::runtime_error);
	EXPECT_EQ(original.FindFirstStreamIndex(AVMEDIA_TYPE_AUDIO), -1);
	EXPECT_EQ(original.StreamAt(0), nullptr);
}

TEST(OutputFormatSharedTest, EmptyOperationalMethodsThrow)
{
	ffmpeg::OutputFormat output;
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(output.NewStream(), std::runtime_error);
	EXPECT_THROW(output.OpenIo(), std::runtime_error);
	EXPECT_THROW(output.WriteHeader(), std::runtime_error);
	EXPECT_THROW(output.WriteInterleavedFrame(packet.get()), std::runtime_error);
}

TEST(OutputFormatSharedTest, MovedFromOperationalMethodsThrow)
{
	ffmpeg::OutputFormat original;
	ffmpeg::OutputFormat moved(std::move(original));
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(original.NewStream(), std::runtime_error);
	EXPECT_THROW(original.OpenIo(), std::runtime_error);
	EXPECT_THROW(original.WriteHeader(), std::runtime_error);
	EXPECT_THROW(original.WriteInterleavedFrame(packet.get()), std::runtime_error);
}

TEST(DecoderSharedTest, EmptyMethodsThrow)
{
	ffmpeg::Decoder decoder;
	auto packet = ffmpeg::MakePacket();
	auto frame = ffmpeg::MakeFrame();
	ASSERT_NE(packet, nullptr);
	ASSERT_NE(frame, nullptr);

	EXPECT_THROW(decoder.SendPacket(packet.get()), std::runtime_error);
	EXPECT_THROW(decoder.ReceiveFrame(frame.get()), std::runtime_error);
}

TEST(DecoderSharedTest, MovedFromMethodsThrow)
{
	if (!HasMpeg4Encoder()) {
		GTEST_SKIP() << "mpeg4 encoder unavailable";
	}
	auto parameters = std::unique_ptr<AVCodecParameters, CodecParametersDeleter>(
		avcodec_parameters_alloc(),
		CodecParametersDeleter{});
	ASSERT_NE(parameters, nullptr);

	const auto packets = EncodePackets(1, parameters.get());
	ffmpeg::Decoder original = ffmpeg::Decoder::OpenFromParameters(parameters.get());
	ffmpeg::Decoder moved(std::move(original));
	EXPECT_NE(moved.get(), nullptr);

	auto frame = ffmpeg::MakeFrame();
	ASSERT_NE(frame, nullptr);

	EXPECT_THROW(original.SendPacket(packets[0].get()), std::runtime_error);
	EXPECT_THROW(original.ReceiveFrame(frame.get()), std::runtime_error);
}

TEST(EncoderSharedTest, EmptyMethodsThrow)
{
	ffmpeg::Encoder encoder;
	auto frame = ffmpeg::MakeFrame();
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(frame, nullptr);
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(encoder.SendFrame(frame.get()), std::runtime_error);
	EXPECT_THROW(encoder.ReceivePacket(packet.get()), std::runtime_error);
}

TEST(EncoderSharedTest, MovedFromMethodsThrow)
{
	if (!HasMpeg4Encoder()) {
		GTEST_SKIP() << "mpeg4 encoder unavailable";
	}
	ffmpeg::Encoder original = MakeMpeg4Encoder();
	ffmpeg::Encoder moved(std::move(original));
	EXPECT_NE(moved.get(), nullptr);

	auto frame = MakeYuv420Frame(16, 16, 0, 10);
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(original.SendFrame(frame.get()), std::runtime_error);
	EXPECT_THROW(original.ReceivePacket(packet.get()), std::runtime_error);
}

TEST(BitstreamFilterSharedTest, EmptyMethodsThrow)
{
	ffmpeg::BitstreamFilter filter;
	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(filter.SendPacket(packet.get()), std::runtime_error);
	EXPECT_THROW(filter.ReceivePacket(packet.get()), std::runtime_error);
}

TEST(BitstreamFilterSharedTest, MovedFromMethodsThrow)
{
	auto params = std::unique_ptr<AVCodecParameters, CodecParametersDeleter>(
		avcodec_parameters_alloc(),
		CodecParametersDeleter{});
	ASSERT_NE(params, nullptr);
	params->codec_type = AVMEDIA_TYPE_VIDEO;
	params->codec_id = AV_CODEC_ID_H264;

	ffmpeg::BitstreamFilter original =
		ffmpeg::BitstreamFilter::Create("null", params.get(), AVRational{1, 90000});
	ffmpeg::BitstreamFilter moved(std::move(original));
	EXPECT_NE(moved.get(), nullptr);

	auto packet = ffmpeg::MakePacket();
	ASSERT_NE(packet, nullptr);

	EXPECT_THROW(original.SendPacket(packet.get()), std::runtime_error);
	EXPECT_THROW(original.ReceivePacket(packet.get()), std::runtime_error);
}
