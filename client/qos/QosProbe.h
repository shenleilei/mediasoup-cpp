#pragma once
#include "QosTypes.h"
#include "QosStateMachine.h"

namespace qos {

inline ProbeContext beginProbe(int previousLevel, int targetLevel,
	bool previousAudioOnly, bool targetAudioOnly, int64_t nowMs)
{
	return {true, nowMs, previousLevel, targetLevel,
		previousAudioOnly, targetAudioOnly, 0, 0, 3, 2};
}

inline bool isProbeHealthy(const DerivedSignals& signals, const Thresholds& thresholds) {
	return signals.lossEwma < thresholds.stableLossRate
		&& signals.rttEwma < thresholds.stableRttMs
		&& !senderPressureActive(signals.senderPressureState)
		&& !signals.bandwidthLimited
		&& !signals.cpuLimited;
}

inline bool isProbeBad(const DerivedSignals& signals, const Thresholds& thresholds) {
	return senderPressureCongested(signals.senderPressureState)
		|| signals.bandwidthLimited
		|| signals.lossEwma >= thresholds.congestedLossRate
		|| signals.rttEwma >= thresholds.congestedRttMs
		|| signals.cpuLimited;
}

inline ProbeResult evaluateProbe(ProbeContext& ctx, const DerivedSignals& signals,
	const Profile& profile)
{
	auto& thresholds = profile.thresholds;
	if (isProbeHealthy(signals, thresholds)) {
		ctx.healthySamples++;
		ctx.badSamples = 0;
	} else if (isProbeBad(signals, thresholds)) {
		ctx.healthySamples = 0;
		ctx.badSamples++;
	}

	if (ctx.healthySamples >= ctx.requiredHealthySamples)
		return ProbeResult::Successful;
	if (ctx.badSamples >= ctx.requiredBadSamples)
		return ProbeResult::Failed;
	return ProbeResult::Inconclusive;
}

} // namespace qos
