#pragma once

#include "PacketGroup.h"
#include "PacketTracker.h"
#include "ProbePacketGroup.h"
#include "TrafficStats.h"
#include "TwccFeedbackTracker.h"

#include "../ccutils/ProbeRegulator.h"
#include "../ccutils/TrendDetector.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

namespace mediasoup::plainclient::sendsidebwe {

enum class CongestionState {
	None,
	EarlyWarning,
	Congested
};

enum class QueuingRegion {
	Dqr,
	Indeterminate,
	Jqr
};

enum class CongestionReason {
	None,
	QueuingDelay,
	Loss
};

struct TransportParityMetrics {
	double senderTransportDelayMs = -1.0;
	double senderTransportJitterMs = -1.0;
	CongestionState congestionState = CongestionState::None;
	CongestionReason congestionReason = CongestionReason::None;
};

inline bool congestionDebugEnabled()
{
	static const bool enabled = []() {
		const char* raw = std::getenv("QOS_DEBUG_CONGESTION");
		return raw && (*raw == '1' || *raw == 't' || *raw == 'T' || *raw == 'y' || *raw == 'Y');
	}();
	return enabled;
}

inline const char* congestionReasonStr(CongestionReason reason)
{
	switch (reason) {
		case CongestionReason::None: return "none";
		case CongestionReason::QueuingDelay: return "qd";
		case CongestionReason::Loss: return "loss";
	}
	return "unknown";
}

inline const char* congestionStateStr(CongestionState state)
{
	switch (state) {
		case CongestionState::None: return "none";
		case CongestionState::EarlyWarning: return "warning";
		case CongestionState::Congested: return "congested";
	}
	return "unknown";
}

struct CongestionSignalConfig {
	int minNumberOfGroups{ 0 };
	std::chrono::microseconds minDuration{ 0 };

	bool IsTriggered(int numGroups, int64_t durationUs) const
	{
		return numGroups >= minNumberOfGroups && durationUs >= minDuration.count();
	}
};

struct ProbeSignalConfig {
	double minBytesRatio{ 0.5 };
	double minDurationRatio{ 0.5 };
	std::chrono::microseconds jqrMinDelay{ std::chrono::milliseconds(50) };
	std::chrono::microseconds dqrMaxDelay{ std::chrono::milliseconds(20) };
	WeightedLossConfig weightedLoss{};
	double jqrMinWeightedLoss{ 0.25 };
	double dqrMaxWeightedLoss{ 0.1 };

	bool IsValid(const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo) const
	{
		return probeClusterInfo.result.Bytes() >
				static_cast<int>(minBytesRatio * static_cast<double>(probeClusterInfo.goal.desiredBytes)) &&
			probeClusterInfo.result.Duration() >
				std::chrono::duration_cast<std::chrono::microseconds>(
					probeClusterInfo.goal.duration * minDurationRatio);
	}

	std::pair<mediasoup::ccutils::ProbeSignal, int64_t> ProbeSignalForGroup(
		const ProbePacketGroup& probePacketGroup) const
	{
		TrafficStats stats(weightedLoss);
		stats.Merge(probePacketGroup.Traffic());
		const int64_t propagatedDelayUs = probePacketGroup.PropagatedQueuingDelayUs();
		if (propagatedDelayUs > jqrMinDelay.count() || stats.WeightedLoss() > jqrMinWeightedLoss) {
			return {mediasoup::ccutils::ProbeSignal::Congesting, stats.AcknowledgedBitrate()};
		}
		if (propagatedDelayUs < dqrMaxDelay.count() && stats.WeightedLoss() < dqrMaxWeightedLoss) {
			return {mediasoup::ccutils::ProbeSignal::NotCongesting, stats.AcknowledgedBitrate()};
		}
		return {mediasoup::ccutils::ProbeSignal::Inconclusive, stats.AcknowledgedBitrate()};
	}
};

class QdMeasurement {
public:
	QdMeasurement(
		const CongestionSignalConfig& jqrConfig,
		const CongestionSignalConfig& dqrConfig,
		int64_t jqrMinDelayUs,
		double jqrMinTrendCoefficient,
		int64_t dqrMaxDelayUs)
		: jqrConfig_(jqrConfig)
		, dqrConfig_(dqrConfig)
		, jqrMinDelayUs_(jqrMinDelayUs)
		, jqrMinTrendCoefficient_(jqrMinTrendCoefficient)
		, dqrMaxDelayUs_(dqrMaxDelayUs)
	{
	}

