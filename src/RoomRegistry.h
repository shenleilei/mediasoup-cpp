#pragma once
#include "Logger.h"
#include "Constants.h"
#include "GeoRouter.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <mutex>
#include <thread>
#include <atomic>
#include <cmath>
#include <unordered_map>
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
		syncAll();
		startSubscriber();
	}

	void heartbeat() {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) return;
		registerNode();
		// Lightweight stale-node eviction: check if cached nodes still exist in Redis.
		// Much cheaper than full syncAll (no KEYS room:*, no room GET).
		evictDeadNodes();
	}

	void updateLoad(size_t rooms, size_t maxRooms) {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) return;
		std::string val = buildNodeValue(rooms, maxRooms);
		auto* reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d",
			(std::string(kKeyPrefixNode) + nodeId_).c_str(), val.c_str(), nodeTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
		// Update local cache for self
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			nodeCache_[nodeId_] = parseNodeValue(val);
		}
		// Publish actual load, not zeros
		if (ctx_) {
			std::string msg = nodeId_ + "=" + val;
			auto* pr = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s", kChannelNodes, msg.c_str());
			if (pr) freeReplyObject(pr);
		}
	}

	void refreshRooms(const std::vector<std::string>& roomIds) {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) return;
		for (auto& roomId : roomIds) {
			auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d",
				(std::string(kKeyPrefixRoom) + roomId).c_str(), roomTTL_);
			if (reply) freeReplyObject(reply);
			else { handleDisconnect(); return; }
		}
	}

	struct ResolveResult {
		std::string wsUrl;
		bool isNew = false;
	};

	ResolveResult resolveRoom(const std::string& roomId, const std::string& clientIp = "") {
		// Check local room cache first (no lock ordering issue — cacheMutex_ only)
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			auto it = roomCache_.find(roomId);
			if (it != roomCache_.end() && !it->second.empty())
				return {it->second, false};
		}

		// Cache miss — check Redis. Lock order: cmdMutex_ first, cacheMutex_ inside.
		{
			std::lock_guard<std::mutex> lock(cmdMutex_);
			if (ensureConnected()) {
				auto* reply = (redisReply*)redisCommand(ctx_, "GET %s", (std::string(kKeyPrefixRoom) + roomId).c_str());
				if (reply && reply->type == REDIS_REPLY_STRING) {
					std::string ownerNodeId = reply->str;
					freeReplyObject(reply);
					// Try local cache first for node address
					std::string addr;
					{
						std::lock_guard<std::mutex> cl(cacheMutex_);
						auto nit = nodeCache_.find(ownerNodeId);
						if (nit != nodeCache_.end()) addr = nit->second.address;
					}
					// Cache miss for node — fall back to Redis GET
					if (addr.empty()) {
						auto* nr = (redisReply*)redisCommand(ctx_, "GET %s", (std::string(kKeyPrefixNode) + ownerNodeId).c_str());
						if (nr && nr->type == REDIS_REPLY_STRING) {
							auto info = parseNodeValue(nr->str);
							addr = info.address;
							if (!addr.empty()) {
								std::lock_guard<std::mutex> cl(cacheMutex_);
								nodeCache_[ownerNodeId] = info;
							}
						}
						if (nr) freeReplyObject(nr);
					}
					if (!addr.empty()) {
						std::lock_guard<std::mutex> cl(cacheMutex_);
						roomCache_[roomId] = addr;
						return {addr, false};
					}
					// Owner node truly gone — pick best available node, not just self
				}
				if (reply) freeReplyObject(reply);
			}
		}

		// Room doesn't exist — pick best node from cache
		auto best = findBestNodeCached(clientIp);
		if (best.empty()) {
			// Cache miss — node may have just registered. Sync nodes from Redis and retry.
			std::lock_guard<std::mutex> lock(cmdMutex_);
			syncNodesUnlocked();
			best = findBestNodeCached(clientIp);
		}
		if (best.empty()) return {"", true};
		return {best, true};
	}

	std::string claimRoom(const std::string& roomId, const std::string& clientIp = "") {
		// Check local room cache first (read-only, quick check)
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			auto it = roomCache_.find(roomId);
			if (it != roomCache_.end() && !it->second.empty()) {
				if (it->second == nodeAddress_) return "";
				return it->second;
			}
		}

		// Cache miss — go to Redis. Lock order: cmdMutex_ first, cacheMutex_ inside.
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) {
			MS_WARN(logger_, "Redis unavailable, degrading to local-only for room {}", roomId);
			return "";
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

		std::string roomKey = std::string(kKeyPrefixRoom) + roomId;
		std::string nodePrefix = kKeyPrefixNode;
		std::string ttlStr = std::to_string(roomTTL_);

		auto* reply = (redisReply*)redisCommand(ctx_,
			"EVAL %s 1 %s %s %s %s",
			luaScript, roomKey.c_str(),
			nodeId_.c_str(), ttlStr.c_str(), nodePrefix.c_str());

		if (!reply) {
			handleDisconnect();
			MS_WARN(logger_, "Redis EVAL failed for room {}, degrading to local-only", roomId);
			return "";
		}

		std::string result;
		if (reply->type == REDIS_REPLY_STRING) result = reply->str;
		freeReplyObject(reply);

		if (result.empty() || result == "self") {
			// New room claimed by us. Check if another node is better
			// (geo-optimal or less loaded).
			if (result.empty()) {
				std::string best = findBestNodeCached(clientIp);
				if (!best.empty() && best != nodeAddress_) {
					auto* del = (redisReply*)redisCommand(ctx_, "DEL %s", roomKey.c_str());
					if (del) freeReplyObject(del);
					return best;
				}
			}
			// Update cache + publish
			{
				std::lock_guard<std::mutex> cl(cacheMutex_);
				roomCache_[roomId] = nodeAddress_;
			}
			publishRoomUpdate(roomId, nodeAddress_);
			return "";
		}
		if (result.rfind("taken:", 0) == 0) {
			MS_DEBUG(logger_, "Node {} is dead, taking over room {}", result.substr(6), roomId);
			{
				std::lock_guard<std::mutex> cl(cacheMutex_);
				roomCache_[roomId] = nodeAddress_;
			}
			publishRoomUpdate(roomId, nodeAddress_);
			return "";
		}
		if (result.rfind("addr:", 0) == 0) {
			auto info = parseNodeValue(result.substr(5));
			if (!info.address.empty()) {
				std::lock_guard<std::mutex> cl(cacheMutex_);
				roomCache_[roomId] = info.address;
				return info.address;
			}
			MS_WARN(logger_, "Owner node value unparseable for room {}, degrading to local-only", roomId);
			return "";
		}

		MS_WARN(logger_, "Unexpected claimRoom result for room {}: {}, degrading to local-only", roomId, result);
		return "";
	}

	void refreshRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d",
			(std::string(kKeyPrefixRoom) + roomId).c_str(), roomTTL_);
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	void unregisterRoom(const std::string& roomId) {
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			roomCache_.erase(roomId);
		}
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (!ensureConnected()) return;
		auto* reply = (redisReply*)redisCommand(ctx_, "DEL %s", (std::string(kKeyPrefixRoom) + roomId).c_str());
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
		publishRoomUpdate(roomId, "");
	}

	const std::string& nodeId() const { return nodeId_; }
	const std::string& nodeAddress() const { return nodeAddress_; }
	void setGeoRouter(GeoRouter* geo) { geo_ = geo; }

	void stop() {
		subStop_ = true;
		// Unregister this node from Redis before shutting down
		{
			std::lock_guard<std::mutex> lock(cmdMutex_);
			if (ctx_ && !ctx_->err) {
				auto* reply = (redisReply*)redisCommand(ctx_, "DEL %s", (std::string(kKeyPrefixNode) + nodeId_).c_str());
				if (reply) freeReplyObject(reply);
				// Publish node removal so other nodes evict from cache
				std::string msg = nodeId_ + "=";
				reply = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s", kChannelNodes, msg.c_str());
				if (reply) freeReplyObject(reply);
			}
			if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
		}
		if (subThread_.joinable()) subThread_.join();
	}

