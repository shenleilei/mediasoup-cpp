#include "ffmpeg/InputFormat.h"

#include "ffmpeg/AvError.h"

#include <atomic>
#include <stdexcept>
#include <utility>

namespace mediasoup::ffmpeg {
namespace {

AVFormatContext* RequireInputContext(AVFormatContext* ctx, const char* method)
{
	if (ctx) return ctx;
	throw std::runtime_error(std::string("InputFormat::") + method + " on empty format");
}

int InterruptCallback(void* opaque)
{
	auto* callback = static_cast<InputInterruptCallback*>(opaque);
	return callback && *callback && (*callback)() ? 1 : 0;
}

	#ifdef MEDIASOUP_TEST_HOOKS
	std::atomic<int> gReadFailureCountdown{-1};
	#endif

} // namespace

InputFormat::InputFormat(AVFormatContext* ctx)
	: ctx_(ctx)
{
}

InputFormat::InputFormat(AVFormatContext* ctx, InputInterruptCallback interrupt)
	: ctx_(ctx), interrupt_(std::move(interrupt))
{
	if (ctx_ && interrupt_) {
		ctx_->interrupt_callback.callback = InterruptCallback;
		ctx_->interrupt_callback.opaque = &interrupt_;
	}
}

InputFormat InputFormat::Open(const std::string& path)
{
	AVFormatContext* ctx = nullptr;
	const int err = avformat_open_input(&ctx, path.c_str(), nullptr, nullptr);
	CheckError(err, "avformat_open_input(" + path + ")");
	return InputFormat(ctx);
}

InputFormat InputFormat::OpenWithFormat(const std::string& path,
	const AVInputFormat* fmt, AVDictionary** opts)
{
	AVFormatContext* ctx = nullptr;
	const int err = avformat_open_input(
		&ctx,
		path.c_str(),
		const_cast<AVInputFormat*>(fmt),
		opts);
	CheckError(err, "avformat_open_input(" + path + ")");
	return InputFormat(ctx);
}

InputFormat InputFormat::OpenWithFormatInterruptible(const std::string& path,
	const AVInputFormat* fmt, AVDictionary** opts, InputInterruptCallback interrupt)
{
	AVFormatContext* ctx = avformat_alloc_context();
	if (!ctx) {
		CheckError(AVERROR(ENOMEM), "avformat_alloc_context");
	}
	InputInterruptCallback callback = std::move(interrupt);
	if (callback) {
		ctx->interrupt_callback.callback = InterruptCallback;
		ctx->interrupt_callback.opaque = &callback;
	}
	AVFormatContext* openCtx = ctx;
	const int err = avformat_open_input(
		&openCtx,
		path.c_str(),
		const_cast<AVInputFormat*>(fmt),
		opts);
	if (err < 0) {
		if (openCtx) avformat_close_input(&openCtx);
		CheckError(err, "avformat_open_input(" + path + ")");
	}
	return InputFormat(openCtx, std::move(callback));
}

	#ifdef MEDIASOUP_TEST_HOOKS
int InputFormat::SetReadFailureCountdown(int readsBeforeFailure)
{
	return gReadFailureCountdown.exchange(readsBeforeFailure, std::memory_order_acq_rel);
}

InputFormat InputFormat::CreateForTesting(AVFormatContext* ctx, InputInterruptCallback interrupt)
{
	return InputFormat(ctx, std::move(interrupt));
}
	#endif

InputFormat::~InputFormat()
{
	Close();
}

InputFormat::InputFormat(InputFormat&& other) noexcept
	: ctx_(other.ctx_), interrupt_(std::move(other.interrupt_))
{
	if (ctx_ && interrupt_) ctx_->interrupt_callback.opaque = &interrupt_;
	other.ctx_ = nullptr;
}

InputFormat& InputFormat::operator=(InputFormat&& other) noexcept
{
	if (this == &other) return *this;
	Close();
	ctx_ = other.ctx_;
	interrupt_ = std::move(other.interrupt_);
	if (ctx_ && interrupt_) ctx_->interrupt_callback.opaque = &interrupt_;
	other.ctx_ = nullptr;
	return *this;
}

void InputFormat::Close()
{
	if (!ctx_) return;
	ctx_->interrupt_callback.callback = nullptr;
	ctx_->interrupt_callback.opaque = nullptr;
	avformat_close_input(&ctx_);
}

void InputFormat::FindStreamInfo()
{
	CheckError(
		avformat_find_stream_info(RequireInputContext(ctx_, "FindStreamInfo"), nullptr),
		"avformat_find_stream_info");
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
#ifdef MEDIASOUP_TEST_HOOKS
	int countdown = gReadFailureCountdown.load(std::memory_order_acquire);
	while (countdown >= 0) {
		if (countdown == 0) {
			CheckError(AVERROR(EIO), "av_read_frame");
		}
		if (gReadFailureCountdown.compare_exchange_weak(
				countdown,
				countdown - 1,
				std::memory_order_acq_rel,
				std::memory_order_acquire)) {
			break;
		}
	}
#endif

	const int err = av_read_frame(RequireInputContext(ctx_, "ReadPacket"), packet);
	if (err >= 0) return true;
	if (err == AVERROR_EOF) return false;
	CheckError(err, "av_read_frame");
	return false;
}

	} // namespace mediasoup::ffmpeg
