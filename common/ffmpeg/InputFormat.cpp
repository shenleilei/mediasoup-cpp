#include "ffmpeg/InputFormat.h"

#include "ffmpeg/AvError.h"

namespace mediasoup::ffmpeg {

InputFormat::InputFormat(AVFormatContext* ctx)
	: ctx_(ctx)
{
}

InputFormat InputFormat::Open(const std::string& path)
{
	AVFormatContext* ctx = nullptr;
	const int err = avformat_open_input(&ctx, path.c_str(), nullptr, nullptr);
	CheckError(err, "avformat_open_input(" + path + ")");
	return InputFormat(ctx);
}

InputFormat::~InputFormat()
{
	Close();
}

InputFormat::InputFormat(InputFormat&& other) noexcept
	: ctx_(other.ctx_)
{
	other.ctx_ = nullptr;
}

InputFormat& InputFormat::operator=(InputFormat&& other) noexcept
{
	if (this == &other) return *this;
	Close();
	ctx_ = other.ctx_;
	other.ctx_ = nullptr;
	return *this;
}

void InputFormat::FindStreamInfo()
{
	CheckError(avformat_find_stream_info(ctx_, nullptr), "avformat_find_stream_info");
}

int InputFormat::FindFirstStreamIndex(AVMediaType mediaType) const
{
	if (!ctx_) return -1;
	for (unsigned int i = 0; i < ctx_->nb_streams; ++i) {
		if (ctx_->streams[i]->codecpar->codec_type == mediaType)
			return static_cast<int>(i);
	}
	return -1;
}

AVStream* InputFormat::StreamAt(int index) const
{
	if (!ctx_ || index < 0 || static_cast<unsigned int>(index) >= ctx_->nb_streams) return nullptr;
	return ctx_->streams[index];
}

bool InputFormat::ReadPacket(AVPacket* packet)
{
	const int err = av_read_frame(ctx_, packet);
	if (err >= 0) return true;
	if (err == AVERROR_EOF) return false;
	CheckError(err, "av_read_frame");
	return false;
}

void InputFormat::Close()
{
	if (!ctx_) return;
	avformat_close_input(&ctx_);
}

} // namespace mediasoup::ffmpeg
