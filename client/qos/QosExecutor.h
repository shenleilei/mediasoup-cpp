#pragma once
#include "QosTypes.h"
#include <functional>
#include <iomanip>
#include <sstream>
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

	// Record idempotent key without calling sink (for async confirm path)
	void recordKey(const PlannedAction& action) {
		if (action.type == ActionType::Noop) return;
		lastKey_ = makeKey(action);
	}

private:
	static std::string makeKey(const PlannedAction& a) {
		std::string k = actionStr(a.type);
		k += ":" + std::to_string(a.level);
		if (a.encodingParameters.has_value()) {
			if (a.encodingParameters->maxBitrateBps.has_value())
				k += ":br" + std::to_string(*a.encodingParameters->maxBitrateBps);
			if (a.encodingParameters->maxFramerate.has_value())
				k += ":fps" + std::to_string(*a.encodingParameters->maxFramerate);
			if (a.encodingParameters->scaleResolutionDownBy.has_value()) {
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(3) << *a.encodingParameters->scaleResolutionDownBy;
				k += ":scale" + oss.str();
			}
			if (a.encodingParameters->adaptivePtime.has_value())
				k += std::string(":ptime") + (*a.encodingParameters->adaptivePtime ? "1" : "0");
		}
		if (a.spatialLayer.has_value())
			k += ":sl" + std::to_string(*a.spatialLayer);
		return k;
	}

	ActionSink sink_;
	std::string lastKey_;
};

} // namespace qos
