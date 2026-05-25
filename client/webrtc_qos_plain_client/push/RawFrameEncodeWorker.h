#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <spdlog/logger.h>

#include "common/BoundedQueue.h"
#include "common/LatestValue.h"
#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Encoder.h"
#include "push/PushSdkTransportThread.h"
#include "push/RawVideoFrame.h"
#include "push/RealtimeH264Source.h"
#include "webrtc_qos/session_config.h"

namespace webrtc_qos_plain {

struct RawFrameEncodeWorkerConfig {
	webrtc_qos::TransportIds ids;
	std::string trackName;
	int width{640};
	int height{360};
	int fps{30};
	int processTickMs{5};
	int injectEncoderDelayMs{0};
	uint32_t startBitrateBps{1200000};
	uint32_t minBitrateBps{300000};
	uint32_t maxBitrateBps{2500000};
};

struct RawFrameEncodeWorkerMetrics {
	bool started{false};
	bool stopped{false};
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	uint64_t queuedAu{0};
	uint64_t enqueueFailures{0};
	uint64_t injectedEncoderDelayCount{0};
	uint64_t injectedEncoderDelayTotalMs{0};
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
	RealtimeH264SourceMetrics sourceMetrics;
};

class RawFrameEncodeWorker {
public:
	RawFrameEncodeWorker(
		RawFrameEncodeWorkerConfig config,
		std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue,
		PushSdkTransportThread* sdkThread,
		std::shared_ptr<spdlog::logger> logger);
	~RawFrameEncodeWorker();

	RawFrameEncodeWorker(const RawFrameEncodeWorker&) = delete;
	RawFrameEncodeWorker& operator=(const RawFrameEncodeWorker&) = delete;

	int Start(std::string* error);
	void Stop();
	void StoreEncoderAdaptation(const webrtc_qos::EncoderAdaptation& adaptation);
	RawFrameEncodeWorkerMetrics metrics() const;
	bool hasFatalError() const;

private:
	void Run();
	bool RecreateEncoder(std::string* error);
	bool ApplyAdaptation(int64_t nowUs, std::string* error);
	bool EncodeFrame(const RawVideoFrame& raw, int64_t nowUs, std::string* error);
	bool CopyRawFrameToEncoderFrame(const RawVideoFrame& raw, std::string* error);
	void RecordForcedKeyframeIfNeeded(int64_t nowUs, bool keyframe);
	void RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs);
	void CompleteStartup(bool ok, int code, const std::string& error);
	void StoreStopReason(const std::string& reason);
	void StoreFatalError(const std::string& error);
	int64_t FrameIntervalUs() const;

	RawFrameEncodeWorkerConfig config_;
	std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue_;
	PushSdkTransportThread* sdkThread_{nullptr};
	std::shared_ptr<spdlog::logger> logger_;
	mediasoup::ffmpeg::Encoder encoder_;
	mediasoup::ffmpeg::FramePtr encoderFrame_;
	LatestValue<webrtc_qos::EncoderAdaptation> adaptation_;
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<uint64_t> queuedAu_{0};
	std::atomic<uint64_t> enqueueFailures_{0};
	std::atomic<uint64_t> injectedEncoderDelayCount_{0};
	std::atomic<uint64_t> injectedEncoderDelayTotalMs_{0};
	std::atomic<uint64_t> loopIterations_{0};
	std::atomic<int64_t> lastHeartbeatUs_{0};
	std::atomic<int64_t> loopGapMaxUs_{0};
	mutable std::mutex metricsMutex_;
	RealtimeH264SourceMetrics sourceMetrics_;
	bool forceKeyframe_{true};
	bool pendingForcedKeyframe_{false};
	int64_t pendingForcedKeyframeRequestUs_{0};
	uint64_t frameIndex_{0};
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
