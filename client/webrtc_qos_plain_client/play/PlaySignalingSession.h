#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <spdlog/logger.h>

#include "WsClient.h"
#include "common/ClientArgs.h"

namespace webrtc_qos_plain {

struct ConsumerInfo {
	std::string peerId;
	std::string producerId;
	std::string consumerId;
	std::string transportId;
	uint16_t plainTransportPort = 0;
	std::string announcedIp;
	uint8_t payloadType = 0;
	uint32_t ssrc = 0;
	uint8_t transportCcExtId = 0;
	json raw = json::object();
};

class PlaySignalingSession {
public:
	explicit PlaySignalingSession(std::shared_ptr<spdlog::logger> logger);

	bool ConnectJoinAndSubscribe(const PlayOptions& options);
	std::optional<ConsumerInfo> TakeSelectedConsumer(const PlayOptions& options);
	std::vector<ConsumerInfo> TakeSelectedConsumers(const PlayOptions& options, size_t maxConsumers);
	void DispatchNotifications();
	bool RequestConsumerKeyFrame(const std::string& consumerId);
	void Close();

	const std::string& transportId() const { return transportId_; }
	uint16_t plainTransportPort() const { return plainTransportPort_; }
	const std::string& announcedIp() const { return announcedIp_; }

private:
	std::optional<ConsumerInfo> TryParseConsumer(
		const json& consumer,
		const PlayOptions& options,
		const std::string& transportId,
		uint16_t plainTransportPort,
		const std::string& announcedIp);

	std::shared_ptr<spdlog::logger> logger_;
	WsClient ws_;
	std::deque<json> pendingConsumers_;
	std::string transportId_;
	uint16_t plainTransportPort_{0};
	std::string announcedIp_;
};

json BuildMinimalPlainReceiveCapabilities();

} // namespace webrtc_qos_plain
