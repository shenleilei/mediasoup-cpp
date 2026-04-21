#pragma once

#include "TransportCcHelpers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>

namespace mediasoup::plainclient {

class DelayBasedBwe {
public:
	enum class TrendState {
		Normal,
		Underusing,
		Overusing
	};

	struct Config {
		uint32_t minBitrateBps{ 100000u };
		uint32_t maxBitrateBps{ std::numeric_limits<uint32_t>::max() };
		size_t trendlineWindowSize{ 20 };
		int64_t ackedBitrateWindowUs{ 500000 };
		double smoothedDelayAlpha{ 0.1 };
		double overuseSlopeThreshold{ 0.01 };
		double underuseSlopeThreshold{ -0.005 };
		uint32_t additiveIncreaseMinBps{ 20000u };
		uint32_t additiveIncreaseDivisor{ 20u };
		double overuseAckedBitrateRatio{ 0.90 };
		double fallbackDecreaseRatio{ 0.85 };
		double ackedBitrateGuardRatio{ 0.85 };
		int64_t minTrendObservationSendDeltaUs{ 5000 };
		int64_t maxTrendObservationDeltaUs{ 1000000 };
	};

	struct Result {
		size_t correlatedPacketCount{ 0 };
		uint32_t ackedBitrateBps{ 0 };
		double trendlineSlope{ 0.0 };
		TrendState state{ TrendState::Normal };
		uint32_t estimateBps{ 0 };
	};

	DelayBasedBwe()
		: DelayBasedBwe(Config{})
	{
	}

	explicit DelayBasedBwe(const Config& config)
		: config_(config)
	{
		if (config_.trendlineWindowSize < 2) {
			config_.trendlineWindowSize = 2;
		}
		if (config_.ackedBitrateWindowUs < 1000) {
			config_.ackedBitrateWindowUs = 1000;
		}
		if (!std::isfinite(config_.smoothedDelayAlpha) || config_.smoothedDelayAlpha <= 0.0) {
			config_.smoothedDelayAlpha = 0.1;
		} else if (config_.smoothedDelayAlpha > 1.0) {
			config_.smoothedDelayAlpha = 1.0;
		}
		if (!std::isfinite(config_.overuseSlopeThreshold) || config_.overuseSlopeThreshold <= 0.0) {
			config_.overuseSlopeThreshold = 0.01;
		}
		if (!std::isfinite(config_.underuseSlopeThreshold) || config_.underuseSlopeThreshold >= 0.0) {
			config_.underuseSlopeThreshold = -0.005;
		}
		if (!std::isfinite(config_.overuseAckedBitrateRatio) || config_.overuseAckedBitrateRatio <= 0.0) {
			config_.overuseAckedBitrateRatio = 0.90;
		}
		if (!std::isfinite(config_.fallbackDecreaseRatio) ||
			config_.fallbackDecreaseRatio <= 0.0 ||
			config_.fallbackDecreaseRatio >= 1.0) {
			config_.fallbackDecreaseRatio = 0.85;
		}
		if (!std::isfinite(config_.ackedBitrateGuardRatio) || config_.ackedBitrateGuardRatio <= 0.0) {
			config_.ackedBitrateGuardRatio = 0.85;
		}
		if (config_.minTrendObservationSendDeltaUs < 0) {
			config_.minTrendObservationSendDeltaUs = 0;
		}
		if (config_.maxTrendObservationDeltaUs <= 0) {
			config_.maxTrendObservationDeltaUs = 1000000;
		}
	}

	void SetEstimateBps(uint32_t estimateBps)
	{
		estimateBps_ = ClampBitrate(estimateBps);
	}

	uint32_t EstimateBps() const
	{
		return estimateBps_;
	}

	void OnPacketSent(uint16_t sequenceNumber, size_t sizeBytes, int64_t sendTimeUs)
	{
		auto& slot = sentPackets_[sequenceNumber];
		slot.valid = true;
		slot.sequenceNumber = sequenceNumber;
		slot.sendTimeUs = sendTimeUs;
		slot.sizeBytes = sizeBytes;
	}

