#include "push/V4L2RawFrameCaptureWorker.h"

extern "C" {
#include <libavdevice/avdevice.h>
#include <libavutil/dict.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <chrono>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <utility>

#include "common/RuntimeLogHelpers.h"

namespace webrtc_qos_plain {
namespace msff = mediasoup::ffmpeg;
namespace {

int ClampFps(int value)
{
	return std::max(1, std::min(value, 60));
}

int EvenAtLeast16(int value)
{
	value = std::max(16, value);
	return value % 2 == 0 ? value : value + 1;
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
	av_dict_set(&opts, "fflags", "nobuffer", 0);
	SetDict(&opts, "input_format", inputFormat);
	return DictionaryPtr(opts);
}

msff::InputFormat OpenV4L2Input(
	const std::string& device,
	const AVInputFormat* v4l2,
	int width,
	int height,
	int fps,
	const std::string& inputFormat,
	msff::InputInterruptCallback interrupt)
{
	auto opts = MakeV4L2Options(width, height, fps, inputFormat);
	AVDictionary* rawOpts = opts.release();
	try {
		auto input = msff::InputFormat::OpenWithFormatInterruptible(
			device,
			v4l2,
			&rawOpts,
			std::move(interrupt));
		av_dict_free(&rawOpts);
		return input;
	} catch (...) {
		av_dict_free(&rawOpts);
		throw;
	}
}

void CopyPlane(uint8_t* dst, const uint8_t* src, int srcStride, int width, int height)
{
	for (int y = 0; y < height; ++y)
		std::memcpy(dst + static_cast<size_t>(y) * static_cast<size_t>(width),
			src + static_cast<size_t>(y) * static_cast<size_t>(srcStride),
			static_cast<size_t>(width));
}

} // namespace

V4L2RawFrameCaptureWorker::V4L2RawFrameCaptureWorker(
	V4L2RawFrameCaptureWorkerConfig config,
	std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue,
	std::shared_ptr<spdlog::logger> logger)
	: config_(std::move(config)),
	  rawQueue_(std::move(rawQueue)),
	  logger_(std::move(logger))
{
}

V4L2RawFrameCaptureWorker::~V4L2RawFrameCaptureWorker()
{
	Stop();
}

int V4L2RawFrameCaptureWorker::Start(std::string* error)
{
	if (!rawQueue_) {
		if (error) *error = "V4L2 capture worker missing raw queue";
		return 2;
	}
	if (running_.exchange(true)) return 0;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	framesDecoded_.store(0, std::memory_order_relaxed);
	queuedFrames_.store(0, std::memory_order_relaxed);
	queuePushFailures_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
	interruptDeadlineUs_.store(0, std::memory_order_relaxed);
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

void V4L2RawFrameCaptureWorker::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	interruptDeadlineUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	if (thread_.joinable()) thread_.join();
}

V4L2RawFrameCaptureWorkerMetrics V4L2RawFrameCaptureWorker::metrics() const
{
	V4L2RawFrameCaptureWorkerMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.trackId = config_.ids.track_id;
	out.senderSsrc = config_.ids.sender_ssrc;
	out.framesDecoded = framesDecoded_.load(std::memory_order_relaxed);
	out.queuedFrames = queuedFrames_.load(std::memory_order_relaxed);
	out.queuePushFailures = queuePushFailures_.load(std::memory_order_relaxed);
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
		std::lock_guard<std::mutex> lock(metadataMutex_);
		out.stopReason = stopReason_;
		out.fatalError = fatalError_;
	}
	return out;
}

bool V4L2RawFrameCaptureWorker::hasFatalError() const
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	return !fatalError_.empty();
}

