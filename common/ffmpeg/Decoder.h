#pragma once

#include "ffmpeg/AvPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace mediasoup::ffmpeg {

class Decoder {
public:
#ifdef MEDIASOUP_TEST_HOOKS
	using OpenFailureHook = int (*)(AVCodecContext* context, const AVCodec* codec);
#endif

	static Decoder OpenFromParameters(const AVCodecParameters* parameters);

	Decoder() = default;
	explicit Decoder(CodecContextPtr context);

#ifdef MEDIASOUP_TEST_HOOKS
	static OpenFailureHook SetOpenFailureHook(OpenFailureHook hook);
#endif

	AVCodecContext* get() const { return context_.get(); }
	bool SendPacket(const AVPacket* packet);
	bool ReceiveFrame(AVFrame* frame);

private:
	CodecContextPtr context_;
};

} // namespace mediasoup::ffmpeg
