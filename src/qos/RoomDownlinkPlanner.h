#pragma once
#include "qos/SubscriberBudgetAllocator.h"
#include "qos/ProducerDemandAggregator.h"
#include "qos/DownlinkQosTypes.h"
#include "qos/DownlinkAllocator.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace mediasoup::qos {

struct SubscriberPlanResult {
	std::string peerId;
	SubscriberBudgetPlan plan;
};

struct RoomDownlinkPlan {
	std::vector<SubscriberPlanResult> subscriberPlans;
	std::vector<ProducerDemandState> producerDemands;
};

struct SubscriberInput {
	std::string peerId;
	DownlinkSnapshot snapshot;
	int degradeLevel{ 0 };
	std::unordered_map<std::string, ConsumerLastState>* lastState{ nullptr };
};

class RoomDownlinkPlanner {
public:
	RoomDownlinkPlan PlanRoom(
		const std::vector<SubscriberInput>& subscribers,
		int64_t nowMs,
		const std::unordered_map<std::string, ProducerDemandState>& prevDemand);

private:
	SubscriberBudgetAllocator budgetAllocator_;
};

} // namespace mediasoup::qos
