#include "common/ClientArgs.h"

#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace webrtc_qos_plain {
namespace {

using OptionMap = std::unordered_map<std::string, std::string>;

bool ParseOptionMap(int argc, char* argv[], OptionMap* values, std::string* error)
{
	for (int i = 1; i < argc; ++i) {
		std::string key = argv[i];
		if (key == "--help" || key == "-h") {
			(*values)[key] = "1";
			continue;
		}
		if (key.rfind("--", 0) != 0) {
			if (error) *error = "unexpected positional argument: " + key;
			return false;
		}

		auto eq = key.find('=');
		if (eq != std::string::npos) {
			const auto option = key.substr(0, eq);
			const auto value = key.substr(eq + 1);
			if (option == "--loop-input" || option == "--input-decode-loop" || option == "--output-null" || option == "--enable-audio" || option == "--decode-qoe") {
				(*values)[option] = value.empty() ? "1" : value;
			} else {
				(*values)[option] = value;
			}
			continue;
		}

		if (key == "--loop-input" || key == "--input-decode-loop" || key == "--output-null" || key == "--enable-audio" || key == "--decode-qoe") {
			(*values)[key] = "1";
			continue;
		}
		if (i + 1 >= argc) {
			if (error) *error = "missing value for " + key;
			return false;
		}
		(*values)[key] = argv[++i];
	}
	return true;
}

bool Has(const OptionMap& values, const std::string& key)
{
	return values.find(key) != values.end();
}

bool GetBoolFlag(const OptionMap& values, const std::string& key)
{
	auto it = values.find(key);
	if (it == values.end()) return false;
	if (it->second == "1" || it->second == "true" || it->second == "TRUE" || it->second == "yes")
		return true;
	if (it->second == "0" || it->second == "false" || it->second == "FALSE" || it->second == "no")
		return false;
	throw std::invalid_argument("invalid boolean for " + key + ": " + it->second);
}

std::string GetString(const OptionMap& values, const std::string& key, const std::string& fallback)
{
	auto it = values.find(key);
	return it == values.end() ? fallback : it->second;
}

uint32_t ParseU32(const std::string& value, const std::string& key)
{
	char* end = nullptr;
	const auto parsed = std::strtoul(value.c_str(), &end, 10);
	if (!end || *end != '\0' || parsed > 0xfffffffful)
		throw std::invalid_argument("invalid uint32 for " + key + ": " + value);
	return static_cast<uint32_t>(parsed);
}

uint16_t ParseU16(const std::string& value, const std::string& key)
{
	const uint32_t parsed = ParseU32(value, key);
	if (parsed > 65535u)
		throw std::invalid_argument("invalid port for " + key + ": " + value);
	return static_cast<uint16_t>(parsed);
}

int ParseInt(const std::string& value, const std::string& key)
{
	char* end = nullptr;
	const long parsed = std::strtol(value.c_str(), &end, 10);
	if (!end || *end != '\0')
		throw std::invalid_argument("invalid integer for " + key + ": " + value);
	return static_cast<int>(parsed);
}

bool ValidateTick(int tickMs, std::string* error)
{
	if (tickMs < 5 || tickMs > 20) {
		if (error) *error = "--process-tick-ms must be in [5,20]";
		return false;
	}
	return true;
}

} // namespace

