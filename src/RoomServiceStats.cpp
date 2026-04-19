#include "RoomService.h"

#include "RoomRegistry.h"
#include "RoomStatsQosHelpers.h"

#include <sstream>

namespace mediasoup {
namespace {

std::string SummarizePeerQosAggregate(const qos::PeerQosAggregate& aggregate)
{
	std::ostringstream summary;
	summary
		<< "quality=" << aggregate.quality
		<< " stale=" << (aggregate.stale ? "true" : "false")
		<< " lost=" << (aggregate.lost ? "true" : "false")
		<< " ageMs=" << aggregate.ageMs
		<< " lastUpdatedMs=" << aggregate.lastUpdatedMs;

	if (aggregate.data.is_object()) {
		if (aggregate.data.contains("seq")) {
			summary << " seq=" << aggregate.data["seq"].dump();
		}

		const auto tracksIt = aggregate.data.find("tracks");
		if (tracksIt != aggregate.data.end() && tracksIt->is_array() && !tracksIt->empty()) {
			const auto& track = (*tracksIt)[0];
			if (track.is_object()) {
				summary
					<< " trackState=" << track.value("state", "unknown")
					<< " trackQuality=" << track.value("quality", "unknown")
					<< " ladderLevel=" << track.value("ladderLevel", -1)
					<< " reason=" << track.value("reason", "unknown");

				const auto signalsIt = track.find("signals");
				if (signalsIt != track.end() && signalsIt->is_object()) {
					summary
						<< " sendBitrateBps=" << signalsIt->value("sendBitrateBps", 0)
						<< " targetBitrateBps=" << signalsIt->value("targetBitrateBps", 0)
						<< " lossRate=" << signalsIt->value("lossRate", 0.0)
						<< " rttMs=" << signalsIt->value("rttMs", 0.0)
						<< " jitterMs=" << signalsIt->value("jitterMs", 0.0);
					if (signalsIt->contains("qualityLimitationReason")) {
						summary << " qualityLimitationReason="
							<< (*signalsIt)["qualityLimitationReason"].dump();
					}
				}
			}
		}
	}

	return summary.str();
}

size_t FilterConsistentDownlinkSubscriptions(
	const std::shared_ptr<Peer>& peer,
	qos::DownlinkSnapshot* snapshot)
{
	if (!peer || !snapshot) return 0;

	std::vector<qos::DownlinkSubscription> filtered;
	filtered.reserve(snapshot->subscriptions.size());
	json filteredRaw = json::array();
	const bool hasRawSubscriptions =
		snapshot->raw.is_object() &&
		snapshot->raw.contains("subscriptions") &&
		snapshot->raw["subscriptions"].is_array();

	size_t dropped = 0;
	for (size_t i = 0; i < snapshot->subscriptions.size(); ++i) {
		const auto& sub = snapshot->subscriptions[i];
		auto consumerIt = peer->consumers.find(sub.consumerId);
		if (consumerIt == peer->consumers.end() ||
			!consumerIt->second ||
			consumerIt->second->producerId() != sub.producerId) {
			++dropped;
			continue;
		}
		auto normalizedSub = sub;
		normalizedSub.kind = consumerIt->second->kind();
		filtered.push_back(std::move(normalizedSub));
		if (hasRawSubscriptions && i < snapshot->raw["subscriptions"].size()) {
			auto raw = snapshot->raw["subscriptions"][i];
			raw["kind"] = consumerIt->second->kind();
			filteredRaw.push_back(std::move(raw));
		}
	}

	snapshot->subscriptions = std::move(filtered);
	if (hasRawSubscriptions)
		snapshot->raw["subscriptions"] = std::move(filteredRaw);

	return dropped;
}

} // namespace

void RoomService::setClientStats(
	const std::string& roomId,
	const std::string& peerId,
	qos::ClientQosSnapshot stats)
{
	std::string rejectReason;
	if (!qosRegistry_.Upsert(roomId, peerId, stats, qos::NowMs(), &rejectReason)) {
		MS_DEBUG(logger_, "[{} {}] dropped QoS clientStats [{}]", roomId, peerId, rejectReason);
		return;
	}

	auto qosEntry = qosRegistry_.Get(roomId, peerId);
	if (!qosEntry) return;
	auto aggregate = qos::QosAggregator::Aggregate(qosEntry, qos::NowMs());
	maybeNotifyConnectionQuality(roomId, peerId, aggregate);
	maybeSendAutomaticQosOverride(roomId, peerId, aggregate);
	maybeBroadcastRoomQosState(roomId);
	cleanupExpiredQosOverrides();
}

void RoomService::setDownlinkClientStats(const std::string& roomId, const std::string& peerId,
	const json& stats)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return;
	auto peer = room->getPeer(peerId);
	if (!peer) return;

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(stats);
	if (!parsed.ok) {
		MS_WARN(logger_, "[{} {}] invalid downlink stats ignored: {}", roomId, peerId, parsed.error);
		return;
	}
	if (parsed.value.subscriberPeerId != peerId) {
		MS_WARN(logger_, "[{} {}] downlink subscriberPeerId mismatch: {}", roomId, peerId, parsed.value.subscriberPeerId);
		return;
	}
	const auto droppedSubscriptions =
		FilterConsistentDownlinkSubscriptions(peer, &parsed.value);
	if (droppedSubscriptions > 0) {
		MS_WARN(logger_,
			"[{} {}] dropped {} inconsistent downlink subscriptions",
			roomId, peerId, droppedSubscriptions);
	}
	std::string rejectReason;
	if (!downlinkQosRegistry_.Upsert(roomId, peerId, parsed.value, qos::NowMs(), &rejectReason)) {
		MS_DEBUG(logger_, "[{} {}] dropped downlink stats [{}]", roomId, peerId, rejectReason);
		return;
	}

