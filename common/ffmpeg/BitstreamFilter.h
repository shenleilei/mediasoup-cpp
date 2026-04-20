#pragma once

#include "ffmpeg/AvPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <string>

namespace mediasoup::ffmpeg {

class BitstreamFilter {
public:
	static BitstreamFilter Create(
		const std::string& name,
		const AVCodecParameters* inputParameters,
		AVRational inputTimeBase);

	BitstreamFilter() = default;
	explicit BitstreamFilter(BitstreamFilterContextPtr context);

	AVBSFContext* get() const { return context_.get(); }
	void SendPacket(AVPacket* packet);
	bool ReceivePacket(AVPacket* packet);

private:
	BitstreamFilterContextPtr context_;
};

} // namespace mediasoup::ffmpeg
