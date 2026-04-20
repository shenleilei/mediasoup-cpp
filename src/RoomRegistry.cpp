#include "RoomRegistry.h"

#include "Logger.h"

#include <chrono>
#include <hiredis/hiredis.h>

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
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		registerNode();
		syncAllUnlocked();
	}
	startSubscriber();
}

size_t RoomRegistry::knownNodeCount() const
{
	return cache_.knownNodeCount();
}

bool RoomRegistry::isReady()
{
	std::lock_guard<std::mutex> lock(command_.mutex);
	return command_.connected();
}

void RoomRegistry::heartbeat()
{
	auto start = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "heartbeat start [nodeId:{}]", nodeId_);
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return;

	auto stepStart = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "heartbeat registerNode start [nodeId:{}]", nodeId_);
	registerNode();
	MS_DEBUG(logger_, "heartbeat registerNode done [nodeId:{} ms:{}]",
		nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - stepStart).count());

	stepStart = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "heartbeat evictDeadNodes start [nodeId:{}]", nodeId_);
	evictDeadNodes();
	MS_DEBUG(logger_, "heartbeat evictDeadNodes done [nodeId:{} ms:{} totalMs:{}]",
		nodeId_,
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - stepStart).count(),
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

void RoomRegistry::updateLoad(size_t rooms, size_t maxRooms)
{
	auto start = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "updateLoad start [nodeId:{} rooms:{} maxRooms:{}]",
		nodeId_, rooms, maxRooms);
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return;

	std::string val = buildNodeValue(rooms, maxRooms);
	MS_DEBUG(logger_, "updateLoad redis SET start [nodeId:{} key:{}]",
		nodeId_, (std::string(kKeyPrefixNode) + nodeId_));
	auto* reply = command_.command(
		"SET %s %s EX %d",
		(std::string(kKeyPrefixNode) + nodeId_).c_str(),
		val.c_str(),
		nodeTTL_);
	if (reply) {
		MS_DEBUG(logger_, "updateLoad redis SET done [nodeId:{} ms:{}]",
			nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		freeReplyObject(reply);
	} else {
		handleDisconnect();
	}

	{
		cache_.upsertNode(nodeId_, parseNodeValue(val));
	}
	publishNodeUpdate(nodeId_, val);
	MS_DEBUG(logger_, "updateLoad done [nodeId:{} totalMs:{}]",
		nodeId_, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

void RoomRegistry::refreshRooms(const std::vector<std::string>& roomIds)
{
	auto start = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "refreshRooms start [count:{}]", roomIds.size());
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return;
	for (const auto& roomId : roomIds) {
		MS_DEBUG(logger_, "refreshRooms redis EXPIRE start [roomId:{}]", roomId);
		auto* reply = command_.command(
			"EXPIRE %s %d",
			(std::string(kKeyPrefixRoom) + roomId).c_str(),
			roomTTL_);
		if (reply) {
			freeReplyObject(reply);
		} else {
			handleDisconnect();
			return;
		}
		MS_DEBUG(logger_, "refreshRooms redis EXPIRE done [roomId:{} elapsedMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
	}
	MS_DEBUG(logger_, "refreshRooms done [count:{} totalMs:{}]",
		roomIds.size(), std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
}

RoomRegistry::ResolveResult RoomRegistry::resolveRoom(
	const std::string& roomId, const std::string& clientIp)
{
	auto start = std::chrono::steady_clock::now();
	MS_DEBUG(logger_, "resolveRoom start [roomId:{} clientIp:{}]", roomId, clientIp);
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (!ensureConnected()) {
			throw std::runtime_error("redis unavailable during resolveRoom");
		}
	}
	{
		if (auto cachedAddress = cache_.roomAddress(roomId); cachedAddress.has_value()) {
			MS_DEBUG(logger_, "resolveRoom cache hit [roomId:{} wsUrl:{} totalMs:{}]",
				roomId, *cachedAddress, std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - start).count());
			return {*cachedAddress, false};
		}
	}

	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (ensureConnected()) {
			auto getRoomStart = std::chrono::steady_clock::now();
			MS_DEBUG(logger_, "resolveRoom redis GET room start [roomId:{} key:{}]",
				roomId, (std::string(kKeyPrefixRoom) + roomId));
			std::unique_ptr<redisReply, decltype(&freeReplyObject)> roomReply(
				command_.command(
					"GET %s",
					(std::string(kKeyPrefixRoom) + roomId).c_str()),
				&freeReplyObject);
			MS_DEBUG(logger_, "resolveRoom redis GET room done [roomId:{} ms:{} replyType:{}]",
				roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
					std::chrono::steady_clock::now() - getRoomStart).count(),
				roomReply ? roomReply->type : -1);
			if (roomReply && roomReply->type == REDIS_REPLY_STRING) {
				std::string ownerNodeId = roomReply->str;
				std::string addr = cache_.nodeAddressForId(ownerNodeId).value_or("");
				if (addr.empty()) {
					auto getNodeStart = std::chrono::steady_clock::now();
					MS_DEBUG(logger_, "resolveRoom redis GET node start [roomId:{} ownerNodeId:{} key:{}]",
						roomId, ownerNodeId, (std::string(kKeyPrefixNode) + ownerNodeId));
					std::unique_ptr<redisReply, decltype(&freeReplyObject)> nodeReply(
						command_.command(
							"GET %s",
							(std::string(kKeyPrefixNode) + ownerNodeId).c_str()),
						&freeReplyObject);
					MS_DEBUG(logger_, "resolveRoom redis GET node done [roomId:{} ownerNodeId:{} ms:{} replyType:{}]",
						roomId,
						ownerNodeId,
						std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - getNodeStart).count(),
						nodeReply ? nodeReply->type : -1);
					if (nodeReply && nodeReply->type == REDIS_REPLY_STRING) {
						auto info = parseNodeValue(nodeReply->str);
						addr = info.address;
						if (!addr.empty()) {
							cache_.upsertNode(ownerNodeId, info);
						}
					}
				}
				if (!addr.empty()) {
					cache_.upsertRoom(roomId, addr);
					MS_DEBUG(logger_, "resolveRoom owner resolved [roomId:{} ownerNodeId:{} wsUrl:{} totalMs:{}]",
						roomId,
						ownerNodeId,
						addr,
						std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now() - start).count());
					return {addr, false};
				}
			}
		}
	}

	auto best = findBestNodeCached(clientIp);
	bool shouldRefreshNodes = best.empty();
	if (!shouldRefreshNodes && best == nodeAddress_ && !hasRemoteNodeCached()) {
		shouldRefreshNodes = true;
	}
	if (shouldRefreshNodes) {
		std::lock_guard<std::mutex> lock(command_.mutex);
		(void)lock;
		MS_DEBUG(logger_, "resolveRoom syncNodes start [roomId:{} clientIp:{}]", roomId, clientIp);
		syncNodesUnlocked();
		MS_DEBUG(logger_, "resolveRoom syncNodes done [roomId:{} elapsedMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		best = findBestNodeCached(clientIp);
	}
	if (best.empty()) {
		MS_DEBUG(logger_, "resolveRoom no node found [roomId:{} totalMs:{}]",
			roomId, std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count());
		return {"", true};
	}

	MS_DEBUG(logger_, "resolveRoom picked node [roomId:{} wsUrl:{} isNew:true totalMs:{}]",
		roomId, best, std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - start).count());
	return {best, true};
}

