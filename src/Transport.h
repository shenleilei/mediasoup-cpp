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
	void setChannelListenerId(uint64_t listenerId) { channelListenerId_ = listenerId; }
	void setContext(std::string roomId, std::string peerId) {
		roomId_ = std::move(roomId);
		peerId_ = std::move(peerId);
	}
	const std::string& roomId() const { return roomId_; }
	const std::string& peerId() const { return peerId_; }
	std::string logPrefix() const {
		if (roomId_.empty() && peerId_.empty()) return "[" + id_ + "]";
		if (peerId_.empty()) return "[" + roomId_ + " " + id_ + "]";
		return "[" + roomId_ + " " + peerId_ + " " + id_ + "]";
	}

	std::shared_ptr<Producer> produce(const json& options);
	std::shared_ptr<Consumer> consume(const json& options);
	json dump(int timeoutMs = kChannelRequestTimeoutMs);
	json getStats(int timeoutMs = kChannelRequestTimeoutMs);

	void close();
	void routerClosed();

	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers() const { return producers_; }
	const std::unordered_map<std::string, std::shared_ptr<Consumer>>& consumers() const { return consumers_; }

protected:
	void cleanupOwnedEntities();
	void emitTerminalClose(const char* reason);

	std::string id_;
	Channel* channel_;
	std::string routerId_;
	std::string roomId_;
	std::string peerId_;
	bool closed_ = false;
	EventEmitter emitter_;
	std::unordered_map<std::string, std::shared_ptr<Producer>> producers_;
	std::unordered_map<std::string, std::shared_ptr<Consumer>> consumers_;
	uint32_t nextMid_ = 0;
	std::shared_ptr<spdlog::logger> logger_;
	uint64_t channelListenerId_{ 0 };
	};

} // namespace mediasoup
