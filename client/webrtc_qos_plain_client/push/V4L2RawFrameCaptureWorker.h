#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include <spdlog/logger.h>

#include "common/BoundedQueue.h"
#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/InputFormat.h"
#include "push/RawVideoFrame.h"
#include "webrtc_qos/session_config.h"

namespace webrtc_qos_plain {

struct V4L2RawFrameCaptureWorkerConfig {
	webrtc_qos::TransportIds ids;
	std::string trackName;
	std::string device{"/dev/video0"};
	int width{640};
	int height{360};
	int fps{30};
	std::string inputFormat;
	int processTickMs{5};
	int openTimeoutMs{3000};
	int readTimeoutMs{1000};
};

struct V4L2RawFrameCaptureWorkerMetrics {
	bool started{false};
	bool stopped{false};
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	uint64_t framesDecoded{0};
	uint64_t queuedFrames{0};
	uint64_t queuePushFailures{0};
	uint64_t loopIterations{0};
	int64_t lastHeartbeatUs{0};
	int64_t loopGapMaxUs{0};
	size_t rawQueueDepth{0};
	size_t rawQueueMaxDepth{0};
	size_t rawQueueDroppedFrames{0};
	size_t rawQueuePushedFrames{0};
	size_t rawQueuePoppedFrames{0};
	std::string stopReason;
	std::string fatalError;
};

class V4L2RawFrameCaptureWorker {
public:
	V4L2RawFrameCaptureWorker(
		V4L2RawFrameCaptureWorkerConfig config,
		std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue,
		std::shared_ptr<spdlog::logger> logger);
	~V4L2RawFrameCaptureWorker();

	V4L2RawFrameCaptureWorker(const V4L2RawFrameCaptureWorker&) = delete;
	V4L2RawFrameCaptureWorker& operator=(const V4L2RawFrameCaptureWorker&) = delete;

	int Start(std::string* error);
	void Stop();
	V4L2RawFrameCaptureWorkerMetrics metrics() const;
	bool hasFatalError() const;

private:
	bool OpenInput(std::string* error);
	bool DecodeNextFrame(std::string* error);
	bool ConvertAndQueueFrame(int64_t nowUs, std::string* error);
	void Run();
	void RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs);
	void CompleteStartup(bool ok, int code, const std::string& error);
	void StoreStopReason(const std::string& reason);
	void StoreFatalError(const std::string& error);
	int64_t FrameIntervalUs() const;

	V4L2RawFrameCaptureWorkerConfig config_;
	std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue_;
	std::shared_ptr<spdlog::logger> logger_;
	std::optional<mediasoup::ffmpeg::InputFormat> input_;
	std::optional<mediasoup::ffmpeg::Decoder> decoder_;
	mediasoup::ffmpeg::PacketPtr packet_;
	mediasoup::ffmpeg::FramePtr decodedFrame_;
	mediasoup::ffmpeg::FramePtr convertedFrame_;
	mediasoup::ffmpeg::SwsContextPtr sws_;
	int videoIndex_{-1};
	uint64_t frameIndex_{0};
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<uint64_t> framesDecoded_{0};
	std::atomic<uint64_t> queuedFrames_{0};
	std::atomic<uint64_t> queuePushFailures_{0};
	std::atomic<uint64_t> loopIterations_{0};
		std::atomic<int64_t> lastHeartbeatUs_{0};
		std::atomic<int64_t> loopGapMaxUs_{0};
		std::atomic<int64_t> interruptDeadlineUs_{0};
	mutable std::mutex metadataMutex_;
	std::string stopReason_;
	std::string fatalError_;
	mutable std::mutex startupMutex_;
	std::condition_variable startupCv_;
	bool startupDone_{false};
	bool startupOk_{false};
	int startupCode_{0};
	std::string startupError_;
	std::thread thread_;
};

} // namespace webrtc_qos_plain
