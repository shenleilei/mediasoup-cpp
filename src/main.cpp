#include "Logger.h"
#include "WorkerManager.h"
#include "RoomManager.h"
#include "RoomRegistry.h"
#include "RoomService.h"
#include "SignalingServer.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <thread>

using namespace mediasoup;
using json = nlohmann::json;

static WorkerManager* g_workerManager = nullptr;
static RoomRegistry* g_registry = nullptr;

void signalHandler(int sig) {
	spdlog::info("Received signal {}, shutting down...", sig);
	if (g_registry) g_registry->stop();
	if (g_workerManager) g_workerManager->close();
	exit(0);
}

int main(int argc, char* argv[]) {
	Logger::Init();
	spdlog::info("mediasoup-cpp SFU starting...");

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// Defaults
	int numWorkers = std::max(1u, std::thread::hardware_concurrency());
	int signalingPort = 3000;
	std::string listenIp = "0.0.0.0";
	std::string announcedIp;
	std::string workerBin;
	std::string redisHost = "127.0.0.1";
	int redisPort = 6379;
	std::string nodeId;
	std::string nodeAddress;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--port=") == 0)          signalingPort = std::stoi(arg.substr(7));
		else if (arg.find("--workers=") == 0)   numWorkers = std::stoi(arg.substr(10));
		else if (arg.find("--listenIp=") == 0)  listenIp = arg.substr(11);
		else if (arg.find("--announcedIp=") == 0) announcedIp = arg.substr(14);
		else if (arg.find("--workerBin=") == 0) workerBin = arg.substr(12);
		else if (arg.find("--redisHost=") == 0) redisHost = arg.substr(12);
		else if (arg.find("--redisPort=") == 0) redisPort = std::stoi(arg.substr(12));
		else if (arg.find("--nodeId=") == 0)    nodeId = arg.substr(9);
		else if (arg.find("--nodeAddress=") == 0) nodeAddress = arg.substr(14);
	}

	// Auto-generate nodeId if not set
	if (nodeId.empty()) {
		char hostname[256] = {};
		gethostname(hostname, sizeof(hostname));
		nodeId = std::string(hostname) + ":" + std::to_string(signalingPort);
	}
	// Auto-generate nodeAddress if not set
	if (nodeAddress.empty()) {
		std::string ip = announcedIp.empty() ? listenIp : announcedIp;
		if (ip == "0.0.0.0") ip = "127.0.0.1";
		nodeAddress = "ws://" + ip + ":" + std::to_string(signalingPort) + "/ws";
	}

	// Media codecs
	std::vector<json> mediaCodecs = {
		{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}},
		{{"mimeType", "video/VP8"}, {"clockRate", 90000}},
		{{"mimeType", "video/VP9"}, {"clockRate", 90000}, {"parameters", {{"profile-id", 0}}}},
		{{"mimeType", "video/H264"}, {"clockRate", 90000},
			{"parameters", {{"packetization-mode", 1}, {"profile-level-id", "4d0032"}, {"level-asymmetry-allowed", 1}}}}
	};

	// Listen infos
	json listenInfo = {{"ip", listenIp}, {"protocol", "udp"}};
	if (!announcedIp.empty()) listenInfo["announcedAddress"] = announcedIp;
	std::vector<json> listenInfos = {listenInfo};

	// Workers
	WorkerManager workerManager;
	g_workerManager = &workerManager;

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

	// Redis room registry
	std::unique_ptr<RoomRegistry> registry;
	try {
		registry = std::make_unique<RoomRegistry>(redisHost, redisPort, nodeId, nodeAddress);
		registry->start();
		g_registry = registry.get();
		spdlog::info("RoomRegistry started [nodeId:{} addr:{}]", nodeId, nodeAddress);
	} catch (const std::exception& e) {
		spdlog::warn("Redis not available ({}), running in single-node mode", e.what());
	}

	// Assemble
	RoomManager roomManager(workerManager, mediaCodecs, listenInfos);
	RoomService roomService(roomManager, registry.get());
	SignalingServer server(signalingPort, roomService);

	spdlog::info("mediasoup-cpp SFU ready - {} workers, signaling on port {}, nodeId={}",
		workerManager.size(), signalingPort, nodeId);

	server.run();
	return 0;
}
