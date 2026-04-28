#include "Logger.h"
#include "MainBootstrap.h"
#include "RuntimeDaemon.h"
#include "SignalingServer.h"
#include "WorkerThread.h"
#include <csignal>
#include <atomic>

using namespace mediasoup;

std::atomic<bool> g_shutdown{false};

void signalHandler(int sig) {
	g_shutdown = true;
}

namespace {

bool installSignalHandler(int signalNumber, void (*handler)(int))
{
	struct sigaction action {};
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	return ::sigaction(signalNumber, &action, nullptr) == 0;
}

} // namespace

int main(int argc, char* argv[]) {
	if (!installSignalHandler(SIGINT, signalHandler) ||
		!installSignalHandler(SIGTERM, signalHandler) ||
		!installSignalHandler(SIGPIPE, SIG_IGN)) {
		return 1;
	}

	auto options = LoadRuntimeOptions(argc, argv);

	// Daemonize unless --nodaemon
	if (!options.noDaemon) {
		if (DaemonizeProcess(options.logDir, options.logPrefix, options.logRotateHours, options.pidFile) < 0)
			return 1;
	}

	Logger::Init(options.noDaemon ? "" : options.logDir, options.logLevel, options.noDaemon, options.logRotateHours, options.logPrefix, getpid());
	spdlog::info("mediasoup-cpp SFU starting (new architecture: WorkerThread pool)...");

	auto failExit = [&options]() {
		NotifyDaemonStartupStatus(false);
		if (!options.pidFile.empty()) std::remove(options.pidFile.c_str());
		return 1;
	};

	if (!FinalizeRuntimeOptions(options)) {
		return failExit();
	}

	auto mediaCodecs = DefaultMediaCodecs();
	auto listenInfos = BuildListenInfos(options);
	auto runtimeServices = CreateRuntimeServices(options);
	if (!runtimeServices.startupError.empty()) {
		return failExit();
	}
	auto workerThreads = CreateWorkerThreadPool(options, mediaCodecs, listenInfos, runtimeServices.registry.get());

	if (workerThreads.empty()) {
		spdlog::error("No WorkerThreads created, exiting");
		return failExit();
	}

	// Assemble and run
	SignalingServer server(
		options.signalingPort,
		workerThreads,
		runtimeServices.registry.get(),
		options.recordDir,
		options.redisRequired);

	spdlog::info("mediasoup-cpp SFU ready - {} WorkerThreads, {} total workers, signaling on port {}, nodeId={}",
		workerThreads.size(), options.numWorkers, options.signalingPort, options.nodeId);

	bool runOk = server.run([](bool ok) {
		NotifyDaemonStartupStatus(ok);
	});

	// Graceful shutdown (reached when g_shutdown causes uWS loop to stop)
	spdlog::info("Shutting down...");
	// Stop WorkerThreads first — closeAllRooms() enqueues unregisterRoom tasks
	for (auto& wt : workerThreads) {
		wt->stop();
	}
	// Now stop registry worker — drains remaining unregister tasks
	server.stopRegistryWorker();
	if (runtimeServices.registry) runtimeServices.registry->stop();
	spdlog::info("Shutdown complete");
	if (!runOk) return failExit();
	NotifyDaemonStartupStatus(true);
	return 0;
}
