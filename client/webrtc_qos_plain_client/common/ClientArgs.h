#pragma once

#include <cstdint>
#include <string>

namespace webrtc_qos_plain {

struct PushOptions {
	std::string serverIp = "127.0.0.1";
	int serverPort = 3000;
	std::string room = "room1";
	std::string peer = "linux-pusher-1";
	std::string input;
	std::string logDir = "logs/webrtc_qos_plain_client/push";
	std::string mediaRemoteIp;
	uint32_t videoSsrc = 11111111u;
	uint32_t audioSsrc = 0;
	bool enableAudio = false;
	uint32_t startBitrateBps = 1200000u;
	uint32_t minBitrateBps = 300000u;
	uint32_t maxBitrateBps = 2500000u;
	int processTickMs = 10;
	bool loopInput = false;
	bool inputSynthetic = false;
	bool inputDecodeLoop = false;
	std::string encoder = "copy";
	int syntheticWidth = 320;
	int syntheticHeight = 180;
	int syntheticFps = 15;
	std::string syntheticPattern = "testsrc";
};

struct PlayOptions {
	std::string serverIp = "127.0.0.1";
	int serverPort = 3000;
	std::string room = "room1";
	std::string peer = "linux-player-1";
	std::string listenIp = "127.0.0.1";
	std::string advertiseIp;
	uint16_t listenPort = 50000;
	std::string outputAu;
	bool outputNull = false;
	std::string logDir = "logs/webrtc_qos_plain_client/play";
	std::string producerId;
	std::string producerPeerId;
	uint32_t receiverId = 0;
	uint32_t startBitrateBps = 1200000u;
	uint32_t minBitrateBps = 300000u;
	uint32_t maxBitrateBps = 2500000u;
	int processTickMs = 10;
	std::string mediaRemoteIp;
	int waitConsumerTimeoutMs = 30000;
	bool decodeQoe = false;
};

bool ParsePushOptions(int argc, char* argv[], PushOptions* options, std::string* error);
bool ParsePlayOptions(int argc, char* argv[], PlayOptions* options, std::string* error);

std::string PushUsage();
std::string PlayUsage();

} // namespace webrtc_qos_plain
