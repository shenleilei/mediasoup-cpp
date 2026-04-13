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

	DownlinkHealthMonitor& healthMonitor() { return healthMonitor_; }
	const DownlinkHealthMonitor& healthMonitor() const { return healthMonitor_; }

private:
	std::unordered_set<std::string> pausedConsumers_;
	DownlinkHealthMonitor healthMonitor_;
	std::shared_ptr<spdlog::logger> logger_ = Logger::Get("SubscriberQosController");
};

} // namespace mediasoup::qos
