#pragma once
#include "Channel.h"
#include <spdlog/spdlog.h>
#include <vector>
#include <any>

namespace mediasoup {

// Extracts and validates the event and notification pointer from an EventEmitter
// args vector emitted by Channel. Throws std::invalid_argument on empty args.
// Performs std::any_cast<> for both event and owned notification, so callers
// should wrap this in a try/catch for std::bad_any_cast.
inline void extractNotificationArgs(const std::vector<std::any>& args,
	const char* owner, const std::string& id,
	FBS::Notification::Event& event,
	const FBS::Notification::Notification*& notif)
{
	notif = nullptr;
	if (args.empty()) {
		spdlog::warn("{} notification args empty [id:{}]", owner, id);
		throw std::invalid_argument("empty notification args");
	}
	event = std::any_cast<FBS::Notification::Event>(args[0]);
	if (args.size() > 1) {
		auto owned = std::any_cast<std::shared_ptr<Channel::OwnedNotification>>(args[1]);
		if (owned) notif = owned->notification();
	}
}

} // namespace mediasoup
