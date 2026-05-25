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
#include "common/LatestValue.h"
#include "push/H264AnnexBSource.h"
#include "push/Mp4DecodeH264Source.h"
#include "push/PushSdkTransportThread.h"
#include "push/RawFrameEncodeWorker.h"
#include "push/RawVideoFrame.h"
#include "push/RealtimeH264Source.h"
#include "push/V4L2RawFrameCaptureWorker.h"
#include "webrtc_qos/session_config.h"

namespace webrtc_qos_plain {

enum class PushTrackSourceMode {
	kCopy,
	kSynthetic,
	kMp4DecodeLoop,
	kV4L2,
};

struct PushTrackSourceWorkerConfig {
	webrtc_qos::TransportIds ids;
	std::string trackName;
	PushTrackSourceMode mode{PushTrackSourceMode::kCopy};
	std::string inputPath;
	bool loopInput{false};
	std::string encoder{"copy"};
	int processTickMs{5};
	int syntheticWidth{320};
	int syntheticHeight{180};
	int syntheticFps{15};
	std::string syntheticPattern{"testsrc"};
	std::string v4l2Device{"/dev/video0"};
	int v4l2Width{640};
	int v4l2Height{360};
	int v4l2Fps{30};
	std::string v4l2InputFormat;
	int injectEncoderDelayMs{0};
	uint32_t startBitrateBps{1200000};
	uint32_t minBitrateBps{300000};
	uint32_t maxBitrateBps{2500000};
};

struct PushTrackSourceWorkerMetrics {
	bool started{false};
	bool stopped{false};
	bool eof{false};
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

class PushTrackSourceWorker {
public:
	PushTrackSourceWorker(
		PushTrackSourceWorkerConfig config,
		PushSdkTransportThread* sdkThread,
		std::shared_ptr<spdlog::logger> logger);
	~PushTrackSourceWorker();

	PushTrackSourceWorker(const PushTrackSourceWorker&) = delete;
	PushTrackSourceWorker& operator=(const PushTrackSourceWorker&) = delete;

	int Start(std::string* error);
	void Stop();
	void StoreEncoderAdaptation(const webrtc_qos::EncoderAdaptation& adaptation);
	PushTrackSourceWorkerMetrics metrics() const;
	bool hasFatalError() const;

private:
	void Run();
	bool OpenSource(std::string* error);
	bool ApplyAdaptation(int64_t nowUs, std::string* error);
	bool NextRealtimeAccessUnit(int64_t nowUs, AnnexBAccessUnit* out, std::string* error);
	bool PumpRealtimeSource(int64_t nowUs, std::string* error);
	bool PumpCopySource(int64_t nowUs, std::string* error);
	bool EnqueueAccessUnit(const AnnexBAccessUnit& accessUnit, int64_t captureTimeUs, std::string* error);
	void PublishSourceMetrics();
	void RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs);
	void CompleteStartup(bool ok, int code, const std::string& error);
	void StoreStopReason(const std::string& reason);
	void StoreFatalError(const std::string& error);
	void StopSplitWorkers();

	PushTrackSourceWorkerConfig config_;
	PushSdkTransportThread* sdkThread_{nullptr};
	std::shared_ptr<spdlog::logger> logger_;
	std::optional<H264AnnexBSource> copySource_;
	std::optional<RealtimeH264Source> realtimeSource_;
	std::optional<Mp4DecodeH264Source> mp4DecodeSource_;
	std::shared_ptr<BoundedQueue<RawVideoFrame>> rawQueue_;
	std::unique_ptr<V4L2RawFrameCaptureWorker> v4l2CaptureWorker_;
	std::unique_ptr<RawFrameEncodeWorker> rawEncodeWorker_;
	AnnexBAccessUnit nextCopyAu_;
	bool haveCopyAu_{false};
	bool firstCopyAu_{true};
	int64_t copyStartWallUs_{0};
	int64_t copyFirstMediaUs_{0};
	int64_t realtimeStartWallUs_{0};
	LatestValue<webrtc_qos::EncoderAdaptation> adaptation_;
	LatestValue<RealtimeH264SourceMetrics> sourceMetrics_;
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<bool> eof_{false};
	std::atomic<uint64_t> queuedAu_{0};
	std::atomic<uint64_t> enqueueFailures_{0};
	std::atomic<uint64_t> injectedEncoderDelayCount_{0};
	std::atomic<uint64_t> injectedEncoderDelayTotalMs_{0};
	std::atomic<uint64_t> loopIterations_{0};
	std::atomic<int64_t> lastHeartbeatUs_{0};
	std::atomic<int64_t> loopGapMaxUs_{0};
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

const char* ToString(PushTrackSourceMode mode);

} // namespace webrtc_qos_plain