bool ParsePushOptions(int argc, char* argv[], PushOptions* options, std::string* error)
{
	if (!options) return false;
	OptionMap values;
	if (!ParseOptionMap(argc, argv, &values, error)) return false;
	if (Has(values, "--help") || Has(values, "-h")) {
		if (error) *error = PushUsage();
		return false;
	}

	try {
		options->serverIp = GetString(values, "--server-ip", options->serverIp);
		options->serverPort = ParseInt(GetString(values, "--server-port", std::to_string(options->serverPort)), "--server-port");
		options->room = GetString(values, "--room", options->room);
		options->peer = GetString(values, "--peer", options->peer);
		options->input = GetString(values, "--input", options->input);
		options->logDir = GetString(values, "--log-dir", options->logDir);
		options->mediaRemoteIp = GetString(values, "--media-remote-ip", options->mediaRemoteIp);
		options->videoSsrc = ParseU32(GetString(values, "--video-ssrc", std::to_string(options->videoSsrc)), "--video-ssrc");
		options->audioSsrc = ParseU32(GetString(values, "--audio-ssrc", std::to_string(options->audioSsrc)), "--audio-ssrc");
		options->enableAudio = GetBoolFlag(values, "--enable-audio");
		options->startBitrateBps = ParseU32(GetString(values, "--start-bitrate", std::to_string(options->startBitrateBps)), "--start-bitrate");
		options->minBitrateBps = ParseU32(GetString(values, "--min-bitrate", std::to_string(options->minBitrateBps)), "--min-bitrate");
		options->maxBitrateBps = ParseU32(GetString(values, "--max-bitrate", std::to_string(options->maxBitrateBps)), "--max-bitrate");
		options->processTickMs = ParseInt(GetString(values, "--process-tick-ms", std::to_string(options->processTickMs)), "--process-tick-ms");
		options->loopInput = GetBoolFlag(values, "--loop-input");
		options->inputSynthetic = GetBoolFlag(values, "--input-synthetic");
		options->inputDecodeLoop = GetBoolFlag(values, "--input-decode-loop");
		options->inputV4L2 = GetString(values, "--input-v4l2", options->inputV4L2);
		options->encoder = GetString(values, "--encoder", options->encoder);
		options->syntheticWidth = ParseInt(GetString(values, "--synthetic-width", std::to_string(options->syntheticWidth)), "--synthetic-width");
		options->syntheticHeight = ParseInt(GetString(values, "--synthetic-height", std::to_string(options->syntheticHeight)), "--synthetic-height");
		options->syntheticFps = ParseInt(GetString(values, "--synthetic-fps", std::to_string(options->syntheticFps)), "--synthetic-fps");
		options->syntheticPattern = GetString(values, "--synthetic-pattern", options->syntheticPattern);
		options->v4l2Width = ParseInt(GetString(values, "--v4l2-width", std::to_string(options->v4l2Width)), "--v4l2-width");
		options->v4l2Height = ParseInt(GetString(values, "--v4l2-height", std::to_string(options->v4l2Height)), "--v4l2-height");
		options->v4l2Fps = ParseInt(GetString(values, "--v4l2-fps", std::to_string(options->v4l2Fps)), "--v4l2-fps");
		options->v4l2InputFormat = GetString(values, "--v4l2-input-format", options->v4l2InputFormat);
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		return false;
	}

	const bool inputV4L2 = !options->inputV4L2.empty();
	if (!options->inputSynthetic && !inputV4L2 && options->input.empty()) {
		if (error) *error = "--input is required";
		return false;
	}
	const int selectedInputs =
		(options->inputSynthetic ? 1 : 0) +
		(options->inputDecodeLoop ? 1 : 0) +
		(inputV4L2 ? 1 : 0);
	if (selectedInputs > 1) {
		if (error) *error = "--input-synthetic, --input-decode-loop, and --input-v4l2 are mutually exclusive";
		return false;
	}
	if (options->inputSynthetic && options->encoder != "x264") {
		if (error) *error = "--input-synthetic requires --encoder x264";
		return false;
	}
	if (options->inputDecodeLoop && options->encoder != "x264") {
		if (error) *error = "--input-decode-loop requires --encoder x264";
		return false;
	}
	if (inputV4L2 && options->encoder != "x264") {
		if (error) *error = "--input-v4l2 requires --encoder x264";
		return false;
	}
	if (!options->inputSynthetic && !options->inputDecodeLoop && !inputV4L2 && options->encoder != "copy") {
		if (error) *error = "--encoder x264 currently requires --input-synthetic, --input-decode-loop, or --input-v4l2";
		return false;
	}
	if (options->syntheticWidth < 16 || options->syntheticHeight < 16 ||
		options->syntheticWidth > 1920 || options->syntheticHeight > 1080) {
		if (error) *error = "--synthetic-width/height must be in [16,1920]x[16,1080]";
		return false;
	}
	if (options->syntheticFps < 1 || options->syntheticFps > 60) {
		if (error) *error = "--synthetic-fps must be in [1,60]";
		return false;
	}
	if (options->v4l2Width < 16 || options->v4l2Height < 16 ||
		options->v4l2Width > 1920 || options->v4l2Height > 1080) {
		if (error) *error = "--v4l2-width/height must be in [16,1920]x[16,1080]";
		return false;
	}
	if (options->v4l2Fps < 1 || options->v4l2Fps > 60) {
		if (error) *error = "--v4l2-fps must be in [1,60]";
		return false;
	}
	if (options->videoSsrc == 0) {
		if (error) *error = "--video-ssrc must be non-zero";
		return false;
	}
	if (options->enableAudio && options->audioSsrc == 0) {
		if (error) *error = "--audio-ssrc must be non-zero when --enable-audio=true";
		return false;
	}
	if (!ValidateTick(options->processTickMs, error)) return false;
	if (options->mediaRemoteIp.empty()) options->mediaRemoteIp = options->serverIp;
	return true;
}

