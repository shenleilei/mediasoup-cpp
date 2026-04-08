#pragma once
#include "Logger.h"
#include "Constants.h"
#include "GeoRouter.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <cmath>
#include <hiredis/hiredis.h>

namespace mediasoup {

class RoomRegistry {
public:
	RoomRegistry(const std::string& redisHost, int redisPort,
		const std::string& nodeId, const std::string& nodeAddress,
		double nodeLat = 0, double nodeLng = 0, const std::string& nodeIsp = "",
		const std::string& nodeCountry = "", bool countryIsolation = false)
		: nodeId_(nodeId), nodeAddress_(nodeAddress)
		, nodeLat_(nodeLat), nodeLng_(nodeLng), nodeIsp_(nodeIsp)
		, nodeCountry_(nodeCountry), countryIsolation_(countryIsolation)
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
		// Format: address|rooms|maxRooms|lat|lng|isp|country
		std::string val = nodeAddress_ + "|" + std::to_string(rooms) + "|" + std::to_string(maxRooms)
			+ "|" + std::to_string(nodeLat_) + "|" + std::to_string(nodeLng_)
			+ "|" + nodeIsp_ + "|" + nodeCountry_;
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

	ResolveResult resolveRoom(const std::string& roomId, const std::string& clientIp = "") {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) return {nodeAddress_, false};

		// Check if room already exists
		auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", ("room:" + roomId).c_str());
		if (reply && reply->type == REDIS_REPLY_STRING) {
			std::string ownerNodeId = reply->str;
			freeReplyObject(reply);
			auto info = parseNodeInfo(ownerNodeId);
			if (!info.address.empty()) return {info.address, false};
			return {nodeAddress_, true};
		}
		if (reply) freeReplyObject(reply);

		// Room doesn't exist — pick best node for this client
		auto best = findBestNode(clientIp);
		if (best.empty()) return {"", true};  // all nodes full
		return {best, true};
	}