	void ProcessPacketGroup(const PacketGroup& packetGroup, int groupIndex)
	{
		if (isSealed_) {
			return;
		}
		int64_t propagatedDelayUs = 0;
		if (!packetGroup.FinalizedPropagatedQueuingDelayUs(&propagatedDelayUs)) {
			return;
		}

		numGroups_++;
		if (minGroupIndex_ == -1 || groupIndex < minGroupIndex_) {
			minGroupIndex_ = groupIndex;
		}
		maxGroupIndex_ = std::max(maxGroupIndex_, groupIndex);

		int64_t minSendTimeUs = 0;
		int64_t maxSendTimeUs = 0;
		packetGroup.SendWindow(&minSendTimeUs, &maxSendTimeUs);
		if (minSendTimeUs_ == 0 || minSendTimeUs < minSendTimeUs_) {
			minSendTimeUs_ = minSendTimeUs;
		}
		maxSendTimeUs_ = std::max(maxSendTimeUs_, maxSendTimeUs);

		propagatedQueuingDelaysUs_.push_back(propagatedDelayUs);

		if (propagatedDelayUs < dqrMaxDelayUs_) {
			numDqrGroups_++;
			if (numJqrGroups_ > 0) {
				isSealed_ = true;
			} else if (dqrConfig_.IsTriggered(numDqrGroups_, maxSendTimeUs_ - minSendTimeUs_)) {
				isSealed_ = true;
				queuingRegion_ = QueuingRegion::Dqr;
			}
			return;
		}

		if (propagatedDelayUs > jqrMinDelayUs_) {
			numJqrGroups_++;
			if (numDqrGroups_ > 0) {
				isSealed_ = true;
			} else if (jqrConfig_.IsTriggered(numJqrGroups_, maxSendTimeUs_ - minSendTimeUs_) &&
				TrendCoefficient() > jqrMinTrendCoefficient_) {
				isSealed_ = true;
				queuingRegion_ = QueuingRegion::Jqr;
			}
			return;
		}

		if (numDqrGroups_ > 0 || numJqrGroups_ > 0) {
			isSealed_ = true;
		}
	}

	bool IsSealed() const { return isSealed_; }
	QueuingRegion GetQueuingRegion() const { return queuingRegion_; }
	std::pair<int, int> GroupRange() const { return {std::max(0, minGroupIndex_), std::max(0, maxGroupIndex_)}; }

private:
	double TrendCoefficient() const
	{
		int concordantPairs = 0;
		int discordantPairs = 0;
		for (size_t i = 0; i + 1 < propagatedQueuingDelaysUs_.size(); ++i) {
			for (size_t j = i + 1; j < propagatedQueuingDelaysUs_.size(); ++j) {
				if (propagatedQueuingDelaysUs_[i] > propagatedQueuingDelaysUs_[j]) {
					concordantPairs++;
				} else if (propagatedQueuingDelaysUs_[i] < propagatedQueuingDelaysUs_[j]) {
					discordantPairs++;
				}
			}
		}
		if (concordantPairs + discordantPairs == 0) {
			return propagatedQueuingDelaysUs_.size() == 1 ? 1.0 : 0.0;
		}
		return static_cast<double>(concordantPairs - discordantPairs) /
			static_cast<double>(concordantPairs + discordantPairs);
	}

	CongestionSignalConfig jqrConfig_;
	CongestionSignalConfig dqrConfig_;
	int64_t jqrMinDelayUs_{ 0 };
	double jqrMinTrendCoefficient_{ 0.0 };
	int64_t dqrMaxDelayUs_{ 0 };
	int numGroups_{ 0 };
	int numJqrGroups_{ 0 };
	int numDqrGroups_{ 0 };
	int64_t minSendTimeUs_{ 0 };
	int64_t maxSendTimeUs_{ 0 };
	std::vector<int64_t> propagatedQueuingDelaysUs_;
	bool isSealed_{ false };
	int minGroupIndex_{ -1 };
	int maxGroupIndex_{ -1 };
	QueuingRegion queuingRegion_{ QueuingRegion::Indeterminate };
};

class LossMeasurement {
public:
	LossMeasurement(
		const CongestionSignalConfig& jqrConfig,
		const CongestionSignalConfig& dqrConfig,
		const WeightedLossConfig& weightedLossConfig,
		double jqrMinLoss,
		double dqrMaxLoss)
		: jqrConfig_(jqrConfig)
		, dqrConfig_(dqrConfig)
		, jqrMinLoss_(jqrMinLoss)
		, dqrMaxLoss_(dqrMaxLoss)
		, trafficStats_(weightedLossConfig)
	{
	}

