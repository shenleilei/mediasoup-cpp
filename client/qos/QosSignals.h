#pragma once
#include "QosTypes.h"
#include "QosConstants.h"
#include <algorithm>
#include <cmath>

namespace qos {

inline double clampUnit(double v) { return std::clamp(v, 0.0, 1.0); }
inline double fin(double v) { return std::isfinite(v) ? v : 0.0; }

inline uint64_t counterDelta(uint64_t cur, uint64_t prev) {
	return cur > prev ? cur - prev : 0;
}

inline double computeBitrateBps(uint64_t curBytes, uint64_t prevBytes, int64_t curMs, int64_t prevMs) {
	auto db = counterDelta(curBytes, prevBytes);
	if (curMs <= prevMs || db == 0) return 0;
	return static_cast<double>(db) * 8000.0 / static_cast<double>(curMs - prevMs);
}

inline double computeLossRate(uint64_t sentDelta, uint64_t lostDelta) {
	auto total = sentDelta + lostDelta;
	return total > 0 ? clampUnit((double)lostDelta / (double)total) : 0;
}

inline double computeEwma(double cur, double prev, double alpha = EWMA_ALPHA) {
	double safeAlpha = clampUnit(fin(alpha));
	double safeCur = fin(cur);
	double safePrev = fin(prev);
	if (safeAlpha <= 0.0) return safePrev;
	if (safeAlpha >= 1.0) return safeCur;
	if (!std::isfinite(prev)) return safeCur;
	return safeAlpha * safeCur + (1.0 - safeAlpha) * safePrev;
}

inline double computeUtilization(double sendBps, double targetBps, double configuredBps) {
	double denom = targetBps > 0 ? targetBps : configuredBps;
	if (denom <= 0) return sendBps > 0 ? 1.0 : 0.0;
	return sendBps / denom;
}

inline Reason classifyReason(const DerivedSignals& s, bool activeOverride, bool manual, int cpuSampleCount) {
	if (activeOverride) return Reason::ServerOverride;
	if (manual) return Reason::Manual;
	if (s.cpuLimited && cpuSampleCount >= CPU_REASON_MIN_SAMPLES) return Reason::Cpu;
	if (s.bandwidthLimited || senderPressureActive(s.senderPressureState)
		|| s.senderLimitationReason == QualityLimitationReason::Bandwidth
		|| s.lossEwma >= NETWORK_WARN_LOSS_RATE || s.rttEwma >= NETWORK_WARN_RTT_MS)
		return Reason::Network;
	return Reason::Unknown;
}

struct DeriveContext {
	bool activeOverride = false;
	bool manual = false;
	int cpuSampleCount = 0;
	std::optional<double> ewmaAlpha;
};

inline DerivedSignals deriveSignals(const CanonicalTransportSnapshot& cur, const CanonicalTransportSnapshot* prev,
	const DerivedSignals* prevSig, const DeriveContext& ctx = {})
{
	DerivedSignals s;
	double ewmaAlpha = ctx.ewmaAlpha.value_or(EWMA_ALPHA);
	s.packetsSentDelta = prev ? counterDelta(cur.packetsSent, prev->packetsSent) : 0;
	s.packetsLostDelta = prev ? counterDelta(cur.packetsLost, prev->packetsLost) : 0;
	s.retransmittedPacketsSentDelta = prev ? counterDelta(cur.retransmittedPacketsSent, prev->retransmittedPacketsSent) : 0;
	s.sendBitrateBps = prev ? computeBitrateBps(cur.bytesSent, prev->bytesSent, cur.timestampMs, prev->timestampMs) : 0;
	s.targetBitrateBps = std::max(0.0, fin(cur.targetBitrateBps));
	s.bitrateUtilization = computeUtilization(s.sendBitrateBps, s.targetBitrateBps, cur.configuredBitrateBps);
	s.lossRate = computeLossRate(s.packetsSentDelta, s.packetsLostDelta);
	s.lossEwma = computeEwma(s.lossRate, prevSig ? prevSig->lossEwma : s.lossRate, ewmaAlpha);

	const bool rawRttPresent = cur.roundTripTimeMs >= 0;
	s.rttMs = rawRttPresent
		? std::max(0.0, cur.roundTripTimeMs)
		: (prevSig ? std::max(0.0, prevSig->rttMs) : 0);
	s.rttEwma = rawRttPresent
		? computeEwma(s.rttMs, prevSig ? prevSig->rttEwma : s.rttMs, ewmaAlpha)
		: (prevSig ? prevSig->rttEwma : 0);

	const bool rawJitterPresent = cur.jitterMs >= 0;
	s.jitterMs = rawJitterPresent
		? std::max(0.0, cur.jitterMs)
		: (prevSig ? std::max(0.0, prevSig->jitterMs) : 0);
	s.jitterEwma = rawJitterPresent
		? computeEwma(s.jitterMs, prevSig ? prevSig->jitterEwma : s.jitterMs, ewmaAlpha)
		: (prevSig ? prevSig->jitterEwma : 0);

	s.senderPressureState = cur.senderPressureState;
	s.senderLimitationReason = cur.senderLimitationReason;
	s.cpuLimited =
		(cur.qualityLimitationReason == QualityLimitationReason::Cpu) ||
		(cur.senderLimitationReason == QualityLimitationReason::Cpu);
	const bool browserBandwidthLimited =
		(cur.qualityLimitationReason == QualityLimitationReason::Bandwidth)
		&& s.bitrateUtilization < NETWORK_CONGESTED_UTILIZATION;
	s.bandwidthLimited =
		browserBandwidthLimited ||
		senderPressureCongested(s.senderPressureState);
	s.reason = classifyReason(s, ctx.activeOverride, ctx.manual, ctx.cpuSampleCount);
	return s;
}

} // namespace qos
