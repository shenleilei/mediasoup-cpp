#include "Logger.h"
#include "WorkerManager.h"
#include "RoomManager.h"
#include "SignalingServer.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <thread>

using namespace mediasoup;
using json = nlohmann::json;

static WorkerManager* g_workerManager = nullptr;

void signalHandler(int sig) {
	spdlog::info("Received signal {}, shutting down...", sig);
	if (g_workerManager) g_workerManager->close();
	exit(0);
}

int main(int argc, char* argv[]) {
	Logger::Init();
	spdlog::info("mediasoup-cpp SFU starting...");

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Configuration
	int numWorkers = std::max(1u, std::thread::hardware_concurrency());
	int signalingPort = 3000;
	std::string listenIp = "0.0.0.0";
	std::string announcedIp; // set to public IP if behind NAT

	// Parse simple args
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--port=") == 0) signalingPort = std::stoi(arg.substr(7));
		else if (arg.find("--workers=") == 0) numWorkers = std::stoi(arg.substr(10));
		else if (arg.find("--listenIp=") == 0) listenIp = arg.substr(11);
		else if (arg.find("--announcedIp=") == 0) announcedIp = arg.substr(14);
		else if (arg.find("--workerBin=") == 0) ; // handled below
	}

	// Media codecs to support
	std::vector<json> mediaCodecs = {
		{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}},
		{{"mimeType", "video/VP8"}, {"clockRate", 90000}},
		{{"mimeType", "video/VP9"}, {"clockRate", 90000}, {"parameters", {{"profile-id", 0}}}},
		{{"mimeType", "video/H264"}, {"clockRate", 90000},
			{"parameters", {{"packetization-mode", 1}, {"profile-level-id", "4d0032"}, {"level-asymmetry-allowed", 1}}}}
	};

	// Listen infos for WebRTC transports
	json listenInfo = {{"ip", listenIp}, {"protocol", "udp"}};
	if (!announcedIp.empty()) listenInfo["announcedAddress"] = announcedIp;
	std::vector<json> listenInfos = {listenInfo};

	// Create workers
	WorkerManager workerManager;
	g_workerManager = &workerManager;

	std::string workerBin;
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--workerBin=") == 0) workerBin = arg.substr(12);
	}

	WorkerSettings workerSettings;
	workerSettings.logLevel = "warn";
	workerSettings.rtcMinPort = 10000;
	workerSettings.rtcMaxPort = 59999;
	if (!workerBin.empty()) workerSettings.workerBin = workerBin;

	spdlog::info("Creating {} workers...", numWorkers);
	for (int i = 0; i < numWorkers; i++) {
		try {
			workerManager.createWorker(workerSettings);
			spdlog::info("Worker {} created", i + 1);
		} catch (const std::exception& e) {
			spdlog::error("Failed to create worker {}: {}", i + 1, e.what());
		}
	}

	if (workerManager.size() == 0) {
		spdlog::error("No workers created, exiting");
		return 1;
	}

	// Create room manager and signaling server
	RoomManager roomManager(workerManager, mediaCodecs, listenInfos);
	SignalingServer server(signalingPort, roomManager);

	spdlog::info("mediasoup-cpp SFU ready - {} workers, signaling on port {}",
		workerManager.size(), signalingPort);

	server.run(); // blocks

	return 0;
}