	void ProcessPacketGroup(const PacketGroup& packetGroup, int groupIndex)
	{
		if ((isJqrSealed_ && isDqrSealed_) || !packetGroup.IsFinalized()) {
			return;
		}

		numGroups_++;
		if (minGroupIndex_ == -1 || groupIndex < minGroupIndex_) {
			minGroupIndex_ = groupIndex;
		}
		maxGroupIndex_ = std::max(maxGroupIndex_, groupIndex);
		trafficStats_.Merge(packetGroup.Traffic());

		if (!isJqrSealed_ && jqrConfig_.IsTriggered(numGroups_, trafficStats_.DurationUs())) {
			isJqrSealed_ = true;
			const double weightedLoss = trafficStats_.WeightedLoss();
			if (weightedLoss > jqrMinLoss_) {
				weightedLoss_ = weightedLoss;
				queuingRegion_ = QueuingRegion::Jqr;
				isDqrSealed_ = true;
				return;
			}
		}

		if (!isDqrSealed_ && dqrConfig_.IsTriggered(numGroups_, trafficStats_.DurationUs())) {
			isDqrSealed_ = true;
			const double weightedLoss = trafficStats_.WeightedLoss();
			if (weightedLoss < dqrMaxLoss_) {
				weightedLoss_ = weightedLoss;
				queuingRegion_ = QueuingRegion::Dqr;
				isJqrSealed_ = true;
			}
		}
	}

	bool IsSealed() const { return isJqrSealed_ && isDqrSealed_; }
	QueuingRegion GetQueuingRegion() const { return queuingRegion_; }
	std::pair<int, int> GroupRange() const { return {std::max(0, minGroupIndex_), std::max(0, maxGroupIndex_)}; }

private:
	CongestionSignalConfig jqrConfig_;
	CongestionSignalConfig dqrConfig_;
	double jqrMinLoss_{ 0.0 };
	double dqrMaxLoss_{ 0.0 };
	int numGroups_{ 0 };
	TrafficStats trafficStats_;
	bool isJqrSealed_{ false };
	bool isDqrSealed_{ false };
	int minGroupIndex_{ -1 };
	int maxGroupIndex_{ -1 };
	double weightedLoss_{ 0.0 };
	QueuingRegion queuingRegion_{ QueuingRegion::Indeterminate };
};

struct CongestionDetectorConfig {
	PacketGroupConfig packetGroup{};
	std::chrono::microseconds packetGroupMaxAge{ std::chrono::seconds(10) };
	ProbePacketGroupConfig probePacketGroup{};
	mediasoup::ccutils::ProbeRegulatorConfig probeRegulator{};
	ProbeSignalConfig probeSignal{};
	std::chrono::microseconds jqrMinDelay{ std::chrono::milliseconds(50) };
	double jqrMinTrendCoefficient{ 0.8 };
	std::chrono::microseconds dqrMaxDelay{ std::chrono::milliseconds(20) };
	WeightedLossConfig weightedLoss{};
	double jqrMinWeightedLoss{ 0.25 };
	double dqrMaxWeightedLoss{ 0.1 };
	CongestionSignalConfig queuingDelayEarlyWarningJqr{ 2, std::chrono::milliseconds(200) };
	CongestionSignalConfig queuingDelayEarlyWarningDqr{ 3, std::chrono::milliseconds(300) };
	CongestionSignalConfig lossEarlyWarningJqr{ 3, std::chrono::milliseconds(300) };
	CongestionSignalConfig lossEarlyWarningDqr{ 4, std::chrono::milliseconds(400) };
	CongestionSignalConfig queuingDelayCongestedJqr{ 4, std::chrono::milliseconds(400) };
	CongestionSignalConfig queuingDelayCongestedDqr{ 5, std::chrono::milliseconds(500) };
	CongestionSignalConfig lossCongestedJqr{ 6, std::chrono::milliseconds(600) };
	CongestionSignalConfig lossCongestedDqr{ 6, std::chrono::milliseconds(600) };
	mediasoup::ccutils::TrendDetectorConfig<double> congestedCtrTrend{
		4, 2, -0.5, std::chrono::seconds(2), std::chrono::milliseconds(500), std::chrono::seconds(10)
	};
	double congestedCtrEpsilon{ 0.05 };
	PacketGroupConfig congestedPacketGroup{ 20, std::chrono::milliseconds(150) };
	std::chrono::microseconds estimationWindowDuration{ std::chrono::seconds(1) };
};

class CongestionDetector {
public:
	explicit CongestionDetector(const CongestionDetectorConfig& config = {})
		: config_(config)
		, probeRegulator_(config.probeRegulator)
	{
		Reset();
	}

