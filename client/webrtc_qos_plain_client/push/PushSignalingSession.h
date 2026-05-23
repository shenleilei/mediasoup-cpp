#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <spdlog/logger.h>

#include "WsClient.h"
#include "common/ClientArgs.h"

namespace webrtc_qos_plain {

struct PublishInfo {
	std::string transportId;
	std::string announcedIp;
	uint16_t port = 0;
	uint8_t payloadType = 0;
	uint32_t ssrc = 0;
	std::string producerId;
	uint8_t transportCcExtId = 0;
};

class PushSignalingSession {
public:
	explicit PushSignalingSession(std::shared_ptr<spdlog::logger> logger);

	bool ConnectAndPublish(const PushOptions& options, PublishInfo* info);
	void DispatchNotifications();
	void Close();

private:
	std::shared_ptr<spdlog::logger> logger_;
	WsClient ws_;
};

} // namespace webrtc_qos_plain
