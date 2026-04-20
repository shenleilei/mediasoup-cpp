#include "WorkerThread.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <deque>
#include <stdexcept>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace mediasoup {

WorkerThread::WorkerThread(int id,
	const WorkerSettings& workerSettings,
	int numWorkers,
	const std::vector<nlohmann::json>& mediaCodecs,
	const std::vector<nlohmann::json>& listenInfos,
	RoomRegistry* registry,
	const std::string& recordDir,
	size_t maxRoutersPerWorker)
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
		auto addEpollFd = [this](int fd, uint32_t events) {
			struct epoll_event ev{};
			ev.events = events;
			ev.data.fd = fd;
			if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) != 0) {
				throw std::runtime_error("epoll_ctl ADD failed for fd " +
					std::to_string(fd) + ": " + std::string(strerror(errno)));
			}
		};

		epollFd_ = ::epoll_create1(0);
		if (epollFd_ < 0) {
			throw std::runtime_error("epoll_create1 failed: " + std::string(strerror(errno)));
		}

		eventFd_ = ::eventfd(0, EFD_NONBLOCK);
		if (eventFd_ < 0) {
			throw std::runtime_error("eventfd failed: " + std::string(strerror(errno)));
		}
		addEpollFd(eventFd_, EPOLLIN);

		delayedTimerFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (delayedTimerFd_ >= 0) {
			addEpollFd(delayedTimerFd_, EPOLLIN);
		}

		healthTimerFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (healthTimerFd_ >= 0) {
			struct itimerspec its{};
			its.it_interval.tv_sec = kHealthCheckIntervalMs / 1000;
			its.it_interval.tv_nsec = (kHealthCheckIntervalMs % 1000) * 1000000L;
			its.it_value = its.it_interval;
			if (::timerfd_settime(healthTimerFd_, 0, &its, nullptr) != 0) {
				throw std::runtime_error("timerfd_settime failed for health timer: " +
					std::string(strerror(errno)));
			}
			addEpollFd(healthTimerFd_, EPOLLIN);
		}

		gcTimerFd_ = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (gcTimerFd_ >= 0) {
			struct itimerspec its{};
			its.it_interval.tv_sec = kGcIntervalMs / 1000;
			its.it_interval.tv_nsec = (kGcIntervalMs % 1000) * 1000000L;
			its.it_value = its.it_interval;
			if (::timerfd_settime(gcTimerFd_, 0, &its, nullptr) != 0) {
				throw std::runtime_error("timerfd_settime failed for gc timer: " +
					std::string(strerror(errno)));
			}
			addEpollFd(gcTimerFd_, EPOLLIN);
		}
	} catch (...) {
		if (delayedTimerFd_ >= 0) { ::close(delayedTimerFd_); delayedTimerFd_ = -1; }
		if (healthTimerFd_ >= 0) { ::close(healthTimerFd_); healthTimerFd_ = -1; }
		if (gcTimerFd_ >= 0) { ::close(gcTimerFd_); gcTimerFd_ = -1; }
		if (eventFd_ >= 0) { ::close(eventFd_); eventFd_ = -1; }
		if (epollFd_ >= 0) { ::close(epollFd_); epollFd_ = -1; }
		throw;
	}
}

WorkerThread::~WorkerThread()
{
	stop();
	if (delayedTimerFd_ >= 0) ::close(delayedTimerFd_);
	if (healthTimerFd_ >= 0) ::close(healthTimerFd_);
	if (gcTimerFd_ >= 0) ::close(gcTimerFd_);
	if (eventFd_ >= 0) ::close(eventFd_);
	if (epollFd_ >= 0) ::close(epollFd_);
}

void WorkerThread::start()
{
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

bool WorkerThread::waitReady(int timeoutMs)
{
	std::unique_lock<std::mutex> lock(ready_mu_);
	return ready_cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
		[this] { return ready_.load(std::memory_order_acquire); });
}

void WorkerThread::stop()
{
	if (stopping_.exchange(true)) return;

	post([] {});

	if (thread_.joinable()) {
		thread_.join();
	}

	for (auto& [fd, worker] : fdToWorker_) {
		worker->close();
	}
	fdToWorker_.clear();
	workers_.clear();

	if (roomService_) {
		try {
			roomService_->closeAllRooms();
		} catch (...) {}
	}
}

void WorkerThread::post(std::function<void()> task)
{
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
	const ssize_t written = ::write(eventFd_, &val, sizeof(val));
	if (written != static_cast<ssize_t>(sizeof(val)) && !stopping_.load(std::memory_order_relaxed)) {
		MS_ERROR(logger_, "WorkerThread {} eventfd write failed: {}", id_, strerror(errno));
	}
}

