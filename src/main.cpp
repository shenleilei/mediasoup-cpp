#include "Logger.h"
#include "WorkerThread.h"
#include "RoomRegistry.h"
#include "SignalingServer.h"
#include "GeoRouter.h"
#include "GeoDbResolver.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <atomic>
#include <fstream>
#include <thread>
#include <array>
#include <cstdio>
#include <cmath>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace mediasoup;
using json = nlohmann::json;

static RoomRegistry* g_registry = nullptr;
static int g_daemonStartupFd = -1;

std::atomic<bool> g_shutdown{false};

void signalHandler(int sig) {
	g_shutdown = true;
}

static void notifyDaemonStartup(bool ok) {
	if (g_daemonStartupFd < 0) return;
	char status = ok ? '1' : '0';
	(void)::write(g_daemonStartupFd, &status, 1);
	::close(g_daemonStartupFd);
	g_daemonStartupFd = -1;
}

static void applyLegacyLogFileSetting(
	const std::string& value,
	std::string& logDir,
	std::string& logPrefix)
{
	if (value.empty()) return;
	std::filesystem::path path(value);
	if (path.has_filename()) {
		logDir = path.parent_path().empty() ? "." : path.parent_path().string();
		logPrefix = path.stem().string();
	} else {
		logDir = path.string();
	}
}

static std::string currentLogPath(
	const std::string& logDir,
	const std::string& logPrefix,
	int rotateHours,
	pid_t pid)
{
	return logging::build_bucketed_log_filename(
		logDir, logPrefix, static_cast<int>(pid), spdlog::details::os::now(), rotateHours);
}

static int daemonize(
	const std::string& logDir,
	const std::string& logPrefix,
	int logRotateHours,
	const std::string& pidFile) {
	int startupPipe[2];
	if (pipe(startupPipe) != 0) return -1;
	(void)fcntl(startupPipe[0], F_SETFD, FD_CLOEXEC);
	(void)fcntl(startupPipe[1], F_SETFD, FD_CLOEXEC);

	pid_t pid = fork();
	if (pid < 0) {
		::close(startupPipe[0]);
		::close(startupPipe[1]);
		return -1;
	}
	if (pid > 0) {
		::close(startupPipe[1]);
		char status = 0;
		ssize_t n = ::read(startupPipe[0], &status, 1);
		::close(startupPipe[0]);
		if (n == 1 && status == '1') {
			if (!pidFile.empty()) {
				std::ofstream pf(pidFile);
				if (pf.is_open()) { pf << pid; pf.close(); }
			}
			_exit(0);
		}
		(void)waitpid(pid, nullptr, 0);
		_exit(1);
	}
	::close(startupPipe[0]);
	// Child: new session
	setsid();
	// Redirect stdout/stderr to log file
	if (!logDir.empty()) {
		std::string logPath = currentLogPath(logDir, logPrefix, logRotateHours, getpid());
		FILE* f = fopen(logPath.c_str(), "a");
		if (f) {
			dup2(fileno(f), STDOUT_FILENO);
			dup2(fileno(f), STDERR_FILENO);
			fclose(f);
		}
	}
	// Close stdin
	close(STDIN_FILENO);
	return startupPipe[1];
}

