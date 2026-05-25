#include "push/RawFrameEncodeWorker.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "common/RuntimeLogHelpers.h"

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

void CopyPlane(uint8_t* dst, int dstStride, const uint8_t* src, int width, int height)
{
	for (int y = 0; y < height; ++y)
		std::memcpy(dst + static_cast<size_t>(y) * static_cast<size_t>(dstStride),
			src + static_cast<size_t>(y) * static_cast<size_t>(width),
			static_cast<size_t>(width));
}

} // namespace

RawFrameEncodeWorker::RawFrameEncodeWorker(
	RawFrameEncodeWorkerConfig config,
	std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue,
	PushSdkTransportThread* sdkThread,
	std::shared_ptr<spdlog::logger> logger)
	: config_(std::move(config)),
	  rawQueue_(std::move(rawQueue)),
	  sdkThread_(sdkThread),
	  logger_(std::move(logger))
{
}

RawFrameEncodeWorker::~RawFrameEncodeWorker()
{
	Stop();
}

int RawFrameEncodeWorker::Start(std::string* error)
{
	if (!rawQueue_) {
		if (error) *error = "raw frame encode worker missing raw queue";
		return 2;
	}
	if (!sdkThread_) {
		if (error) *error = "raw frame encode worker missing SDK thread";
		return 2;
	}
	if (running_.exchange(true)) return 0;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	queuedAu_.store(0, std::memory_order_relaxed);
	enqueueFailures_.store(0, std::memory_order_relaxed);
	injectedEncoderDelayCount_.store(0, std::memory_order_relaxed);
	injectedEncoderDelayTotalMs_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lock(metricsMutex_);
		sourceMetrics_ = RealtimeH264SourceMetrics{};
	}
	StoreStopReason("");
	StoreFatalError("");
	{
		std::lock_guard<std::mutex> lock(startupMutex_);
		startupDone_ = false;
		startupOk_ = false;
		startupCode_ = 0;
		startupError_.clear();
	}
	thread_ = std::thread([this]() { Run(); });
	std::unique_lock<std::mutex> lock(startupMutex_);
	startupCv_.wait(lock, [&] { return startupDone_; });
	if (startupOk_) return 0;
	running_.store(false, std::memory_order_relaxed);
	if (rawQueue_) rawQueue_->Close();
	lock.unlock();
	if (thread_.joinable()) thread_.join();
	if (error) *error = startupError_;
	return startupCode_ == 0 ? 3 : startupCode_;
}

void RawFrameEncodeWorker::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	if (rawQueue_) rawQueue_->Close();
	if (thread_.joinable()) thread_.join();
}

void RawFrameEncodeWorker::StoreEncoderAdaptation(const webrtc_qos::EncoderAdaptation& adaptation)
{
	adaptation_.Store(adaptation);
}

RawFrameEncodeWorkerMetrics RawFrameEncodeWorker::metrics() const
{
	RawFrameEncodeWorkerMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.trackId = config_.ids.track_id;
	out.senderSsrc = config_.ids.sender_ssrc;
	out.queuedAu = queuedAu_.load(std::memory_order_relaxed);
	out.enqueueFailures = enqueueFailures_.load(std::memory_order_relaxed);
	out.injectedEncoderDelayCount = injectedEncoderDelayCount_.load(std::memory_order_relaxed);
	out.injectedEncoderDelayTotalMs = injectedEncoderDelayTotalMs_.load(std::memory_order_relaxed);
	out.loopIterations = loopIterations_.load(std::memory_order_relaxed);
	out.lastHeartbeatUs = lastHeartbeatUs_.load(std::memory_order_relaxed);
	out.loopGapMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	if (rawQueue_) {
		out.rawQueueDepth = rawQueue_->depth();
		out.rawQueueMaxDepth = rawQueue_->maxDepth();
		out.rawQueueDroppedFrames = rawQueue_->dropped();
		out.rawQueuePushedFrames = rawQueue_->pushed();
		out.rawQueuePoppedFrames = rawQueue_->popped();
	}
	{
		std::lock_guard<std::mutex> lock(metricsMutex_);
		out.sourceMetrics = sourceMetrics_;
	}
	{
		std::lock_guard<std::mutex> lock(metadataMutex_);
		out.stopReason = stopReason_;
		out.fatalError = fatalError_;
	}
	return out;
}

