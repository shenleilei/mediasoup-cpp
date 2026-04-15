#pragma once
#include "QosTypes.h"
#include <functional>
#include <string>
#include <unordered_map>

namespace qos {

// ActionSink: external callback to apply encoding changes
using ActionSink = std::function<bool(const PlannedAction& action)>;

class QosActionExecutor {
public:
	explicit QosActionExecutor(ActionSink sink) : sink_(std::move(sink)) {}

	bool execute(const PlannedAction& action) {
		if (action.type == ActionType::Noop) return true;
		std::string key = makeKey(action);
		if (key == lastKey_) return true; // idempotent
		bool ok = sink_(action);
		if (ok) lastKey_ = key;
		return ok;
	}

	void executeMany(const std::vector<PlannedAction>& actions) {
		for (auto& a : actions) execute(a);
	}

	void reset() { lastKey_.clear(); }

private:
	static std::string makeKey(const PlannedAction& a) {
		std::string k = actionStr(a.type);
		k += ":" + std::to_string(a.level);
		if (a.encodingParameters.has_value() && a.encodingParameters->maxBitrateBps.has_value())
			k += ":br" + std::to_string(*a.encodingParameters->maxBitrateBps);
		if (a.spatialLayer.has_value())
			k += ":sl" + std::to_string(*a.spatialLayer);
		return k;
	}

	ActionSink sink_;
	std::string lastKey_;
};

} // namespace qos
