#pragma once
#include "qos/DownlinkQosTypes.h"
#include <cstdint>
#include <string>
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

class DownlinkAllocator {
public:
	static constexpr uint32_t kSmallTileMaxWidth = 320;
	static constexpr uint8_t kPriorityScreenShare = 255;
	static constexpr uint8_t kPriorityPinned = 220;
	static constexpr uint8_t kPriorityActiveSpeaker = 180;
	static constexpr uint8_t kPriorityVisibleGrid = 120;
	static constexpr uint8_t kPriorityHidden = 1;

	static uint8_t ComputePriority(const DownlinkSubscription& sub);

	static std::vector<DownlinkAction> Compute(
		const std::vector<DownlinkSubscription>& subscriptions,
		const std::vector<bool>& currentlyPaused,
		int degradeLevel = 0);
};

} // namespace mediasoup::qos
