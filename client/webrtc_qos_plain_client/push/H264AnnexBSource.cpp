#include "push/H264AnnexBSource.h"

#include "ffmpeg/AvPtr.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <stdexcept>

namespace webrtc_qos_plain {
namespace msff = mediasoup::ffmpeg;

H264AnnexBSource::H264AnnexBSource(std::string path, bool loopInput)
	: path_(std::move(path)), loopInput_(loopInput)
{
}

bool H264AnnexBSource::Open(std::string* error)
{
	loopOffsetUs_ = 0;
	lastOutputTimeUs_ = 0;
	fallbackFrameIndex_ = 0;
	draining_ = false;
	return OpenInternal(error);
}

bool H264AnnexBSource::OpenInternal(std::string* error)
{
	try {
		input_.emplace(msff::InputFormat::Open(path_));
		input_->FindStreamInfo();
		videoIndex_ = input_->FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		if (videoIndex_ < 0) throw std::runtime_error("input has no video stream");
		auto* stream = input_->StreamAt(videoIndex_);
		if (!stream || !stream->codecpar) throw std::runtime_error("invalid video stream");
		if (stream->codecpar->codec_id != AV_CODEC_ID_H264)
			throw std::runtime_error("input video codec is not H264");
		timeBase_ = stream->time_base;
		firstPacketTimeUs_ = AV_NOPTS_VALUE;
		draining_ = false;
		bsf_.emplace(msff::BitstreamFilter::Create(
			"h264_mp4toannexb",
			stream->codecpar,
			stream->time_base));
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		input_.reset();
		bsf_.reset();
		videoIndex_ = -1;
		return false;
	}
}

bool H264AnnexBSource::NextAccessUnit(AnnexBAccessUnit* out, std::string* error)
{
	if (!out) return false;
	if (!input_ || !bsf_) {
		if (error) *error = "H264AnnexBSource is not open";
		return false;
	}

	auto packet = msff::MakePacket();
	auto filtered = msff::MakePacket();
	while (true) {
		msff::PacketUnref(filtered.get());
		if (bsf_->ReceivePacket(filtered.get())) {
			out->bytes.assign(filtered->data, filtered->data + filtered->size);
			out->keyframe = (filtered->flags & AV_PKT_FLAG_KEY) != 0;
			out->mediaTimeUs = PacketTimeUs(filtered.get());
			return true;
		}

		if (draining_) return false;

		msff::PacketUnref(packet.get());
		if (!input_->ReadPacket(packet.get())) {
			if (!loopInput_) {
				(void)bsf_->SendPacket(nullptr);
				draining_ = true;
				continue;
			}
			if (!ReopenForLoop(error)) return false;
			continue;
		}
		if (packet->stream_index != videoIndex_) continue;
		if (!bsf_->SendPacket(packet.get())) continue;
	}
}

bool H264AnnexBSource::ReopenForLoop(std::string* error)
{
	loopOffsetUs_ = lastOutputTimeUs_ + 33333;
	input_.reset();
	bsf_.reset();
	videoIndex_ = -1;
	firstPacketTimeUs_ = AV_NOPTS_VALUE;
	draining_ = false;
	return OpenInternal(error);
}

int64_t H264AnnexBSource::PacketTimeUs(const AVPacket* packet)
{
	int64_t packetTs = AV_NOPTS_VALUE;
	if (packet) {
		if (packet->pts != AV_NOPTS_VALUE) packetTs = packet->pts;
		else if (packet->dts != AV_NOPTS_VALUE) packetTs = packet->dts;
	}

	int64_t relativeUs = 0;
	if (packetTs != AV_NOPTS_VALUE) {
		const int64_t packetUs = av_rescale_q(packetTs, timeBase_, AVRational{1, 1000000});
		if (firstPacketTimeUs_ == AV_NOPTS_VALUE) firstPacketTimeUs_ = packetUs;
		relativeUs = packetUs - firstPacketTimeUs_;
	} else {
		relativeUs = fallbackFrameIndex_ * 33333;
	}
	++fallbackFrameIndex_;
	const int64_t outputUs = loopOffsetUs_ + relativeUs;
	lastOutputTimeUs_ = outputUs > lastOutputTimeUs_ ? outputUs : lastOutputTimeUs_ + 33333;
	return lastOutputTimeUs_;
}

} // namespace webrtc_qos_plain