int main(int argc, char* argv[]) {
	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);
	signal(SIGPIPE, SIG_IGN);

	// Defaults
	int numWorkers = static_cast<int>(std::thread::hardware_concurrency());
	numWorkers = std::max(1, numWorkers - 2);
	int numWorkerThreads = 0; // 0 = auto (will be calculated)
	int signalingPort = 3000;
	std::string listenIp = "0.0.0.0";
	std::string announcedIp;
	std::string workerBin;
	std::string redisHost = "127.0.0.1";
	int redisPort = 6379;
	std::string nodeId;
	std::string nodeAddress;
	std::string recordDir = "./recordings";
	int maxRoutersPerWorker = 0;
	double nodeLat = 0, nodeLng = 0;
	std::string nodeIsp;
	std::string nodeCountry;
	bool countryIsolation = true;
	std::string geoDbPath = "./ip2region.xdb";
	bool geoDbPathExplicit = false;
	bool noDaemon = false;
	std::string logDir = "/var/log/mediasoup";
	std::string logPrefix = "mediasoup-sfu";
	std::string pidFile = "/var/run/mediasoup-sfu.pid";
	std::string logLevel = "info";
	int logRotateHours = 3;

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
				if (cfg.contains("workerThreads"))  { int w = cfg["workerThreads"].get<int>(); if (w > 0) numWorkerThreads = w; }
				if (cfg.contains("workerBin"))      workerBin = cfg["workerBin"].get<std::string>();
				if (cfg.contains("listenIp"))       listenIp = cfg["listenIp"].get<std::string>();
				if (cfg.contains("announcedIp"))    announcedIp = cfg["announcedIp"].get<std::string>();
				if (cfg.contains("redisHost"))      redisHost = cfg["redisHost"].get<std::string>();
				if (cfg.contains("redisPort"))      redisPort = cfg["redisPort"].get<int>();
				if (cfg.contains("nodeId"))         nodeId = cfg["nodeId"].get<std::string>();
				if (cfg.contains("nodeAddress"))    nodeAddress = cfg["nodeAddress"].get<std::string>();
				if (cfg.contains("recordDir"))      recordDir = cfg["recordDir"].get<std::string>();
				if (cfg.contains("maxRoutersPerWorker")) maxRoutersPerWorker = cfg["maxRoutersPerWorker"].get<int>();
				if (cfg.contains("lat"))            nodeLat = cfg["lat"].get<double>();
				if (cfg.contains("lng"))            nodeLng = cfg["lng"].get<double>();
				if (cfg.contains("isp"))            nodeIsp = cfg["isp"].get<std::string>();
				if (cfg.contains("country"))        nodeCountry = cfg["country"].get<std::string>();
				if (cfg.contains("countryIsolation")) countryIsolation = cfg["countryIsolation"].get<bool>();
				if (cfg.contains("geoDb"))          { geoDbPath = cfg["geoDb"].get<std::string>(); geoDbPathExplicit = true; }
				if (cfg.contains("logDir"))         logDir = cfg["logDir"].get<std::string>();
				if (cfg.contains("logPrefix"))      logPrefix = cfg["logPrefix"].get<std::string>();
				if (cfg.contains("logFile"))        applyLegacyLogFileSetting(cfg["logFile"].get<std::string>(), logDir, logPrefix);
				if (cfg.contains("pidFile"))        pidFile = cfg["pidFile"].get<std::string>();
				if (cfg.contains("logLevel"))       logLevel = cfg["logLevel"].get<std::string>();
				if (cfg.contains("logRotateHours")) logRotateHours = cfg["logRotateHours"].get<int>();
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
		else if (arg.find("--workerThreads=") == 0) numWorkerThreads = std::stoi(arg.substr(16));
		else if (arg.find("--listenIp=") == 0)  listenIp = arg.substr(11);
		else if (arg.find("--announcedIp=") == 0) announcedIp = arg.substr(14);
		else if (arg.find("--workerBin=") == 0) workerBin = arg.substr(12);
		else if (arg.find("--redisHost=") == 0) redisHost = arg.substr(12);
		else if (arg.find("--redisPort=") == 0) redisPort = std::stoi(arg.substr(12));
		else if (arg.find("--nodeId=") == 0)    nodeId = arg.substr(9);
		else if (arg.find("--nodeAddress=") == 0) nodeAddress = arg.substr(14);
		else if (arg.find("--recordDir=") == 0) recordDir = arg.substr(12);
		else if (arg.find("--maxRoutersPerWorker=") == 0) maxRoutersPerWorker = std::stoi(arg.substr(22));
		else if (arg.find("--lat=") == 0)          nodeLat = std::stod(arg.substr(6));
		else if (arg.find("--lng=") == 0)          nodeLng = std::stod(arg.substr(6));
		else if (arg.find("--isp=") == 0)          nodeIsp = arg.substr(6);
		else if (arg.find("--country=") == 0)      nodeCountry = arg.substr(10);
		else if (arg == "--countryIsolation")       countryIsolation = true;
		else if (arg == "--noCountryIsolation")     countryIsolation = false;
		else if (arg.find("--geoDb=") == 0)        { geoDbPath = arg.substr(8); geoDbPathExplicit = true; }
		else if (arg.find("--logDir=") == 0) logDir = arg.substr(9);
		else if (arg.find("--logPrefix=") == 0) logPrefix = arg.substr(12);
		else if (arg.find("--logFile=") == 0)  applyLegacyLogFileSetting(arg.substr(10), logDir, logPrefix);
		else if (arg.find("--pidFile=") == 0)  pidFile = arg.substr(10);
		else if (arg.find("--logLevel=") == 0) logLevel = arg.substr(11);
		else if (arg.find("--logRotateHours=") == 0) logRotateHours = std::stoi(arg.substr(17));
		else if (arg == "--nodaemon")           noDaemon = true;
	}

	// Daemonize unless --nodaemon
	if (!noDaemon) {
		g_daemonStartupFd = daemonize(logDir, logPrefix, logRotateHours, pidFile);
		if (g_daemonStartupFd < 0) return 1;
	}

	Logger::Init(noDaemon ? "" : logDir, logLevel, noDaemon, logRotateHours, logPrefix, getpid());
	spdlog::info("mediasoup-cpp SFU starting (new architecture: WorkerThread pool)...");

	auto failExit = [&pidFile]() {
		notifyDaemonStartup(false);
		if (!pidFile.empty()) std::remove(pidFile.c_str());
		return 1;
	};

	// Auto-detect public IP if announcedIp not set
	if (announcedIp.empty()) {
		auto detectPublicIp = []() -> std::string {
			const char* cmds[] = {
				"curl -s --max-time 2 http://ifconfig.me 2>/dev/null",
				"curl -s --max-time 2 http://api.ipify.org 2>/dev/null",
				nullptr
			};
			for (auto cmd = cmds; *cmd; ++cmd) {
				FILE* fp = popen(*cmd, "r");
				if (!fp) continue;
				std::array<char, 64> buf{};
				std::string result;
				while (fgets(buf.data(), buf.size(), fp)) result += buf.data();
				pclose(fp);
				while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' '))
					result.pop_back();
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
	// Validate nodeId: [A-Za-z0-9_\-.:]{1,128}
	{
		bool valid = !nodeId.empty() && nodeId.size() <= 128;
		for (char c : nodeId) {
			if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
				  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':')) {
				valid = false; break;
			}
		}
		if (!valid) {
			spdlog::error("Invalid nodeId '{}' (allowed: [A-Za-z0-9_-.:]{{1,128}})", nodeId);
			return failExit();
		}
	}
	if (nodeAddress.empty()) {
		std::string ip = announcedIp.empty() ? listenIp : announcedIp;
		if (ip == "0.0.0.0") {
			// Try to find a non-loopback interface IP via UDP connect trick
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (sock >= 0) {
				sockaddr_in target{};
				target.sin_family = AF_INET;
				target.sin_port = htons(53);
				inet_pton(AF_INET, "8.8.8.8", &target.sin_addr);
				if (::connect(sock, (sockaddr*)&target, sizeof(target)) == 0) {
					sockaddr_in local{};
					socklen_t len = sizeof(local);
					if (getsockname(sock, (sockaddr*)&local, &len) == 0) {
						char buf[INET_ADDRSTRLEN]{};
						inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
						if (std::string(buf) != "0.0.0.0") ip = buf;
					}
				}
				::close(sock);
			}
			if (ip == "0.0.0.0") {
				ip = "127.0.0.1";
				spdlog::warn("Could not determine local IP, nodeAddress will use 127.0.0.1 — multi-node routing will not work");
			}
		}
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

	// GeoRouter for IP-based node selection
	std::unique_ptr<GeoRouter> geoRouter;
	auto geoDbResolution = resolveGeoDbPath(geoDbPath, geoDbPathExplicit);
	if (geoDbResolution.resolvedPath.empty()) {
		std::ostringstream searched;
		for (size_t i = 0; i < geoDbResolution.candidates.size(); ++i) {
			if (i != 0) searched << ", ";
			searched << geoDbResolution.candidates[i];
		}
		spdlog::warn("GeoRouter DB not found (requested: {}). Searched: {}. Geo-routing disabled",
			geoDbPath, searched.str());
	} else {
		if (geoDbResolution.usedFallback) {
			spdlog::warn("GeoRouter DB {} not found, falling back to {}",
				geoDbPath, geoDbResolution.resolvedPath);
		}
		geoDbPath = geoDbResolution.resolvedPath;
		geoRouter = std::make_unique<GeoRouter>();
		if (geoRouter->init(geoDbPath)) {
			spdlog::info("GeoRouter initialized from {}", geoDbPath);
			if (nodeLat == 0 && nodeLng == 0 && !announcedIp.empty()) {
				auto info = geoRouter->lookup(announcedIp);
				if (info.valid) {
					nodeLat = info.lat;
					nodeLng = info.lng;
					if (nodeIsp.empty()) nodeIsp = info.isp;
					if (nodeCountry.empty()) nodeCountry = info.country;
					spdlog::info("Auto-detected node geo: {}/{} lat={} lng={} isp={} country={}",
						info.province, info.city, nodeLat, nodeLng, nodeIsp, nodeCountry);
				}
			}
		} else {
			spdlog::warn("GeoRouter init failed ({}), geo-routing disabled", geoDbPath);
			geoRouter.reset();
		}
	}

	// Redis room registry
	std::unique_ptr<RoomRegistry> registry;
	try {
		registry = std::make_unique<RoomRegistry>(redisHost, redisPort, nodeId, nodeAddress,
			nodeLat, nodeLng, nodeIsp, nodeCountry, countryIsolation);
		if (geoRouter) registry->setGeoRouter(geoRouter.get());
		registry->start();
		g_registry = registry.get();
		spdlog::info("RoomRegistry started [nodeId:{} addr:{} lat:{} lng:{} isp:{} country:{} isolation:{}]",
			nodeId, nodeAddress, nodeLat, nodeLng, nodeIsp, nodeCountry, countryIsolation);
	} catch (const std::exception& e) {
		spdlog::warn("Redis not available ({}), running in single-node mode", e.what());
	}

	// ── Calculate WorkerThread pool size ──
	// Auto: numWorkerThreads = max(1, min(numWorkers, ceil(numWorkers / 2)))
	if (numWorkerThreads <= 0) {
		numWorkerThreads = std::max(1, std::min(numWorkers, static_cast<int>(std::ceil(numWorkers / 2.0))));
	}
	numWorkerThreads = std::min(numWorkerThreads, numWorkers); // can't have more threads than workers

	// Distribute workers across threads: workersPerThread[i]
	std::vector<int> workersPerThread(numWorkerThreads, 0);
	for (int i = 0; i < numWorkers; i++) {
		workersPerThread[i % numWorkerThreads]++;
	}

	// Worker settings
	WorkerSettings workerSettings;
	workerSettings.logLevel = "warn";
	workerSettings.rtcMinPort = 10000;
	workerSettings.rtcMaxPort = 59999;
	if (!workerBin.empty()) workerSettings.workerBin = workerBin;

	// Create WorkerThread pool
	std::vector<std::unique_ptr<WorkerThread>> workerThreads;
	for (int i = 0; i < numWorkerThreads; i++) {
		try {
			auto wt = std::make_unique<WorkerThread>(
				i, workerSettings, workersPerThread[i],
				mediaCodecs, listenInfos,
				registry.get(), recordDir,
				maxRoutersPerWorker > 0 ? static_cast<size_t>(maxRoutersPerWorker) : 0);
			workerThreads.push_back(std::move(wt));
			spdlog::info("WorkerThread {} created ({} workers)", i, workersPerThread[i]);
		} catch (const std::exception& e) {
			spdlog::error("Failed to create WorkerThread {}: {}", i, e.what());
		}
	}

	if (workerThreads.empty()) {
		spdlog::error("No WorkerThreads created, exiting");
		return failExit();
	}

	// Assemble and run
	SignalingServer server(signalingPort, workerThreads, registry.get(), recordDir);

	spdlog::info("mediasoup-cpp SFU ready - {} WorkerThreads, {} total workers, signaling on port {}, nodeId={}",
		workerThreads.size(), numWorkers, signalingPort, nodeId);

	bool runOk = server.run([](bool ok) {
		notifyDaemonStartup(ok);
	});

	// Graceful shutdown (reached when g_shutdown causes uWS loop to stop)
	spdlog::info("Shutting down...");
	// Stop WorkerThreads first — closeAllRooms() enqueues unregisterRoom tasks
	for (auto& wt : workerThreads) {
		wt->stop();
	}
	// Now stop registry worker — drains remaining unregister tasks
	server.stopRegistryWorker();
	if (registry) registry->stop();
	spdlog::info("Shutdown complete");
	if (!runOk) return failExit();
	notifyDaemonStartup(true);
	return 0;
}