void WorkerThread::postDelayed(std::function<void()> task, uint64_t delayMs)
{
	if (delayMs == 0 || delayedTimerFd_ < 0) {
		post(std::move(task));
		return;
	}

	{
		std::lock_guard<std::mutex> lock(delayedQueueMutex_);
		delayedTasks_.push(DelayedTask{
			std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMs),
			nextDelayedTaskId_++,
			std::move(task)
		});
		armDelayedTimerLocked();
	}
}

void WorkerThread::updateRoomCount()
{
	if (roomManager_) {
		roomCount_.store(roomManager_->roomCount(), std::memory_order_relaxed);
	}
}

double WorkerThread::avgTaskWaitUs() const
{
	auto samples = taskWaitSamples_.load(std::memory_order_relaxed);
	if (samples == 0) return 0.0;
	auto total = taskWaitUsTotal_.load(std::memory_order_relaxed);
	return static_cast<double>(total) / static_cast<double>(samples);
}

size_t WorkerThread::maxRoomsCapacity() const
{
	if (maxRoutersPerWorker_ == 0) return 0;
	return workerCount() * maxRoutersPerWorker_;
}

void WorkerThread::createWorkers()
{
	workerManager_ = std::make_unique<WorkerManager>();
	if (maxRoutersPerWorker_ > 0) {
		workerManager_->setMaxRoutersPerWorker(maxRoutersPerWorker_);
	}

	for (int i = 0; i < numWorkersTarget_; i++) {
		try {
			auto worker = std::make_shared<Worker>(workerSettings_, /*threaded=*/false);

			int fd = worker->channelConsumerFd();
			if (fd < 0) {
				MS_ERROR(logger_, "WorkerThread {} worker {} has invalid channel fd", id_, i);
				worker->close();
				continue;
			}

			struct epoll_event ev{};
			ev.events = EPOLLIN;
			ev.data.fd = fd;
			if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) != 0) {
				MS_ERROR(logger_,
					"WorkerThread {} failed to add worker fd {} to epoll: {}",
					id_,
					fd,
					strerror(errno));
				worker->close();
				continue;
			}

			workers_.push_back(worker);
			fdToWorker_[fd] = worker;
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

	roomManager_ = std::make_unique<RoomManager>(*workerManager_, mediaCodecs_, listenInfos_);
	roomService_ = std::make_unique<RoomService>(*roomManager_, registry_, recordDir_);

	if (notifyFn_) roomService_->setNotify(notifyFn_);
	if (broadcastFn_) roomService_->setBroadcast(broadcastFn_);
	if (roomLifecycleFn_) roomService_->setRoomLifecycle(roomLifecycleFn_);
	if (registryTaskFn_) roomService_->setRegistryTask(registryTaskFn_);
	if (taskPosterFn_) roomService_->setTaskPoster(taskPosterFn_);
	if (downlinkSnapshotAppliedFn_) {
		roomService_->setDownlinkSnapshotApplied(downlinkSnapshotAppliedFn_);
	}
	roomService_->setDelayedTaskPoster([this](std::function<void()> task, uint32_t delayMs) {
		postDelayed(std::move(task), delayMs);
	});
}

