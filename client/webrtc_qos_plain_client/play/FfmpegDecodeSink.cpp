#include "play/FfmpegDecodeSink.h"

#include "ffmpeg/AvError.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <stdexcept>

namespace webrtc_qos_plain {
namespace msff = mediasoup::ffmpeg;

bool FfmpegDecodeSink::Open(std::string* error)
{
	try {
		const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
		if (!codec) throw std::runtime_error("H264 decoder unavailable");
		context_ = msff::MakeCodecContext(codec);
		if (!context_) throw std::runtime_error("avcodec_alloc_context3 failed");
		context_->flags2 |= AV_CODEC_FLAG2_CHUNKS;
		context_->thread_count = 1;
		msff::CheckError(avcodec_open2(context_.get(), codec, nullptr), "avcodec_open2(h264 decoder)");
		frame_ = msff::MakeFrame();
		if (!frame_) throw std::runtime_error("av_frame_alloc failed");
		metrics_ = FfmpegDecodeSinkMetrics{};
		metrics_.enabled = true;
		startUs_ = 0;
		firstDecodedFrameUs_ = 0;
		lastDecodedFrameUs_ = 0;
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		context_.reset();
		frame_.reset();
		metrics_ = FfmpegDecodeSinkMetrics{};
		return false;
	}
}

bool FfmpegDecodeSink::Decode(
	const webrtc_qos::AnnexBAccessUnitView& accessUnit,
	int64_t nowUs,
	std::string* error)
{
	if (!context_ || !frame_) {
		if (error) *error = "FfmpegDecodeSink is not open";
		return false;
	}
	if (!accessUnit.bytes || accessUnit.size == 0) {
		RecordDecodeError("empty access unit", error);
		return false;
	}
	if (startUs_ == 0) startUs_ = nowUs;
	++metrics_.accessUnitsIn;
	if (accessUnit.keyframe) ++metrics_.keyframesIn;

	AVPacket packet;
	av_init_packet(&packet);
	packet.data = const_cast<uint8_t*>(accessUnit.bytes);
	packet.size = static_cast<int>(accessUnit.size);
	packet.pts = accessUnit.capture_time_us;
	packet.dts = accessUnit.capture_time_us;

	const int sendErr = avcodec_send_packet(context_.get(), &packet);
	if (sendErr == AVERROR(EAGAIN)) {
		if (!DrainFrames(nowUs, error)) return false;
		const int retryErr = avcodec_send_packet(context_.get(), &packet);
		if (retryErr < 0 && retryErr != AVERROR(EAGAIN)) {
			RecordDecodeError(msff::ErrorToString(retryErr), error);
			return false;
		}
	} else if (sendErr < 0) {
		RecordDecodeError(msff::ErrorToString(sendErr), error);
		return false;
	}

	return DrainFrames(nowUs, error);
}

bool FfmpegDecodeSink::DrainFrames(int64_t nowUs, std::string* error)
{
	while (true) {
		const int recvErr = avcodec_receive_frame(context_.get(), frame_.get());
		if (recvErr == AVERROR(EAGAIN) || recvErr == AVERROR_EOF) return true;
		if (recvErr < 0) {
			RecordDecodeError(msff::ErrorToString(recvErr), error);
			return false;
		}
		RecordDecodedFrame(nowUs);
		av_frame_unref(frame_.get());
	}
}

void FfmpegDecodeSink::RecordDecodedFrame(int64_t nowUs)
{
	if (firstDecodedFrameUs_ == 0) {
		firstDecodedFrameUs_ = nowUs;
		metrics_.firstFrameDelayUs = startUs_ == 0 ? 0 : nowUs - startUs_;
	}
	if (lastDecodedFrameUs_ != 0) {
		const int64_t gapUs = nowUs - lastDecodedFrameUs_;
		if (gapUs > metrics_.maxFrameGapUs) metrics_.maxFrameGapUs = gapUs;
		if (gapUs >= freezeThresholdUs_) ++metrics_.freezeCount;
	}
	lastDecodedFrameUs_ = nowUs;
	++metrics_.decodedFrames;
	metrics_.width = frame_->width;
	metrics_.height = frame_->height;
	if (firstDecodedFrameUs_ > 0 && nowUs > firstDecodedFrameUs_) {
		const double elapsedSeconds = static_cast<double>(nowUs - firstDecodedFrameUs_) / 1000000.0;
		metrics_.outputFps = elapsedSeconds > 0.0
			? static_cast<double>(metrics_.decodedFrames - 1) / elapsedSeconds
			: 0.0;
	}
}

void FfmpegDecodeSink::RecordDecodeError(const std::string& message, std::string* error)
{
	++metrics_.decodeErrors;
	if (error) *error = message;
}

} // namespace webrtc_qos_plain
