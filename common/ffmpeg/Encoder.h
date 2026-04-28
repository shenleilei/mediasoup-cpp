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
	bool SendFrame(const AVFrame* frame);
	bool ReceivePacket(AVPacket* packet);

private:
	CodecContextPtr context_;
};

} // namespace mediasoup::ffmpeg
