#include "qos/DownlinkAllocator.h"

namespace mediasoup::qos {
namespace {

void ApplyActionToLastState(const DownlinkAction& action,
	std::unordered_map<std::string, ConsumerLastState>& lastState)
{
	auto& state = lastState[action.consumerId];
	switch (action.type) {
	case DownlinkAction::Type::kPause:
		state.paused = true;
		break;
	case DownlinkAction::Type::kResume:
		state.paused = false;
		break;
	case DownlinkAction::Type::kSetLayers:
		state.spatialLayer = action.spatialLayer;
		state.temporalLayer = action.temporalLayer;
		break;
	case DownlinkAction::Type::kSetPriority:
		state.priority = action.priority;
		break;
	case DownlinkAction::Type::kNone:
		break;
	}
}

} // namespace

uint8_t DownlinkAllocator::ComputePriority(const DownlinkSubscription& sub) {
	if (!sub.visible) return kPriorityHidden;
	if (sub.isScreenShare) return kPriorityScreenShare;
	if (sub.pinned) return kPriorityPinned;
	if (sub.activeSpeaker) return kPriorityActiveSpeaker;
	return kPriorityVisibleGrid;
}

std::vector<DownlinkAction> DownlinkAllocator::Compute(
	const std::vector<DownlinkSubscription>& subscriptions,
	const std::vector<bool>& currentlyPaused,
	int degradeLevel)
{
	std::vector<DownlinkAction> actions;
	for (size_t i = 0; i < subscriptions.size(); ++i) {
		const auto& sub = subscriptions[i];
		bool wasPaused = (i < currentlyPaused.size()) ? currentlyPaused[i] : false;
		uint8_t priority = ComputePriority(sub);

		if (!sub.visible) {
			if (!wasPaused) {
				actions.push_back({DownlinkAction::Type::kPause, sub.consumerId, 0, 0, false, priority});
			}
			actions.push_back({DownlinkAction::Type::kSetPriority, sub.consumerId, 0, 0, false, priority});
			continue;
		}

		if (sub.kind != "video") {
			if (wasPaused) {
				actions.push_back({DownlinkAction::Type::kResume, sub.consumerId, 0, 0, false, priority});
			}
			actions.push_back({DownlinkAction::Type::kSetPriority, sub.consumerId, 0, 0, false, priority});
			continue;
		}

		bool isLarge = sub.pinned || sub.isScreenShare || sub.targetWidth > kSmallTileMaxWidth;
		uint8_t spatial = isLarge ? 2 : 0;
		uint8_t temporal = isLarge ? 2 : 0;

		if (degradeLevel >= 3) {
			spatial = 0; temporal = 0;
		} else if (degradeLevel == 2) {
			spatial = std::min(spatial, uint8_t(1));
			temporal = std::min(temporal, uint8_t(1));
		} else if (degradeLevel == 1) {
			spatial = std::min(spatial, uint8_t(1));
			temporal = std::min(temporal, uint8_t(2));
		}

		if (wasPaused) {
			actions.push_back({DownlinkAction::Type::kResume, sub.consumerId, 0, 0, false, priority});
			actions.push_back({DownlinkAction::Type::kSetLayers, sub.consumerId, spatial, temporal, true, priority});
		} else {
			actions.push_back({DownlinkAction::Type::kSetLayers, sub.consumerId, spatial, temporal, false, priority});
		}
		actions.push_back({DownlinkAction::Type::kSetPriority, sub.consumerId, 0, 0, false, priority});
	}
	return actions;
}

std::vector<DownlinkAction> DownlinkAllocator::ComputeDiff(
	const std::vector<DownlinkSubscription>& subscriptions,
	const std::vector<bool>& currentlyPaused,
	int degradeLevel,
	std::unordered_map<std::string, ConsumerLastState>& lastState)
{
	auto fullActions = Compute(subscriptions, currentlyPaused, degradeLevel);
	std::vector<DownlinkAction> filtered;
	filtered.reserve(fullActions.size());
	std::unordered_map<std::string, bool> actualPaused;
	actualPaused.reserve(subscriptions.size());
	for (size_t i = 0; i < subscriptions.size(); ++i) {
		actualPaused[subscriptions[i].consumerId] =
			(i < currentlyPaused.size()) ? currentlyPaused[i] : false;
		auto& state = lastState[subscriptions[i].consumerId];
		state.paused = actualPaused[subscriptions[i].consumerId];
	}

	for (auto& action : fullActions) {
		auto it = lastState.find(action.consumerId);
		if (it != lastState.end()) {
			auto& prev = it->second;
			switch (action.type) {
			case DownlinkAction::Type::kPause:
				if (actualPaused[action.consumerId]) continue; // already paused
				break;
			case DownlinkAction::Type::kResume:
				if (!actualPaused[action.consumerId]) continue; // already resumed
				break;
			case DownlinkAction::Type::kSetLayers:
				if (!action.requestKeyFrame &&
					prev.spatialLayer == action.spatialLayer &&
					prev.temporalLayer == action.temporalLayer)
					continue; // layers unchanged
				break;
			case DownlinkAction::Type::kSetPriority:
				if (prev.priority == action.priority) continue; // priority unchanged
				break;
			case DownlinkAction::Type::kNone:
				continue;
			}
		}
		filtered.push_back(action);
		ApplyActionToLastState(action, lastState);
	}

	return filtered;
}

std::vector<DownlinkAction> DownlinkAllocator::ComputeBudgetDiff(
	const std::vector<DownlinkAction>& planActions,
	std::unordered_map<std::string, ConsumerLastState>& lastState)
{
	std::vector<DownlinkAction> filtered;
	filtered.reserve(planActions.size());

	// Build desired end-state per consumer from the plan
	std::unordered_map<std::string, ConsumerLastState> desired;
	std::unordered_map<std::string, bool> initiallyPaused;
	initiallyPaused.reserve(lastState.size());
	for (const auto& [consumerId, state] : lastState)
		initiallyPaused[consumerId] = state.paused;
	for (auto& a : planActions) {
		auto& d = desired[a.consumerId];
		switch (a.type) {
		case DownlinkAction::Type::kPause:
			d.paused = true; break;
		case DownlinkAction::Type::kResume:
			d.paused = false; break;
		case DownlinkAction::Type::kSetLayers:
			d.spatialLayer = a.spatialLayer;
			d.temporalLayer = a.temporalLayer; break;
		case DownlinkAction::Type::kSetPriority:
			d.priority = a.priority; break;
		case DownlinkAction::Type::kNone: break;
		}
	}

	// Diff against lastState
	for (auto& a : planActions) {
		auto action = a;
		auto it = lastState.find(a.consumerId);
		if (it != lastState.end()) {
			auto& prev = it->second;
			auto desiredIt = desired.find(a.consumerId);
			bool resumingFromPause =
				desiredIt != desired.end() &&
				initiallyPaused[a.consumerId] &&
				!desiredIt->second.paused;

			if (action.type == DownlinkAction::Type::kSetLayers && resumingFromPause)
				action.requestKeyFrame = true;

			switch (a.type) {
			case DownlinkAction::Type::kPause:
				if (prev.paused) continue; break;
			case DownlinkAction::Type::kResume:
				if (!prev.paused) continue; break;
			case DownlinkAction::Type::kSetLayers:
				if (!action.requestKeyFrame &&
					prev.spatialLayer == action.spatialLayer &&
					prev.temporalLayer == action.temporalLayer) continue; break;
			case DownlinkAction::Type::kSetPriority:
				if (prev.priority == action.priority) continue; break;
			case DownlinkAction::Type::kNone: continue;
			}
		}
		filtered.push_back(std::move(action));
		ApplyActionToLastState(filtered.back(), lastState);
	}

	return filtered;
}

} // namespace mediasoup::qos
