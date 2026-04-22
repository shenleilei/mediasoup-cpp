#pragma once

#include "RtcpHandler.h"
#include "ThreadTypes.h"

#include "qos/QosController.h"

#include <cstdint>
#include <string>
#include <vector>

int64_t steadyNowMs();
int64_t wallNowMs();

qos::CanonicalTransportSnapshot makeCanonicalTransportSnapshotBase(
	int64_t timestampMs,
	const std::string& trackId,
	const std::string& producerId,
	qos::Source source,
	qos::TrackKind kind,
	double targetBitrateBps,
	double configuredBitrateBps);

void populateCanonicalTransportSnapshotFromRtcp(
	qos::CanonicalTransportSnapshot& snapshot,
	const VideoStreamRtcpState* stream,
	int frameWidth,
	int frameHeight,
	double framesPerSecond);

void populateCanonicalTransportSnapshotFromThreadedStats(
	qos::CanonicalTransportSnapshot& snapshot,
	const mt::SenderStatsSnapshot& stats,
	int frameWidth,
	int frameHeight,
	double framesPerSecond);

size_t loadVideoTrackCountFromEnv();
std::vector<double> loadVideoTrackWeightsFromEnv(size_t trackCount);
std::vector<std::string> loadVideoSourcePathsFromEnv();
