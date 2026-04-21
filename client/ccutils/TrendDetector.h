#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

namespace mediasoup::ccutils {

enum class TrendDirection {
	Inconclusive,
	Upward,
	Downward
};

template<typename T>
struct TrendDetectorConfig {
	int requiredSamples{ 4 };
	int requiredSamplesMin{ 2 };
	double downwardTrendThreshold{ -0.5 };
	std::chrono::microseconds downwardTrendMaxWait{ std::chrono::seconds(2) };
	std::chrono::microseconds collapseThreshold{ std::chrono::milliseconds(500) };
	std::chrono::microseconds validityWindow{ std::chrono::seconds(10) };
};

template<typename T>
class TrendDetector {
public:
	explicit TrendDetector(const TrendDetectorConfig<T>& config = {})
		: config_(config)
		, startTime_(std::chrono::steady_clock::now())
	{
	}

	void Seed(T value)
	{
		if (!samples_.empty()) {
			return;
		}
		samples_.push_back({value, std::chrono::steady_clock::now()});
	}

	void AddValue(T value)
	{
		numSamples_++;
		if (!hasLowest_ || value < lowestValue_) {
			lowestValue_ = value;
			hasLowest_ = true;
		}
		if (!hasHighest_ || value > highestValue_) {
			highestValue_ = value;
			hasHighest_ = true;
		}

		const auto now = std::chrono::steady_clock::now();
		if (!samples_.empty() &&
			samples_.back().value == value &&
			config_.collapseThreshold.count() > 0 &&
			now - samples_.back().at < config_.collapseThreshold) {
			return;
		}

		samples_.push_back({value, now});
		Prune(now);
		UpdateDirection();
	}

	TrendDirection GetDirection() const
	{
		return direction_;
	}

	T GetLowest() const
	{
		return lowestValue_;
	}

	T GetHighest() const
	{
		return highestValue_;
	}

	bool HasEnoughSamples() const
	{
		return numSamples_ >= config_.requiredSamples;
	}

private:
	struct Sample {
		T value;
		std::chrono::steady_clock::time_point at;
	};

	void Prune(std::chrono::steady_clock::time_point now)
	{
		if (static_cast<int>(samples_.size()) > config_.requiredSamples) {
			samples_.erase(samples_.begin(), samples_.end() - config_.requiredSamples);
		}

		if (!samples_.empty() && config_.validityWindow.count() > 0) {
			const auto cutoff = now - config_.validityWindow;
			auto it = std::find_if(samples_.begin(), samples_.end(),
				[&](const Sample& sample) { return sample.at > cutoff; });
			if (it != samples_.end()) {
				samples_.erase(samples_.begin(), it);
			}
		}

		if (!samples_.empty()) {
			const T firstValue = samples_.front().value;
			auto it = std::find_if(samples_.begin() + 1, samples_.end(),
				[&](const Sample& sample) { return sample.value != firstValue; });
			if (it != samples_.end()) {
				samples_.erase(samples_.begin(), it - 1);
			} else {
				samples_.erase(samples_.begin(), samples_.end() - 1);
			}
		}
	}

	void UpdateDirection()
	{
		if (static_cast<int>(samples_.size()) < config_.requiredSamplesMin) {
			direction_ = TrendDirection::Inconclusive;
			return;
		}

		const double tau = KendallsTau();
		direction_ = TrendDirection::Inconclusive;
		if (tau > 0.0 && static_cast<int>(samples_.size()) >= config_.requiredSamples) {
			direction_ = TrendDirection::Upward;
			return;
		}

		const auto duration = samples_.back().at - samples_.front().at;
		if (tau < config_.downwardTrendThreshold &&
			(static_cast<int>(samples_.size()) >= config_.requiredSamples ||
				duration > config_.downwardTrendMaxWait)) {
			direction_ = TrendDirection::Downward;
		}
	}

	double KendallsTau() const
	{
		int concordantPairs = 0;
		int discordantPairs = 0;
		for (size_t i = 0; i + 1 < samples_.size(); ++i) {
			for (size_t j = i + 1; j < samples_.size(); ++j) {
				if (samples_[i].value < samples_[j].value) {
					concordantPairs++;
				} else if (samples_[i].value > samples_[j].value) {
					discordantPairs++;
				}
			}
		}
		if (concordantPairs + discordantPairs == 0) {
			return 0.0;
		}
		return static_cast<double>(concordantPairs - discordantPairs) /
			static_cast<double>(concordantPairs + discordantPairs);
	}

	TrendDetectorConfig<T> config_;
	std::chrono::steady_clock::time_point startTime_;
	int numSamples_{ 0 };
	std::vector<Sample> samples_;
	T lowestValue_{};
	T highestValue_{};
	bool hasLowest_{ false };
	bool hasHighest_{ false };
	TrendDirection direction_{ TrendDirection::Inconclusive };
};

} // namespace mediasoup::ccutils