	void Reset()
	{
		rttSeconds_ = 0.070;
		packetTracker_ = PacketTracker();
		twccFeedback_ = TwccFeedbackTracker();
		packetGroups_.clear();
		probePacketGroup_.reset();
		probeRegulator_ = mediasoup::ccutils::ProbeRegulator(config_.probeRegulator);
		estimatedAvailableChannelCapacityBps_ = 100000000;
		estimateTrafficStats_.reset();
		congestionState_ = CongestionState::None;
		congestionStateSwitchedAt_ = std::chrono::steady_clock::now();
		ClearCtrTrend();
		congestionReason_ = CongestionReason::None;
		qdMeasurement_.reset();
		lossMeasurement_.reset();
	}

	uint16_t RecordPacketSendAndGetSequenceNumber(
		int64_t atUs,
		size_t size,
		bool isRtx = false,
		mediasoup::ccutils::ProbeClusterId probeClusterId = mediasoup::ccutils::ProbeClusterIdInvalid,
		bool isProbe = false)
	{
		return packetTracker_.RecordPacketSendAndGetSequenceNumber(atUs, size, isRtx, probeClusterId, isProbe);
	}

	uint16_t GetNextSequenceNumber()
	{
		return packetTracker_.GetNextSequenceNumber();
	}

	void RecordPacketSent(
		uint16_t sequenceNumber,
		int64_t atUs,
		size_t size,
		bool isRtx = false,
		mediasoup::ccutils::ProbeClusterId probeClusterId = mediasoup::ccutils::ProbeClusterIdInvalid,
		bool isProbe = false)
	{
		packetTracker_.RecordPacketSent(
			sequenceNumber, atUs, size, isRtx, probeClusterId, isProbe);
	}

	void SeedSentPacketForTest(
		uint16_t sequenceNumber,
		int64_t sendTimeUs,
		size_t size,
		bool isRtx = false)
	{
		packetTracker_.SeedPacketForTest(sequenceNumber, sendTimeUs, size, isRtx);
	}

