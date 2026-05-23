#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "ffmpeg/BitstreamFilter.h"
#include "ffmpeg/InputFormat.h"

namespace webrtc_qos_plain {

struct AnnexBAccessUnit {
	std::vector<uint8_t> bytes;
	int64_t mediaTimeUs = 0;
	bool keyframe = false;
};

class H264AnnexBSource {
public:
	H264AnnexBSource(std::string path, bool loopInput);

	bool Open(std::string* error);
	bool NextAccessUnit(AnnexBAccessUnit* out, std::string* error);

private:
	bool OpenInternal(std::string* error);
	bool ReopenForLoop(std::string* error);
	int64_t PacketTimeUs(const AVPacket* packet);

	std::string path_;
	bool loopInput_{false};
	std::optional<mediasoup::ffmpeg::InputFormat> input_;
	std::optional<mediasoup::ffmpeg::BitstreamFilter> bsf_;
	int videoIndex_{-1};
	AVRational timeBase_{1, 90000};
	int64_t firstPacketTimeUs_{AV_NOPTS_VALUE};
	int64_t loopOffsetUs_{0};
	int64_t lastOutputTimeUs_{0};
	int64_t fallbackFrameIndex_{0};
	bool draining_{false};
};

} // namespace webrtc_qos_plain
