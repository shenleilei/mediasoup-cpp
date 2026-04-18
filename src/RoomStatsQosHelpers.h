#pragma once

#include "Logger.h"
#include "RoomManager.h"
#include "qos/DownlinkQosRegistry.h"
#include "qos/QosAggregator.h"
#include "qos/QosRegistry.h"
#include "qos/SubscriberQosController.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mediasoup::roomstatsqos {

inline std::string MakePeerKey(const std::string& roomId, const std::string& peerId)
{
	return roomId + "/" + peerId;
}

inline std::string MakeRoomPressureKey(const std::string& roomId, const std::string& peerId)
{
	return MakePeerKey(roomId, peerId) + "#room";
}

inline std::string MakeTrackOverrideKey(
	const std::string& roomId,
	const std::string& peerId,
	const std::string& trackId,
	const std::string& slot)
{
	return MakePeerKey(roomId, peerId) + "/" + trackId + "/" + slot;
}

inline json BuildDefaultQosPolicy()
{
	return {
		{"schema", qos::contract::kPolicySchema},
		{"sampleIntervalMs", 1000},
		{"snapshotIntervalMs", 2000},
		{"allowAudioOnly", true},
		{"allowVideoPause", true},
		{"profiles", {
			{"camera", "default"},
			{"screenShare", "clarity-first"},
			{"audio", "speech-first"}
		}}
	};
}

inline json BuildPeerOverrideClearPayload(const std::string& reason)
{
	return {
		{"schema", qos::contract::kOverrideSchema},
		{"scope", "peer"},
		{"trackId", nullptr},
		{"ttlMs", 0},
		{"reason", reason}
	};
}

inline json BuildConnectionQualityPayload(const qos::PeerQosAggregate& aggregate)
{
	return {
		{"quality", aggregate.quality},
		{"stale", aggregate.stale},
		{"lost", aggregate.lost},
		{"lastUpdatedMs", aggregate.lastUpdatedMs}
	};
}

inline bool ParseRoomPeerKey(
	const std::string& key,
	std::string& roomId,
	std::string& peerId)
{
	auto slash = key.find('/');
	if (slash == std::string::npos) {
		return false;
	}

	auto hash = key.find('#', slash);
	roomId = key.substr(0, slash);
	peerId = key.substr(
		slash + 1,
		hash == std::string::npos ? std::string::npos : hash - slash - 1);
	return true;
}

inline bool IsPeerAlreadyDegraded(const qos::PeerQosAggregate& aggregate)
{
	return aggregate.quality == "poor" ||
		aggregate.quality == "lost" ||
		aggregate.lost;
}

template<typename BudgetLeftFn, typename StatsTimeoutFn>
json BuildProducerStats(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<spdlog::logger>& logger,
	BudgetLeftFn budgetLeft,
	StatsTimeoutFn statsTimeout)
{
	json producers = json::object();
	if (!peer) {
		return producers;
	}

	for (const auto& [producerId, producer] : peer->producers) {
		json producerStats;
		if (budgetLeft() > 0) {
			try {
				producerStats["stats"] = producer->getStats(statsTimeout());
			} catch (const std::exception& e) {
				MS_WARN(logger, "[{} {}] producer {} getStats failed: {}",
					roomId, peerId, producerId, e.what());
				producerStats["stats"] = nullptr;
			} catch (...) {
				MS_WARN(logger, "[{} {}] producer {} getStats failed: unknown error",
					roomId, peerId, producerId);
				producerStats["stats"] = nullptr;
			}
		} else {
			producerStats["stats"] = nullptr;
		}

		json scores = json::array();
		for (const auto& score : producer->scores()) {
			scores.push_back({
				{"ssrc", score.ssrc},
				{"score", score.score},
				{"rid", score.rid}
			});
		}
		producerStats["scores"] = std::move(scores);
		producerStats["kind"] = producer->kind();
		producers[producerId] = std::move(producerStats);
	}

	return producers;
}