private:
	// ── Redis connection (command context) ──

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

	// ── Node value format ──

	std::string buildNodeValue(size_t rooms, size_t maxRooms) {
		return nodeAddress_ + "|" + std::to_string(rooms) + "|" + std::to_string(maxRooms)
			+ "|" + std::to_string(nodeLat_) + "|" + std::to_string(nodeLng_)
			+ "|" + nodeIsp_ + "|" + nodeCountry_;
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

	// ── Registration ──

	void registerNode() {
		if (!ctx_) return;
		std::string key = std::string(kKeyPrefixNode) + nodeId_;
		// Try EXPIRE first (preserve existing value). If key doesn't exist, SET with initial value.
		auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d", key.c_str(), nodeTTL_);
		if (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
			// Key doesn't exist — first registration
			freeReplyObject(reply);
			std::string val = buildNodeValue(0, 0);
			reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d", key.c_str(), val.c_str(), nodeTTL_);
		}
		if (reply) freeReplyObject(reply);
		else handleDisconnect();
	}

	// ── Pub/Sub ──

	static constexpr const char* kChannelNodes = "sfu:ch:nodes";
	static constexpr const char* kChannelRooms = "sfu:ch:rooms";
	static constexpr const char* kKeyPrefixRoom = "sfu:room:";
	static constexpr const char* kKeyPrefixNode = "sfu:node:";

	void publishRoomUpdate(const std::string& roomId, const std::string& ownerAddr) {
		if (!ctx_) return;
		// Message: "roomId=ownerAddr" (empty ownerAddr = room removed)
		std::string msg = roomId + "=" + ownerAddr;
		auto* reply = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s",
			kChannelRooms, msg.c_str());
		if (reply) freeReplyObject(reply);
	}

	void startSubscriber() {
		subStop_ = false;
		subThread_ = std::thread([this] { subscriberLoop(); });
	}

	void subscriberLoop() {
		while (!subStop_) {
			// Create a dedicated connection for subscribe (blocking)
			struct timeval tv = {kRedisConnectTimeoutSec, 0};
			redisContext* sub = redisConnectWithTimeout(redisHost_.c_str(), redisPort_, tv);
			if (!sub || sub->err) {
				if (sub) redisFree(sub);
				MS_WARN(logger_, "Subscriber connect failed, retrying in 2s");
				for (int i = 0; i < 20 && !subStop_; i++)
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			// Set read timeout so we can check subStop_ periodically
			struct timeval rtv = {2, 0};
			redisSetTimeout(sub, rtv);

			auto* reply = (redisReply*)redisCommand(sub, "SUBSCRIBE %s %s",
				kChannelNodes, kChannelRooms);
			if (reply) freeReplyObject(reply);
			else { redisFree(sub); continue; }

			MS_DEBUG(logger_, "Subscriber connected");

			// On (re)connect, do a full sync to catch anything missed
			syncAll();

			while (!subStop_) {
				redisReply* msg = nullptr;
				int rc = redisGetReply(sub, (void**)&msg);
				if (rc != REDIS_OK || !msg) {
					if (sub->err == REDIS_ERR_IO &&
						(errno == EAGAIN || errno == EWOULDBLOCK))
						continue; // read timeout, not a real disconnect
					if (sub->err == REDIS_ERR_IO || sub->err == REDIS_ERR_EOF) break;
					continue;
				}

				if (msg->type == REDIS_REPLY_ARRAY && msg->elements >= 3) {
					std::string type = msg->element[0]->str;
					if (type == "message") {
						std::string channel = msg->element[1]->str;
						std::string data = msg->element[2]->str;
						handlePubSubMessage(channel, data);
					}
				}
				freeReplyObject(msg);
			}

			redisFree(sub);
			if (!subStop_) MS_WARN(logger_, "Subscriber disconnected, reconnecting...");
		}
	}

	void handlePubSubMessage(const std::string& channel, const std::string& data) {
		auto eq = data.find('=');
		if (eq == std::string::npos) return;
		std::string key = data.substr(0, eq);
		std::string val = data.substr(eq + 1);

		std::lock_guard<std::mutex> cl(cacheMutex_);
		if (channel == kChannelNodes) {
			if (val.empty()) {
				// Node removed — also evict all rooms owned by this node
				auto it = nodeCache_.find(key);
				if (it != nodeCache_.end()) {
					std::string deadAddr = it->second.address;
					nodeCache_.erase(it);
					if (!deadAddr.empty()) {
						for (auto rit = roomCache_.begin(); rit != roomCache_.end(); ) {
							if (rit->second == deadAddr) rit = roomCache_.erase(rit);
							else ++rit;
						}
					}
				}
			} else {
				nodeCache_[key] = parseNodeValue(val);
			}
		} else if (channel == kChannelRooms) {
			if (val.empty())
				roomCache_.erase(key);
			else
				roomCache_[key] = val;
		}
	}

	// ── Full sync (on startup and reconnect) ──

	void syncAll() {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		syncAllUnlocked();
	}

	// Lightweight: check each cached node still exists in Redis. Evict dead ones + their rooms.
	// Must be called with cmdMutex_ held.
	void evictDeadNodes() {
		if (!ctx_) return;

		// Step 1: copy node IDs (under cacheMutex_)
		std::vector<std::string> nodeIds;
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			for (auto& [nid, _] : nodeCache_)
				if (nid != nodeId_) nodeIds.push_back(nid);
		}
		if (nodeIds.empty()) return;

		// Step 2: pipeline EXISTS — one round-trip (no cacheMutex_)
		for (auto& nid : nodeIds)
			redisAppendCommand(ctx_, "EXISTS %s", (std::string(kKeyPrefixNode) + nid).c_str());

		std::vector<std::string> deadNodeIds;
		for (auto& nid : nodeIds) {
			redisReply* r = nullptr;
			if (redisGetReply(ctx_, (void**)&r) != REDIS_OK || !r) {
				handleDisconnect(); return;
			}
			if (!(r->type == REDIS_REPLY_INTEGER && r->integer == 1))
				deadNodeIds.push_back(nid);
			freeReplyObject(r);
		}
		if (deadNodeIds.empty()) return;

		// Step 3: evict (under cacheMutex_, pure memory)
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			for (auto& nid : deadNodeIds) {
				auto it = nodeCache_.find(nid);
				if (it != nodeCache_.end()) {
					std::string deadAddr = it->second.address;
					nodeCache_.erase(it);
					if (!deadAddr.empty()) {
						for (auto rit = roomCache_.begin(); rit != roomCache_.end(); )
							if (rit->second == deadAddr) rit = roomCache_.erase(rit);
							else ++rit;
					}
				}
			}
		}
		MS_DEBUG(logger_, "Evicted {} dead nodes from cache", deadNodeIds.size());
	}

	// Safe MGET using redisCommandArgv (no string concatenation).
	// Must be called with cmdMutex_ held.
	redisReply* mgetArgv(const std::vector<std::string>& keys) {
		std::vector<const char*> argv;
		std::vector<size_t> argvlen;
		argv.push_back("MGET"); argvlen.push_back(4);
		for (auto& k : keys) {
			argv.push_back(k.c_str());
			argvlen.push_back(k.size());
		}
		return (redisReply*)redisCommandArgv(ctx_,
			static_cast<int>(argv.size()), argv.data(), argvlen.data());
	}

	// SCAN-based key discovery (non-blocking alternative to KEYS).
	// Must be called with cmdMutex_ held.
	std::vector<std::string> scanKeys(const char* pattern) {
		std::vector<std::string> keys;
		std::string cursor = "0";
		do {
			auto* r = (redisReply*)redisCommand(ctx_, "SCAN %s MATCH %s COUNT 100",
				cursor.c_str(), pattern);
			if (!r || r->type != REDIS_REPLY_ARRAY || r->elements != 2) {
				if (r) freeReplyObject(r);
				break;
			}
			cursor = r->element[0]->str;
			auto* arr = r->element[1];
			for (size_t i = 0; i < arr->elements; i++)
				keys.push_back(arr->element[i]->str);
			freeReplyObject(r);
		} while (cursor != "0");
		return keys;
	}

	// Sync only node cache from Redis (lightweight, no room sync).
	// Must be called with cmdMutex_ held.
	void syncNodesUnlocked() {
		if (!ensureConnected()) return;
		std::unordered_map<std::string, NodeInfo> tmpNodes;
		auto nodeKeys = scanKeys("sfu:node:*");
		if (!nodeKeys.empty()) {
			std::vector<std::string> nids;
			for (auto& key : nodeKeys)
				nids.push_back(key.substr(9));
			auto* mr = mgetArgv(nodeKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < nids.size(); i++)
					if (mr->element[i]->type == REDIS_REPLY_STRING)
						tmpNodes[nids[i]] = parseNodeValue(mr->element[i]->str);
			}
			if (mr) freeReplyObject(mr);
		}
		if (!tmpNodes.empty()) {
			std::lock_guard<std::mutex> cl(cacheMutex_);
			for (auto& [nid, info] : tmpNodes)
				nodeCache_[nid] = info; // merge, don't replace — preserve existing entries
		}
	}

	// Must be called with cmdMutex_ held
	void syncAllUnlocked() {
		if (!ensureConnected()) return;

		// Step 1: fetch all node data via SCAN + MGET (no cacheMutex_)
		std::unordered_map<std::string, NodeInfo> tmpNodes;
		auto nodeKeys = scanKeys("sfu:node:*");
		if (!nodeKeys.empty()) {
			std::vector<std::string> nids;
			for (auto& key : nodeKeys)
				nids.push_back(key.substr(9));
			auto* mr = mgetArgv(nodeKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < nids.size(); i++)
					if (mr->element[i]->type == REDIS_REPLY_STRING)
						tmpNodes[nids[i]] = parseNodeValue(mr->element[i]->str);
			}
			if (mr) freeReplyObject(mr);
		}

		// Step 2: fetch all room data via SCAN + MGET (no cacheMutex_)
		std::unordered_map<std::string, std::string> tmpRooms;
		auto roomKeys = scanKeys("sfu:room:*");
		if (!roomKeys.empty()) {
			std::vector<std::string> rids;
			for (auto& key : roomKeys)
				rids.push_back(key.substr(9));
			auto* mr = mgetArgv(roomKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < rids.size(); i++)
					if (mr->element[i]->type == REDIS_REPLY_STRING) {
						std::string ownerNid = mr->element[i]->str;
						auto nit = tmpNodes.find(ownerNid);
						if (nit != tmpNodes.end())
							tmpRooms[rids[i]] = nit->second.address;
					}
			}
			if (mr) freeReplyObject(mr);
		}

		// Step 3: swap caches (under cacheMutex_, pure memory)
		{
			std::lock_guard<std::mutex> cl(cacheMutex_);
			nodeCache_ = std::move(tmpNodes);
			roomCache_ = std::move(tmpRooms);
		}
		MS_DEBUG(logger_, "Synced {} nodes, {} rooms", nodeCache_.size(), roomCache_.size());
	}

	// ── Cache-based lookups ──

	std::string findBestNodeCached(const std::string& clientIp) {
		std::lock_guard<std::mutex> cl(cacheMutex_);

		struct Candidate { std::string address; size_t rooms; double score; };
		std::vector<Candidate> candidates;

		GeoInfo clientGeo;
		if (geo_ && !clientIp.empty()) clientGeo = geo_->lookup(clientIp);

		for (auto& [nid, info] : nodeCache_) {
			if (info.address.empty()) continue;
			if (info.maxRooms > 0 && info.rooms >= info.maxRooms) continue;
			if (countryIsolation_ && clientGeo.valid && !clientGeo.country.empty()
				&& !info.country.empty() && info.country != clientGeo.country) continue;

			double s = 0;
			if (clientGeo.valid && (info.lat != 0 || info.lng != 0)) {
				s = geo_->score(clientGeo, info.lat, info.lng, info.isp);
			} else if (clientGeo.valid) {
				s = 99999.0;
			}
			candidates.push_back({info.address, info.rooms, s});
		}

		if (candidates.empty()) return "";

		if (clientGeo.valid) {
			std::sort(candidates.begin(), candidates.end(), [](auto& a, auto& b) {
				if (std::abs(a.score - b.score) > 100.0) return a.score < b.score;
				return a.rooms < b.rooms;
			});
		} else {
			// No geo — sort by load. Stable tiebreaker: prefer self to avoid redirect churn.
			auto self = nodeAddress_;
			std::sort(candidates.begin(), candidates.end(), [&self](auto& a, auto& b) {
				if (a.rooms != b.rooms) return a.rooms < b.rooms;
				if (a.address == self) return true;
				if (b.address == self) return false;
				return a.address < b.address;
			});
		}

		return candidates.front().address;
	}

	// ── Member fields ──

	std::string nodeId_;
	std::string nodeAddress_;
	double nodeLat_;
	double nodeLng_;
	std::string nodeIsp_;
	std::string nodeCountry_;
	bool countryIsolation_ = false;
	std::string redisHost_;
	int redisPort_;

	// Command connection (for SET/GET/EVAL)
	redisContext* ctx_ = nullptr;
	std::mutex cmdMutex_;

	// Local cache
	std::unordered_map<std::string, NodeInfo> nodeCache_;
	std::unordered_map<std::string, std::string> roomCache_; // roomId → ownerAddress
	std::mutex cacheMutex_;

	// Subscriber thread
	std::thread subThread_;
	std::atomic<bool> subStop_{false};

	int roomTTL_ = kRedisRoomTtlSec;
	int nodeTTL_ = kRedisNodeTtlSec;
	GeoRouter* geo_ = nullptr;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
