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
	return (double)(db * 8 * 1000) / (double)(curMs - prevMs);
}

inline double computeLossRate(uint64_t sentDelta, uint64_t lostDelta) {
	auto total = sentDelta + lostDelta;
	return total > 0 ? clampUnit((double)lostDelta / (double)total) : 0;
}

inline double computeEwma(double cur, double prev, double alpha = EWMA_ALPHA) {
	if (!std::isfinite(prev)) return fin(cur);
	return alpha * fin(cur) + (1.0 - alpha) * fin(prev);
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
	if (s.bandwidthLimited || s.lossEwma >= NETWORK_WARN_LOSS_RATE || s.rttEwma >= NETWORK_WARN_RTT_MS)
		return Reason::Network;
	return Reason::Unknown;
}

struct DeriveContext {
	bool activeOverride = false;
	bool manual = false;
	int cpuSampleCount = 0;
};

inline DerivedSignals deriveSignals(const RawSenderSnapshot& cur, const RawSenderSnapshot* prev,
	const DerivedSignals* prevSig, const DeriveContext& ctx = {})
{
	DerivedSignals s;
	s.packetsSentDelta = prev ? counterDelta(cur.packetsSent, prev->packetsSent) : 0;
	s.packetsLostDelta = prev ? counterDelta(cur.packetsLost, prev->packetsLost) : 0;
	s.retransmittedPacketsSentDelta = prev ? counterDelta(cur.retransmittedPacketsSent, prev->retransmittedPacketsSent) : 0;
	s.sendBitrateBps = prev ? computeBitrateBps(cur.bytesSent, prev->bytesSent, cur.timestampMs, prev->timestampMs) : 0;
	s.targetBitrateBps = std::max(0.0, fin(cur.targetBitrateBps));
	s.bitrateUtilization = computeUtilization(s.sendBitrateBps, s.targetBitrateBps, cur.configuredBitrateBps);
	s.lossRate = computeLossRate(s.packetsSentDelta, s.packetsLostDelta);
	s.lossEwma = computeEwma(s.lossRate, prevSig ? prevSig->lossEwma : s.lossRate);

	s.rttMs = cur.roundTripTimeMs >= 0 ? cur.roundTripTimeMs : (prevSig ? prevSig->rttMs : 0);
	s.rttEwma = cur.roundTripTimeMs >= 0
		? computeEwma(s.rttMs, prevSig ? prevSig->rttEwma : s.rttMs)
		: (prevSig ? prevSig->rttEwma : 0);

	s.jitterMs = cur.jitterMs >= 0 ? cur.jitterMs : (prevSig ? prevSig->jitterMs : 0);
	s.jitterEwma = cur.jitterMs >= 0
		? computeEwma(s.jitterMs, prevSig ? prevSig->jitterEwma : s.jitterMs)
		: (prevSig ? prevSig->jitterEwma : 0);

	s.cpuLimited = (cur.qualityLimitationReason == QualityLimitationReason::Cpu);
	s.bandwidthLimited = (cur.qualityLimitationReason == QualityLimitationReason::Bandwidth)
		&& s.bitrateUtilization < NETWORK_CONGESTED_UTILIZATION;
	s.reason = classifyReason(s, ctx.activeOverride, ctx.manual, ctx.cpuSampleCount);
	return s;
}

} // namespace qos