bool RawFrameEncodeWorker::hasFatalError() const
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	return !fatalError_.empty();
}

void RawFrameEncodeWorker::Run()
{
	std::string error;
	config_.width = EvenAtLeast16(config_.width);
	config_.height = EvenAtLeast16(config_.height);
	config_.fps = ClampFps(static_cast<uint32_t>(config_.fps));
	config_.startBitrateBps = ClampBitrate(config_.startBitrateBps, config_.minBitrateBps, config_.maxBitrateBps);
	if (!RecreateEncoder(&error)) {
		StoreFatalError(error);
		StoreStopReason("encoder_open_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 2, error);
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	if (logger_) {
		logger_->info(
			"raw_frame_encode_worker_started trackId={} senderSsrc={} trackName={} width={} height={} fps={} injectEncoderDelayMs={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			config_.trackName,
			config_.width,
			config_.height,
			config_.fps,
			config_.injectEncoderDelayMs);
	}
	CompleteStartup(true, 0, "");

	int64_t lastLoopUs = MonotonicNowUs();
	while (running_.load(std::memory_order_relaxed)) {
		const int64_t nowUs = MonotonicNowUs();
		RecordLoopTick(nowUs, &lastLoopUs);
		RawVideoFrame raw;
		if (!rawQueue_->PopFor(&raw, std::chrono::milliseconds(std::max(1, config_.processTickMs)))) {
			if (rawQueue_->closed()) break;
			continue;
		}
		error.clear();
		if (!ApplyAdaptation(nowUs, &error) || !EncodeFrame(raw, nowUs, &error)) {
			StoreFatalError(error);
			if (logger_) {
				logger_->error(
					"raw_frame_encode_worker_failed trackId={} senderSsrc={} error={}",
					config_.ids.track_id,
					config_.ids.sender_ssrc,
					error);
			}
			break;
		}
	}

	StoreStopReason(hasFatalError() ? "fatal_error" : "raw_queue_closed");
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto finalMetrics = metrics();
		logger_->info(
			"raw_frame_encode_worker_stopped trackId={} senderSsrc={} queuedAu={} enqueueFailures={} rawQueueDroppedFrames={} rawQueueMaxDepth={} injectedEncoderDelayCount={} injectedEncoderDelayTotalMs={} framesGenerated={} framesEncoded={} accessUnits={} keyframes={} loopIterations={} loopGapMaxUs={} stopReason={} fatalError={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			finalMetrics.queuedAu,
			finalMetrics.enqueueFailures,
			finalMetrics.rawQueueDroppedFrames,
			finalMetrics.rawQueueMaxDepth,
			finalMetrics.injectedEncoderDelayCount,
			finalMetrics.injectedEncoderDelayTotalMs,
			finalMetrics.sourceMetrics.framesGenerated,
			finalMetrics.sourceMetrics.framesEncoded,
			finalMetrics.sourceMetrics.accessUnits,
			finalMetrics.sourceMetrics.keyframes,
			finalMetrics.loopIterations,
			finalMetrics.loopGapMaxUs,
			finalMetrics.stopReason,
			finalMetrics.fatalError);
	}
}

bool RawFrameEncodeWorker::RecreateEncoder(std::string* error)
{
	try {
		encoder_ = msff::Encoder::Create(AV_CODEC_ID_H264, [&](AVCodecContext* ctx) {
			ctx->width = config_.width;
			ctx->height = config_.height;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->time_base = AVRational{1, std::max(1, config_.fps)};
			ctx->framerate = AVRational{std::max(1, config_.fps), 1};
			ctx->bit_rate = config_.startBitrateBps;
			ctx->rc_max_rate = config_.startBitrateBps;
			ctx->rc_buffer_size = config_.startBitrateBps;
			ctx->gop_size = std::max(1, config_.fps);
			ctx->max_b_frames = 0;
			av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
			av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
			av_opt_set(ctx->priv_data, "profile", "baseline", 0);
			av_opt_set(ctx->priv_data, "repeat-headers", "1", 0);
		});

		encoderFrame_ = msff::MakeFrame();
		if (!encoderFrame_) throw std::runtime_error("av_frame_alloc failed");
		encoderFrame_->format = AV_PIX_FMT_YUV420P;
		encoderFrame_->width = config_.width;
		encoderFrame_->height = config_.height;
		msff::FrameGetBuffer(encoderFrame_.get(), 32);
		{
			std::lock_guard<std::mutex> lock(metricsMutex_);
			sourceMetrics_.width = config_.width;
			sourceMetrics_.height = config_.height;
			sourceMetrics_.currentBitrateBps = config_.startBitrateBps;
			sourceMetrics_.currentFps = static_cast<uint32_t>(config_.fps);
			++sourceMetrics_.encoderRecreates;
		}
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		encoder_ = msff::Encoder();
		encoderFrame_.reset();
		return false;
	}
}

bool RawFrameEncodeWorker::ApplyAdaptation(int64_t nowUs, std::string* error)
{
	webrtc_qos::EncoderAdaptation adaptation;
	(void)adaptation_.Load(&adaptation);
	const uint32_t targetBitrate = ClampBitrate(
		adaptation.target_bitrate_bps == 0 ? config_.startBitrateBps : adaptation.target_bitrate_bps,
		config_.minBitrateBps,
		config_.maxBitrateBps);
	const int targetFps = ClampFps(adaptation.max_fps == 0 ? static_cast<uint32_t>(config_.fps) : adaptation.max_fps);
	const bool bitrateChanged = targetBitrate != config_.startBitrateBps;
	if (targetFps != config_.fps) {
		config_.fps = targetFps;
		config_.startBitrateBps = targetBitrate;
		{
			std::lock_guard<std::mutex> lock(metricsMutex_);
			++sourceMetrics_.fpsChanges;
			if (bitrateChanged) ++sourceMetrics_.bitrateChanges;
		}
		if (!RecreateEncoder(error)) return false;
		forceKeyframe_ = true;
	} else if (bitrateChanged) {
		config_.startBitrateBps = targetBitrate;
		encoder_.setBitRate(config_.startBitrateBps);
		std::lock_guard<std::mutex> lock(metricsMutex_);
		++sourceMetrics_.bitrateChanges;
		sourceMetrics_.currentBitrateBps = config_.startBitrateBps;
	}

	if (adaptation.request_keyframe) {
		forceKeyframe_ = true;
		std::lock_guard<std::mutex> lock(metricsMutex_);
		++sourceMetrics_.forcedKeyframeRequests;
		if (!pendingForcedKeyframe_) {
			pendingForcedKeyframe_ = true;
			pendingForcedKeyframeRequestUs_ = nowUs;
		}
	}

	{
		std::lock_guard<std::mutex> lock(metricsMutex_);
		sourceMetrics_.currentBitrateBps = config_.startBitrateBps;
		sourceMetrics_.currentFps = static_cast<uint32_t>(config_.fps);
	}
	return true;
}

bool RawFrameEncodeWorker::EncodeFrame(const RawVideoFrame& raw, int64_t nowUs, std::string* error)
{
	if (config_.injectEncoderDelayMs > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(config_.injectEncoderDelayMs));
		injectedEncoderDelayCount_.fetch_add(1, std::memory_order_relaxed);
		injectedEncoderDelayTotalMs_.fetch_add(
			static_cast<uint64_t>(config_.injectEncoderDelayMs),
			std::memory_order_relaxed);
	}
	if (!CopyRawFrameToEncoderFrame(raw, error)) return false;
	encoderFrame_->pts = static_cast<int64_t>(frameIndex_);
	encoderFrame_->pict_type = forceKeyframe_ ? AV_PICTURE_TYPE_I : AV_PICTURE_TYPE_NONE;
	forceKeyframe_ = false;

	if (!encoder_.SendFrame(encoderFrame_.get())) return true;
	{
		std::lock_guard<std::mutex> lock(metricsMutex_);
		++sourceMetrics_.framesGenerated;
		++sourceMetrics_.framesEncoded;
	}

	auto packet = msff::MakePacket();
	if (!packet) {
		if (error) *error = "av_packet_alloc failed";
		return false;
	}
	bool emitted = false;
	while (encoder_.ReceivePacket(packet.get())) {
		EncodedAccessUnit item;
		item.bytes.assign(packet->data, packet->data + packet->size);
		item.captureTimeUs = raw.captureTimeUs;
		item.keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0 || PacketHasIdr(packet.get());
		item.ids = config_.ids;
		auto status = sdkThread_->Enqueue(std::move(item));
		if (!status) {
			enqueueFailures_.fetch_add(1, std::memory_order_relaxed);
			if (error) *error = StatusToString(status);
			return false;
		}
		queuedAu_.fetch_add(1, std::memory_order_relaxed);
		{
			std::lock_guard<std::mutex> lock(metricsMutex_);
			++sourceMetrics_.accessUnits;
			if ((packet->flags & AV_PKT_FLAG_KEY) != 0 || PacketHasIdr(packet.get())) {
				++sourceMetrics_.keyframes;
				RecordForcedKeyframeIfNeeded(nowUs, true);
			}
			sourceMetrics_.lastAccessUnitKeyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0 || PacketHasIdr(packet.get());
		}
		emitted = true;
		msff::PacketUnref(packet.get());
	}
	++frameIndex_;
	return emitted || true;
}

