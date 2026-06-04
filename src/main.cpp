#include "Logger.h"
#include "HawkeyeRegisterClient.h"
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

std::string BuildRegisterServer(const RuntimeOptions& options)
{
	if (!options.hawkeyeRegisterServer.empty()) {
		return options.hawkeyeRegisterServer;
	}
	if (!options.announcedIp.empty()) {
		return options.announcedIp + ":" + std::to_string(options.signalingPort);
	}
	return {};
}

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
	MS_SPDLOG_INFO("mediasoup-cpp SFU starting (new architecture: WorkerThread pool)...");

	auto failExit = [&options]() {
		NotifyDaemonStartupStatus(false);
		if (!options.pidFile.empty()) std::remove(options.pidFile.c_str());
		return 1;
	};

	if (!FinalizeRuntimeOptions(options)) {
		return failExit();
	}

	SignalingTlsOptions tlsOptions;
	std::string tlsError;
	if (!ValidateSignalingTlsFiles(tlsOptions, tlsError)) {
		MS_SPDLOG_ERROR(
			"Signaling TLS validation failed: {} [cert:{} key:{}]",
			tlsError,
			tlsOptions.certFile,
			tlsOptions.keyFile);
		return failExit();
	}

	auto mediaCodecs = DefaultMediaCodecs();
	auto listenInfos = BuildListenInfos(options);
	auto runtimeServices = CreateRuntimeServices(options);
	if (!runtimeServices.startupError.empty()) {
		MS_SPDLOG_ERROR("Runtime services startup failed: {}", runtimeServices.startupError);
		return failExit();
	}
	auto workerThreads = CreateWorkerThreadPool(options, mediaCodecs, listenInfos);

	if (workerThreads.empty()) {
		MS_SPDLOG_ERROR("No WorkerThreads created, exiting");
		return failExit();
	}

	auto hawkeyeRegisterClient = std::make_unique<HawkeyeRegisterClient>(
		options.hawkeyeRegisterUrl,
		BuildRegisterServer(options),
		options.hawkeyeRegisterType);
	hawkeyeRegisterClient->start();

	// Assemble and run
	SignalingServer server(
		options.signalingPort,
		workerThreads,
		tlsOptions);

	MS_SPDLOG_INFO("mediasoup-cpp SFU ready - {} WorkerThreads, {} total workers, signaling on port {}, nodeId={}",
		workerThreads.size(), options.numWorkers, options.signalingPort, options.nodeId);

	bool runOk = server.run([](bool ok) {
		NotifyDaemonStartupStatus(ok);
	});

	// Graceful shutdown (reached when g_shutdown causes uWS loop to stop)
	MS_SPDLOG_INFO("Shutting down...");
	// Stop WorkerThreads first so room state and worker resources are released before exit.
	for (auto& wt : workerThreads) {
		wt->stop();
	}
	if (hawkeyeRegisterClient) {
		hawkeyeRegisterClient->stop();
	}
	// Local-only runtime keeps this as a no-op compatibility hook.
	server.stopRegistryWorker();
	MS_SPDLOG_INFO("Shutdown complete");
	if (!runOk) return failExit();
	NotifyDaemonStartupStatus(true);
	return 0;
}
