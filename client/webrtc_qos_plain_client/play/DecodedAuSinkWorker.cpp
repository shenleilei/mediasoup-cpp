#include "play/DecodedAuSinkWorker.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>

#include "common/RuntimeLogHelpers.h"

namespace webrtc_qos_plain {

DecodedAuSinkWorker::DecodedAuSinkWorker(
	bool outputNull,
	std::string outputPath,
	bool decodeQoe,
	std::shared_ptr<spdlog::logger> logger,
	size_t queueCapacity,
	int injectSinkDelayMs,
	uint32_t trackId,
	uint32_t senderSsrc,
	std::string trackName)
	: outputNull_(outputNull),
	  outputPath_(std::move(outputPath)),
	  decodeQoe_(decodeQoe),
	  injectSinkDelayMs_(std::max(0, injectSinkDelayMs)),
	  trackId_(trackId),
	  senderSsrc_(senderSsrc),
	  trackName_(std::move(trackName)),
	  logger_(std::move(logger)),
	  queue_(queueCapacity)
{
	if (!outputNull_ && trackId_ > 0 && !outputPath_.empty()) {
		const std::filesystem::path base(outputPath_);
		const auto stem = base.stem().string();
		const auto ext = base.extension().string();
		outputPath_ = (base.parent_path() /
			(stem + ".track" + std::to_string(trackId_) + ext)).string();
	}
}

DecodedAuSinkWorker::~DecodedAuSinkWorker()
{
	Stop();
}

bool DecodedAuSinkWorker::Start(std::string* error)
{
	if (running_.exchange(true)) return true;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	injectedSinkDelayCount_.store(0, std::memory_order_relaxed);
	injectedSinkDelayTotalMs_.store(0, std::memory_order_relaxed);
	StoreStopReason("");
	AnnexBSink probeSink;
	if (outputNull_) {
		probeSink.EnableNull();
	} else if (!probeSink.OpenFile(outputPath_, error)) {
		running_.store(false);
		return false;
	}
	if (decodeQoe_) {
		FfmpegDecodeSink probeDecoder;
		if (!probeDecoder.Open(error)) {
			running_.store(false);
			return false;
		}
	}
	thread_ = std::thread([this]() { Run(); });
	return true;
}

void DecodedAuSinkWorker::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	queue_.Close();
	if (thread_.joinable()) thread_.join();
}

webrtc_qos::Status DecodedAuSinkWorker::Enqueue(const webrtc_qos::AnnexBAccessUnitView& accessUnit)
{
	if (!accessUnit.bytes || accessUnit.size == 0) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"empty decoded access unit");
	}
	if (trackId_ > 0 && accessUnit.ids.track_id != trackId_) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"decoded access unit routed to wrong track sink");
	}
	DecodedAccessUnit item;
	item.bytes.assign(accessUnit.bytes, accessUnit.bytes + accessUnit.size);
	item.captureTimeUs = accessUnit.capture_time_us;
	item.keyframe = accessUnit.keyframe;
	item.ids = accessUnit.ids;
	if (!queue_.PushDropOldest(std::move(item))) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kQueueFull,
			"decoded access unit queue is closed");
	}
	{
		std::lock_guard<std::mutex> lock(trackMetricsMutex_);
		++enqueuedAccessUnitsByTrack_[accessUnit.ids.track_id];
	}
	return webrtc_qos::Status::Ok();
}

DecodedAuSinkWorkerMetrics DecodedAuSinkWorker::metrics() const
{
	DecodedAuSinkWorkerMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.lastHeartbeatUs = lastHeartbeatUs_.load(std::memory_order_relaxed);
	out.loopGapMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	out.loopIterations = loopIterations_.load(std::memory_order_relaxed);
	out.enqueuedAccessUnits = queue_.pushed();
	out.droppedAccessUnits = queue_.dropped();
	out.writtenAccessUnits = writtenAccessUnits_.load(std::memory_order_relaxed);
	out.sinkWriteFailures = sinkWriteFailures_.load(std::memory_order_relaxed);
	out.injectedSinkDelayCount = injectedSinkDelayCount_.load(std::memory_order_relaxed);
	out.injectedSinkDelayTotalMs = injectedSinkDelayTotalMs_.load(std::memory_order_relaxed);
	out.queueDepth = queue_.depth();
	out.queueMaxDepth = queue_.maxDepth();
	out.trackId = trackId_;
	out.senderSsrc = senderSsrc_;
	out.trackName = trackName_;
	{
		std::lock_guard<std::mutex> lock(stopReasonMutex_);
		out.stopReason = stopReason_;
	}
	{
		std::lock_guard<std::mutex> lock(qoeMutex_);
		out.qoe = qoeMetrics_;
	}
	{
		std::lock_guard<std::mutex> lock(trackMetricsMutex_);
		out.writtenAccessUnitsByTrack = writtenAccessUnitsByTrack_;
		out.enqueuedAccessUnitsByTrack = enqueuedAccessUnitsByTrack_;
	}
	return out;
}

void DecodedAuSinkWorker::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(stopReasonMutex_);
	stopReason_ = reason;
}

void DecodedAuSinkWorker::StoreQoeMetrics(const FfmpegDecodeSinkMetrics& metrics)
{
	std::lock_guard<std::mutex> lock(qoeMutex_);
	qoeMetrics_ = metrics;
}

