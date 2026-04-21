#include "RoomRegistry.h"

#include "GeoRouter.h"
#include "RoomRegistryReplyUtils.h"
#include "RoomRegistrySelection.h"

#include <algorithm>
#include <hiredis/hiredis.h>

namespace mediasoup {

void RoomRegistry::syncAll()
{
	std::lock_guard<std::mutex> lock(command_.mutex);
	syncAllUnlocked();
}

void RoomRegistry::evictDeadNodes()
{
	if (!command_.connected()) return;

	std::vector<std::string> nodeIds = cache_.otherNodeIds(nodeId_);
	if (nodeIds.empty()) return;

	for (auto& nodeId : nodeIds)
		command_.appendCommand("EXISTS %s", (std::string(kKeyPrefixNode) + nodeId).c_str());

	std::vector<std::string> deadNodeIds;
	for (auto& nodeId : nodeIds) {
		redisReply* reply = nullptr;
		if (command_.getReply((void**)&reply) != REDIS_OK || !reply) {
			handleDisconnect();
			return;
		}
		if (!(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1))
			deadNodeIds.push_back(nodeId);
		freeReplyObject(reply);
	}
	if (deadNodeIds.empty()) return;

	for (auto& nodeId : deadNodeIds)
		cache_.eraseNodeAndOwnedRooms(nodeId);
	MS_DEBUG(logger_, "Evicted {} dead nodes from cache", deadNodeIds.size());
}

redisReply* RoomRegistry::mgetArgv(const std::vector<std::string>& keys)
{
	std::vector<const char*> argv;
	std::vector<size_t> argvLen;
	argv.push_back("MGET");
	argvLen.push_back(4);
	for (auto& key : keys) {
		argv.push_back(key.c_str());
		argvLen.push_back(key.size());
	}
	return command_.commandArgv(
		static_cast<int>(argv.size()), argv.data(), argvLen.data());
}

std::vector<std::string> RoomRegistry::scanKeys(const char* pattern)
{
	std::vector<std::string> keys;
	std::string cursor = "0";
	do {
		auto* reply = command_.command(
			"SCAN %s MATCH %s COUNT 100", cursor.c_str(), pattern);
		if (!reply || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
			if (reply) freeReplyObject(reply);
			break;
		}
		if (!redisreply::GetTextElement(reply, 0, cursor)) {
			freeReplyObject(reply);
			break;
		}
		const auto* arr = redisreply::GetArrayElement(reply, 1, REDIS_REPLY_ARRAY);
		if (!arr) {
			freeReplyObject(reply);
			break;
		}
		for (size_t i = 0; i < arr->elements; ++i) {
			std::string key;
			if (redisreply::GetTextElement(arr, i, key))
				keys.push_back(std::move(key));
		}
		freeReplyObject(reply);
	} while (cursor != "0");
	return keys;
}

void RoomRegistry::syncNodesUnlocked()
{
	if (!ensureConnected()) return;

	std::unordered_map<std::string, NodeInfo> tmpNodes;
	auto nodeKeys = scanKeys("sfu:node:*");
	if (!nodeKeys.empty()) {
		std::vector<std::string> nodeIds;
		for (auto& key : nodeKeys) {
			nodeIds.push_back(key.substr(9));
		}
		auto* reply = mgetArgv(nodeKeys);
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements && i < nodeIds.size(); ++i) {
				std::string value;
				if (redisreply::GetTextElement(reply, i, value)) {
					tmpNodes[nodeIds[i]] = parseNodeValue(value);
				}
			}
		}
		if (reply) freeReplyObject(reply);
	}
	cache_.mergeNodes(tmpNodes);
}

void RoomRegistry::syncAllUnlocked()
{
	if (!ensureConnected()) return;

	std::unordered_map<std::string, NodeInfo> tmpNodes;
	auto nodeKeys = scanKeys("sfu:node:*");
	if (!nodeKeys.empty()) {
		std::vector<std::string> nodeIds;
		for (auto& key : nodeKeys) {
			nodeIds.push_back(key.substr(9));
		}
		auto* reply = mgetArgv(nodeKeys);
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements && i < nodeIds.size(); ++i) {
				std::string value;
				if (redisreply::GetTextElement(reply, i, value)) {
					tmpNodes[nodeIds[i]] = parseNodeValue(value);
				}
			}
		}
		if (reply) freeReplyObject(reply);
	}

	std::unordered_map<std::string, std::string> tmpRooms;
	auto roomKeys = scanKeys("sfu:room:*");
	if (!roomKeys.empty()) {
		std::vector<std::string> roomIds;
		for (auto& key : roomKeys) {
			roomIds.push_back(key.substr(9));
		}
		auto* reply = mgetArgv(roomKeys);
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements && i < roomIds.size(); ++i) {
				std::string ownerNodeId;
				if (redisreply::GetTextElement(reply, i, ownerNodeId)) {
					auto nit = tmpNodes.find(ownerNodeId);
					if (nit != tmpNodes.end())
						tmpRooms[roomIds[i]] = nit->second.address;
				}
			}
		}
		if (reply) freeReplyObject(reply);
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
