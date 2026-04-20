#include "Worker.h"
#include "Router.h"
#include "Utils.h"
#include "worker_generated.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <spawn.h>
#include <cstring>
#include <chrono>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

extern char** environ;

namespace mediasoup {

static const char* MEDIASOUP_VERSION = "3.14.6";
static constexpr int kTerminateGraceMs = 500;
static constexpr int kWorkerDeathReapTimeoutMs = 100;

namespace {
void closeFdIfValid(int& fd)
{
	if (fd >= 0) {
		::close(fd);
		fd = -1;
	}
}

void setCloseOnExec(int fd)
{
	int flags = ::fcntl(fd, F_GETFD, 0);
	if (flags >= 0) {
		(void)::fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
	}
}

int normalizeChildFdForSpawn(int fd)
{
	if (fd > 4) {
		return fd;
	}

	int dupFd = ::fcntl(fd, F_DUPFD_CLOEXEC, 5);
	if (dupFd < 0) {
		return -1;
	}

	::close(fd);
	return dupFd;
}

std::vector<std::string> buildWorkerArgs(
	const std::string& workerBin,
	const WorkerSettings& settings)
{
	std::vector<std::string> args;
	args.push_back(workerBin);
	if (!settings.logLevel.empty())
		args.push_back("--logLevel=" + settings.logLevel);
	for (const auto& tag : settings.logTags)
		args.push_back("--logTag=" + tag);
	args.push_back("--rtcMinPort=" + std::to_string(settings.rtcMinPort));
	args.push_back("--rtcMaxPort=" + std::to_string(settings.rtcMaxPort));
	if (!settings.dtlsCertificateFile.empty())
		args.push_back("--dtlsCertificateFile=" + settings.dtlsCertificateFile);
	if (!settings.dtlsPrivateKeyFile.empty())
		args.push_back("--dtlsPrivateKeyFile=" + settings.dtlsPrivateKeyFile);
	return args;
}

std::vector<char*> buildCStringVector(std::vector<std::string>& storage)
{
	std::vector<char*> result;
	result.reserve(storage.size() + 1);
	for (auto& entry : storage) {
		result.push_back(entry.data());
	}
	result.push_back(nullptr);
	return result;
}

std::vector<std::string> buildWorkerEnvironment()
{
	std::vector<std::string> envStorage;
	bool replacedVersion = false;

	for (char** current = environ; current && *current; ++current) {
		std::string entry(*current);
		if (entry.rfind("MEDIASOUP_VERSION=", 0) == 0) {
			envStorage.push_back("MEDIASOUP_VERSION=" + std::string(MEDIASOUP_VERSION));
			replacedVersion = true;
			continue;
		}
		envStorage.push_back(std::move(entry));
	}

	if (!replacedVersion) {
		envStorage.push_back("MEDIASOUP_VERSION=" + std::string(MEDIASOUP_VERSION));
	}

	return envStorage;
}

std::optional<int> waitChildWithTimeout(pid_t pid, int timeoutMs)
{
	if (pid <= 0) return 0;
	int status = 0;
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	while (std::chrono::steady_clock::now() < deadline) {
		pid_t r = ::waitpid(pid, &status, WNOHANG);
		if (r == pid) return std::optional<int>(status);
		if (r < 0 && errno == ECHILD) return std::optional<int>(status);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return std::nullopt;
}
} // namespace

// Original constructor — threaded mode (backward compat)
Worker::Worker(const WorkerSettings& settings)
	: logger_(Logger::Get("Worker"))
{
	spawn(settings, /*threaded=*/true);
}

// New constructor with explicit threading control
Worker::Worker(const WorkerSettings& settings, bool threaded)
	: threaded_(threaded), logger_(Logger::Get("Worker"))
{
	spawn(settings, threaded);
}

Worker::~Worker() {
	close();
	// If workerDied() set closed_ but didn't close channel, do it now
	if (channel_) channel_->close();
	// Ensure waitThread is not joinable at destruction (would call std::terminate)
	if (waitThread_.joinable()) {
		if (waitThread_.get_id() == std::this_thread::get_id())
			waitThread_.detach();
		else
			waitThread_.join();
	}
}

void Worker::spawn(const WorkerSettings& settings, bool threaded) {
	std::string workerBin = settings.workerBin;
	if (workerBin.empty()) {
		// Try common locations
		workerBin = "./mediasoup-worker";
	}

	// Create two pipe pairs for channel communication
	int producerPipe[2]; // parent writes, child reads (child fd 3)
	int consumerPipe[2]; // child writes, parent reads (child fd 4)

	if (::pipe(producerPipe) != 0) {
		throw std::runtime_error("pipe() failed: " + std::string(strerror(errno)));
	}
	if (::pipe(consumerPipe) != 0) {
		int pipeErrno = errno;
		::close(producerPipe[0]);
		::close(producerPipe[1]);
		throw std::runtime_error("pipe() failed: " + std::string(strerror(pipeErrno)));
	}

	for (int fd : {producerPipe[0], producerPipe[1], consumerPipe[0], consumerPipe[1]}) {
		setCloseOnExec(fd);
	}

	producerPipe[0] = normalizeChildFdForSpawn(producerPipe[0]);
	if (producerPipe[0] < 0) {
		int dupErrno = errno;
		::close(producerPipe[1]);
		::close(consumerPipe[0]);
		::close(consumerPipe[1]);
		throw std::runtime_error("fcntl(F_DUPFD_CLOEXEC) failed: " + std::string(strerror(dupErrno)));
	}

	consumerPipe[1] = normalizeChildFdForSpawn(consumerPipe[1]);
	if (consumerPipe[1] < 0) {
		int dupErrno = errno;
		::close(producerPipe[0]);
		::close(producerPipe[1]);
		::close(consumerPipe[0]);
		throw std::runtime_error("fcntl(F_DUPFD_CLOEXEC) failed: " + std::string(strerror(dupErrno)));
	}

	auto args = buildWorkerArgs(workerBin, settings);
	auto argv = buildCStringVector(args);
	auto envStorage = buildWorkerEnvironment();
	auto envp = buildCStringVector(envStorage);

	posix_spawn_file_actions_t fileActions;
	int actionRc = ::posix_spawn_file_actions_init(&fileActions);
	if (actionRc != 0) {
		::close(producerPipe[0]);
		::close(producerPipe[1]);
		::close(consumerPipe[0]);
		::close(consumerPipe[1]);
		throw std::runtime_error("posix_spawn_file_actions_init() failed: " + std::string(strerror(actionRc)));
	}

	auto throwSpawnActionError = [&](const char* action, int rc) {
		::posix_spawn_file_actions_destroy(&fileActions);
		::close(producerPipe[0]);
		::close(producerPipe[1]);
		::close(consumerPipe[0]);
		::close(consumerPipe[1]);
		throw std::runtime_error(std::string(action) + " failed: " + std::string(strerror(rc)));
	};

	actionRc = ::posix_spawn_file_actions_addclose(&fileActions, producerPipe[1]);
	if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_addclose(producer parent)", actionRc);
	actionRc = ::posix_spawn_file_actions_addclose(&fileActions, consumerPipe[0]);
	if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_addclose(consumer parent)", actionRc);
	actionRc = ::posix_spawn_file_actions_adddup2(&fileActions, producerPipe[0], 3);
	if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_adddup2(fd3)", actionRc);
	actionRc = ::posix_spawn_file_actions_adddup2(&fileActions, consumerPipe[1], 4);
	if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_adddup2(fd4)", actionRc);
	if (producerPipe[0] != 3) {
		actionRc = ::posix_spawn_file_actions_addclose(&fileActions, producerPipe[0]);
		if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_addclose(child read)", actionRc);
	}
	if (consumerPipe[1] != 4) {
		actionRc = ::posix_spawn_file_actions_addclose(&fileActions, consumerPipe[1]);
		if (actionRc != 0) throwSpawnActionError("posix_spawn_file_actions_addclose(child write)", actionRc);
	}

	int spawnRc = ::posix_spawnp(
		&pid_,
		workerBin.c_str(),
		&fileActions,
		nullptr,
		argv.data(),
		envp.data());
	::posix_spawn_file_actions_destroy(&fileActions);
	if (spawnRc != 0) {
		::close(producerPipe[0]);
		::close(producerPipe[1]);
		::close(consumerPipe[0]);
		::close(consumerPipe[1]);
		throw std::runtime_error("posix_spawnp() failed: " + std::string(strerror(spawnRc)));
	}

	::close(producerPipe[0]); // close child's read end
	::close(consumerPipe[1]); // close child's write end

	// Parent process
	int parentWriteFd = producerPipe[1]; // parent writes to child's fd 3
	int parentReadFd = consumerPipe[0];  // parent reads from child's fd 4

	try {
		channel_ = std::make_unique<Channel>(parentWriteFd, parentReadFd, pid_, threaded);
	} catch (...) {
		closeFdIfValid(parentWriteFd);
		closeFdIfValid(parentReadFd);
		if (pid_ > 0) {
			::kill(pid_, SIGKILL);
			int status = 0;
			(void)::waitpid(pid_, &status, 0);
		}
		throw;
	}

	MS_DEBUG(logger_, "worker spawned [pid:{}] threaded={}", pid_, threaded);

	// Listen for WORKER_RUNNING notification
	channel_->emitter().once(std::to_string(pid_), [this](const std::vector<std::any>& args) {
		try {
			if (args.empty()) {
				MS_WARN(logger_, "worker running notification args empty [pid:{}]", pid_);
				return;
			}
			auto event = std::any_cast<FBS::Notification::Event>(args[0]);
			if (event == FBS::Notification::Event::WORKER_RUNNING) {
				MS_DEBUG(logger_, "worker running [pid:{}]", pid_);
				emitter_.emit("@success");
			}
		} catch (const std::bad_any_cast& e) {
			MS_WARN(logger_, "worker notification cast failed [pid:{}]: {}", pid_, e.what());
		} catch (const std::exception& e) {
			MS_WARN(logger_, "worker notification failed [pid:{}]: {}", pid_, e.what());
		} catch (...) {
			MS_WARN(logger_, "worker notification failed [pid:{}]: unknown error", pid_);
		}
	});

	if (threaded) {
		// Wait thread for child exit (threaded mode only)
		waitThread_ = std::thread([this]() {
			int status;
			::waitpid(pid_, &status, 0);
			if (!closed_.load(std::memory_order_acquire)) {
				workerDied("worker process exited with status " + std::to_string(status));
			}
		});
	}
}

void Worker::close() {
	if (closed_.exchange(true, std::memory_order_acq_rel)) return;

	MS_DEBUG(logger_, "close() [pid:{}]", pid_);

	// Close all routers
	std::vector<std::shared_ptr<Router>> routersToClose;
	{
		std::lock_guard<std::mutex> lock(routersMutex_);
		routersToClose.assign(routers_.begin(), routers_.end());
		routers_.clear();
	}
	for (auto& router : routersToClose) {
		router->workerClosed();
	}

	// Send close request to worker (in 3.14.x, WORKER_CLOSE is a Request, not Notification)
	if (channel_) {
		try {
			channel_->request(FBS::Request::Method::WORKER_CLOSE);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "Worker::close() WORKER_CLOSE request failed [pid:{}]: {}", pid_, e.what());
		} catch (...) {
			MS_WARN(logger_, "Worker::close() WORKER_CLOSE request failed [pid:{}]: unknown error", pid_);
		}
		channel_->close();
	}

