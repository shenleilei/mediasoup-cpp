#pragma once

#include <atomic>
#include <memory>

#include <spdlog/logger.h>

#include "common/ClientArgs.h"
#include "common/PlainUdpTransport.h"
#include "play/AnnexBSink.h"
#include "play/PlaySignalingSession.h"

namespace webrtc_qos_plain {

class WebRtcQosPlayRuntime {
public:
	WebRtcQosPlayRuntime(
		PlayOptions options,
		ConsumerInfo consumerInfo,
		std::shared_ptr<spdlog::logger> logger,
		PlainUdpTransport udp);

	int Run(std::atomic<bool>& running, PlaySignalingSession* signaling);

private:
	PlayOptions options_;
	ConsumerInfo consumerInfo_;
	std::shared_ptr<spdlog::logger> logger_;
	PlainUdpTransport udp_;
};

} // namespace webrtc_qos_plain
