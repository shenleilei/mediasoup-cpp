#pragma once

#include "ffmpeg/AvPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <functional>

namespace mediasoup::ffmpeg {

class Encoder {
public:
	using ConfigureFn = std::function<void(AVCodecContext*)>;

	static Encoder Create(AVCodecID codecId, ConfigureFn configure);

	Encoder() = default;
	explicit Encoder(CodecContextPtr context);

	AVCodecContext* get() const { return context_.get(); }
	explicit operator bool() const { return context_ != nullptr; }

	int width() const { return context_ ? context_->width : 0; }
	int height() const { return context_ ? context_->height : 0; }
	int64_t bitRate() const { return context_ ? context_->bit_rate : 0; }
	void setBitRate(int64_t br);

	bool SendFrame(const AVFrame* frame);
	bool ReceivePacket(AVPacket* packet);

private:
	CodecContextPtr context_;
};

} // namespace mediasoup::ffmpeg
