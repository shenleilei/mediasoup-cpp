#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <spdlog/logger.h>

#include "common/ClientArgs.h"
#include "common/PlainUdpTransport.h"
#include "push/H264AnnexBSource.h"
#include "push/PushSignalingSession.h"
#include "webrtc_qos/video_push_client.h"

namespace webrtc_qos_plain {

class WebRtcQosPushRuntime {
public:
	WebRtcQosPushRuntime(
		PushOptions options,
		PublishInfo publishInfo,
		std::shared_ptr<spdlog::logger> logger);

	int Run(std::atomic<bool>& running, PushSignalingSession* signaling);

private:
	struct FeedbackCounters {
		uint64_t rtcpPacketsIn{ 0 };
		uint64_t rtcpBytesIn{ 0 };
		uint64_t rtcpFailures{ 0 };
	};

	bool DrainUdpFeedback(
		PlainUdpTransport& udp,
		webrtc_qos::VideoPushClient& push,
		int64_t nowUs,
		FeedbackCounters& counters);

	PushOptions options_;
	PublishInfo publishInfo_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace webrtc_qos_plain