void DecodedAuSinkWorker::Run()
{
	AnnexBSink sink;
	std::string error;
	if (outputNull_) {
		sink.EnableNull();
	} else if (!sink.OpenFile(outputPath_, &error)) {
		if (logger_) logger_->error("decoded_sink_open_failed output={} error={}", outputPath_, error);
		StoreStopReason("sink_open_failed");
		stopped_.store(true, std::memory_order_relaxed);
		return;
	}
	FfmpegDecodeSink decodeSink;
	if (decodeQoe_ && !decodeSink.Open(&error)) {
		if (logger_) logger_->error("decoded_sink_qoe_open_failed error={}", error);
		StoreStopReason("qoe_open_failed");
		stopped_.store(true, std::memory_order_relaxed);
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	if (logger_) logger_->info("decoded_sink_worker_started trackId={} senderSsrc={} trackName={} outputNull={} outputPath={} decodeQoe={} injectSinkDelayMs={}",
		trackId_, senderSsrc_, trackName_, outputNull_, outputPath_, decodeQoe_, injectSinkDelayMs_);
	DecodedAccessUnit item;
	int64_t lastLoopUs = MonotonicNowUs();
	auto recordLoopTick = [&](int64_t nowUs) {
		lastHeartbeatUs_.store(nowUs, std::memory_order_relaxed);
		loopIterations_.fetch_add(1, std::memory_order_relaxed);
		const int64_t gapUs = std::max<int64_t>(0, nowUs - lastLoopUs);
		int64_t currentMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
		while (gapUs > currentMaxUs &&
			!loopGapMaxUs_.compare_exchange_weak(
				currentMaxUs,
				gapUs,
				std::memory_order_relaxed,
				std::memory_order_relaxed)) {
		}
		lastLoopUs = nowUs;
	};
	while (true) {
		if (!queue_.PopFor(&item, std::chrono::milliseconds(100))) {
			recordLoopTick(MonotonicNowUs());
			if (queue_.closed()) break;
			continue;
		}
		recordLoopTick(MonotonicNowUs());
		webrtc_qos::AnnexBAccessUnitView view;
		view.bytes = item.bytes.data();
		view.size = item.bytes.size();
		view.capture_time_us = item.captureTimeUs;
		view.keyframe = item.keyframe;
		view.ids = item.ids;
		if (injectSinkDelayMs_ > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(injectSinkDelayMs_));
			injectedSinkDelayCount_.fetch_add(1, std::memory_order_relaxed);
			injectedSinkDelayTotalMs_.fetch_add(
				static_cast<uint64_t>(injectSinkDelayMs_),
				std::memory_order_relaxed);
		}
		auto status = sink.Write(view);
		if (!status) {
			sinkWriteFailures_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->warn("annexb_sink_write_failed status={}", StatusToString(status));
		} else if (status) {
			writtenAccessUnits_.store(sink.writtenAccessUnits(), std::memory_order_relaxed);
			{
				std::lock_guard<std::mutex> lock(trackMetricsMutex_);
				++writtenAccessUnitsByTrack_[item.ids.track_id];
			}
		}
		if (decodeQoe_) {
			std::string decodeError;
			if (!decodeSink.Decode(view, MonotonicNowUs(), &decodeError) && logger_) {
				logger_->warn("decode_qoe_failed error={}", decodeError);
			}
			StoreQoeMetrics(decodeSink.metrics());
		}
	}
	StoreStopReason("queue_closed");
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto& qoe = decodeSink.metrics();
		logger_->info(
			"decoded_sink_worker_stopped stopReason={} outputAu={} queuePushed={} queuePopped={} queueDropped={} queueMaxDepth={} injectedSinkDelayCount={} injectedSinkDelayTotalMs={} loopIterations={} loopGapMaxUs={} lastHeartbeatUs={} qoeDecodedFrames={} qoeDecodeErrors={}",
			"queue_closed",
			sink.writtenAccessUnits(),
			queue_.pushed(),
			queue_.popped(),
			queue_.dropped(),
			queue_.maxDepth(),
			injectedSinkDelayCount_.load(std::memory_order_relaxed),
			injectedSinkDelayTotalMs_.load(std::memory_order_relaxed),
			loopIterations_.load(std::memory_order_relaxed),
			loopGapMaxUs_.load(std::memory_order_relaxed),
			lastHeartbeatUs_.load(std::memory_order_relaxed),
			qoe.decodedFrames,
			qoe.decodeErrors);
		logger_->info(
			"decoded_sink_track_stopped trackId={} senderSsrc={} trackName={} stopReason={} outputAu={} queuePushed={} queuePopped={} queueDropped={} queueMaxDepth={} injectedSinkDelayCount={} injectedSinkDelayTotalMs={} loopIterations={} loopGapMaxUs={} lastHeartbeatUs={} qoeEnabled={} qoeAccessUnitsIn={} qoeKeyframesIn={} qoeDecodedFrames={} qoeDecodeErrors={} qoeFreezeCount={} qoeFirstFrameDelayUs={} qoeMaxFrameGapUs={} qoeOutputFps={:.2f} qoeWidth={} qoeHeight={}",
			trackId_,
			senderSsrc_,
			trackName_,
			"queue_closed",
			sink.writtenAccessUnits(),
			queue_.pushed(),
			queue_.popped(),
			queue_.dropped(),
			queue_.maxDepth(),
			injectedSinkDelayCount_.load(std::memory_order_relaxed),
			injectedSinkDelayTotalMs_.load(std::memory_order_relaxed),
			loopIterations_.load(std::memory_order_relaxed),
			loopGapMaxUs_.load(std::memory_order_relaxed),
			lastHeartbeatUs_.load(std::memory_order_relaxed),
			qoe.enabled,
			qoe.accessUnitsIn,
			qoe.keyframesIn,
			qoe.decodedFrames,
			qoe.decodeErrors,
			qoe.freezeCount,
			qoe.firstFrameDelayUs,
			qoe.maxFrameGapUs,
			qoe.outputFps,
			qoe.width,
			qoe.height);
	}
}

} // namespace webrtc_qos_plain
