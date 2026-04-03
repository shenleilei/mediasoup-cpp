#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

namespace mediasoup {

class Logger {
public:
	static void Init() {
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
		spdlog::set_level(spdlog::level::debug);
	}

	static std::shared_ptr<spdlog::logger> Get(const std::string& name) {
		auto logger = spdlog::get(name);
		if (!logger) {
			logger = spdlog::stdout_color_mt(name);
		}
		return logger;
	}
};

#define MS_LOG(logger, level, ...) (logger)->level(__VA_ARGS__)
#define MS_DEBUG(logger, ...) MS_LOG(logger, debug, __VA_ARGS__)
#define MS_WARN(logger, ...) MS_LOG(logger, warn, __VA_ARGS__)
#define MS_ERROR(logger, ...) MS_LOG(logger, error, __VA_ARGS__)

} // namespace mediasoup
