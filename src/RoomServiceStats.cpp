#include "RoomService.h"

#include "RoomStatsQosHelpers.h"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace mediasoup {

namespace {

uint8_t LatestProducerScore(const std::shared_ptr<Producer>& producer)
{
	if (!producer) {
		return 0;
	}

	const auto& scores = producer->scores();
	if (scores.empty()) {
		return 0;
	}

	return scores.back().score;
}

json BuildRoomProducerScoreSnapshot(const std::shared_ptr<Room>& room)
{
	json snapshot = json::object();
	if (!room) {
		return snapshot;
	}

	for (const auto& peerId : room->getPeerIds()) {
		auto peer = room->getPeer(peerId);
		if (!peer) {
			continue;
		}

		json peerScores = json::object();
		for (const auto& [producerId, producer] : peer->producers) {
			if (!producer || producer->closed()) {
				continue;
			}
			peerScores[producerId] = LatestProducerScore(producer);
		}
		snapshot[peerId] = std::move(peerScores);
	}

	return snapshot;
}

constexpr double kProducerScoreBroadcastDelta = 1.0;

json BuildRoomSnapshot(const std::shared_ptr<Room>& room)
{
	json snapshot = {
		{"participants", 0},
		{"publishers", 0},
		{"subscribers", 0}
	};
	if (!room) {
		return snapshot;
	}

	size_t publishers = 0;
	size_t subscribers = 0;
	const auto peerIds = room->getPeerIds();
	for (const auto& peerId : peerIds) {
		auto peer = room->getPeer(peerId);
		if (!peer) {
			continue;
		}
		if (!peer->producers.empty()) {
			++publishers;
		}
		if (!peer->consumers.empty()) {
			++subscribers;
		}
	}

	snapshot["participants"] = peerIds.size();
	snapshot["publishers"] = publishers;
	snapshot["subscribers"] = subscribers;
	return snapshot;
}

} // namespace

json RoomService::collectPeerStats(
	const std::string& roomId,
	const std::string& peerId,
	bool includeConsumers) {
	MS_INFO(logger_, "[{} {}] collectPeerStats start includeConsumers={}",
		roomId, peerId, includeConsumers ? "true" : "false");
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] collectPeerStats failed: room not found", roomId, peerId);
		return {};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] collectPeerStats failed: peer not found", roomId, peerId);
		return {};
	}

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
	if (includeConsumers) {
		result["consumers"] = roomstatsqos::BuildConsumerStats(
			roomId, peerId, room, peer, logger_, budgetLeft, statsTimeout);
	}
	roomstatsqos::AppendPeerQosStats(result, qosRegistry_, roomId, peerId);
	roomstatsqos::AppendPeerDownlinkStats(
		result, downlinkQosRegistry_, subscriberControllers_, roomId, peerId);
	MS_INFO(logger_, "[{} {}] collectPeerStats done includeConsumers={} producers={} consumers={} hasSendTransport={} hasRecvTransport={}",
		roomId,
		peerId,
		includeConsumers ? "true" : "false",
		result.contains("producers") && result["producers"].is_array() ? result["producers"].size() : 0,
		includeConsumers && result.contains("consumers") && result["consumers"].is_array() ? result["consumers"].size() : 0,
		result.contains("sendTransport") && !result["sendTransport"].is_null() ? "true" : "false",
		result.contains("recvTransport") && !result["recvTransport"].is_null() ? "true" : "false");

	return result;
}

void RoomService::broadcastStats() {
	if (statsBroadcastActive_) {
		MS_DEBUG(logger_, "[system] broadcastStats skipped: already active");
		return;
	}

	auto roomIds = roomManager_.getRoomIds();
	if (roomIds.empty()) {
		MS_DEBUG(logger_, "[system] broadcastStats skipped: no rooms");
		return;
	}

	std::string names;
	for (auto& id : roomIds) { if (!names.empty()) names += ", "; names += id; }
	MS_DEBUG(logger_, "broadcastStats: {} rooms [{}]", roomIds.size(), names);

	statsBroadcastActive_ = true;
	pendingStatsRooms_.clear();
	for (auto& roomId : roomIds) pendingStatsRooms_.push_back(roomId);
	try {
		emitGlobalRoomSnapshot();
		continueBroadcastStats();
	} catch (...) {
		statsBroadcastActive_ = false;
		pendingStatsRooms_.clear();
		throw;
	}
}

