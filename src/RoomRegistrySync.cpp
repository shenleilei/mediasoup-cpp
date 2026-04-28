#include "RoomRegistry.h"

#include "GeoRouter.h"
#include "RoomRegistryReplyUtils.h"
#include "RoomRegistrySelection.h"

#include <algorithm>
#include <hiredis/hiredis.h>

namespace mediasoup {

namespace {

constexpr size_t kRedisKeyPrefixLen = 9;

bool HasExpectedArrayReply(const redisReply* reply, size_t expectedElements)
{
	return reply &&
		reply->type == REDIS_REPLY_ARRAY &&
		reply->elements == expectedElements;
}

} // namespace

void RoomRegistry::syncAll()
{
	syncAllSnapshot();
}

void RoomRegistry::evictDeadNodes()
{
	std::vector<std::string> nodeIds = cache_.otherNodeIds(nodeId_);
	if (nodeIds.empty()) return;

	std::vector<std::string> deadNodeIds;
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (!ensureConnected()) return;

		for (const auto& nodeId : nodeIds) {
			if (command_.appendCommand(
				"EXISTS %s",
				(std::string(kKeyPrefixNode) + nodeId).c_str()) != REDIS_OK) {
				handleDisconnect();
				return;
			}
		}

		for (const auto& nodeId : nodeIds) {
			redisReply* reply = nullptr;
			if (command_.getReply((void**)&reply) != REDIS_OK || !reply) {
				handleDisconnect();
				return;
			}
			if (!(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1))
				deadNodeIds.push_back(nodeId);
			freeReplyObject(reply);
		}
	}
	if (deadNodeIds.empty()) return;

	for (auto& nodeId : deadNodeIds)
		cache_.eraseNodeAndOwnedRooms(nodeId);
	MS_DEBUG(logger_, "Evicted {} dead nodes from cache", deadNodeIds.size());
}

redisReply* RoomRegistry::mgetArgv(const std::vector<std::string>& keys)
{
	if (keys.empty()) return nullptr;

	std::vector<const char*> argv;
	std::vector<size_t> argvLen;
	argv.push_back("MGET");
	argvLen.push_back(4);
	for (auto& key : keys) {
		argv.push_back(key.c_str());
		argvLen.push_back(key.size());
	}
	std::lock_guard<std::mutex> lock(command_.mutex);
	if (!ensureConnected()) return nullptr;
	auto* reply = command_.commandArgv(
		static_cast<int>(argv.size()), argv.data(), argvLen.data());
	if (!reply) {
		handleDisconnect();
	}
	return reply;
}

RoomRegistry::ScanKeysResult RoomRegistry::scanKeys(const char* pattern)
{
	ScanKeysResult result;
	result.complete = true;
	std::string cursor = "0";
	do {
		redisReply* reply = nullptr;
		{
			std::lock_guard<std::mutex> lock(command_.mutex);
			if (!ensureConnected()) {
				result.complete = false;
				break;
			}
			reply = command_.command(
				"SCAN %s MATCH %s COUNT 100", cursor.c_str(), pattern);
			if (!reply) {
				handleDisconnect();
			}
		}
		if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
			if (reply) freeReplyObject(reply);
			result.complete = false;
			break;
		}
		if (!redisreply::GetTextElement(reply, 0, cursor)) {
			freeReplyObject(reply);
			result.complete = false;
			break;
		}
		const auto* arr = redisreply::GetArrayElement(reply, 1, REDIS_REPLY_ARRAY);
		if (!arr) {
			freeReplyObject(reply);
			result.complete = false;
			break;
		}
		for (size_t i = 0; i < arr->elements; ++i) {
			std::string key;
			if (redisreply::GetTextElement(arr, i, key))
				result.keys.push_back(std::move(key));
		}
		freeReplyObject(reply);
	} while (cursor != "0");
	return result;
}

void RoomRegistry::syncNodesSnapshot()
{
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (!ensureConnected()) return;
	}

	std::unordered_map<std::string, NodeInfo> tmpNodes;
	const auto nodeScan = scanKeys("sfu:node:*");
	if (!nodeScan.complete) {
		MS_WARN(logger_, "Skipping node snapshot merge due to incomplete Redis scan");
		return;
	}
	if (!nodeScan.keys.empty()) {
		std::vector<std::string> nodeIds;
		for (const auto& key : nodeScan.keys) {
			nodeIds.push_back(key.substr(kRedisKeyPrefixLen));
		}
		std::unique_ptr<redisReply, decltype(&freeReplyObject)> reply(
			mgetArgv(nodeScan.keys),
			&freeReplyObject);
		if (!HasExpectedArrayReply(reply.get(), nodeIds.size())) {
			MS_WARN(logger_, "Skipping node snapshot merge due to incomplete Redis MGET reply");
			return;
		}
		if (reply) {
			for (size_t i = 0; i < reply->elements && i < nodeIds.size(); ++i) {
				std::string value;
				if (redisreply::GetTextElement(reply.get(), i, value)) {
					tmpNodes[nodeIds[i]] = parseNodeValue(value);
				}
			}
		}
	}
	cache_.mergeNodes(tmpNodes);
}

