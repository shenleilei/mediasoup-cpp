#include "common/RuntimeLogHelpers.h"

#include <chrono>
#include <filesystem>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace webrtc_qos_plain {

std::shared_ptr<spdlog::logger> CreateClientLogger(
	const std::string& name,
	const std::string& logDir)
{
	if (auto existing = spdlog::get(name)) return existing;

	std::vector<spdlog::sink_ptr> sinks;
	sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
	if (!logDir.empty()) {
		std::filesystem::create_directories(logDir);
		sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
			logDir + "/" + name + ".log",
			true));
	}
	auto logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
	logger->set_level(spdlog::level::info);
	logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%n] %v");
	spdlog::register_logger(logger);
	return logger;
}

const char* StatusCodeName(webrtc_qos::StatusCode code)
{
	switch (code) {
		case webrtc_qos::StatusCode::kOk:
			return "ok";
		case webrtc_qos::StatusCode::kInvalidArgument:
			return "invalid_argument";
		case webrtc_qos::StatusCode::kUnsupported:
			return "unsupported";
		case webrtc_qos::StatusCode::kMalformedPacket:
			return "malformed_packet";
		case webrtc_qos::StatusCode::kQueueFull:
			return "queue_full";
		case webrtc_qos::StatusCode::kInternalError:
			return "internal_error";
	}
	return "unknown";
}

std::string StatusToString(const webrtc_qos::Status& status)
{
	if (status) return "ok";
	return std::string(StatusCodeName(status.code)) + ": " + status.message;
}

int64_t MonotonicNowUs()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

} // namespace webrtc_qos_plain
