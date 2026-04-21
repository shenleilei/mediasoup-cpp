#pragma once

#include "PacketInfo.h"
#include "TrafficStats.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <unordered_set>

namespace mediasoup::plainclient::sendsidebwe {

enum class PacketGroupAddResult {
	Added,
	GroupFinalized,
	OldPacket
};

struct PacketGroupConfig {
	int minPackets{ 30 };
	std::chrono::microseconds maxWindowDuration{ std::chrono::milliseconds(500) };
};

class PacketGroup {
public:
	PacketGroup(
		const PacketGroupConfig& config = {},
		const WeightedLossConfig& weightedLoss = {},
		int64_t inheritedQueuingDelayUs = 0)
		: config_(config)
		, weightedLossConfig_(weightedLoss)
		, inheritedQueuingDelayUs_(inheritedQueuingDelayUs)
	{
	}

	PacketGroupAddResult Add(const PacketInfo& packetInfo, int64_t sendDeltaUs, int64_t recvDeltaUs, bool isLost)
	{
		if (isLost) {
			return AddLostPacket(packetInfo);
		}

		const auto inGroup = InGroup(packetInfo.sequenceNumber);
		if (inGroup != PacketGroupAddResult::Added) {
			return inGroup;
		}

		if (minSequenceNumber_ == 0 || packetInfo.sequenceNumber < minSequenceNumber_) {
			minSequenceNumber_ = packetInfo.sequenceNumber;
		}
		maxSequenceNumber_ = std::max(maxSequenceNumber_, packetInfo.sequenceNumber);

		if (minSendTimeUs_ == 0 || (packetInfo.sendTimeUs - sendDeltaUs) < minSendTimeUs_) {
			minSendTimeUs_ = packetInfo.sendTimeUs - sendDeltaUs;
		}
		maxSendTimeUs_ = std::max(maxSendTimeUs_, packetInfo.sendTimeUs);

		if (minRecvTimeUs_ == 0 || (packetInfo.recvTimeUs - recvDeltaUs) < minRecvTimeUs_) {
			minRecvTimeUs_ = packetInfo.recvTimeUs - recvDeltaUs;
		}
		maxRecvTimeUs_ = std::max(maxRecvTimeUs_, packetInfo.recvTimeUs);

		ackedPackets_++;
		ackedBytes_ += packetInfo.size;
		if (lostSequenceNumbers_.erase(packetInfo.sequenceNumber) > 0) {
			lostPackets_--;
			lostBytes_ -= packetInfo.size;
		}

		aggregateSendDeltaUs_ += sendDeltaUs;
		aggregateRecvDeltaUs_ += recvDeltaUs;

		if (ackedPackets_ == config_.minPackets ||
			(packetInfo.sendTimeUs - minSendTimeUs_) > config_.maxWindowDuration.count()) {
			isFinalized_ = true;
		}
		return PacketGroupAddResult::Added;
	}

	int64_t PropagatedQueuingDelayUs() const
	{
		const int64_t inherited = inheritedQueuingDelayUs_ + aggregateRecvDeltaUs_ - aggregateSendDeltaUs_;
		if (inherited > 0) {
			return inherited;
		}
		return std::max<int64_t>(0, aggregateRecvDeltaUs_ - aggregateSendDeltaUs_);
	}

	bool FinalizedPropagatedQueuingDelayUs(int64_t* outDelayUs) const
	{
		if (!isFinalized_ || !outDelayUs) {
			return false;
		}
		*outDelayUs = PropagatedQueuingDelayUs();
		return true;
	}

	bool IsFinalized() const { return isFinalized_; }
	int64_t MinSendTimeUs() const { return minSendTimeUs_; }
	void SendWindow(int64_t* minSendTimeUs, int64_t* maxSendTimeUs) const
	{
		if (minSendTimeUs) *minSendTimeUs = minSendTimeUs_;
		if (maxSendTimeUs) *maxSendTimeUs = maxSendTimeUs_;
	}

	TrafficStats Traffic() const
	{
		TrafficStats stats(weightedLossConfig_);
		stats.SetFields(
			minSendTimeUs_,
			maxSendTimeUs_,
			aggregateSendDeltaUs_,
			aggregateRecvDeltaUs_,
			ackedPackets_,
			ackedBytes_,
			lostPackets_,
			lostBytes_);
		return stats;
	}

	uint64_t MaxSequenceNumber() const { return maxSequenceNumber_; }

private:
	PacketGroupAddResult AddLostPacket(const PacketInfo& packetInfo)
	{
		if (packetInfo.recvTimeUs != 0) {
			return PacketGroupAddResult::Added;
		}

		const auto inGroup = InGroup(packetInfo.sequenceNumber);
		if (inGroup != PacketGroupAddResult::Added) {
			return inGroup;
		}

		if (minSequenceNumber_ == 0 || packetInfo.sequenceNumber < minSequenceNumber_) {
			minSequenceNumber_ = packetInfo.sequenceNumber;
		}
		maxSequenceNumber_ = std::max(maxSequenceNumber_, packetInfo.sequenceNumber);
		lostSequenceNumbers_.insert(packetInfo.sequenceNumber);
		lostPackets_++;
		lostBytes_ += packetInfo.size;
		return PacketGroupAddResult::Added;
	}

	PacketGroupAddResult InGroup(uint64_t sequenceNumber) const
	{
		if (isFinalized_ && sequenceNumber > maxSequenceNumber_) {
			return PacketGroupAddResult::GroupFinalized;
		}
		if (minSequenceNumber_ != 0 && sequenceNumber < minSequenceNumber_) {
			return PacketGroupAddResult::OldPacket;
		}
		return PacketGroupAddResult::Added;
	}

	PacketGroupConfig config_;
	WeightedLossConfig weightedLossConfig_;
	uint64_t minSequenceNumber_{ 0 };
	uint64_t maxSequenceNumber_{ 0 };
	int64_t minSendTimeUs_{ 0 };
	int64_t maxSendTimeUs_{ 0 };
	int64_t minRecvTimeUs_{ 0 };
	int64_t maxRecvTimeUs_{ 0 };
	int ackedPackets_{ 0 };
	int ackedBytes_{ 0 };
	int lostPackets_{ 0 };
	int lostBytes_{ 0 };
	std::unordered_set<uint64_t> lostSequenceNumbers_;
	int64_t aggregateSendDeltaUs_{ 0 };
	int64_t aggregateRecvDeltaUs_{ 0 };
	int64_t inheritedQueuingDelayUs_{ 0 };
	bool isFinalized_{ false };
};

} // namespace mediasoup::plainclient::sendsidebwe
