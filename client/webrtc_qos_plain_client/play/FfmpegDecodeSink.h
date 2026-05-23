#pragma once

#include <cstdint>
#include <string>

#include "ffmpeg/AvPtr.h"
#include "webrtc_qos/session_config.h"

namespace webrtc_qos_plain {

struct FfmpegDecodeSinkMetrics {
	bool enabled{false};
	uint64_t accessUnitsIn{0};
	uint64_t keyframesIn{0};
	uint64_t decodedFrames{0};
	uint64_t decodeErrors{0};
	uint64_t freezeCount{0};
	int64_t firstFrameDelayUs{-1};
	int64_t maxFrameGapUs{0};
	double outputFps{0.0};
	int width{0};
	int height{0};
};

class FfmpegDecodeSink {
public:
	bool Open(std::string* error);
	bool Decode(const webrtc_qos::AnnexBAccessUnitView& accessUnit, int64_t nowUs, std::string* error);

	const FfmpegDecodeSinkMetrics& metrics() const { return metrics_; }

private:
	bool DrainFrames(int64_t nowUs, std::string* error);
	void RecordDecodedFrame(int64_t nowUs);
	void RecordDecodeError(const std::string& message, std::string* error);

	mediasoup::ffmpeg::CodecContextPtr context_;
	mediasoup::ffmpeg::FramePtr frame_;
	FfmpegDecodeSinkMetrics metrics_;
	int64_t startUs_{0};
	int64_t firstDecodedFrameUs_{0};
	int64_t lastDecodedFrameUs_{0};
	int64_t freezeThresholdUs_{500000};
};

} // namespace webrtc_qos_plain
