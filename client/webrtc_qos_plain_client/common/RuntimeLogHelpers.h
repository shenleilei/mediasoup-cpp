#pragma once

#include <memory>
#include <string>

#include <spdlog/logger.h>

#include "webrtc_qos/status.h"

namespace webrtc_qos_plain {

std::shared_ptr<spdlog::logger> CreateClientLogger(
	const std::string& name,
	const std::string& logDir);

const char* StatusCodeName(webrtc_qos::StatusCode code);
std::string StatusToString(const webrtc_qos::Status& status);
int64_t MonotonicNowUs();

} // namespace webrtc_qos_plain
