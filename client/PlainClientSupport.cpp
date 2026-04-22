#include "PlainClientSupport.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>

int64_t steadyNowMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

int64_t wallNowMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

qos::CanonicalTransportSnapshot makeCanonicalTransportSnapshotBase(
	int64_t timestampMs,
	const std::string& trackId,
	const std::string& producerId,
	qos::Source source,
	qos::TrackKind kind,
	double targetBitrateBps,
	double configuredBitrateBps)
{
	qos::CanonicalTransportSnapshot snapshot;
	snapshot.timestampMs = timestampMs;
	snapshot.trackId = trackId;
	snapshot.producerId = producerId;
	snapshot.source = source;
	snapshot.kind = kind;
	snapshot.targetBitrateBps = targetBitrateBps;
	snapshot.configuredBitrateBps = configuredBitrateBps;
	return snapshot;
}

void populateCanonicalTransportSnapshotFromRtcp(
	qos::CanonicalTransportSnapshot& snapshot,
	const VideoStreamRtcpState* stream,
	int frameWidth,
	int frameHeight,
	double framesPerSecond)
{
	if (stream) {
		snapshot.bytesSent = stream->octetCount;
		snapshot.packetsSent = stream->packetCount;
		snapshot.packetsLost = stream->rrCumulativeLost;
		snapshot.roundTripTimeMs = stream->rrRttMs;
		snapshot.jitterMs = stream->rrJitterMs;
	}

	snapshot.frameWidth = frameWidth;
	snapshot.frameHeight = frameHeight;
	snapshot.framesPerSecond = framesPerSecond;
}

void populateCanonicalTransportSnapshotFromThreadedStats(
	qos::CanonicalTransportSnapshot& snapshot,
	const mt::SenderStatsSnapshot& stats,
	int frameWidth,
	int frameHeight,
	double framesPerSecond)
{
	const auto deriveSenderLimitationReason = [&stats]() {
		if (qos::senderPressureActive(stats.senderPressureState)) {
			return qos::QualityLimitationReason::Bandwidth;
		}
		if (stats.senderTransportDelayMs >= 60.0 || stats.senderTransportJitterMs >= 20.0) {
			return qos::QualityLimitationReason::Bandwidth;
		}
		return qos::QualityLimitationReason::None;
	};

	snapshot.timestampMs = stats.timestampMs;
	snapshot.bytesSent = stats.octetCount;
	snapshot.packetsSent = stats.packetCount;
	snapshot.packetsLost = stats.rrCumulativeLost;
	snapshot.roundTripTimeMs = stats.rrRttMs;
	snapshot.jitterMs = stats.rrJitterMs;
	snapshot.senderTransportDelayMs = stats.senderTransportDelayMs;
	snapshot.senderTransportJitterMs = stats.senderTransportJitterMs;
	snapshot.senderPressureState = stats.senderPressureState;
	snapshot.senderLimitationReason = deriveSenderLimitationReason();
	snapshot.frameWidth = frameWidth;
	snapshot.frameHeight = frameHeight;
	snapshot.framesPerSecond = framesPerSecond;
}

size_t loadVideoTrackCountFromEnv()
{
	const char* raw = std::getenv("PLAIN_CLIENT_VIDEO_TRACK_COUNT");
	if (!raw || std::strlen(raw) == 0) return 1;
	char* end = nullptr;
	long parsed = std::strtol(raw, &end, 10);
	if (!end || *end != '\0') return 1;
	return static_cast<size_t>(std::clamp<long>(parsed, 1, 8));
}

std::vector<double> loadVideoTrackWeightsFromEnv(size_t trackCount)
{
	std::vector<double> weights(trackCount, 1.0);
	const char* raw = std::getenv("PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS");
	if (!raw || std::strlen(raw) == 0) return weights;

	std::stringstream ss(raw);
	std::string item;
	size_t index = 0;
	while (index < trackCount && std::getline(ss, item, ',')) {
		try {
			double weight = std::stod(item);
			if (std::isfinite(weight) && weight > 0.0) weights[index] = weight;
		} catch (...) {
		}
		++index;
	}

	return weights;
}

std::vector<std::string> loadVideoSourcePathsFromEnv()
{
	std::vector<std::string> paths;
	const char* raw = std::getenv("PLAIN_CLIENT_VIDEO_SOURCES");
	if (!raw || std::strlen(raw) == 0) return paths;

	std::istringstream ss(raw);
	std::string item;
	while (std::getline(ss, item, ',')) {
		if (!item.empty()) paths.push_back(item);
	}
	return paths;
}
