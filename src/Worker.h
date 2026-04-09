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
	// Original constructor — threaded mode (creates readThread + waitThread)
	explicit Worker(const WorkerSettings& settings);
	// New constructor — explicit threading control
	Worker(const WorkerSettings& settings, bool threaded);
	~Worker();

	int pid() const { return pid_; }
	bool closed() const { return closed_; }
	Channel& channel() { return *channel_; }
	EventEmitter& emitter() { return emitter_; }
	size_t routerCount() const { return routers_.size(); }
	bool isThreaded() const { return threaded_; }

	std::shared_ptr<Router> createRouter(
		const std::vector<nlohmann::json>& mediaCodecs = {});

	void close();

	// ── Non-threaded mode API ──

	// Get Channel's consumer fd for epoll registration
	int channelConsumerFd() const { return channel_ ? channel_->consumerFd() : -1; }

	// Process available Channel data (called by WorkerThread's epoll loop).
	// Returns true if still alive; false if worker died (EOF).
	bool processChannelData();

	// Called by WorkerThread when processChannelData() returns false (EOF).
	// Reaps the child process and fires "died" event.
	void handleWorkerDeath();

private:
	void spawn(const WorkerSettings& settings, bool threaded);
	void workerDied(const std::string& reason);

	int pid_ = -1;
	bool closed_ = false;
	bool threaded_ = true;
	std::unique_ptr<Channel> channel_;
	std::set<std::shared_ptr<Router>> routers_;
	EventEmitter emitter_;
	std::thread waitThread_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
