#include "qos/RoomDownlinkPlanner.h"

namespace mediasoup::qos {

RoomDownlinkPlan RoomDownlinkPlanner::PlanRoom(
	const std::vector<SubscriberInput>& subscribers,
	int64_t nowMs,
	const std::unordered_map<std::string, ProducerDemandState>& prevDemand)
{
	RoomDownlinkPlan result;
	ProducerDemandAggregator aggregator;

	for (auto& sub : subscribers) {
		auto plan = budgetAllocator_.Allocate(sub.snapshot, sub.degradeLevel);
		aggregator.addSubscriberPlan(sub.peerId, sub.snapshot, plan);
		result.subscriberPlans.push_back({ sub.peerId, std::move(plan) });
	}

	result.producerDemands = aggregator.finalize(nowMs, prevDemand);
	return result;
}

} // namespace mediasoup::qos
