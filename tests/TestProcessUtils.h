#pragma once

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <signal.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

inline bool isSfuProcessAlive(pid_t pid) {
	if (pid <= 0 || kill(pid, 0) != 0) return false;

	std::ifstream cmdline("/proc/" + std::to_string(pid) + "/cmdline", std::ios::binary);
	if (!cmdline.is_open()) return false;

	std::string cmd((std::istreambuf_iterator<char>(cmdline)), std::istreambuf_iterator<char>());
	return cmd.find("mediasoup-sfu") != std::string::npos;
}

inline bool isTcpPortBindable(int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return false;

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	const bool bindable = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
	::close(fd);
	return bindable;
}

inline bool waitForTcpPortFree(int port, int polls = 40, int sleepUs = 50000) {
	for (int i = 0; i < polls; ++i) {
		if (isTcpPortBindable(port)) return true;
		usleep(sleepUs);
	}
	return isTcpPortBindable(port);
}

inline bool waitForTcpPortListening(int port, int polls = 70, int sleepUs = 100000) {
	for (int i = 0; i < polls; ++i) {
		usleep(sleepUs);

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) return false;

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
			::close(fd);
			usleep(200000);
			return true;
		}
		::close(fd);
	}
	return false;
}

inline int allocateUniqueTestPort(int base = 18000, int span = 2000) {
	static std::atomic<int> nextOffset{0};
	for (int attempt = 0; attempt < span * 2; ++attempt) {
		const int offset = nextOffset.fetch_add(1);
		const int port = base + (offset % span);
		if (isTcpPortBindable(port)) return port;
	}
	return -1;
}

inline bool waitForDirectChildExit(pid_t pid, int polls, int sleepUs) {
	for (int i = 0; i < polls; ++i) {
		int status = 0;
		const pid_t waited = waitpid(pid, &status, WNOHANG);
		if (waited == pid) return true;
		if (waited == -1 && errno == ECHILD) return !isSfuProcessAlive(pid);
		if (!isSfuProcessAlive(pid)) return true;
		usleep(sleepUs);
	}

	int status = 0;
	const pid_t waited = waitpid(pid, &status, WNOHANG);
	if (waited == pid) return true;
	if (waited == -1 && errno == ECHILD) return !isSfuProcessAlive(pid);
	return !isSfuProcessAlive(pid);
}

inline pid_t sfuSignalTarget(pid_t pid) {
	const pid_t pgid = getpgid(pid);
	return (pgid == pid ? -pid : pid);
}

inline void terminateSfuProcess(pid_t pid, int gracefulPolls = 40, int sleepUs = 50000) {
	if (pid <= 0) return;
	if (!isSfuProcessAlive(pid)) {
		(void)waitForDirectChildExit(pid, 1, sleepUs);
		return;
	}

	const pid_t signalTarget = sfuSignalTarget(pid);
	kill(signalTarget, SIGTERM);
	if (waitForDirectChildExit(pid, gracefulPolls, sleepUs)) return;

	if (!isSfuProcessAlive(pid)) return;
	kill(signalTarget, SIGKILL);
	(void)waitForDirectChildExit(pid, 10, sleepUs);
}

class TestSfuProcess {
public:
	bool start(int port, const std::vector<std::string>& extraArgs = {}, const std::string& logPath = "/dev/null") {
		stop();
		port_ = port;
		logPath_ = logPath;

		if (!waitForTcpPortFree(port_)) return false;

		std::vector<std::string> args = {
			"./build/mediasoup-sfu",
			"--nodaemon",
			"--port=" + std::to_string(port_),
			"--workers=1",
			"--workerBin=./mediasoup-worker",
			"--announcedIp=127.0.0.1",
			"--listenIp=127.0.0.1",
			"--redisHost=0.0.0.0",
			"--redisPort=1",
		};
		args.insert(args.end(), extraArgs.begin(), extraArgs.end());

		pid_ = fork();
		if (pid_ < 0) {
			port_ = -1;
			return false;
		}

		if (pid_ == 0) {
			(void)setpgid(0, 0);

			const char* targetLog = logPath_.empty() ? "/dev/null" : logPath_.c_str();
			int logFd = open(targetLog, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (logFd < 0) logFd = open("/dev/null", O_WRONLY);
			if (logFd >= 0) {
				dup2(logFd, STDOUT_FILENO);
				dup2(logFd, STDERR_FILENO);
				if (logFd > STDERR_FILENO) close(logFd);
			}

			std::vector<char*> argv;
			argv.reserve(args.size() + 1);
			for (auto& arg : args) argv.push_back(const_cast<char*>(arg.c_str()));
			argv.push_back(nullptr);

			execv(argv[0], argv.data());
			_exit(127);
		}

		(void)setpgid(pid_, pid_);
		if (!waitForTcpPortListening(port_)) {
			stop();
			return false;
		}

		return true;
	}

	bool stop(int portReleasePolls = 40, int sleepUs = 50000) {
		if (pid_ <= 0 && port_ <= 0) return true;

		const pid_t pid = pid_;
		const int port = port_;
		pid_ = -1;
		port_ = -1;

		if (pid > 0) terminateSfuProcess(pid, 40, sleepUs);
		return port > 0 ? waitForTcpPortFree(port, portReleasePolls, sleepUs) : true;
	}

	~TestSfuProcess() { stop(); }

	pid_t pid() const { return pid_; }
	int port() const { return port_; }
	const std::string& logPath() const { return logPath_; }

private:
	pid_t pid_ = -1;
	int port_ = -1;
	std::string logPath_;
};

inline std::string makeTestSfuLogPath(const std::string& prefix, int port) {
	return "/tmp/" + prefix + "_" + std::to_string(getpid()) + "_" + std::to_string(port) + ".log";
}