	markDownlinkRoomDirty(roomId);
}

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

void RoomService::heartbeatRegistry() {
	if (!registry_) return;
	registry_->heartbeat();
	auto roomIds = roomManager_.getRoomIds();
	if (!roomIds.empty())
		registry_->refreshRooms(roomIds);
	registry_->updateLoad(roomManager_.roomCount(), roomManager_.workerManager().maxTotalRouters());
}

json RoomService::resolveRoom(const std::string& roomId, const std::string& clientIp) {
	if (!registry_) return {{"wsUrl", ""}, {"isNew", true}};
	try {
		auto result = registry_->resolveRoom(roomId, clientIp);
		if (result.isNew && result.wsUrl.empty())
			return {{"error", "no available nodes"}, {"wsUrl", ""}, {"isNew", true}};
		return {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
	} catch (const std::exception& e) {
		MS_WARN(logger_, "resolveRoom failed ({}), degrading to local", e.what());
		return {{"wsUrl", ""}, {"isNew", true}};
	}
}

json RoomService::getNodeLoad() const {
	return {
		{"rooms", roomManager_.roomCount()},
		{"maxRooms", roomManager_.workerManager().maxTotalRouters()}
	};
}

json RoomService::getDefaultQosPolicy() const {
	return roomstatsqos::BuildDefaultQosPolicy();
}

void RoomService::maybeSendAutomaticQosOverride(
	const std::string& roomId, const std::string& peerId,
	const qos::PeerQosAggregate& aggregate)
{
	auto automatic = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	const std::string key = roomstatsqos::MakePeerKey(roomId, peerId);
	if (!automatic.has_value()) {
		auto it = autoQosOverrideRecords_.find(key);
		if (it != autoQosOverrideRecords_.end()) {
			if (notify_) {
				MS_INFO(
					logger_,
					"[{} {}] clearing automatic QoS override [{}]",
					roomId,
					peerId,
					SummarizePeerQosAggregate(aggregate)
				);
				notify_(roomId, peerId, {
					{"notification", true},
					{"method", "qosOverride"},
					{"data", roomstatsqos::BuildPeerOverrideClearPayload("server_auto_clear")}
				});
			}
			autoQosOverrideRecords_.erase(it);
		}
		return;
	}

	const auto nowMs = qos::NowMs();
	auto it = autoQosOverrideRecords_.find(key);
	if (it != autoQosOverrideRecords_.end()) {
		const auto refreshInterval = static_cast<int64_t>(it->second.ttlMs) / 2;
		if (it->second.signature == automatic->signature &&
			(nowMs - it->second.sentAtMs) < std::max<int64_t>(1000, refreshInterval)) {
			return;
		}
	}

	if (notify_) {
		MS_INFO(
			logger_,
			"[{} {}] sending automatic QoS override [{}] reason={} ttlMs={} forceAudioOnly={} maxLevelClamp={} disableRecovery={}",
			roomId,
			peerId,
			SummarizePeerQosAggregate(aggregate),
			automatic->overrideData.reason,
			automatic->overrideData.ttlMs,
			automatic->overrideData.hasForceAudioOnly ? (automatic->overrideData.forceAudioOnly ? "true" : "false") : "unset",
			automatic->overrideData.hasMaxLevelClamp ? std::to_string(automatic->overrideData.maxLevelClamp) : "unset",
			automatic->overrideData.hasDisableRecovery ? (automatic->overrideData.disableRecovery ? "true" : "false") : "unset"
		);
		notify_(roomId, peerId, {
			{"notification", true},
			{"method", "qosOverride"},
			{"data", qos::ToJson(automatic->overrideData)}
		});
	}

	autoQosOverrideRecords_[key] = {
		automatic->signature,
		nowMs,
		automatic->overrideData.ttlMs
	};
}

void RoomService::maybeNotifyConnectionQuality(
	const std::string& roomId, const std::string& peerId,
	const qos::PeerQosAggregate& aggregate)
{
	if (!notify_ || !aggregate.hasSnapshot) return;

	const std::string key = roomstatsqos::MakePeerKey(roomId, peerId);
	const json payload = roomstatsqos::BuildConnectionQualityPayload(aggregate);
	const std::string signature = payload.dump();
	auto it = lastConnectionQualitySignatures_.find(key);
	if (it != lastConnectionQualitySignatures_.end() && it->second == signature) return;

	notify_(roomId, peerId, {
		{"notification", true},
		{"method", "qosConnectionQuality"},
		{"data", payload}
	});
	lastConnectionQualitySignatures_[key] = signature;
}

void RoomService::maybeBroadcastRoomQosState(const std::string& roomId) {
	if (!broadcast_) return;
	auto room = roomManager_.getRoom(roomId);
	if (!room) return;

	std::vector<qos::PeerQosAggregate> peers;
	for (auto& peerId : room->getPeerIds()) {
		auto entry = qosRegistry_.Get(roomId, peerId);
		if (entry) peers.push_back(qos::QosAggregator::Aggregate(entry, qos::NowMs()));
	}
	auto roomAggregate = qos::QosRoomAggregator::Aggregate(peers);
	if (!roomAggregate.hasQos) {
		lastRoomQosStateSignatures_.erase(roomId);
		return;
	}

	const std::string signature = roomAggregate.data.dump();
	auto it = lastRoomQosStateSignatures_.find(roomId);
	if (it == lastRoomQosStateSignatures_.end() || it->second != signature) {
		broadcast_(roomId, "", {
			{"notification", true},
			{"method", "qosRoomState"},
			{"data", roomAggregate.data}
		});
		lastRoomQosStateSignatures_[roomId] = signature;
	}

	maybeSendRoomPressureOverrides(roomId, roomAggregate);
}

void RoomService::maybeSendRoomPressureOverrides(
	const std::string& roomId, const qos::RoomQosAggregate& aggregate)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room || !notify_) return;

	auto roomAutomatic = qos::QosOverrideBuilder::BuildForRoomAggregate(aggregate);
	for (auto& peerId : room->getPeerIds()) {
		const std::string key = roomstatsqos::MakeRoomPressureKey(roomId, peerId);
		if (!roomAutomatic.has_value()) {
			auto it = autoQosOverrideRecords_.find(key);
			if (it != autoQosOverrideRecords_.end()) {
				notify_(roomId, peerId, {
					{"notification", true},
					{"method", "qosOverride"},
					{"data", roomstatsqos::BuildPeerOverrideClearPayload("server_room_pressure_clear")}
				});
				autoQosOverrideRecords_.erase(it);
			}
			continue;
		}

		auto peerEntry = qosRegistry_.Get(roomId, peerId);
		bool peerAlreadyDegraded = false;
		if (peerEntry) {
			auto peerAggregate = qos::QosAggregator::Aggregate(peerEntry, qos::NowMs());
			peerAlreadyDegraded = roomstatsqos::IsPeerAlreadyDegraded(peerAggregate);
		}

		if (peerAlreadyDegraded) {
			auto it = autoQosOverrideRecords_.find(key);
			if (it != autoQosOverrideRecords_.end()) {
				notify_(roomId, peerId, {
					{"notification", true},
					{"method", "qosOverride"},
					{"data", roomstatsqos::BuildPeerOverrideClearPayload("server_room_pressure_clear")}
				});
				autoQosOverrideRecords_.erase(it);
			}
			continue;
		}

		auto it = autoQosOverrideRecords_.find(key);
		if (it != autoQosOverrideRecords_.end() && it->second.signature == roomAutomatic->signature) {
			continue;
		}

		notify_(roomId, peerId, {
			{"notification", true},
			{"method", "qosOverride"},
			{"data", qos::ToJson(roomAutomatic->overrideData)}
		});
		autoQosOverrideRecords_[key] = {
			roomAutomatic->signature,
			qos::NowMs(),
			roomAutomatic->overrideData.ttlMs
		};
	}
}

void RoomService::cleanupExpiredQosOverrides() {
	const auto nowMs = qos::NowMs();
	if (nowMs - lastOverrideCleanupMs_ < 1000) return;
	lastOverrideCleanupMs_ = nowMs;

	for (auto it = autoQosOverrideRecords_.begin(); it != autoQosOverrideRecords_.end(); ) {
		if (it->second.ttlMs == 0u) { ++it; continue; }
		const auto expiresAtMs = it->second.sentAtMs + static_cast<int64_t>(it->second.ttlMs);
		if (nowMs < expiresAtMs) { ++it; continue; }

		std::string roomId;
		std::string peerId;
		if (notify_ && roomstatsqos::ParseRoomPeerKey(it->first, roomId, peerId)) {
			notify_(roomId, peerId, {
				{"notification", true},
				{"method", "qosOverride"},
				{"data", roomstatsqos::BuildPeerOverrideClearPayload("server_ttl_expired")}
			});
		}
		it = autoQosOverrideRecords_.erase(it);
	}
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
