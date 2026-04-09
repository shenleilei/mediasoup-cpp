#pragma once
#include "Worker.h"
#include "RoomManager.h"
#include "RoomService.h"
#include "RoomRegistry.h"
#include "Constants.h"
#include "Logger.h"
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <deque>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace mediasoup {

/// WorkerThread: an event-loop thread owning a set of Worker processes and Rooms.
///
/// Architecture:
///   Main Thread (uWS)  ──post()──▶  WorkerThread event loop
///   WorkerThread monitors Channel fds via epoll (no readThread per Channel).
///   All Room/Router/Transport operations happen within the WorkerThread (single-threaded, no locks needed).
///
class WorkerThread {
public:
	/// Construct a WorkerThread.
	/// @param id                Thread index (0, 1, 2, ...)
	/// @param workerSettings    Settings for spawning Worker processes
	/// @param numWorkers        Number of Worker processes this thread owns
	/// @param mediaCodecs       Codec list for Router creation
	/// @param listenInfos       Listen IP info for Transport creation
	/// @param registry          Shared RoomRegistry (nullable, thread-safe)
	/// @param recordDir         Recording directory
	/// @param maxRoutersPerWorker  Max routers per worker (0 = unlimited)
	WorkerThread(int id,
		const WorkerSettings& workerSettings,
		int numWorkers,
		const std::vector<nlohmann::json>& mediaCodecs,
		const std::vector<nlohmann::json>& listenInfos,
		RoomRegistry* registry,
		const std::string& recordDir,
		size_t maxRoutersPerWorker = 0)
		: id_(id)
		, workerSettings_(workerSettings)
		, mediaCodecs_(mediaCodecs)
		, listenInfos_(listenInfos)
		, registry_(registry)
		, recordDir_(recordDir)
		, maxRoutersPerWorker_(maxRoutersPerWorker)
		, numWorkersTarget_(numWorkers)
		, logger_(Logger::Get("WorkerThread"))
	{
		try {
			epollFd_ = ::epoll_create1(0);
			if (epollFd_ < 0)
				throw std::runtime_error("epoll_create1 failed: " + std::string(strerror(errno)));

			eventFd_ = ::eventfd(0, EFD_NONBLOCK);
			if (eventFd_ < 0)
				throw std::runtime_error("eventfd failed: " + std::string(strerror(errno)));

			// Register eventfd in epoll (for task queue wakeup)
			struct epoll_event ev{};
			ev.events = EPOLLIN;
			ev.data.fd = eventFd_;
			::epoll_ctl(epollFd_, EPOLL_CTL_ADD, eventFd_, &ev);

			// Create health check timer (timerfd)
			healthTimerFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
			if (healthTimerFd_ >= 0) {
				struct itimerspec its{};
				its.it_interval.tv_sec = kHealthCheckIntervalMs / 1000;
				its.it_interval.tv_nsec = (kHealthCheckIntervalMs % 1000) * 1000000L;
				its.it_value = its.it_interval;
				::timerfd_settime(healthTimerFd_, 0, &its, nullptr);

				struct epoll_event tev{};
				tev.events = EPOLLIN;
				tev.data.fd = healthTimerFd_;
				::epoll_ctl(epollFd_, EPOLL_CTL_ADD, healthTimerFd_, &tev);
			}

			// Create GC timer (idle room cleanup)
			gcTimerFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
			if (gcTimerFd_ >= 0) {
				struct itimerspec its{};
				its.it_interval.tv_sec = kGcIntervalMs / 1000;
				its.it_interval.tv_nsec = (kGcIntervalMs % 1000) * 1000000L;
				its.it_value = its.it_interval;
				::timerfd_settime(gcTimerFd_, 0, &its, nullptr);

				struct epoll_event tev{};
				tev.events = EPOLLIN;
				tev.data.fd = gcTimerFd_;
				::epoll_ctl(epollFd_, EPOLL_CTL_ADD, gcTimerFd_, &tev);
			}
		} catch (...) {
			if (healthTimerFd_ >= 0) { ::close(healthTimerFd_); healthTimerFd_ = -1; }
			if (gcTimerFd_ >= 0) { ::close(gcTimerFd_); gcTimerFd_ = -1; }
			if (eventFd_ >= 0) { ::close(eventFd_); eventFd_ = -1; }
			if (epollFd_ >= 0) { ::close(epollFd_); epollFd_ = -1; }
			throw;
		}
	}

	~WorkerThread() {
		stop();
		if (healthTimerFd_ >= 0) ::close(healthTimerFd_);
		if (gcTimerFd_ >= 0) ::close(gcTimerFd_);
		if (eventFd_ >= 0) ::close(eventFd_);
		if (epollFd_ >= 0) ::close(epollFd_);
	}

	/// Start the event loop thread. Creates Worker processes and begins processing.
	void start() {
		thread_ = std::thread([this] {
			try {
				createWorkers();
				ready_.store(true, std::memory_order_release);
				ready_cv_.notify_all();
				loop();
			} catch (const std::exception& e) {
				MS_ERROR(logger_, "WorkerThread {} fatal: {}", id_, e.what());
				ready_.store(true, std::memory_order_release);
				ready_cv_.notify_all();
			}
		});
	}

	/// Wait until createWorkers() has finished (max 5s).
	bool waitReady(int timeoutMs = 5000) {
		std::unique_lock<std::mutex> lock(ready_mu_);
		return ready_cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
			[this] { return ready_.load(std::memory_order_acquire); });
	}

	/// Stop the event loop and join the thread.
	void stop() {
		if (stopping_.exchange(true)) return;

		// Drain by posting a no-op to unblock epoll
		post([] {});

		if (thread_.joinable())
			thread_.join();

		// Close all workers
		for (auto& [fd, w] : fdToWorker_) {
			w->close();
		}
		fdToWorker_.clear();
		workers_.clear();

		// Close all rooms via RoomService
		if (roomService_) {
			try {
				roomService_->closeAllRooms();
			} catch (...) {}
		}
	}

	/// Thread-safe: post a task to this WorkerThread's queue.
	void post(std::function<void()> task) {
		{
			std::lock_guard<std::mutex> lock(queueMutex_);
			taskQueue_.push(QueuedTask{
				std::move(task),
				std::chrono::steady_clock::now()
			});
			auto depth = queueDepth_.fetch_add(1, std::memory_order_relaxed) + 1;
			size_t peak = queuePeakDepth_.load(std::memory_order_relaxed);
			while (depth > peak &&
				!queuePeakDepth_.compare_exchange_weak(
					peak, depth, std::memory_order_relaxed, std::memory_order_relaxed)) {}
		}
		uint64_t val = 1;
		::write(eventFd_, &val, sizeof(val));
	}

	/// Thread-safe: get the number of rooms managed by this thread.
	size_t roomCount() const {
		return roomCount_.load(std::memory_order_relaxed);
	}

	/// Thread-safe: get the number of workers.
	size_t workerCount() const {
		return workerCount_.load(std::memory_order_relaxed);
	}

	int id() const { return id_; }

	/// Access RoomService (only safe from within the WorkerThread context).
	RoomService* roomService() { return roomService_.get(); }

	/// Access RoomManager (only safe from within the WorkerThread context).
	RoomManager* roomManager() { return roomManager_.get(); }

	/// Access WorkerManager (only safe from within the WorkerThread context).
	WorkerManager* workerManager() { return workerManager_.get(); }

	/// Set notification callbacks (call before start()).
	void setNotify(RoomService::NotifyFn fn) { notifyFn_ = std::move(fn); }
	void setBroadcast(RoomService::BroadcastFn fn) { broadcastFn_ = std::move(fn); }
	void setRoomLifecycle(RoomService::RoomLifecycleFn fn) { roomLifecycleFn_ = std::move(fn); }
	void setRegistryTask(RoomService::RegistryTaskFn fn) { registryTaskFn_ = std::move(fn); }
	void setTaskPoster(RoomService::TaskPosterFn fn) { taskPosterFn_ = std::move(fn); }

	/// Called by RoomService to update room count (from within WorkerThread).
	void updateRoomCount() {
		if (roomManager_)
			roomCount_.store(roomManager_->roomCount(), std::memory_order_relaxed);
	}

	size_t queueDepth() const {
		return queueDepth_.load(std::memory_order_relaxed);
	}

	size_t queuePeakDepth() const {
		return queuePeakDepth_.load(std::memory_order_relaxed);
	}

	double avgTaskWaitUs() const {
		auto samples = taskWaitSamples_.load(std::memory_order_relaxed);
		if (samples == 0) return 0.0;
		auto total = taskWaitUsTotal_.load(std::memory_order_relaxed);
		return static_cast<double>(total) / static_cast<double>(samples);
	}

	size_t maxRoomsCapacity() const {
		// 0 means unlimited capacity when maxRoutersPerWorker is unset.
		if (maxRoutersPerWorker_ == 0) return 0;
		return workerCount() * maxRoutersPerWorker_;
	}

private:
	void createWorkers() {
		workerManager_ = std::make_unique<WorkerManager>();
		if (maxRoutersPerWorker_ > 0)
			workerManager_->setMaxRoutersPerWorker(maxRoutersPerWorker_);

		for (int i = 0; i < numWorkersTarget_; i++) {
			try {
				auto worker = std::make_shared<Worker>(workerSettings_, /*threaded=*/false);
				workers_.push_back(worker);

				// Register Channel consumer fd in epoll
				int fd = worker->channelConsumerFd();
				if (fd >= 0) {
					struct epoll_event ev{};
					ev.events = EPOLLIN;
					ev.data.fd = fd;
					::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
					fdToWorker_[fd] = worker;
				}

				// Add pre-created worker to WorkerManager for room creation.
				workerManager_->addExistingWorker(worker);

				MS_DEBUG(logger_, "WorkerThread {} created worker {} [pid:{}]", id_, i, worker->pid());
			} catch (const std::exception& e) {
				MS_ERROR(logger_, "WorkerThread {} failed to create worker {}: {}", id_, i, e.what());
			}
		}

		workerCount_.store(workers_.size(), std::memory_order_relaxed);

		if (workers_.empty()) {
			MS_ERROR(logger_, "WorkerThread {} has no workers!", id_);
			return;
		}

		// Create per-thread RoomManager and RoomService
		roomManager_ = std::make_unique<RoomManager>(*workerManager_, mediaCodecs_, listenInfos_);
		roomService_ = std::make_unique<RoomService>(*roomManager_, registry_, recordDir_);

		if (notifyFn_) roomService_->setNotify(notifyFn_);
		if (broadcastFn_) roomService_->setBroadcast(broadcastFn_);
		if (roomLifecycleFn_) roomService_->setRoomLifecycle(roomLifecycleFn_);
		if (registryTaskFn_) roomService_->setRegistryTask(registryTaskFn_);
		if (taskPosterFn_) roomService_->setTaskPoster(taskPosterFn_);
	}

	void loop() {
		MS_DEBUG(logger_, "WorkerThread {} entering event loop with {} workers",
			id_, workers_.size());

		static constexpr int MAX_EVENTS = 32;
		struct epoll_event events[MAX_EVENTS];

		while (!stopping_) {
			int nfds = ::epoll_wait(epollFd_, events, MAX_EVENTS, 1000 /*1s timeout*/);

			for (int i = 0; i < nfds; i++) {
				int fd = events[i].data.fd;

				if (fd == eventFd_) {
					// Task queue wakeup — drain eventfd and process tasks
					uint64_t val;
					(void)::read(eventFd_, &val, sizeof(val));
					processTaskQueue();
				} else if (fd == healthTimerFd_) {
					// Health check timer fired
					uint64_t expirations;
					(void)::read(healthTimerFd_, &expirations, sizeof(expirations));
					onHealthCheck();
				} else if (fd == gcTimerFd_) {
					// GC timer fired
					uint64_t expirations;
					(void)::read(gcTimerFd_, &expirations, sizeof(expirations));
					onGcTimer();
				} else {
					// Channel data from a Worker process
					auto it = fdToWorker_.find(fd);
					if (it != fdToWorker_.end()) {
						if (!it->second->processChannelData()) {
							// Worker died — handle it
							auto worker = it->second;
							::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr);
							fdToWorker_.erase(it);
							worker->handleWorkerDeath();
							onWorkerDied(worker);
						}
					}
				}
			}
		}

		MS_DEBUG(logger_, "WorkerThread {} exiting event loop", id_);
	}

	void processTaskQueue() {
		// Drain all tasks
		std::queue<QueuedTask> tasks;
		{
			std::lock_guard<std::mutex> lock(queueMutex_);
			tasks.swap(taskQueue_);
		}
		while (!tasks.empty()) {
			auto task = std::move(tasks.front());
			auto waitUs = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now() - task.enqueueAt).count();
			taskWaitUsTotal_.fetch_add(static_cast<uint64_t>(std::max<int64_t>(0, waitUs)),
				std::memory_order_relaxed);
			taskWaitSamples_.fetch_add(1, std::memory_order_relaxed);
			try {
				task.fn();
			} catch (const std::exception& e) {
				MS_ERROR(logger_, "WorkerThread {} task exception: {}", id_, e.what());
			} catch (...) {
				MS_ERROR(logger_, "WorkerThread {} task unknown exception", id_);
			}
			tasks.pop();
			queueDepth_.fetch_sub(1, std::memory_order_relaxed);
		}
		updateRoomCount();
	}

	void onHealthCheck() {
		if (!roomService_) return;
		try {
			roomService_->checkRoomHealth();
		} catch (const std::exception& e) {
			MS_ERROR(logger_, "WorkerThread {} checkRoomHealth exception: {}", id_, e.what());
		}
		updateRoomCount();
	}

	void onGcTimer() {
		if (!roomService_) return;
		try {
			roomService_->cleanIdleRooms(kIdleRoomTimeoutSec);
		} catch (const std::exception& e) {
			MS_ERROR(logger_, "WorkerThread {} cleanIdleRooms exception: {}", id_, e.what());
		}
		updateRoomCount();
	}

	void onWorkerDied(std::shared_ptr<Worker> worker) {
		MS_ERROR(logger_, "WorkerThread {} worker died [pid:{}], attempting respawn",
			id_, worker->pid());

		// Remove dead worker from WorkerManager before removing from local list
		workerManager_->removeWorker(worker);

		// Remove from workers list
		workers_.erase(
			std::remove(workers_.begin(), workers_.end(), worker),
			workers_.end());

		// Rate-limit respawns: max 3 within 10 seconds
		auto now = std::chrono::steady_clock::now();
		respawnTimes_.push_back(now);
		while (!respawnTimes_.empty() &&
			   (now - respawnTimes_.front()) > std::chrono::seconds(10))
			respawnTimes_.pop_front();
		if (respawnTimes_.size() > 3) {
			MS_ERROR(logger_, "WorkerThread {} respawn rate exceeded ({}x in 10s), giving up",
				id_, respawnTimes_.size());
			workerCount_.store(workers_.size(), std::memory_order_relaxed);
			return;
		}

		// Try to respawn
		try {
			auto newWorker = std::make_shared<Worker>(workerSettings_, /*threaded=*/false);
			workers_.push_back(newWorker);

			int fd = newWorker->channelConsumerFd();
			if (fd >= 0) {
				struct epoll_event ev{};
				ev.events = EPOLLIN;
				ev.data.fd = fd;
				::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
				fdToWorker_[fd] = newWorker;
			}

			workerManager_->addExistingWorker(newWorker);
			MS_WARN(logger_, "WorkerThread {} respawned worker [pid:{}]", id_, newWorker->pid());
		} catch (const std::exception& e) {
			MS_ERROR(logger_, "WorkerThread {} failed to respawn worker: {}", id_, e.what());
		}

		workerCount_.store(workers_.size(), std::memory_order_relaxed);
	}

	int id_;
	WorkerSettings workerSettings_;
	std::vector<nlohmann::json> mediaCodecs_;
	std::vector<nlohmann::json> listenInfos_;
	RoomRegistry* registry_;
	std::string recordDir_;
	size_t maxRoutersPerWorker_;
	int numWorkersTarget_;

	// Event loop resources
	int epollFd_ = -1;
	int eventFd_ = -1;
	int healthTimerFd_ = -1;
	int gcTimerFd_ = -1;
	std::thread thread_;
	std::atomic<bool> ready_{false};
	std::mutex ready_mu_;
	std::condition_variable ready_cv_;
	std::atomic<bool> stopping_{false};

	// Task queue (accessed from main thread + WorkerThread)
	struct QueuedTask {
		std::function<void()> fn;
		std::chrono::steady_clock::time_point enqueueAt;
	};
	std::mutex queueMutex_;
	std::queue<QueuedTask> taskQueue_;

	// Owned resources (only accessed from WorkerThread)
	std::vector<std::shared_ptr<Worker>> workers_;
	std::deque<std::chrono::steady_clock::time_point> respawnTimes_;
	std::unordered_map<int, std::shared_ptr<Worker>> fdToWorker_; // consumer fd → Worker
	std::unique_ptr<WorkerManager> workerManager_;
	std::unique_ptr<RoomManager> roomManager_;
	std::unique_ptr<RoomService> roomService_;

	// Thread-safe counters (atomic, read from main thread)
	std::atomic<size_t> roomCount_{0};
	std::atomic<size_t> workerCount_{0};
	std::atomic<size_t> queueDepth_{0};
	std::atomic<size_t> queuePeakDepth_{0};
	std::atomic<uint64_t> taskWaitUsTotal_{0};
	std::atomic<uint64_t> taskWaitSamples_{0};

	// Callbacks
	RoomService::NotifyFn notifyFn_;
	RoomService::BroadcastFn broadcastFn_;
	RoomService::RoomLifecycleFn roomLifecycleFn_;
	RoomService::RegistryTaskFn registryTaskFn_;
	RoomService::TaskPosterFn taskPosterFn_;

	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
