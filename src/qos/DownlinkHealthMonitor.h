#pragma once
#include "qos/DownlinkQosTypes.h"
#include <cstdint>
#include <string>

namespace mediasoup::qos {

enum class DownlinkHealth { kStable, kEarlyWarning, kCongested, kRecovering };

class DownlinkHealthMonitor {
public:
	struct Thresholds {
		double warnFreezeRate = 0.05;
		double congestedFreezeRate = 0.15;
		double warnLossRate = 0.03;
		double congestedLossRate = 0.08;
		double warnJitter = 0.03;
		double congestedJitter = 0.06;
		double warnRtt = 0.2;
		double congestedRtt = 0.4;
		double minFps = 10.0;
	};

	DownlinkHealth state() const { return state_; }
	int degradeLevel() const { return degradeLevel_; }

	// Returns true if degradeLevel changed
	bool update(const DownlinkSnapshot& snapshot);

	static constexpr int kMaxDegradeLevel = 3;

private:
	DownlinkHealth state_ = DownlinkHealth::kStable;
	int degradeLevel_ = 0;
	int stableCount_ = 0;
	Thresholds thresholds_;

	DownlinkHealth classify(const DownlinkSnapshot& snapshot) const;
};

} // namespace mediasoup::qos
