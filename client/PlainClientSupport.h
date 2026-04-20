#pragma once

#include <nlohmann/json.hpp>
#include "qos/QosController.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

using json = nlohmann::json;

struct ServerProducerStats {
	uint64_t packetCount = 0;
	uint64_t byteCount = 0;
	uint64_t packetsLost = 0;
	double bitrateBps = 0;
	double roundTripTimeMs = -1;
	double jitterMs = -1;
};

struct CachedServerProducerStats {
	std::mutex mutex;
	std::optional<ServerProducerStats> latest;
	int64_t updatedAtSteadyMs = 0;
	int64_t requestedAtSteadyMs = 0;
	bool requestInFlight = false;
	bool lossBaseInitialized = false;
	uint64_t lossBase = 0;
};

struct CachedServerStatsResponse {
	std::mutex mutex;
	std::optional<json> latestPeerStats;
	int64_t updatedAtSteadyMs = 0;
	int64_t requestedAtSteadyMs = 0;
	bool requestInFlight = false;
};

struct MatrixTestPhase {
	std::string name;
	int64_t durationMs = 0;
	double sendCeilingBps = 0.0;
	double lossRate = 0.0;
	double rttMs = -1.0;
	double jitterMs = -1.0;
	qos::QualityLimitationReason qualityLimitationReason = qos::QualityLimitationReason::None;
};

struct MatrixTestProfile {
	int64_t warmupMs = 0;
	std::vector<MatrixTestPhase> phases;
};

struct MatrixTestRuntimeState {
	int64_t startMs = 0;
	int64_t lastSampleMs = 0;
	uint64_t lastPacketsSent = 0;
	uint64_t syntheticBytesSent = 0;
	uint64_t syntheticPacketsLost = 0;
	bool initialized = false;
};

struct TestClientStatsPayloadEntry {
	int delayMs = 0;
	json payload = json::object();
};

struct TestWsRequestEntry {
	int delayMs = 0;
	std::string method;
	json data = json::object();
};

int64_t steadyNowMs();
int64_t wallNowMs();

std::optional<ServerProducerStats> parseServerProducerStats(
	const json& peerStats, const std::string& producerId, const std::string& expectedKind);

std::optional<MatrixTestProfile> loadMatrixTestProfileFromEnv();
bool envFlagEnabled(const char* name);
size_t loadVideoTrackCountFromEnv();
std::vector<double> loadVideoTrackWeightsFromEnv(size_t trackCount);
std::vector<std::string> loadVideoSourcePathsFromEnv();
std::optional<double> applyMatrixTestProfile(
	qos::RawSenderSnapshot& snap,
	int encBitrate,
	const MatrixTestProfile& profile,
	MatrixTestRuntimeState& runtime,
	int64_t nowMs);
std::vector<TestClientStatsPayloadEntry> loadTestClientStatsPayloadsFromEnv();
std::vector<TestWsRequestEntry> loadTestWsRequestsFromEnv();
