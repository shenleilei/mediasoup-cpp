#pragma once

#include "PacketGroup.h"

#include "../ccutils/ProbeTypes.h"

#include <algorithm>

namespace mediasoup::plainclient::sendsidebwe {

struct ProbePacketGroupConfig {
	PacketGroupConfig packetGroup{ PacketGroupConfig{16384, std::chrono::minutes(1)} };
	uint32_t settleWaitNumRtt{ 5 };
	std::chrono::microseconds settleWaitMin{ std::chrono::milliseconds(250) };
	std::chrono::microseconds settleWaitMax{ std::chrono::seconds(5) };
};

class ProbePacketGroup {
public:
	ProbePacketGroup(
		const ProbePacketGroupConfig& config,
		const WeightedLossConfig& weightedLossConfig,
		const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo)
		: config_(config)
		, probeClusterInfo_(probeClusterInfo)
		, packetGroup_(config.packetGroup, weightedLossConfig, 0)
	{
	}

	void ProbeClusterDone(const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo)
	{
		if (probeClusterInfo_.id != probeClusterInfo.id) {
			return;
		}
		probeClusterInfo_.result = probeClusterInfo.result;
		doneAt_ = std::chrono::steady_clock::now();
	}

	const mediasoup::ccutils::ProbeClusterInfo& ProbeClusterInfoValue() const
	{
		return probeClusterInfo_;
	}

	bool MaybeFinalizeProbe(
		uint64_t maxSequenceNumber,
		double rttSeconds,
		mediasoup::ccutils::ProbeClusterInfo* outProbeClusterInfo) const
	{
		if (!outProbeClusterInfo || doneAt_ == std::chrono::steady_clock::time_point{}) {
			return false;
		}

		if (maxSequenceNumber != 0 && maxSequenceNumber_ >= maxSequenceNumber) {
			*outProbeClusterInfo = probeClusterInfo_;
			return true;
		}

		auto settleWait = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::duration<double>(rttSeconds * static_cast<double>(config_.settleWaitNumRtt)));
		settleWait = std::max(settleWait, config_.settleWaitMin);
		settleWait = std::min(settleWait, config_.settleWaitMax);
		if (std::chrono::steady_clock::now() - doneAt_ < settleWait) {
			return false;
		}

		*outProbeClusterInfo = probeClusterInfo_;
		return true;
	}

	void Add(const PacketInfo& packetInfo, int64_t sendDeltaUs, int64_t recvDeltaUs, bool isLost)
	{
		if (packetInfo.probeClusterId != probeClusterInfo_.id) {
			return;
		}
		maxSequenceNumber_ = std::max(maxSequenceNumber_, packetInfo.sequenceNumber);
		(void)packetGroup_.Add(packetInfo, sendDeltaUs, recvDeltaUs, isLost);
	}

	int64_t PropagatedQueuingDelayUs() const
	{
		return packetGroup_.PropagatedQueuingDelayUs();
	}

	TrafficStats Traffic() const
	{
		return packetGroup_.Traffic();
	}

private:
	ProbePacketGroupConfig config_;
	mediasoup::ccutils::ProbeClusterInfo probeClusterInfo_;
	PacketGroup packetGroup_;
	uint64_t maxSequenceNumber_{ 0 };
	std::chrono::steady_clock::time_point doneAt_{};
};

} // namespace mediasoup::plainclient::sendsidebwe
