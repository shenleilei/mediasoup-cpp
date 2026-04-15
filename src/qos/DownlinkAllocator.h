#pragma once
#include "qos/DownlinkQosTypes.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mediasoup::qos {

struct DownlinkAction {
	enum class Type { kNone, kPause, kResume, kSetLayers, kSetPriority };
	Type type{ Type::kNone };
	std::string consumerId;
	uint8_t spatialLayer{ 0 };
	uint8_t temporalLayer{ 0 };
	bool requestKeyFrame{ false };
	uint8_t priority{ 0 };
};

/// Tracks the last-applied state per consumer so that unchanged
/// actions can be suppressed to reduce control-plane noise.
struct ConsumerLastState {
	bool paused{ false };
	uint8_t spatialLayer{ 0 };
	uint8_t temporalLayer{ 0 };
	uint8_t priority{ 0 };
	bool hasLayerState{ false };
	int64_t lastLayerChangeAtMs{ 0 };
	int64_t lastUpgradeAtMs{ 0 };
	uint64_t layerSwitchCount{ 0 };
	uint64_t upgradeBlockedByCooldownCount{ 0 };
	uint64_t flappingEvents{ 0 };
};

class DownlinkAllocator {
public:
	static constexpr uint32_t kSmallTileMaxWidth = 320;
	static constexpr uint8_t kPriorityScreenShare = 255;
	static constexpr uint8_t kPriorityPinned = 220;
	static constexpr uint8_t kPriorityActiveSpeaker = 180;
	static constexpr uint8_t kPriorityVisibleGrid = 120;
	static constexpr uint8_t kPriorityHidden = 1;

	static uint8_t ComputePriority(const DownlinkSubscription& sub);

	/// Compute the full set of actions (always emitted regardless of prior state).
	static std::vector<DownlinkAction> Compute(
		const std::vector<DownlinkSubscription>& subscriptions,
		const std::vector<bool>& currentlyPaused,
		int degradeLevel = 0);

	/// Compute actions, but suppress any that would be redundant
	/// given the previously-applied state tracked in |lastState|.
	/// The map is updated in-place to reflect what was actually emitted.
	static std::vector<DownlinkAction> ComputeDiff(
		const std::vector<DownlinkSubscription>& subscriptions,
		const std::vector<bool>& currentlyPaused,
		int degradeLevel,
		std::unordered_map<std::string, ConsumerLastState>& lastState);

	/// Diff a pre-computed budget plan against lastState, suppressing
	/// redundant actions.  lastState is updated to reflect the plan.
	static std::vector<DownlinkAction> ComputeBudgetDiff(
		const std::vector<DownlinkAction>& planActions,
		std::unordered_map<std::string, ConsumerLastState>& lastState,
		int64_t nowMs = 0);
};

} // namespace mediasoup::qos
