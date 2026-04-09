#include "Worker.h"
#include "Router.h"
#include "Utils.h"
#include "worker_generated.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace mediasoup {

static const char* MEDIASOUP_VERSION = "3.14.0";
static constexpr int kTerminateGraceMs = 500;
static constexpr int kWorkerDeathReapTimeoutMs = 100;

namespace {
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

	if (::pipe(producerPipe) != 0 || ::pipe(consumerPipe) != 0) {
		throw std::runtime_error("pipe() failed: " + std::string(strerror(errno)));
	}

	pid_ = ::fork();
	if (pid_ < 0) {
		throw std::runtime_error("fork() failed");
	}

	if (pid_ == 0) {
		// Child process
		// Close parent's ends first
		::close(producerPipe[1]);
		::close(consumerPipe[0]);

		// Map pipes to fd 3 and 4
		// fd 3: child reads commands from parent
		// fd 4: child writes responses to parent
		if (producerPipe[0] != 3) {
			::dup2(producerPipe[0], 3);
			::close(producerPipe[0]);
		}
		if (consumerPipe[1] != 4) {
			::dup2(consumerPipe[1], 4);
			::close(consumerPipe[1]);
		}

		// Build args
		std::vector<std::string> args;
		args.push_back(workerBin);
		if (!settings.logLevel.empty())
			args.push_back("--logLevel=" + settings.logLevel);
		for (auto& tag : settings.logTags)
			args.push_back("--logTag=" + tag);
		args.push_back("--rtcMinPort=" + std::to_string(settings.rtcMinPort));
		args.push_back("--rtcMaxPort=" + std::to_string(settings.rtcMaxPort));
		if (!settings.dtlsCertificateFile.empty())
			args.push_back("--dtlsCertificateFile=" + settings.dtlsCertificateFile);
		if (!settings.dtlsPrivateKeyFile.empty())
			args.push_back("--dtlsPrivateKeyFile=" + settings.dtlsPrivateKeyFile);

		// Convert to char*[]
		std::vector<char*> argv;
		for (auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
		argv.push_back(nullptr);

		// Set env
		setenv("MEDIASOUP_VERSION", MEDIASOUP_VERSION, 1);

		::execvp(workerBin.c_str(), argv.data());
		// If exec fails
		_exit(1);
	}

	// Parent process
	::close(producerPipe[0]); // close child's read end
	::close(consumerPipe[1]); // close child's write end

	int parentWriteFd = producerPipe[1]; // parent writes to child's fd 3
	int parentReadFd = consumerPipe[0];  // parent reads from child's fd 4

	channel_ = std::make_unique<Channel>(parentWriteFd, parentReadFd, pid_, threaded);

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
			if (!closed_) {
				workerDied("worker process exited with status " + std::to_string(status));
			}
		});
	}
}

void Worker::close() {
	if (closed_) return;
	closed_ = true;

	MS_DEBUG(logger_, "close() [pid:{}]", pid_);

	// Close all routers
	for (auto& router : routers_) {
		router->workerClosed();
	}
	routers_.clear();

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
	if (closed_) return;
	closed_ = true;

	MS_ERROR(logger_, "worker died [pid:{}]: {}", pid_, reason);

	for (auto& router : routers_) {
		router->workerClosed();
	}
	routers_.clear();

	// Don't call channel_->close() here — we're in waitThread, and close()
	// would join readThread which may deadlock. Channel will be cleaned up
	// when the Worker shared_ptr is released.

	emitter_.emit("died", {std::any(reason)});
}

bool Worker::processChannelData() {
	if (!channel_ || closed_) return false;
	return channel_->processAvailableData();
}

void Worker::handleWorkerDeath() {
	if (closed_) return;

	int status = waitChildWithTimeout(pid_, kWorkerDeathReapTimeoutMs).value_or(0);
	workerDied("worker process exited with status " + std::to_string(status));
}

std::shared_ptr<Router> Worker::createRouter(
	const std::vector<nlohmann::json>& mediaCodecs)
{
	if (closed_) throw std::runtime_error("Worker closed");

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
	routers_.insert(router);

	router->emitter().on("@close", [this, weak = std::weak_ptr<Router>(router)](auto&) {
		if (auto r = weak.lock()) routers_.erase(r);
	});

	MS_DEBUG(logger_, "Router created [id:{}]", routerId);
	return router;
}

} // namespace mediasoup