	void stop() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}

	std::string claimRoom(const std::string& roomId, const std::string& clientIp = "") {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!ensureConnected()) {
			MS_WARN(logger_, "Redis unavailable, degrading to local-only for room {}", roomId);
			return "";  // degrade: create locally
		}

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

		if (!reply) {
			handleDisconnect();
			MS_WARN(logger_, "Redis EVAL failed for room {}, degrading to local-only", roomId);
			return "";  // degrade: create locally
		}

		std::string result;
		if (reply->type == REDIS_REPLY_STRING) result = reply->str;
		freeReplyObject(reply);

		if (result.empty() || result == "self") {
			// We own this room (new or already ours).
			// For new rooms, check if a geo-better node exists.
			if (result.empty() && geo_ && !clientIp.empty()) {
				std::string best = findBestNode(clientIp);
				if (!best.empty() && best != nodeAddress_) {
					// Better node exists — release our claim and redirect
					auto* del = (redisReply*)redisCommand(ctx_, "DEL %s", roomKey.c_str());
					if (del) freeReplyObject(del);
					return best;
				}
			}
			return "";
		}
		if (result.rfind("taken:", 0) == 0) {
			MS_DEBUG(logger_, "Node {} is dead, taking over room {}", result.substr(6), roomId);
			return "";
		}
		if (result.rfind("addr:", 0) == 0) {
			auto info = parseNodeValue(result.substr(5));
			if (!info.address.empty()) return info.address;
			MS_WARN(logger_, "Owner node value unparseable for room {}, degrading to local-only", roomId);
			return "";  // degrade: create locally
		}

		MS_WARN(logger_, "Unexpected claimRoom result for room {}: {}, degrading to local-only", roomId, result);
		return "";  // degrade: create locally
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

	void setGeoRouter(GeoRouter* geo) { geo_ = geo; }

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
		std::string val = nodeAddress_ + "|0|0|" + std::to_string(nodeLat_) + "|"
			+ std::to_string(nodeLng_) + "|" + nodeIsp_ + "|" + nodeCountry_;
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			("node:" + nodeId_).c_str(), val.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	struct NodeInfo {
		std::string address;
		size_t rooms = 0;
		size_t maxRooms = 0;
		double lat = 0;
		double lng = 0;
		std::string isp;
		std::string country;
	};

	// Parse "address|rooms|maxRooms|lat|lng|isp|country" format
	NodeInfo parseNodeValue(const std::string& val) {
		NodeInfo info;
		std::vector<std::string> parts;
		std::istringstream ss(val);
		std::string token;
		while (std::getline(ss, token, '|')) parts.push_back(token);

		if (parts.empty()) return info;
		info.address = parts[0];
		if (parts.size() > 1) try { info.rooms = std::stoul(parts[1]); } catch (...) {}
		if (parts.size() > 2) try { info.maxRooms = std::stoul(parts[2]); } catch (...) {}
		if (parts.size() > 3) try { info.lat = std::stod(parts[3]); } catch (...) {}
		if (parts.size() > 4) try { info.lng = std::stod(parts[4]); } catch (...) {}
		if (parts.size() > 5) info.isp = parts[5];
		if (parts.size() > 6) info.country = parts[6];
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
		return findBestNode("");
	}

	// Geo-aware node selection: ISP match (China) > distance > load
	std::string findBestNode(const std::string& clientIp) {
		if (!ctx_) return "";
		auto* reply = (redisReply*)redisCommand(ctx_, "KEYS node:*");
		if (!reply || reply->type != REDIS_REPLY_ARRAY) {
			if (reply) freeReplyObject(reply);
			return "";
		}

		// Collect all candidate nodes
		struct Candidate { std::string address; size_t rooms; double score; };
		std::vector<Candidate> candidates;

		GeoInfo clientGeo;
		if (geo_ && !clientIp.empty()) clientGeo = geo_->lookup(clientIp);

		for (size_t i = 0; i < reply->elements; i++) {
			std::string key = reply->element[i]->str;
			auto* vr = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
			if (!vr || vr->type != REDIS_REPLY_STRING) { if (vr) freeReplyObject(vr); continue; }
			auto info = parseNodeValue(vr->str);
			freeReplyObject(vr);
			if (info.address.empty()) continue;
			if (info.maxRooms > 0 && info.rooms >= info.maxRooms) continue;

			// Country isolation: skip nodes in different countries
			if (countryIsolation_ && clientGeo.valid && !clientGeo.country.empty()
				&& !info.country.empty() && info.country != clientGeo.country) continue;

			double s = 0;
			if (clientGeo.valid && (info.lat != 0 || info.lng != 0)) {
				s = geo_->score(clientGeo, info.lat, info.lng, info.isp);
			} else if (clientGeo.valid) {
				// Node has no geo info — deprioritize by giving max score
				s = 99999.0;
			}
			candidates.push_back({info.address, info.rooms, s});
		}
		freeReplyObject(reply);

		if (candidates.empty()) return "";

		if (clientGeo.valid) {
			// Sort by geo score first, then by load as tiebreaker
			std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
				if (std::abs(a.score - b.score) > 100.0) return a.score < b.score; // >100km difference matters
				return a.rooms < b.rooms;
			});
		} else {
			// No geo info — pure load balancing
			std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
				return a.rooms < b.rooms;
			});
		}

		return candidates.front().address;
	}

	std::string getNodeAddress(const std::string& nid) {
		return parseNodeInfo(nid).address;
	}

	std::string nodeId_;
	std::string nodeAddress_;
	double nodeLat_;
	double nodeLng_;
	std::string nodeIsp_;
	std::string nodeCountry_;
	bool countryIsolation_ = false;
	std::string redisHost_;
	int redisPort_;
	redisContext* ctx_ = nullptr;
	std::mutex mutex_;
	int roomTTL_ = kRedisRoomTtlSec;
	int nodeTTL_ = kRedisNodeTtlSec;
	GeoRouter* geo_ = nullptr;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
