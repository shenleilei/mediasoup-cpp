#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <spdlog/logger.h>

#include "common/BoundedQueue.h"
#include "play/AnnexBSink.h"
#include "play/FfmpegDecodeSink.h"
#include "webrtc_qos/session_config.h"
#include "webrtc_qos/status.h"

namespace webrtc_qos_plain {

struct DecodedAccessUnit {
	std::vector<uint8_t> bytes;
	int64_t captureTimeUs{0};
	bool keyframe{false};
	webrtc_qos::TransportIds ids;
};

struct DecodedAuSinkWorkerMetrics {
	bool started{false};
	bool stopped{false};
	int64_t lastHeartbeatUs{0};
	int64_t loopGapMaxUs{0};
	uint64_t loopIterations{0};
	uint64_t enqueuedAccessUnits{0};
	uint64_t droppedAccessUnits{0};
	uint64_t writtenAccessUnits{0};
	uint64_t sinkWriteFailures{0};
	uint64_t injectedSinkDelayCount{0};
	uint64_t injectedSinkDelayTotalMs{0};
	size_t queueDepth{0};
	size_t queueMaxDepth{0};
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	std::string trackName;
	std::string stopReason;
	FfmpegDecodeSinkMetrics qoe;
	std::unordered_map<uint32_t, uint64_t> writtenAccessUnitsByTrack;
	std::unordered_map<uint32_t, uint64_t> enqueuedAccessUnitsByTrack;
};

class DecodedAuSinkWorker {
public:
	DecodedAuSinkWorker(
		bool outputNull,
		std::string outputPath,
		bool decodeQoe,
		std::shared_ptr<spdlog::logger> logger,
		size_t queueCapacity = 64,
		int injectSinkDelayMs = 0,
		uint32_t trackId = 0,
		uint32_t senderSsrc = 0,
		std::string trackName = "");
	~DecodedAuSinkWorker();

	DecodedAuSinkWorker(const DecodedAuSinkWorker&) = delete;
	DecodedAuSinkWorker& operator=(const DecodedAuSinkWorker&) = delete;

	bool Start(std::string* error);
	void Stop();
	webrtc_qos::Status Enqueue(const webrtc_qos::AnnexBAccessUnitView& accessUnit);
	DecodedAuSinkWorkerMetrics metrics() const;

private:
	void Run();
	void StoreStopReason(const std::string& reason);
	void StoreQoeMetrics(const FfmpegDecodeSinkMetrics& metrics);

	bool outputNull_{false};
	std::string outputPath_;
	bool decodeQoe_{false};
	int injectSinkDelayMs_{0};
	uint32_t trackId_{0};
	uint32_t senderSsrc_{0};
	std::string trackName_;
	std::shared_ptr<spdlog::logger> logger_;
	BoundedQueue<DecodedAccessUnit> queue_;
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<int64_t> lastHeartbeatUs_{0};
	std::atomic<int64_t> loopGapMaxUs_{0};
	std::atomic<uint64_t> loopIterations_{0};
	std::atomic<uint64_t> writtenAccessUnits_{0};
	std::atomic<uint64_t> sinkWriteFailures_{0};
	std::atomic<uint64_t> injectedSinkDelayCount_{0};
	std::atomic<uint64_t> injectedSinkDelayTotalMs_{0};
	mutable std::mutex trackMetricsMutex_;
	std::unordered_map<uint32_t, uint64_t> writtenAccessUnitsByTrack_;
	std::unordered_map<uint32_t, uint64_t> enqueuedAccessUnitsByTrack_;
	mutable std::mutex qoeMutex_;
	FfmpegDecodeSinkMetrics qoeMetrics_;
	mutable std::mutex stopReasonMutex_;
	std::string stopReason_;
	std::thread thread_;
};

} // namespace webrtc_qos_plain
