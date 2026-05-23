#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Encoder.h"
#include "push/H264AnnexBSource.h"
#include "webrtc_qos/types.h"

namespace webrtc_qos_plain {

struct RealtimeH264SourceConfig {
	int width{320};
	int height{180};
	int fps{15};
	uint32_t bitrateBps{1200000};
	uint32_t minBitrateBps{300000};
	uint32_t maxBitrateBps{2500000};
	std::string pattern{"testsrc"};
};

struct RealtimeH264SourceMetrics {
	uint64_t framesGenerated{0};
	uint64_t framesEncoded{0};
	uint64_t accessUnits{0};
	uint64_t keyframes{0};
	uint64_t encoderRecreates{0};
	uint64_t bitrateChanges{0};
	uint64_t fpsChanges{0};
	uint64_t forcedKeyframeRequests{0};
	uint32_t currentBitrateBps{0};
	uint32_t currentFps{0};
	int width{0};
	int height{0};
	bool lastAccessUnitKeyframe{false};
};

class RealtimeH264Source {
public:
	explicit RealtimeH264Source(RealtimeH264SourceConfig config);

	bool Open(std::string* error);
	bool ApplyEncoderAdaptation(const webrtc_qos::EncoderAdaptation& adaptation, std::string* error);
	bool NextAccessUnit(int64_t nowUs, AnnexBAccessUnit* out, std::string* error);

	const RealtimeH264SourceMetrics& metrics() const { return metrics_; }

private:
	bool RecreateEncoder(std::string* error);
	void GenerateFrame();
	int64_t FrameIntervalUs() const;

	RealtimeH264SourceConfig config_;
	RealtimeH264SourceMetrics metrics_;
	mediasoup::ffmpeg::Encoder encoder_;
	mediasoup::ffmpeg::FramePtr frame_;
	bool opened_{false};
	bool forceKeyframe_{true};
	int64_t startWallUs_{0};
	int64_t nextFrameTimeUs_{0};
	uint64_t frameIndex_{0};
};

} // namespace webrtc_qos_plain
