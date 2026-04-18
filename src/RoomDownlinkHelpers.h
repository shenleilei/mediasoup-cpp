#pragma once

#include "RoomManager.h"
#include "RoomStatsQosHelpers.h"
#include "qos/DownlinkAllocator.h"
#include "qos/DownlinkQosRegistry.h"
#include "qos/ProducerDemandAggregator.h"
#include "qos/QosRegistry.h"
#include "qos/RoomDownlinkPlanner.h"
#include "qos/SubscriberQosController.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mediasoup::roomdownlink {

struct PlanningInputs {
	std::vector<qos::SubscriberInput> subscriberInputs;
	std::unordered_set<std::string> staleProducerIds;
};

inline PlanningInputs CollectPlanningInputs(
	const std::string& roomId,
	const std::shared_ptr<Room>& room,
	const qos::DownlinkQosRegistry& downlinkQosRegistry,
	std::unordered_map<std::string, qos::SubscriberQosController>& subscriberControllers,
	int64_t nowMs)
{
	PlanningInputs inputs;
	if (!room) {
		return inputs;
	}

	for (const auto& peerId : room->getPeerIds()) {
		auto* entry = downlinkQosRegistry.Get(roomId, peerId);
		if (!entry) {
			continue;
		}

		if (nowMs - entry->receivedAtMs > qos::kQosStaleAfterMs) {
			for (const auto& subscription : entry->snapshot.subscriptions) {
				inputs.staleProducerIds.insert(subscription.producerId);
			}
			continue;
		}

		auto& controller = subscriberControllers[roomstatsqos::MakePeerKey(roomId, peerId)];
		auto peer = room->getPeer(peerId);
		if (peer) {
			controller.pruneStaleConsumers(peer->consumers);
			controller.syncConsumerState(peer->consumers);
		}
		controller.healthMonitor().update(entry->snapshot);

		qos::SubscriberInput input;
		input.peerId = peerId;
		input.snapshot = entry->snapshot;
		input.degradeLevel = controller.healthMonitor().degradeLevel();
		inputs.subscriberInputs.push_back(std::move(input));
	}

	return inputs;
}

inline std::vector<qos::ProducerDemandState> DecayAndReindexDemandStates(
	int64_t nowMs,
	std::unordered_map<std::string, qos::ProducerDemandState>& prevDemand)
{
	qos::ProducerDemandAggregator aggregator;
	auto demandStates = aggregator.finalize(nowMs, prevDemand);
	prevDemand.clear();
	for (const auto& state : demandStates) {
		prevDemand[state.producerId] = state;
	}
	return demandStates;
}

inline std::vector<qos::ProducerDemandState> MergeStaleProducerDemands(
	std::vector<qos::ProducerDemandState> effectiveDemandStates,
	const std::unordered_set<std::string>& staleProducerIds,
	const std::unordered_map<std::string, qos::ProducerDemandState>& prevDemand)
{
	std::unordered_set<std::string> plannedProducerIds;
	plannedProducerIds.reserve(effectiveDemandStates.size());
	for (const auto& demand : effectiveDemandStates) {
		plannedProducerIds.insert(demand.producerId);
	}

	for (const auto& producerId : staleProducerIds) {
		if (plannedProducerIds.count(producerId) > 0) {
			continue;
		}
		auto prevIt = prevDemand.find(producerId);
		if (prevIt == prevDemand.end()) {
			continue;
		}
		effectiveDemandStates.push_back(prevIt->second);
	}

	return effectiveDemandStates;
}

inline void ApplySubscriberPlans(
	const std::string& roomId,
	const std::shared_ptr<Room>& room,
	const std::vector<qos::SubscriberPlanResult>& subscriberPlans,
	std::unordered_map<std::string, qos::SubscriberQosController>& subscriberControllers)
{
	if (!room) {
		return;
	}

	for (const auto& subscriberPlan : subscriberPlans) {
		auto& controller = subscriberControllers[roomstatsqos::MakePeerKey(roomId, subscriberPlan.peerId)];
		auto peer = room->getPeer(subscriberPlan.peerId);
		if (!peer) {
			continue;
		}

		auto diffActions = qos::DownlinkAllocator::ComputeBudgetDiff(
			subscriberPlan.plan.actions,
			controller.lastState());
		controller.applyActions(diffActions, peer->consumers);
	}
}

