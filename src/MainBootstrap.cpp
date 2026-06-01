#include "MainBootstrap.h"

#include "GeoDbResolver.h"
#include "Logger.h"
#include "PublicIpResolver.h"
#include "RuntimeOptionParsers.h"
#include "WorkerThread.h"

#include <arpa/inet.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

namespace mediasoup {
namespace {

void AppendLoadError(RuntimeOptions& options, const std::string& message)
{
	if (!options.loadError.empty()) {
		options.loadError += "; ";
	}
	options.loadError += message;
}

void ApplyLegacyLogFileSetting(
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

std::string DetectPublicIp()
{
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
		while (!result.empty() &&
			(result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
			result.pop_back();
		}
		if (!result.empty() &&
			result.find('.') != std::string::npos &&
			result.find(' ') == std::string::npos) {
			return result;
		}
	}
	return {};
}

bool IsValidNodeId(const std::string& nodeId)
{
	if (nodeId.empty() || nodeId.size() > 128) {
		MS_SPDLOG_ERROR("Invalid nodeId length [value:{}]", nodeId);
		return false;
	}
	for (char c : nodeId) {
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':')) {
			MS_SPDLOG_ERROR("Invalid nodeId character [value:{}]", nodeId);
			return false;
		}
	}
	return true;
}

std::string ResolveNodeAddressIp(const RuntimeOptions& options)
{
	std::string ip = options.announcedIp.empty() ? options.listenIp : options.announcedIp;
	if (ip != "0.0.0.0") {
		return ip;
	}

	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock >= 0) {
		sockaddr_in target{};
		target.sin_family = AF_INET;
		target.sin_port = htons(53);
		inet_pton(AF_INET, "8.8.8.8", &target.sin_addr);
		if (::connect(sock, reinterpret_cast<sockaddr*>(&target), sizeof(target)) == 0) {
			sockaddr_in local{};
			socklen_t len = sizeof(local);
			if (getsockname(sock, reinterpret_cast<sockaddr*>(&local), &len) == 0) {
				char buf[INET_ADDRSTRLEN]{};
				inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
				if (std::string(buf) != "0.0.0.0") {
					ip = buf;
				}
			}
		}
		::close(sock);
	}

	return ip == "0.0.0.0" ? "127.0.0.1" : ip;
}

} // namespace

