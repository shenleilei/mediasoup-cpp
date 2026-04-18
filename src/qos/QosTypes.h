#pragma once
#include "qos/QosContract.h"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace mediasoup::qos {

using json = nlohmann::json;

struct ClientQosTrackSnapshot {
	std::string localTrackId;
	std::string producerId;
	std::string kind;
	std::string source;
	std::string state;
	std::string reason;
	std::string quality;
	uint32_t ladderLevel{ 0u };
	json signals;
	json lastAction;
	json raw;
};

struct ClientQosSnapshot {
	std::string schema;
	uint64_t seq{ 0u };
	int64_t tsMs{ 0 };
	std::string peerMode;
	std::string peerQuality;
	bool peerStale{ false };
	std::vector<ClientQosTrackSnapshot> tracks;
	json raw;
};

struct QosPolicy {
	std::string schema;
	uint32_t sampleIntervalMs{ 0u };
	uint32_t snapshotIntervalMs{ 0u };
	bool allowAudioOnly{ false };
	bool allowVideoPause{ false };
	std::string cameraProfile;
	std::string screenShareProfile;
	std::string audioProfile;
	json raw;
};

struct QosOverride {
	std::string schema;
	std::string scope;
	std::string trackId;
	bool hasTrackId{ false };
	bool hasMaxLevelClamp{ false };
	uint32_t maxLevelClamp{ 0u };
	bool hasForceAudioOnly{ false };
	bool forceAudioOnly{ false };
	bool hasDisableRecovery{ false };
	bool disableRecovery{ false };
	bool hasPauseUpstream{ false };
	bool pauseUpstream{ false };
	bool hasResumeUpstream{ false };
	bool resumeUpstream{ false };
	uint32_t ttlMs{ 0u };
	std::string reason;
	json raw;
};

template<typename T>
struct ParseResult {
	bool ok{ false };
	T value{};
	std::string error;
};

struct PeerQosAggregate {
	bool hasSnapshot{ false };
	bool stale{ false };
	bool lost{ false };
	int64_t lastUpdatedMs{ 0 };
	int64_t receivedAtMs{ 0 };
	int64_t ageMs{ 0 };
	std::string quality{ "unknown" };
	json data;
};

struct RoomQosAggregate {
	bool hasQos{ false };
	size_t peerCount{ 0u };
	size_t stalePeers{ 0u };
	size_t poorPeers{ 0u };
	size_t lostPeers{ 0u };
	std::string quality{ "unknown" };
	json data;
};

constexpr uint64_t kQosMaxSeq = 9007199254740991ULL;
constexpr size_t kQosMaxTracksPerSnapshot = contract::kMaxTracksPerSnapshot;
constexpr int64_t kQosStaleAfterMs = 6000;
constexpr int64_t kQosLostAfterMs = 15000;

} // namespace mediasoup::qos
