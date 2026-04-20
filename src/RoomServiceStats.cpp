#include "RoomService.h"

#include "RoomStatsQosHelpers.h"

namespace mediasoup {

json RoomService::collectPeerStats(const std::string& roomId, const std::string& peerId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {};
	auto peer = room->getPeer(peerId);
	if (!peer) return {};

	json result = {{"peerId", peerId}};

	static constexpr int kPerPeerBudgetMs = 2000;
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kPerPeerBudgetMs);

	auto budgetLeft = [&]() -> int {
		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now()).count();
		return remaining > 0 ? static_cast<int>(remaining) : 0;
	};

	auto statsTimeout = [&]() -> int {
		return std::min(kStatsTimeoutMs, budgetLeft());
	};

	if (peer->sendTransport && budgetLeft() > 0) {
		try { result["sendTransport"] = peer->sendTransport->getStats(statsTimeout()); }
		catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: {}", roomId, peerId, e.what());
			result["sendTransport"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: unknown error", roomId, peerId);
			result["sendTransport"] = nullptr;
		}
	}
	if (peer->recvTransport && budgetLeft() > 0) {
		try { result["recvTransport"] = peer->recvTransport->getStats(statsTimeout()); }
		catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] recvTransport getStats failed: {}", roomId, peerId, e.what());
			result["recvTransport"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] recvTransport getStats failed: unknown error", roomId, peerId);
			result["recvTransport"] = nullptr;
		}
	}

	result["producers"] = roomstatsqos::BuildProducerStats(
		roomId, peerId, peer, logger_, budgetLeft, statsTimeout);
	result["consumers"] = roomstatsqos::BuildConsumerStats(
		roomId, peerId, room, peer, logger_, budgetLeft, statsTimeout);
	roomstatsqos::AppendPeerQosStats(result, qosRegistry_, roomId, peerId);
	roomstatsqos::AppendPeerDownlinkStats(
		result, downlinkQosRegistry_, subscriberControllers_, roomId, peerId);

	return result;
}

void RoomService::broadcastStats() {
	if (statsBroadcastActive_) return;

	auto roomIds = roomManager_.getRoomIds();
	if (roomIds.empty()) return;

	std::string names;
	for (auto& id : roomIds) { if (!names.empty()) names += ", "; names += id; }
	MS_DEBUG(logger_, "broadcastStats: {} rooms [{}]", roomIds.size(), names);

	statsBroadcastActive_ = true;
	pendingStatsRooms_.clear();
	for (auto& roomId : roomIds) pendingStatsRooms_.push_back(roomId);
	try {
		continueBroadcastStats();
	} catch (...) {
		statsBroadcastActive_ = false;
		pendingStatsRooms_.clear();
		throw;
	}
}

void RoomService::continueBroadcastStats() {
	if (pendingStatsRooms_.empty()) {
		statsBroadcastActive_ = false;
		return;
	}

	std::string roomId = std::move(pendingStatsRooms_.front());
	pendingStatsRooms_.pop_front();
	try {
		broadcastStatsForRoom(roomId);
	} catch (...) {
		statsBroadcastActive_ = false;
		pendingStatsRooms_.clear();
		throw;
	}

	if (pendingStatsRooms_.empty()) {
		statsBroadcastActive_ = false;
		return;
	}

	if (taskPoster_) {
		taskPoster_([this] { continueBroadcastStats(); });
	} else {
		continueBroadcastStats();
	}
}

void RoomService::broadcastStatsForRoom(const std::string& roomId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return;

	json allStats = json::array();
	for (auto& peerId : room->getPeerIds()) {
		try {
			auto stats = collectPeerStats(roomId, peerId);
			if (!stats.empty()) allStats.push_back(stats);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "broadcastStats collectPeerStats failed [room:{} peer:{}]: {}", roomId, peerId, e.what());
			allStats.push_back({{"peerId", peerId}});
		} catch (...) {
			MS_WARN(logger_, "broadcastStats collectPeerStats failed [room:{} peer:{}]: unknown error", roomId, peerId);
			allStats.push_back({{"peerId", peerId}});
		}
	}

	if (allStats.empty()) return;

	if (broadcast_) {
		broadcast_(roomId, "", {
			{"notification", true}, {"method", "statsReport"},
			{"data", {{"roomId", roomId}, {"peers", allStats}}}
		});
	}

	for (auto& peerStats : allStats) {
		std::string key = roomstatsqos::MakePeerKey(
			roomId,
			peerStats.value("peerId", ""));
		auto it = recorders_.find(key);
		if (it != recorders_.end() && it->second)
			it->second->appendQosSnapshot(peerStats);
	}
}

} // namespace mediasoup
