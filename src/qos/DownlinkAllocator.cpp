#include "qos/DownlinkAllocator.h"
#include <unordered_set>

namespace mediasoup::qos {

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

	for (auto& action : fullActions) {
		auto it = lastState.find(action.consumerId);
		if (it != lastState.end()) {
			auto& prev = it->second;
			switch (action.type) {
			case DownlinkAction::Type::kPause:
				if (prev.paused) continue; // already paused
				break;
			case DownlinkAction::Type::kResume:
				if (!prev.paused) continue; // already resumed
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
	}

	// Update lastState to reflect the full desired state (not just what was emitted).
	std::unordered_set<std::string> activeConsumerIds;
	activeConsumerIds.reserve(subscriptions.size());
	for (size_t i = 0; i < subscriptions.size(); ++i) {
		const auto& sub = subscriptions[i];
		activeConsumerIds.insert(sub.consumerId);
		uint8_t priority = ComputePriority(sub);
		auto& st = lastState[sub.consumerId];
		st.priority = priority;
		if (!sub.visible) {
			st.paused = true;
			st.spatialLayer = 0;
			st.temporalLayer = 0;
		} else {
			bool wasPaused = (i < currentlyPaused.size()) ? currentlyPaused[i] : false;
			st.paused = false;

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
			st.spatialLayer = spatial;
			st.temporalLayer = temporal;
			(void)wasPaused; // used only by Compute for resume logic
		}
	}

	// Prune stale entries for consumers that no longer exist in current snapshot.
	for (auto it = lastState.begin(); it != lastState.end();) {
		if (activeConsumerIds.find(it->first) == activeConsumerIds.end())
			it = lastState.erase(it);
		else
			++it;
	}

	return filtered;
}

} // namespace mediasoup::qos