void RoomRegistry::syncAllSnapshot()
{
	{
		std::lock_guard<std::mutex> lock(command_.mutex);
		if (!ensureConnected()) return;
	}

	std::unordered_map<std::string, NodeInfo> tmpNodes;
	const auto nodeScan = scanKeys("sfu:node:*");
	if (!nodeScan.complete) {
		MS_WARN(logger_, "Skipping full snapshot publish due to incomplete node scan");
		return;
	}
	if (!nodeScan.keys.empty()) {
		std::vector<std::string> nodeIds;
		for (const auto& key : nodeScan.keys) {
			nodeIds.push_back(key.substr(kRedisKeyPrefixLen));
		}
		std::unique_ptr<redisReply, decltype(&freeReplyObject)> reply(
			mgetArgv(nodeScan.keys),
			&freeReplyObject);
		if (!HasExpectedArrayReply(reply.get(), nodeIds.size())) {
			MS_WARN(logger_, "Skipping full snapshot publish due to incomplete node MGET reply");
			return;
		}
		if (reply) {
			for (size_t i = 0; i < reply->elements && i < nodeIds.size(); ++i) {
				std::string value;
				if (redisreply::GetTextElement(reply.get(), i, value)) {
					tmpNodes[nodeIds[i]] = parseNodeValue(value);
				}
			}
		}
	}

	std::unordered_map<std::string, std::string> tmpRooms;
	const auto roomScan = scanKeys("sfu:room:*");
	if (!roomScan.complete) {
		MS_WARN(logger_, "Skipping full snapshot publish due to incomplete room scan");
		return;
	}
	if (!roomScan.keys.empty()) {
		std::vector<std::string> roomIds;
		for (const auto& key : roomScan.keys) {
			roomIds.push_back(key.substr(kRedisKeyPrefixLen));
		}
		std::unique_ptr<redisReply, decltype(&freeReplyObject)> reply(
			mgetArgv(roomScan.keys),
			&freeReplyObject);
		if (!HasExpectedArrayReply(reply.get(), roomIds.size())) {
			MS_WARN(logger_, "Skipping full snapshot publish due to incomplete room MGET reply");
			return;
		}
		if (reply) {
			for (size_t i = 0; i < reply->elements && i < roomIds.size(); ++i) {
				std::string ownerNodeId;
				if (redisreply::GetTextElement(reply.get(), i, ownerNodeId)) {
					auto nit = tmpNodes.find(ownerNodeId);
					if (nit != tmpNodes.end())
						tmpRooms[roomIds[i]] = nit->second.address;
				}
			}
		}
	}

	size_t syncedNodeCount = tmpNodes.size();
	size_t syncedRoomCount = tmpRooms.size();
	cache_.replaceAll(std::move(tmpNodes), std::move(tmpRooms));
	MS_DEBUG(logger_, "Synced {} nodes, {} rooms", syncedNodeCount, syncedRoomCount);
}

std::string RoomRegistry::findBestNodeCached(const std::string& clientIp)
{
	std::vector<roomregistry::LoadCandidate> candidates;

	GeoInfo clientGeo;
	if (geo_ && !clientIp.empty()) clientGeo = geo_->lookup(clientIp);

	{
		std::lock_guard<std::mutex> lock(cache_.mutex);
		for (auto& [nodeId, info] : cache_.nodes) {
			(void)nodeId;
			if (info.address.empty()) continue;
			if (info.maxRooms > 0 && info.rooms >= info.maxRooms) continue;
			if (countryIsolation_ && clientGeo.valid && !clientGeo.country.empty() &&
				!info.country.empty() && info.country != clientGeo.country) continue;

			double score = 0;
			if (clientGeo.valid && (info.lat != 0 || info.lng != 0)) {
				score = geo_->score(clientGeo, info.lat, info.lng, info.isp);
			} else if (clientGeo.valid) {
				score = 99999.0;
			}
			candidates.push_back({info.address, info.rooms, score});
		}
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
	std::lock_guard<std::mutex> lock(cache_.mutex);
	for (auto& [nodeId, info] : cache_.nodes) {
		if (nodeId == nodeId_) continue;
		if (!info.address.empty()) return true;
	}
	return false;
}

} // namespace mediasoup