void RoomService::emitGlobalRoomSnapshot()
{
	const auto roomIds = roomManager_.getRoomIds();
	size_t rooms = 0;
	size_t peers = 0;
	size_t publishers = 0;
	size_t subscribers = 0;

	for (const auto& roomId : roomIds) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) {
			continue;
		}
		++rooms;
		const auto roomSnapshot = BuildRoomSnapshot(room);
		peers += roomSnapshot.value("participants", 0);
		publishers += roomSnapshot.value("publishers", 0);
		subscribers += roomSnapshot.value("subscribers", 0);
		MS_INFO(logger_, "[{} system] roomSnapshot participants={} publishers={} subscribers={}",
			roomId,
			roomSnapshot.value("participants", 0),
			roomSnapshot.value("publishers", 0),
			roomSnapshot.value("subscribers", 0));
	}

	MS_INFO(logger_, "[system] globalSnapshot rooms={} publishers={} subscribers={} peers={}",
		rooms, publishers, subscribers, peers);
}

void RoomService::continueBroadcastStats() {
	if (pendingStatsRooms_.empty()) {
		statsBroadcastActive_ = false;
		return;
	}

	std::string roomId = std::move(pendingStatsRooms_.front());
	pendingStatsRooms_.pop_front();
	try {
		broadcastStatsForRoom(roomId, true);
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

void RoomService::broadcastStatsForRoom(const std::string& roomId, bool forceBroadcast) {
	if (statsBroadcastActive_ && !forceBroadcast) {
		MS_DEBUG(logger_, "[{} system] broadcastStats skipped: active and not forced", roomId);
		return;
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_DEBUG(logger_, "[{} system] broadcastStats skipped: room not found", roomId);
		return;
	}

	const auto currentProducerScores = BuildRoomProducerScoreSnapshot(room);
	auto previousScoresIt = lastStatsReportProducerScores_.find(roomId);
	if (!forceBroadcast &&
		previousScoresIt != lastStatsReportProducerScores_.end() &&
		!roomstatsqos::HasSignificantStatsReportScoreChange(
			previousScoresIt->second,
			currentProducerScores,
			kProducerScoreBroadcastDelta)) {
		MS_DEBUG(logger_, "[{} system] broadcastStats skipped: producer scores unchanged", roomId);
		return;
	}

	json allStats = json::array();
	for (auto& peerId : room->getPeerIds()) {
		try {
			auto stats = collectPeerStats(roomId, peerId, /*includeConsumers=*/false);
			if (!stats.empty()) allStats.push_back(stats);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] broadcastStats collectPeerStats failed: {}", roomId, peerId, e.what());
			allStats.push_back({{"peerId", peerId}});
		} catch (...) {
			MS_WARN(logger_, "[{} {}] broadcastStats collectPeerStats failed: unknown error", roomId, peerId);
			allStats.push_back({{"peerId", peerId}});
		}
	}

	if (allStats.empty()) {
		MS_DEBUG(logger_, "[{} system] broadcastStats skipped: no peer stats", roomId);
		return;
	}

	if (broadcast_) {
		const auto targetPeers = room->getPeerIds();
		MS_INFO(logger_, "[{} system] notify statsReport targetPeerCount={} targetPeers={} statsPeers={}",
			roomId, targetPeers.size(), json(targetPeers).dump(), allStats.size());
		broadcast_(roomId, "", {
			{"notification", true}, {"method", "statsReport"},
			{"data", {{"roomId", roomId}, {"peers", allStats}}}
		});
	}

	lastStatsReportProducerScores_[roomId] = std::move(currentProducerScores);
}

void RoomService::watchProducerScore(
	const std::string& roomId,
	const std::shared_ptr<Producer>& producer)
{
	if (!producer) {
		return;
	}

	producer->emitter().on("score", [this, roomId](const std::vector<std::any>&) {
		broadcastStatsForRoom(roomId, false);
	});
}

} // namespace mediasoup
