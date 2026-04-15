#pragma once
#include "QosTypes.h"
#include <algorithm>

namespace qos {

inline bool isWarning(const DerivedSignals& s, const Thresholds& t) {
	return s.lossEwma >= t.warnLossRate || s.rttEwma >= t.warnRttMs
		|| s.jitterEwma >= t.warnJitterMs || s.bandwidthLimited || s.cpuLimited;
}

inline bool isCongested(const DerivedSignals& s, const Thresholds& t) {
	return s.bandwidthLimited || s.lossEwma >= t.congestedLossRate
		|| s.rttEwma >= t.congestedRttMs || s.jitterEwma >= t.congestedJitterMs;
}

inline bool isHealthy(const DerivedSignals& s, const Thresholds& t) {
	return s.lossEwma < t.stableLossRate && s.rttEwma < t.stableRttMs
		&& s.jitterEwma < t.stableJitterMs && !s.bandwidthLimited && !s.cpuLimited;
}

inline bool isRecoveryHealthy(const DerivedSignals& s, const Thresholds& t) {
	double recoveryJitter = std::max(t.stableJitterMs, t.warnJitterMs);
	return s.lossEwma < t.stableLossRate && s.rttEwma < t.stableRttMs
		&& s.jitterEwma < recoveryJitter && !s.bandwidthLimited && !s.cpuLimited;
}

inline Quality mapStateToQuality(State state) {
	switch (state) {
		case State::Stable: return Quality::Excellent;
		case State::EarlyWarning: return Quality::Good;
		case State::Congested: return Quality::Poor;
		case State::Recovering: return Quality::Good;
	}
	return Quality::Good;
}

inline StateTransitionResult evaluateStateTransition(
	const StateMachineContext& ctx, const DerivedSignals& signals,
	const Profile& profile, int64_t nowMs)
{
	auto& t = profile.thresholds;
	StateMachineContext next = ctx;
	bool transitioned = false;

	// Update consecutive counters
	next.consecutiveHealthySamples = isHealthy(signals, t) ? ctx.consecutiveHealthySamples + 1 : 0;
	next.consecutiveRecoverySamples = isRecoveryHealthy(signals, t) ? ctx.consecutiveRecoverySamples + 1 : 0;
	next.consecutiveWarningSamples = isWarning(signals, t) ? ctx.consecutiveWarningSamples + 1 : 0;
	next.consecutiveCongestedSamples = isCongested(signals, t) ? ctx.consecutiveCongestedSamples + 1 : 0;

	switch (ctx.state) {
	case State::Stable:
		if (isCongested(signals, t)) {
			next.state = State::Congested;
			next.enteredAtMs = nowMs;
			next.lastCongestedAtMs = nowMs;
			transitioned = true;
		} else if (isWarning(signals, t)) {
			next.state = State::EarlyWarning;
			next.enteredAtMs = nowMs;
			transitioned = true;
		}
		break;

	case State::EarlyWarning:
		if (isCongested(signals, t)) {
			next.state = State::Congested;
			next.enteredAtMs = nowMs;
			next.lastCongestedAtMs = nowMs;
			transitioned = true;
		} else if (next.consecutiveHealthySamples >= 3) {
			next.state = State::Stable;
			next.enteredAtMs = nowMs;
			transitioned = true;
		}
		break;

	case State::Congested:
		if (next.consecutiveRecoverySamples >= 3) {
			int64_t cooldown = profile.recoveryCooldownMs;
			if (nowMs - ctx.lastCongestedAtMs >= cooldown) {
				next.state = State::Recovering;
				next.enteredAtMs = nowMs;
				next.lastRecoveryAtMs = nowMs;
				transitioned = true;
			}
		}
		break;

	case State::Recovering:
		if (isCongested(signals, t)) {
			next.state = State::Congested;
			next.enteredAtMs = nowMs;
			next.lastCongestedAtMs = nowMs;
			transitioned = true;
		} else if (next.consecutiveHealthySamples >= 5) {
			next.state = State::Stable;
			next.enteredAtMs = nowMs;
			transitioned = true;
		}
		break;
	}

	if (transitioned) {
		next.consecutiveHealthySamples = 0;
		next.consecutiveRecoverySamples = 0;
		next.consecutiveWarningSamples = 0;
		next.consecutiveCongestedSamples = 0;
	}

	return {next, mapStateToQuality(next.state), transitioned};
}

} // namespace qos
