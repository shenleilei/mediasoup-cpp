#pragma once
#include <nlohmann/json.hpp>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mediasoup {

using json = nlohmann::json;

inline constexpr const char* kSignalingTlsCertFile = "/opt/mediasoup-cpp/certs/tls.pem";
inline constexpr const char* kSignalingTlsKeyFile = "/opt/mediasoup-cpp/certs/tls.key";

class RoomRegistry;
class WorkerThread;
struct SignalingServerHttp;
struct SignalingServerWs;

struct SignalingTlsOptions {
	std::string certFile = kSignalingTlsCertFile;
	std::string keyFile = kSignalingTlsKeyFile;
};

bool ValidateSignalingTlsFiles(const SignalingTlsOptions& options, std::string& error);

class SignalingServer {
public:
	/// Construct with WorkerThread pool (new architecture).
	/// @param port           WebSocket listening port
	/// @param workerThreads  Pool of WorkerThreads (each owns Workers + Rooms)
	/// @param registry       Shared RoomRegistry (nullable)
	SignalingServer(int port,
		std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
		RoomRegistry* registry,
		bool redisRequired = true,
		SignalingTlsOptions tlsOptions = {});
	~SignalingServer();
	bool run(const std::function<void(bool)>& startupResult = {});
	void stop();
	void stopRegistryWorker();

private:
	friend struct SignalingServerHttp;
	friend struct SignalingServerWs;

	struct RuntimeSnapshot {
		size_t totalRooms = 0;
		size_t totalWorkers = 0;
		size_t totalMaxRooms = 0;
		size_t availableWorkerThreads = 0;
		size_t knownNodes = 0;
		size_t dispatchRooms = 0;
		uint64_t staleRequestDrops = 0;
		uint64_t rejectedClientStats = 0;
		bool registryEnabled = false;
		bool redisRequired = true;
		bool redisReady = false;
		bool startupSucceeded = false;
		bool shutdownRequested = false;
		json workerQueues = json::array();
		json roomOwnership = json::object();
	};

	RuntimeSnapshot collectRuntimeSnapshot() const;
	bool isHealthy(const RuntimeSnapshot& snapshot) const;
	bool isReady(const RuntimeSnapshot& snapshot) const;
	std::string buildPrometheusMetrics(const RuntimeSnapshot& snapshot) const;

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
	void enqueueRegistryTask(std::function<void()> task, std::string label = "registry.task");

	int port_;
	std::vector<std::unique_ptr<WorkerThread>>& workerThreads_;
	RoomRegistry* registry_;
	bool redisRequired_ = true;
	SignalingTlsOptions tlsOptions_;
	std::atomic<bool> running_{false};

	WorkerThread* findWorkerThreadById(int workerThreadId) const;

		// Room → WorkerThread dispatch table and destroyed-room markers are only
		// accessed from the main uWS thread, so no extra lock is used here.
		std::unordered_map<std::string, WorkerThread*> roomDispatch_;
		std::unordered_set<std::string> destroyedRooms_;

	std::mutex registryTaskMutex_;
	std::condition_variable registryTaskCv_;
	std::queue<std::function<void()>> registryTasks_;
	std::thread registryThread_;
	std::atomic<bool> stopRegistryThread_{false};
	std::atomic<bool> startupSucceeded_{false};
	std::atomic<uint64_t> nextRegistryTaskId_{1};

	std::atomic<uint64_t> staleRequestDrops_{0};
	std::atomic<uint64_t> rejectedClientStats_{0};
};

} // namespace mediasoup
