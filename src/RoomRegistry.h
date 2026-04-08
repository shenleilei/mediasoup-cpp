#pragma once
#include "Logger.h"
#include "Constants.h"
#include <string>
#include <mutex>
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
	}

	// Called periodically from SignalingServer timer via postWork
	void heartbeat() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return;
		registerNode();
	}

	// Update node load info in Redis (called from heartbeat)
	void updateLoad(size_t rooms, size_t maxRooms) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return;
		std::string val = nodeAddress_ + "|" + std::to_string(rooms) + "|" + std::to_string(maxRooms);
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			("node:" + nodeId_).c_str(), val.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	// Refresh TTL for all active rooms
	void refreshRooms(const std::vector<std::string>& roomIds) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return;
		for (auto& roomId : roomIds) {
			auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d",
				("room:" + roomId).c_str(), roomTTL_);
			if (reply) freeReplyObject(reply);
			else { handleDisconnect(); return; }
		}
	}

	// Resolve: find which node owns a room, or pick least-loaded node for new room
	struct ResolveResult {
		std::string wsUrl;   // target node's WS address
		bool isNew = false;  // true if room doesn't exist yet
	};

	ResolveResult resolveRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return {nodeAddress_, false};

		// Check if room already exists
		auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", ("room:" + roomId).c_str());
		if (reply && reply->type == REDIS_REPLY_STRING) {
			std::string ownerNodeId = reply->str;
			freeReplyObject(reply);
			// Room exists — return owner's address
			auto info = parseNodeInfo(ownerNodeId);
			if (!info.address.empty()) return {info.address, false};
			// Owner node key missing — node is dead or expired, route to self
			return {nodeAddress_, true};
		}
		if (reply) freeReplyObject(reply);

		// Room doesn't exist — pick least-loaded node
		auto best = findLeastLoadedNode();
		return {best.empty() ? nodeAddress_ : best, true};
	}

	void stop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}

	std::string claimRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) throw std::runtime_error("Redis not connected");

		static const char* luaScript = R"LUA(
			local ok = redis.call('SET', KEYS[1], ARGV[1], 'NX', 'EX', ARGV[2])
			if ok then return '' end
			local owner = redis.call('GET', KEYS[1])
			if not owner then return '' end
			if owner == ARGV[1] then return 'self' end
			local node = redis.call('GET', ARGV[3] .. owner)
			if not node then
				redis.call('SET', KEYS[1], ARGV[1], 'EX', ARGV[2])
				return 'taken:' .. owner
			end
			return 'addr:' .. node
		)LUA";

		std::string roomKey = "room:" + roomId;
		std::string nodePrefix = "node:";
		std::string ttlStr = std::to_string(roomTTL_);

		auto* reply = (redisReply*)redisCommand(ctx_,
			"EVAL %s 1 %s %s %s %s",
			luaScript, roomKey.c_str(),
			nodeId_.c_str(), ttlStr.c_str(), nodePrefix.c_str());

		if (!reply) { handleDisconnect(); throw std::runtime_error("Redis EVAL failed"); }

		std::string result;
		if (reply->type == REDIS_REPLY_STRING) result = reply->str;
		freeReplyObject(reply);

		if (result.empty() || result == "self") return "";
		if (result.rfind("taken:", 0) == 0) {
			MS_DEBUG(logger_, "Node {} is dead, taking over room {}", result.substr(6), roomId);
			return "";
		}
		if (result.rfind("addr:", 0) == 0) {
			// Lua returned the node value directly — parse address from it
			auto info = parseNodeValue(result.substr(5));
			if (!info.address.empty()) return info.address;
			MS_WARN(logger_, "Owner node value unparseable for room {}: {}", roomId, result.substr(5));
			throw std::runtime_error("cannot parse room owner address");
		}

		// Unexpected result
		MS_WARN(logger_, "Unexpected claimRoom result for room {}: {}", roomId, result);
		throw std::runtime_error("unexpected claimRoom result");
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
		struct timeval tv = {kRedisConnectTimeoutSec, 0};
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
		// Store address with placeholder load (updated by updateLoad)
		std::string val = nodeAddress_ + "|0|0";
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			("node:" + nodeId_).c_str(), val.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	struct NodeInfo {
		std::string address;
		size_t rooms = 0;
		size_t maxRooms = 0;
	};

	// Parse "address|rooms|maxRooms" format; falls back to plain address for compatibility
	NodeInfo parseNodeValue(const std::string& val) {
		NodeInfo info;
		auto p1 = val.find('|');
		if (p1 == std::string::npos) {
			info.address = val;
			return info;
		}
		info.address = val.substr(0, p1);
		auto p2 = val.find('|', p1 + 1);
		if (p2 != std::string::npos) {
			try { info.rooms = std::stoul(val.substr(p1 + 1, p2 - p1 - 1)); } catch (...) {}
			try { info.maxRooms = std::stoul(val.substr(p2 + 1)); } catch (...) {}
		}
		return info;
	}

	NodeInfo parseNodeInfo(const std::string& nid) {
		if (!ctx_) return {};
		auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", ("node:" + nid).c_str());
		if (!reply) { handleDisconnect(); return {}; }
		NodeInfo info;
		if (reply->type == REDIS_REPLY_STRING)
			info = parseNodeValue(reply->str);
		freeReplyObject(reply);
		return info;
	}

	std::string findLeastLoadedNode() {
		if (!ctx_) return "";
		auto* reply = (redisReply*)redisCommand(ctx_, "KEYS node:*");
		if (!reply || reply->type != REDIS_REPLY_ARRAY) {
			if (reply) freeReplyObject(reply);
			return "";
		}
		std::string bestAddr;
		size_t bestLoad = SIZE_MAX;
		for (size_t i = 0; i < reply->elements; i++) {
			std::string key = reply->element[i]->str;
			auto* vr = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
			if (!vr || vr->type != REDIS_REPLY_STRING) { if (vr) freeReplyObject(vr); continue; }
			auto info = parseNodeValue(vr->str);
			freeReplyObject(vr);
			if (info.address.empty()) continue;
			// Skip nodes at capacity
			if (info.maxRooms > 0 && info.rooms >= info.maxRooms) continue;
			if (info.rooms < bestLoad) {
				bestLoad = info.rooms;
				bestAddr = info.address;
			}
		}
		freeReplyObject(reply);
		return bestAddr;
	}

	std::string getNodeAddress(const std::string& nid) {
		return parseNodeInfo(nid).address;
	}

	std::string nodeId_;
	std::string nodeAddress_;
	std::string redisHost_;
	int redisPort_;
	redisContext* ctx_ = nullptr;
	std::mutex mutex_;
	int roomTTL_ = kRedisRoomTtlSec;
	int nodeTTL_ = kRedisNodeTtlSec;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
