#include "push/PushTrackSourceWorker.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include "common/RuntimeLogHelpers.h"
#include "push/EncodedAccessUnit.h"

namespace webrtc_qos_plain {

const char* ToString(PushTrackSourceMode mode)
{
	switch (mode) {
		case PushTrackSourceMode::kCopy:
			return "copy";
		case PushTrackSourceMode::kSynthetic:
			return "synthetic";
		case PushTrackSourceMode::kMp4DecodeLoop:
			return "mp4_decode_loop";
		case PushTrackSourceMode::kV4L2:
			return "v4l2";
	}
	return "unknown";
}

PushTrackSourceWorker::PushTrackSourceWorker(
	PushTrackSourceWorkerConfig config,
	PushSdkTransportThread* sdkThread,
	std::shared_ptr<spdlog::logger> logger)
	: config_(std::move(config)),
	  sdkThread_(sdkThread),
	  logger_(std::move(logger))
{
}

PushTrackSourceWorker::~PushTrackSourceWorker()
{
	Stop();
}

int PushTrackSourceWorker::Start(std::string* error)
{
	if (!sdkThread_) {
		if (error) *error = "push track source worker missing SDK thread";
		return 2;
	}
	if (running_.exchange(true)) return 0;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	eof_.store(false, std::memory_order_relaxed);
	queuedAu_.store(0, std::memory_order_relaxed);
	enqueueFailures_.store(0, std::memory_order_relaxed);
	injectedEncoderDelayCount_.store(0, std::memory_order_relaxed);
	injectedEncoderDelayTotalMs_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
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
	lock.unlock();
	if (thread_.joinable()) thread_.join();
	if (error) *error = startupError_;
	return startupCode_ == 0 ? 3 : startupCode_;
}

void PushTrackSourceWorker::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	if (thread_.joinable()) thread_.join();
}

void PushTrackSourceWorker::StoreEncoderAdaptation(const webrtc_qos::EncoderAdaptation& adaptation)
{
	adaptation_.Store(adaptation);
	if (rawEncodeWorker_) rawEncodeWorker_->StoreEncoderAdaptation(adaptation);
}

PushTrackSourceWorkerMetrics PushTrackSourceWorker::metrics() const
{
	PushTrackSourceWorkerMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.eof = eof_.load(std::memory_order_relaxed);
	out.trackId = config_.ids.track_id;
	out.senderSsrc = config_.ids.sender_ssrc;
	out.queuedAu = queuedAu_.load(std::memory_order_relaxed);
	out.enqueueFailures = enqueueFailures_.load(std::memory_order_relaxed);
	out.injectedEncoderDelayCount = injectedEncoderDelayCount_.load(std::memory_order_relaxed);
	out.injectedEncoderDelayTotalMs = injectedEncoderDelayTotalMs_.load(std::memory_order_relaxed);
	out.loopIterations = loopIterations_.load(std::memory_order_relaxed);
	out.lastHeartbeatUs = lastHeartbeatUs_.load(std::memory_order_relaxed);
	out.loopGapMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	if (rawEncodeWorker_) {
		const auto encodeMetrics = rawEncodeWorker_->metrics();
		out.queuedAu = encodeMetrics.queuedAu;
		out.enqueueFailures = encodeMetrics.enqueueFailures;
		out.injectedEncoderDelayCount = encodeMetrics.injectedEncoderDelayCount;
		out.injectedEncoderDelayTotalMs = encodeMetrics.injectedEncoderDelayTotalMs;
		out.rawQueueDepth = encodeMetrics.rawQueueDepth;
		out.rawQueueMaxDepth = encodeMetrics.rawQueueMaxDepth;
		out.rawQueueDroppedFrames = encodeMetrics.rawQueueDroppedFrames;
		out.rawQueuePushedFrames = encodeMetrics.rawQueuePushedFrames;
		out.rawQueuePoppedFrames = encodeMetrics.rawQueuePoppedFrames;
		out.sourceMetrics = encodeMetrics.sourceMetrics;
	} else {
		(void)sourceMetrics_.Load(&out.sourceMetrics);
	}
	if (v4l2CaptureWorker_) {
		const auto captureMetrics = v4l2CaptureWorker_->metrics();
		out.rawQueueDepth = captureMetrics.rawQueueDepth;
		out.rawQueueMaxDepth = std::max(out.rawQueueMaxDepth, captureMetrics.rawQueueMaxDepth);
		out.rawQueueDroppedFrames = std::max(out.rawQueueDroppedFrames, captureMetrics.rawQueueDroppedFrames);
		out.rawQueuePushedFrames = std::max(out.rawQueuePushedFrames, captureMetrics.rawQueuePushedFrames);
		out.rawQueuePoppedFrames = std::max(out.rawQueuePoppedFrames, captureMetrics.rawQueuePoppedFrames);
	}
	{
		std::lock_guard<std::mutex> lock(metadataMutex_);
		out.stopReason = stopReason_;
		out.fatalError = fatalError_;
	}
	return out;
}

