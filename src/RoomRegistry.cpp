#include "RoomRegistry.h"

#include "GeoRouter.h"
#include "Logger.h"
#include "RoomRegistryReplyUtils.h"
#include "RoomRegistrySelection.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <poll.h>
#include <sstream>
#include <thread>

namespace mediasoup {

RoomRegistry::RoomRegistry(const std::string& redisHost, int redisPort,
	const std::string& nodeId, const std::string& nodeAddress,
	double nodeLat, double nodeLng, const std::string& nodeIsp,
	const std::string& nodeCountry, bool countryIsolation)
	: nodeId_(nodeId)
	, nodeAddress_(nodeAddress)
	, nodeLat_(nodeLat)
	, nodeLng_(nodeLng)
	, nodeIsp_(nodeIsp)
	, nodeCountry_(nodeCountry)
	, countryIsolation_(countryIsolation)
	, redisHost_(redisHost)
	, redisPort_(redisPort)
	, logger_(Logger::Get("RoomRegistry"))
{
	if (!reconnect()) {
		throw std::runtime_error("Redis connect failed");
	}
	MS_DEBUG(logger_, "Connected to Redis {}:{}", redisHost, redisPort);
}

RoomRegistry::~RoomRegistry()
{
	stop();
}

void RoomRegistry::start()
{
	registerNode();
	syncAll();
	startSubscriber();
}

size_t RoomRegistry::knownNodeCount() const
{
	std::lock_guard<std::mutex> cl(cacheMutex_);
	return nodeCache_.size();
}

void RoomRegistry::heartbeat()
{
	auto start = std::chrono::steady_clock::now();
	MS_WARN(logger_, "heartbeat start [nodeId:{}]", nodeId_);
	std::lock_guard<std::mutex> lock(cmdMutex_);
	if (!ensureConnected()) return;

	auto stepStart = std::chrono::steady_clock::now();
	MS_WARN(logger_, "heartbeat registerNode start [nodeId:{}]", nodeId_);
	registerNode();
	MS_WARN(logger_, "heartbeat registerNode done [nodeId:{} ms:{}]",
		nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - stepStart).count());

