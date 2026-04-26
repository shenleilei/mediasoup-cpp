#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace mediasoup::plainclient::sendsidebwe {

struct WeightedLossConfig {
	std::chrono::microseconds minDurationForLossValidity{ std::chrono::milliseconds(100) };
	std::chrono::microseconds baseDuration{ std::chrono::milliseconds(500) };
	int basePps{ 30 };
	double lossPenaltyFactor{ 0.25 };
};

class TrafficStats {
public:
	explicit TrafficStats(const WeightedLossConfig& config = {})
		: config_(config)
	{
	}

	void Merge(const TrafficStats& rhs)
	{
		if (minSendTimeUs_ == 0 || rhs.minSendTimeUs_ < minSendTimeUs_) {
			minSendTimeUs_ = rhs.minSendTimeUs_;
		}
		maxSendTimeUs_ = std::max(maxSendTimeUs_, rhs.maxSendTimeUs_);
		sendDeltaUs_ += rhs.sendDeltaUs_;
		recvDeltaUs_ += rhs.recvDeltaUs_;
		ackedPackets_ += rhs.ackedPackets_;
		ackedBytes_ += rhs.ackedBytes_;
		lostPackets_ += rhs.lostPackets_;
		lostBytes_ += rhs.lostBytes_;
	}

	void SetFields(
		int64_t minSendTimeUs,
		int64_t maxSendTimeUs,
		int64_t sendDeltaUs,
		int64_t recvDeltaUs,
		int ackedPackets,
		int ackedBytes,
		int lostPackets,
		int lostBytes)
	{
		minSendTimeUs_ = minSendTimeUs;
		maxSendTimeUs_ = maxSendTimeUs;
		sendDeltaUs_ = sendDeltaUs;
		recvDeltaUs_ = recvDeltaUs;
		ackedPackets_ = ackedPackets;
		ackedBytes_ = ackedBytes;
		lostPackets_ = lostPackets;
		lostBytes_ = lostBytes;
	}

	int64_t DurationUs() const
	{
		return std::max<int64_t>(1, maxSendTimeUs_ - minSendTimeUs_);
	}

	int64_t AcknowledgedBitrate() const
	{
		const int64_t durationUs = DurationUs();
		const double ackedBitrate = static_cast<double>(ackedBytes_) * 8.0 * 1000000.0 /
			static_cast<double>(durationUs);
		return static_cast<int64_t>(ackedBitrate * CapturedTrafficRatio());
	}

	double CapturedTrafficRatio() const
	{
		if (recvDeltaUs_ == 0) {
			return 0.0;
		}
		return std::min(1.0,
			static_cast<double>(sendDeltaUs_) /
				static_cast<double>(recvDeltaUs_ + LossPenaltyUs()));
	}

	double WeightedLoss() const
	{
		const int64_t durationUs = DurationUs();
		if (std::chrono::microseconds(durationUs) < config_.minDurationForLossValidity) {
			return 0.0;
		}

		const double totalPackets = static_cast<double>(lostPackets_ + ackedPackets_);
		const double pps = totalPackets * 1000000.0 / static_cast<double>(durationUs);
		auto deltaDuration = std::chrono::microseconds(durationUs) - config_.baseDuration;
		if (deltaDuration.count() < 0) {
			deltaDuration = std::chrono::microseconds(0);
		}
		const double threshold = std::exp(-deltaDuration.count() / 1000000.0) *
			static_cast<double>(config_.basePps);
		if (pps < threshold) {
			return 0.0;
		}

		double lossRatio = 0.0;
		if (totalPackets != 0.0) {
			lossRatio = static_cast<double>(lostPackets_) / totalPackets;
		}
		return lossRatio * std::log10(std::max(1.0, pps));
	}

	int64_t MinSendTimeUs() const { return minSendTimeUs_; }
	int64_t MaxSendTimeUs() const { return maxSendTimeUs_; }
	int64_t SendDeltaUs() const { return sendDeltaUs_; }
	int64_t RecvDeltaUs() const { return recvDeltaUs_; }
	int AckedPackets() const { return ackedPackets_; }
	int AckedBytes() const { return ackedBytes_; }
	int LostPackets() const { return lostPackets_; }
	int LostBytes() const { return lostBytes_; }

private:
	int64_t LossPenaltyUs() const
	{
		return static_cast<int64_t>(
			static_cast<double>(recvDeltaUs_) * WeightedLoss() * config_.lossPenaltyFactor);
	}

	WeightedLossConfig config_;
	int64_t minSendTimeUs_{ 0 };
	int64_t maxSendTimeUs_{ 0 };
	int64_t sendDeltaUs_{ 0 };
	int64_t recvDeltaUs_{ 0 };
	int ackedPackets_{ 0 };
	int ackedBytes_{ 0 };
	int lostPackets_{ 0 };
	int lostBytes_{ 0 };
};

} // namespace mediasoup::plainclient::sendsidebwe
