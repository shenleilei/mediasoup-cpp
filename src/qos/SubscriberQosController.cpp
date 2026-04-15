#include "qos/SubscriberQosController.h"

namespace mediasoup::qos {

void SubscriberQosController::pruneStaleConsumers(
	const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers)
{
	for (auto it = pausedConsumers_.begin(); it != pausedConsumers_.end(); ) {
		if (consumers.find(*it) == consumers.end())
			it = pausedConsumers_.erase(it);
		else
			++it;
	}
	for (auto it = lastState_.begin(); it != lastState_.end(); ) {
		if (consumers.find(it->first) == consumers.end())
			it = lastState_.erase(it);
		else
			++it;
	}
}

void SubscriberQosController::syncConsumerState(
	const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers)
{
	for (const auto& [consumerId, consumer] : consumers) {
		if (!consumer) continue;
		auto& state = lastState_[consumerId];
		state.paused = consumer->paused();
		state.spatialLayer = consumer->preferredSpatialLayer();
		state.temporalLayer = consumer->preferredTemporalLayer();
		state.priority = consumer->priority();
		if (consumer->paused())
			pausedConsumers_.insert(consumerId);
		else
			pausedConsumers_.erase(consumerId);
	}
}

void SubscriberQosController::applyActions(const std::vector<DownlinkAction>& actions,
	const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers)
{
	for (const auto& action : actions) {
		auto it = consumers.find(action.consumerId);
		if (it == consumers.end()) {
			// Consumer gone — drop any stale paused state.
			pausedConsumers_.erase(action.consumerId);
			continue;
		}
		auto& consumer = it->second;

		try {
			switch (action.type) {
			case DownlinkAction::Type::kPause:
				consumer->pause();
				pausedConsumers_.insert(action.consumerId);
				break;
			case DownlinkAction::Type::kResume:
				consumer->resume();
				pausedConsumers_.erase(action.consumerId);
				break;
			case DownlinkAction::Type::kSetLayers:
				if (consumer->kind() == "video") {
					consumer->setPreferredLayers(action.spatialLayer, action.temporalLayer);
					if (action.requestKeyFrame)
						consumer->requestKeyFrame();
				}
				break;
			case DownlinkAction::Type::kSetPriority:
				consumer->setPriority(action.priority);
				break;
			case DownlinkAction::Type::kNone:
				break;
			}
		} catch (const std::exception& e) {
			MS_WARN(logger_, "SubscriberQosController action failed [consumer:{}]: {}",
				action.consumerId, e.what());
		}
	}
}

std::vector<bool> SubscriberQosController::getPausedFlags(
	const std::vector<DownlinkSubscription>& subscriptions) const
{
	std::vector<bool> flags;
	flags.reserve(subscriptions.size());
	for (const auto& sub : subscriptions)
		flags.push_back(pausedConsumers_.count(sub.consumerId) > 0);
	return flags;
}

} // namespace mediasoup::qos
