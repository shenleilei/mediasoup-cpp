#pragma once
#include "Logger.h"
#include <string>
#include <mutex>
#include <thread>
#include <atomic>
#include <hiredis/hiredis.h>

namespace mediasoup {

class RoomRegistry {
public:
	RoomRegistry(const std::string& redisHost, int redisPort,
		const std::string& nodeId, const std::string& nodeAddress)
		: nodeId_(nodeId), nodeAddress_(nodeAddress)
		, logger_(Logger::Get("RoomRegistry"))
	{
		ctx_ = redisConnect(redisHost.c_str(), redisPort);
		if (!ctx_ || ctx_->err) {
			std::string err = ctx_ ? ctx_->errstr : "alloc failed";
			if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
			throw std::runtime_error("Redis connect failed: " + err);
		}
		MS_DEBUG(logger_, "Connected to Redis {}:{}", redisHost, redisPort);
	}

	~RoomRegistry() { stop(); }

	void start() {
		registerNode();
		running_ = true;
		heartbeatThread_ = std::thread([this]() {
			while (running_) {
				std::this_thread::sleep_for(std::chrono::seconds(10));
				if (running_) {
					std::lock_guard<std::mutex> lock(mutex_);
					registerNode();
				}
			}
		});
	}

	void stop() {
		running_ = false;
		if (heartbeatThread_.joinable()) heartbeatThread_.join();
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}

	// Returns "" if room claimed by this node, or redirect address if owned by another.
	std::string claimRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ctx_) throw std::runtime_error("Redis not connected");

		std::string key = "room:" + roomId;
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s NX EX %d",
			key.c_str(), nodeId_.c_str(), roomTTL_);
		if (reply && reply->type == REDIS_REPLY_STATUS) {
			freeReplyObject(reply);
			return "";
		}
		if (reply) freeReplyObject(reply);

		reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
		if (!reply) throw std::runtime_error("Redis GET failed");
		std::string owner = (reply->type == REDIS_REPLY_STRING) ? reply->str : "";
		freeReplyObject(reply);

		if (owner == nodeId_) return "";

		// Check if owner node is still alive; if not, take over the room
		std::string addr = getNodeAddress(owner);
		if (addr.empty()) {
			MS_DEBUG(logger_, "Node {} is dead, taking over room {}", owner, roomId);
			reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
				key.c_str(), nodeId_.c_str(), roomTTL_);
			if (reply) freeReplyObject(reply);
			return "";
		}
		return addr;
	}

	void refreshRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ctx_) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d",
			("room:" + roomId).c_str(), roomTTL_);
		if (reply) freeReplyObject(reply);
	}

	void unregisterRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ctx_) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "DEL %s", ("room:" + roomId).c_str());
		if (reply) freeReplyObject(reply);
	}

	const std::string& nodeId() const { return nodeId_; }
	const std::string& nodeAddress() const { return nodeAddress_; }

private:
	void registerNode() {
		if (!ctx_) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			("node:" + nodeId_).c_str(), nodeAddress_.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
	}

	std::string getNodeAddress(const std::string& nid) {
		if (!ctx_) return "";
		auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", ("node:" + nid).c_str());
		if (!reply) return "";
		std::string addr = (reply->type == REDIS_REPLY_STRING) ? reply->str : "";
		freeReplyObject(reply);
		return addr;
	}

	std::string nodeId_;
	std::string nodeAddress_;
	redisContext* ctx_ = nullptr;
	std::mutex mutex_;
	std::atomic<bool> running_{false};
	std::thread heartbeatThread_;
	int roomTTL_ = 300;
	int nodeTTL_ = 30;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
