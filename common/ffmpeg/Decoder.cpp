#include "ffmpeg/Decoder.h"

#include "ffmpeg/AvError.h"

#include <stdexcept>

namespace mediasoup::ffmpeg {

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

void Decoder::SendPacket(const AVPacket* packet)
{
	CheckError(avcodec_send_packet(context_.get(), packet), "avcodec_send_packet");
}

bool Decoder::ReceiveFrame(AVFrame* frame)
{
	const int err = avcodec_receive_frame(context_.get(), frame);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) return false;
	CheckError(err, "avcodec_receive_frame");
	return false;
}

} // namespace mediasoup::ffmpeg