RuntimeOptions LoadRuntimeOptions(int argc, char* argv[])
{
	RuntimeOptions options;
	options.numWorkers = static_cast<int>(std::thread::hardware_concurrency());
	options.numWorkers = std::max(1, options.numWorkers - 2);

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--config=") == 0) options.configPath = arg.substr(9);
		if (arg.rfind("--", 0) == 0 &&
			arg.find('=') == std::string::npos &&
			arg != "--localOnly" &&
			arg != "--countryIsolation" &&
			arg != "--noCountryIsolation" &&
			arg != "--nodaemon") {
			AppendLoadError(options, "unsupported CLI flag '" + arg + "'");
		}
	}

	{
		std::ifstream cfgFile(options.configPath);
		if (cfgFile.is_open()) {
			try {
				auto cfg = nlohmann::json::parse(cfgFile);
					if (cfg.contains("port")) options.signalingPort = cfg["port"].get<int>();
					if (cfg.contains("workers")) { int v = cfg["workers"].get<int>(); if (v > 0) options.numWorkers = v; }
					if (cfg.contains("workerThreads")) { int v = cfg["workerThreads"].get<int>(); if (v > 0) options.numWorkerThreads = v; }
				if (cfg.contains("workerBin")) options.workerBin = cfg["workerBin"].get<std::string>();
				if (cfg.contains("webRtcServerPort")) { int v = cfg["webRtcServerPort"].get<int>(); if (v > 0) options.webRtcServerPort = v; }
				if (cfg.contains("nodeId")) options.nodeId = cfg["nodeId"].get<std::string>();
				if (cfg.contains("nodeAddress")) options.nodeAddress = cfg["nodeAddress"].get<std::string>();
				if (cfg.contains("hawkeyeRegisterUrl")) options.hawkeyeRegisterUrl = cfg["hawkeyeRegisterUrl"].get<std::string>();
				if (cfg.contains("hawkeyeRegisterType")) options.hawkeyeRegisterType = cfg["hawkeyeRegisterType"].get<std::string>();
				if (cfg.contains("maxRoutersPerWorker")) options.maxRoutersPerWorker = cfg["maxRoutersPerWorker"].get<int>();
				if (cfg.contains("lat")) options.nodeLat = cfg["lat"].get<double>();
				if (cfg.contains("lng")) options.nodeLng = cfg["lng"].get<double>();
				if (cfg.contains("isp")) options.nodeIsp = cfg["isp"].get<std::string>();
				if (cfg.contains("country")) options.nodeCountry = cfg["country"].get<std::string>();
				if (cfg.contains("countryIsolation")) options.countryIsolation = cfg["countryIsolation"].get<bool>();
				if (cfg.contains("geoDb")) { options.geoDbPath = cfg["geoDb"].get<std::string>(); options.geoDbPathExplicit = true; }
				if (cfg.contains("logDir")) options.logDir = cfg["logDir"].get<std::string>();
				if (cfg.contains("logPrefix")) options.logPrefix = cfg["logPrefix"].get<std::string>();
				if (cfg.contains("logFile")) ApplyLegacyLogFileSetting(cfg["logFile"].get<std::string>(), options.logDir, options.logPrefix);
				if (cfg.contains("pidFile")) options.pidFile = cfg["pidFile"].get<std::string>();
				if (cfg.contains("logLevel")) options.logLevel = cfg["logLevel"].get<std::string>();
				if (cfg.contains("logRotateHours")) options.logRotateHours = cfg["logRotateHours"].get<int>();
				if (cfg.contains("nodaemon")) options.noDaemon = cfg["nodaemon"].get<bool>();
				MS_SPDLOG_INFO("Loaded config from {}", options.configPath);
			} catch (const std::exception& e) {
				MS_SPDLOG_WARN("Failed to parse {}: {}", options.configPath, e.what());
			}
		}
	}

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		auto trySetInt = [&](const char* prefix, int& target) {
			if (arg.find(prefix) != 0) return false;
			int parsed = 0;
			if (TryParseIntArgValue(arg.substr(std::strlen(prefix)), parsed)) {
				target = parsed;
			} else {
				AppendLoadError(options, "invalid numeric CLI arg '" + arg + "'");
			}
			return true;
		};
		auto trySetDouble = [&](const char* prefix, double& target) {
			if (arg.find(prefix) != 0) return false;
			double parsed = 0.0;
			if (TryParseDoubleArgValue(arg.substr(std::strlen(prefix)), parsed)) {
				target = parsed;
			} else {
				AppendLoadError(options, "invalid numeric CLI arg '" + arg + "'");
			}
			return true;
		};

			if (trySetInt("--port=", options.signalingPort)) {}
			else if (trySetInt("--workers=", options.numWorkers)) {}
			else if (trySetInt("--workerThreads=", options.numWorkerThreads)) {}
		else if (arg == "--localOnly") options.localOnly = true;
		else if (arg.find("--listenIp=") == 0) { options.listenIp = arg.substr(11); options.listenIpExplicit = true; }
		else if (arg.find("--workerBin=") == 0) options.workerBin = arg.substr(12);
		else if (trySetInt("--webRtcServerPort=", options.webRtcServerPort)) {}
		else if (arg.find("--nodeId=") == 0) options.nodeId = arg.substr(9);
		else if (arg.find("--nodeAddress=") == 0) options.nodeAddress = arg.substr(14);
		else if (arg.find("--hawkeyeRegisterUrl=") == 0) options.hawkeyeRegisterUrl = arg.substr(21);
		else if (arg.find("--hawkeyeRegisterType=") == 0) options.hawkeyeRegisterType = arg.substr(22);
		else if (trySetInt("--maxRoutersPerWorker=", options.maxRoutersPerWorker)) {}
		else if (trySetDouble("--lat=", options.nodeLat)) {}
		else if (trySetDouble("--lng=", options.nodeLng)) {}
		else if (arg.find("--isp=") == 0) options.nodeIsp = arg.substr(6);
		else if (arg.find("--country=") == 0) options.nodeCountry = arg.substr(10);
		else if (arg == "--countryIsolation") options.countryIsolation = true;
		else if (arg == "--noCountryIsolation") options.countryIsolation = false;
		else if (arg.find("--geoDb=") == 0) { options.geoDbPath = arg.substr(8); options.geoDbPathExplicit = true; }
		else if (arg.find("--logDir=") == 0) options.logDir = arg.substr(9);
		else if (arg.find("--logPrefix=") == 0) options.logPrefix = arg.substr(12);
		else if (arg.find("--logFile=") == 0) ApplyLegacyLogFileSetting(arg.substr(10), options.logDir, options.logPrefix);
		else if (arg.find("--pidFile=") == 0) options.pidFile = arg.substr(10);
		else if (arg.find("--logLevel=") == 0) options.logLevel = arg.substr(11);
		else if (trySetInt("--logRotateHours=", options.logRotateHours)) {}
		else if (arg == "--nodaemon") options.noDaemon = true;
	}

	return options;
}