	void HandleTransportFeedback(const mediasoup::plainclient::TransportCcFeedback& report, int64_t atUs)
	{
		auto [recvRefTimeUs, _] = twccFeedback_.ProcessReport(report, atUs);
		if (packetGroups_.empty()) {
			packetGroups_.emplace_back(config_.packetGroup, config_.weightedLoss, 0);
		}

		auto* currentPacketGroup = &packetGroups_.back();
		auto trackPacketGroup = [&](const PacketInfo& packetInfo, int64_t sendDeltaUs, int64_t recvDeltaUs, bool isLost) {
			UpdateCtrTrend(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
			if (probePacketGroup_) {
				probePacketGroup_->Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
			}

			PacketGroupAddResult addResult = currentPacketGroup->Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
			if (addResult == PacketGroupAddResult::Added) {
				return;
			}

			if (addResult == PacketGroupAddResult::GroupFinalized) {
				const int64_t inheritedDelayUs = currentPacketGroup->PropagatedQueuingDelayUs();
				packetGroups_.emplace_back(config_.packetGroup, config_.weightedLoss, inheritedDelayUs);
				currentPacketGroup = &packetGroups_.back();
				(void)currentPacketGroup->Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
				return;
			}

			for (int index = static_cast<int>(packetGroups_.size()) - 2; index >= 0; --index) {
				PacketGroupAddResult olderResult =
					packetGroups_[static_cast<size_t>(index)].Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
				if (olderResult == PacketGroupAddResult::Added) {
					return;
				}
			}
		};

		for (const auto& packet : report.packets) {
			int64_t recvTimeUs = 0;
			bool isLost = false;
			if (packet.received && packet.deltaPresent) {
				recvRefTimeUs += packet.deltaUs;
				recvTimeUs = recvRefTimeUs;
			} else {
				isLost = true;
			}
			auto indication = packetTracker_.RecordPacketIndicationFromRemote(packet.sequenceNumber, recvTimeUs);
			if (indication.valid && indication.packetInfo.sendTimeUs != 0) {
				trackPacketGroup(
					indication.packetInfo,
					indication.sendDeltaUs,
					indication.recvDeltaUs,
					isLost);
			}
		}

		PrunePacketGroups(atUs);
		(void)CongestionDetectionStateMachine();
	}

	void UpdateRtt(double rttSeconds)
	{
		if (rttSeconds == 0.0) {
			rttSeconds_ = 0.070;
			return;
		}
		if (rttSeconds_ == 0.0) {
			rttSeconds_ = rttSeconds;
		} else {
			rttSeconds_ = 0.5 * rttSeconds + 0.5 * rttSeconds_;
		}
	}

	void SeedEstimatedAvailableChannelCapacityForTest(int64_t capacityBps)
	{
		estimatedAvailableChannelCapacityBps_ = capacityBps;
	}

	CongestionState GetCongestionState() const { return congestionState_; }
	CongestionReason GetCongestionReason() const { return congestionReason_; }
	int64_t EstimatedAvailableChannelCapacityBps() const { return estimatedAvailableChannelCapacityBps_; }

	TransportParityMetrics GetTransportParityMetrics() const
	{
		TransportParityMetrics metrics;
		metrics.congestionState = congestionState_;
		metrics.congestionReason = congestionReason_;

		constexpr size_t kMaxFinalizedGroups = 6;
		int64_t latestDelayUs = -1;
		int64_t minDelayUs = std::numeric_limits<int64_t>::max();
		int64_t maxDelayUs = std::numeric_limits<int64_t>::min();
		size_t finalizedGroups = 0;

		for (auto it = packetGroups_.rbegin();
			it != packetGroups_.rend() && finalizedGroups < kMaxFinalizedGroups;
			++it) {
			int64_t propagatedDelayUs = 0;
			if (!it->FinalizedPropagatedQueuingDelayUs(&propagatedDelayUs)) {
				continue;
			}
			if (latestDelayUs < 0) {
				latestDelayUs = propagatedDelayUs;
			}
			minDelayUs = std::min(minDelayUs, propagatedDelayUs);
			maxDelayUs = std::max(maxDelayUs, propagatedDelayUs);
			finalizedGroups++;
		}

		if (latestDelayUs >= 0) {
			metrics.senderTransportDelayMs =
				std::max(0.0, static_cast<double>(latestDelayUs) / 1000.0);
		}
		if (finalizedGroups == 1) {
			metrics.senderTransportJitterMs = 0.0;
		} else if (finalizedGroups > 1) {
			metrics.senderTransportJitterMs = std::max(
				0.0,
				static_cast<double>(maxDelayUs - minDelayUs) / 1000.0);
		}

		return metrics;
	}

	bool CanProbe() const
	{
		return congestionState_ == CongestionState::None &&
			!probePacketGroup_.has_value() &&
			probeRegulator_.CanProbe();
	}

	std::chrono::microseconds ProbeDuration() const
	{
		return probeRegulator_.ProbeDuration();
	}

	void ProbeClusterStarting(const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo)
	{
		probePacketGroup_.emplace(config_.probePacketGroup, config_.weightedLoss, probeClusterInfo);
		packetTracker_.ProbeClusterStarting(probeClusterInfo.id);
	}

	void ProbeClusterDone(const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo)
	{
		packetTracker_.ProbeClusterDone(probeClusterInfo.id);
		if (probePacketGroup_) {
			probePacketGroup_->ProbeClusterDone(probeClusterInfo);
		}
	}

	bool ProbeClusterIsGoalReached(
		const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo) const
	{
		if (!probePacketGroup_ || congestionState_ != CongestionState::None) {
			return false;
		}
		if (!config_.probeSignal.IsValid(probeClusterInfo)) {
			return false;
		}
		auto [probeSignal, estimatedCapacity] = config_.probeSignal.ProbeSignalForGroup(*probePacketGroup_);
		return probeSignal == mediasoup::ccutils::ProbeSignal::NotCongesting &&
			estimatedCapacity > probeClusterInfo.goal.desiredBps;
	}

	std::tuple<mediasoup::ccutils::ProbeSignal, int64_t, bool> ProbeClusterFinalize()
	{
		if (!probePacketGroup_) {
			return {mediasoup::ccutils::ProbeSignal::Inconclusive, 0, false};
		}

		mediasoup::ccutils::ProbeClusterInfo probeClusterInfo;
		const bool isFinalized =
			probePacketGroup_->MaybeFinalizeProbe(packetTracker_.ProbeMaxSequenceNumber(), rttSeconds_, &probeClusterInfo);
		if (!isFinalized) {
			return {mediasoup::ccutils::ProbeSignal::Inconclusive, 0, false};
		}

		if (congestionState_ != CongestionState::None) {
			const auto probeSignal = mediasoup::ccutils::ProbeSignal::Congesting;
			probeRegulator_.ProbeSignalReceived(probeSignal, probeClusterInfo.createdAt);
			probePacketGroup_.reset();
			return {probeSignal, estimatedAvailableChannelCapacityBps_, true};
		}

		auto [probeSignal, estimatedCapacity] = config_.probeSignal.ProbeSignalForGroup(*probePacketGroup_);
		if (probeSignal == mediasoup::ccutils::ProbeSignal::NotCongesting &&
			estimatedCapacity > estimatedAvailableChannelCapacityBps_) {
			estimatedAvailableChannelCapacityBps_ = estimatedCapacity;
		}
		probeRegulator_.ProbeSignalReceived(probeSignal, probeClusterInfo.createdAt);
		probePacketGroup_.reset();
		return {probeSignal, estimatedAvailableChannelCapacityBps_, true};
	}

private:
	void PrunePacketGroups(int64_t nowUs)
	{
		if (packetGroups_.empty()) {
			return;
		}
		const int64_t thresholdUs =
			packetTracker_.BaseSendTimeThreshold(config_.packetGroupMaxAge.count(), nowUs);
		if (thresholdUs == 0) {
			return;
		}
		for (size_t index = 0; index < packetGroups_.size(); ++index) {
			if (packetGroups_[index].MinSendTimeUs() > thresholdUs) {
				packetGroups_.erase(packetGroups_.begin(), packetGroups_.begin() + static_cast<long>(index));
				return;
			}
		}
	}

	QueuingRegion UpdateCongestionSignal(
		const CongestionSignalConfig& qdJqrConfig,
		const CongestionSignalConfig& qdDqrConfig,
		const CongestionSignalConfig& lossJqrConfig,
		const CongestionSignalConfig& lossDqrConfig)
	{
		qdMeasurement_.emplace(
			qdJqrConfig,
			qdDqrConfig,
			config_.jqrMinDelay.count(),
			config_.jqrMinTrendCoefficient,
			config_.dqrMaxDelay.count());
		lossMeasurement_.emplace(
			lossJqrConfig,
			lossDqrConfig,
			config_.weightedLoss,
			config_.jqrMinWeightedLoss,
			config_.dqrMaxWeightedLoss);

		for (int index = static_cast<int>(packetGroups_.size()) - 1; index >= 0; --index) {
			const PacketGroup& packetGroup = packetGroups_[static_cast<size_t>(index)];
			qdMeasurement_->ProcessPacketGroup(packetGroup, index);
			lossMeasurement_->ProcessPacketGroup(packetGroup, index);
			if (qdMeasurement_->IsSealed() && lossMeasurement_->IsSealed()) {
				break;
			}
		}

		const QueuingRegion qdRegion = qdMeasurement_->GetQueuingRegion();
		const QueuingRegion lossRegion = lossMeasurement_->GetQueuingRegion();
		if (qdRegion == QueuingRegion::Jqr) {
			congestionReason_ = CongestionReason::QueuingDelay;
			return QueuingRegion::Jqr;
		}
		if (lossRegion == QueuingRegion::Jqr) {
			congestionReason_ = CongestionReason::Loss;
			return QueuingRegion::Jqr;
		}
		if (qdRegion == QueuingRegion::Dqr && lossRegion == QueuingRegion::Dqr) {
			congestionReason_ = CongestionReason::None;
			return QueuingRegion::Dqr;
		}
		return QueuingRegion::Indeterminate;
	}

	QueuingRegion UpdateEarlyWarningSignal()
	{
		return UpdateCongestionSignal(
			config_.queuingDelayEarlyWarningJqr,
			config_.queuingDelayEarlyWarningDqr,
			config_.lossEarlyWarningJqr,
			config_.lossEarlyWarningDqr);
	}

	QueuingRegion UpdateCongestedSignal()
	{
		return UpdateCongestionSignal(
			config_.queuingDelayCongestedJqr,
			config_.queuingDelayCongestedDqr,
			config_.lossCongestedJqr,
			config_.lossCongestedDqr);
	}

	void UpdateCtrTrend(const PacketInfo& packetInfo, int64_t sendDeltaUs, int64_t recvDeltaUs, bool isLost)
	{
		if (!congestedCtrTrend_) {
			return;
		}
		if (!congestedPacketGroup_) {
			congestedPacketGroup_.emplace(config_.congestedPacketGroup, config_.weightedLoss, 0);
		}
		if (congestedPacketGroup_->Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost) ==
			PacketGroupAddResult::GroupFinalized) {
			congestedTrafficStats_.Merge(congestedPacketGroup_->Traffic());
			TrafficStats sampleStats(config_.weightedLoss);
			sampleStats.Merge(congestedPacketGroup_->Traffic());
			const double ctr = sampleStats.CapturedTrafficRatio();
			const double quantizedCtr =
				static_cast<int>((ctr + (config_.congestedCtrEpsilon / 2.0)) / config_.congestedCtrEpsilon) *
				config_.congestedCtrEpsilon;
			congestedCtrTrend_->AddValue(quantizedCtr);
			congestedPacketGroup_.emplace(
				config_.congestedPacketGroup,
				config_.weightedLoss,
				congestedPacketGroup_->PropagatedQueuingDelayUs());
		}
	}

