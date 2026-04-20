#include "RoomService.h"

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

} // namespace

RoomService::Result RoomService::setClientStats(
	const std::string& roomId,
	const std::string& peerId,
	qos::ClientQosSnapshot stats)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room)
		return {false, json::object(), "", "room missing during clientStats"};

	auto peer = room->getPeer(peerId);
	if (!peer)
		return {false, json::object(), "", "peer missing during clientStats"};

	std::string rejectReason;
	if (!qosRegistry_.Upsert(roomId, peerId, stats, qos::NowMs(), &rejectReason)) {
		MS_DEBUG(logger_, "[{} {}] dropped QoS clientStats [{}]", roomId, peerId, rejectReason);
		return {true, roomstatsqos::BuildStatsStoreResponseData(false, rejectReason), "", ""};
	}

	auto qosEntry = qosRegistry_.Get(roomId, peerId);
	if (!qosEntry)
		return {false, json::object(), "", "clientStats stored entry missing"};

	auto aggregate = qos::QosAggregator::Aggregate(qosEntry, qos::NowMs());
	maybeNotifyConnectionQuality(roomId, peerId, aggregate);
	maybeSendAutomaticQosOverride(roomId, peerId, aggregate);
	maybeBroadcastRoomQosState(roomId);
	cleanupExpiredQosOverrides();
	return {true, roomstatsqos::BuildStatsStoreResponseData(true), "", ""};
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

void RoomService::maybeBroadcastRoomQosState(const std::string& roomId)
{
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

void RoomService::cleanupExpiredQosOverrides()
{
	const auto nowMs = qos::NowMs();
	if (nowMs - lastOverrideCleanupMs_ < 1000) return;
	lastOverrideCleanupMs_ = nowMs;

	for (auto it = autoQosOverrideRecords_.begin(); it != autoQosOverrideRecords_.end(); ) {
		if (it->second.ttlMs == 0u) {
			++it;
			continue;
		}
		const auto expiresAtMs =
			it->second.sentAtMs + static_cast<int64_t>(it->second.ttlMs);
		if (nowMs < expiresAtMs) {
			++it;
			continue;
		}

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

} // namespace mediasoup