template<typename BudgetLeftFn, typename StatsTimeoutFn>
json BuildConsumerStats(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Room>& room,
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<spdlog::logger>& logger,
	BudgetLeftFn budgetLeft,
	StatsTimeoutFn statsTimeout)
{
	json consumers = json::object();
	if (!room || !peer) {
		return consumers;
	}

	for (const auto& [consumerId, consumer] : peer->consumers) {
		json consumerStats;
		consumerStats["kind"] = consumer->kind();
		consumerStats["type"] = consumer->type();
		consumerStats["producerId"] = consumer->producerId();
		consumerStats["paused"] = consumer->paused();
		consumerStats["producerPaused"] = consumer->producerPaused();
		consumerStats["preferredSpatialLayer"] = consumer->preferredSpatialLayer();
		consumerStats["preferredTemporalLayer"] = consumer->preferredTemporalLayer();
		consumerStats["priority"] = consumer->priority();

		if (budgetLeft() > 0) {
			try {
				auto stats = consumer->getStats(statsTimeout());
				if (!stats.empty() && stats[0].contains("score")) {
					consumerStats["score"] = stats[0]["score"];
				} else {
					consumerStats["score"] = consumer->currentScore().score;
				}
			} catch (const std::exception& e) {
				MS_WARN(logger, "[{} {}] consumer {} getStats failed: {}",
					roomId, peerId, consumerId, e.what());
				consumerStats["score"] = consumer->currentScore().score;
			} catch (...) {
				consumerStats["score"] = consumer->currentScore().score;
			}
		} else {
			consumerStats["score"] = consumer->currentScore().score;
		}

		auto producer = room->router()->getProducerById(consumer->producerId());
		if (producer && !producer->scores().empty()) {
			consumerStats["producerScore"] = producer->scores().back().score;
		} else {
			consumerStats["producerScore"] = consumer->currentScore().producerScore;
		}

		consumers[consumerId] = std::move(consumerStats);
	}

	return consumers;
}

inline void AppendPeerQosStats(
	json& result,
	const qos::QosRegistry& qosRegistry,
	const std::string& roomId,
	const std::string& peerId)
{
	auto qosEntry = qosRegistry.Get(roomId, peerId);
	if (!qosEntry) {
		return;
	}

	auto aggregate = qos::QosAggregator::Aggregate(qosEntry, qos::NowMs());
	result["clientStats"] = qos::ToJson(qosEntry->snapshot);
	result["qos"] = qos::ToJson(aggregate);
	result["qosStale"] = aggregate.stale;
	result["qosLastUpdatedMs"] = aggregate.lastUpdatedMs;
}

inline const char* DownlinkHealthToString(qos::DownlinkHealth health)
{
	switch (health) {
	case qos::DownlinkHealth::kStable:
		return "stable";
	case qos::DownlinkHealth::kEarlyWarning:
		return "early_warning";
	case qos::DownlinkHealth::kCongested:
		return "congested";
	case qos::DownlinkHealth::kRecovering:
		return "recovering";
	default:
		return "unknown";
	}
}

inline void AppendPeerDownlinkStats(
	json& result,
	const qos::DownlinkQosRegistry& downlinkQosRegistry,
	const std::unordered_map<std::string, qos::SubscriberQosController>& subscriberControllers,
	const std::string& roomId,
	const std::string& peerId)
{
	auto downlinkEntry = downlinkQosRegistry.Get(roomId, peerId);
	if (!downlinkEntry) {
		return;
	}

	const auto nowMs = qos::NowMs();
	result["downlinkClientStats"] = qos::ToJson(downlinkEntry->snapshot);
	result["downlinkLastUpdatedMs"] = downlinkEntry->receivedAtMs;
	result["downlinkAgeMs"] = nowMs - downlinkEntry->receivedAtMs;

	const auto controllerIt = subscriberControllers.find(MakePeerKey(roomId, peerId));
	if (controllerIt == subscriberControllers.end()) {
		return;
	}

	const auto& monitor = controllerIt->second.healthMonitor();
	result["downlinkQos"] = {
		{"health", DownlinkHealthToString(monitor.state())},
		{"degradeLevel", monitor.degradeLevel()}
	};
}

} // namespace mediasoup::roomstatsqos
