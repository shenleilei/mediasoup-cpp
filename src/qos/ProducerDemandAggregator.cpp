#include "qos/ProducerDemandAggregator.h"
#include <algorithm>
#include <unordered_set>

namespace mediasoup::qos {

void ProducerDemandAggregator::reset() {
	accumulators_.clear();
}

void ProducerDemandAggregator::addSubscriberPlan(
	const std::string& /*subscriberPeerId*/,
	const DownlinkSnapshot& snapshot,
	const SubscriberBudgetPlan& plan)
{
	// Build a map from consumerId -> allocated layer from the plan actions
	struct AllocatedLayer { uint8_t spatial{0}; uint8_t temporal{0}; bool paused{true}; };
	std::unordered_map<std::string, AllocatedLayer> layerMap;
	for (auto& a : plan.actions) {
		auto& entry = layerMap[a.consumerId];
		if (a.type == DownlinkAction::Type::kSetLayers) {
			entry.spatial = a.spatialLayer;
			entry.temporal = a.temporalLayer;
		} else if (a.type == DownlinkAction::Type::kResume) {
			entry.paused = false;
		} else if (a.type == DownlinkAction::Type::kPause) {
			entry.paused = true;
		}
	}

	for (auto& sub : snapshot.subscriptions) {
		auto& acc = accumulators_[sub.producerId];
		acc.producerId = sub.producerId;

		auto it = layerMap.find(sub.consumerId);
		if (it != layerMap.end() && !it->second.paused) {
			acc.maxSpatialLayer = std::max(acc.maxSpatialLayer, it->second.spatial);
			acc.maxTemporalLayer = std::max(acc.maxTemporalLayer, it->second.temporal);
			if (sub.visible) acc.visibleSubscriberCount++;
			if (sub.pinned || sub.isScreenShare) acc.highPrioritySubscriberCount++;
		}
	}
}

ProducerSupplyState ProducerDemandAggregator::advanceSupplyState(
	const ProducerDemandState& state, bool zeroDemand, int64_t nowMs)
{
	auto prev = state.supplyState;

	if (!zeroDemand) {
		// Demand is present
		if (prev == ProducerSupplyState::kPaused)
			return ProducerSupplyState::kResumeWarmup;
		if (prev == ProducerSupplyState::kResumeWarmup) {
			if (state.resumeEligibleAtMs != kUnsetDemandTimeMs &&
				nowMs >= state.resumeEligibleAtMs)
				return state.maxSpatialLayer < 2
					? ProducerSupplyState::kLowClamped
					: ProducerSupplyState::kActive;
			return ProducerSupplyState::kResumeWarmup;
		}
		return state.maxSpatialLayer < 2
			? ProducerSupplyState::kLowClamped
			: ProducerSupplyState::kActive;
	}

	// Zero demand
	switch (prev) {
	case ProducerSupplyState::kActive:
	case ProducerSupplyState::kLowClamped:
		return ProducerSupplyState::kZeroDemandHold;
	case ProducerSupplyState::kZeroDemandHold:
		if (state.pauseEligibleAtMs != kUnsetDemandTimeMs &&
			nowMs >= state.pauseEligibleAtMs)
			return ProducerSupplyState::kPaused;
		return ProducerSupplyState::kZeroDemandHold;
	case ProducerSupplyState::kPaused:
		return ProducerSupplyState::kPaused;
	case ProducerSupplyState::kResumeWarmup:
		// Was warming up but demand dropped again
		return ProducerSupplyState::kZeroDemandHold;
	}
	return ProducerSupplyState::kZeroDemandHold;
}

std::vector<ProducerDemandState> ProducerDemandAggregator::finalize(
	int64_t nowMs,
	const std::unordered_map<std::string, ProducerDemandState>& prev)
{
	std::vector<ProducerDemandState> result;
	result.reserve(accumulators_.size() + prev.size());

	std::unordered_set<std::string> seenProducerIds;
	seenProducerIds.reserve(accumulators_.size());
	for (auto& [pid, acc] : accumulators_) {
		ProducerDemandState state;
		state.producerId = acc.producerId;
		state.maxSpatialLayer = acc.maxSpatialLayer;
		state.maxTemporalLayer = acc.maxTemporalLayer;
		state.visibleSubscriberCount = acc.visibleSubscriberCount;
		state.highPrioritySubscriberCount = acc.highPrioritySubscriberCount;
		seenProducerIds.insert(pid);

		bool zeroDemand = (acc.visibleSubscriberCount == 0);
		auto prevIt = prev.find(pid);

		// Carry forward v3 state from previous cycle
		if (prevIt != prev.end()) {
			state.supplyState = prevIt->second.supplyState;
			state.lastNonZeroDemandAtMs = prevIt->second.lastNonZeroDemandAtMs;
			state.pauseEligibleAtMs = prevIt->second.pauseEligibleAtMs;
			state.resumeEligibleAtMs = prevIt->second.resumeEligibleAtMs;
			state.lastPauseSentAtMs = prevIt->second.lastPauseSentAtMs;
			state.lastResumeSentAtMs = prevIt->second.lastResumeSentAtMs;
		}

		if (zeroDemand) {
			if (prevIt != prev.end() && prevIt->second.zeroDemandSinceMs > 0)
				state.zeroDemandSinceMs = prevIt->second.zeroDemandSinceMs;
			else
				state.zeroDemandSinceMs = nowMs;
			state.holdUntilMs = state.zeroDemandSinceMs + kHoldMs;

			// Set pause eligibility on first zero-demand entry
			if (state.pauseEligibleAtMs == kUnsetDemandTimeMs)
				state.pauseEligibleAtMs = state.zeroDemandSinceMs + kPauseConfirmMs;
		} else {
			state.lastNonZeroDemandAtMs = nowMs;
			state.zeroDemandSinceMs = 0;
			state.holdUntilMs = 0;
			state.pauseEligibleAtMs = kUnsetDemandTimeMs;

			// Set resume eligibility when transitioning from paused
			if (prevIt != prev.end() &&
				prevIt->second.supplyState == ProducerSupplyState::kPaused &&
				state.resumeEligibleAtMs == kUnsetDemandTimeMs)
			{
				state.resumeEligibleAtMs = nowMs + kResumeWarmupMs;
			}
		}

		state.supplyState = advanceSupplyState(state, zeroDemand, nowMs);

		// Clear resume timer once warmup completes
		if (state.supplyState != ProducerSupplyState::kResumeWarmup)
			state.resumeEligibleAtMs = kUnsetDemandTimeMs;

		result.push_back(std::move(state));
	}

	// If a producer disappears from all current subscriptions, treat it as
	// zero-demand for one hold window so supply can decay instead of snapping.
	for (auto& [pid, prevState] : prev) {
		if (seenProducerIds.count(pid) > 0) continue;

		ProducerDemandState state;
		state.producerId = pid;
		state.publisherPeerId = prevState.publisherPeerId;
		state.zeroDemandSinceMs =
			prevState.zeroDemandSinceMs > 0 ? prevState.zeroDemandSinceMs : nowMs;
		state.holdUntilMs = state.zeroDemandSinceMs + kHoldMs;

		// Carry forward v3 state
		state.supplyState = prevState.supplyState;
		state.lastNonZeroDemandAtMs = prevState.lastNonZeroDemandAtMs;
		state.pauseEligibleAtMs = prevState.pauseEligibleAtMs;
		state.resumeEligibleAtMs = prevState.resumeEligibleAtMs;
		state.lastPauseSentAtMs = prevState.lastPauseSentAtMs;
		state.lastResumeSentAtMs = prevState.lastResumeSentAtMs;
		if (state.pauseEligibleAtMs == kUnsetDemandTimeMs)
			state.pauseEligibleAtMs = state.zeroDemandSinceMs + kPauseConfirmMs;
		state.supplyState = advanceSupplyState(state, /*zeroDemand=*/true, nowMs);

		// Keep in result if still in hold or paused (paused producers stay until explicitly removed)
		if (state.holdUntilMs > nowMs ||
			state.supplyState == ProducerSupplyState::kPaused)
		{
			result.push_back(std::move(state));
		}
	}
	return result;
}

} // namespace mediasoup::qos
