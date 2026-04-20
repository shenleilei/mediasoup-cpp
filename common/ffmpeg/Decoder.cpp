#include "ffmpeg/Decoder.h"

#include "ffmpeg/AvError.h"

#include <stdexcept>

namespace mediasoup::ffmpeg {
namespace {

AVCodecContext* RequireDecoderContext(AVCodecContext* context, const char* method)
{
	if (context) return context;
	throw std::runtime_error(std::string("Decoder::") + method + " on empty decoder");
}

} // namespace

Decoder::Decoder(CodecContextPtr context)
	: context_(std::move(context))
{
}

Decoder Decoder::OpenFromParameters(const AVCodecParameters* parameters)
{
	if (!parameters)
		throw std::runtime_error("Decoder::OpenFromParameters missing codec parameters");

	const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
	if (!codec)
		throw std::runtime_error("avcodec_find_decoder failed");

	auto context = MakeCodecContext(codec);
	if (!context)
		throw std::runtime_error("avcodec_alloc_context3 failed");

	CheckError(avcodec_parameters_to_context(context.get(), parameters),
		"avcodec_parameters_to_context");
	CheckError(avcodec_open2(context.get(), codec, nullptr), "avcodec_open2(decoder)");
	return Decoder(std::move(context));
}

bool Decoder::SendPacket(const AVPacket* packet)
{
	const int err = avcodec_send_packet(
		RequireDecoderContext(context_.get(), "SendPacket"),
		packet);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN)) return false;
	CheckError(err, "avcodec_send_packet");
	return false;
}

bool Decoder::ReceiveFrame(AVFrame* frame)
{
	const int err = avcodec_receive_frame(
		RequireDecoderContext(context_.get(), "ReceiveFrame"),
		frame);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) return false;
	CheckError(err, "avcodec_receive_frame");
	return false;
}

} // namespace mediasoup::ffmpeg