bool FinalizeRuntimeOptions(RuntimeOptions& options)
{
	if (options.hasLoadError()) {
		MS_SPDLOG_ERROR("Runtime option parsing failed: {}", options.loadError);
		return false;
	}

	if (options.localOnly) {
		options.listenIp = "127.0.0.1";
		options.listenIpExplicit = true;
		options.announcedIp.clear();
	}

	const bool localLoopbackMode =
		(options.localOnly ||
			(options.listenIpExplicit && (options.listenIp == "127.0.0.1" || options.listenIp == "::1"))) &&
		options.announcedIp.empty();

	if (localLoopbackMode) {
		MS_SPDLOG_INFO("Loopback listenIp detected, skipping announced public IP detection");
	} else {
		options.announcedIp = DetectPublicIp();
		if (!options.announcedIp.empty()) {
			MS_SPDLOG_INFO("Auto-detected public IP: {}", options.announcedIp);
		} else {
			MS_SPDLOG_ERROR("Could not detect public IP, startup failed");
			return false;
		}
		auto announcedPublicIp = mediasoup::ResolvePublicIpv4Address(options.announcedIp);
		if (!announcedPublicIp) {
			MS_SPDLOG_ERROR("Detected public IP must resolve to a public IPv4 address: {}", options.announcedIp);
			return false;
		}
		if (*announcedPublicIp != options.announcedIp) {
			MS_SPDLOG_INFO("Resolved announcedIp {} to public IPv4 {}", options.announcedIp, *announcedPublicIp);
			options.announcedIp = *announcedPublicIp;
		}
	}

	if (options.hawkeyeRegisterUrl.empty()) {
		if (const char* env = std::getenv("HAWKEYE_REGISTER_URL")) {
			options.hawkeyeRegisterUrl = env;
		} else if (const char* env = std::getenv("REGISTER_URL")) {
			options.hawkeyeRegisterUrl = env;
		}
	}
	if (const char* env = std::getenv("HAWKEYE_REGISTER_TYPE")) {
		options.hawkeyeRegisterType = env;
	}

	if (options.nodeId.empty()) {
		char hostname[256]{};
		if (::gethostname(hostname, sizeof(hostname) - 1) != 0) {
			MS_SPDLOG_WARN("gethostname() failed: {}", std::strerror(errno));
		}
		hostname[sizeof(hostname) - 1] = '\0';
		options.nodeId = std::string(hostname) + ":" + std::to_string(options.signalingPort);
	}
	if (!IsValidNodeId(options.nodeId)) {
		MS_SPDLOG_ERROR("Invalid nodeId '{}' (allowed: [A-Za-z0-9_-.:]{{1,128}})", options.nodeId);
		return false;
	}
	if (options.webRtcServerPort <= 0) {
		options.webRtcServerPort = 9000;
		MS_SPDLOG_INFO("webRtcServerPort not provided, defaulting to 9000");
	}
	if (options.webRtcServerPort <= 0) {
		MS_SPDLOG_ERROR("Invalid WebRtcServer port: {}", options.webRtcServerPort);
		return false;
	}

	if (options.nodeAddress.empty()) {
		auto ip = ResolveNodeAddressIp(options);
		if (ip == "127.0.0.1" && options.listenIp == "0.0.0.0" && options.announcedIp.empty()) {
			MS_SPDLOG_WARN("Could not determine local IP, nodeAddress will use 127.0.0.1 — multi-node routing will not work");
		}
		options.nodeAddress = "ws://" + ip + ":" + std::to_string(options.signalingPort) + "/ws";
	}

	return true;
}

