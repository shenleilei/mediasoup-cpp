#pragma once

#include "ffmpeg/AvPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace mediasoup::ffmpeg {

class Decoder {
public:
	static Decoder OpenFromParameters(const AVCodecParameters* parameters);

	Decoder() = default;
	explicit Decoder(CodecContextPtr context);

	AVCodecContext* get() const { return context_.get(); }
	bool SendPacket(const AVPacket* packet);
	bool ReceiveFrame(AVFrame* frame);

private:
	CodecContextPtr context_;
};

} // namespace mediasoup::ffmpeg