std::string RoomRegistry::claimRoom(const std::string& roomId, const std::string& clientIp)
{
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) {
		throw std::runtime_error("redis unavailable during claimRoom");
	}

	const auto cachedAddress = cache_.roomAddress(roomId);
	if (cachedAddress.has_value() && *cachedAddress == nodeAddress_) {
		return "";
	}

	std::string roomKey = std::string(kKeyPrefixRoom) + roomId;
	if (cachedAddress.has_value()) {
		std::string refreshedAddress;
		if (refreshCachedRemoteRoomAddressUnlocked(roomId, *cachedAddress, &refreshedAddress)) {
			return refreshedAddress == nodeAddress_ ? "" : refreshedAddress;
		}
		if (!refreshedAddress.empty()) {
			cache_.upsertRoom(roomId, refreshedAddress);
			return refreshedAddress == nodeAddress_ ? "" : refreshedAddress;
		}
		MS_WARN(logger_,
			"claimRoom dropping stale cached redirect [roomId:{} cached:{}]",
			roomId,
			*cachedAddress);
		cache_.eraseRoom(roomId);
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

	std::string nodePrefix = kKeyPrefixNode;
	std::string ttlStr = std::to_string(roomTTL_);

	auto* reply = command_.command(
		"EVAL %s 1 %s %s %s %s",
		luaScript,
		roomKey.c_str(),
		nodeId_.c_str(),
		ttlStr.c_str(),
		nodePrefix.c_str());

	if (!reply) {
		handleDisconnect();
		throw std::runtime_error("redis unavailable during claimRoom eval");
	}

	std::string result;
	if (reply->type == REDIS_REPLY_STRING) result = reply->str;
	freeReplyObject(reply);

	if (result.empty() || result == "self") {
		if (result.empty()) {
			std::string best = findBestNodeCached(clientIp);
			if (!best.empty() && best != nodeAddress_) {
				auto* del = command_.command("DEL %s", roomKey.c_str());
				if (del) freeReplyObject(del);
				return best;
			}
		}
		{
			cache_.upsertRoom(roomId, nodeAddress_);
		}
		publishRoomUpdate(roomId, nodeAddress_);
		return "";
	}
	if (result.rfind("taken:", 0) == 0) {
		MS_DEBUG(logger_, "Node {} is dead, taking over room {}", result.substr(6), roomId);
		{
			cache_.upsertRoom(roomId, nodeAddress_);
		}
		publishRoomUpdate(roomId, nodeAddress_);
		return "";
	}
	if (result.rfind("addr:", 0) == 0) {
		auto info = parseNodeValue(result.substr(5));
		if (!info.address.empty()) {
			cache_.upsertRoom(roomId, info.address);
			return info.address;
		}
		throw std::runtime_error("owner node value unparseable during claimRoom");
	}

	throw std::runtime_error("unexpected claimRoom result: " + result);
}

