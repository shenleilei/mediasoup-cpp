#include "Logger.h"
#include "WorkerManager.h"
#include "RoomManager.h"
#include "RoomRegistry.h"
#include "RoomService.h"
#include "SignalingServer.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <fstream>
#include <thread>
#include <array>
#include <cstdio>
#include <sys/stat.h>

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

static bool daemonize(const std::string& logFile, const std::string& pidFile) {
	pid_t pid = fork();
	if (pid < 0) return false;
	if (pid > 0) {
		// Parent: write PID file and exit
		if (!pidFile.empty()) {
			std::ofstream pf(pidFile);
			if (pf.is_open()) { pf << pid; pf.close(); }
		}
		_exit(0);
	}
	// Child: new session
	setsid();
	// Redirect stdout/stderr to log file
	if (!logFile.empty()) {
		FILE* f = fopen(logFile.c_str(), "a");
		if (f) {
			dup2(fileno(f), STDOUT_FILENO);
			dup2(fileno(f), STDERR_FILENO);
			fclose(f);
		}
	}
	// Close stdin
	close(STDIN_FILENO);
	return true;
}

int main(int argc, char* argv[]) {
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
	std::string recordDir = "./recordings";
	bool noDaemon = false;
	std::string logFile = "/var/log/mediasoup-sfu.log";
	std::string pidFile = "/var/run/mediasoup-sfu.pid";

	// Load config file (--config=path or default config.json)
	std::string configPath = "config.json";
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--config=") == 0) configPath = arg.substr(9);
	}
	{
		std::ifstream cfgFile(configPath);
		if (cfgFile.is_open()) {
			try {
				json cfg = json::parse(cfgFile);
				if (cfg.contains("port"))          signalingPort = cfg["port"].get<int>();
				if (cfg.contains("workers"))        { int w = cfg["workers"].get<int>(); if (w > 0) numWorkers = w; }
				if (cfg.contains("workerBin"))      workerBin = cfg["workerBin"].get<std::string>();
				if (cfg.contains("listenIp"))       listenIp = cfg["listenIp"].get<std::string>();
				if (cfg.contains("announcedIp"))    announcedIp = cfg["announcedIp"].get<std::string>();
				if (cfg.contains("redisHost"))      redisHost = cfg["redisHost"].get<std::string>();
				if (cfg.contains("redisPort"))      redisPort = cfg["redisPort"].get<int>();
				if (cfg.contains("nodeId"))         nodeId = cfg["nodeId"].get<std::string>();
				if (cfg.contains("nodeAddress"))    nodeAddress = cfg["nodeAddress"].get<std::string>();
				if (cfg.contains("recordDir"))      recordDir = cfg["recordDir"].get<std::string>();
				if (cfg.contains("logFile"))        logFile = cfg["logFile"].get<std::string>();
				if (cfg.contains("pidFile"))        pidFile = cfg["pidFile"].get<std::string>();
				if (cfg.contains("nodaemon"))        noDaemon = cfg["nodaemon"].get<bool>();
				spdlog::info("Loaded config from {}", configPath);
			} catch (const std::exception& e) {
				spdlog::warn("Failed to parse {}: {}", configPath, e.what());
			}
		}
	}

	// CLI args override config file
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
		else if (arg.find("--recordDir=") == 0) recordDir = arg.substr(12);
		else if (arg.find("--logFile=") == 0)  logFile = arg.substr(10);
		else if (arg.find("--pidFile=") == 0)  pidFile = arg.substr(10);
		else if (arg == "--nodaemon")           noDaemon = true;
	}

	// Daemonize unless --nodaemon
	if (!noDaemon) {
		daemonize(logFile, pidFile);
	}

	Logger::Init(noDaemon ? "" : logFile);
	spdlog::info("mediasoup-cpp SFU starting...");

	// Auto-detect public IP if announcedIp not set
	if (announcedIp.empty()) {
		auto detectPublicIp = []() -> std::string {
			const char* cmds[] = {
				"curl -s --max-time 3 http://ifconfig.me 2>/dev/null",
				"curl -s --max-time 3 http://api.ipify.org 2>/dev/null",
				"curl -s --max-time 3 http://icanhazip.com 2>/dev/null",
				nullptr
			};
			for (auto cmd = cmds; *cmd; ++cmd) {
				FILE* fp = popen(*cmd, "r");
				if (!fp) continue;
				std::array<char, 64> buf{};
				std::string result;
				while (fgets(buf.data(), buf.size(), fp)) result += buf.data();
				pclose(fp);
				// trim whitespace
				while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
					result.pop_back();
				// basic validation: non-empty, contains a dot, no spaces
				if (!result.empty() && result.find('.') != std::string::npos && result.find(' ') == std::string::npos)
					return result;
			}
			return {};
		};
		std::string detected = detectPublicIp();
		if (!detected.empty()) {
			announcedIp = detected;
			spdlog::info("Auto-detected public IP: {}", announcedIp);
		} else {
			spdlog::warn("Could not detect public IP, announcedIp is empty. WebRTC may fail for remote clients.");
		}
	}

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
		{{"mimeType", "video/H264"}, {"clockRate", 90000},
			{"parameters", {{"packetization-mode", 1}, {"profile-level-id", "4d0032"}, {"level-asymmetry-allowed", 1}}}},
		{{"mimeType", "video/VP8"}, {"clockRate", 90000}},
		{{"mimeType", "video/VP9"}, {"clockRate", 90000}, {"parameters", {{"profile-id", 0}}}}
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
	RoomService roomService(roomManager, registry.get(), recordDir);
	SignalingServer server(signalingPort, roomService, recordDir);

	spdlog::info("mediasoup-cpp SFU ready - {} workers, signaling on port {}, nodeId={}",
		workerManager.size(), signalingPort, nodeId);

	server.run();
	return 0;
}
