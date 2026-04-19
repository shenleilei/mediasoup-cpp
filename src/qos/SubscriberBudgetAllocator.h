#pragma once
#include "qos/DownlinkAllocator.h"
#include "qos/DownlinkQosTypes.h"
#include <vector>

namespace mediasoup::qos {

struct SubscriberBudgetPlan {
	double budgetBps{ 0.0 };
	std::vector<DownlinkAction> actions;
};

class SubscriberBudgetAllocator {
public:
	static constexpr double kAlpha = 0.85;
	static constexpr double kDefaultSafetyCap = 5'000'000.0; // 5 Mbps

	SubscriberBudgetPlan Allocate(
		const DownlinkSnapshot& snapshot,
		int degradeLevel) const;

private:
	double computeBudgetBps(const DownlinkSnapshot& snapshot) const;
	double computeUtility(const DownlinkSubscription& sub) const;
	double estimateLayerBitrateBps(const DownlinkSubscription& sub,
		uint8_t spatialLayer, uint8_t temporalLayer) const;
};

} // namespace mediasoup::qos
