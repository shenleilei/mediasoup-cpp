#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>

// Forward declare uWS types to avoid header issues in this header
namespace mediasoup {

using json = nlohmann::json;

class RoomManager;

class SignalingServer {
public:
	SignalingServer(int port, RoomManager& roomManager);
	void run();
	void stop();

private:
	int port_;
	RoomManager& roomManager_;
	bool running_ = false;
};

} // namespace mediasoup
