#include "SignalingServer.h"
#include "Constants.h"
#include "qos/QosValidator.h"
#include <App.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <thread>
#include <random>
#include <sstream>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {

static constexpr uint64_t kInvalidSessionId = 0;

struct PerSocketData {
	std::string peerId;
	std::string roomId;
	uint64_t sessionId = kInvalidSessionId;  // Monotonic session token to detect stale requests
	// Shared token: set to false when .close fires, checked in deferred callbacks
	std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
};

static std::atomic<uint64_t> g_nextSessionId{0};
static std::atomic<uint64_t> g_nextResolveRequestId{1};

struct DownlinkStatsRateLimitState {
	int64_t pendingSinceMs{0};
	uint64_t lastAcceptedSeq{0};
	uint64_t pendingSeq{0};
	bool hasAcceptedSeq{false};
	bool pending{false};
};

bool IsDownlinkSeqWrapOrReset(uint64_t prevSeq, uint64_t nextSeq)
{
	static constexpr uint64_t kSeqResetThreshold = 1000u;
	if (prevSeq >= qos::kDownlinkMaxSeq - kSeqResetThreshold &&
		nextSeq <= kSeqResetThreshold) {
		return true;
	}

	return prevSeq > nextSeq && (prevSeq - nextSeq) > kSeqResetThreshold;
}

bool IsAdvancingDownlinkSeq(const DownlinkStatsRateLimitState& state, uint64_t seq)
{
	if (!state.hasAcceptedSeq) return true;
	if (seq > state.lastAcceptedSeq) return true;
	if (seq < state.lastAcceptedSeq &&
		IsDownlinkSeqWrapOrReset(state.lastAcceptedSeq, seq)) {
		return true;
	}
	return false;
}

static uint64_t initSessionIdBase() {
	std::random_device rd;
	uint64_t base = (static_cast<uint64_t>(rd()) << 32) | rd();
	base = (base & 0x0000FFFFFFFFFFFF) | 0x0001000000000000; // ensure high bits set, never 0
	g_nextSessionId.store(base, std::memory_order_relaxed);
	return base;
}
static uint64_t g_sessionIdBase [[maybe_unused]] = initSessionIdBase();

namespace {
namespace fs = std::filesystem;

static constexpr size_t kInlineFileReadBytes = 128 * 1024;
static constexpr size_t kFileStreamChunkBytes = 64 * 1024;

struct ResolvedFile {
	fs::path path;
	uintmax_t size = 0;
};

bool isPathWithin(const fs::path& base, const fs::path& candidate) {
	auto baseIt = base.begin();
	auto candidateIt = candidate.begin();
	for (; baseIt != base.end() && candidateIt != candidate.end(); ++baseIt, ++candidateIt) {
		if (*baseIt != *candidateIt) return false;
	}
	return baseIt == base.end();
}

std::optional<ResolvedFile> resolveFileUnderBase(
	const fs::path& baseDir, std::string_view requestPath, bool& forbidden)
{
	forbidden = false;
	std::error_code ec;
	fs::path base = fs::weakly_canonical(baseDir, ec);
	if (ec) return std::nullopt;

	fs::path relative = fs::path(std::string(requestPath)).relative_path();
	fs::path normalized = (base / relative).lexically_normal();
	if (!isPathWithin(base, normalized)) {
		forbidden = true;
		return std::nullopt;
	}
	if (!fs::exists(normalized, ec) || ec) return std::nullopt;
	if (!fs::is_regular_file(normalized, ec) || ec) return std::nullopt;

	fs::path canonical = fs::weakly_canonical(normalized, ec);
	if (ec) return std::nullopt;
	if (!isPathWithin(base, canonical)) {
		forbidden = true;
		return std::nullopt;
	}

	auto size = fs::file_size(canonical, ec);
	if (ec) return std::nullopt;
	return ResolvedFile{canonical, size};
}

std::string contentTypeForPath(std::string_view path) {
	if (path.find(".webm") != std::string::npos) return "video/webm";
	if (path.find(".json") != std::string::npos) return "application/json";
	if (path.find(".html") != std::string::npos) return "text/html";
	if (path.find(".js") != std::string::npos) return "application/javascript";
	if (path.find(".css") != std::string::npos) return "text/css";
	return "application/octet-stream";
}

template <bool SSL>
struct FileStreamState {
	std::ifstream file;
	std::array<char, kFileStreamChunkBytes> buffer{};
	uintmax_t totalSize = 0;
	std::shared_ptr<std::atomic<bool>> aborted = std::make_shared<std::atomic<bool>>(false);
};

template <bool SSL>
void streamResolvedFile(uWS::HttpResponse<SSL>* res, std::shared_ptr<FileStreamState<SSL>> state) {
	while (!state->aborted->load(std::memory_order_relaxed)) {
		uintmax_t offset = res->getWriteOffset();
		if (offset >= state->totalSize) return;

		state->file.clear();
		state->file.seekg(static_cast<std::streamoff>(offset));
		if (!state->file.good()) {
			res->close();
			return;
		}

		auto remaining = state->totalSize - offset;
		auto toRead = std::min<uintmax_t>(state->buffer.size(), remaining);
		state->file.read(state->buffer.data(), static_cast<std::streamsize>(toRead));
		auto readBytes = state->file.gcount();
		if (readBytes <= 0) {
			res->close();
			return;
		}

		auto [ok, done] = res->tryEnd(
			std::string_view(state->buffer.data(), static_cast<size_t>(readBytes)),
			state->totalSize);
		if (done) return;
		if (!ok) {
			res->onWritable([res, state](int) {
				streamResolvedFile(res, state);
				return false;
			});
			return;
		}
	}
}

template <bool SSL>
void serveResolvedFile(uWS::HttpResponse<SSL>* res, const ResolvedFile& resolved, const std::string& contentType) {
	if (resolved.size <= kInlineFileReadBytes) {
		std::ifstream file(resolved.path, std::ios::binary);
		if (!file.is_open()) {
			res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		res->writeHeader("Content-Type", contentType);
		res->end(content);
		return;
	}

	auto state = std::make_shared<FileStreamState<SSL>>();
	state->totalSize = resolved.size;
	state->file.open(resolved.path, std::ios::binary);
	if (!state->file.is_open()) {
		res->writeStatus("404 Not Found")->end("Not Found");
		return;
	}

	res->writeHeader("Content-Type", contentType);
	auto aborted = state->aborted;
	res->onAborted([aborted] {
		aborted->store(true, std::memory_order_relaxed);
	});
	streamResolvedFile(res, state);
}
} // namespace

SignalingServer::SignalingServer(int port,
	std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
	RoomRegistry* registry,
	const std::string& recordDir)
	: port_(port), workerThreads_(workerThreads), registry_(registry), recordDir_(recordDir)
{}

SignalingServer::~SignalingServer() {
	stop();
}

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
	// Check dispatch table first
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
		// Registry worker already stopped; execute inline to avoid losing task
		const auto taskId = nextRegistryTaskId_.fetch_add(1, std::memory_order_relaxed);
		spdlog::warn("registry task inline [id:{} label:{}]", taskId, label);
		try { task(); } catch (...) {}
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
			spdlog::warn("registry task start [id:{} label:{} waitMs:{}]",
				taskId, label, waitMs);
			try {
				task();
				const auto finishedAt = std::chrono::steady_clock::now();
				const auto execMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					finishedAt - startedAt).count();
				const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
					finishedAt - enqueuedAt).count();
				spdlog::warn("registry task done [id:{} label:{} execMs:{} totalMs:{}]",
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
	spdlog::warn("registry task queued [id:{} label:{} queueDepth:{}]", taskId, queuedLabel, queueDepth);
	registryTaskCv_.notify_one();
}

