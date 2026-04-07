#pragma once
#include "RoomService.h"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

namespace mediasoup {

using json = nlohmann::json;

struct SecurityOptions {
	std::string wsAuthToken;
	std::string recordingsToken;
	size_t maxWsConnectionsPerIp = 0;
	size_t maxRequestsPerIpPerWindow = 0;
	size_t requestWindowSec = 60;
};

class SignalingServer {
public:
	SignalingServer(
		int port,
		RoomService& roomService,
		const std::string& recordDir = "",
		SecurityOptions security = {}
	);
	~SignalingServer();
	void run();
	void stop();

private:
	// Single worker thread that executes RoomService calls off the uWS thread
	void workerLoop();
	void postWork(std::function<void()> fn);

	int port_;
	RoomService& roomService_;
	std::string recordDir_;
	SecurityOptions security_;
	bool running_ = false;

	// Worker thread + task queue
	std::thread workerThread_;
	std::mutex queueMutex_;
	std::condition_variable queueCv_;
	std::queue<std::function<void()>> taskQueue_;
	bool workerStop_ = false;
	void* uwsLoop_ = nullptr;  // uWS::Loop*, captured for cross-thread defer()
};

} // namespace mediasoup
