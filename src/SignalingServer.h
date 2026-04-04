#pragma once
#include "RoomService.h"
#include <nlohmann/json.hpp>
#include <string>

namespace mediasoup {

using json = nlohmann::json;

class SignalingServer {
public:
	SignalingServer(int port, RoomService& roomService);
	void run();
	void stop();

private:
	int port_;
	RoomService& roomService_;
	bool running_ = false;
};

} // namespace mediasoup
