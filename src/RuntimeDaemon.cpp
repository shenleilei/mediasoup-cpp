#include "RuntimeDaemon.h"

#include "Logger.h"

#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace mediasoup {
namespace {

int g_daemonStartupFd = -1;

std::string CurrentLogPath(
	const std::string& logDir,
	const std::string& logPrefix,
	int rotateHours,
	pid_t pid)
{
	return logging::build_bucketed_log_filename(
		logDir, logPrefix, static_cast<int>(pid), spdlog::details::os::now(), rotateHours);
}

} // namespace

int DaemonizeProcess(
	const std::string& logDir,
	const std::string& logPrefix,
	int logRotateHours,
	const std::string& pidFile)
{
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
	setsid();
	if (!logDir.empty()) {
		std::string logPath = CurrentLogPath(logDir, logPrefix, logRotateHours, getpid());
		FILE* f = fopen(logPath.c_str(), "a");
		if (f) {
			dup2(fileno(f), STDOUT_FILENO);
			dup2(fileno(f), STDERR_FILENO);
			fclose(f);
		}
	}
	close(STDIN_FILENO);
	g_daemonStartupFd = startupPipe[1];
	return g_daemonStartupFd;
}

void NotifyDaemonStartupStatus(bool ok)
{
	if (g_daemonStartupFd < 0) return;
	char status = ok ? '1' : '0';
	(void)::write(g_daemonStartupFd, &status, 1);
	::close(g_daemonStartupFd);
	g_daemonStartupFd = -1;
}

} // namespace mediasoup
