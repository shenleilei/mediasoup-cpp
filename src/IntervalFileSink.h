#pragma once

#include <spdlog/common.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/os.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <string>

namespace mediasoup {
namespace logging {

using log_clock = spdlog::log_clock;

inline std::tm bucket_tm(log_clock::time_point tp, int rotateHours) {
	auto time = log_clock::to_time_t(tp);
	auto tm = spdlog::details::os::localtime(time);
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;
	if (rotateHours > 0) {
		tm.tm_hour = (tm.tm_hour / rotateHours) * rotateHours;
	}
	return tm;
}

inline spdlog::filename_t build_bucketed_log_filename(
	const spdlog::filename_t& logDir,
	const std::string& logPrefix,
	int pid,
	log_clock::time_point tp,
	int rotateHours)
{
	auto tm = bucket_tm(tp, rotateHours);
	auto filename = fmt::format(
		SPDLOG_FILENAME_T("{}_{:04d}{:02d}{:02d}{:02d}_{}.log"),
		logPrefix,
		tm.tm_year + 1900,
		tm.tm_mon + 1,
		tm.tm_mday,
		tm.tm_hour,
		pid);

	if (logDir.empty()) {
		return filename;
	}

	return (std::filesystem::path(logDir) / filename).string();
}

template <typename Mutex>
class interval_file_sink final : public spdlog::sinks::base_sink<Mutex> {
public:
	explicit interval_file_sink(
		spdlog::filename_t logDir,
		std::string logPrefix,
		int pid,
		int rotateHours = 3,
		bool truncate = false,
		const spdlog::file_event_handlers& eventHandlers = {})
		: logDir_(std::move(logDir))
		, logPrefix_(std::move(logPrefix))
		, pid_(pid)
		, rotateHours_(rotateHours)
		, rotateEnabled_(rotateHours_ > 0)
		, fileHelper_(eventHandlers)
		, truncate_(truncate)
	{
		const auto now = spdlog::details::os::now();
		currentFilename_ = build_bucketed_log_filename(logDir_, logPrefix_, pid_, now, rotateHours_);
		fileHelper_.open(currentFilename_, truncate_);
		if (rotateEnabled_) {
			currentPeriodStart_ = period_start_(now);
			rotationTp_ = currentPeriodStart_ + std::chrono::hours(rotateHours_);
		}
	}

	spdlog::filename_t filename() {
		std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
		return fileHelper_.filename();
	}

protected:
	void sink_it_(const spdlog::details::log_msg& msg) override {
		if (rotateEnabled_ && msg.time >= rotationTp_) {
			rotate_(msg.time);
		}

		spdlog::memory_buf_t formatted;
		spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
		fileHelper_.write(formatted);
	}

	void flush_() override {
		fileHelper_.flush();
	}

private:
	log_clock::time_point period_start_(log_clock::time_point tp) const {
		auto tm = bucket_tm(tp, rotateHours_);
		return log_clock::from_time_t(std::mktime(&tm));
	}

	void rotate_(log_clock::time_point now) {
		fileHelper_.flush();
		currentFilename_ = build_bucketed_log_filename(logDir_, logPrefix_, pid_, now, rotateHours_);
		fileHelper_.open(currentFilename_, truncate_);
		currentPeriodStart_ = period_start_(now);
		rotationTp_ = currentPeriodStart_ + std::chrono::hours(rotateHours_);
	}

	spdlog::filename_t logDir_;
	std::string logPrefix_;
	int pid_ = 0;
	int rotateHours_ = 0;
	bool rotateEnabled_ = false;
	log_clock::time_point currentPeriodStart_{};
	log_clock::time_point rotationTp_{};
	spdlog::filename_t currentFilename_;
	spdlog::details::file_helper fileHelper_;
	bool truncate_ = false;
};

using interval_file_sink_mt = interval_file_sink<std::mutex>;
using interval_file_sink_st = interval_file_sink<spdlog::details::null_mutex>;

} // namespace logging
} // namespace mediasoup
