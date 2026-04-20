#include "ffmpeg/BitstreamFilter.h"

#include "ffmpeg/AvError.h"

#include <stdexcept>

namespace mediasoup::ffmpeg {
namespace {

AVBSFContext* RequireBitstreamFilterContext(AVBSFContext* context, const char* method)
{
	if (context) return context;
	throw std::runtime_error(std::string("BitstreamFilter::") + method + " on empty filter");
}

} // namespace

BitstreamFilter::BitstreamFilter(BitstreamFilterContextPtr context)
	: context_(std::move(context))
{
}

BitstreamFilter BitstreamFilter::Create(
	const std::string& name,
	const AVCodecParameters* inputParameters,
	AVRational inputTimeBase)
{
	const AVBitStreamFilter* filter = av_bsf_get_by_name(name.c_str());
	if (!filter)
		throw std::runtime_error("av_bsf_get_by_name(" + name + ") failed");

	auto context = MakeBitstreamFilterContext(filter);
	if (!context)
		throw std::runtime_error("av_bsf_alloc(" + name + ") failed");

	CheckError(avcodec_parameters_copy(context->par_in, inputParameters),
		"avcodec_parameters_copy");
	context->time_base_in = inputTimeBase;
	CheckError(av_bsf_init(context.get()), "av_bsf_init(" + name + ")");
	return BitstreamFilter(std::move(context));
}

bool BitstreamFilter::SendPacket(AVPacket* packet)
{
	const int err = av_bsf_send_packet(
		RequireBitstreamFilterContext(context_.get(), "SendPacket"),
		packet);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN)) return false;
	CheckError(err, "av_bsf_send_packet");
	return false;
}

bool BitstreamFilter::ReceivePacket(AVPacket* packet)
{
	const int err = av_bsf_receive_packet(
		RequireBitstreamFilterContext(context_.get(), "ReceivePacket"),
		packet);
	if (err >= 0) return true;
	if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) return false;
	CheckError(err, "av_bsf_receive_packet");
	return false;
}

} // namespace mediasoup::ffmpeg
