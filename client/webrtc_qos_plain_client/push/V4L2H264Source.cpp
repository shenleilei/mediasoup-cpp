#include "push/V4L2H264Source.h"

#include "ffmpeg/AvError.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <utility>

namespace webrtc_qos_plain {
namespace msff = mediasoup::ffmpeg;
namespace {

uint32_t ClampBitrate(uint32_t value, uint32_t minValue, uint32_t maxValue)
{
	if (maxValue < minValue) maxValue = minValue;
	return std::max(minValue, std::min(value, maxValue));
}

int ClampFps(int value)
{
	return std::max(1, std::min(value, 60));
}

int EvenAtLeast16(int value)
{
	value = std::max(16, value);
	return value % 2 == 0 ? value : value + 1;
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

void SetDict(AVDictionary** opts, const char* key, const std::string& value)
{
	if (!value.empty()) av_dict_set(opts, key, value.c_str(), 0);
}

struct DictionaryDeleter {
	void operator()(AVDictionary* dict) const {
		if (dict) av_dict_free(&dict);
	}
};

using DictionaryPtr = std::unique_ptr<AVDictionary, DictionaryDeleter>;

DictionaryPtr MakeV4L2Options(int width, int height, int fps, const std::string& inputFormat)
{
	AVDictionary* opts = nullptr;
	const std::string videoSize = std::to_string(width) + "x" + std::to_string(height);
	const std::string frameRate = std::to_string(fps);
	av_dict_set(&opts, "video_size", videoSize.c_str(), 0);
	av_dict_set(&opts, "framerate", frameRate.c_str(), 0);
	SetDict(&opts, "input_format", inputFormat);
	return DictionaryPtr(opts);
}

msff::InputFormat OpenV4L2Input(const std::string& device,
	const AVInputFormat* v4l2,
	int width,
	int height,
	int fps,
	const std::string& inputFormat)
{
	auto opts = MakeV4L2Options(width, height, fps, inputFormat);
	AVDictionary* rawOpts = opts.release();
	try {
		auto input = msff::InputFormat::OpenWithFormat(device, v4l2, &rawOpts);
		av_dict_free(&rawOpts);
		return input;
	} catch (...) {
		av_dict_free(&rawOpts);
		throw;
	}
}

} // namespace

V4L2H264Source::V4L2H264Source(V4L2H264SourceConfig config)
	: config_(std::move(config))
{
}

bool V4L2H264Source::Open(std::string* error)
{
	metrics_ = RealtimeH264SourceMetrics{};
	input_.reset();
	decoder_.reset();
	encoder_ = msff::Encoder();
	packet_.reset();
	decodedFrame_.reset();
	convertedFrame_.reset();
	sws_.reset();
	videoIndex_ = -1;
	opened_ = false;
	decodedFrameReady_ = false;
	forceKeyframe_ = true;
	pendingForcedKeyframe_ = false;
	pendingForcedKeyframeRequestUs_ = 0;
	startWallUs_ = 0;
	nextFrameTimeUs_ = 0;
	frameIndex_ = 0;

	config_.width = EvenAtLeast16(config_.width);
	config_.height = EvenAtLeast16(config_.height);
	config_.fps = ClampFps(config_.fps);
	config_.bitrateBps = ClampBitrate(config_.bitrateBps, config_.minBitrateBps, config_.maxBitrateBps);
	if (!OpenInput(error)) return false;
	if (!RecreateEncoder(error)) return false;
	packet_ = msff::MakePacket();
	decodedFrame_ = msff::MakeFrame();
	if (!packet_ || !decodedFrame_) {
		if (error) *error = "av packet/frame alloc failed";
		return false;
	}
	opened_ = true;
	return true;
}

bool V4L2H264Source::OpenInput(std::string* error)
{
	try {
		avdevice_register_all();
		const AVInputFormat* v4l2 = av_find_input_format("v4l2");
		if (!v4l2) throw std::runtime_error("v4l2 input format not available");

		try {
			input_.emplace(OpenV4L2Input(
				config_.device, v4l2, config_.width, config_.height, config_.fps, config_.inputFormat));
		} catch (...) {
			if (config_.inputFormat.empty()) throw;
			input_.emplace(OpenV4L2Input(
				config_.device, v4l2, config_.width, config_.height, config_.fps, ""));
		}
		input_->FindStreamInfo();
		videoIndex_ = input_->FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		if (videoIndex_ < 0) throw std::runtime_error("v4l2 input has no video stream");
		auto* stream = input_->StreamAt(videoIndex_);
		if (!stream || !stream->codecpar) throw std::runtime_error("invalid v4l2 video stream");
		decoder_.emplace(msff::Decoder::OpenFromParameters(stream->codecpar));
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		input_.reset();
		decoder_.reset();
		videoIndex_ = -1;
		return false;
	}
}

bool V4L2H264Source::ApplyEncoderAdaptation(
	const webrtc_qos::EncoderAdaptation& adaptation,
	int64_t nowUs,
	std::string* error)
{
	if (!opened_) {
		if (error) *error = "V4L2H264Source is not open";
		return false;
	}

	const uint32_t targetBitrate = ClampBitrate(
		adaptation.target_bitrate_bps == 0 ? config_.bitrateBps : adaptation.target_bitrate_bps,
		config_.minBitrateBps,
		config_.maxBitrateBps);
	const int targetFps = ClampFps(adaptation.max_fps == 0 ? config_.fps : static_cast<int>(adaptation.max_fps));
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

bool V4L2H264Source::NextAccessUnit(int64_t nowUs, AnnexBAccessUnit* out, std::string* error)
{
	if (!out) return false;
	if (!opened_) {
		if (error) *error = "V4L2H264Source is not open";
		return false;
	}
	if (startWallUs_ == 0) {
		startWallUs_ = nowUs;
		nextFrameTimeUs_ = nowUs;
	}
	if (nowUs < nextFrameTimeUs_) return false;

	while (true) {
		if (EncodeDecodedFrame(nowUs, out, error)) return true;
		if (!DecodeNextFrame(error)) return false;
	}
}

bool V4L2H264Source::DecodeNextFrame(std::string* error)
{
	try {
		while (true) {
			if (decoder_ && decoder_->ReceiveFrame(decodedFrame_.get())) {
				decodedFrameReady_ = true;
				return true;
			}
			msff::PacketUnref(packet_.get());
			if (!input_->ReadPacket(packet_.get())) {
				if (error) *error = "v4l2 input ended";
				return false;
			}
			if (packet_->stream_index != videoIndex_) continue;
			(void)decoder_->SendPacket(packet_.get());
		}
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		return false;
	}
}

bool V4L2H264Source::EncodeDecodedFrame(int64_t nowUs, AnnexBAccessUnit* out, std::string* error)
{
	try {
		if (!decodedFrameReady_) return false;
		AVFrame* frame = PrepareEncoderFrame(decodedFrame_.get(), error);
		if (!frame) return false;
		frame->pts = static_cast<int64_t>(frameIndex_);
		frame->pict_type = forceKeyframe_ ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
		forceKeyframe_ = false;

		if (!encoder_.SendFrame(frame)) return false;
		++metrics_.framesEncoded;

		auto packet = msff::MakePacket();
		if (!packet) throw std::runtime_error("av_packet_alloc failed");
		if (!encoder_.ReceivePacket(packet.get())) {
			++frameIndex_;
			nextFrameTimeUs_ += FrameIntervalUs();
			av_frame_unref(decodedFrame_.get());
			decodedFrameReady_ = false;
			return false;
		}

		out->bytes.assign(packet->data, packet->data + packet->size);
		out->mediaTimeUs = static_cast<int64_t>(frameIndex_) * FrameIntervalUs();
		out->keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0 || PacketHasIdr(packet.get());
		++metrics_.accessUnits;
		if (out->keyframe) {
			++metrics_.keyframes;
			RecordForcedKeyframeIfNeeded(nowUs, true);
		}
		metrics_.lastAccessUnitKeyframe = out->keyframe;
		++frameIndex_;
		nextFrameTimeUs_ += FrameIntervalUs();
		av_frame_unref(decodedFrame_.get());
		decodedFrameReady_ = false;
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		return false;
	}
}

AVFrame* V4L2H264Source::PrepareEncoderFrame(AVFrame* decodedFrame, std::string* error)
{
	if (!decodedFrame) {
		if (error) *error = "missing decoded frame";
		return nullptr;
	}
	if (decodedFrame->width <= 0 || decodedFrame->height <= 0 ||
		decodedFrame->format == AV_PIX_FMT_NONE || !decodedFrame->data[0]) {
		if (error) *error = "decoded frame is incomplete";
		return nullptr;
	}
	++metrics_.framesGenerated;
	if (decodedFrame->format == AV_PIX_FMT_YUV420P &&
		decodedFrame->width == metrics_.width &&
		decodedFrame->height == metrics_.height) {
		return decodedFrame;
	}

	if (!convertedFrame_) {
		convertedFrame_ = msff::MakeFrame();
		if (!convertedFrame_) {
			if (error) *error = "av_frame_alloc failed";
			return nullptr;
		}
		convertedFrame_->format = AV_PIX_FMT_YUV420P;
		convertedFrame_->width = metrics_.width;
		convertedFrame_->height = metrics_.height;
		msff::FrameGetBuffer(convertedFrame_.get(), 32);
	}
	msff::FrameMakeWritable(convertedFrame_.get());
	sws_.reset(sws_getCachedContext(
		sws_.release(),
		decodedFrame->width,
		decodedFrame->height,
		static_cast<AVPixelFormat>(decodedFrame->format),
		metrics_.width,
		metrics_.height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		nullptr,
		nullptr,
		nullptr));
	if (!sws_) {
		if (error) *error = "sws_getCachedContext failed";
		return nullptr;
	}
	sws_scale(
		sws_.get(),
		decodedFrame->data,
		decodedFrame->linesize,
		0,
		decodedFrame->height,
		convertedFrame_->data,
		convertedFrame_->linesize);
	return convertedFrame_.get();
}

bool V4L2H264Source::RecreateEncoder(std::string* error)
{
	try {
		encoder_ = msff::Encoder::Create(AV_CODEC_ID_H264, [&](AVCodecContext* ctx) {
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

		metrics_.width = encoder_.width();
		metrics_.height = encoder_.height();
		metrics_.currentBitrateBps = config_.bitrateBps;
		metrics_.currentFps = static_cast<uint32_t>(config_.fps);
		convertedFrame_.reset();
		sws_.reset();
		++metrics_.encoderRecreates;
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		encoder_ = msff::Encoder();
		return false;
	}
}

int64_t V4L2H264Source::FrameIntervalUs() const
{
	return 1000000 / std::max(1, config_.fps);
}

void V4L2H264Source::RecordForcedKeyframeIfNeeded(int64_t nowUs, bool keyframe)
{
	if (!keyframe || !pendingForcedKeyframe_) return;
	++metrics_.forcedKeyframes;
	const int64_t delayUs = std::max<int64_t>(0, nowUs - pendingForcedKeyframeRequestUs_);
	metrics_.maxForcedKeyframeDelayUs = std::max(metrics_.maxForcedKeyframeDelayUs, delayUs);
	pendingForcedKeyframe_ = false;
	pendingForcedKeyframeRequestUs_ = 0;
}

} // namespace webrtc_qos_plain