	stepStart = std::chrono::steady_clock::now();
	MS_WARN(logger_, "heartbeat evictDeadNodes start [nodeId:{}]", nodeId_);
	evictDeadNodes();
	MS_WARN(logger_, "heartbeat evictDeadNodes done [nodeId:{} ms:{} totalMs:{}]",
		nodeId_,
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - stepStart).count(),
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

void RoomRegistry::updateLoad(size_t rooms, size_t maxRooms)
{
	auto start = std::chrono::steady_clock::now();
	MS_WARN(logger_, "updateLoad start [nodeId:{} rooms:{} maxRooms:{}]",
		nodeId_, rooms, maxRooms);
	std::lock_guard<std::mutex> lock(cmdMutex_);
	if (!ensureConnected()) return;

	std::string val = buildNodeValue(rooms, maxRooms);
	MS_WARN(logger_, "updateLoad redis SET start [nodeId:{} key:{}]",
		nodeId_, (std::string(kKeyPrefixNode) + nodeId_));
	auto* reply = (redisReply*)redisCommand(
		ctx_,
		"SET %s %s EX %d",
		(std::string(kKeyPrefixNode) + nodeId_).c_str(),
		val.c_str(),
		nodeTTL_);
	if (reply) {
		MS_WARN(logger_, "updateLoad redis SET done [nodeId:{} ms:{}]",
			nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		freeReplyObject(reply);
	} else {
		handleDisconnect();
	}

	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		nodeCache_[nodeId_] = parseNodeValue(val);
	}
	publishNodeUpdate(nodeId_, val);
	MS_WARN(logger_, "updateLoad done [nodeId:{} totalMs:{}]",
		nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

void RoomRegistry::refreshRooms(const std::vector<std::string>& roomIds)
{
	auto start = std::chrono::steady_clock::now();
	MS_WARN(logger_, "refreshRooms start [count:{}]", roomIds.size());
	std::lock_guard<std::mutex> lock(cmdMutex_);
	if (!ensureConnected()) return;
	for (const auto& roomId : roomIds) {
		MS_WARN(logger_, "refreshRooms redis EXPIRE start [roomId:{}]", roomId);
		auto* reply = (redisReply*)redisCommand(
			ctx_,
			"EXPIRE %s %d",
			(std::string(kKeyPrefixRoom) + roomId).c_str(),
			roomTTL_);
		if (reply) {
			freeReplyObject(reply);
		} else {
			handleDisconnect();
			return;
		}
		MS_WARN(logger_, "refreshRooms redis EXPIRE done [roomId:{} elapsedMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
	}
	MS_WARN(logger_, "refreshRooms done [count:{} totalMs:{}]",
		roomIds.size(), std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

RoomRegistry::ResolveResult RoomRegistry::resolveRoom(const std::string& roomId, const std::string& clientIp)
{
	auto start = std::chrono::steady_clock::now();
	MS_WARN(logger_, "resolveRoom start [roomId:{} clientIp:{}]", roomId, clientIp);
	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		auto it = roomCache_.find(roomId);
		if (it != roomCache_.end() && !it->second.empty()) {
			MS_WARN(logger_, "resolveRoom cache hit [roomId:{} wsUrl:{} totalMs:{}]",
				roomId, it->second, std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - start).count());
			return {it->second, false};
		}
	}

	{
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (ensureConnected()) {
			auto getRoomStart = std::chrono::steady_clock::now();
			MS_WARN(logger_, "resolveRoom redis GET room start [roomId:{} key:{}]",
				roomId, (std::string(kKeyPrefixRoom) + roomId));
			auto* reply = (redisReply*)redisCommand(
				ctx_, "GET %s", (std::string(kKeyPrefixRoom) + roomId).c_str());
			MS_WARN(logger_, "resolveRoom redis GET room done [roomId:{} ms:{} replyType:{}]",
				roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - getRoomStart).count(),
				reply ? reply->type : -1);
			if (reply && reply->type == REDIS_REPLY_STRING) {
				std::string ownerNodeId = reply->str;
				freeReplyObject(reply);
				std::string addr;
				{
					std::lock_guard<std::mutex> cl(cacheMutex_);
					auto nit = nodeCache_.find(ownerNodeId);
					if (nit != nodeCache_.end()) addr = nit->second.address;
				}
				if (addr.empty()) {
					auto getNodeStart = std::chrono::steady_clock::now();
					MS_WARN(logger_, "resolveRoom redis GET node start [roomId:{} ownerNodeId:{} key:{}]",
						roomId, ownerNodeId, (std::string(kKeyPrefixNode) + ownerNodeId));
					auto* nr = (redisReply*)redisCommand(
						ctx_, "GET %s", (std::string(kKeyPrefixNode) + ownerNodeId).c_str());
					MS_WARN(logger_, "resolveRoom redis GET node done [roomId:{} ownerNodeId:{} ms:{} replyType:{}]",
						roomId, ownerNodeId,
						std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - getNodeStart).count(),
						nr ? nr->type : -1);
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
					MS_WARN(logger_, "resolveRoom owner resolved [roomId:{} ownerNodeId:{} wsUrl:{} totalMs:{}]",
						roomId, ownerNodeId, addr,
						std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - start).count());
					return {addr, false};
				}
			}
			if (reply) freeReplyObject(reply);
		}
	}

	auto best = findBestNodeCached(clientIp);
	bool shouldRefreshNodes = best.empty();
	if (!shouldRefreshNodes && best == nodeAddress_ && !hasRemoteNodeCached()) {
		shouldRefreshNodes = true;
	}
	if (shouldRefreshNodes) {
		std::lock_guard<std::mutex> lock(cmdMutex_);
		MS_WARN(logger_, "resolveRoom syncNodes start [roomId:{} clientIp:{}]", roomId, clientIp);
		syncNodesUnlocked();
		MS_WARN(logger_, "resolveRoom syncNodes done [roomId:{} elapsedMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		best = findBestNodeCached(clientIp);
	}
	if (best.empty()) {
		MS_WARN(logger_, "resolveRoom no node found [roomId:{} totalMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		return {"", true};
	}

	MS_WARN(logger_, "resolveRoom picked node [roomId:{} wsUrl:{} isNew:true totalMs:{}]",
		roomId, best, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
	return {best, true};
}

std::string RoomRegistry::claimRoom(const std::string& roomId, const std::string& clientIp)
{
	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		auto it = roomCache_.find(roomId);
		if (it != roomCache_.end() && !it->second.empty()) {
			if (it->second == nodeAddress_) return "";
			return it->second;
		}
	}

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

	auto* reply = (redisReply*)redisCommand(
		ctx_,
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
		if (result.empty()) {
			std::string best = findBestNodeCached(clientIp);
			if (!best.empty() && best != nodeAddress_) {
				auto* del = (redisReply*)redisCommand(ctx_, "DEL %s", roomKey.c_str());
				if (del) freeReplyObject(del);
				return best;
			}
		}
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

void RoomRegistry::refreshRoom(const std::string& roomId)
{
	std::lock_guard<std::mutex> lock(cmdMutex_);
	if (!ensureConnected()) return;
	auto* reply = (redisReply*)redisCommand(
		ctx_, "EXPIRE %s %d",
		(std::string(kKeyPrefixRoom) + roomId).c_str(), roomTTL_);
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

void RoomRegistry::unregisterRoom(const std::string& roomId)
{
	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		roomCache_.erase(roomId);
	}
	std::lock_guard<std::mutex> lock(cmdMutex_);
	if (!ensureConnected()) return;
	auto* reply = (redisReply*)redisCommand(
		ctx_, "DEL %s", (std::string(kKeyPrefixRoom) + roomId).c_str());
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
	publishRoomUpdate(roomId, "");
}

void RoomRegistry::stop()
{
	subStop_ = true;
	{
		std::lock_guard<std::mutex> lock(cmdMutex_);
		if (ctx_ && !ctx_->err) {
			auto* reply = (redisReply*)redisCommand(
				ctx_, "DEL %s", (std::string(kKeyPrefixNode) + nodeId_).c_str());
			if (reply) freeReplyObject(reply);
			std::string msg = nodeId_ + "=";
			reply = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s", kChannelNodes, msg.c_str());
			if (reply) freeReplyObject(reply);
		}
		if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
	}
	if (subThread_.joinable()) subThread_.join();
}

bool RoomRegistry::reconnect()
{
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

bool RoomRegistry::ensureConnected()
{
	if (ctx_ && !ctx_->err) return true;
	return reconnect();
}

void RoomRegistry::handleDisconnect()
{
	MS_WARN(logger_, "Redis connection lost, will reconnect on next operation");
	if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
}

std::string RoomRegistry::buildNodeValue(size_t rooms, size_t maxRooms)
{
	return nodeAddress_ + "|" + std::to_string(rooms) + "|" + std::to_string(maxRooms)
		+ "|" + std::to_string(nodeLat_) + "|" + std::to_string(nodeLng_)
		+ "|" + nodeIsp_ + "|" + nodeCountry_;
}

RoomRegistry::NodeInfo RoomRegistry::parseNodeValue(const std::string& val)
{
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

void RoomRegistry::registerNode()
{
	if (!ctx_) return;
	std::string key = std::string(kKeyPrefixNode) + nodeId_;
	auto* reply = (redisReply*)redisCommand(ctx_, "EXPIRE %s %d", key.c_str(), nodeTTL_);
	if (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
		freeReplyObject(reply);
		std::string val = buildNodeValue(0, 0);
		reply = (redisReply*)redisCommand(ctx_, "SET %s %s EX %d", key.c_str(), val.c_str(), nodeTTL_);
		if (reply) {
			{
				std::lock_guard<std::mutex> cl(cacheMutex_);
				nodeCache_[nodeId_] = parseNodeValue(val);
			}
			publishNodeUpdate(nodeId_, val);
		}
	}
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

void RoomRegistry::publishNodeUpdate(const std::string& nodeId, const std::string& nodeValue)
{
	if (!ctx_) return;
	std::string msg = nodeId + "=" + nodeValue;
	auto* reply = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s", kChannelNodes, msg.c_str());
	if (reply) freeReplyObject(reply);
}

void RoomRegistry::publishRoomUpdate(const std::string& roomId, const std::string& ownerAddr)
{
	if (!ctx_) return;
	std::string msg = roomId + "=" + ownerAddr;
	auto* reply = (redisReply*)redisCommand(ctx_, "PUBLISH %s %s", kChannelRooms, msg.c_str());
	if (reply) freeReplyObject(reply);
}

void RoomRegistry::startSubscriber()
{
	subStop_ = false;
	subThread_ = std::thread([this] { subscriberLoop(); });
}

void RoomRegistry::subscriberLoop()
{
	while (!subStop_) {
		struct timeval tv = {kRedisConnectTimeoutSec, 0};
		redisContext* sub = redisConnectWithTimeout(redisHost_.c_str(), redisPort_, tv);
		if (!sub || sub->err) {
			if (sub) redisFree(sub);
			MS_WARN(logger_, "Subscriber connect failed, retrying in 2s");
			for (int i = 0; i < 20 && !subStop_; i++)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto* reply = (redisReply*)redisCommand(sub, "SUBSCRIBE %s %s", kChannelNodes, kChannelRooms);
		if (reply) freeReplyObject(reply);
		else { redisFree(sub); continue; }

		redisSetTimeout(sub, (struct timeval){0, 0});
		int flags = fcntl(sub->fd, F_GETFL, 0);
		fcntl(sub->fd, F_SETFL, flags | O_NONBLOCK);

		MS_DEBUG(logger_, "Subscriber connected");
		syncAll();

		while (!subStop_) {
			struct pollfd pfd = {sub->fd, POLLIN, 0};
			int ret = poll(&pfd, 1, 2000);
			if (ret == 0) continue;
			if (ret < 0) break;

			redisReply* msg = nullptr;
			int rc = redisGetReply(sub, (void**)&msg);
			if (rc != REDIS_OK || !msg) {
				if (sub->err == REDIS_ERR_IO || sub->err == REDIS_ERR_EOF) break;
				if (!msg) continue;
			}

			if (msg->type == REDIS_REPLY_ARRAY && msg->elements >= 3) {
				std::string type;
				if (!redisreply::GetTextElement(msg, 0, type)) {
					MS_WARN(logger_, "Subscriber received malformed pubsub message header");
				} else if (type == "message") {
					std::string channel;
					std::string data;
					if (redisreply::GetTextElement(msg, 1, channel) &&
						redisreply::GetTextElement(msg, 2, data)) {
						handlePubSubMessage(channel, data);
					} else {
						MS_WARN(logger_, "Subscriber received malformed pubsub message payload");
					}
				}
			}
			freeReplyObject(msg);
		}

		redisFree(sub);
		if (!subStop_) MS_WARN(logger_, "Subscriber disconnected, reconnecting...");
	}
}

void RoomRegistry::handlePubSubMessage(const std::string& channel, const std::string& data)
{
	auto eq = data.find('=');
	if (eq == std::string::npos) return;
	std::string key = data.substr(0, eq);
	std::string val = data.substr(eq + 1);

	std::lock_guard<std::mutex> cl(cacheMutex_);
	if (channel == kChannelNodes) {
		if (val.empty()) {
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
		if (val.empty()) roomCache_.erase(key);
		else roomCache_[key] = val;
	}
}

void RoomRegistry::syncAll()
{
	std::lock_guard<std::mutex> lock(cmdMutex_);
	syncAllUnlocked();
}

void RoomRegistry::evictDeadNodes()
{
	if (!ctx_) return;

	std::vector<std::string> nodeIds;
	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		for (auto& [nid, _] : nodeCache_)
			if (nid != nodeId_) nodeIds.push_back(nid);
	}
	if (nodeIds.empty()) return;

	for (auto& nid : nodeIds)
		redisAppendCommand(ctx_, "EXISTS %s", (std::string(kKeyPrefixNode) + nid).c_str());

	std::vector<std::string> deadNodeIds;
	for (auto& nid : nodeIds) {
		redisReply* r = nullptr;
		if (redisGetReply(ctx_, (void**)&r) != REDIS_OK || !r) {
			handleDisconnect();
			return;
		}
		if (!(r->type == REDIS_REPLY_INTEGER && r->integer == 1))
			deadNodeIds.push_back(nid);
		freeReplyObject(r);
	}
	if (deadNodeIds.empty()) return;

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

redisReply* RoomRegistry::mgetArgv(const std::vector<std::string>& keys)
{
	std::vector<const char*> argv;
	std::vector<size_t> argvlen;
	argv.push_back("MGET"); argvlen.push_back(4);
	for (auto& k : keys) {
		argv.push_back(k.c_str());
		argvlen.push_back(k.size());
	}
	return (redisReply*)redisCommandArgv(
		ctx_, static_cast<int>(argv.size()), argv.data(), argvlen.data());
}

std::vector<std::string> RoomRegistry::scanKeys(const char* pattern)
{
	std::vector<std::string> keys;
	std::string cursor = "0";
	do {
		auto* r = (redisReply*)redisCommand(ctx_, "SCAN %s MATCH %s COUNT 100", cursor.c_str(), pattern);
		if (!r || r->type != REDIS_REPLY_ARRAY || r->elements != 2) {
			if (r) freeReplyObject(r);
			break;
		}
		if (!redisreply::GetTextElement(r, 0, cursor)) {
			freeReplyObject(r);
			break;
		}
		const auto* arr = redisreply::GetArrayElement(r, 1, REDIS_REPLY_ARRAY);
		if (!arr) {
			freeReplyObject(r);
			break;
		}
		for (size_t i = 0; i < arr->elements; i++) {
			std::string key;
			if (redisreply::GetTextElement(arr, i, key)) {
				keys.push_back(std::move(key));
			}
		}
		freeReplyObject(r);
	} while (cursor != "0");
	return keys;
}

void RoomRegistry::syncNodesUnlocked()
{
	if (!ensureConnected()) return;
	std::unordered_map<std::string, NodeInfo> tmpNodes;
	auto nodeKeys = scanKeys("sfu:node:*");
	if (!nodeKeys.empty()) {
		std::vector<std::string> nids;
		for (auto& key : nodeKeys)
			nids.push_back(key.substr(9));
			auto* mr = mgetArgv(nodeKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < nids.size(); i++) {
					std::string value;
					if (redisreply::GetTextElement(mr, i, value)) {
						tmpNodes[nids[i]] = parseNodeValue(value);
					}
				}
			}
			if (mr) freeReplyObject(mr);
		}
	if (!tmpNodes.empty()) {
		std::lock_guard<std::mutex> cl(cacheMutex_);
		for (auto& [nid, info] : tmpNodes)
			nodeCache_[nid] = info;
	}
}

void RoomRegistry::syncAllUnlocked()
{
	if (!ensureConnected()) return;

	std::unordered_map<std::string, NodeInfo> tmpNodes;
	auto nodeKeys = scanKeys("sfu:node:*");
	if (!nodeKeys.empty()) {
		std::vector<std::string> nids;
		for (auto& key : nodeKeys)
			nids.push_back(key.substr(9));
			auto* mr = mgetArgv(nodeKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < nids.size(); i++) {
					std::string value;
					if (redisreply::GetTextElement(mr, i, value)) {
						tmpNodes[nids[i]] = parseNodeValue(value);
					}
				}
			}
			if (mr) freeReplyObject(mr);
		}

	std::unordered_map<std::string, std::string> tmpRooms;
	auto roomKeys = scanKeys("sfu:room:*");
	if (!roomKeys.empty()) {
		std::vector<std::string> rids;
		for (auto& key : roomKeys)
			rids.push_back(key.substr(9));
			auto* mr = mgetArgv(roomKeys);
			if (mr && mr->type == REDIS_REPLY_ARRAY) {
				for (size_t i = 0; i < mr->elements && i < rids.size(); i++) {
					std::string ownerNid;
					if (redisreply::GetTextElement(mr, i, ownerNid)) {
						auto nit = tmpNodes.find(ownerNid);
						if (nit != tmpNodes.end())
							tmpRooms[rids[i]] = nit->second.address;
					}
				}
			}
			if (mr) freeReplyObject(mr);
		}

	size_t syncedNodeCount = 0;
	size_t syncedRoomCount = 0;
	{
		std::lock_guard<std::mutex> cl(cacheMutex_);
		nodeCache_ = std::move(tmpNodes);
		roomCache_ = std::move(tmpRooms);
		syncedNodeCount = nodeCache_.size();
		syncedRoomCount = roomCache_.size();
	}
	MS_DEBUG(logger_, "Synced {} nodes, {} rooms", syncedNodeCount, syncedRoomCount);
}

std::string RoomRegistry::findBestNodeCached(const std::string& clientIp)
{
	std::lock_guard<std::mutex> cl(cacheMutex_);

	std::vector<roomregistry::LoadCandidate> candidates;

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
		std::sort(candidates.begin(), candidates.end(), roomregistry::CompareGeoCandidates);
	} else {
		auto self = nodeAddress_;
		std::sort(candidates.begin(), candidates.end(),
			[&self](const auto& a, const auto& b) {
				return roomregistry::CompareNoGeoCandidates(a, b, self);
			});
	}

	return candidates.front().address;
}

bool RoomRegistry::hasRemoteNodeCached() const
{
	std::lock_guard<std::mutex> cl(cacheMutex_);
	for (auto& [nid, info] : nodeCache_) {
		if (nid == nodeId_) continue;
		if (!info.address.empty()) return true;
	}
	return false;
}

} // namespace mediasoup
