#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <spdlog/logger.h>

#include "common/LatestValue.h"
#include "common/PlainUdpTransport.h"
#include "webrtc_qos/qos_metrics.h"
#include "webrtc_qos/video_play_client.h"

namespace webrtc_qos_plain {

struct PlaySdkTransportThreadConfig {
	webrtc_qos::SessionConfig session;
	PlainUdpTransport udp;
	std::string mediaRemoteIp;
	uint16_t mediaRemotePort{0};
	std::string logDir;
	int processTickMs{5};
	webrtc_qos::AnnexBAccessUnitCallback decodedAccessUnitOutput;
};

struct PlaySdkTrackMetrics {
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	bool snapshotAvailable{false};
	webrtc_qos::QosSnapshot snapshot;
};

struct PlaySdkTransportThreadMetrics {
	bool started{false};
	bool stopped{false};
	int64_t lastHeartbeatUs{0};
	int64_t loopGapMaxUs{0};
	uint64_t loopIterations{0};
	uint64_t rtpPacketsIn{0};
	uint64_t rtcpPacketsIn{0};
	uint64_t malformedPacketsIn{0};
	uint64_t packetInputFailures{0};
	uint64_t rtcpPacketsOut{0};
	uint64_t rtcpBytesOut{0};
	uint64_t rtcpSendFailures{0};
	UdpEndpoint localEndpoint;
	UdpEndpoint remoteEndpoint;
	std::string stopReason;
	std::string fatalError;
	webrtc_qos::QosSnapshot snapshot;
	std::vector<PlaySdkTrackMetrics> tracks;
};

class PlaySdkTransportThread {
public:
	PlaySdkTransportThread(
		PlaySdkTransportThreadConfig config,
		std::shared_ptr<spdlog::logger> logger);
	~PlaySdkTransportThread();

	PlaySdkTransportThread(const PlaySdkTransportThread&) = delete;
	PlaySdkTransportThread& operator=(const PlaySdkTransportThread&) = delete;

	int Start(std::string* error);
	void Stop();
	PlaySdkTransportThreadMetrics metrics() const;
	bool hasFatalError() const;

private:
	void Run();
	void CompleteStartup(bool ok, int code, const std::string& error);
	void RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs);
	void DrainUdp(
		PlainUdpTransport& udp,
		webrtc_qos::VideoPlayClient& play,
		int64_t nowUs,
		size_t maxPackets);
	void StoreEndpoints(const UdpEndpoint& local, const UdpEndpoint& remote);
	void StoreStopReason(const std::string& reason);
	void StoreFatalError(const std::string& error);

	PlaySdkTransportThreadConfig config_;
	std::shared_ptr<spdlog::logger> logger_;
	std::atomic<bool> running_{false};
	std::atomic<bool> started_{false};
	std::atomic<bool> stopped_{false};
	std::atomic<int64_t> lastHeartbeatUs_{0};
	std::atomic<int64_t> loopGapMaxUs_{0};
	std::atomic<uint64_t> loopIterations_{0};
	std::atomic<uint64_t> rtpPacketsIn_{0};
	std::atomic<uint64_t> rtcpPacketsIn_{0};
	std::atomic<uint64_t> malformedPacketsIn_{0};
	std::atomic<uint64_t> packetInputFailures_{0};
	std::atomic<uint64_t> rtcpPacketsOut_{0};
	std::atomic<uint64_t> rtcpBytesOut_{0};
	std::atomic<uint64_t> rtcpSendFailures_{0};
	LatestValue<webrtc_qos::QosSnapshot> snapshot_;
	std::vector<std::pair<webrtc_qos::TransportIds, std::unique_ptr<LatestValue<webrtc_qos::QosSnapshot>>>> trackSnapshots_;
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
