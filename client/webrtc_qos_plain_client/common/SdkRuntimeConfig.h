#pragma once

#include <string>
#include <type_traits>
#include <utility>

namespace webrtc_qos_plain {
namespace detail {

template<typename T, typename = void>
struct HasSdkLogging : std::false_type {};

template<typename T>
struct HasSdkLogging<T, std::void_t<decltype(std::declval<T&>().logging.file.enabled)>>
	: std::true_type {};

template<typename T, typename = void>
struct HasSdkMetrics : std::false_type {};

template<typename T>
struct HasSdkMetrics<T, std::void_t<decltype(std::declval<T&>().metrics.file.enabled)>>
	: std::true_type {};

template<typename T, typename = void>
struct HasSdkAlerts : std::false_type {};

template<typename T>
struct HasSdkAlerts<T, std::void_t<decltype(std::declval<T&>().alerts.file.enabled)>>
	: std::true_type {};

template<typename Config>
void ConfigureSdkLogging(Config& config, const std::string& role, const std::string& logDir, std::true_type)
{
	config.logging.file.enabled = true;
	config.logging.file.directory = logDir;
	config.logging.file.basename = role;
	config.logging.file.json_lines = true;
}

template<typename Config>
void ConfigureSdkLogging(Config&, const std::string&, const std::string&, std::false_type)
{
}

template<typename Config>
void ConfigureSdkMetrics(Config& config, const std::string& role, const std::string& logDir, std::true_type)
{
	config.metrics.file.enabled = true;
	config.metrics.file.directory = logDir;
	config.metrics.file.basename = role + "_metrics";
	config.metrics.interval_ms = 1000;
	config.metrics.include_track_snapshots = true;
}

template<typename Config>
void ConfigureSdkMetrics(Config&, const std::string&, const std::string&, std::false_type)
{
}

template<typename Config>
void ConfigureSdkAlerts(Config& config, const std::string& role, const std::string& logDir, std::true_type)
{
	config.alerts.file.enabled = true;
	config.alerts.file.directory = logDir;
	config.alerts.file.basename = role + "_alerts";
	config.alerts.suppress_repeated_alerts_ms = 1000;
}

template<typename Config>
void ConfigureSdkAlerts(Config&, const std::string&, const std::string&, std::false_type)
{
}

} // namespace detail

template<typename Config>
bool ConfigureSdkRuntimeFiles(Config& config, const std::string& role, const std::string& logDir)
{
	detail::ConfigureSdkLogging(config, role, logDir, detail::HasSdkLogging<Config>{});
	detail::ConfigureSdkMetrics(config, role, logDir, detail::HasSdkMetrics<Config>{});
	detail::ConfigureSdkAlerts(config, role, logDir, detail::HasSdkAlerts<Config>{});
	return detail::HasSdkLogging<Config>::value &&
		detail::HasSdkMetrics<Config>::value &&
		detail::HasSdkAlerts<Config>::value;
}

} // namespace webrtc_qos_plain
