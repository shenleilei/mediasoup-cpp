#pragma once
#include "RoomService.h"
#include "WorkerThread.h"
#include <nlohmann/json.hpp>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <atomic>

namespace mediasoup {

using json = nlohmann::json;

class SignalingServer {
public:
	/// Construct with WorkerThread pool (new architecture).
	/// @param port           WebSocket listening port
	/// @param workerThreads  Pool of WorkerThreads (each owns Workers + Rooms)
	/// @param registry       Shared RoomRegistry (nullable)
	/// @param recordDir      Recording directory
	SignalingServer(int port,
		std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
		RoomRegistry* registry,
		const std::string& recordDir = "");
	~SignalingServer();
	void run();
	void stop();

private:
	// Pick the WorkerThread for a given roomId.
	// If the room is already assigned, returns that thread.
	// Otherwise, assigns to least-loaded thread.
	WorkerThread* getWorkerThread(const std::string& roomId, bool assignIfMissing);
	WorkerThread* pickLeastLoadedWorkerThread() const;

	// Assign a specific roomId to a WorkerThread (called from main thread).
	void assignRoom(const std::string& roomId, WorkerThread* wt);

	// Remove room assignment (called when room is cleaned up).
	void unassignRoom(const std::string& roomId);
	void startRegistryWorker();
	void stopRegistryWorker();
	void enqueueRegistryTask(std::function<void()> task);

	int port_;
	std::vector<std::unique_ptr<WorkerThread>>& workerThreads_;
	RoomRegistry* registry_;
	std::string recordDir_;
	bool running_ = false;

	// Room → WorkerThread dispatch table (only accessed from main uWS thread, no lock needed)
	std::unordered_map<std::string, WorkerThread*> roomDispatch_;
	std::unordered_set<std::string> destroyedRooms_;

	std::mutex registryTaskMutex_;
	std::condition_variable registryTaskCv_;
	std::queue<std::function<void()>> registryTasks_;
	std::thread registryThread_;
	std::atomic<bool> stopRegistryThread_{false};

	std::atomic<uint64_t> staleRequestDrops_{0};
	std::atomic<uint64_t> rejectedClientStats_{0};
};

} // namespace mediasoup
