#pragma once
#include "Channel.h"
#include "Constants.h"
#include "RtpTypes.h"
#include "EventEmitter.h"
#include "Logger.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace mediasoup {

class Producer;
class Consumer;

class Transport : public std::enable_shared_from_this<Transport> {
public:
	Transport(const std::string& id, Channel* channel, const std::string& routerId)
		: id_(id), channel_(channel), routerId_(routerId)
		, logger_(Logger::Get("Transport")) {}

	virtual ~Transport() = default;

	const std::string& id() const { return id_; }
	bool closed() const { return closed_; }
	Channel* channel() const { return channel_; }
	const std::string& routerId() const { return routerId_; }
	EventEmitter& emitter() { return emitter_; }

	std::shared_ptr<Producer> produce(const json& options);
	std::shared_ptr<Consumer> consume(const json& options);
	json getStats(int timeoutMs = kChannelRequestTimeoutMs);

	void close();
	void routerClosed();

	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers() const { return producers_; }
	const std::unordered_map<std::string, std::shared_ptr<Consumer>>& consumers() const { return consumers_; }

protected:
	std::string id_;
	Channel* channel_;
	std::string routerId_;
	bool closed_ = false;
	EventEmitter emitter_;
	std::unordered_map<std::string, std::shared_ptr<Producer>> producers_;
	std::unordered_map<std::string, std::shared_ptr<Consumer>> consumers_;
	uint32_t nextMid_ = 0;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
