#pragma once

#include "ProbeSignal.h"

#include <algorithm>
#include <chrono>

namespace mediasoup::ccutils {

struct ProbeRegulatorConfig {
	std::chrono::microseconds baseInterval{ std::chrono::seconds(3) };
	double backoffFactor{ 1.5 };
	std::chrono::microseconds maxInterval{ std::chrono::minutes(2) };
	std::chrono::microseconds minDuration{ std::chrono::milliseconds(200) };
	std::chrono::microseconds maxDuration{ std::chrono::seconds(20) };
	double durationIncreaseFactor{ 1.5 };
};

class ProbeRegulator {
public:
	explicit ProbeRegulator(const ProbeRegulatorConfig& config = {})
		: config_(config)
		, probeInterval_(config.baseInterval)
		, probeDuration_(config.minDuration)
		, nextProbeEarliestAt_(std::chrono::steady_clock::now())
	{
	}

	bool CanProbe() const
	{
		return std::chrono::steady_clock::now() >= nextProbeEarliestAt_;
	}

	std::chrono::microseconds ProbeDuration() const
	{
		return probeDuration_;
	}

	void ProbeSignalReceived(
		ProbeSignal probeSignal,
		std::chrono::steady_clock::time_point baseTime = {})
	{
		if (probeSignal == ProbeSignal::Congesting) {
			const auto nextInterval = std::chrono::duration_cast<std::chrono::microseconds>(
				probeInterval_ * config_.backoffFactor);
			probeInterval_ = std::min(nextInterval, config_.maxInterval);
			probeDuration_ = config_.minDuration;
		} else {
			probeInterval_ = config_.baseInterval;
			const auto nextDuration = std::chrono::duration_cast<std::chrono::microseconds>(
				probeDuration_ * config_.durationIncreaseFactor);
			probeDuration_ = std::min(nextDuration, config_.maxDuration);
		}

		if (baseTime == std::chrono::steady_clock::time_point{}) {
			nextProbeEarliestAt_ = std::chrono::steady_clock::now() + probeInterval_;
		} else {
			nextProbeEarliestAt_ = baseTime + probeInterval_;
		}
	}

private:
	ProbeRegulatorConfig config_;
	std::chrono::microseconds probeInterval_;
	std::chrono::microseconds probeDuration_;
	std::chrono::steady_clock::time_point nextProbeEarliestAt_;
};

} // namespace mediasoup::ccutils
