#include "qos/SubscriberQosController.h"

namespace mediasoup::qos {

void SubscriberQosController::applyActions(const std::vector<DownlinkAction>& actions,
	const std::unordered_map<std::string, std::shared_ptr<mediasoup::Consumer>>& consumers)
{
	for (const auto& action : actions) {
		auto it = consumers.find(action.consumerId);
		if (it == consumers.end()) continue;
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
				consumer->setPreferredLayers(action.spatialLayer, action.temporalLayer);
				if (action.requestKeyFrame)
					consumer->requestKeyFrame();
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
