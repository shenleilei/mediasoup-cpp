#include "SignalingServer.h"

#include "WorkerThread.h"

#include <chrono>
#include <sstream>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {

SignalingServer::RuntimeSnapshot SignalingServer::collectRuntimeSnapshot() const {
	RuntimeSnapshot snapshot;
	snapshot.startupSucceeded = startupSucceeded_.load(std::memory_order_relaxed);
	snapshot.shutdownRequested = g_shutdown.load(std::memory_order_relaxed);
	snapshot.staleRequestDrops = staleRequestDrops_.load(std::memory_order_relaxed);
	snapshot.rejectedClientStats = rejectedClientStats_.load(std::memory_order_relaxed);
	snapshot.joinFailures = joinFailures_.load(std::memory_order_relaxed);
	snapshot.plainPublishFailures = plainPublishFailures_.load(std::memory_order_relaxed);
	snapshot.plainSubscribeFailures = plainSubscribeFailures_.load(std::memory_order_relaxed);
	{
		std::lock_guard<std::mutex> lock(requestRejectsMutex_);
		for (const auto& [method, counter] : requestRejectsByMethod_) {
			snapshot.requestRejectsByMethod[method] = counter.load(std::memory_order_relaxed);
		}
	}
	snapshot.workerDeaths = workerDeaths_.load(std::memory_order_relaxed);
	snapshot.workerRespawns = workerRespawns_.load(std::memory_order_relaxed);
	snapshot.wsDisconnects = wsDisconnects_.load(std::memory_order_relaxed);
	snapshot.malformedWsMessages = malformedWsMessages_.load(std::memory_order_relaxed);

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

bool SignalingServer::isReady(const RuntimeSnapshot& snapshot) const {
	return isHealthy(snapshot);
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
	auto writeLabeledCounter = [&out](const char* name, const char* help, const std::string& labels, double value) {
		out << "# HELP " << name << ' ' << help << "\n";
		out << "# TYPE " << name << " counter\n";
		out << name << '{' << labels << "} " << value << "\n";
	};
	auto writeLabeledGauge = [&out](const char* name, const char* help, const std::string& labels, double value) {
		out << "# HELP " << name << ' ' << help << "\n";
		out << "# TYPE " << name << " gauge\n";
		out << name << '{' << labels << "} " << value << "\n";
	};

	writeMetric("mediasoup_sfu_up", "Whether the signaling server finished startup successfully", snapshot.startupSucceeded ? 1 : 0);
	writeMetric("mediasoup_sfu_healthy", "Whether the signaling server currently has capacity to serve traffic", isHealthy(snapshot) ? 1 : 0);
	writeMetric("mediasoup_sfu_ready", "Whether the signaling server currently passes readiness checks", isReady(snapshot) ? 1 : 0);
	writeMetric("mediasoup_sfu_shutdown_requested", "Whether shutdown has been requested", snapshot.shutdownRequested ? 1 : 0);
	writeMetric("mediasoup_sfu_workers", "Total mediasoup worker processes currently attached", static_cast<double>(snapshot.totalWorkers));
	writeMetric("mediasoup_sfu_worker_threads", "Configured worker thread count", static_cast<double>(workerThreads_.size()));
	writeMetric("mediasoup_sfu_available_worker_threads", "Worker threads that still have at least one worker", static_cast<double>(snapshot.availableWorkerThreads));
	writeMetric("mediasoup_sfu_has_available_workers", "Whether at least one worker is available", snapshot.totalWorkers > 0 ? 1 : 0);
	writeMetric("mediasoup_sfu_rooms", "Current room count on this node", static_cast<double>(snapshot.totalRooms));
	writeMetric("mediasoup_sfu_max_rooms", "Current total room capacity on this node", static_cast<double>(snapshot.totalMaxRooms));
	writeMetric("mediasoup_sfu_dispatch_rooms", "Rooms tracked in the dispatch table", static_cast<double>(snapshot.dispatchRooms));
	writeCounter("mediasoup_sfu_stale_request_drops_total", "Dropped stale requests", static_cast<double>(snapshot.staleRequestDrops));
	writeCounter("mediasoup_sfu_rejected_client_stats_total", "Rejected clientStats requests", static_cast<double>(snapshot.rejectedClientStats));
	writeCounter("mediasoup_sfu_join_failures_total", "Failed join requests", static_cast<double>(snapshot.joinFailures));
	writeCounter("mediasoup_sfu_plain_publish_failures_total", "Failed plainPublish requests", static_cast<double>(snapshot.plainPublishFailures));
	writeCounter("mediasoup_sfu_plain_subscribe_failures_total", "Failed plainSubscribe requests", static_cast<double>(snapshot.plainSubscribeFailures));
	for (auto it = snapshot.requestRejectsByMethod.begin(); it != snapshot.requestRejectsByMethod.end(); ++it) {
		const std::string method = it.key();
		const double count = it.value().get<double>();
		writeLabeledCounter(
			"mediasoup_sfu_request_rejects_total",
			"Rejected signaling requests grouped by method",
			"method=\"" + method + "\"",
			count);
	}
	writeCounter("mediasoup_sfu_worker_deaths_total", "Observed worker death events", static_cast<double>(snapshot.workerDeaths));
	writeCounter("mediasoup_sfu_worker_respawns_total", "Observed worker respawn events", static_cast<double>(snapshot.workerRespawns));
	writeCounter("mediasoup_sfu_ws_disconnects_total", "Observed websocket disconnects", static_cast<double>(snapshot.wsDisconnects));
	writeCounter("mediasoup_sfu_malformed_ws_messages_total", "Dropped malformed websocket messages", static_cast<double>(snapshot.malformedWsMessages));
	for (const auto& queueEntry : snapshot.workerQueues) {
		const int threadId = queueEntry.value("threadId", -1);
		const double roomCount = static_cast<double>(queueEntry.value("roomCount", 0));
		const double workerCount = static_cast<double>(queueEntry.value("workerCount", 0));
		const double queueDepth = static_cast<double>(queueEntry.value("queueDepth", 0));
		const double queuePeakDepth = static_cast<double>(queueEntry.value("queuePeakDepth", 0));
		const double avgTaskWaitUs = queueEntry.value("avgTaskWaitUs", 0.0);
		const std::string labels = "thread_id=\"" + std::to_string(threadId) + "\"";
		writeLabeledGauge("mediasoup_sfu_worker_thread_room_count",
			"Current room count for a worker thread",
			labels,
			roomCount);
		writeLabeledGauge("mediasoup_sfu_worker_thread_worker_count",
			"Current worker process count for a worker thread",
			labels,
			workerCount);
		writeLabeledGauge("mediasoup_sfu_worker_thread_queue_depth",
			"Current posted task queue depth for a worker thread",
			labels,
			queueDepth);
		writeLabeledGauge("mediasoup_sfu_worker_thread_queue_peak_depth",
			"Observed queue peak depth for a worker thread",
			labels,
			queuePeakDepth);
		writeLabeledGauge("mediasoup_sfu_worker_thread_avg_task_wait_us",
			"Average queued task wait time in microseconds for a worker thread",
			labels,
			avgTaskWaitUs);
	}
	return out.str();
}

void SignalingServer::onWorkerDiedEvent()
{
	workerDeaths_.fetch_add(1, std::memory_order_relaxed);
}

void SignalingServer::onWorkerRespawnedEvent()
{
	workerRespawns_.fetch_add(1, std::memory_order_relaxed);
}

void SignalingServer::recordRequestReject(const std::string& method)
{
	const auto key = method.empty() ? "unknown" : method;
	std::lock_guard<std::mutex> lock(requestRejectsMutex_);
	requestRejectsByMethod_[key].fetch_add(1, std::memory_order_relaxed);
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

WorkerThread* SignalingServer::findWorkerThreadById(int workerThreadId) const {
	for (const auto& wt : workerThreads_) {
		if (wt && wt->id() == workerThreadId) {
			return wt.get();
		}
	}
	return nullptr;
}

void SignalingServer::assignRoom(const std::string& roomId, WorkerThread* wt) {
	roomDispatch_[roomId] = wt;
	destroyedRooms_.erase(roomId);
}

void SignalingServer::unassignRoom(const std::string& roomId) {
	roomDispatch_.erase(roomId);
}

void SignalingServer::startRegistryWorker() {
}

void SignalingServer::stopRegistryWorker() {
	stopRegistryThread_.store(true, std::memory_order_relaxed);
}

void SignalingServer::enqueueRegistryTask(std::function<void()> task, std::string label) {
	(void)task;
	(void)label;
}

void SignalingServer::stop() {
	running_.store(false, std::memory_order_relaxed);
	startupSucceeded_.store(false, std::memory_order_relaxed);
	stopRegistryWorker();
}

} // namespace mediasoup
