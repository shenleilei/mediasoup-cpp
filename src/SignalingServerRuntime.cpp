#include "SignalingServer.h"

#include "RoomRegistry.h"
#include "WorkerThread.h"

#include <chrono>
#include <sstream>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {

SignalingServer::RuntimeSnapshot SignalingServer::collectRuntimeSnapshot() const {
	RuntimeSnapshot snapshot;
	snapshot.registryEnabled = (registry_ != nullptr);
	if (registry_) snapshot.knownNodes = registry_->knownNodeCount();
	snapshot.startupSucceeded = startupSucceeded_.load(std::memory_order_relaxed);
	snapshot.shutdownRequested = g_shutdown.load(std::memory_order_relaxed);
	snapshot.staleRequestDrops = staleRequestDrops_.load(std::memory_order_relaxed);
	snapshot.rejectedClientStats = rejectedClientStats_.load(std::memory_order_relaxed);

	for (auto& wt : workerThreads_) {
		auto workerCount = wt->workerCount();
		snapshot.totalRooms += wt->roomCount();
		snapshot.totalWorkers += workerCount;
		snapshot.totalMaxRooms += wt->maxRoomsCapacity();
		if (workerCount > 0) snapshot.availableWorkerThreads++;
		snapshot.workerQueues.push_back({
			{"threadId", wt->id()},
			{"roomCount", wt->roomCount()},
			{"workerCount", workerCount},
			{"queueDepth", wt->queueDepth()},
			{"queuePeakDepth", wt->queuePeakDepth()},
			{"avgTaskWaitUs", wt->avgTaskWaitUs()}
		});
	}

	snapshot.dispatchRooms = roomDispatch_.size();
	for (auto& [roomId, wt] : roomDispatch_) {
		snapshot.roomOwnership[roomId] = wt ? wt->id() : -1;
	}

	return snapshot;
}

bool SignalingServer::isHealthy(const RuntimeSnapshot& snapshot) const {
	return snapshot.startupSucceeded &&
		!snapshot.shutdownRequested &&
		snapshot.totalWorkers > 0 &&
		snapshot.availableWorkerThreads > 0;
}

std::string SignalingServer::buildPrometheusMetrics(const RuntimeSnapshot& snapshot) const {
	std::ostringstream out;
	auto writeMetric = [&out](const char* name, const char* help, double value) {
		out << "# HELP " << name << ' ' << help << "\n";
		out << "# TYPE " << name << " gauge\n";
		out << name << ' ' << value << "\n";
	};
	auto writeCounter = [&out](const char* name, const char* help, double value) {
		out << "# HELP " << name << ' ' << help << "\n";
		out << "# TYPE " << name << " counter\n";
		out << name << ' ' << value << "\n";
	};

	writeMetric("mediasoup_sfu_up", "Whether the signaling server finished startup successfully", snapshot.startupSucceeded ? 1 : 0);
	writeMetric("mediasoup_sfu_healthy", "Whether the signaling server currently has capacity to serve traffic", isHealthy(snapshot) ? 1 : 0);
	writeMetric("mediasoup_sfu_shutdown_requested", "Whether shutdown has been requested", snapshot.shutdownRequested ? 1 : 0);
	writeMetric("mediasoup_sfu_registry_enabled", "Whether Redis-backed registry is enabled", snapshot.registryEnabled ? 1 : 0);
	writeMetric("mediasoup_sfu_workers", "Total mediasoup worker processes currently attached", static_cast<double>(snapshot.totalWorkers));
	writeMetric("mediasoup_sfu_worker_threads", "Configured worker thread count", static_cast<double>(workerThreads_.size()));
	writeMetric("mediasoup_sfu_available_worker_threads", "Worker threads that still have at least one worker", static_cast<double>(snapshot.availableWorkerThreads));
	writeMetric("mediasoup_sfu_known_nodes", "Registry nodes currently visible in local cache", static_cast<double>(snapshot.knownNodes));
	writeMetric("mediasoup_sfu_has_available_workers", "Whether at least one worker is available", snapshot.totalWorkers > 0 ? 1 : 0);
	writeMetric("mediasoup_sfu_rooms", "Current room count on this node", static_cast<double>(snapshot.totalRooms));
	writeMetric("mediasoup_sfu_max_rooms", "Current total room capacity on this node", static_cast<double>(snapshot.totalMaxRooms));
	writeMetric("mediasoup_sfu_dispatch_rooms", "Rooms tracked in the dispatch table", static_cast<double>(snapshot.dispatchRooms));
	writeCounter("mediasoup_sfu_stale_request_drops_total", "Dropped stale requests", static_cast<double>(snapshot.staleRequestDrops));
	writeCounter("mediasoup_sfu_rejected_client_stats_total", "Rejected clientStats requests", static_cast<double>(snapshot.rejectedClientStats));
	return out.str();
}

