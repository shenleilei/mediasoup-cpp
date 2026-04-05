#pragma once
#include "RoomService.h"
#include <nlohmann/json.hpp>
#include <string>

namespace mediasoup {

using json = nlohmann::json;

class SignalingServer {
public:
	SignalingServer(int port, RoomService& roomService, const std::string& recordDir = "");
	void run();
	void stop();

private:
	int port_;
	RoomService& roomService_;
	std::string recordDir_;
	bool running_ = false;
};

} // namespace mediasoup
