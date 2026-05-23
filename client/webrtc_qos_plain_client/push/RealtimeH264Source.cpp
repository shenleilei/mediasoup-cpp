#include "push/RealtimeH264Source.h"

#include "ffmpeg/AvError.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace webrtc_qos_plain {
namespace msff = mediasoup::ffmpeg;
namespace {

uint32_t ClampBitrate(uint32_t value, uint32_t minValue, uint32_t maxValue)
{
	if (maxValue < minValue) maxValue = minValue;
	return std::max(minValue, std::min(value, maxValue));
}

int ClampFps(uint32_t value)
{
	return static_cast<int>(std::max<uint32_t>(1, std::min<uint32_t>(value, 60)));
}

void FillPlane(uint8_t* data, int linesize, int width, int height, uint8_t value)
{
	for (int y = 0; y < height; ++y)
		std::memset(data + y * linesize, value, static_cast<size_t>(width));
}

bool PacketHasIdr(const AVPacket* packet)
{
	if (!packet || !packet->data || packet->size <= 0) return false;
	const uint8_t* data = packet->data;
	const size_t size = static_cast<size_t>(packet->size);
	for (size_t i = 0; i + 4 < size; ++i) {
		size_t startCodeSize = 0;
		if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
			startCodeSize = 3;
		} else if (i + 5 < size && data[i] == 0 && data[i + 1] == 0 &&
			data[i + 2] == 0 && data[i + 3] == 1) {
			startCodeSize = 4;
		}
		if (startCodeSize == 0) continue;
		const uint8_t nalType = data[i + startCodeSize] & 0x1fu;
		if (nalType == 5) return true;
	}
	return false;
}

} // namespace

RealtimeH264Source::RealtimeH264Source(RealtimeH264SourceConfig config)
	: config_(std::move(config))
{
}

bool RealtimeH264Source::Open(std::string* error)
{
	config_.width = std::max(16, config_.width);
	config_.height = std::max(16, config_.height);
	if (config_.width % 2 != 0) ++config_.width;
	if (config_.height % 2 != 0) ++config_.height;
	config_.fps = ClampFps(static_cast<uint32_t>(config_.fps));
	config_.bitrateBps = ClampBitrate(config_.bitrateBps, config_.minBitrateBps, config_.maxBitrateBps);
	if (!RecreateEncoder(error)) return false;
	opened_ = true;
	forceKeyframe_ = true;
	startWallUs_ = 0;
	nextFrameTimeUs_ = 0;
	frameIndex_ = 0;
	return true;
}

bool RealtimeH264Source::ApplyEncoderAdaptation(
	const webrtc_qos::EncoderAdaptation& adaptation,
	int64_t nowUs,
	std::string* error)
{
	if (!opened_) {
		if (error) *error = "RealtimeH264Source is not open";
		return false;
	}

	const uint32_t targetBitrate = ClampBitrate(
		adaptation.target_bitrate_bps == 0 ? config_.bitrateBps : adaptation.target_bitrate_bps,
		config_.minBitrateBps,
		config_.maxBitrateBps);
	const int targetFps = ClampFps(adaptation.max_fps == 0 ? static_cast<uint32_t>(config_.fps) : adaptation.max_fps);

	const bool bitrateChanged = targetBitrate != config_.bitrateBps;
	if (targetFps != config_.fps) {
		config_.fps = targetFps;
		config_.bitrateBps = targetBitrate;
		++metrics_.fpsChanges;
		if (bitrateChanged) ++metrics_.bitrateChanges;
		if (!RecreateEncoder(error)) return false;
		forceKeyframe_ = true;
	} else if (bitrateChanged) {
		config_.bitrateBps = targetBitrate;
		encoder_.setBitRate(config_.bitrateBps);
		++metrics_.bitrateChanges;
	}

	if (adaptation.request_keyframe) {
		forceKeyframe_ = true;
		++metrics_.forcedKeyframeRequests;
		if (!pendingForcedKeyframe_) {
			pendingForcedKeyframe_ = true;
			pendingForcedKeyframeRequestUs_ = nowUs;
		}
	}

	metrics_.currentBitrateBps = config_.bitrateBps;
	metrics_.currentFps = static_cast<uint32_t>(config_.fps);
	return true;
}

