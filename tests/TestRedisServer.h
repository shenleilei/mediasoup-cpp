#pragma once

#include <hiredis/hiredis.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

class TestRedisServer {
public:
	TestRedisServer() = default;
	~TestRedisServer() {
		stop();
		cleanupFiles();
	}

	TestRedisServer(const TestRedisServer&) = delete;
	TestRedisServer& operator=(const TestRedisServer&) = delete;

	bool start(int timeoutMs = 5000) {
		stop();
		cleanupFiles();
		lastError_.clear();

		char dirTemplate[] = "/tmp/mediasoup-test-redis-XXXXXX";
		char* createdDir = ::mkdtemp(dirTemplate);
		if (!createdDir) {
			lastError_ = std::string("mkdtemp failed: ") + std::strerror(errno);
			return false;
		}
		tempDir_ = createdDir;
		logPath_ = tempDir_ + "/redis.log";
		configPath_ = tempDir_ + "/redis.conf";

		port_ = pickFreePort();
		if (port_ <= 0) {
			lastError_ = "failed to allocate a free Redis port";
			return false;
		}

		{
			std::ofstream config(configPath_);
			if (!config.is_open()) {
				lastError_ = "failed to write redis config";
				return false;
			}
			config
				<< "bind 127.0.0.1\n"
				<< "port " << port_ << "\n"
				<< "save \"\"\n"
				<< "appendonly no\n"
				<< "daemonize no\n"
				<< "protected-mode no\n"
				<< "dir " << tempDir_ << "\n"
				<< "dbfilename dump.rdb\n"
				<< "pidfile " << tempDir_ << "/redis.pid\n"
				<< "logfile " << logPath_ << "\n";
		}

		pid_ = ::fork();
		if (pid_ < 0) {
			lastError_ = std::string("fork failed: ") + std::strerror(errno);
			pid_ = -1;
			return false;
		}
		if (pid_ == 0) {
			::execlp("redis-server", "redis-server", configPath_.c_str(), static_cast<char*>(nullptr));
			::_exit(127);
		}

		if (!waitUntilReady(timeoutMs)) {
			stop();
			return false;
		}

		return true;
	}

	void stop() {
		if (pid_ <= 0) {
			port_ = 0;
			return;
		}

		::kill(pid_, SIGTERM);
		for (int i = 0; i < 50; ++i) {
			if (!isRunning()) {
				pid_ = -1;
				port_ = 0;
				return;
			}
			::usleep(100000);
		}

		if (isRunning()) {
			::kill(pid_, SIGKILL);
			for (int i = 0; i < 10; ++i) {
				if (!isRunning()) break;
				::usleep(100000);
			}
		}

		int status = 0;
		(void)::waitpid(pid_, &status, WNOHANG);
		pid_ = -1;
		port_ = 0;
	}

	int port() const { return port_; }
	const std::string& logPath() const { return logPath_; }

	std::string redisArgs() const {
		return " --redisHost=127.0.0.1 --redisPort=" + std::to_string(port_);
	}

	std::string failureMessage() const {
		if (logPath_.empty()) return lastError_;
		return lastError_ + " (redis log: " + logPath_ + ")";
	}

private:
	static int pickFreePort() {
		int fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) return 0;

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
			::close(fd);
			return 0;
		}

		socklen_t len = sizeof(addr);
		if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
			::close(fd);
			return 0;
		}

		const int port = ntohs(addr.sin_port);
		::close(fd);
		return port;
	}

	bool waitUntilReady(int timeoutMs) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		while (std::chrono::steady_clock::now() < deadline) {
			if (!isRunning()) {
				lastError_ = "redis-server exited before becoming ready";
				return false;
			}

			timeval timeout{0, 200000};
			redisContext* ctx = redisConnectWithTimeout("127.0.0.1", port_, timeout);
			if (ctx && !ctx->err) {
				auto* reply = static_cast<redisReply*>(redisCommand(ctx, "PING"));
				const bool ready =
					reply &&
					reply->type == REDIS_REPLY_STATUS &&
					reply->str &&
					std::string(reply->str) == "PONG";
				if (reply) freeReplyObject(reply);
				redisFree(ctx);
				if (ready) return true;
			} else if (ctx) {
				redisFree(ctx);
			}

			::usleep(100000);
		}

		lastError_ = "redis-server did not become ready within timeout";
		return false;
	}

	bool isRunning() const {
		if (pid_ <= 0) return false;

		int status = 0;
		pid_t result = ::waitpid(pid_, &status, WNOHANG);
		return result == 0;
	}

	void cleanupFiles() {
		if (tempDir_.empty()) return;
		std::error_code ec;
		std::filesystem::remove_all(tempDir_, ec);
		tempDir_.clear();
		configPath_.clear();
		logPath_.clear();
	}

	pid_t pid_{-1};
	int port_{0};
	std::string tempDir_;
	std::string configPath_;
	std::string logPath_;
	std::string lastError_;
};