bool RoomRegistry::refreshCachedRemoteRoomAddressUnlocked(
	const std::string& roomId,
	const std::string& cachedAddress,
	std::string* refreshedAddress)
{
	if (refreshedAddress) {
		refreshedAddress->clear();
	}
	if (cachedAddress.empty() || cachedAddress == nodeAddress_) {
		if (refreshedAddress) {
			*refreshedAddress = cachedAddress;
		}
		return !cachedAddress.empty();
	}
	if (!ensureConnected()) {
		return false;
	}

	const std::string roomKey = std::string(kKeyPrefixRoom) + roomId;
	std::unique_ptr<redisReply, decltype(&freeReplyObject)> roomReply(
		command_.command("GET %s", roomKey.c_str()),
		&freeReplyObject);
	if (!roomReply || roomReply->type != REDIS_REPLY_STRING || !roomReply->str) {
		return false;
	}

	const std::string ownerNodeKey =
		std::string(kKeyPrefixNode) + std::string(roomReply->str, roomReply->len);
	std::unique_ptr<redisReply, decltype(&freeReplyObject)> nodeReply(
		command_.command("GET %s", ownerNodeKey.c_str()),
		&freeReplyObject);
	if (!nodeReply || nodeReply->type != REDIS_REPLY_STRING || !nodeReply->str) {
		return false;
	}

	auto info = parseNodeValue(std::string(nodeReply->str, nodeReply->len));
	if (info.address.empty()) {
		return false;
	}
	if (refreshedAddress) {
		*refreshedAddress = info.address;
	}
	return info.address == cachedAddress;
}

void RoomRegistry::refreshRoom(const std::string& roomId)
{
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return;
	auto* reply = command_.command(
		"EXPIRE %s %d",
		(std::string(kKeyPrefixRoom) + roomId).c_str(),
		roomTTL_);
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

void RoomRegistry::unregisterRoom(const std::string& roomId)
{
	{
		cache_.eraseRoom(roomId);
	}
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return;
	auto* reply = command_.command(
		"DEL %s",
		(std::string(kKeyPrefixRoom) + roomId).c_str());
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
	publishRoomUpdate(roomId, "");
}

void RoomRegistry::stop()
{
	subStop_ = true;
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (command_.connected()) {
			auto* reply = command_.command(
				"DEL %s",
				(std::string(kKeyPrefixNode) + nodeId_).c_str());
			if (reply) freeReplyObject(reply);
			std::string msg = nodeId_ + "=";
			reply = command_.command("PUBLISH %s %s", kChannelNodes, msg.c_str());
			if (reply) freeReplyObject(reply);
		}
		command_.disconnect(logger_, /*logAsWarning=*/false);
	}
	if (subThread_.joinable()) subThread_.join();
}

} // namespace mediasoup