bool RealtimeH264Source::NextAccessUnit(int64_t nowUs, AnnexBAccessUnit* out, std::string* error)
{
	if (!out) return false;
	if (!opened_) {
		if (error) *error = "RealtimeH264Source is not open";
		return false;
	}
	if (startWallUs_ == 0) {
		startWallUs_ = nowUs;
		nextFrameTimeUs_ = nowUs;
	}
	if (nowUs < nextFrameTimeUs_) return false;

	try {
		GenerateFrame();
		frame_->pts = static_cast<int64_t>(frameIndex_);
		frame_->pict_type = forceKeyframe_ ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
		forceKeyframe_ = false;

		if (!encoder_.SendFrame(frame_.get())) return false;
		++metrics_.framesEncoded;

		auto packet = msff::MakePacket();
		if (!packet) throw std::runtime_error("av_packet_alloc failed");
		while (encoder_.ReceivePacket(packet.get())) {
			out->bytes.assign(packet->data, packet->data + packet->size);
			out->mediaTimeUs = (static_cast<int64_t>(frameIndex_) * 1000000) /
				std::max(1, config_.fps);
			out->keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0 || PacketHasIdr(packet.get());
			++metrics_.accessUnits;
			if (out->keyframe) {
				++metrics_.keyframes;
				if (pendingForcedKeyframe_) {
					++metrics_.forcedKeyframes;
					const int64_t delayUs = std::max<int64_t>(0, nowUs - pendingForcedKeyframeRequestUs_);
					metrics_.maxForcedKeyframeDelayUs = std::max(metrics_.maxForcedKeyframeDelayUs, delayUs);
					pendingForcedKeyframe_ = false;
					pendingForcedKeyframeRequestUs_ = 0;
				}
			}
			metrics_.lastAccessUnitKeyframe = out->keyframe;
			msff::PacketUnref(packet.get());
			++frameIndex_;
			nextFrameTimeUs_ += FrameIntervalUs();
			return true;
		}
		++frameIndex_;
		nextFrameTimeUs_ += FrameIntervalUs();
		return false;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		return false;
	}
}

bool RealtimeH264Source::RecreateEncoder(std::string* error)
{
	try {
		encoder_ = msff::Encoder::Create(AV_CODEC_ID_H264, [this](AVCodecContext* ctx) {
			ctx->width = config_.width;
			ctx->height = config_.height;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->time_base = AVRational{1, std::max(1, config_.fps)};
			ctx->framerate = AVRational{std::max(1, config_.fps), 1};
			ctx->bit_rate = config_.bitrateBps;
			ctx->rc_max_rate = config_.bitrateBps;
			ctx->rc_buffer_size = config_.bitrateBps;
			ctx->gop_size = std::max(1, config_.fps);
			ctx->max_b_frames = 0;
			av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
			av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
			av_opt_set(ctx->priv_data, "profile", "baseline", 0);
			av_opt_set(ctx->priv_data, "repeat-headers", "1", 0);
		});

		frame_ = msff::MakeFrame();
		if (!frame_) throw std::runtime_error("av_frame_alloc failed");
		frame_->format = AV_PIX_FMT_YUV420P;
		frame_->width = config_.width;
		frame_->height = config_.height;
		msff::FrameGetBuffer(frame_.get(), 32);

		metrics_.width = config_.width;
		metrics_.height = config_.height;
		metrics_.currentBitrateBps = config_.bitrateBps;
		metrics_.currentFps = static_cast<uint32_t>(config_.fps);
		++metrics_.encoderRecreates;
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		encoder_ = msff::Encoder();
		frame_.reset();
		return false;
	}
}

void RealtimeH264Source::GenerateFrame()
{
	msff::FrameMakeWritable(frame_.get());
	const uint8_t luma = static_cast<uint8_t>((frameIndex_ * 3) % 256);
	const uint8_t chromaU = static_cast<uint8_t>(96 + ((frameIndex_ * 5) % 64));
	const uint8_t chromaV = static_cast<uint8_t>(128 + ((frameIndex_ * 7) % 64));

	for (int y = 0; y < config_.height; ++y) {
		uint8_t* row = frame_->data[0] + y * frame_->linesize[0];
		for (int x = 0; x < config_.width; ++x)
			row[x] = static_cast<uint8_t>(luma + ((x + y) % 64));
	}
	FillPlane(frame_->data[1], frame_->linesize[1], config_.width / 2, config_.height / 2, chromaU);
	FillPlane(frame_->data[2], frame_->linesize[2], config_.width / 2, config_.height / 2, chromaV);
	++metrics_.framesGenerated;
}

int64_t RealtimeH264Source::FrameIntervalUs() const
{
	return 1000000 / std::max(1, config_.fps);
}

} // namespace webrtc_qos_plain