std::vector<nlohmann::json> DefaultMediaCodecs()
{
	return {
		{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}},
		{{"mimeType", "video/H264"}, {"clockRate", 90000},
			{"parameters", {{"packetization-mode", 1}, {"profile-level-id", "4d0032"}, {"level-asymmetry-allowed", 1}}}},
		{{"mimeType", "video/H264"}, {"clockRate", 90000},
			{"parameters", {{"packetization-mode", 1}, {"profile-level-id", "42e01f"}, {"level-asymmetry-allowed", 1}}}},
		{{"mimeType", "video/VP8"}, {"clockRate", 90000}},
		{{"mimeType", "video/VP9"}, {"clockRate", 90000}, {"parameters", {{"profile-id", 0}}}}
	};
}

std::vector<nlohmann::json> BuildListenInfos(const RuntimeOptions& options)
{
	nlohmann::json listenInfo = {{"ip", options.listenIp}, {"protocol", "udp"}};
	if (!options.announcedIp.empty()) listenInfo["announcedAddress"] = options.announcedIp;
	return {listenInfo};
}

RuntimeServices CreateRuntimeServices(RuntimeOptions& options)
{
	RuntimeServices services;

	auto geoDbResolution = resolveGeoDbPath(options.geoDbPath, options.geoDbPathExplicit);
	if (geoDbResolution.resolvedPath.empty()) {
		std::ostringstream searched;
		for (size_t i = 0; i < geoDbResolution.candidates.size(); ++i) {
			if (i != 0) searched << ", ";
			searched << geoDbResolution.candidates[i];
		}
		MS_SPDLOG_WARN("GeoRouter DB not found (requested: {}). Searched: {}. Geo-routing disabled",
			options.geoDbPath, searched.str());
	} else {
		if (geoDbResolution.usedFallback) {
			MS_SPDLOG_WARN("GeoRouter DB {} not found, falling back to {}",
				options.geoDbPath, geoDbResolution.resolvedPath);
		}
		options.geoDbPath = geoDbResolution.resolvedPath;
		services.geoRouter = std::make_unique<GeoRouter>();
		if (services.geoRouter->init(options.geoDbPath)) {
			MS_SPDLOG_INFO("GeoRouter initialized from {}", options.geoDbPath);
			if (options.nodeLat == 0 && options.nodeLng == 0 && !options.announcedIp.empty()) {
				auto info = services.geoRouter->lookup(options.announcedIp);
				if (info.valid) {
					options.nodeLat = info.lat;
					options.nodeLng = info.lng;
					if (options.nodeIsp.empty()) options.nodeIsp = info.isp;
					if (options.nodeCountry.empty()) options.nodeCountry = info.country;
					MS_SPDLOG_INFO("Auto-detected node geo: {}/{} lat={} lng={} isp={} country={}",
						info.province, info.city, options.nodeLat, options.nodeLng, options.nodeIsp, options.nodeCountry);
				}
			}
		} else {
			MS_SPDLOG_WARN("GeoRouter init failed ({}), geo-routing disabled", options.geoDbPath);
			services.geoRouter.reset();
		}
	}

	try {
		MS_SPDLOG_INFO("Starting in local-only mode");
	} catch (const std::exception& e) {
		(void)e;
	}

	return services;
}