	Result OnTransportFeedback(const TransportCcFeedback& feedback)
	{
		Result result;
		result.estimateBps = estimateBps_;

		for (const auto& packet : feedback.packets) {
			if (!packet.received || !packet.deltaPresent) {
				continue;
			}

			auto& slot = sentPackets_[packet.sequenceNumber];
			if (!slot.valid || slot.sequenceNumber != packet.sequenceNumber) {
				continue;
			}

			AckedObservation observation;
			observation.sequenceNumber = packet.sequenceNumber;
			observation.sendTimeUs = slot.sendTimeUs;
			observation.receiveTimeUs = packet.receiveTimeUs;
			observation.sizeBytes = slot.sizeBytes;
			slot.valid = false;

			result.correlatedPacketCount++;
			AddAckedObservation(observation, &result);
		}

		result.ackedBitrateBps = ComputeAckedBitrateBps();
		result.trendlineSlope = ComputeTrendlineSlope();
		result.state = ClassifyTrend(result.trendlineSlope);

		if (result.correlatedPacketCount == 0) {
			return result;
		}

		if (estimateBps_ == 0) {
			estimateBps_ = ClampBitrate(result.ackedBitrateBps);
			result.estimateBps = estimateBps_;
			return result;
		}

		switch (result.state) {
			case TrendState::Overusing: {
				const uint32_t decreaseFromCurrent =
					static_cast<uint32_t>(static_cast<double>(estimateBps_) * config_.fallbackDecreaseRatio);
				uint32_t nextEstimate = decreaseFromCurrent;
				if (result.ackedBitrateBps > 0) {
					const uint32_t guardedAcked =
						static_cast<uint32_t>(static_cast<double>(result.ackedBitrateBps) * config_.overuseAckedBitrateRatio);
					nextEstimate = std::min(nextEstimate, guardedAcked);
				}
				estimateBps_ = ClampBitrate(nextEstimate);
				break;
			}
			case TrendState::Normal:
			case TrendState::Underusing: {
				if (ShouldIncreaseEstimate(result.ackedBitrateBps, result.state)) {
					const uint32_t additiveStep = std::max<uint32_t>(
						config_.additiveIncreaseMinBps,
						config_.additiveIncreaseDivisor == 0
							? config_.additiveIncreaseMinBps
							: (estimateBps_ / config_.additiveIncreaseDivisor));
					const uint32_t guardBps = result.ackedBitrateBps == 0
						? static_cast<uint32_t>(estimateBps_ + additiveStep)
						: static_cast<uint32_t>(result.ackedBitrateBps + additiveStep);
					const uint32_t increasedEstimate =
						estimateBps_ > config_.maxBitrateBps - additiveStep
							? config_.maxBitrateBps
							: (estimateBps_ + additiveStep);
					estimateBps_ = ClampBitrate(std::min(increasedEstimate, guardBps));
				}
				break;
			}
		}

		result.estimateBps = estimateBps_;
		return result;
	}

private:
	struct SentPacket {
		bool valid{ false };
		uint16_t sequenceNumber{ 0 };
		int64_t sendTimeUs{ 0 };
		size_t sizeBytes{ 0 };
	};

	struct AckedObservation {
		uint16_t sequenceNumber{ 0 };
		int64_t sendTimeUs{ 0 };
		int64_t receiveTimeUs{ 0 };
		size_t sizeBytes{ 0 };
	};

	struct TrendPoint {
		int64_t receiveTimeUs{ 0 };
		double smoothedDelayUs{ 0.0 };
	};

	uint32_t ClampBitrate(uint32_t bitrateBps) const
	{
		const uint32_t minBitrate = std::min(config_.minBitrateBps, config_.maxBitrateBps);
		const uint32_t maxBitrate = std::max(config_.minBitrateBps, config_.maxBitrateBps);
		if (bitrateBps == 0) {
			return 0;
		}
		return std::clamp<uint32_t>(bitrateBps, minBitrate, maxBitrate);
	}

