#pragma once

#include <algorithm>
#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <filesystem>
#include <signal.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <vector>

inline bool isExecutableFile(const std::string& path) {
	return !path.empty() && access(path.c_str(), X_OK) == 0;
}

inline std::string firstExecutablePath(const std::vector<std::string>& candidates, const std::string& fallback) {
	for (const auto& candidate : candidates) {
		if (isExecutableFile(candidate)) return candidate;
	}
	return fallback;
}

inline std::string testSfuBinaryPath() {
	if (const char* env = std::getenv("MEDIASOUP_TEST_SFU_BIN")) {
		if (*env) return env;
	}
	return firstExecutablePath({
		"./build/mediasoup-sfu",
		"./build_dbg/mediasoup-sfu",
	}, "./build/mediasoup-sfu");
}

inline std::string testWorkerBinaryPath() {
	if (const char* env = std::getenv("MEDIASOUP_TEST_WORKER_BIN")) {
		if (*env) return env;
	}
	if (const char* env = std::getenv("QOS_CPP_CLIENT_WORKER_BIN")) {
		if (*env) return env;
	}
	return firstExecutablePath({
		"./mediasoup-worker",
		"./build/mediasoup-worker",
	}, "./mediasoup-worker");
}

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

inline bool isUdpPortBindable(int port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
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

inline bool waitForUdpPortFree(int port, int polls = 40, int sleepUs = 50000) {
	for (int i = 0; i < polls; ++i) {
		if (isUdpPortBindable(port)) return true;
		usleep(sleepUs);
	}
	return isUdpPortBindable(port);
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

inline int testWebRtcServerPortForSignalingPort(int signalingPort) {
	if (signalingPort > 0) {
		const int base = 50000;
		const int candidate = base + (signalingPort - 14000);
		if (candidate >= base && candidate < 60000) return candidate;
	}
	return 50000;
}

inline bool ensureTestSignalingTlsFiles() {
	const std::string certPath = "/opt/mediasoup-cpp/certs/tls.pem";
	const std::string keyPath = "/opt/mediasoup-cpp/certs/tls.key";

	std::error_code ec;
	std::filesystem::create_directories("/opt/mediasoup-cpp/certs", ec);
	if (ec) return false;

	const std::string repoCertPath = "docker/_.zelostech.com.cn.pem";
	const std::string repoKeyPath = "docker/_.zelostech.com.cn.key";
	if (access(repoCertPath.c_str(), R_OK) == 0 && access(repoKeyPath.c_str(), R_OK) == 0) {
		std::filesystem::copy_file(
			repoCertPath,
			certPath,
			std::filesystem::copy_options::overwrite_existing,
			ec);
		if (ec) return false;
		std::filesystem::copy_file(
			repoKeyPath,
			keyPath,
			std::filesystem::copy_options::overwrite_existing,
			ec);
		if (ec) return false;
		std::filesystem::permissions(
			certPath,
			std::filesystem::perms::owner_read | std::filesystem::perms::group_read |
				std::filesystem::perms::others_read,
			std::filesystem::perm_options::replace,
			ec);
		if (ec) return false;
		std::filesystem::permissions(
			keyPath,
			std::filesystem::perms::owner_read,
			std::filesystem::perm_options::replace,
			ec);
		return !ec &&
			access(certPath.c_str(), R_OK) == 0 &&
			access(keyPath.c_str(), R_OK) == 0;
	}

	if (access(certPath.c_str(), R_OK) == 0 && access(keyPath.c_str(), R_OK) == 0) {
		return true;
	}

	const std::string cmd =
		"openssl req -x509 -newkey rsa:2048 -sha256 -nodes "
		"-days 7 "
		"-subj '/CN=127.0.0.1' "
		"-addext 'subjectAltName=IP:127.0.0.1,DNS:localhost' "
		"-keyout " + keyPath + " "
		"-out " + certPath + " >/dev/null 2>&1";
	return std::system(cmd.c_str()) == 0 &&
		access(certPath.c_str(), R_OK) == 0 &&
		access(keyPath.c_str(), R_OK) == 0;
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

inline void cleanupFixedTestPortProcesses(int tcpPort)
{
	if (tcpPort <= 0) return;

	for (int i = 0; i < 3; ++i) {
		if (isTcpPortBindable(tcpPort)) return;

		std::vector<pid_t> victims;
		for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
			if (!entry.is_directory()) continue;
			const auto name = entry.path().filename().string();
			if (name.empty() ||
				!std::all_of(name.begin(), name.end(), [](unsigned char ch) { return std::isdigit(ch) != 0; }))
				continue;
			const pid_t pid = static_cast<pid_t>(std::atoi(name.c_str()));
			if (pid <= 0) continue;
			if (!isSfuProcessAlive(pid)) continue;

			std::ifstream cmdline(entry.path() / "cmdline", std::ios::binary);
			if (!cmdline.is_open()) continue;
			std::string cmd((std::istreambuf_iterator<char>(cmdline)), std::istreambuf_iterator<char>());
			const std::string portArg = "--port=" + std::to_string(tcpPort);
			if (cmd.find(portArg) != std::string::npos) {
				victims.push_back(pid);
			}
		}

		if (victims.empty()) {
			return;
		}

		for (const auto pid : victims) {
			terminateSfuProcess(pid);
		}

		usleep(100000);
	}
}

class TestSfuProcess {
public:
	bool start(int port, const std::vector<std::string>& extraArgs = {}, const std::string& logPath = "/dev/null") {
		stop();
		port_ = port;
		logPath_ = logPath;
		webRtcServerPort_ = testWebRtcServerPortForSignalingPort(port_);

		cleanupFixedTestPortProcesses(port_);
		if (!waitForTcpPortFree(port_)) return false;
		if (webRtcServerPort_ <= 0 || !isUdpPortBindable(webRtcServerPort_)) return false;
		if (!ensureTestSignalingTlsFiles()) return false;

		std::vector<std::string> args = {
			testSfuBinaryPath(),
			"--nodaemon",
			"--port=" + std::to_string(port_),
			"--webRtcServerPort=" + std::to_string(webRtcServerPort_),
			"--workers=1",
			"--workerBin=" + testWorkerBinaryPath(),
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
			dprintf(STDERR_FILENO, "TestSfuProcess execv failed: path=%s errno=%d (%s)\n",
				argv[0],
				errno,
				strerror(errno));
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
		if (pid_ <= 0 && port_ <= 0 && webRtcServerPort_ <= 0) return true;

		const pid_t pid = pid_;
		const int port = port_;
		const int webRtcServerPort = webRtcServerPort_;
		pid_ = -1;
		port_ = -1;
		webRtcServerPort_ = -1;

		if (pid > 0) terminateSfuProcess(pid, 40, sleepUs);
		const bool tcpFree = port > 0 ? waitForTcpPortFree(port, portReleasePolls, sleepUs) : true;
		const bool udpFree = webRtcServerPort > 0 ? waitForUdpPortFree(webRtcServerPort, portReleasePolls, sleepUs) : true;
		return tcpFree && udpFree;
	}

	~TestSfuProcess() { stop(); }

	pid_t pid() const { return pid_; }
	int port() const { return port_; }
	int webRtcServerPort() const { return webRtcServerPort_; }
	const std::string& logPath() const { return logPath_; }

private:
	pid_t pid_ = -1;
	int port_ = -1;
	int webRtcServerPort_ = -1;
	std::string logPath_;
};

inline std::string makeTestSfuLogPath(const std::string& prefix, int port) {
	return "/tmp/" + prefix + "_" + std::to_string(getpid()) + "_" + std::to_string(port) + ".log";
}