bool RawFrameEncodeWorker::CopyRawFrameToEncoderFrame(const RawVideoFrame& raw, std::string* error)
{
	if (raw.width != config_.width || raw.height != config_.height ||
		raw.yuv420p.size() != RawVideoFrameSize(raw.width, raw.height)) {
		if (error) *error = "raw frame dimensions or buffer size do not match encoder";
		return false;
	}
	msff::FrameMakeWritable(encoderFrame_.get());
	const size_t ySize = static_cast<size_t>(raw.width) * static_cast<size_t>(raw.height);
	const size_t uvSize = ySize / 4;
	CopyPlane(encoderFrame_->data[0], encoderFrame_->linesize[0], raw.yuv420p.data(), raw.width, raw.height);
	CopyPlane(encoderFrame_->data[1], encoderFrame_->linesize[1], raw.yuv420p.data() + ySize, raw.width / 2, raw.height / 2);
	CopyPlane(encoderFrame_->data[2], encoderFrame_->linesize[2], raw.yuv420p.data() + ySize + uvSize, raw.width / 2, raw.height / 2);
	return true;
}

void RawFrameEncodeWorker::RecordForcedKeyframeIfNeeded(int64_t nowUs, bool keyframe)
{
	if (!keyframe || !pendingForcedKeyframe_) return;
	++sourceMetrics_.forcedKeyframes;
	const int64_t delayUs = std::max<int64_t>(0, nowUs - pendingForcedKeyframeRequestUs_);
	sourceMetrics_.maxForcedKeyframeDelayUs = std::max(sourceMetrics_.maxForcedKeyframeDelayUs, delayUs);
	pendingForcedKeyframe_ = false;
	pendingForcedKeyframeRequestUs_ = 0;
}

