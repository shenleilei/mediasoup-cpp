#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/InputFormat.h"
#include "push/H264AnnexBSource.h"
#include "push/RealtimeH264Source.h"
#include "webrtc_qos/types.h"

namespace webrtc_qos_plain {

struct Mp4DecodeH264SourceConfig {
	std::string path;
	bool loopInput{false};
	uint32_t bitrateBps{1200000};
	uint32_t minBitrateBps{300000};
	uint32_t maxBitrateBps{2500000};
};

class Mp4DecodeH264Source {
public:
	explicit Mp4DecodeH264Source(Mp4DecodeH264SourceConfig config);

	bool Open(std::string* error);
	bool ApplyEncoderAdaptation(
		const webrtc_qos::EncoderAdaptation& adaptation,
		int64_t nowUs,
		std::string* error);
	bool NextAccessUnit(int64_t nowUs, AnnexBAccessUnit* out, std::string* error);

	const RealtimeH264SourceMetrics& metrics() const { return metrics_; }

private:
	bool OpenInput(std::string* error);
	bool RecreateEncoder(std::string* error);
	bool ReopenForLoop(std::string* error);
	bool DecodeNextFrame(std::string* error);
	bool EncodeDecodedFrame(int64_t nowUs, AnnexBAccessUnit* out, std::string* error);
	AVFrame* PrepareEncoderFrame(AVFrame* decodedFrame, std::string* error);
	int64_t DecodedFrameTimeUs(const AVFrame* frame);
	int64_t FrameIntervalUs() const;
	void RecordForcedKeyframeIfNeeded(int64_t nowUs, bool keyframe);

	Mp4DecodeH264SourceConfig config_;
	RealtimeH264SourceMetrics metrics_;
	std::optional<mediasoup::ffmpeg::InputFormat> input_;
	std::optional<mediasoup::ffmpeg::Decoder> decoder_;
	mediasoup::ffmpeg::Encoder encoder_;
	mediasoup::ffmpeg::PacketPtr packet_;
	mediasoup::ffmpeg::FramePtr decodedFrame_;
	mediasoup::ffmpeg::FramePtr convertedFrame_;
	mediasoup::ffmpeg::SwsContextPtr sws_;
	int videoIndex_{-1};
	AVRational timeBase_{1, 90000};
	int fps_{30};
	bool opened_{false};
	bool draining_{false};
	bool decodedFrameReady_{false};
	bool forceKeyframe_{true};
	bool pendingForcedKeyframe_{false};
	int64_t pendingForcedKeyframeRequestUs_{0};
	int64_t startWallUs_{0};
	int64_t nextFrameTimeUs_{0};
	int64_t firstDecodedFrameTimeUs_{AV_NOPTS_VALUE};
	int64_t loopOffsetUs_{0};
	int64_t lastOutputMediaUs_{0};
	uint64_t frameIndex_{0};
};

} // namespace webrtc_qos_plain
