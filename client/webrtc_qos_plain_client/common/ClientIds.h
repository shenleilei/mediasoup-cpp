#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "webrtc_qos/session_config.h"

namespace webrtc_qos_plain {

struct SingleVideoSessionParams {
	std::string roomId;
	std::string transportId;
	std::string sourceId;
	std::string receiverId;
	uint32_t receiverIdOverride = 0;
	uint32_t senderSsrc = 0;
	uint8_t payloadType = 0;
	uint8_t transportCcExtId = 0;
	uint32_t startBitrateBps = 1200000u;
	uint32_t minBitrateBps = 300000u;
	uint32_t maxBitrateBps = 2500000u;
	uint32_t trackId = 1;
	std::string debugName;
};

struct VideoTrackSessionParams {
	std::string trackIdString;
	uint32_t trackId = 0;
	uint32_t senderSsrc = 0;
	uint8_t payloadType = 0;
	uint8_t transportCcExtId = 0;
	uint32_t weight = 100;
	bool baseTrack = false;
};

struct VideoSessionParams {
	std::string roomId;
	std::string transportId;
	std::string sourceId;
	std::string receiverId;
	uint32_t receiverIdOverride = 0;
	uint32_t startBitrateBps = 1200000u;
	uint32_t minBitrateBps = 300000u;
	uint32_t maxBitrateBps = 2500000u;
	std::string debugName;
	std::vector<VideoTrackSessionParams> tracks;
};

uint32_t StableId(const std::string& value, uint32_t salt = 2166136261u);
webrtc_qos::SessionConfig MakeSingleVideoSessionConfig(const SingleVideoSessionParams& params);
webrtc_qos::SessionConfig MakeVideoSessionConfig(const VideoSessionParams& params);

} // namespace webrtc_qos_plain
