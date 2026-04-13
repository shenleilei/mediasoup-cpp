#pragma once
#include "qos/SubscriberBudgetAllocator.h"
#include "qos/DownlinkQosTypes.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mediasoup::qos {

struct ProducerDemandState {
	std::string producerId;
	std::string publisherPeerId;
	uint8_t maxSpatialLayer{ 0 };
	uint8_t maxTemporalLayer{ 0 };
	size_t visibleSubscriberCount{ 0 };
	size_t highPrioritySubscriberCount{ 0 };
	int64_t zeroDemandSinceMs{ 0 };
	int64_t holdUntilMs{ 0 };
};

class ProducerDemandAggregator {
public:
	static constexpr int64_t kHoldMs = 2000;

	void reset();
	void addSubscriberPlan(
		const std::string& subscriberPeerId,
		const DownlinkSnapshot& snapshot,
		const SubscriberBudgetPlan& plan);
	std::vector<ProducerDemandState> finalize(
		int64_t nowMs,
		const std::unordered_map<std::string, ProducerDemandState>& prev);

private:
	struct Accumulator {
		std::string producerId;
		uint8_t maxSpatialLayer{ 0 };
		uint8_t maxTemporalLayer{ 0 };
		size_t visibleSubscriberCount{ 0 };
		size_t highPrioritySubscriberCount{ 0 };
	};
	std::unordered_map<std::string, Accumulator> accumulators_;
};

} // namespace mediasoup::qos