std::vector<int> ComputeWorkersPerThread(int numWorkers, int numWorkerThreads)
{
	int effectiveThreads = numWorkerThreads;
	if (effectiveThreads <= 0) {
		effectiveThreads = std::max(1, std::min(numWorkers, static_cast<int>(std::ceil(numWorkers / 2.0))));
	}
	effectiveThreads = std::min(effectiveThreads, numWorkers);

	std::vector<int> workersPerThread(effectiveThreads, 0);
	for (int i = 0; i < numWorkers; i++) {
		workersPerThread[i % effectiveThreads]++;
	}
	return workersPerThread;
}

WorkerSettings BuildWorkerSettings(const RuntimeOptions& options)
{
	WorkerSettings workerSettings;
	workerSettings.logLevel = "warn";
	if (const char* envWorkerLogLevel = std::getenv("MEDIASOUP_WORKER_LOG_LEVEL")) {
		if (*envWorkerLogLevel != '\0') {
			workerSettings.logLevel = envWorkerLogLevel;
		}
	}
	if (const char* envWorkerLogTags = std::getenv("MEDIASOUP_WORKER_LOG_TAGS")) {
		if (*envWorkerLogTags != '\0') {
			std::stringstream ss(envWorkerLogTags);
			std::string tag;
			while (std::getline(ss, tag, ',')) {
				tag.erase(0, tag.find_first_not_of(" \t"));
				tag.erase(tag.find_last_not_of(" \t") + 1);
				if (!tag.empty()) {
					workerSettings.logTags.push_back(tag);
				}
			}
		}
	}
	workerSettings.rtcPort = static_cast<uint16_t>(
		options.webRtcServerPort > 0 ? options.webRtcServerPort : 9000);
	if (!options.workerBin.empty()) workerSettings.workerBin = options.workerBin;
	return workerSettings;
}

std::vector<std::unique_ptr<WorkerThread>> CreateWorkerThreadPool(
	const RuntimeOptions& options,
	const std::vector<nlohmann::json>& mediaCodecs,
	const std::vector<nlohmann::json>& listenInfos)
{
	auto workerSettings = BuildWorkerSettings(options);
	auto workersPerThread = ComputeWorkersPerThread(options.numWorkers, options.numWorkerThreads);

	std::vector<std::unique_ptr<WorkerThread>> workerThreads;
	int nextWebRtcServerPort = options.webRtcServerPort;
	for (size_t i = 0; i < workersPerThread.size(); ++i) {
		try {
			auto workerThread = std::make_unique<WorkerThread>(
				static_cast<int>(i),
				workerSettings,
				workersPerThread[i],
				mediaCodecs,
				listenInfos,
				nextWebRtcServerPort,
				options.maxRoutersPerWorker > 0 ? static_cast<size_t>(options.maxRoutersPerWorker) : 0);
			workerThreads.push_back(std::move(workerThread));
			nextWebRtcServerPort += workersPerThread[i];
			MS_SPDLOG_INFO("WorkerThread {} created ({} workers)", i, workersPerThread[i]);
		} catch (const std::exception& e) {
			MS_SPDLOG_ERROR("Failed to create WorkerThread {}: {}", i, e.what());
		}
	}

	return workerThreads;
}

} // namespace mediasoup