inline std::optional<std::string> ResolveProducerOwnerPeerId(
	const std::string& roomId,
	const std::string& producerId,
	const std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& producerOwnerPeerIds,
	const std::shared_ptr<Room>& room)
{
	auto roomIt = producerOwnerPeerIds.find(roomId);
	if (roomIt != producerOwnerPeerIds.end()) {
		auto ownerIt = roomIt->second.find(producerId);
		if (ownerIt != roomIt->second.end()) {
			return ownerIt->second;
		}
	}

	if (!room) {
		return std::nullopt;
	}

	for (const auto& peerId : room->getPeerIds()) {
		auto peer = room->getPeer(peerId);
		if (peer && peer->producers.count(producerId)) {
			return peerId;
		}
	}

	return std::nullopt;
}

inline std::optional<std::string> ResolvePublisherTrackId(
	const qos::QosRegistry& qosRegistry,
	const std::string& roomId,
	const std::string& publisherPeerId,
	const std::string& producerId)
{
	auto* entry = qosRegistry.Get(roomId, publisherPeerId);
	if (!entry) {
		return std::nullopt;
	}

	for (const auto& track : entry->snapshot.tracks) {
		if (track.producerId == producerId) {
			return track.localTrackId;
		}
	}

	return std::nullopt;
}

inline const char* ResolveOverrideSlot(const qos::QosOverride& overrideData)
{
	if (overrideData.hasPauseUpstream ||
		overrideData.reason == "downlink_v3_zero_demand_pause") {
		return "pauseUpstream";
	}
	if (overrideData.hasResumeUpstream ||
		overrideData.reason == "downlink_v3_demand_resumed") {
		return "resumeUpstream";
	}
	return "maxLevelClamp";
}

template<typename TrackOverrideRecordMap>
void CleanupExpiredTrackOverrideRecords(
	TrackOverrideRecordMap& trackQosOverrideRecords,
	int64_t nowMs)
{
	for (auto it = trackQosOverrideRecords.begin(); it != trackQosOverrideRecords.end(); ) {
		if (it->second.ttlMs > 0 &&
			nowMs - it->second.sentAtMs > static_cast<int64_t>(it->second.ttlMs)) {
			it = trackQosOverrideRecords.erase(it);
		} else {
			++it;
		}
	}
}

template<typename TrackOverrideRecordMap>
void CleanupPeerTrackOverrideRecords(
	const std::string& roomId,
	const std::string& peerId,
	TrackOverrideRecordMap& trackQosOverrideRecords)
{
	for (auto it = trackQosOverrideRecords.begin(); it != trackQosOverrideRecords.end(); ) {
		if (it->second.roomId == roomId && it->second.peerId == peerId) {
			it = trackQosOverrideRecords.erase(it);
		} else {
			++it;
		}
	}
}

template<typename CacheMap>
void CleanupIndexedProducers(
	const std::string& roomId,
	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers,
	CacheMap& cache)
{
	auto roomIt = cache.find(roomId);
	if (roomIt == cache.end()) {
		return;
	}

	for (const auto& [producerId, _] : producers) {
		roomIt->second.erase(producerId);
	}

	if (roomIt->second.empty()) {
		cache.erase(roomIt);
	}
}

inline void IndexPeerProducers(
	const std::string& roomId,
	const std::string& peerId,
	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers,
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>>& producerOwnerPeerIds)
{
	auto& owners = producerOwnerPeerIds[roomId];
	for (const auto& [producerId, _] : producers) {
		owners[producerId] = peerId;
	}
}

} // namespace mediasoup::roomdownlink
