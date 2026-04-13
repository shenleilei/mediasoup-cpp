#pragma once
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace mediasoup::qos {

using json = nlohmann::json;

struct DownlinkSubscription {
	std::string consumerId;
	std::string producerId;
	bool visible{ true };
	bool pinned{ false };
	bool activeSpeaker{ false };
	bool isScreenShare{ false };
	uint32_t targetWidth{ 0u };
	uint32_t targetHeight{ 0u };
	uint32_t packetsLost{ 0u };
	double jitter{ 0.0 };
	double framesPerSecond{ 0.0 };
	uint32_t frameWidth{ 0u };
	uint32_t frameHeight{ 0u };
	double freezeRate{ 0.0 };
};

struct DownlinkSnapshot {
	std::string schema;
	uint64_t seq{ 0u };
	int64_t tsMs{ 0 };
	std::string subscriberPeerId;
	double availableIncomingBitrate{ 0.0 };
	double currentRoundTripTime{ 0.0 };
	std::vector<DownlinkSubscription> subscriptions;
	json raw;
};

constexpr uint64_t kDownlinkMaxSeq = 9007199254740991ULL;
constexpr size_t kDownlinkMaxSubscriptions = 64u;

} // namespace mediasoup::qos
