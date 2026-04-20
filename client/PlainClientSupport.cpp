#include "PlainClientSupport.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace {

std::optional<double> readFiniteNumber(const json& obj, const char* key)
{
	auto it = obj.find(key);
	if (it == obj.end() || !it->is_number()) return std::nullopt;
	double value = it->get<double>();
	if (!std::isfinite(value)) return std::nullopt;
	return value;
}

std::optional<uint64_t> readUint64Metric(const json& obj, const char* key)
{
	auto value = readFiniteNumber(obj, key);
	if (!value.has_value() || *value < 0) return std::nullopt;
	return static_cast<uint64_t>(*value);
}

double normalizeStatsTimeToMs(double value)
{
	return value;
}

double producerJitterToMs(const json& stat, double rawJitter)
{
	std::string kind = stat.value("kind", "");
	std::string mimeType = stat.value("mimeType", "");
	double clockRate = readFiniteNumber(stat, "clockRate").value_or(0.0);
	if (clockRate <= 0.0) {
		clockRate = 90000.0;
		if (kind == "audio" || mimeType.rfind("audio/", 0) == 0) clockRate = 48000.0;
	}
	if (clockRate <= 0.0) return -1.0;
	return rawJitter >= 0 ? (rawJitter * 1000.0 / clockRate) : -1.0;
}

qos::QualityLimitationReason parseQualityLimitationReason(const std::string& value)
{
	if (value == "bandwidth") return qos::QualityLimitationReason::Bandwidth;
	if (value == "cpu") return qos::QualityLimitationReason::Cpu;
	if (value == "other") return qos::QualityLimitationReason::Other;
	if (value == "none") return qos::QualityLimitationReason::None;
	return qos::QualityLimitationReason::Unknown;
}

const MatrixTestPhase* resolveMatrixTestPhase(
	const MatrixTestProfile& profile, int64_t startMs, int64_t nowMs)
{
	const int64_t elapsedMs = nowMs - startMs;
	if (elapsedMs < profile.warmupMs) return nullptr;

	int64_t phaseElapsedMs = elapsedMs - profile.warmupMs;
	for (const auto& phase : profile.phases) {
		if (phaseElapsedMs < phase.durationMs) return &phase;
		phaseElapsedMs -= phase.durationMs;
	}

	if (!profile.phases.empty()) return &profile.phases.back();
	return nullptr;
}

} // namespace

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

std::optional<ServerProducerStats> parseServerProducerStats(
	const json& peerStats, const std::string& producerId, const std::string& expectedKind)
{
	auto producersIt = peerStats.find("producers");
	if (producersIt == peerStats.end() || !producersIt->is_object()) return std::nullopt;

	auto parseStatArray = [&](const json& producerEntry) -> std::optional<ServerProducerStats> {
		auto statsIt = producerEntry.find("stats");
		if (statsIt == producerEntry.end() || !statsIt->is_array()) return std::nullopt;

		std::optional<ServerProducerStats> best;
		for (const auto& stat : *statsIt) {
			if (!stat.is_object()) continue;
			if (stat.value("type", "") != "inbound-rtp") continue;
			if (!expectedKind.empty() && stat.value("kind", "") != expectedKind) continue;

			ServerProducerStats parsed;
			parsed.packetCount = readUint64Metric(stat, "packetCount").value_or(0);
			parsed.byteCount = readUint64Metric(stat, "byteCount").value_or(0);
			parsed.packetsLost = readUint64Metric(stat, "packetsLost").value_or(0);
			parsed.bitrateBps = readFiniteNumber(stat, "bitrate").value_or(0.0);
			if (auto rtt = readFiniteNumber(stat, "roundTripTime")) {
				parsed.roundTripTimeMs = normalizeStatsTimeToMs(*rtt);
			}
			if (auto jitter = readFiniteNumber(stat, "jitter")) {
				parsed.jitterMs = producerJitterToMs(stat, *jitter);
			}

			if (!best.has_value()
				|| parsed.byteCount > best->byteCount
				|| parsed.packetCount > best->packetCount
				|| parsed.bitrateBps > best->bitrateBps) {
				best = parsed;
			}
		}
		return best;
	};

	if (!producerId.empty()) {
		auto producerIt = producersIt->find(producerId);
		if (producerIt != producersIt->end() && producerIt->is_object()) {
			if (auto parsed = parseStatArray(*producerIt)) return parsed;
		}
	}

	for (auto it = producersIt->begin(); it != producersIt->end(); ++it) {
		if (!it.value().is_object()) continue;
		if (auto parsed = parseStatArray(it.value())) return parsed;
	}

	return std::nullopt;
}

