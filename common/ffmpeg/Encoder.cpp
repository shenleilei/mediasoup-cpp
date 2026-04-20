#include "ffmpeg/Encoder.h"

#include "ffmpeg/AvError.h"

#include <stdexcept>

namespace mediasoup::ffmpeg {
namespace {

AVCodecContext* RequireEncoderContext(AVCodecContext* context, const char* method)
{
	if (context) return context;
	throw std::runtime_error(std::string("Encoder::") + method + " on empty encoder");
}

} // namespace

Encoder::Encoder(CodecContextPtr context)
	: context_(std::move(context))
{
}

Encoder Encoder::Create(AVCodecID codecId, ConfigureFn configure)
{
	const AVCodec* codec = avcodec_find_encoder(codecId);
	if (!codec)
		throw std::runtime_error("avcodec_find_encoder failed");

	auto context = MakeCodecContext(codec);
	if (!context)
		throw std::runtime_error("avcodec_alloc_context3 failed");

	if (configure)
		configure(context.get());

	CheckError(avcodec_open2(context.get(), codec, nullptr), "avcodec_open2(encoder)");
	return Encoder(std::move(context));
}

bool Encoder::SendFrame(const AVFrame* frame)
{
	const int err = avcodec_send_frame(
		RequireEncoderContext(context_.get(), "SendFrame"),
		frame);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN)) return false;
	CheckError(err, "avcodec_send_frame");
	return false;
}

bool Encoder::ReceivePacket(AVPacket* packet)
{
	const int err = avcodec_receive_packet(
		RequireEncoderContext(context_.get(), "ReceivePacket"),
		packet);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) return false;
	CheckError(err, "avcodec_receive_packet");
	return false;
}

} // namespace mediasoup::ffmpeg