	void EstimateAvailableChannelCapacity()
	{
		estimateTrafficStats_.reset();
		if (packetGroups_.empty() || probePacketGroup_) {
			return;
		}

		bool useWindow = false;
		bool isAggregateValid = true;
		int minGroupIndex = 0;
		int maxGroupIndex = static_cast<int>(packetGroups_.size()) - 1;
		switch (congestionReason_) {
			case CongestionReason::QueuingDelay: {
				auto range = qdMeasurement_->GroupRange();
				minGroupIndex = range.first;
				maxGroupIndex = range.second;
				break;
			}
			case CongestionReason::Loss: {
				auto range = lossMeasurement_->GroupRange();
				minGroupIndex = range.first;
				maxGroupIndex = range.second;
				break;
			}
			default:
				useWindow = true;
				isAggregateValid = false;
				break;
		}

		TrafficStats aggregate(config_.weightedLoss);
		for (int index = maxGroupIndex; index >= minGroupIndex; --index) {
			const PacketGroup& packetGroup = packetGroups_[static_cast<size_t>(index)];
			if (!packetGroup.IsFinalized()) {
				continue;
			}
			aggregate.Merge(packetGroup.Traffic());
			if (useWindow && aggregate.DurationUs() > config_.estimationWindowDuration.count()) {
				isAggregateValid = true;
				break;
			}
		}
		if (isAggregateValid) {
			const int64_t previousEstimateBps = estimatedAvailableChannelCapacityBps_;
			const int64_t aggregateEstimateBps = aggregate.AcknowledgedBitrate();
			const bool allowDownwardUpdate =
				congestionState_ == CongestionState::Congested ||
				congestionReason_ == CongestionReason::Loss;
			if (aggregateEstimateBps > estimatedAvailableChannelCapacityBps_ || allowDownwardUpdate) {
				estimatedAvailableChannelCapacityBps_ = aggregateEstimateBps;
			}
			estimateTrafficStats_ = aggregate;
			if (congestionDebugEnabled() &&
				estimatedAvailableChannelCapacityBps_ != previousEstimateBps) {
				std::printf(
					"[BWE_TRACE] source=aggregate state=%s reason=%s prevEstimateBps=%lld newEstimateBps=%lld ackedPackets=%d lostPackets=%d sendDeltaUs=%lld recvDeltaUs=%lld weightedLoss=%.6f capturedRatio=%.6f\n",
					congestionStateStr(congestionState_),
					congestionReasonStr(congestionReason_),
					static_cast<long long>(previousEstimateBps),
					static_cast<long long>(estimatedAvailableChannelCapacityBps_),
					aggregate.AckedPackets(),
					aggregate.LostPackets(),
					static_cast<long long>(aggregate.SendDeltaUs()),
					static_cast<long long>(aggregate.RecvDeltaUs()),
					aggregate.WeightedLoss(),
					aggregate.CapturedTrafficRatio());
			}
		}
	}