bool ParsePlayOptions(int argc, char* argv[], PlayOptions* options, std::string* error)
{
	if (!options) return false;
	OptionMap values;
	if (!ParseOptionMap(argc, argv, &values, error)) return false;
	if (Has(values, "--help") || Has(values, "-h")) {
		if (error) *error = PlayUsage();
		return false;
	}

	try {
		options->serverIp = GetString(values, "--server-ip", options->serverIp);
		options->serverPort = ParseInt(GetString(values, "--server-port", std::to_string(options->serverPort)), "--server-port");
		options->room = GetString(values, "--room", options->room);
		options->peer = GetString(values, "--peer", options->peer);
		options->listenIp = GetString(values, "--listen-ip", options->listenIp);
		options->advertiseIp = GetString(values, "--advertise-ip", options->advertiseIp);
		options->listenPort = ParseU16(GetString(values, "--listen-port", std::to_string(options->listenPort)), "--listen-port");
		options->outputAu = GetString(values, "--output-au", options->outputAu);
		options->outputNull = GetBoolFlag(values, "--output-null");
		options->logDir = GetString(values, "--log-dir", options->logDir);
		options->producerId = GetString(values, "--producer-id", options->producerId);
		options->producerPeerId = GetString(values, "--producer-peer-id", options->producerPeerId);
		options->receiverId = ParseU32(GetString(values, "--receiver-id", std::to_string(options->receiverId)), "--receiver-id");
		options->startBitrateBps = ParseU32(GetString(values, "--start-bitrate", std::to_string(options->startBitrateBps)), "--start-bitrate");
		options->minBitrateBps = ParseU32(GetString(values, "--min-bitrate", std::to_string(options->minBitrateBps)), "--min-bitrate");
		options->maxBitrateBps = ParseU32(GetString(values, "--max-bitrate", std::to_string(options->maxBitrateBps)), "--max-bitrate");
		options->processTickMs = ParseInt(GetString(values, "--process-tick-ms", std::to_string(options->processTickMs)), "--process-tick-ms");
		options->mediaRemoteIp = GetString(values, "--media-remote-ip", options->mediaRemoteIp);
		options->waitConsumerTimeoutMs = ParseInt(GetString(values, "--wait-consumer-timeout-ms", std::to_string(options->waitConsumerTimeoutMs)), "--wait-consumer-timeout-ms");
		options->decodeQoe = GetBoolFlag(values, "--decode-qoe");
	} catch (const std::exception& e) {
		if (error) *error = e.what();
		return false;
	}

	if (!options->outputNull && options->outputAu.empty()) {
		if (error) *error = "either --output-au or --output-null is required";
		return false;
	}
	if (options->advertiseIp.empty()) {
		if (options->listenIp == "0.0.0.0") {
			if (error) *error = "--advertise-ip is required when --listen-ip=0.0.0.0";
			return false;
		}
		options->advertiseIp = options->listenIp;
	}
	if (!ValidateTick(options->processTickMs, error)) return false;
	if (options->mediaRemoteIp.empty()) options->mediaRemoteIp = options->serverIp;
	return true;
}

std::string PushUsage()
{
	return
		"webrtc-qos-plain-push-client --server-ip <ip> --server-port <port> "
		"--room <room> --peer <peer> (--input <h264.mp4>|--input <mp4> --input-decode-loop --encoder x264|--input-synthetic --encoder x264|--input-v4l2 </dev/videoN> --encoder x264) [--loop-input] "
		"[--video-ssrc <u32>] [--enable-audio] [--audio-ssrc <u32>] [--media-remote-ip <ip>] "
		"[--synthetic-width <px>] [--synthetic-height <px>] [--synthetic-fps <n>] "
		"[--v4l2-width <px>] [--v4l2-height <px>] [--v4l2-fps <n>] [--v4l2-input-format <fmt>] [--log-dir <dir>]";
}

std::string PlayUsage()
{
	return
		"webrtc-qos-plain-play-client --server-ip <ip> --server-port <port> "
		"--room <room> --peer <peer> --listen-ip <ip> --advertise-ip <ip> "
		"--listen-port <port> (--output-au <file>|--output-null) "
		"[--producer-id <id>] [--producer-peer-id <peer>] [--media-remote-ip <ip>] "
		"[--wait-consumer-timeout-ms <ms>] [--decode-qoe] [--log-dir <dir>]";
}

} // namespace webrtc_qos_plain
