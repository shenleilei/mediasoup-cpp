#include "SignalingServer.h"
#include "Constants.h"
#include "SignalingServerHttp.h"
#include "SignalingRequestDispatcher.h"
#include "SignalingSocketState.h"
#include "SignalingServerWs.h"
#include "StaticFileResponder.h"
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
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <utility>
#include <unistd.h>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {
namespace {

bool ValidateReadableRegularFile(const std::string& path, const char* label, std::string& error)
{
	if (path.empty()) {
		error = std::string(label) + " path is empty";
		MS_SPDLOG_ERROR("{} validation failed: path is empty", label);
		return false;
	}

	std::error_code ec;
	const std::filesystem::path fsPath(path);
	if (!std::filesystem::exists(fsPath, ec)) {
		error = std::string(label) + " not found: " + path;
		MS_SPDLOG_ERROR("{} validation failed: not found [path:{}]", label, path);
		return false;
	}
	if (ec) {
		error = std::string(label) + " existence check failed for " + path + ": " + ec.message();
		MS_SPDLOG_ERROR("{} validation failed: existence check failed [path:{} error:{}]", label, path, ec.message());
		return false;
	}
	if (!std::filesystem::is_regular_file(fsPath, ec)) {
		error = std::string(label) + " is not a regular file: " + path;
		MS_SPDLOG_ERROR("{} validation failed: not a regular file [path:{}]", label, path);
		return false;
	}
	if (ec) {
		error = std::string(label) + " type check failed for " + path + ": " + ec.message();
		MS_SPDLOG_ERROR("{} validation failed: type check failed [path:{} error:{}]", label, path, ec.message());
		return false;
	}
	if (access(path.c_str(), R_OK) != 0) {
		error = std::string(label) + " is not readable: " + path + " (" + std::strerror(errno) + ")";
		MS_SPDLOG_ERROR("{} validation failed: not readable [path:{} error:{}]", label, path, std::strerror(errno));
		return false;
	}

	return true;
}

} // namespace

bool ValidateSignalingTlsFiles(const SignalingTlsOptions& options, std::string& error)
{
	if (!ValidateReadableRegularFile(options.certFile, "signaling TLS certificate", error)) {
		return false;
	}
	if (!ValidateReadableRegularFile(options.keyFile, "signaling TLS private key", error)) {
		return false;
	}
	return true;
}

SignalingServer::SignalingServer(int port,
	std::vector<std::unique_ptr<WorkerThread>>& workerThreads,
	SignalingTlsOptions tlsOptions)
	: port_(port)
	, workerThreads_(workerThreads)
	, tlsOptions_(std::move(tlsOptions))
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

	running_.store(true, std::memory_order_relaxed);
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
	struct us_timer_t* shutdownTimer = nullptr;

	bool listenSucceeded = false;
	uWS::SocketContextOptions sslOptions;
	sslOptions.key_file_name = tlsOptions_.keyFile.c_str();
	sslOptions.cert_file_name = tlsOptions_.certFile.c_str();
	uWS::SSLApp app(sslOptions);
	SignalingServerWs::RegisterWebSocketRoutes(app, *this, wsMap, loop, downlinkStatsRateLimit);

	SignalingServerHttp::RegisterHttpRoutes(app, *this, loop);

		app.listen(port_, [this, &statsTimer, &shutdownTimer, &listenSucceeded, &notifyStartup](auto* listenSocket) {
			if (listenSocket) {
				listenSucceeded = true;
				notifyStartup(true);
				MS_SPDLOG_INFO(
					"SignalingServer listening with HTTPS/WSS on port {} cert={} key={}",
					port_, tlsOptions_.certFile, tlsOptions_.keyFile);
				auto* loop = uWS::Loop::get();
				SignalingServerHttp::StartBackgroundTimers(
					*this,
					loop,
					listenSocket,
					statsTimer,
					shutdownTimer);
			} else {
				notifyStartup(false);
				MS_SPDLOG_ERROR("SignalingServer failed to listen on port {}", port_);
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
	closeTimer(shutdownTimer);

	running_.store(false, std::memory_order_relaxed);
	if (!listenSucceeded) {
		stopRegistryWorker();
		return false;
	}
	return true;
}

} // namespace mediasoup
