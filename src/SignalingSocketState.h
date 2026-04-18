#pragma once

#include <App.h>
#include "qos/DownlinkQosTypes.h"
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace mediasoup {

inline constexpr uint64_t kInvalidSessionId = 0;

struct PerSocketData {
	std::string peerId;
	std::string roomId;
	uint64_t sessionId = kInvalidSessionId;
	std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
};

using SignalingWebSocket = uWS::WebSocket<false, true, PerSocketData>;

struct WsMap {
	std::unordered_map<std::string, SignalingWebSocket*> peers;
	std::unordered_map<std::string, std::unordered_set<SignalingWebSocket*>> roomPeers;

	static std::string key(const std::string& roomId, const std::string& peerId) {
		return roomId + "/" + peerId;
	}
};

struct DownlinkStatsRateLimitState {
	int64_t pendingSinceMs{ 0 };
	uint64_t lastAcceptedSeq{ 0 };
	uint64_t pendingSeq{ 0 };
	bool hasAcceptedSeq{ false };
	bool pending{ false };
};

inline bool IsDownlinkSeqWrapOrReset(uint64_t prevSeq, uint64_t nextSeq)
{
	static constexpr uint64_t kSeqResetThreshold = 1000u;
	if (prevSeq >= qos::kDownlinkMaxSeq - kSeqResetThreshold &&
		nextSeq <= kSeqResetThreshold) {
		return true;
	}

	return prevSeq > nextSeq && (prevSeq - nextSeq) > kSeqResetThreshold;
}

inline bool IsAdvancingDownlinkSeq(const DownlinkStatsRateLimitState& state, uint64_t seq)
{
	if (!state.hasAcceptedSeq) return true;
	if (seq > state.lastAcceptedSeq) return true;
	if (seq < state.lastAcceptedSeq &&
		IsDownlinkSeqWrapOrReset(state.lastAcceptedSeq, seq)) {
		return true;
	}
	return false;
}

inline bool HasMappedSession(
	const std::shared_ptr<WsMap>& wsMap,
	SignalingWebSocket* ws,
	const std::string& roomId,
	const std::string& peerId,
	uint64_t sessionId)
{
	auto it = wsMap->peers.find(WsMap::key(roomId, peerId));
	return it != wsMap->peers.end() &&
		it->second == ws &&
		it->second->getUserData()->sessionId == sessionId;
}

inline void RemoveSocketFromRoomPeers(
	const std::shared_ptr<WsMap>& wsMap,
	SignalingWebSocket* ws,
	const std::string& roomId)
{
	auto roomIt = wsMap->roomPeers.find(roomId);
	if (roomIt == wsMap->roomPeers.end()) {
		return;
	}
	roomIt->second.erase(ws);
	if (roomIt->second.empty()) {
		wsMap->roomPeers.erase(roomIt);
	}
}

inline SignalingWebSocket* RegisterJoinedSocket(
	const std::shared_ptr<WsMap>& wsMap,
	SignalingWebSocket* ws,
	const std::string& roomId,
	const std::string& peerId,
	uint64_t sessionId)
{
	auto* socketData = ws->getUserData();
	socketData->roomId = roomId;
	socketData->peerId = peerId;
	socketData->sessionId = sessionId;

	const auto mapKey = WsMap::key(roomId, peerId);
	SignalingWebSocket* oldWs = nullptr;
	auto it = wsMap->peers.find(mapKey);
	if (it != wsMap->peers.end() && it->second != ws) {
		oldWs = it->second;
	}

	wsMap->peers[mapKey] = ws;
	if (oldWs) {
		oldWs->getUserData()->alive->store(false);
		RemoveSocketFromRoomPeers(wsMap, oldWs, roomId);
	}
	wsMap->roomPeers[roomId].insert(ws);
	return oldWs;
}

inline bool UnregisterSocketSession(
	const std::shared_ptr<WsMap>& wsMap,
	SignalingWebSocket* ws,
	const std::string& roomId,
	const std::string& peerId)
{
	const auto mapKey = WsMap::key(roomId, peerId);
	auto it = wsMap->peers.find(mapKey);
	if (it == wsMap->peers.end() || it->second != ws) {
		return false;
	}

	wsMap->peers.erase(it);
	RemoveSocketFromRoomPeers(wsMap, ws, roomId);
	return true;
}

inline void PostNotify(
	uWS::Loop* loop,
	const std::shared_ptr<WsMap>& wsMap,
	const std::string& roomId,
	const std::string& peerId,
	std::string payload)
{
	const auto mapKey = WsMap::key(roomId, peerId);
	loop->defer([wsMap, mapKey, payload = std::move(payload)] {
		auto it = wsMap->peers.find(mapKey);
		if (it != wsMap->peers.end()) {
			it->second->send(payload, uWS::OpCode::TEXT);
		}
	});
}

inline void PostBroadcast(
	uWS::Loop* loop,
	const std::shared_ptr<WsMap>& wsMap,
	const std::string& roomId,
	const std::string& excludePeerId,
	std::string payload)
{
	const std::string excludeKey = excludePeerId.empty() ? "" : WsMap::key(roomId, excludePeerId);
	loop->defer([wsMap, roomId, excludeKey, payload = std::move(payload)] {
		SignalingWebSocket* excludeWs = nullptr;
		if (!excludeKey.empty()) {
			auto excludeIt = wsMap->peers.find(excludeKey);
			if (excludeIt != wsMap->peers.end()) {
				excludeWs = excludeIt->second;
			}
		}

		auto roomIt = wsMap->roomPeers.find(roomId);
		if (roomIt == wsMap->roomPeers.end()) {
			return;
		}
		for (auto* ws : roomIt->second) {
			if (ws == excludeWs) {
				continue;
			}
			ws->send(payload, uWS::OpCode::TEXT);
		}
	});
}

} // namespace mediasoup
