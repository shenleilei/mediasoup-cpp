#pragma once

#include "CongestionDetector.h"

namespace mediasoup::plainclient::sendsidebwe {

class SendSideBwe {
public:
	explicit SendSideBwe(const CongestionDetectorConfig& config = {})
		: congestionDetector_(config)
	{
	}

	void Reset()
	{
		congestionDetector_.Reset();
	}

	uint16_t RecordPacketSendAndGetSequenceNumber(
		int64_t atUs,
		size_t size,
		bool isRtx = false,
		mediasoup::ccutils::ProbeClusterId probeClusterId = mediasoup::ccutils::ProbeClusterIdInvalid,
		bool isProbe = false)
	{
		return congestionDetector_.RecordPacketSendAndGetSequenceNumber(
			atUs, size, isRtx, probeClusterId, isProbe);
	}

	void SeedSentPacketForTest(
		uint16_t sequenceNumber,
		int64_t sendTimeUs,
		size_t size,
		bool isRtx = false)
	{
		congestionDetector_.SeedSentPacketForTest(sequenceNumber, sendTimeUs, size, isRtx);
	}

	void HandleTransportFeedback(const mediasoup::plainclient::TransportCcFeedback& feedback, int64_t atUs)
	{
		congestionDetector_.HandleTransportFeedback(feedback, atUs);
	}

	void UpdateRtt(double rttSeconds)
	{
		congestionDetector_.UpdateRtt(rttSeconds);
	}

	void SeedEstimatedAvailableChannelCapacityForTest(int64_t capacityBps)
	{
		congestionDetector_.SeedEstimatedAvailableChannelCapacityForTest(capacityBps);
	}

	CongestionState GetCongestionState() const
	{
		return congestionDetector_.GetCongestionState();
	}

	int64_t EstimatedAvailableChannelCapacityBps() const
	{
		return congestionDetector_.EstimatedAvailableChannelCapacityBps();
	}

	bool CanProbe() const { return congestionDetector_.CanProbe(); }
	std::chrono::microseconds ProbeDuration() const { return congestionDetector_.ProbeDuration(); }

private:
	CongestionDetector congestionDetector_;
};

} // namespace mediasoup::plainclient::sendsidebwe