bool PushTrackSourceWorker::hasFatalError() const
{
	if (v4l2CaptureWorker_ && v4l2CaptureWorker_->hasFatalError()) return true;
	if (rawEncodeWorker_ && rawEncodeWorker_->hasFatalError()) return true;
	std::lock_guard<std::mutex> lock(metadataMutex_);
	return !fatalError_.empty();
}

void PushTrackSourceWorker::Run()
{
	std::string error;
	if (!OpenSource(&error)) {
		StoreFatalError(error);
		StoreStopReason("source_open_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 2, error);
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	if (logger_) {
		logger_->info(
			"push_track_source_worker_started trackId={} senderSsrc={} mode={} trackName={} injectEncoderDelayMs={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			ToString(config_.mode),
			config_.trackName,
			config_.injectEncoderDelayMs);
	}
	CompleteStartup(true, 0, "");

	int64_t lastLoopUs = MonotonicNowUs();
	while (running_.load(std::memory_order_relaxed)) {
		const int64_t nowUs = MonotonicNowUs();
		RecordLoopTick(nowUs, &lastLoopUs);
		if (config_.mode == PushTrackSourceMode::kV4L2 && rawEncodeWorker_) {
			webrtc_qos::EncoderAdaptation adaptation;
			(void)adaptation_.Load(&adaptation);
			rawEncodeWorker_->StoreEncoderAdaptation(adaptation);
			PublishSourceMetrics();
			if (v4l2CaptureWorker_ && v4l2CaptureWorker_->hasFatalError()) {
				error = v4l2CaptureWorker_->metrics().fatalError;
				StoreFatalError(error);
				break;
			}
			if (rawEncodeWorker_->hasFatalError()) {
				error = rawEncodeWorker_->metrics().fatalError;
				StoreFatalError(error);
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, config_.processTickMs)));
			continue;
		}
		error.clear();
		const bool ok = config_.mode == PushTrackSourceMode::kCopy
			? PumpCopySource(nowUs, &error)
			: PumpRealtimeSource(nowUs, &error);
		PublishSourceMetrics();
		if (!ok && !error.empty()) {
			enqueueFailures_.fetch_add(1, std::memory_order_relaxed);
			StoreFatalError(error);
			if (logger_) {
				logger_->error(
					"push_track_source_worker_failed trackId={} senderSsrc={} mode={} error={}",
					config_.ids.track_id,
					config_.ids.sender_ssrc,
					ToString(config_.mode),
					error);
			}
			break;
		}
		if (config_.mode == PushTrackSourceMode::kCopy &&
			eof_.load(std::memory_order_relaxed) &&
			!config_.loopInput) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, config_.processTickMs)));
	}

	StoreStopReason(hasFatalError() ? "fatal_error" : (eof_.load(std::memory_order_relaxed) ? "eof" : "stopped"));
	StopSplitWorkers();
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto finalMetrics = metrics();
		logger_->info(
			"push_track_source_worker_stopped trackId={} senderSsrc={} mode={} queuedAu={} enqueueFailures={} injectedEncoderDelayCount={} injectedEncoderDelayTotalMs={} eof={} loopIterations={} loopGapMaxUs={} rawQueueDroppedFrames={} rawQueueMaxDepth={} rawQueuePushedFrames={} rawQueuePoppedFrames={} stopReason={} fatalError={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			ToString(config_.mode),
			finalMetrics.queuedAu,
			finalMetrics.enqueueFailures,
			finalMetrics.injectedEncoderDelayCount,
			finalMetrics.injectedEncoderDelayTotalMs,
			finalMetrics.eof,
			finalMetrics.loopIterations,
			finalMetrics.loopGapMaxUs,
			finalMetrics.rawQueueDroppedFrames,
			finalMetrics.rawQueueMaxDepth,
			finalMetrics.rawQueuePushedFrames,
			finalMetrics.rawQueuePoppedFrames,
			finalMetrics.stopReason,
			finalMetrics.fatalError);
	}
}