	void AddAckedObservation(const AckedObservation& observation, Result* result)
	{
		ackedWindow_.push_back(observation);
		ackedWindowBytes_ += observation.sizeBytes;
		while (!ackedWindow_.empty() &&
			observation.receiveTimeUs - ackedWindow_.front().receiveTimeUs > config_.ackedBitrateWindowUs) {
			ackedWindowBytes_ -= ackedWindow_.front().sizeBytes;
			ackedWindow_.pop_front();
		}

		if (!hasPreviousObservation_) {
			previousObservation_ = observation;
			hasPreviousObservation_ = true;
			return;
		}

		const int64_t sendDeltaUs = observation.sendTimeUs - previousObservation_.sendTimeUs;
		const int64_t recvDeltaUs = observation.receiveTimeUs - previousObservation_.receiveTimeUs;
		previousObservation_ = observation;

		if (sendDeltaUs <= 0 || recvDeltaUs <= 0) {
			return;
		}
		if (sendDeltaUs < config_.minTrendObservationSendDeltaUs) {
			return;
		}
		if (sendDeltaUs > config_.maxTrendObservationDeltaUs ||
			recvDeltaUs > config_.maxTrendObservationDeltaUs) {
			return;
		}

		const int64_t delayDeltaUs = recvDeltaUs - sendDeltaUs;
		accumulatedDelayUs_ += static_cast<double>(delayDeltaUs);
		smoothedDelayUs_ =
			config_.smoothedDelayAlpha * accumulatedDelayUs_ +
			(1.0 - config_.smoothedDelayAlpha) * smoothedDelayUs_;

		TrendPoint point;
		point.receiveTimeUs = observation.receiveTimeUs;
		point.smoothedDelayUs = smoothedDelayUs_;
		trendlinePoints_.push_back(point);
		while (trendlinePoints_.size() > config_.trendlineWindowSize) {
			trendlinePoints_.pop_front();
		}

		if (result) {
			result->trendlineSlope = ComputeTrendlineSlope();
		}
	}

	uint32_t ComputeAckedBitrateBps() const
	{
		if (ackedWindow_.size() < 2 || ackedWindowBytes_ == 0) {
			return 0;
		}
		const int64_t windowDurationUs =
			ackedWindow_.back().receiveTimeUs - ackedWindow_.front().receiveTimeUs;
		if (windowDurationUs <= 0) {
			return 0;
		}
		const uint64_t bits = static_cast<uint64_t>(ackedWindowBytes_) * 8u * 1000000u;
		const uint64_t bitrate = bits / static_cast<uint64_t>(windowDurationUs);
		return bitrate > std::numeric_limits<uint32_t>::max()
			? std::numeric_limits<uint32_t>::max()
			: static_cast<uint32_t>(bitrate);
	}

	double ComputeTrendlineSlope() const
	{
		if (trendlinePoints_.size() < 2) {
			return 0.0;
		}

		const double baseTimeUs = static_cast<double>(trendlinePoints_.front().receiveTimeUs);
		double meanX = 0.0;
		double meanY = 0.0;
		for (const auto& point : trendlinePoints_) {
			meanX += (static_cast<double>(point.receiveTimeUs) - baseTimeUs) / 1000.0;
			meanY += point.smoothedDelayUs / 1000.0;
		}
		meanX /= static_cast<double>(trendlinePoints_.size());
		meanY /= static_cast<double>(trendlinePoints_.size());

		double numerator = 0.0;
		double denominator = 0.0;
		for (const auto& point : trendlinePoints_) {
			const double x = (static_cast<double>(point.receiveTimeUs) - baseTimeUs) / 1000.0;
			const double y = point.smoothedDelayUs / 1000.0;
			numerator += (x - meanX) * (y - meanY);
			denominator += (x - meanX) * (x - meanX);
		}
		if (denominator <= 0.0) {
			return 0.0;
		}
		return numerator / denominator;
	}

	TrendState ClassifyTrend(double slope) const
	{
		if (slope >= config_.overuseSlopeThreshold) {
			return TrendState::Overusing;
		}
		if (slope <= config_.underuseSlopeThreshold) {
			return TrendState::Underusing;
		}
		return TrendState::Normal;
	}

	bool ShouldIncreaseEstimate(uint32_t ackedBitrateBps, TrendState state) const
	{
		if (estimateBps_ == 0) {
			return ackedBitrateBps > 0;
		}
		if (state == TrendState::Overusing) {
			return false;
		}
		if (ackedBitrateBps == 0) {
			return false;
		}
		const double guard = static_cast<double>(estimateBps_) * config_.ackedBitrateGuardRatio;
		if (static_cast<double>(ackedBitrateBps) >= guard) {
			return true;
		}
		return state == TrendState::Underusing &&
			static_cast<double>(ackedBitrateBps) >= guard * 0.9;
	}

	Config config_;
	std::array<SentPacket, 65536> sentPackets_{};
	std::deque<AckedObservation> ackedWindow_;
	size_t ackedWindowBytes_{ 0 };
	std::deque<TrendPoint> trendlinePoints_;
	AckedObservation previousObservation_{};
	bool hasPreviousObservation_{ false };
	double accumulatedDelayUs_{ 0.0 };
	double smoothedDelayUs_{ 0.0 };
	uint32_t estimateBps_{ 0 };
};

} // namespace mediasoup::plainclient