void V4L2RawFrameCaptureWorker::Run()
{
	std::string error;
	config_.width = EvenAtLeast16(config_.width);
	config_.height = EvenAtLeast16(config_.height);
	config_.fps = ClampFps(config_.fps);
	config_.openTimeoutMs = std::max(1, config_.openTimeoutMs);
	config_.readTimeoutMs = std::max(1, config_.readTimeoutMs);
	if (!OpenInput(&error)) {
		StoreFatalError(error);
		StoreStopReason("v4l2_open_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 2, error);
		if (rawQueue_) rawQueue_->Close();
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	if (logger_) {
		logger_->info(
			"v4l2_capture_worker_started trackId={} senderSsrc={} trackName={} device={} width={} height={} fps={} inputFormat={} openTimeoutMs={} readTimeoutMs={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			config_.trackName,
			config_.device,
			config_.width,
			config_.height,
			config_.fps,
			config_.inputFormat,
			config_.openTimeoutMs,
			config_.readTimeoutMs);
	}
	CompleteStartup(true, 0, "");

	int64_t lastLoopUs = MonotonicNowUs();
	while (running_.load(std::memory_order_relaxed)) {
		const int64_t nowUs = MonotonicNowUs();
		RecordLoopTick(nowUs, &lastLoopUs);
		error.clear();
		if (!DecodeNextFrame(&error)) {
			if (!error.empty()) {
				StoreFatalError(error);
				if (logger_) {
					logger_->error(
						"v4l2_capture_worker_failed trackId={} senderSsrc={} device={} error={}",
						config_.ids.track_id,
						config_.ids.sender_ssrc,
						config_.device,
						error);
				}
			}
			break;
		}
		if (!ConvertAndQueueFrame(MonotonicNowUs(), &error)) {
			queuePushFailures_.fetch_add(1, std::memory_order_relaxed);
			StoreFatalError(error);
			break;
		}
	}

	if (rawQueue_) rawQueue_->Close();
	StoreStopReason(hasFatalError() ? "fatal_error" : "stopped");
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto finalMetrics = metrics();
		logger_->info(
			"v4l2_capture_worker_stopped trackId={} senderSsrc={} device={} framesDecoded={} queuedFrames={} rawQueueDroppedFrames={} rawQueueMaxDepth={} loopIterations={} loopGapMaxUs={} stopReason={} fatalError={}",
			config_.ids.track_id,
			config_.ids.sender_ssrc,
			config_.device,
			finalMetrics.framesDecoded,
			finalMetrics.queuedFrames,
			finalMetrics.rawQueueDroppedFrames,
			finalMetrics.rawQueueMaxDepth,
			finalMetrics.loopIterations,
			finalMetrics.loopGapMaxUs,
			finalMetrics.stopReason,
			finalMetrics.fatalError);
	}
}

bool V4L2RawFrameCaptureWorker::OpenInput(std::string* error)
{
	try {
		avdevice_register_all();
		const AVInputFormat* v4l2 = av_find_input_format("v4l2");
		if (!v4l2) throw std::runtime_error("v4l2 input format not available");
		interruptDeadlineUs_.store(
			MonotonicNowUs() + static_cast<int64_t>(config_.openTimeoutMs) * 1000,
			std::memory_order_relaxed);
		auto interrupt = [this]() {
			if (!running_.load(std::memory_order_relaxed)) return true;
			const int64_t deadlineUs = interruptDeadlineUs_.load(std::memory_order_relaxed);
			return deadlineUs > 0 && MonotonicNowUs() >= deadlineUs;
		};

		try {
			input_.emplace(OpenV4L2Input(
				config_.device,
				v4l2,
				config_.width,
				config_.height,
				config_.fps,
				config_.inputFormat,
				interrupt));
		} catch (...) {
			if (config_.inputFormat.empty()) throw;
			input_.emplace(OpenV4L2Input(
				config_.device,
				v4l2,
				config_.width,
				config_.height,
				config_.fps,
				"",
				interrupt));
		}
		input_->FindStreamInfo();
		interruptDeadlineUs_.store(0, std::memory_order_relaxed);
		videoIndex_ = input_->FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		if (videoIndex_ < 0) throw std::runtime_error("v4l2 input has no video stream");
		auto* stream = input_->StreamAt(videoIndex_);
		if (!stream || !stream->codecpar) throw std::runtime_error("invalid v4l2 video stream");
		decoder_.emplace(msff::Decoder::OpenFromParameters(stream->codecpar));
		packet_ = msff::MakePacket();
		decodedFrame_ = msff::MakeFrame();
		if (!packet_ || !decodedFrame_) throw std::runtime_error("av packet/frame alloc failed");
		return true;
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		interruptDeadlineUs_.store(0, std::memory_order_relaxed);
		input_.reset();
		decoder_.reset();
		videoIndex_ = -1;
		return false;
	}
}

bool V4L2RawFrameCaptureWorker::DecodeNextFrame(std::string* error)
{
	try {
		while (running_.load(std::memory_order_relaxed)) {
			if (decoder_ && decoder_->ReceiveFrame(decodedFrame_.get())) {
				framesDecoded_.fetch_add(1, std::memory_order_relaxed);
				return true;
			}
			msff::PacketUnref(packet_.get());
			interruptDeadlineUs_.store(
				MonotonicNowUs() + static_cast<int64_t>(config_.readTimeoutMs) * 1000,
				std::memory_order_relaxed);
			if (!input_->ReadPacket(packet_.get())) {
				interruptDeadlineUs_.store(0, std::memory_order_relaxed);
				if (error) *error = "v4l2 input ended";
				return false;
			}
			interruptDeadlineUs_.store(0, std::memory_order_relaxed);
			if (packet_->stream_index != videoIndex_) continue;
			(void)decoder_->SendPacket(packet_.get());
		}
		return false;
	} catch (const std::exception& e) {
		interruptDeadlineUs_.store(0, std::memory_order_relaxed);
		if (error) *error = e.what();
		return false;
	}
}

bool V4L2RawFrameCaptureWorker::ConvertAndQueueFrame(int64_t nowUs, std::string* error)
{
	if (!decodedFrame_) {
		if (error) *error = "missing decoded frame";
		return false;
	}
	AVFrame* frame = decodedFrame_.get();
	if (frame->width <= 0 || frame->height <= 0 ||
		frame->format == AV_PIX_FMT_NONE || !frame->data[0]) {
		if (error) *error = "decoded frame is incomplete";
		return false;
	}
	if (frame->format != AV_PIX_FMT_YUV420P || frame->width != config_.width || frame->height != config_.height) {
		if (!convertedFrame_) {
			convertedFrame_ = msff::MakeFrame();
			if (!convertedFrame_) {
				if (error) *error = "av_frame_alloc failed";
				return false;
			}
			convertedFrame_->format = AV_PIX_FMT_YUV420P;
			convertedFrame_->width = config_.width;
			convertedFrame_->height = config_.height;
			msff::FrameGetBuffer(convertedFrame_.get(), 32);
		}
		msff::FrameMakeWritable(convertedFrame_.get());
		sws_.reset(sws_getCachedContext(
			sws_.release(),
			frame->width,
			frame->height,
			static_cast<AVPixelFormat>(frame->format),
			config_.width,
			config_.height,
			AV_PIX_FMT_YUV420P,
			SWS_BILINEAR,
			nullptr,
			nullptr,
			nullptr));
		if (!sws_) {
			if (error) *error = "sws_getCachedContext failed";
			return false;
		}
		sws_scale(
			sws_.get(),
			frame->data,
			frame->linesize,
			0,
			frame->height,
			convertedFrame_->data,
			convertedFrame_->linesize);
		frame = convertedFrame_.get();
	}

	RawVideoFrame raw;
	raw.width = config_.width;
	raw.height = config_.height;
	raw.captureTimeUs = nowUs;
	raw.mediaTimeUs = static_cast<int64_t>(frameIndex_) * FrameIntervalUs();
	raw.frameIndex = frameIndex_++;
	raw.yuv420p.resize(RawVideoFrameSize(raw.width, raw.height));
	const size_t ySize = static_cast<size_t>(raw.width) * static_cast<size_t>(raw.height);
	const size_t uvSize = ySize / 4;
	CopyPlane(raw.yuv420p.data(), frame->data[0], frame->linesize[0], raw.width, raw.height);
	CopyPlane(raw.yuv420p.data() + ySize, frame->data[1], frame->linesize[1], raw.width / 2, raw.height / 2);
	CopyPlane(raw.yuv420p.data() + ySize + uvSize, frame->data[2], frame->linesize[2], raw.width / 2, raw.height / 2);
	av_frame_unref(decodedFrame_.get());

	if (!rawQueue_->PushDropOldest(std::move(raw))) {
		if (error) *error = "raw frame queue is closed";
		return false;
	}
	queuedFrames_.fetch_add(1, std::memory_order_relaxed);
	return true;
}

void V4L2RawFrameCaptureWorker::RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs)
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

void V4L2RawFrameCaptureWorker::CompleteStartup(bool ok, int code, const std::string& error)
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

void V4L2RawFrameCaptureWorker::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	stopReason_ = reason;
}

void V4L2RawFrameCaptureWorker::StoreFatalError(const std::string& error)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	fatalError_ = error;
}

int64_t V4L2RawFrameCaptureWorker::FrameIntervalUs() const
{
	return 1000000 / std::max(1, config_.fps);
}

} // namespace webrtc_qos_plain
