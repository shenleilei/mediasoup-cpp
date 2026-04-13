#include "qos/DownlinkHealthMonitor.h"
#include <algorithm>

namespace mediasoup::qos {

DownlinkHealth DownlinkHealthMonitor::classify(const DownlinkSnapshot& snapshot) const {
	// Aggregate worst signals across all subscriptions
	double worstFreeze = 0, worstJitter = 0, worstLoss = 0, lowestFps = 999;
	for (const auto& sub : snapshot.subscriptions) {
		if (!sub.visible) continue;
		worstFreeze = std::max(worstFreeze, sub.freezeRate);
		worstJitter = std::max(worstJitter, sub.jitter);
		if (sub.framesPerSecond > 0) lowestFps = std::min(lowestFps, sub.framesPerSecond);
		// Normalize packetsLost as a rate approximation (per snapshot interval)
		double lossRate = sub.packetsLost > 0 ? std::min(1.0, sub.packetsLost / 100.0) : 0;
		worstLoss = std::max(worstLoss, lossRate);
	}
	double rtt = snapshot.currentRoundTripTime;

	if (worstFreeze >= thresholds_.congestedFreezeRate ||
		worstLoss >= thresholds_.congestedLossRate ||
		worstJitter >= thresholds_.congestedJitter ||
		rtt >= thresholds_.congestedRtt) {
		return DownlinkHealth::kCongested;
	}
	if (worstFreeze >= thresholds_.warnFreezeRate ||
		worstLoss >= thresholds_.warnLossRate ||
		worstJitter >= thresholds_.warnJitter ||
		rtt >= thresholds_.warnRtt ||
		(lowestFps < thresholds_.minFps && lowestFps < 999)) {
		return DownlinkHealth::kEarlyWarning;
	}
	return DownlinkHealth::kStable;
}

bool DownlinkHealthMonitor::update(const DownlinkSnapshot& snapshot) {
	int prevLevel = degradeLevel_;
	auto observed = classify(snapshot);

	switch (state_) {
	case DownlinkHealth::kStable:
		if (observed == DownlinkHealth::kCongested) {
			state_ = DownlinkHealth::kCongested;
			degradeLevel_ = std::min(degradeLevel_ + 2, kMaxDegradeLevel);
			stableCount_ = 0;
		} else if (observed == DownlinkHealth::kEarlyWarning) {
			state_ = DownlinkHealth::kEarlyWarning;
			degradeLevel_ = std::min(degradeLevel_ + 1, kMaxDegradeLevel);
			stableCount_ = 0;
		}
		break;
	case DownlinkHealth::kEarlyWarning:
		if (observed == DownlinkHealth::kCongested) {
			state_ = DownlinkHealth::kCongested;
			degradeLevel_ = std::min(degradeLevel_ + 2, kMaxDegradeLevel);
			stableCount_ = 0;
		} else if (observed == DownlinkHealth::kStable) {
			state_ = DownlinkHealth::kRecovering;
			stableCount_ = 1;
		}
		break;
	case DownlinkHealth::kCongested:
		if (observed == DownlinkHealth::kCongested) {
			degradeLevel_ = std::min(degradeLevel_ + 1, kMaxDegradeLevel);
		} else if (observed != DownlinkHealth::kCongested) {
			state_ = DownlinkHealth::kRecovering;
			stableCount_ = 1;
		}
		break;
	case DownlinkHealth::kRecovering:
		if (observed == DownlinkHealth::kCongested) {
			state_ = DownlinkHealth::kCongested;
			degradeLevel_ = std::min(degradeLevel_ + 2, kMaxDegradeLevel);
			stableCount_ = 0;
		} else if (observed == DownlinkHealth::kStable) {
			stableCount_++;
			if (stableCount_ >= 3 && degradeLevel_ > 0) {
				degradeLevel_--;
				stableCount_ = 0;
				if (degradeLevel_ == 0) state_ = DownlinkHealth::kStable;
			}
		} else {
			stableCount_ = 0;
		}
		break;
	}
	return degradeLevel_ != prevLevel;
}

} // namespace mediasoup::qos