std::optional<MatrixTestProfile> loadMatrixTestProfileFromEnv()
{
	const char* raw = std::getenv("QOS_TEST_MATRIX_PROFILE");
	if (!raw || std::strlen(raw) == 0) return std::nullopt;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_object()) return std::nullopt;

		MatrixTestProfile profile;
		profile.warmupMs = static_cast<int64_t>(readFiniteNumber(payload, "warmupMs").value_or(0.0));
		auto phasesIt = payload.find("phases");
		if (phasesIt == payload.end() || !phasesIt->is_array()) return std::nullopt;

		for (const auto& phaseJson : *phasesIt) {
			if (!phaseJson.is_object()) continue;
			MatrixTestPhase phase;
			phase.name = phaseJson.value("name", "");
			phase.durationMs = static_cast<int64_t>(readFiniteNumber(phaseJson, "durationMs").value_or(0.0));
			phase.sendCeilingBps = readFiniteNumber(phaseJson, "sendCeilingBps").value_or(0.0);
			phase.lossRate = readFiniteNumber(phaseJson, "lossRate").value_or(0.0);
			phase.rttMs = readFiniteNumber(phaseJson, "rttMs").value_or(-1.0);
			phase.jitterMs = readFiniteNumber(phaseJson, "jitterMs").value_or(-1.0);
			phase.qualityLimitationReason =
				parseQualityLimitationReason(phaseJson.value("qualityLimitationReason", "unknown"));
			if (phase.durationMs > 0) profile.phases.push_back(std::move(phase));
		}

		if (profile.phases.empty()) return std::nullopt;
		return profile;
	} catch (...) {
		return std::nullopt;
	}
}

bool envFlagEnabled(const char* name)
{
	const char* raw = std::getenv(name);
	if (!raw) return false;
	std::string value(raw);
	return value == "1" || value == "true" || value == "yes" || value == "on";
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

std::optional<double> applyMatrixTestProfile(
	qos::RawSenderSnapshot& snap,
	int encBitrate,
	const MatrixTestProfile& profile,
	MatrixTestRuntimeState& runtime,
	int64_t nowMs)
{
	const MatrixTestPhase* phase = resolveMatrixTestPhase(profile, runtime.startMs, nowMs);
	if (!phase) return std::nullopt;

	if (!runtime.initialized) {
		runtime.initialized = true;
		runtime.lastSampleMs = nowMs;
		runtime.lastPacketsSent = snap.packetsSent;
		runtime.syntheticBytesSent = snap.bytesSent;
		runtime.syntheticPacketsLost = snap.packetsLost;
	}

	const int64_t deltaMs = std::max<int64_t>(0, nowMs - runtime.lastSampleMs);
	const uint64_t sentDelta = snap.packetsSent > runtime.lastPacketsSent
		? snap.packetsSent - runtime.lastPacketsSent
		: 0;
	const double sendCeilingBps = phase->sendCeilingBps > 0.0 ? phase->sendCeilingBps : static_cast<double>(encBitrate);
	const double mergedSendBps = std::min(static_cast<double>(encBitrate), sendCeilingBps);

	if (deltaMs > 0) {
		const uint64_t bytesDelta = static_cast<uint64_t>(std::llround(mergedSendBps * static_cast<double>(deltaMs) / 8000.0));
		runtime.syntheticBytesSent += bytesDelta;
	}

	if (sentDelta > 0 && phase->lossRate > 0.0 && phase->lossRate < 1.0) {
		const double lostDelta = (phase->lossRate / std::max(1e-9, 1.0 - phase->lossRate)) * static_cast<double>(sentDelta);
		runtime.syntheticPacketsLost += static_cast<uint64_t>(std::llround(lostDelta));
	}

	runtime.lastSampleMs = nowMs;
	runtime.lastPacketsSent = snap.packetsSent;

	snap.bytesSent = runtime.syntheticBytesSent;
	snap.packetsLost = runtime.syntheticPacketsLost;
	snap.roundTripTimeMs = phase->rttMs;
	snap.jitterMs = phase->jitterMs;
	snap.qualityLimitationReason = phase->qualityLimitationReason;

	return mergedSendBps;
}

std::vector<TestClientStatsPayloadEntry> loadTestClientStatsPayloadsFromEnv()
{
	std::vector<TestClientStatsPayloadEntry> entries;
	const char* raw = std::getenv("QOS_TEST_CLIENT_STATS_PAYLOADS");
	if (!raw || std::strlen(raw) == 0) return entries;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_array()) return entries;

		for (const auto& item : payload) {
			if (!item.is_object()) continue;
			TestClientStatsPayloadEntry entry;
			entry.delayMs = static_cast<int>(readFiniteNumber(item, "delayMs").value_or(0.0));
			if (auto payloadIt = item.find("payload"); payloadIt != item.end() && payloadIt->is_object()) {
				entry.payload = *payloadIt;
			} else {
				entry.payload = item;
				entry.payload.erase("delayMs");
			}
			if (!entry.payload.empty()) entries.push_back(std::move(entry));
		}
	} catch (...) {
	}

	return entries;
}

std::vector<TestWsRequestEntry> loadTestWsRequestsFromEnv()
{
	std::vector<TestWsRequestEntry> entries;
	const char* raw = std::getenv("QOS_TEST_SELF_REQUESTS");
	if (!raw || std::strlen(raw) == 0) return entries;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_array()) return entries;

		for (const auto& item : payload) {
			if (!item.is_object()) continue;
			TestWsRequestEntry entry;
			entry.delayMs = static_cast<int>(readFiniteNumber(item, "delayMs").value_or(0.0));
			entry.method = item.value("method", "");
			entry.data = item.value("data", json::object());
			if (!entry.method.empty()) entries.push_back(std::move(entry));
		}
	} catch (...) {
	}

	return entries;
}
