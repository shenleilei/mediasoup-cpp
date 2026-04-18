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

bool IsRedundantAction(const DownlinkAction& action, const ConsumerLastState& state)
{
	switch (action.type) {
	case DownlinkAction::Type::kPause:
		return state.paused;
	case DownlinkAction::Type::kResume:
		return !state.paused;
	case DownlinkAction::Type::kSetLayers:
		return !action.requestKeyFrame &&
			state.spatialLayer == action.spatialLayer &&
			state.temporalLayer == action.temporalLayer;
	case DownlinkAction::Type::kSetPriority:
		return state.priority == action.priority;
	case DownlinkAction::Type::kNone:
		return true;
	}

	return true;
}

void SyncPauseState(const std::vector<DownlinkSubscription>& subscriptions,
	const std::vector<bool>& currentlyPaused,
	std::unordered_map<std::string, ConsumerLastState>& lastState)
{
	for (size_t i = 0; i < subscriptions.size(); ++i) {
		auto& state = lastState[subscriptions[i].consumerId];
		state.paused = (i < currentlyPaused.size()) ? currentlyPaused[i] : false;
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
	SyncPauseState(subscriptions, currentlyPaused, lastState);

	for (const auto& action : fullActions) {
		auto it = lastState.find(action.consumerId);
		if (it != lastState.end() && IsRedundantAction(action, it->second)) {
			continue;
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

	const auto initialState = lastState;
	auto desiredState = initialState;
	for (const auto& action : planActions) {
		ApplyActionToLastState(action, desiredState);
	}

	for (const auto& plannedAction : planActions) {
		auto action = plannedAction;
		if (action.type == DownlinkAction::Type::kSetLayers) {
			auto initialIt = initialState.find(action.consumerId);
			auto desiredIt = desiredState.find(action.consumerId);
			bool resumingFromPause =
				desiredIt != desiredState.end() &&
				initialIt != initialState.end() &&
				initialIt->second.paused &&
				!desiredIt->second.paused;

			if (resumingFromPause) {
				action.requestKeyFrame = true;
			}
		}

		auto it = lastState.find(action.consumerId);
		if (it != lastState.end() && IsRedundantAction(action, it->second)) {
			continue;
		}
		filtered.push_back(std::move(action));
		ApplyActionToLastState(filtered.back(), lastState);
	}

	return filtered;
}

} // namespace mediasoup::qos
