#pragma once

#include <fstream>
#include <iterator>
#include <signal.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

inline bool isSfuProcessAlive(pid_t pid) {
	if (pid <= 0 || kill(pid, 0) != 0) return false;

	std::ifstream cmdline("/proc/" + std::to_string(pid) + "/cmdline", std::ios::binary);
	if (!cmdline.is_open()) return false;

	std::string cmd((std::istreambuf_iterator<char>(cmdline)), std::istreambuf_iterator<char>());
	return cmd.find("mediasoup-sfu") != std::string::npos;
}

inline void terminateSfuProcess(pid_t pid, int gracefulPolls = 40, int sleepUs = 50000) {
	if (!isSfuProcessAlive(pid)) return;

	kill(pid, SIGTERM);
	for (int i = 0; i < gracefulPolls; ++i) {
		usleep(sleepUs);
		if (!isSfuProcessAlive(pid)) return;
	}

	if (!isSfuProcessAlive(pid)) return;
	kill(pid, SIGKILL);
	for (int i = 0; i < 10; ++i) {
		usleep(sleepUs);
		if (!isSfuProcessAlive(pid)) return;
	}
}
