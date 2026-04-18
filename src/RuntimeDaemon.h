#pragma once

#include <string>

namespace mediasoup {

int DaemonizeProcess(
	const std::string& logDir,
	const std::string& logPrefix,
	int logRotateHours,
	const std::string& pidFile);

void NotifyDaemonStartupStatus(bool ok);

} // namespace mediasoup