bool SignalingServer::run(const std::function<void(bool)>& startupResult) {
	bool startupNotified = false;
	startupSucceeded_.store(false, std::memory_order_relaxed);
	auto notifyStartup = [&](bool ok) {
		if (!startupNotified) {
			startupNotified = true;
			startupSucceeded_.store(ok, std::memory_order_relaxed);
			if (startupResult) startupResult(ok);
		}
	};

	running_ = true;
	startRegistryWorker();
	auto downlinkStatsRateLimit = std::make_shared<
		std::unordered_map<std::string, DownlinkStatsRateLimitState>>();

	// peerId → ws* mapping for notify/broadcast
	struct WsMap {
		// Key: "roomId/peerId" — isolates peers across rooms
		std::unordered_map<std::string, uWS::WebSocket<false, true, PerSocketData>*> peers;
		// Secondary index: roomId -> sockets in room
		std::unordered_map<std::string, std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*>> roomPeers;
		static std::string key(const std::string& roomId, const std::string& peerId) {
			return roomId + "/" + peerId;
		}
	};
	auto wsMap = std::make_shared<WsMap>();

	// Capture the uWS event loop for defer() calls from worker threads
	uWS::Loop* loop = uWS::Loop::get();

	// Wire up RoomService callbacks on each WorkerThread
	for (auto& wt : workerThreads_) {
		WorkerThread* wtRaw = wt.get();
		wt->setNotify([wsMap, loop](const std::string& roomId, const std::string& peerId, const json& msg) {
			std::string data = msg.dump();
			std::string mapKey = WsMap::key(roomId, peerId);
			loop->defer([wsMap, mapKey, data = std::move(data)] {
				auto it = wsMap->peers.find(mapKey);
				if (it != wsMap->peers.end())
					it->second->send(data, uWS::OpCode::TEXT);
			});
		});

		wt->setBroadcast([wsMap, loop](const std::string& roomId,
			const std::string& excludePeerId, const json& msg)
		{
			std::string data = msg.dump();
			std::string excludeKey = excludePeerId.empty() ? "" : WsMap::key(roomId, excludePeerId);
			loop->defer([wsMap, roomId, excludeKey, data = std::move(data)] {
				uWS::WebSocket<false, true, PerSocketData>* excludeWs = nullptr;
				if (!excludeKey.empty()) {
					auto eit = wsMap->peers.find(excludeKey);
					if (eit != wsMap->peers.end())
						excludeWs = eit->second;
				}
				auto rit = wsMap->roomPeers.find(roomId);
				if (rit == wsMap->roomPeers.end()) return;
				for (auto* ws : rit->second) {
					if (ws == excludeWs) continue;
					ws->send(data, uWS::OpCode::TEXT);
				}
			});
		});

		wt->setRoomLifecycle([this, loop, wtRaw](const std::string& roomId, bool created) {
			loop->defer([this, roomId, wtRaw, created] {
				if (created) {
					destroyedRooms_.erase(roomId);
					return;
				}
				destroyedRooms_.insert(roomId);
				auto it = roomDispatch_.find(roomId);
				if (it != roomDispatch_.end() && it->second == wtRaw) {
					roomDispatch_.erase(it);
				}
			});
		});

		wt->setRegistryTask([this](std::function<void()> task) {
			enqueueRegistryTask(std::move(task), "worker.registryTask");
		});
		wt->setTaskPoster([wtRaw](std::function<void()> task) {
			wtRaw->post(std::move(task));
		});
		wt->setDownlinkSnapshotApplied([loop, downlinkStatsRateLimit](
			const std::string& roomId, const std::string& peerId, uint64_t seq) {
			const auto mapKey = WsMap::key(roomId, peerId);
			loop->defer([downlinkStatsRateLimit, mapKey, seq] {
				auto it = downlinkStatsRateLimit->find(mapKey);
				if (it == downlinkStatsRateLimit->end()) return;
				if (it->second.pending && it->second.pendingSeq == seq)
					it->second.pending = false;
			});
		});

		// Start the WorkerThread event loop
		wt->start();
	}

	// Wait for all WorkerThreads to be ready before accepting connections
	for (auto& wt : workerThreads_) {
		wt->waitReady();
	}

	// Verify at least one worker is available
	bool anyWorkers = false;
	for (auto& wt : workerThreads_) {
		if (wt->workerCount() > 0) { anyWorkers = true; break; }
	}
	if (!anyWorkers) {
		spdlog::error("No mediasoup workers available, refusing to listen");
		notifyStartup(false);
		running_ = false;
		stopRegistryWorker();
		return false;
	}

	// Stats broadcast timer — dispatches to all WorkerThreads
	struct us_timer_t* statsTimer = nullptr;

	// Redis heartbeat timer — runs in main thread (RoomRegistry is thread-safe)
	struct us_timer_t* redisTimer = nullptr;

	bool listenSucceeded = false;
	uWS::App().ws<PerSocketData>("/ws", {
		.compression = uWS::DISABLED,
		.maxPayloadLength = kWsMaxPayloadBytes,
		.idleTimeout = kWsIdleTimeoutSec,

		.open = [wsMap](auto* ws) {
			auto* d = ws->getUserData();
			d->peerId = "";
			d->roomId = "";
		},

		.message = [this, wsMap, loop, downlinkStatsRateLimit](auto* ws, std::string_view message, uWS::OpCode) {
			// Parse on the main thread (cheap) to extract parameters
			json req;
			try {
				req = json::parse(message);
			} catch (...) { return; }
			if (!req.contains("request") || !req["request"].get<bool>()) return;

			auto* sd = ws->getUserData();
			// Reject requests from invalidated sockets (replaced by reconnect)
			if (!sd->alive->load()) return;

			std::string method = req.value("method", "");
			uint64_t id = req.value("id", 0);
			json data = req.value("data", json::object());
			std::string roomId = sd->roomId;
			std::string peerId = sd->peerId;
			uint64_t sessionId = sd->sessionId;

			if (method != "clientStats") {
				spdlog::debug("[{} {}] {} id={}", roomId, peerId, method, id);
			}

			// clientStats is lightweight — validate on main thread and post to room worker
			if (method == "clientStats") {
				if (roomId.empty() || peerId.empty() || sessionId == kInvalidSessionId) {
					rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "clientStats requires joined session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				bool validSession = false;
				auto mapKey = WsMap::key(roomId, peerId);
				auto it = wsMap->peers.find(mapKey);
				validSession = (it != wsMap->peers.end() && it->second == ws &&
					it->second->getUserData()->sessionId == sessionId);
				if (!validSession) {
					staleRequestDrops_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "stale session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto* wt = getWorkerThread(roomId, false);
				if (!wt || !wt->roomService()) {
					rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "room not assigned"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto qosParse = qos::QosValidator::ParseClientSnapshot(data);
				if (!qosParse.ok) {
					rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "invalid clientStats: " + qosParse.error}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				wt->post([wt, roomId, peerId, data] {
					wt->roomService()->setClientStats(roomId, peerId, data);
				});
				json resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", json::object()}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
				return;
			}

			// downlinkClientStats — parse-then-respond, same as clientStats
			if (method == "downlinkClientStats") {
				static constexpr int64_t kMinDownlinkStatsIntervalMs = 100;
				if (roomId.empty() || peerId.empty() || sessionId == kInvalidSessionId) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "downlinkClientStats requires joined session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				{
					auto mapKey = WsMap::key(roomId, peerId);
					auto it = wsMap->peers.find(mapKey);
					bool validSession = (it != wsMap->peers.end() && it->second == ws &&
						it->second->getUserData()->sessionId == sessionId);
					if (!validSession) {
						staleRequestDrops_.fetch_add(1, std::memory_order_relaxed);
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "stale session"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
						return;
					}
				}
				auto dlParse = qos::QosValidator::ParseDownlinkSnapshot(data);
				if (!dlParse.ok) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "invalid downlinkClientStats: " + dlParse.error}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				{
					auto mapKey = WsMap::key(roomId, peerId);
					const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count();
					auto& state = (*downlinkStatsRateLimit)[mapKey];
					const auto seq = dlParse.value.seq;
					const bool advancingSeq = IsAdvancingDownlinkSeq(state, seq);
					if (advancingSeq &&
						state.pending &&
						nowMs - state.pendingSinceMs <= kMinDownlinkStatsIntervalMs) {
						rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "downlinkClientStats rate limited"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
						return;
					}
					if (advancingSeq) {
						state.pending = true;
						state.pendingSinceMs = nowMs;
						state.pendingSeq = seq;
						state.lastAcceptedSeq = seq;
						state.hasAcceptedSeq = true;
					}
				}
				auto* wt = getWorkerThread(roomId, false);
				if (!wt || !wt->roomService()) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "room not assigned"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				wt->post([wt, roomId, peerId, data] {
					wt->roomService()->setDownlinkClientStats(roomId, peerId, data);
				});
				json resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", json::object()}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
				return;
			}

			// For join, extract params now (before ws might change)
			std::string joinRoomId, joinPeerId, joinDisplayName, joinClientIp;
			json joinRtpCaps;
			if (method == "join") {
				joinRoomId = data.value("roomId", "default");
				joinPeerId = data.value("peerId", "");
				// Validate roomId/peerId: [A-Za-z0-9_-]{1,128}
				auto isValidId = [](const std::string& s) {
					if (s.empty() || s.size() > 128) return false;
					for (char c : s)
						if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
							  (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
					return true;
				};
				if (!isValidId(joinRoomId) || !isValidId(joinPeerId)) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "invalid roomId or peerId (allowed: [A-Za-z0-9_-]{1,128})"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				joinDisplayName = data.value("displayName", joinPeerId);
				joinRtpCaps = data.value("rtpCapabilities", json::object());
				joinClientIp = data.value("clientIp", "");
				if (joinClientIp.empty())
					joinClientIp = std::string(ws->getRemoteAddressAsText());
			}

			// Capture alive token to detect if ws was closed before defer runs
			auto alive = sd->alive;

			// For join, pre-assign sessionId so WorkerThread can set it on the peer
			uint64_t newSessionId = kInvalidSessionId;
			if (method == "join") newSessionId = g_nextSessionId++;

			// Determine target WorkerThread
			std::string targetRoomId = (method == "join") ? joinRoomId : roomId;
			WorkerThread* wt = nullptr;
			if (method == "join") {
				bool destroyed = (destroyedRooms_.find(targetRoomId) != destroyedRooms_.end());
				wt = destroyed ? nullptr : getWorkerThread(targetRoomId, false);
				if (!wt) {
					wt = pickLeastLoadedWorkerThread();
					// Atomically bind room to this WorkerThread before dispatching,
					// so concurrent joins for the same room go to the same thread.
					if (wt) assignRoom(targetRoomId, wt);
				}
			} else {
				wt = getWorkerThread(targetRoomId, false);
			}
			if (!wt) {
				json resp = {{"response", true}, {"id", id}, {"ok", false},
					{"error", method == "join" ? "no available worker thread" : "room not assigned"}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
				return;
			}

			// Dispatch to the WorkerThread
			wt->post([this, wt, wsMap, ws, alive, loop, method, id, data,
				roomId, peerId, joinRoomId, joinPeerId, joinDisplayName,
				joinRtpCaps, joinClientIp, targetRoomId, sessionId, newSessionId]
			{
				auto* rs = wt->roomService();
				if (!rs) {
					loop->defer([ws, alive, id] {
						if (!alive->load(std::memory_order_relaxed)) return;
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "worker not ready"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
					});
					return;
				}

				// For non-join methods, verify the peer's session hasn't been
				// replaced by a reconnect. This closes the race window where a
				// stale request was already queued before alive was set to false.
				if (method != "join" && sessionId != kInvalidSessionId) {
					auto room = rs->getRoom(roomId);
					if (room) {
						auto peer = room->getPeer(peerId);
						if (peer && peer->sessionId != 0 && peer->sessionId != sessionId) {
							spdlog::warn("[{} {}] dropping stale {} (session:{} current:{})",
								roomId, peerId, method, sessionId, peer->sessionId);
							loop->defer([ws, alive, id] {
								if (!alive->load()) return;
								json resp = {{"response", true}, {"id", id}, {"ok", false},
									{"error", "remote login"}};
								ws->send(resp.dump(), uWS::OpCode::TEXT);
							});
							return;
						}
					}
				}

				RoomService::Result result;
				try {
					if (method == "join") {
						result = rs->join(joinRoomId, joinPeerId, joinDisplayName, joinRtpCaps, joinClientIp);
						// Stamp the peer with this session's ID for stale-request detection
						if (result.ok && result.redirect.empty()) {
							auto room = rs->getRoom(joinRoomId);
							if (room) {
								auto peer = room->getPeer(joinPeerId);
								if (peer) peer->sessionId = newSessionId;
							}
						}
					} else if (method == "createWebRtcTransport") {
						result = rs->createTransport(roomId, peerId,
							data.value("producing", false), data.value("consuming", false));
					} else if (method == "connectWebRtcTransport") {
						result = rs->connectTransport(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("dtlsParameters").get<DtlsParameters>());
					} else if (method == "produce") {
						result = rs->produce(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("kind").get<std::string>(),
							data.at("rtpParameters"));
					} else if (method == "consume") {
						result = rs->consume(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("producerId").get<std::string>(),
							data.at("rtpCapabilities"));
					} else if (method == "pauseProducer") {
						result = rs->pauseProducer(roomId,
							data.at("producerId").get<std::string>());
					} else if (method == "resumeProducer") {
						result = rs->resumeProducer(roomId,
							data.at("producerId").get<std::string>());
					} else if (method == "restartIce") {
						result = rs->restartIce(roomId, peerId,
							data.at("transportId").get<std::string>());
					} else if (method == "getStats") {
						json stats = rs->collectPeerStats(roomId,
							data.value("peerId", peerId));
						result = {true, stats};
					} else if (method == "setQosOverride") {
						result = rs->setQosOverride(roomId,
							peerId,
							data.value("peerId", peerId),
							data.at("override"));
					} else if (method == "setQosPolicy") {
						result = rs->setQosPolicy(roomId,
							peerId,
							data.value("peerId", peerId),
							data.at("policy"));
					} else if (method == "pauseConsumer") {
						result = rs->pauseConsumer(roomId, peerId,
							data.at("consumerId").get<std::string>());
					} else if (method == "resumeConsumer") {
						result = rs->resumeConsumer(roomId, peerId,
							data.at("consumerId").get<std::string>());
					} else if (method == "getConsumerState") {
						result = rs->getConsumerState(roomId, peerId,
							data.at("consumerId").get<std::string>());
					} else if (method == "setConsumerPreferredLayers") {
						result = rs->setConsumerPreferredLayers(roomId, peerId,
							data.at("consumerId").get<std::string>(),
							data.at("spatialLayer").get<uint8_t>(),
							data.at("temporalLayer").get<uint8_t>());
					} else if (method == "setConsumerPriority") {
						result = rs->setConsumerPriority(roomId, peerId,
							data.at("consumerId").get<std::string>(),
							data.at("priority").get<uint8_t>());
					} else if (method == "requestConsumerKeyFrame") {
						result = rs->requestConsumerKeyFrame(roomId, peerId,
							data.at("consumerId").get<std::string>());
					} else if (method == "plainPublish") {
						result = rs->plainPublish(roomId, peerId,
							data.value("videoSsrc", 1111u),
							data.value("audioSsrc", 2222u));
					} else if (method == "plainSubscribe") {
						result = rs->plainSubscribe(roomId, peerId,
							data.at("recvIp").get<std::string>(),
							data.at("recvPort").get<uint16_t>());
					} else {
						result = {false, {}, "", "unknown method: " + method};
					}
				} catch (const std::exception& e) {
					spdlog::error("[{} {}] {} error: {}", targetRoomId, peerId, method, e.what());
					result = {false, {}, "", e.what()};
				}

				wt->updateRoomCount();

				// Build response
				json resp;
				if (!result.redirect.empty()) {
					resp = {{"response", true}, {"id", id}, {"ok", false},
						{"redirect", result.redirect}};
				} else if (result.ok) {
					resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", result.data}};
				} else {
					resp = {{"response", true}, {"id", id}, {"ok", false}, {"error", result.error}};
				}
				std::string respStr = resp.dump();

				bool joinOk = (method == "join" && result.ok && result.redirect.empty());
				bool joinFailed = (method == "join" && !joinOk);
				std::string jRoomId = joinRoomId, jPeerId = joinPeerId;
				json joinQosPolicy = json::object();
				if (joinOk && wt->roomService()) {
					joinQosPolicy = wt->roomService()->getDefaultQosPolicy();
				}

				// Defer ws->send() back to the uWS event loop thread
				loop->defer([this, wt, wsMap, ws, alive, respStr = std::move(respStr),
					joinOk, joinFailed, newSessionId,
					jRoomId = std::move(jRoomId), jPeerId = std::move(jPeerId),
					joinQosPolicy = std::move(joinQosPolicy)]
				{
					if (!alive->load()) {
						return;
					}
					if (joinFailed) {
						// Roll back early room assignment if join didn't succeed
						// Only unassign if no room was actually created on this thread
						if (!wt->roomService() || !wt->roomService()->hasRoom(jRoomId))
							unassignRoom(jRoomId);
					}
					if (joinOk) {
						auto* sd = ws->getUserData();
						sd->roomId = jRoomId;
						sd->peerId = jPeerId;
						std::string mapKey = WsMap::key(jRoomId, jPeerId);
						decltype(ws) oldWs = nullptr;
						sd->sessionId = newSessionId;
						auto it = wsMap->peers.find(mapKey);
						if (it != wsMap->peers.end() && it->second != ws)
							oldWs = it->second;
						wsMap->peers[mapKey] = ws;
						if (oldWs) {
							// Immediately invalidate old socket to prevent stale requests
							oldWs->getUserData()->alive->store(false);
							auto oldRoomIt = wsMap->roomPeers.find(jRoomId);
							if (oldRoomIt != wsMap->roomPeers.end()) {
								oldRoomIt->second.erase(oldWs);
								if (oldRoomIt->second.empty())
									wsMap->roomPeers.erase(oldRoomIt);
							}
						}
						wsMap->roomPeers[jRoomId].insert(ws);
						assignRoom(jRoomId, wt);
						spdlog::info("[{} {}] joined (session:{})", jRoomId, jPeerId, sd->sessionId);
						if (oldWs) {
							spdlog::info("[{} {}] kicking old connection (new session:{})", jRoomId, jPeerId, newSessionId);
							oldWs->end(4000, "replaced");
						}
					}
					ws->send(respStr, uWS::OpCode::TEXT);
					if (joinOk && !joinQosPolicy.empty()) {
						json msg = {
							{"notification", true},
							{"method", "qosPolicy"},
							{"data", joinQosPolicy}
						};
						ws->send(msg.dump(), uWS::OpCode::TEXT);
					}
				});
			});
		},

		.close = [this, wsMap, downlinkStatsRateLimit](auto* ws, int, std::string_view) {
			auto* sd = ws->getUserData();
			sd->alive->store(false);
			if (!sd->peerId.empty()) {
				spdlog::info("[{} {}] disconnected (session:{})", sd->roomId, sd->peerId, sd->sessionId);
				std::string roomId = sd->roomId;
				std::string peerId = sd->peerId;
				std::string mapKey = WsMap::key(roomId, peerId);
				bool erased = false;
				auto it = wsMap->peers.find(mapKey);
				if (it != wsMap->peers.end() && it->second == ws) {
					wsMap->peers.erase(it);
					downlinkStatsRateLimit->erase(mapKey);
					auto roomIt = wsMap->roomPeers.find(roomId);
					if (roomIt != wsMap->roomPeers.end()) {
						roomIt->second.erase(ws);
						if (roomIt->second.empty())
							wsMap->roomPeers.erase(roomIt);
					}
					erased = true;
				}
				if (erased && !roomId.empty()) {
					auto* wt = getWorkerThread(roomId, false);
					if (wt) {
						wt->post([wt, roomId, peerId] {
							try {
								if (wt->roomService())
									wt->roomService()->leave(roomId, peerId);
							} catch (const std::exception& e) {
								spdlog::error("[{} {}] leave failed: {}", roomId, peerId, e.what());
							} catch (...) {
								spdlog::error("[{} {}] leave failed: unknown error", roomId, peerId);
							}
							wt->updateRoomCount();
						});
					}
				}
			}
		}

	}).get("/api/resolve", [this, loop](auto* res, auto* req) {
		const auto requestId = g_nextResolveRequestId.fetch_add(1, std::memory_order_relaxed);
		std::string roomId(req->getQuery("roomId"));
		if (roomId.empty()) {
			res->writeStatus("400 Bad Request")->writeHeader("Content-Type", "application/json")
				->end(R"({"error":"roomId required"})");
			return;
		}
		std::string clientIp(req->getQuery("clientIp"));
		if (clientIp.empty()) {
			std::string xff(req->getHeader("x-forwarded-for"));
			if (!xff.empty()) {
				auto comma = xff.find(',');
				clientIp = (comma != std::string::npos) ? xff.substr(0, comma) : xff;
			}
		}
		if (clientIp.empty()) clientIp = std::string(res->getRemoteAddressAsText());
		const auto requestStart = std::chrono::steady_clock::now();
		spdlog::warn("api.resolve received [req:{} roomId:{} clientIp:{} registry:{}]",
			requestId, roomId, clientIp, registry_ != nullptr);

		if (!registry_) {
			res->writeHeader("Content-Type", "application/json")
				->writeHeader("Access-Control-Allow-Origin", "*")
				->end(R"({"wsUrl":"","isNew":true})");
			return;
		}

		auto aborted = std::make_shared<std::atomic<bool>>(false);
		res->onAborted([aborted] {
			aborted->store(true, std::memory_order_relaxed);
		});

		enqueueRegistryTask([this, loop, res, aborted, roomId, clientIp, requestId, requestStart] {
			RoomRegistry::ResolveResult result{"", true};
			const auto resolveStart = std::chrono::steady_clock::now();
			spdlog::warn("api.resolve executing [req:{} roomId:{} clientIp:{}]",
				requestId, roomId, clientIp);
			try {
				result = registry_->resolveRoom(roomId, clientIp);
			} catch (const std::exception& e) {
				spdlog::warn("api.resolve failed [req:{} roomId:{} error:{}]",
					requestId, roomId, e.what());
			} catch (...) {
				spdlog::warn("api.resolve failed [req:{} roomId:{} error:unknown]",
					requestId, roomId);
			}
			const auto resolveMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - resolveStart).count();
			spdlog::warn("api.resolve resolved [req:{} roomId:{} wsUrl:{} isNew:{} resolveMs:{}]",
				requestId, roomId, result.wsUrl, result.isNew, resolveMs);

			json j;
			if (!result.wsUrl.empty()) {
				j = {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
			} else {
				j = {{"wsUrl", ""}, {"isNew", true}};
			}
			std::string body = j.dump();

			loop->defer([res, aborted, body = std::move(body)] {
				if (aborted->load(std::memory_order_relaxed)) return;
				res->writeHeader("Content-Type", "application/json")
					->writeHeader("Access-Control-Allow-Origin", "*")
					->end(body);
			});
			const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - requestStart).count();
			spdlog::warn("api.resolve response scheduled [req:{} roomId:{} totalMs:{}]",
				requestId, roomId, totalMs);
		}, "api.resolve#" + std::to_string(requestId) + " room=" + roomId);

	}).get("/api/node-load", [this](auto* res, auto*) {
		auto snapshot = collectRuntimeSnapshot();
			json load = {
				{"rooms", snapshot.totalRooms},
				{"maxRooms", snapshot.totalMaxRooms},
				{"workers", snapshot.totalWorkers},
				{"workerThreads", workerThreads_.size()},
				{"availableWorkerThreads", snapshot.availableWorkerThreads},
				{"knownNodes", snapshot.knownNodes},
				{"registryEnabled", snapshot.registryEnabled},
				{"startupSucceeded", snapshot.startupSucceeded},
				{"shutdownRequested", snapshot.shutdownRequested},
			{"healthy", isHealthy(snapshot)},
			{"dispatchRooms", snapshot.dispatchRooms},
			{"staleRequestDrops", snapshot.staleRequestDrops},
			{"rejectedClientStats", snapshot.rejectedClientStats},
			{"workerQueueStats", snapshot.workerQueues},
			{"roomOwnership", snapshot.roomOwnership}
		};
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(load.dump());

	}).get("/healthz", [this](auto* res, auto*) {
		auto snapshot = collectRuntimeSnapshot();
		bool healthy = isHealthy(snapshot);
		json health = {
			{"ok", healthy},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"registryEnabled", snapshot.registryEnabled},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads},
			{"rooms", snapshot.totalRooms},
			{"maxRooms", snapshot.totalMaxRooms}
		};
		if (healthy) {
			res->writeHeader("Content-Type", "application/json")->end(health.dump());
		} else {
			res->writeStatus("503 Service Unavailable")
				->writeHeader("Content-Type", "application/json")
				->end(health.dump());
		}

	}).get("/metrics", [this](auto* res, auto*) {
		auto snapshot = collectRuntimeSnapshot();
		res->writeHeader("Content-Type", "text/plain; version=0.0.4")
			->end(buildPrometheusMetrics(snapshot));

	}).get("/api/recordings", [this](auto* res, auto*) {
		if (recordDir_.empty()) {
			res->writeHeader("Content-Type", "application/json")->end("[]");
			return;
		}
		json result = json::array();
		DIR* dir = opendir(recordDir_.c_str());
		if (!dir) { res->writeHeader("Content-Type", "application/json")->end("[]"); return; }
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.') continue;
			std::string roomPath = recordDir_ + "/" + ent->d_name;
			struct stat st;
			if (stat(roomPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
			std::string roomId(ent->d_name);
			DIR* rdir = opendir(roomPath.c_str());
			if (!rdir) continue;
			struct dirent* rent;
			while ((rent = readdir(rdir))) {
				std::string fname(rent->d_name);
				if (fname.size() < 6 || fname.substr(fname.size()-5) != ".webm") continue;
				std::string base = fname.substr(0, fname.size()-5);
				auto pos = base.rfind('_');
				std::string peerId = (pos != std::string::npos) ? base.substr(0, pos) : base;
				int64_t ts = 0;
				if (pos != std::string::npos) try { ts = std::stoll(base.substr(pos+1)); } catch(...) {}
				std::string fullPath = roomPath + "/" + fname;
				struct stat fst;
				int64_t fileSize = 0;
				if (stat(fullPath.c_str(), &fst) == 0) fileSize = fst.st_size;
				bool hasQos = (stat((roomPath + "/" + base + ".qos.json").c_str(), &fst) == 0);
				result.push_back({
					{"roomId", roomId}, {"peerId", peerId}, {"timestamp", ts},
					{"file", fname}, {"size", fileSize}, {"hasQos", hasQos}
				});
			}
			closedir(rdir);
		}
		closedir(dir);
		res->writeHeader("Content-Type", "application/json")->end(result.dump());

		}).get("/recordings/*", [this](auto* res, auto* req) {
			if (recordDir_.empty()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
			std::string url(req->getUrl());
			std::string relPath = url.substr(12);
			bool forbidden = false;
			auto resolved = resolveFileUnderBase(recordDir_, relPath, forbidden);
			if (!resolved) {
				if (forbidden) res->writeStatus("403 Forbidden")->end("Forbidden");
				else res->writeStatus("404 Not Found")->end("Not Found");
				return;
			}
			serveResolvedFile(res, *resolved, contentTypeForPath(relPath));

		}).get("/*", [](auto* res, auto* req) {
			std::string url(req->getUrl());
			if (url == "/") url = "/index.html";
			bool forbidden = false;
			auto resolved = resolveFileUnderBase("public", url, forbidden);
			if (!resolved) {
				if (forbidden) res->writeStatus("403 Forbidden")->end("Forbidden");
				else res->writeStatus("404 Not Found")->end("Not Found");
				return;
			}
			serveResolvedFile(res, *resolved, contentTypeForPath(url));

		}).listen(port_, [this, &statsTimer, &redisTimer, &listenSucceeded, &notifyStartup](auto* listenSocket) {
		if (listenSocket) {
			listenSucceeded = true;
			notifyStartup(true);
			spdlog::info("SignalingServer listening on port {}", port_);
			auto* loop = uWS::Loop::get();

			// Stats broadcast timer — dispatch to all WorkerThreads
			statsTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			SignalingServer* self = this;
			memcpy(us_timer_ext(statsTimer), &self, sizeof(SignalingServer*));
			us_timer_set(statsTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				for (auto& wt : s->workerThreads_) {
					wt->post([wtp = wt.get()] {
						try {
							if (wtp->roomService())
								wtp->roomService()->broadcastStats();
						} catch (const std::exception& e) {
							spdlog::error("broadcastStats exception: {}", e.what());
						}
					});
				}
			}, kStatsBroadcastIntervalMs, kStatsBroadcastIntervalMs);

			// Redis heartbeat timer
			redisTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			memcpy(us_timer_ext(redisTimer), &self, sizeof(SignalingServer*));
			us_timer_set(redisTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				// Aggregate in main thread, execute redis I/O in registry worker thread.
				if (s->registry_) {
					size_t totalRooms = 0;
					size_t maxRooms = 0;
					for (auto& wt : s->workerThreads_) {
						totalRooms += wt->roomCount();
						maxRooms += wt->maxRoomsCapacity();
					}
					std::vector<std::string> allRoomIds;
					allRoomIds.reserve(s->roomDispatch_.size());
					for (auto& [rid, assignedThread] : s->roomDispatch_) {
						(void)assignedThread;
						allRoomIds.push_back(rid);
					}
					s->enqueueRegistryTask([s, totalRooms, maxRooms, allRoomIds = std::move(allRoomIds)]() mutable {
						try {
							s->registry_->heartbeat();
							if (!allRoomIds.empty())
								s->registry_->refreshRooms(allRoomIds);
							s->registry_->updateLoad(totalRooms, maxRooms);
						} catch (const std::exception& e) {
							spdlog::error("registry heartbeat failed: {}", e.what());
						}
					}, "registry.heartbeat rooms=" + std::to_string(allRoomIds.size()));
				}
			}, kRedisHeartbeatIntervalSec * 1000, kRedisHeartbeatIntervalSec * 1000);

			// Shutdown poll timer
			struct ShutdownCtx { us_listen_socket_t* sock; };
			auto* shutdownTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(ShutdownCtx));
			ShutdownCtx sctx{listenSocket};
			memcpy(us_timer_ext(shutdownTimer), &sctx, sizeof(ShutdownCtx));
			us_timer_set(shutdownTimer, [](struct us_timer_t* t) {
				if (g_shutdown) {
					ShutdownCtx ctx;
					memcpy(&ctx, us_timer_ext(t), sizeof(ShutdownCtx));
					us_listen_socket_close(0, ctx.sock);
					us_timer_close(t);
				}
			}, kShutdownPollIntervalMs, kShutdownPollIntervalMs);
		} else {
			notifyStartup(false);
			spdlog::error("SignalingServer failed to listen on port {}", port_);
		}
	}).run();

	running_ = false;
	if (!listenSucceeded) {
		stopRegistryWorker();
		return false;
	}
	return true;
}

void SignalingServer::stop() {
	running_ = false;
	startupSucceeded_.store(false, std::memory_order_relaxed);
	stopRegistryWorker();
	// WorkerThreads are stopped by main.cpp
}

} // namespace mediasoup