bool PushTrackSourceWorker::OpenSource(std::string* error)
{
	switch (config_.mode) {
		case PushTrackSourceMode::kCopy:
			copySource_.emplace(config_.inputPath, config_.loopInput);
			if (!copySource_->Open(error)) return false;
			haveCopyAu_ = copySource_->NextAccessUnit(&nextCopyAu_, error);
			if (!haveCopyAu_ && error && !error->empty()) return false;
			if (!haveCopyAu_) eof_.store(true, std::memory_order_relaxed);
			return true;
		case PushTrackSourceMode::kSynthetic: {
			RealtimeH264SourceConfig sourceConfig;
			sourceConfig.width = config_.syntheticWidth;
			sourceConfig.height = config_.syntheticHeight;
			sourceConfig.fps = config_.syntheticFps;
			sourceConfig.bitrateBps = config_.startBitrateBps;
			sourceConfig.minBitrateBps = config_.minBitrateBps;
			sourceConfig.maxBitrateBps = config_.maxBitrateBps;
			sourceConfig.pattern = config_.syntheticPattern;
			realtimeSource_.emplace(sourceConfig);
			return realtimeSource_->Open(error);
		}
		case PushTrackSourceMode::kMp4DecodeLoop: {
			Mp4DecodeH264SourceConfig sourceConfig;
			sourceConfig.path = config_.inputPath;
			sourceConfig.loopInput = config_.loopInput;
			sourceConfig.bitrateBps = config_.startBitrateBps;
			sourceConfig.minBitrateBps = config_.minBitrateBps;
			sourceConfig.maxBitrateBps = config_.maxBitrateBps;
			mp4DecodeSource_.emplace(sourceConfig);
			return mp4DecodeSource_->Open(error);
		}
		case PushTrackSourceMode::kV4L2: {
			rawQueue_ = std::make_shared<BoundedQueue<RawVideoFrame>>(3);
			V4L2RawFrameCaptureWorkerConfig captureConfig;
			captureConfig.ids = config_.ids;
			captureConfig.trackName = config_.trackName;
			captureConfig.device = config_.v4l2Device;
			captureConfig.width = config_.v4l2Width;
			captureConfig.height = config_.v4l2Height;
			captureConfig.fps = config_.v4l2Fps;
			captureConfig.inputFormat = config_.v4l2InputFormat;
			captureConfig.processTickMs = config_.processTickMs;
			v4l2CaptureWorker_ = std::make_unique<V4L2RawFrameCaptureWorker>(captureConfig, rawQueue_, logger_);

			RawFrameEncodeWorkerConfig encodeConfig;
			encodeConfig.ids = config_.ids;
			encodeConfig.trackName = config_.trackName;
			encodeConfig.width = config_.v4l2Width;
			encodeConfig.height = config_.v4l2Height;
			encodeConfig.fps = config_.v4l2Fps;
			encodeConfig.processTickMs = config_.processTickMs;
			encodeConfig.injectEncoderDelayMs = config_.injectEncoderDelayMs;
			encodeConfig.startBitrateBps = config_.startBitrateBps;
			encodeConfig.minBitrateBps = config_.minBitrateBps;
			encodeConfig.maxBitrateBps = config_.maxBitrateBps;
			rawEncodeWorker_ = std::make_unique<RawFrameEncodeWorker>(encodeConfig, rawQueue_, sdkThread_, logger_);

			const int captureRc = v4l2CaptureWorker_->Start(error);
			if (captureRc != 0) return false;
			const int encodeRc = rawEncodeWorker_->Start(error);
			if (encodeRc != 0) {
				v4l2CaptureWorker_->Stop();
				return false;
			}
			return true;
		}
	}
	if (error) *error = "unsupported push track source mode";
	return false;
}

bool PushTrackSourceWorker::ApplyAdaptation(int64_t nowUs, std::string* error)
{
	webrtc_qos::EncoderAdaptation adaptation;
	(void)adaptation_.Load(&adaptation);
	if (realtimeSource_) return realtimeSource_->ApplyEncoderAdaptation(adaptation, nowUs, error);
	if (mp4DecodeSource_) return mp4DecodeSource_->ApplyEncoderAdaptation(adaptation, nowUs, error);
	return true;
}

