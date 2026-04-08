#pragma once
#include "RoomService.h"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <memory>

namespace mediasoup {

using json = nlohmann::json;

class SignalingServer {
public:
	SignalingServer(int port, RoomService& roomService, const std::string& recordDir = "",
		size_t signalingWorkers = 1);
	~SignalingServer();
	void run();
	void stop();

private:
	struct WorkerQueue {
		std::thread thread;
		std::mutex mutex;
		std::condition_variable cv;
		std::queue<std::function<void()>> tasks;
		bool stop = false;
	};

	// One or more worker threads execute RoomService calls off the uWS thread.
	// Tasks for the same room always go to the same queue to keep per-room serialization.
	void workerLoop(size_t queueIndex);
	void postWork(std::function<void()> fn, const std::string& roomId = "");
	size_t pickQueueIndex(const std::string& roomId) const;
	void startWorkerQueues(size_t queueCount);
	void stopWorkerQueues();

	int port_;
	RoomService& roomService_;
	std::string recordDir_;
	size_t signalingWorkers_ = 1;
	bool running_ = false;

	// Worker threads + sharded task queues
	std::vector<std::unique_ptr<WorkerQueue>> workerQueues_;
	void* uwsLoop_ = nullptr;  // uWS::Loop*, captured for cross-thread defer()
};

} // namespace mediasoup
