#include "qos/DownlinkAllocator.h"

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

} // namespace mediasoup::qos
