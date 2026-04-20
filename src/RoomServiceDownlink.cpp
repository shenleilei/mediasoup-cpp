#include "RoomService.h"

#include "RoomDownlinkHelpers.h"
#include "RoomStatsQosHelpers.h"

namespace mediasoup {
namespace {

json BuildStatsStoreResponseData(bool stored, std::string reason = "")
{
	json data = {
		{"stored", stored}
	};
	if (!reason.empty())
		data["reason"] = std::move(reason);
	return data;
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

RoomService::Result RoomService::setDownlinkClientStats(
	const std::string& roomId,
	const std::string& peerId,
	qos::DownlinkSnapshot stats)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room)
		return {false, json::object(), "", "room missing during downlinkClientStats"};

	auto peer = room->getPeer(peerId);
	if (!peer)
		return {false, json::object(), "", "peer missing during downlinkClientStats"};

	if (stats.subscriberPeerId != peerId) {
		MS_WARN(logger_, "[{} {}] downlink subscriberPeerId mismatch: {}", roomId, peerId, stats.subscriberPeerId);
		return {false, json::object(), "", "downlink subscriberPeerId mismatch"};
	}
	const auto droppedSubscriptions =
		FilterConsistentDownlinkSubscriptions(peer, &stats);
	if (droppedSubscriptions > 0) {
		MS_WARN(logger_,
			"[{} {}] dropped {} inconsistent downlink subscriptions",
			roomId, peerId, droppedSubscriptions);
	}
	std::string rejectReason;
	if (!downlinkQosRegistry_.Upsert(roomId, peerId, stats, qos::NowMs(), &rejectReason)) {
		MS_DEBUG(logger_, "[{} {}] dropped downlink stats [{}]", roomId, peerId, rejectReason);
		return {true, BuildStatsStoreResponseData(false, rejectReason), "", ""};
	}

	markDownlinkRoomDirty(roomId);
	return {true, BuildStatsStoreResponseData(true), "", ""};
}

void RoomService::markDownlinkRoomDirty(const std::string& roomId) {
	if (dirtyDownlinkRooms_.insert(roomId).second)
		pendingDownlinkRooms_.push_back(roomId);
	if (!downlinkPlanningActive_) {
		downlinkPlanningActive_ = true;
		scheduleDownlinkPlanning(/*delayMs=*/0);
		return;
	}

	// If the planner is currently sleeping for a deferred room, wake it up so
	// one-shot events (leave/reconnect) are not stranded until the next stats push.
	if (downlinkPlanningNextWakeAtMs_ > 0)
		scheduleDownlinkPlanning(/*delayMs=*/0);
}

void RoomService::scheduleDownlinkPlanning(uint32_t delayMs) {
	const uint64_t token = ++downlinkPlanningScheduleToken_;
	downlinkPlanningNextWakeAtMs_ =
		delayMs > 0 ? qos::NowMs() + static_cast<int64_t>(delayMs) : 0;

	auto task = [this, token] {
		if (!downlinkPlanningActive_ || token != downlinkPlanningScheduleToken_)
			return;
		downlinkPlanningNextWakeAtMs_ = 0;
		continueDownlinkPlanning();
	};

	if (delayMs > 0 && delayedTaskPoster_) {
		delayedTaskPoster_(std::move(task), delayMs);
	} else if (taskPoster_) {
		taskPoster_(std::move(task));
	} else {
		task();
	}
}

void RoomService::continueDownlinkPlanning() {
	if (pendingDownlinkRooms_.empty()) {
		downlinkPlanningActive_ = false;
		downlinkPlanningNextWakeAtMs_ = 0;
		return;
	}

	const int64_t nowMs = qos::NowMs();
	int64_t minDeferredDelayMs = 0;
	std::deque<std::string> deferredRooms;

	while (!pendingDownlinkRooms_.empty()) {
		std::string roomId = std::move(pendingDownlinkRooms_.front());
		pendingDownlinkRooms_.pop_front();
		dirtyDownlinkRooms_.erase(roomId);

		auto& planState = downlinkRoomPlanStates_[roomId];
		if (nowMs < planState.nextEligiblePlanAtMs) {
			const int64_t delayMs = std::max<int64_t>(1, planState.nextEligiblePlanAtMs - nowMs);
			if (minDeferredDelayMs == 0 || delayMs < minDeferredDelayMs)
				minDeferredDelayMs = delayMs;
			deferredRooms.push_back(std::move(roomId));
			continue;
		}

		try {
			runDownlinkPlanningForRoom(roomId);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{}] downlink planning failed: {}", roomId, e.what());
		} catch (...) {
			MS_WARN(logger_, "[{}] downlink planning failed: unknown error", roomId);
		}

		planState.lastPlannedAtMs = nowMs;
		planState.nextEligiblePlanAtMs = nowMs + downlinkPlanningIntervalMs_;
	}

	for (auto& roomId : deferredRooms) {
		dirtyDownlinkRooms_.insert(roomId);
		pendingDownlinkRooms_.push_back(std::move(roomId));
	}

	if (pendingDownlinkRooms_.empty()) {
		downlinkPlanningActive_ = false;
		downlinkPlanningNextWakeAtMs_ = 0;
		return;
	}

	scheduleDownlinkPlanning(static_cast<uint32_t>(minDeferredDelayMs));
}

