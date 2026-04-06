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
		, redisHost_(redisHost), redisPort_(redisPort)
		, logger_(Logger::Get("RoomRegistry"))
	{
		if (!reconnect()) {
			throw std::runtime_error("Redis connect failed");
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
				if (!running_) break;
				std::lock_guard<std::mutex> lock(mutex_);
				if (!ensureConnected()) continue;
				registerNode();
			}
		});
	}

	void stop() {
		running_ = false;
		if (heartbeatThread_.joinable()) heartbeatThread_.join();
		std::lock_guard<std::mutex> lock(mutex_);
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}

	std::string claimRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) throw std::runtime_error("Redis not connected");

		std::string key = "room:" + roomId;
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s NX EX %d",
			key.c_str(), nodeId_.c_str(), roomTTL_);
		if (reply && reply->type == REDIS_REPLY_STATUS) {
			freeReplyObject(reply);
			return "";
		}
		if (reply) freeReplyObject(reply);

		reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
		if (!reply) { handleDisconnect(); throw std::runtime_error("Redis GET failed"); }
		std::string owner = (reply->type == REDIS_REPLY_STRING) ? reply->str : "";
		freeReplyObject(reply);

		if (owner == nodeId_) return "";

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
		if (!ensureConnected()) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d",
			("room:" + roomId).c_str(), roomTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	void unregisterRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "DEL %s", ("room:" + roomId).c_str());
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	const std::string& nodeId() const { return nodeId_; }
	const std::string& nodeAddress() const { return nodeAddress_; }

private:
	bool reconnect() {
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
		struct timeval tv = {2, 0}; // 2s connect timeout
		ctx_ = redisConnectWithTimeout(redisHost_.c_str(), redisPort_, tv);
		if (!ctx_ || ctx_->err) {
			std::string err = ctx_ ? ctx_->errstr : "alloc failed";
			MS_ERROR(logger_, "Redis connect failed: {}", err);
			if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
			return false;
		}
		MS_DEBUG(logger_, "Redis reconnected to {}:{}", redisHost_, redisPort_);
		return true;
	}

	bool ensureConnected() {
		if (ctx_ && !ctx_->err) return true;
		return reconnect();
	}

	void handleDisconnect() {
		MS_WARN(logger_, "Redis connection lost, will reconnect on next operation");
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}

	void registerNode() {
		if (!ctx_) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			("node:" + nodeId_).c_str(), nodeAddress_.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	std::string getNodeAddress(const std::string& nid) {
		if (!ctx_) return "";
		auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", ("node:" + nid).c_str());
		if (!reply) { handleDisconnect(); return ""; }
		std::string addr = (reply->type == REDIS_REPLY_STRING) ? reply->str : "";
		freeReplyObject(reply);
		return addr;
	}

	std::string nodeId_;
	std::string nodeAddress_;
	std::string redisHost_;
	int redisPort_;
	redisContext* ctx_ = nullptr;
	std::mutex mutex_;
	std::atomic<bool> running_{false};
	std::thread heartbeatThread_;
	int roomTTL_ = 300;
	int nodeTTL_ = 30;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
