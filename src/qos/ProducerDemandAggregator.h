#pragma once
#include "qos/SubscriberBudgetAllocator.h"
#include "qos/DownlinkQosTypes.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace mediasoup::qos {

enum class ProducerSupplyState {
	kActive,
	kLowClamped,
	kZeroDemandHold,
	kPaused,
	kResumeWarmup,
};

struct ProducerDemandState {
	std::string producerId;
	std::string publisherPeerId;
	uint8_t maxSpatialLayer{ 0 };
	uint8_t maxTemporalLayer{ 0 };
	size_t visibleSubscriberCount{ 0 };
	size_t highPrioritySubscriberCount{ 0 };
	int64_t zeroDemandSinceMs{ 0 };
	int64_t holdUntilMs{ 0 };
	// v3 state machine fields
	ProducerSupplyState supplyState{ ProducerSupplyState::kActive };
	int64_t lastNonZeroDemandAtMs{ 0 };
	int64_t pauseEligibleAtMs{ 0 };
	int64_t resumeEligibleAtMs{ 0 };
	int64_t lastPauseSentAtMs{ 0 };
	int64_t lastResumeSentAtMs{ 0 };
};

class ProducerDemandAggregator {
public:
	static constexpr int64_t kHoldMs = 2000;
	static constexpr int64_t kPauseConfirmMs = 4000;
	static constexpr int64_t kResumeWarmupMs = 1000;

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

	static ProducerSupplyState advanceSupplyState(
		const ProducerDemandState& state, bool zeroDemand, int64_t nowMs);
};

} // namespace mediasoup::qos