WorkerThread* SignalingServer::pickLeastLoadedWorkerThread() const {
	WorkerThread* best = nullptr;
	size_t minLoad = SIZE_MAX;
	for (auto& wt : workerThreads_) {
		if (wt->workerCount() == 0) continue;
		size_t load = wt->roomCount();
		if (load < minLoad) {
			minLoad = load;
			best = wt.get();
		}
	}
	return best;
}

WorkerThread* SignalingServer::getWorkerThread(const std::string& roomId, bool assignIfMissing) {
	auto it = roomDispatch_.find(roomId);
	if (it != roomDispatch_.end()) return it->second;

	if (!assignIfMissing) return nullptr;
	WorkerThread* best = pickLeastLoadedWorkerThread();
	if (best) roomDispatch_[roomId] = best;
	return best;
}

void SignalingServer::assignRoom(const std::string& roomId, WorkerThread* wt) {
	roomDispatch_[roomId] = wt;
	destroyedRooms_.erase(roomId);
}

void SignalingServer::unassignRoom(const std::string& roomId) {
	roomDispatch_.erase(roomId);
}

void SignalingServer::startRegistryWorker() {
	if (!registry_ || registryThread_.joinable()) return;
	stopRegistryThread_.store(false, std::memory_order_relaxed);
	registryThread_ = std::thread([this] {
		for (;;) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(registryTaskMutex_);
				registryTaskCv_.wait(lock, [this] {
					return stopRegistryThread_.load(std::memory_order_relaxed) || !registryTasks_.empty();
				});
				if (stopRegistryThread_.load(std::memory_order_relaxed) && registryTasks_.empty())
					return;
				task = std::move(registryTasks_.front());
				registryTasks_.pop();
			}
			try {
				task();
			} catch (const std::exception& e) {
				spdlog::error("registry background task failed: {}", e.what());
			} catch (...) {
				spdlog::error("registry background task failed: unknown error");
			}
		}
	});
}

void SignalingServer::stopRegistryWorker() {
	stopRegistryThread_.store(true, std::memory_order_relaxed);
	registryTaskCv_.notify_all();
	if (registryThread_.joinable()) {
		registryThread_.join();
	}
	{
		std::lock_guard<std::mutex> lock(registryTaskMutex_);
		std::queue<std::function<void()>> empty;
		registryTasks_.swap(empty);
	}
}

void SignalingServer::enqueueRegistryTask(std::function<void()> task, std::string label) {
	if (!registry_) return;
	if (stopRegistryThread_.load(std::memory_order_relaxed)) {
		const auto taskId = nextRegistryTaskId_.fetch_add(1, std::memory_order_relaxed);
		spdlog::debug("registry task inline [id:{} label:{}]", taskId, label);
		try {
			task();
		} catch (const std::exception& e) {
			spdlog::error("registry inline task failed [id:{} label:{} error:{}]", taskId, label, e.what());
		} catch (...) {
			spdlog::error("registry inline task failed [id:{} label:{} error:unknown]", taskId, label);
		}
		return;
	}
	const auto taskId = nextRegistryTaskId_.fetch_add(1, std::memory_order_relaxed);
	const auto enqueuedAt = std::chrono::steady_clock::now();
	const auto queuedLabel = label;
	size_t queueDepth = 0;
	{
		std::lock_guard<std::mutex> lock(registryTaskMutex_);
		queueDepth = registryTasks_.size() + 1;
		registryTasks_.push([task = std::move(task), label = std::move(label), taskId, enqueuedAt]() mutable {
			const auto startedAt = std::chrono::steady_clock::now();
			const auto waitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				startedAt - enqueuedAt).count();
				spdlog::debug("registry task start [id:{} label:{} waitMs:{}]",
					taskId, label, waitMs);
			try {
				task();
				const auto finishedAt = std::chrono::steady_clock::now();
				const auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					finishedAt - startedAt).count();
				const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					finishedAt - enqueuedAt).count();
					spdlog::debug("registry task done [id:{} label:{} execMs:{} totalMs:{}]",
						taskId, label, execMs, totalMs);
			} catch (const std::exception& e) {
				const auto failedAt = std::chrono::steady_clock::now();
				const auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					failedAt - startedAt).count();
				spdlog::error("registry task failed [id:{} label:{} execMs:{} error:{}]",
					taskId, label, execMs, e.what());
				throw;
			} catch (...) {
				const auto failedAt = std::chrono::steady_clock::now();
				const auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					failedAt - startedAt).count();
				spdlog::error("registry task failed [id:{} label:{} execMs:{} error:unknown]",
					taskId, label, execMs);
				throw;
			}
		});
	}
		spdlog::debug("registry task queued [id:{} label:{} queueDepth:{}]", taskId, queuedLabel, queueDepth);
	registryTaskCv_.notify_one();
}

void SignalingServer::stop() {
	running_ = false;
	startupSucceeded_.store(false, std::memory_order_relaxed);
	stopRegistryWorker();
}

} // namespace mediasoup