	// Kill the process
	if (pid_ > 0) {
		::kill(pid_, SIGTERM);
	}

	// Wait for waitThread to finish — but not if we ARE the waitThread
	if (waitThread_.joinable() && waitThread_.get_id() != std::this_thread::get_id())
		waitThread_.join();
	else if (waitThread_.joinable())
		waitThread_.detach();

	// In non-threaded mode, wait/reap child to avoid zombie
	if (!threaded_ && pid_ > 0) {
		auto status = waitChildWithTimeout(pid_, kTerminateGraceMs);
		if (!status.has_value()) {
			int forcedStatus = 0;
			::kill(pid_, SIGKILL);
			(void)::waitpid(pid_, &forcedStatus, 0);
		}
	}

	emitter_.emit("close");
}

void Worker::workerDied(const std::string& reason) {
	if (closed_.exchange(true, std::memory_order_acq_rel)) return;

	MS_ERROR(logger_, "worker died [pid:{}]: {}", pid_, reason);

	std::vector<std::shared_ptr<Router>> routersToClose;
	{
		std::lock_guard<std::mutex> lock(routersMutex_);
		routersToClose.assign(routers_.begin(), routers_.end());
		routers_.clear();
	}
	for (auto& router : routersToClose) {
		router->workerClosed();
	}

	// Don't call channel_->close() here — we're in waitThread, and close()
	// would join readThread which may deadlock. Channel will be cleaned up
	// when the Worker shared_ptr is released.

	emitter_.emit("died", {std::any(reason)});
}

bool Worker::processChannelData() {
	if (!channel_ || closed_.load(std::memory_order_acquire)) return false;
	return channel_->processAvailableData();
}

void Worker::handleWorkerDeath() {
	if (closed_.load(std::memory_order_acquire)) return;

	int status = waitChildWithTimeout(pid_, kWorkerDeathReapTimeoutMs).value_or(0);
	workerDied("worker process exited with status " + std::to_string(status));
}

std::shared_ptr<Router> Worker::createRouter(
	const std::vector<nlohmann::json>& mediaCodecs)
{
	if (closed_.load(std::memory_order_acquire)) throw std::runtime_error("Worker closed");

	std::string routerId = utils::generateUUIDv4();
	channel_->requestBuildWait(
		FBS::Request::Method::WORKER_CREATE_ROUTER,
		FBS::Request::Body::Worker_CreateRouterRequest,
		[routerId](flatbuffers::FlatBufferBuilder& builder) {
			auto routerIdOff = builder.CreateString(routerId);
			auto reqOff = FBS::Worker::CreateCreateRouterRequest(builder, routerIdOff);
			return reqOff.Union();
		}, ""); // wait for response

	auto router = std::make_shared<Router>(routerId, channel_.get(), mediaCodecs);
	{
		std::lock_guard<std::mutex> lock(routersMutex_);
		if (closed_.load(std::memory_order_acquire)) {
			router->workerClosed();
			throw std::runtime_error("Worker closed");
		}
		routers_.insert(router);
	}

	router->emitter().on("@close", [this, weak = std::weak_ptr<Router>(router)](auto&) {
		if (auto r = weak.lock()) {
			std::lock_guard<std::mutex> lock(routersMutex_);
			routers_.erase(r);
		}
	});

	MS_DEBUG(logger_, "Router created [id:{}]", routerId);
	return router;
}

} // namespace mediasoup
