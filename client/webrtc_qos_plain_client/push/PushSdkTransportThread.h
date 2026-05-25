#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <spdlog/logger.h>

#include "common/BoundedQueue.h"
#include "common/LatestValue.h"
#include "common/PlainUdpTransport.h"
#include "push/EncodedAccessUnit.h"
#include "webrtc_qos/qos_metrics.h"
#include "webrtc_qos/video_push_client.h"

namespace webrtc_qos_plain {

struct PushSdkTransportThreadConfig {
	webrtc_qos::SessionConfig session;
	std::string mediaRemoteIp;
	uint16_t mediaRemotePort{0};
	std::string logDir;
	int processTickMs{5};
	size_t encodedQueueCapacity{128};
};

struct PushSdkTrackQueueMetrics {
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	size_t queueDepth{0};
	size_t queueMaxDepth{0};
	uint64_t enqueuedAccessUnits{0};
	uint64_t droppedAccessUnits{0};
	uint64_t pushedAccessUnits{0};
	uint64_t pushFailures{0};
	bool adaptationAvailable{false};
	bool snapshotAvailable{false};
	webrtc_qos::EncoderAdaptation adaptation;
	webrtc_qos::QosSnapshot snapshot;
};

struct PushSdkTransportThreadMetrics {
	bool started{false};
	bool stopped{false};
	int64_t lastHeartbeatUs{0};
	int64_t loopGapMaxUs{0};
	uint64_t loopIterations{0};
	uint64_t enqueuedAccessUnits{0};
	uint64_t droppedAccessUnits{0};
	uint64_t pushedAccessUnits{0};
	uint64_t pushFailures{0};
	uint64_t rtcpPacketsIn{0};
	uint64_t rtcpBytesIn{0};
	uint64_t rtcpFailures{0};
	uint64_t unexpectedRtpPacketsIn{0};
	uint64_t malformedPacketsIn{0};
	size_t queueDepth{0};
	size_t queueMaxDepth{0};
	UdpEndpoint localEndpoint;
	UdpEndpoint remoteEndpoint;
	std::string stopReason;
	std::string fatalError;
	webrtc_qos::EncoderAdaptation adaptation;
	webrtc_qos::QosSnapshot snapshot;
	std::vector<PushSdkTrackQueueMetrics> tracks;
};

class PushSdkTransportThread {
public:
	PushSdkTransportThread(
		PushSdkTransportThreadConfig config,
		std::shared_ptr<spdlog::logger> logger);
	~PushSdkTransportThread();

	PushSdkTransportThread(const PushSdkTransportThread&) = delete;
	PushSdkTransportThread& operator=(const PushSdkTransportThread&) = delete;

	int Start(std::string* error);
	void Stop();
	webrtc_qos::Status Enqueue(EncodedAccessUnit item);
	webrtc_qos::EncoderAdaptation encoderAdaptation() const;
	PushSdkTransportThreadMetrics metrics() const;
	bool hasFatalError() const;

private:
	struct TrackQueueState;

	void Run();
	void CompleteStartup(bool ok, int code, const std::string& error);
	void RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs);
	void DrainUdpFeedback(
		PlainUdpTransport& udp,
		webrtc_qos::VideoPushClient& push,
		int64_t nowUs,
		size_t maxPackets);
	bool DrainEncodedQueues(
		webrtc_qos::VideoPushClient& push,
		size_t maxItems,
		size_t* drained);
	bool PushAccessUnit(
		webrtc_qos::VideoPushClient& push,
		TrackQueueState& track,
		EncodedAccessUnit item);
	void PublishSdkSnapshots(
		const webrtc_qos::VideoPushClient& push,
		int64_t nowUs);
	TrackQueueState* FindTrackQueue(uint32_t trackId);
	const TrackQueueState* FindTrackQueue(uint32_t trackId) const;
	void CloseTrackQueues();
	void StoreEndpoints(const UdpEndpoint& local, const UdpEndpoint& remote);
	void StoreStopReason(const std::string& reason);
	void StoreFatalError(const std::string& error);

	PushSdkTransportThreadConfig config_;
	std::shared_ptr<spdlog::logger> logger_;
	std::vector<std::unique_ptr<TrackQueueState>> trackQueues_;
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<int64_t> lastHeartbeatUs_{0};
	std::atomic<int64_t> loopGapMaxUs_{0};
	std::atomic<uint64_t> loopIterations_{0};
	std::atomic<uint64_t> pushedAccessUnits_{0};
	std::atomic<uint64_t> pushFailures_{0};
	std::atomic<uint64_t> rtcpPacketsIn_{0};
	std::atomic<uint64_t> rtcpBytesIn_{0};
	std::atomic<uint64_t> rtcpFailures_{0};
	std::atomic<uint64_t> unexpectedRtpPacketsIn_{0};
	std::atomic<uint64_t> malformedPacketsIn_{0};
	LatestValue<webrtc_qos::EncoderAdaptation> adaptation_;
	LatestValue<webrtc_qos::QosSnapshot> snapshot_;
	size_t nextDrainIndex_{0};
	mutable std::mutex metadataMutex_;
	UdpEndpoint localEndpoint_;
	UdpEndpoint remoteEndpoint_;
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
