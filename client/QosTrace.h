#pragma once

#include "ThreadTypes.h"

#include <cstdio>
#include <string>

namespace mediasoup {
namespace plainclient {

// Formats a QOS_TRACE log line from common fields.
// Used by both Legacy and Threaded plain-client modes to avoid duplication.
// If `transportStats` is non-null (threaded mode), extended transport-controller
// fields are appended in a single formatting pass.
inline std::string formatQosTraceLine(
	int64_t tsMs,
	const char* trackId,
	const char* state,
	int level,
	const char* mode,
	const char* sample,
	int bitrateBps,
	double sendBps,
	double lossRate,
	uint64_t packetsLost,
	double rttMs,
	double jitterMs,
	int width,
	int height,
	int fps,
	int suppressed,
	const SenderStatsSnapshot* transportStats = nullptr)
{
	char buf[2048];
	int n = std::snprintf(buf, sizeof(buf),
		"[QOS_TRACE] tsMs=%lld track=%s state=%s level=%d mode=%s sample=%s bitrateBps=%d sendBps=%.0f lossRate=%.6f packetsLost=%llu rttMs=%.1f jitterMs=%.1f width=%d height=%d fps=%d suppressed=%d",
		static_cast<long long>(tsMs),
		trackId,
		state,
		level,
		mode,
		sample,
		bitrateBps,
		sendBps,
		lossRate,
		static_cast<unsigned long long>(packetsLost),
		rttMs,
		jitterMs,
		width,
		height,
		fps,
		suppressed);

	if (transportStats && n > 0 && static_cast<size_t>(n) < sizeof(buf)) {
		std::snprintf(buf + n, sizeof(buf) - n,
			" senderUsageBps=%u transportEstimateBps=%u effectivePacingBps=%u "
			"feedbackReports=%llu probePackets=%u probeActive=%d "
			"probeClusterStarts=%llu probeClusterCompletes=%llu probeClusterEarlyStops=%llu "
			"probeBytesSent=%llu wouldBlockTotal=%llu queuedVideoRetentions=%llu "
			"audioDeadlineDrops=%llu retransmissionDrops=%llu retransmissionSent=%llu "
			"queuedFreshVideoPackets=%u queuedAudioPackets=%u queuedRetransmissionPackets=%u",
			transportStats->senderUsageBitrateBps,
			transportStats->transportEstimatedBitrateBps,
			transportStats->effectivePacingBitrateBps,
			static_cast<unsigned long long>(transportStats->transportCcFeedbackReports),
			transportStats->probePacketCount,
			transportStats->probeActive ? 1 : 0,
			static_cast<unsigned long long>(transportStats->probeClusterStartCount),
			static_cast<unsigned long long>(transportStats->probeClusterCompleteCount),
			static_cast<unsigned long long>(transportStats->probeClusterEarlyStopCount),
			static_cast<unsigned long long>(transportStats->probeBytesSent),
			static_cast<unsigned long long>(transportStats->transportWouldBlockTotal),
			static_cast<unsigned long long>(transportStats->queuedVideoRetentions),
			static_cast<unsigned long long>(transportStats->audioDeadlineDrops),
			static_cast<unsigned long long>(transportStats->retransmissionDrops),
			static_cast<unsigned long long>(transportStats->retransmissionSent),
			transportStats->queuedFreshVideoPackets,
			transportStats->queuedAudioPackets,
			transportStats->queuedRetransmissionPackets);
	}

	return std::string(buf);
}

} // namespace plainclient
} // namespace mediasoup