	void CreateCtrTrend()
	{
		congestedCtrTrend_ = std::make_unique<mediasoup::ccutils::TrendDetector<double>>(config_.congestedCtrTrend);
		congestedTrafficStats_ = TrafficStats(config_.weightedLoss);
		congestedPacketGroup_.reset();
	}

	void ClearCtrTrend()
	{
		congestedCtrTrend_.reset();
		congestedPacketGroup_.reset();
		congestedTrafficStats_ = TrafficStats(config_.weightedLoss);
	}

	bool CongestionDetectionStateMachine()
	{
		const CongestionState fromState = congestionState_;
		CongestionState toState = congestionState_;
		switch (fromState) {
			case CongestionState::None:
				if (UpdateEarlyWarningSignal() == QueuingRegion::Jqr) {
					toState = CongestionState::EarlyWarning;
				}
				break;
			case CongestionState::EarlyWarning:
				if (UpdateCongestedSignal() == QueuingRegion::Jqr) {
					toState = CongestionState::Congested;
				} else if (UpdateEarlyWarningSignal() == QueuingRegion::Dqr) {
					toState = CongestionState::None;
				}
				break;
			case CongestionState::Congested:
				if (UpdateCongestedSignal() == QueuingRegion::Dqr) {
					toState = CongestionState::None;
				}
				break;
		}

		bool changed = false;
		if (toState != fromState) {
			EstimateAvailableChannelCapacity();
			congestionState_ = toState;
			congestionStateSwitchedAt_ = std::chrono::steady_clock::now();
			changed = true;
		}

		if (congestionState_ == CongestionState::Congested && fromState != CongestionState::Congested) {
			CreateCtrTrend();
		} else if (congestionState_ != CongestionState::Congested) {
			ClearCtrTrend();
		}

		if (congestedCtrTrend_ &&
			congestedCtrTrend_->GetDirection() == mediasoup::ccutils::TrendDirection::Downward) {
			const int64_t congestedAckedBitrate = congestedTrafficStats_.AcknowledgedBitrate();
			if (congestedAckedBitrate < estimatedAvailableChannelCapacityBps_) {
				const int64_t previousEstimateBps = estimatedAvailableChannelCapacityBps_;
				estimatedAvailableChannelCapacityBps_ = congestedAckedBitrate;
				changed = true;
				if (congestionDebugEnabled()) {
					std::printf(
						"[BWE_TRACE] source=ctr_downward state=%s reason=%s prevEstimateBps=%lld newEstimateBps=%lld ackedPackets=%d lostPackets=%d sendDeltaUs=%lld recvDeltaUs=%lld weightedLoss=%.6f capturedRatio=%.6f\n",
						congestionStateStr(congestionState_),
						congestionReasonStr(congestionReason_),
						static_cast<long long>(previousEstimateBps),
						static_cast<long long>(estimatedAvailableChannelCapacityBps_),
						congestedTrafficStats_.AckedPackets(),
						congestedTrafficStats_.LostPackets(),
						static_cast<long long>(congestedTrafficStats_.SendDeltaUs()),
						static_cast<long long>(congestedTrafficStats_.RecvDeltaUs()),
						congestedTrafficStats_.WeightedLoss(),
						congestedTrafficStats_.CapturedTrafficRatio());
				}
			}
			CreateCtrTrend();
		}

		return changed;
	}

	CongestionDetectorConfig config_;
	double rttSeconds_{ 0.070 };
	PacketTracker packetTracker_;
	TwccFeedbackTracker twccFeedback_;
	std::vector<PacketGroup> packetGroups_;
	std::optional<ProbePacketGroup> probePacketGroup_;
	mediasoup::ccutils::ProbeRegulator probeRegulator_;
	int64_t estimatedAvailableChannelCapacityBps_{ 100000000 };
	std::optional<TrafficStats> estimateTrafficStats_;
	CongestionState congestionState_{ CongestionState::None };
	std::chrono::steady_clock::time_point congestionStateSwitchedAt_{};
	std::unique_ptr<mediasoup::ccutils::TrendDetector<double>> congestedCtrTrend_;
	TrafficStats congestedTrafficStats_;
	std::optional<PacketGroup> congestedPacketGroup_;
	CongestionReason congestionReason_{ CongestionReason::None };
	std::optional<QdMeasurement> qdMeasurement_;
	std::optional<LossMeasurement> lossMeasurement_;
};

} // namespace mediasoup::plainclient::sendsidebwe
