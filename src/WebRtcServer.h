#pragma once

#include "Channel.h"
#include "EventEmitter.h"
#include "Logger.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mediasoup {

class WebRtcServer {
public:
	WebRtcServer(
		const std::string& id,
		Channel* channel,
		const std::vector<nlohmann::json>& listenInfos);

	const std::string& id() const { return id_; }
	bool closed() const { return closed_; }
	uint16_t firstListenPort() const;
	const std::vector<nlohmann::json>& listenInfos() const { return listenInfos_; }
	EventEmitter& emitter() { return emitter_; }

	nlohmann::json dump();
	void close();
	void workerClosed();

private:
	std::string id_;
	Channel* channel_;
	std::vector<nlohmann::json> listenInfos_;
	bool closed_{ false };
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
