#pragma once
#include "qos/DownlinkAllocator.h"
#include "qos/DownlinkHealthMonitor.h"
#include "Consumer.h"
#include "Logger.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace mediasoup::qos {

class SubscriberQosController {
public:
	void applyActions(const std::vector<DownlinkAction>& actions,
		const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers);

	bool isPaused(const std::string& consumerId) const {
		return pausedConsumers_.count(consumerId) > 0;
	}

	std::vector<bool> getPausedFlags(
		const std::vector<DownlinkSubscription>& subscriptions) const;

	/// Remove paused-consumer entries whose IDs no longer appear in the
	/// current consumer map.  Call periodically to prevent unbounded growth
	/// of the set during long-lived sessions with frequent stream churn.
	void pruneStaleConsumers(
		const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers);

	size_t pausedCount() const { return pausedConsumers_.size(); }

	DownlinkHealthMonitor& healthMonitor() { return healthMonitor_; }
	const DownlinkHealthMonitor& healthMonitor() const { return healthMonitor_; }

	/// Access the per-consumer last-state map used by ComputeDiff.
	std::unordered_map<std::string, ConsumerLastState>& lastState() { return lastState_; }

private:
	std::unordered_set<std::string> pausedConsumers_;
	std::unordered_map<std::string, ConsumerLastState> lastState_;
	DownlinkHealthMonitor healthMonitor_;
	std::shared_ptr<spdlog::logger> logger_ = Logger::Get("SubscriberQosController");
};

} // namespace mediasoup::qos