bool PushTrackSourceWorker::NextRealtimeAccessUnit(
	int64_t nowUs,
	AnnexBAccessUnit* out,
	std::string* error)
{
	if (realtimeSource_) return realtimeSource_->NextAccessUnit(nowUs, out, error);
	if (mp4DecodeSource_) return mp4DecodeSource_->NextAccessUnit(nowUs, out, error);
	if (error) *error = "push track realtime source is not open";
	return false;
}

bool PushTrackSourceWorker::PumpRealtimeSource(int64_t nowUs, std::string* error)
{
	if (!ApplyAdaptation(nowUs, error)) return false;
	bool produced = false;
	while (running_.load(std::memory_order_relaxed)) {
		AnnexBAccessUnit au;
		if (!NextRealtimeAccessUnit(nowUs, &au, error)) break;
		produced = true;
		if (config_.injectEncoderDelayMs > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(config_.injectEncoderDelayMs));
			injectedEncoderDelayCount_.fetch_add(1, std::memory_order_relaxed);
			injectedEncoderDelayTotalMs_.fetch_add(
				static_cast<uint64_t>(config_.injectEncoderDelayMs),
				std::memory_order_relaxed);
		}
		if (!EnqueueAccessUnit(au, realtimeStartWallUs_ == 0 ? nowUs : realtimeStartWallUs_ + au.mediaTimeUs, error)) {
			return false;
		}
	}
	if (error && !error->empty()) return false;
	if (produced && realtimeStartWallUs_ == 0) realtimeStartWallUs_ = nowUs;
	return true;
}

bool PushTrackSourceWorker::PumpCopySource(int64_t nowUs, std::string* error)
{
	if (!haveCopyAu_) {
		eof_.store(true, std::memory_order_relaxed);
		return true;
	}
	if (firstCopyAu_) {
		firstCopyAu_ = false;
		copyStartWallUs_ = nowUs;
		copyFirstMediaUs_ = nextCopyAu_.mediaTimeUs;
	}
	const int64_t scheduledUs = copyStartWallUs_ + (nextCopyAu_.mediaTimeUs - copyFirstMediaUs_);
	if (nowUs < scheduledUs) return true;
	if (config_.injectEncoderDelayMs > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(config_.injectEncoderDelayMs));
		injectedEncoderDelayCount_.fetch_add(1, std::memory_order_relaxed);
		injectedEncoderDelayTotalMs_.fetch_add(
			static_cast<uint64_t>(config_.injectEncoderDelayMs),
			std::memory_order_relaxed);
	}
	if (!EnqueueAccessUnit(nextCopyAu_, scheduledUs, error)) return false;
	error->clear();
	haveCopyAu_ = copySource_->NextAccessUnit(&nextCopyAu_, error);
	if (!haveCopyAu_ && error && !error->empty()) return false;
	if (!haveCopyAu_) eof_.store(true, std::memory_order_relaxed);
	return true;
}

bool PushTrackSourceWorker::EnqueueAccessUnit(
	const AnnexBAccessUnit& accessUnit,
	int64_t captureTimeUs,
	std::string* error)
{
	EncodedAccessUnit item;
	auto status = CopyEncodedAccessUnit(accessUnit, captureTimeUs, config_.ids, &item);
	if (status) status = sdkThread_->Enqueue(std::move(item));
	if (!status) {
		enqueueFailures_.fetch_add(1, std::memory_order_relaxed);
		if (error) *error = StatusToString(status);
		return false;
	}
	queuedAu_.fetch_add(1, std::memory_order_relaxed);
	return true;
}

void PushTrackSourceWorker::PublishSourceMetrics()
{
	if (realtimeSource_) sourceMetrics_.Store(realtimeSource_->metrics());
	else if (mp4DecodeSource_) sourceMetrics_.Store(mp4DecodeSource_->metrics());
	else if (rawEncodeWorker_) sourceMetrics_.Store(rawEncodeWorker_->metrics().sourceMetrics);
}

void PushTrackSourceWorker::RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs)
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

void PushTrackSourceWorker::CompleteStartup(bool ok, int code, const std::string& error)
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

void PushTrackSourceWorker::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	stopReason_ = reason;
}

void PushTrackSourceWorker::StoreFatalError(const std::string& error)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	fatalError_ = error;
}

void PushTrackSourceWorker::StopSplitWorkers()
{
	if (v4l2CaptureWorker_) v4l2CaptureWorker_->Stop();
	if (rawEncodeWorker_) rawEncodeWorker_->Stop();
}

} // namespace webrtc_qos_plain
