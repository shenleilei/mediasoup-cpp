#pragma once

#include <cstdint>
#include <string>

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

uint32_t StableId(const std::string& value, uint32_t salt = 2166136261u);
webrtc_qos::SessionConfig MakeSingleVideoSessionConfig(const SingleVideoSessionParams& params);

} // namespace webrtc_qos_plain
