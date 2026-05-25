#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <spdlog/logger.h>

#include "common/ClientArgs.h"
#include "push/PushSignalingSession.h"

namespace webrtc_qos_plain {

class WebRtcQosPushRuntime {
public:
	WebRtcQosPushRuntime(
		PushOptions options,
		PublishInfo publishInfo,
		std::shared_ptr<spdlog::logger> logger);

	int Run(std::atomic<bool>& running, PushSignalingSession* signaling);

private:
	PushOptions options_;
	PublishInfo publishInfo_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace webrtc_qos_plain
