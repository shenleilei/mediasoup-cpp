#include "SignalingServer.h"
#include "Constants.h"
#include "SignalingServerHttp.h"
#include "SignalingRequestDispatcher.h"
#include "SignalingSocketState.h"
#include "SignalingServerWs.h"
#include "StaticFileResponder.h"
#include "RoomRegistry.h"
#include "WorkerThread.h"
#include "qos/QosValidator.h"
#include <App.h>
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

SignalingServer::SignalingServer(int port,
	std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
	RoomRegistry* registry,
	const std::string& recordDir)
	: port_(port), workerThreads_(workerThreads), registry_(registry), recordDir_(recordDir)
{}

SignalingServer::~SignalingServer() {
	stop();
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

	auto wsMap = std::make_shared<WsMap>();

	// Capture the uWS event loop for defer() calls from worker threads
	uWS::Loop* loop = uWS::Loop::get();

	SignalingServerWs::ConfigureWorkerCallbacks(*this, wsMap, loop, downlinkStatsRateLimit);
	if (!SignalingServerWs::EnsureWorkersReady(*this, notifyStartup)) {
		return false;
	}

	// Stats broadcast timer — dispatches to all WorkerThreads
	struct us_timer_t* statsTimer = nullptr;

	// Redis heartbeat timer — runs in main thread (RoomRegistry is thread-safe)
	struct us_timer_t* redisTimer = nullptr;
	struct us_timer_t* shutdownTimer = nullptr;

	bool listenSucceeded = false;
	uWS::App app;
	SignalingServerWs::RegisterWebSocketRoutes(app, *this, wsMap, loop, downlinkStatsRateLimit);

	SignalingServerHttp::RegisterHttpRoutes(app, *this, loop);

		app.listen(port_, [this, &statsTimer, &redisTimer, &shutdownTimer, &listenSucceeded, &notifyStartup](auto* listenSocket) {
			if (listenSocket) {
				listenSucceeded = true;
				notifyStartup(true);
				spdlog::info("SignalingServer listening on port {}", port_);
				auto* loop = uWS::Loop::get();
				SignalingServerHttp::StartBackgroundTimers(
					*this,
					loop,
					listenSocket,
					statsTimer,
					redisTimer,
					shutdownTimer);
			} else {
				notifyStartup(false);
				spdlog::error("SignalingServer failed to listen on port {}", port_);
			}
		});

	app.run();

	const auto closeTimer = [](us_timer_t*& timer) {
		if (timer) {
			us_timer_close(timer);
			timer = nullptr;
		}
	};
	closeTimer(statsTimer);
	closeTimer(redisTimer);
	closeTimer(shutdownTimer);

	running_ = false;
	if (!listenSucceeded) {
		stopRegistryWorker();
		return false;
	}
	return true;
}

} // namespace mediasoup
