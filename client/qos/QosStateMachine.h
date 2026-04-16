#pragma once
#include "QosTypes.h"
#include <algorithm>
#include <cmath>
#include <limits>

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

inline bool isFastRecoveryHealthy(const DerivedSignals& s, const Thresholds& t) {
	double recoveryJitter = std::max(t.stableJitterMs, t.warnJitterMs);
	double rawJitter = std::isfinite(s.jitterMs) ? std::max(0.0, s.jitterMs) : std::numeric_limits<double>::infinity();
	double targetBitrateBps = std::max(0.0, s.targetBitrateBps);
	bool sendReady = targetBitrateBps <= 0.0 || s.sendBitrateBps >= targetBitrateBps * 0.85;
	return s.lossEwma < t.stableLossRate && s.rttEwma < t.stableRttMs
		&& rawJitter < recoveryJitter && sendReady && !s.bandwidthLimited && !s.cpuLimited;
}

inline int nextCounter(int current, bool matched) {
	return matched ? current + 1 : 0;
}

inline StateMachineContext createInitialQosStateMachineContext(int64_t nowMs) {
	StateMachineContext context;
	context.state = State::Stable;
	context.enteredAtMs = nowMs;
	return context;
}

inline Quality mapStateToQuality(State state, const DerivedSignals& signals) {
	if (state == State::Congested && signals.sendBitrateBps <= 0) return Quality::Lost;
	switch (state) {
		case State::Stable: return Quality::Excellent;
		case State::EarlyWarning: return Quality::Good;
		case State::Congested: return Quality::Poor;
		case State::Recovering: return Quality::Good;
	}
	return Quality::Good;
}

inline Quality mapStateToQuality(State state) {
	return mapStateToQuality(state, DerivedSignals{});
}

inline StateTransitionResult evaluateStateTransition(
	const StateMachineContext& ctx, const DerivedSignals& signals,
	const Profile& profile, int64_t nowMs)
{
	auto& t = profile.thresholds;
	bool healthy = isHealthy(signals, t);
	bool recoveryHealthy = isRecoveryHealthy(signals, t);
	bool fastRecoveryHealthy = !recoveryHealthy && isFastRecoveryHealthy(signals, t);
	bool warning = isWarning(signals, t);
	bool congested = isCongested(signals, t);

	StateMachineContext next = ctx;
	next.consecutiveHealthySamples = nextCounter(ctx.consecutiveHealthySamples, healthy);
	next.consecutiveRecoverySamples = nextCounter(ctx.consecutiveRecoverySamples, recoveryHealthy);
	next.consecutiveFastRecoverySamples = nextCounter(ctx.consecutiveFastRecoverySamples, fastRecoveryHealthy);
	next.consecutiveWarningSamples = nextCounter(ctx.consecutiveWarningSamples, warning);
	next.consecutiveCongestedSamples = nextCounter(ctx.consecutiveCongestedSamples, congested);

	State nextState = ctx.state;
	bool transitioned = false;

	switch (ctx.state) {
		case State::Stable:
			if (next.consecutiveWarningSamples >= 2) nextState = State::EarlyWarning;
			break;

		case State::EarlyWarning:
			if (next.consecutiveCongestedSamples >= 2) {
				nextState = State::Congested;
			} else if (next.consecutiveHealthySamples >= 3) {
				nextState = State::Stable;
			}
			break;

		case State::Congested: {
			int64_t congestedSinceMs = ctx.lastCongestedAtMs > 0 ? ctx.lastCongestedAtMs : ctx.enteredAtMs;
			bool cooldownElapsed = nowMs - congestedSinceMs >= profile.recoveryCooldownMs;
			bool regularRecoveryReady = next.consecutiveRecoverySamples >= 5;
			bool fastRecoveryReady = next.consecutiveFastRecoverySamples >= 2;
			if (cooldownElapsed && (regularRecoveryReady || fastRecoveryReady)) nextState = State::Recovering;
			break;
		}

		case State::Recovering:
			if (next.consecutiveCongestedSamples >= 2) {
				nextState = State::Congested;
			} else if (next.consecutiveHealthySamples >= 5) {
				nextState = State::Stable;
			}
			break;
	}

	if (nextState != ctx.state) {
		transitioned = true;
		next.state = nextState;
		next.enteredAtMs = nowMs;
		if (nextState == State::Congested) next.lastCongestedAtMs = nowMs;
		if (nextState == State::Recovering) next.lastRecoveryAtMs = nowMs;
	}

	return {next, mapStateToQuality(next.state, signals), transitioned};
}

} // namespace qos