void RoomService::runDownlinkPlanningForRoom(const std::string& roomId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return;

	const int64_t nowMs = qos::NowMs();
	auto planningInputs = roomdownlink::CollectPlanningInputs(
		roomId,
		room,
		downlinkQosRegistry_,
		subscriberControllers_,
		nowMs);
	auto& prevDemand = producerDemandCache_[roomId];
	if (planningInputs.subscriberInputs.empty()) {
		if (prevDemand.empty()) return;
		auto decayedDemandStates = roomdownlink::DecayAndReindexDemandStates(nowMs, prevDemand);
		applyPublisherSupplyPlan(roomId, decayedDemandStates);
		return;
	}

	qos::RoomDownlinkPlanner planner;
	auto plan = planner.PlanRoom(planningInputs.subscriberInputs, nowMs, prevDemand);
	auto effectiveDemandStates = roomdownlink::MergeStaleProducerDemands(
		plan.producerDemands,
		planningInputs.staleProducerIds,
		prevDemand);

	roomdownlink::ApplySubscriberPlans(
		roomId,
		room,
		plan.subscriberPlans,
		subscriberControllers_);

	prevDemand.clear();
	for (const auto& demand : effectiveDemandStates)
		prevDemand[demand.producerId] = demand;

	applyPublisherSupplyPlan(roomId, effectiveDemandStates);

	if (downlinkSnapshotApplied_) {
		for (const auto& input : planningInputs.subscriberInputs)
			downlinkSnapshotApplied_(roomId, input.peerId, input.snapshot.seq);
	}
}

std::optional<std::string> RoomService::resolveProducerOwnerPeerId(
	const std::string& roomId, const std::string& producerId) const
{
	auto room = roomManager_.getRoom(roomId);
	return roomdownlink::ResolveProducerOwnerPeerId(
		roomId,
		producerId,
		producerOwnerPeerIds_,
		room);
}

std::optional<std::string> RoomService::resolvePublisherTrackId(
	const std::string& roomId, const std::string& publisherPeerId,
	const std::string& producerId) const
{
	return roomdownlink::ResolvePublisherTrackId(
		qosRegistry_,
		roomId,
		publisherPeerId,
		producerId);
}

void RoomService::applyPublisherSupplyPlan(const std::string& roomId,
	const std::vector<qos::ProducerDemandState>& demandStates)
{
	cleanupExpiredTrackQosOverrides();

	auto room = roomManager_.getRoom(roomId);
	qos::PublisherSupplyController supplyCtrl;
	auto overrides = supplyCtrl.BuildOverrides(
		demandStates,
		[&](const std::string& producerId) -> std::optional<std::pair<std::string, std::string>> {
			auto ownerPeerId = roomdownlink::ResolveProducerOwnerPeerId(
				roomId,
				producerId,
				producerOwnerPeerIds_,
				room);
			if (!ownerPeerId) return std::nullopt;
			auto trackId = roomdownlink::ResolvePublisherTrackId(
				qosRegistry_,
				roomId,
				*ownerPeerId,
				producerId);
			if (!trackId) return std::nullopt;
			return std::make_pair(*ownerPeerId, *trackId);
		},
		qos::NowMs());

	for (auto& to : overrides)
		maybeSendTrackQosOverride(roomId, to.targetPeerId, to.overrideData);
}

void RoomService::maybeSendTrackQosOverride(const std::string& roomId,
	const std::string& targetPeerId, const qos::QosOverride& overrideData)
{
	std::string key = roomstatsqos::MakeTrackOverrideKey(
		roomId,
		targetPeerId,
		overrideData.trackId,
		roomdownlink::ResolveOverrideSlot(overrideData));
	std::string sig = overrideData.raw.dump();
	int64_t now = qos::NowMs();

	auto it = trackQosOverrideRecords_.find(key);
	if (it != trackQosOverrideRecords_.end()) {
		bool expired = it->second.ttlMs > 0 &&
			(now - it->second.sentAtMs > static_cast<int64_t>(it->second.ttlMs));
		if (!expired && it->second.signature == sig)
			return;
	}

	trackQosOverrideRecords_[key] = TrackQosOverrideRecord{
		sig, roomId, targetPeerId, overrideData.trackId, now, overrideData.ttlMs
	};

	if (notify_) {
		notify_(roomId, targetPeerId, {
			{"notification", true}, {"method", "qosOverride"},
			{"data", overrideData.raw}
		});
	}
}

void RoomService::cleanupExpiredTrackQosOverrides() {
	roomdownlink::CleanupExpiredTrackOverrideRecords(trackQosOverrideRecords_, qos::NowMs());
}

void RoomService::cleanupPeerTrackQosOverrides(const std::string& roomId,
	const std::string& peerId)
{
	roomdownlink::CleanupPeerTrackOverrideRecords(roomId, peerId, trackQosOverrideRecords_);
}

void RoomService::cleanupPeerProducerDemandCache(const std::string& roomId,
	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers)
{
	roomdownlink::CleanupIndexedProducers(roomId, producers, producerDemandCache_);
}

void RoomService::indexPeerProducers(const std::string& roomId, const std::string& peerId,
	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers)
{
	roomdownlink::IndexPeerProducers(roomId, peerId, producers, producerOwnerPeerIds_);
}

void RoomService::cleanupPeerProducerOwnerCache(const std::string& roomId,
	const std::unordered_map<std::string, std::shared_ptr<Producer>>& producers)
{
	roomdownlink::CleanupIndexedProducers(roomId, producers, producerOwnerPeerIds_);
}

} // namespace mediasoup
