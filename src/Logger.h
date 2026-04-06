#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace mediasoup {

class Logger {
public:
	static void Init(const std::string& logFile = "") {
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		if (!logFile.empty()) {
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
			sinks.push_back(fileSink);
		}
		auto defaultLogger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
		defaultLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
		defaultLogger->set_level(spdlog::level::debug);
		spdlog::set_default_logger(defaultLogger);
		sinks_ = sinks;
	}

	static std::shared_ptr<spdlog::logger> Get(const std::string& name) {
		auto logger = spdlog::get(name);
		if (!logger) {
			if (!sinks_.empty())
				logger = std::make_shared<spdlog::logger>(name, sinks_.begin(), sinks_.end());
			else
				logger = spdlog::stdout_color_mt(name);
			logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
			logger->set_level(spdlog::level::debug);
			try { spdlog::register_logger(logger); } catch (...) {}
		}
		return logger;
	}

private:
	static inline std::vector<spdlog::sink_ptr> sinks_;
};

// Standard log macros
#define MS_LOG(logger, level, ...) (logger)->level(__VA_ARGS__)
#define MS_DEBUG(logger, ...) MS_LOG(logger, debug, __VA_ARGS__)
#define MS_WARN(logger, ...) MS_LOG(logger, warn, __VA_ARGS__)
#define MS_ERROR(logger, ...) MS_LOG(logger, error, __VA_ARGS__)

} // namespace mediasoup
