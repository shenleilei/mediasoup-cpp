#pragma once
#include "Channel.h"
#include "EventEmitter.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <set>

namespace mediasoup {

class Router;

struct WorkerSettings {
	std::string logLevel = "warn";
	std::vector<std::string> logTags;
	uint16_t rtcMinPort = 10000;
	uint16_t rtcMaxPort = 59999;
	std::string dtlsCertificateFile;
	std::string dtlsPrivateKeyFile;
	std::string workerBin;
};

class Worker : public std::enable_shared_from_this<Worker> {
public:
	explicit Worker(const WorkerSettings& settings);
	~Worker();

	int pid() const { return pid_; }
	bool closed() const { return closed_; }
	Channel& channel() { return *channel_; }
	EventEmitter& emitter() { return emitter_; }
	size_t routerCount() const { return routers_.size(); }

	std::shared_ptr<Router> createRouter(
		const std::vector<nlohmann::json>& mediaCodecs = {});

	void close();

private:
	void spawn(const WorkerSettings& settings);
	void workerDied(const std::string& reason);

	int pid_ = -1;
	bool closed_ = false;
	std::unique_ptr<Channel> channel_;
	std::set<std::shared_ptr<Router>> routers_;
	EventEmitter emitter_;
	std::thread waitThread_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
