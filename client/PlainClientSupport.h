#pragma once

#include <nlohmann/json.hpp>
#include "qos/QosController.h"

#include <cmath>
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
	// CC convergence simulation state.
	// sendCeiling, RTT, jitter, and lossRate all use exponential convergence
	// to avoid instantaneous jumps on phase transitions (matching GCC behavior).
	// qualityLimitationReason is intentionally NOT converged because the real
	// WebRTC encoder quality limitation reason is a discrete flag derived from
	// the encoder's instantaneous state, not a smoothed network metric.
	double convergedCeilingBps = -1.0;
	double convergedRttMs = -1.0;
	double convergedJitterMs = -1.0;
	double convergedLossRate = 0.0;
	std::string lastPhaseName;
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

// Exponential convergence for CC simulation.
// GCC degrades in ~1-2s but recovers in ~5-7s (additive increase).
constexpr double CC_DEGRADE_TAU_MS = 1500.0;
constexpr double CC_RECOVER_TAU_MS = 6000.0;

inline double exponentialConverge(double current, double target, double deltaMs, double tauMs) {
	if (tauMs <= 0.0 || deltaMs <= 0.0) return target;
	double alpha = 1.0 - std::exp(-deltaMs / tauMs);
	return current + alpha * (target - current);
}

std::optional<ServerProducerStats> parseServerProducerStats(
	const json& peerStats, const std::string& producerId, const std::string& expectedKind);

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

#ifdef MEDIASOUP_TEST_HOOKS
std::optional<MatrixTestProfile> loadMatrixTestProfileFromEnv();
std::vector<TestClientStatsPayloadEntry> loadTestClientStatsPayloadsFromEnv();
std::vector<TestWsRequestEntry> loadTestWsRequestsFromEnv();
#endif
