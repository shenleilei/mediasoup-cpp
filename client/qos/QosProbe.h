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

inline ProbeResult evaluateProbe(ProbeContext& ctx, const DerivedSignals& signals,
	const Profile& profile)
{
	auto& t = profile.thresholds;
	if (isRecoveryHealthy(signals, t))
		ctx.healthySamples++;
	else
		ctx.badSamples++;

	if (ctx.healthySamples >= ctx.requiredHealthySamples)
		return ProbeResult::Successful;
	if (ctx.badSamples >= ctx.requiredBadSamples)
		return ProbeResult::Failed;
	return ProbeResult::Inconclusive;
}

} // namespace qos
