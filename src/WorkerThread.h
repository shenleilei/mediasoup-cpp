#pragma once

#include "Constants.h"
#include "Logger.h"
#include "RoomManager.h"
#include "RoomService.h"
#include "Worker.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace mediasoup {

class RoomRegistry;

/// WorkerThread: an event-loop thread owning a set of Worker processes and Rooms.
///
/// Architecture:
///   Main Thread (uWS)  ──post()──▶  WorkerThread event loop
///   WorkerThread monitors Channel fds via epoll (no readThread per Channel).
///   All Room/Router/Transport operations happen within the WorkerThread (single-threaded, no locks needed).
class WorkerThread {
private:
	struct DelayedTask {
		std::chrono::steady_clock::time_point dueAt;
		uint64_t id{ 0 };
		std::function<void()> fn;
	};

	struct DelayedTaskCompare {
		bool operator()(const DelayedTask& lhs, const DelayedTask& rhs) const {
			if (lhs.dueAt != rhs.dueAt) return lhs.dueAt > rhs.dueAt;
			return lhs.id > rhs.id;
		}
	};

	struct QueuedTask {
		std::function<void()> fn;
		std::chrono::steady_clock::time_point enqueueAt;
	};

public:
	WorkerThread(int id,
		const WorkerSettings& workerSettings,
		int numWorkers,
		const std::vector<nlohmann::json>& mediaCodecs,
		const std::vector<nlohmann::json>& listenInfos,
		RoomRegistry* registry,
		const std::string& recordDir,
		size_t maxRoutersPerWorker = 0);
	~WorkerThread();

	void start();
	bool waitReady(int timeoutMs = 5000);
	void stop();

	void post(std::function<void()> task);
	void postDelayed(std::function<void()> task, uint64_t delayMs);

	size_t roomCount() const {
		return roomCount_.load(std::memory_order_relaxed);
	}

	size_t workerCount() const {
		return workerCount_.load(std::memory_order_relaxed);
	}

	int id() const { return id_; }

	RoomService* roomService() { return roomService_.get(); }
	RoomManager* roomManager() { return roomManager_.get(); }
	WorkerManager* workerManager() { return workerManager_.get(); }

	void setNotify(RoomService::NotifyFn fn) { notifyFn_ = std::move(fn); }
	void setBroadcast(RoomService::BroadcastFn fn) { broadcastFn_ = std::move(fn); }
	void setRoomLifecycle(RoomService::RoomLifecycleFn fn) { roomLifecycleFn_ = std::move(fn); }
	void setRegistryTask(RoomService::RegistryTaskFn fn) { registryTaskFn_ = std::move(fn); }
	void setTaskPoster(RoomService::TaskPosterFn fn) { taskPosterFn_ = std::move(fn); }
	void setDownlinkSnapshotApplied(RoomService::DownlinkSnapshotAppliedFn fn) {
		downlinkSnapshotAppliedFn_ = std::move(fn);
	}

	void updateRoomCount();

	size_t queueDepth() const {
		return queueDepth_.load(std::memory_order_relaxed);
	}

	size_t queuePeakDepth() const {
		return queuePeakDepth_.load(std::memory_order_relaxed);
	}

	double avgTaskWaitUs() const;
	size_t maxRoomsCapacity() const;

private:
	void createWorkers();
	void loop();
	void processTaskQueue();
	void processDelayedTasks();
	void armDelayedTimerLocked();
	void onHealthCheck();
	void onGcTimer();
	void onWorkerDied(std::shared_ptr<Worker> worker);

	int id_;
	WorkerSettings workerSettings_;
	std::vector<nlohmann::json> mediaCodecs_;
	std::vector<nlohmann::json> listenInfos_;
	RoomRegistry* registry_;
	std::string recordDir_;
	size_t maxRoutersPerWorker_;
	int numWorkersTarget_;

	int epollFd_ = -1;
	int eventFd_ = -1;
	int delayedTimerFd_ = -1;
	int healthTimerFd_ = -1;
	int gcTimerFd_ = -1;
	std::thread thread_;
	std::atomic<bool> ready_{false};
	std::mutex ready_mu_;
	std::condition_variable ready_cv_;
	std::atomic<bool> stopping_{false};

	std::mutex queueMutex_;
	std::queue<QueuedTask> taskQueue_;
	std::mutex delayedQueueMutex_;
	std::priority_queue<DelayedTask, std::vector<DelayedTask>, DelayedTaskCompare> delayedTasks_;
	uint64_t nextDelayedTaskId_{ 1 };

	std::vector<std::shared_ptr<Worker>> workers_;
	std::deque<std::chrono::steady_clock::time_point> respawnTimes_;
	std::unordered_map<int, std::shared_ptr<Worker>> fdToWorker_;
	std::unique_ptr<WorkerManager> workerManager_;
	std::unique_ptr<RoomManager> roomManager_;
	std::unique_ptr<RoomService> roomService_;

	std::atomic<size_t> roomCount_{0};
	std::atomic<size_t> workerCount_{0};
	std::atomic<size_t> queueDepth_{0};
	std::atomic<size_t> queuePeakDepth_{0};
	std::atomic<uint64_t> taskWaitUsTotal_{0};
	std::atomic<uint64_t> taskWaitSamples_{0};

	RoomService::NotifyFn notifyFn_;
	RoomService::BroadcastFn broadcastFn_;
	RoomService::RoomLifecycleFn roomLifecycleFn_;
	RoomService::RegistryTaskFn registryTaskFn_;
	RoomService::TaskPosterFn taskPosterFn_;
	RoomService::DownlinkSnapshotAppliedFn downlinkSnapshotAppliedFn_;

	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
