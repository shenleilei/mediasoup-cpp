#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>

namespace mediasoup {

class Logger {
public:
	static spdlog::level::level_enum ParseLevel(const std::string& level) {
		if (level == "trace") return spdlog::level::trace;
		if (level == "debug") return spdlog::level::debug;
		if (level == "info") return spdlog::level::info;
		if (level == "warn" || level == "warning") return spdlog::level::warn;
		if (level == "error" || level == "err") return spdlog::level::err;
		if (level == "critical") return spdlog::level::critical;
		if (level == "off") return spdlog::level::off;
		return spdlog::level::info;
	}

	static void Init(const std::string& logFile = "", const std::string& level = "info") {
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		if (!logFile.empty()) {
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
			sinks.push_back(fileSink);
		}
		auto defaultLogger = std::make_shared<spdlog::logger>("", sinks.begin(), sinks.end());
		defaultLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v");
		level_ = ParseLevel(level);
		defaultLogger->set_level(level_);
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
			logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%s:%#] %v");
			logger->set_level(level_);
			try { spdlog::register_logger(logger); } catch (...) {}
		}
		return logger;
	}

private:
	static inline std::vector<spdlog::sink_ptr> sinks_;
	static inline spdlog::level::level_enum level_ = spdlog::level::info;
};

// Log macros with source location (file:line)
#define MS_DEBUG(logger, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::debug, __VA_ARGS__)
#define MS_INFO(logger, ...)  (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::info, __VA_ARGS__)
#define MS_WARN(logger, ...)  (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::warn, __VA_ARGS__)
#define MS_ERROR(logger, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::err, __VA_ARGS__)

} // namespace mediasoup