void WorkerThread::loop()
{
	MS_DEBUG(logger_, "WorkerThread {} entering event loop with {} workers", id_, workers_.size());

	static constexpr int MAX_EVENTS = 32;
	struct epoll_event events[MAX_EVENTS];

	while (!stopping_) {
		int nfds = ::epoll_wait(epollFd_, events, MAX_EVENTS, 1000);

		for (int i = 0; i < nfds; i++) {
			int fd = events[i].data.fd;

			if (fd == eventFd_) {
				uint64_t val;
				(void)::read(eventFd_, &val, sizeof(val));
				processTaskQueue();
			} else if (fd == healthTimerFd_) {
				uint64_t expirations;
				(void)::read(healthTimerFd_, &expirations, sizeof(expirations));
				onHealthCheck();
			} else if (fd == gcTimerFd_) {
				uint64_t expirations;
				(void)::read(gcTimerFd_, &expirations, sizeof(expirations));
				onGcTimer();
			} else if (fd == delayedTimerFd_) {
				uint64_t expirations;
				(void)::read(delayedTimerFd_, &expirations, sizeof(expirations));
				processDelayedTasks();
			} else {
				auto it = fdToWorker_.find(fd);
				if (it != fdToWorker_.end()) {
						if (!it->second->processChannelData()) {
							auto worker = it->second;
							if (::epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, nullptr) != 0) {
								MS_WARN(logger_,
									"WorkerThread {} failed to remove worker fd {} from epoll: {}",
									id_,
									fd,
									strerror(errno));
							}
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

void WorkerThread::processTaskQueue()
{
	std::queue<QueuedTask> tasks;
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		tasks.swap(taskQueue_);
	}

	while (!tasks.empty()) {
		auto task = std::move(tasks.front());
		auto waitUs = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now() - task.enqueueAt).count();
		taskWaitUsTotal_.fetch_add(
			static_cast<uint64_t>(std::max<int64_t>(0, waitUs)),
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

void WorkerThread::processDelayedTasks()
{
	std::vector<std::function<void()>> readyTasks;
	{
		std::lock_guard<std::mutex> lock(delayedQueueMutex_);
		const auto now = std::chrono::steady_clock::now();
		while (!delayedTasks_.empty() && delayedTasks_.top().dueAt <= now) {
			// std::priority_queue::top() returns const&, so move the element out
			// before pop to avoid copying the stored closure.
			auto task = std::move(const_cast<DelayedTask&>(delayedTasks_.top()));
			delayedTasks_.pop();
			readyTasks.push_back(std::move(task.fn));
		}
		armDelayedTimerLocked();
	}

	for (auto& task : readyTasks) {
		try {
			task();
		} catch (const std::exception& e) {
			MS_ERROR(logger_, "WorkerThread {} delayed task exception: {}", id_, e.what());
		} catch (...) {
			MS_ERROR(logger_, "WorkerThread {} delayed task unknown exception", id_);
		}
	}

	updateRoomCount();
}

void WorkerThread::armDelayedTimerLocked()
{
	if (delayedTimerFd_ < 0) return;

	struct itimerspec its{};
	if (!delayedTasks_.empty()) {
		const auto now = std::chrono::steady_clock::now();
		auto delay = delayedTasks_.top().dueAt - now;
		if (delay <= std::chrono::steady_clock::duration::zero()) {
			delay = std::chrono::nanoseconds(1);
		}
		const auto delayNs = std::chrono::duration_cast<std::chrono::nanoseconds>(delay);
		its.it_value.tv_sec = static_cast<time_t>(delayNs.count() / 1000000000LL);
		its.it_value.tv_nsec = static_cast<long>(delayNs.count() % 1000000000LL);
	}
	if (::timerfd_settime(delayedTimerFd_, 0, &its, nullptr) != 0) {
		MS_ERROR(logger_, "WorkerThread {} timerfd_settime failed: {}", id_, strerror(errno));
	}
}

void WorkerThread::onHealthCheck()
{
	if (!roomService_) return;
	try {
		roomService_->checkRoomHealth();
	} catch (const std::exception& e) {
		MS_ERROR(logger_, "WorkerThread {} checkRoomHealth exception: {}", id_, e.what());
	}
	updateRoomCount();
}

void WorkerThread::onGcTimer()
{
	if (!roomService_) return;
	try {
		roomService_->cleanIdleRooms(kIdleRoomTimeoutSec);
	} catch (const std::exception& e) {
		MS_ERROR(logger_, "WorkerThread {} cleanIdleRooms exception: {}", id_, e.what());
	}
	updateRoomCount();
}

void WorkerThread::onWorkerDied(std::shared_ptr<Worker> worker)
{
	MS_ERROR(logger_, "WorkerThread {} worker died [pid:{}], attempting respawn", id_, worker->pid());

	workerManager_->removeWorker(worker);
	workers_.erase(std::remove(workers_.begin(), workers_.end(), worker), workers_.end());

	auto now = std::chrono::steady_clock::now();
	respawnTimes_.push_back(now);
	while (!respawnTimes_.empty() &&
		(now - respawnTimes_.front()) > std::chrono::seconds(10)) {
		respawnTimes_.pop_front();
	}
	if (respawnTimes_.size() > 3) {
		MS_ERROR(logger_, "WorkerThread {} respawn rate exceeded ({}x in 10s), giving up",
			id_, respawnTimes_.size());
		workerCount_.store(workers_.size(), std::memory_order_relaxed);
		return;
	}

	try {
		auto newWorker = std::make_shared<Worker>(workerSettings_, /*threaded=*/false);

		int fd = newWorker->channelConsumerFd();
		if (fd < 0) {
			MS_ERROR(logger_, "WorkerThread {} respawned worker has invalid channel fd", id_);
			newWorker->close();
			workerCount_.store(workers_.size(), std::memory_order_relaxed);
			return;
		}

		struct epoll_event ev{};
		ev.events = EPOLLIN;
		ev.data.fd = fd;
		if (::epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev) != 0) {
			MS_ERROR(logger_,
				"WorkerThread {} failed to add respawned worker fd {} to epoll: {}",
				id_,
				fd,
				strerror(errno));
			newWorker->close();
			workerCount_.store(workers_.size(), std::memory_order_relaxed);
			return;
		}

		workers_.push_back(newWorker);
		fdToWorker_[fd] = newWorker;
		workerManager_->addExistingWorker(newWorker);
		MS_WARN(logger_, "WorkerThread {} respawned worker [pid:{}]", id_, newWorker->pid());
	} catch (const std::exception& e) {
		MS_ERROR(logger_, "WorkerThread {} failed to respawn worker: {}", id_, e.what());
	}

	workerCount_.store(workers_.size(), std::memory_order_relaxed);
}

} // namespace mediasoup
