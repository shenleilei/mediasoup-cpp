#pragma once

#include "GeoRouter.h"
#include "RoomRegistry.h"
#include "Worker.h"

#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <vector>

namespace mediasoup {
class WorkerThread;

struct RuntimeOptions {
	int numWorkers{ 1 };
	int numWorkerThreads{ 0 };
	int signalingPort{ 3000 };
	std::string listenIp{ "0.0.0.0" };
	std::string announcedIp;
	std::string workerBin;
	std::string redisHost{ "127.0.0.1" };
	int redisPort{ 6379 };
	bool redisRequired{ true };
	int rtcMinPort{ 10000 };
	int rtcMaxPort{ 59999 };
	bool webRtcServerEnabled{ false };
	int webRtcServerMinPort{ 0 };
	int webRtcServerMaxPort{ 0 };
	std::string nodeId;
	std::string nodeAddress;
	int maxRoutersPerWorker{ 0 };
	double nodeLat{ 0.0 };
	double nodeLng{ 0.0 };
	std::string nodeIsp;
	std::string nodeCountry;
	bool countryIsolation{ true };
	std::string geoDbPath{ "./ip2region.xdb" };
	bool geoDbPathExplicit{ false };
	bool noDaemon{ false };
	std::string logDir{ "/var/log/mediasoup-cpp" };
	std::string logPrefix{ "mediasoup-sfu" };
	std::string pidFile{ "/var/run/mediasoup-sfu.pid" };
	std::string logLevel{ "info" };
	int logRotateHours{ 3 };
	std::string configPath{ "config.json" };
	std::string loadError;

	bool hasLoadError() const { return !loadError.empty(); }
};

struct RuntimeServices {
	std::unique_ptr<GeoRouter> geoRouter;
	std::unique_ptr<RoomRegistry> registry;
	std::string startupError;
};

RuntimeOptions LoadRuntimeOptions(int argc, char* argv[]);
bool FinalizeRuntimeOptions(RuntimeOptions& options);
RuntimeServices CreateRuntimeServices(RuntimeOptions& options);
std::vector<nlohmann::json> DefaultMediaCodecs();
std::vector<nlohmann::json> BuildListenInfos(const RuntimeOptions& options);
std::vector<int> ComputeWorkersPerThread(int numWorkers, int numWorkerThreads);
WorkerSettings BuildWorkerSettings(const RuntimeOptions& options);
std::vector<std::unique_ptr<WorkerThread>> CreateWorkerThreadPool(
	const RuntimeOptions& options,
	const std::vector<nlohmann::json>& mediaCodecs,
	const std::vector<nlohmann::json>& listenInfos,
	RoomRegistry* registry);

} // namespace mediasoup
