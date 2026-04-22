#pragma once

#include "qos/SenderPressureState.h"

#include <cstdint>

namespace mediasoup::plainclient {

struct SenderPressureSample {
	uint32_t aggregateTargetBitrateBps = 0;
	uint32_t effectivePacingBitrateBps = 0;
	uint32_t queuedFreshVideoPackets = 0;
	double roundTripTimeMs = -1.0;
	double jitterMs = -1.0;
	uint64_t transportWouldBlockTotal = 0;
	uint64_t queuedVideoRetentions = 0;
};

class SenderPressureTracker {
public:
	::qos::SenderPressureState Update(const SenderPressureSample& sample)
	{
		const uint32_t queue = sample.queuedFreshVideoPackets;
		const uint32_t previousQueue = lastQueuedFreshVideoPackets_;
		const uint64_t wouldBlockDelta =
			counterDelta(sample.transportWouldBlockTotal, lastTransportWouldBlockTotal_);
		const uint64_t retentionDelta =
			counterDelta(sample.queuedVideoRetentions, lastQueuedVideoRetentions_);
		const bool sendBlocked = wouldBlockDelta > 0 || retentionDelta > 0;
		const bool pacingClamped =
			isPacingClamped(sample.aggregateTargetBitrateBps, sample.effectivePacingBitrateBps);
		const bool transportRestricted = pacingClamped || sendBlocked;
		const bool queueGrowing = queue > previousQueue + kQueueGrowthPackets;
		const bool backlogNetworkWarning =
			queue >= kWarningQueuePackets && hasElevatedNetworkMetrics(sample);
		const bool growingBacklogNetworkWarning =
			queue >= kGrowingQueuePackets && queueGrowing && hasElevatedNetworkMetrics(sample);
		const bool warningEvidence =
			backlogNetworkWarning ||
			growingBacklogNetworkWarning ||
			(transportRestricted &&
				(queue >= kWarningQueuePackets
					|| (queue >= kGrowingQueuePackets && queueGrowing)
					|| (sendBlocked && queue >= kBlockedQueuePackets)));
		const bool congestedEvidence =
			transportRestricted &&
			(queue >= kCongestedQueuePackets
				|| (queue >= kWarningQueuePackets && queueGrowing)
				|| (sendBlocked && queue >= kWarningQueuePackets));
		const bool clearEvidence = !transportRestricted && queue <= kClearQueuePackets;

		warningEvidenceSamples_ = warningEvidence ? warningEvidenceSamples_ + 1 : 0;
		congestedEvidenceSamples_ = congestedEvidence ? congestedEvidenceSamples_ + 1 : 0;
		clearEvidenceSamples_ = clearEvidence ? clearEvidenceSamples_ + 1 : 0;

		switch (state_) {
			case ::qos::SenderPressureState::None:
				if (congestedEvidenceSamples_ >= 2) {
					state_ = ::qos::SenderPressureState::Congested;
				} else if (warningEvidenceSamples_ >= 2) {
					state_ = ::qos::SenderPressureState::Warning;
				}
				break;

			case ::qos::SenderPressureState::Warning:
				if (congestedEvidenceSamples_ >= 2) {
					state_ = ::qos::SenderPressureState::Congested;
				} else if (clearEvidenceSamples_ >= 3) {
					state_ = ::qos::SenderPressureState::None;
				}
				break;

			case ::qos::SenderPressureState::Congested:
				if (clearEvidenceSamples_ >= 3) {
					state_ = ::qos::SenderPressureState::None;
				}
				break;
		}

		lastQueuedFreshVideoPackets_ = queue;
		lastTransportWouldBlockTotal_ = sample.transportWouldBlockTotal;
		lastQueuedVideoRetentions_ = sample.queuedVideoRetentions;

		return state_;
	}

	void Reset()
	{
		state_ = ::qos::SenderPressureState::None;
		lastQueuedFreshVideoPackets_ = 0;
		lastTransportWouldBlockTotal_ = 0;
		lastQueuedVideoRetentions_ = 0;
		warningEvidenceSamples_ = 0;
		congestedEvidenceSamples_ = 0;
		clearEvidenceSamples_ = 0;
	}

	::qos::SenderPressureState state() const
	{
		return state_;
	}

private:
	static constexpr uint32_t kGrowingQueuePackets = 48;
	static constexpr uint32_t kBlockedQueuePackets = 64;
	static constexpr uint32_t kWarningQueuePackets = 72;
	static constexpr uint32_t kCongestedQueuePackets = 112;
	static constexpr uint32_t kClearQueuePackets = 24;
	static constexpr uint32_t kQueueGrowthPackets = 12;

	static uint64_t counterDelta(uint64_t current, uint64_t previous)
	{
		return current >= previous ? (current - previous) : 0;
	}

	static bool isPacingClamped(uint32_t aggregateTargetBps, uint32_t effectivePacingBps)
	{
		return
			aggregateTargetBps > 0 &&
			effectivePacingBps > 0 &&
			static_cast<uint64_t>(effectivePacingBps) * 10u <
				static_cast<uint64_t>(aggregateTargetBps) * 9u;
	}

	static bool hasElevatedNetworkMetrics(const SenderPressureSample& sample)
	{
		return
			(sample.roundTripTimeMs >= 60.0) ||
			(sample.jitterMs >= 20.0);
	}

	::qos::SenderPressureState state_{ ::qos::SenderPressureState::None };
	uint32_t lastQueuedFreshVideoPackets_{ 0 };
	uint64_t lastTransportWouldBlockTotal_{ 0 };
	uint64_t lastQueuedVideoRetentions_{ 0 };
	int warningEvidenceSamples_{ 0 };
	int congestedEvidenceSamples_{ 0 };
	int clearEvidenceSamples_{ 0 };
};

} // namespace mediasoup::plainclient