void RawFrameEncodeWorker::RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs)
{
	lastHeartbeatUs_.store(nowUs, std::memory_order_relaxed);
	loopIterations_.fetch_add(1, std::memory_order_relaxed);
	if (!lastLoopUs) return;
	const int64_t gapUs = std::max<int64_t>(0, nowUs - *lastLoopUs);
	int64_t currentMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	while (gapUs > currentMaxUs &&
		!loopGapMaxUs_.compare_exchange_weak(
			currentMaxUs,
			gapUs,
			std::memory_order_relaxed,
			std::memory_order_relaxed)) {
	}
	*lastLoopUs = nowUs;
}

void RawFrameEncodeWorker::CompleteStartup(bool ok, int code, const std::string& error)
{
	{
		std::lock_guard<std::mutex> lock(startupMutex_);
		startupDone_ = true;
		startupOk_ = ok;
		startupCode_ = code;
		startupError_ = error;
	}
	startupCv_.notify_all();
}

void RawFrameEncodeWorker::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	stopReason_ = reason;
}

void RawFrameEncodeWorker::StoreFatalError(const std::string& error)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	fatalError_ = error;
}

int64_t RawFrameEncodeWorker::FrameIntervalUs() const
{
	return 1000000 / std::max(1, config_.fps);
}

} // namespace webrtc_qos_plain
