#include "RoomRecordingHelpers.h"
#include "RoomStatsQosHelpers.h"
#include <algorithm>
#include <chrono>
#include <filesystem>

namespace mediasoup::roomrecording {
namespace {

struct RecorderMediaConfig {
	uint8_t audioPayloadType{ 100 };
	uint8_t videoPayloadType{ 101 };
	uint32_t audioClockRate{ 48000 };
	uint32_t videoClockRate{ 90000 };
	VideoCodec videoCodec{ VideoCodec::VP8 };
};

struct RecordingDirEntry {
	std::string path;
	time_t mtime{ 0 };
	uint64_t size{ 0 };
};

RecorderMediaConfig ResolveRecorderMediaConfig(const std::shared_ptr<Room>& room)
{
	RecorderMediaConfig config;
	if (!room || !room->router()) {
		return config;
	}

	auto capabilities = room->router()->rtpCapabilities();
	for (const auto& codec : capabilities.codecs) {
		if (codec.mimeType.find("/rtx") != std::string::npos) {
			continue;
		}
		if (codec.mimeType.find("opus") != std::string::npos) {
			config.audioPayloadType = codec.preferredPayloadType;
			config.audioClockRate = codec.clockRate;
		} else if (codec.mimeType.find("H264") != std::string::npos ||
			codec.mimeType.find("h264") != std::string::npos) {
			if (config.videoCodec != VideoCodec::H264) {
				config.videoPayloadType = codec.preferredPayloadType;
				config.videoClockRate = codec.clockRate;
				config.videoCodec = VideoCodec::H264;
			}
		} else if (codec.mimeType.find("VP8") != std::string::npos) {
			if (config.videoCodec != VideoCodec::H264) {
				config.videoPayloadType = codec.preferredPayloadType;
				config.videoClockRate = codec.clockRate;
				config.videoCodec = VideoCodec::VP8;
			}
		}
	}

	return config;
}

std::string BuildRecordingOutputPath(
	const std::string& recordDir,
	const std::string& roomId,
	const std::string& peerId)
{
	std::error_code ec;
	const auto roomPath = std::filesystem::path(recordDir) / roomId;
	std::filesystem::create_directories(roomPath, ec);

	auto now = std::chrono::system_clock::now().time_since_epoch();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
	return (roomPath / (peerId + "_" + std::to_string(ms) + ".webm")).string();
}

bool HasActiveRecorderForDir(const RecorderMap& recorders, const std::string& roomId)
{
	const std::string prefix = roomId + "/";
	for (const auto& [key, _] : recorders) {
		if (key.compare(0, prefix.size(), prefix) == 0) {
			return true;
		}
	}
	return false;
}

void StopRecorder(
	const std::string& context,
	const std::shared_ptr<PeerRecorder>& recorder,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (!recorder) {
		return;
	}

	try {
		recorder->stop();
	} catch (const std::exception& e) {
		MS_WARN(logger, "{} recorder stop failed: {}", context, e.what());
	} catch (...) {
		MS_WARN(logger, "{} recorder stop failed: unknown error", context);
	}
}

} // namespace

void RemovePeerRecorder(
	const std::string& key,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger)
{
	std::shared_ptr<PeerRecorder> recorderToStop;
	auto it = recorders.find(key);
	if (it != recorders.end()) {
		recorderToStop = std::move(it->second);
		recorders.erase(it);
	}
	recorderTransports.erase(key);
	StopRecorder("peer recorder cleanup [" + key + "]", recorderToStop, logger);
}

void CleanupRoomRecorders(
	const std::string& roomId,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger)
{
	std::vector<std::shared_ptr<PeerRecorder>> recordersToStop;
	const std::string prefix = roomId + "/";

	for (auto it = recorders.begin(); it != recorders.end(); ) {
		if (it->first.compare(0, prefix.size(), prefix) == 0) {
			if (it->second) {
				recordersToStop.push_back(it->second);
			}
			recorderTransports.erase(it->first);
			it = recorders.erase(it);
		} else {
			++it;
		}
	}

	for (const auto& recorder : recordersToStop) {
		StopRecorder("cleanupRoomResources [" + roomId + "]", recorder, logger);
	}
}

void CleanOldRecordingDirs(
	const std::string& recordDir,
	uint64_t maxBytes,
	const RecorderMap& recorders,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (recordDir.empty()) {
		return;
	}

	std::vector<RecordingDirEntry> dirs;
	uint64_t totalSize = 0;
	std::error_code ec;
	if (!std::filesystem::exists(recordDir, ec) ||
		!std::filesystem::is_directory(recordDir, ec)) {
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(recordDir, ec)) {
		if (ec) {
			return;
		}
		if (!entry.is_directory(ec) || ec) {
			continue;
		}

		uint64_t dirSize = 0;
		time_t newest = 0;
		for (const auto& fileEntry : std::filesystem::directory_iterator(entry.path(), ec)) {
			if (ec) {
				break;
			}
			std::error_code fileEc;
			if (!fileEntry.is_regular_file(fileEc) || fileEc) {
				continue;
			}
			auto fileTime = std::filesystem::last_write_time(fileEntry.path(), fileEc);
			auto fileSize = fileEntry.file_size(fileEc);
			if (fileEc) {
				continue;
			}
			dirSize += fileSize;
			auto secs = std::chrono::time_point_cast<std::chrono::seconds>(fileTime).time_since_epoch().count();
			newest = std::max<time_t>(newest, static_cast<time_t>(secs));
		}

		totalSize += dirSize;
		dirs.push_back({ entry.path().string(), newest, dirSize });
	}

	if (totalSize <= maxBytes) {
		return;
	}

	std::sort(dirs.begin(), dirs.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.mtime < rhs.mtime;
	});

	for (const auto& dir : dirs) {
		if (totalSize <= maxBytes) {
			break;
		}

		const auto roomId = std::filesystem::path(dir.path).filename().string();
		if (HasActiveRecorderForDir(recorders, roomId)) {
			continue;
		}

		for (const auto& fileEntry : std::filesystem::directory_iterator(dir.path, ec)) {
			if (ec) {
				break;
			}
			std::error_code fileEc;
			std::filesystem::remove(fileEntry.path(), fileEc);
		}
		std::filesystem::remove(dir.path, ec);
		totalSize -= dir.size;
		MS_DEBUG(logger, "Cleaned old recording dir: {} (freed {} bytes)", dir.path, dir.size);
	}
}

void AutoRecordProducer(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Room>& room,
	const std::shared_ptr<Producer>& producer,
	const std::string& recordDir,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (recordDir.empty() || !room || !producer) {
		return;
	}

	const std::string key = roomstatsqos::MakePeerKey(roomId, peerId);
	auto& recorder = recorders[key];
	if (!recorder) {
		auto mediaConfig = ResolveRecorderMediaConfig(room);
		auto outputPath = BuildRecordingOutputPath(recordDir, roomId, peerId);

		recorder = std::make_shared<PeerRecorder>(
			peerId,
			outputPath,
			mediaConfig.audioPayloadType,
			mediaConfig.videoPayloadType,
			mediaConfig.audioClockRate,
			mediaConfig.videoClockRate,
			mediaConfig.videoCodec,
			roomId);
		int port = recorder->createSocket();
		if (port < 0) {
			MS_ERROR(logger, "[{}] Failed to create recorder socket", key);
			recorder.reset();
			return;
		}
		if (!recorder->start()) {
			MS_ERROR(logger, "[{}] Failed to start recorder", key);
			recorder.reset();
			return;
		}

		PlainTransportOptions options;
		options.listenInfos = {{{"ip", "127.0.0.1"}}};
		options.rtcpMux = true;
		options.comedia = false;
		auto plainTransport = room->router()->createPlainTransport(options);
		recorderTransports[key] = plainTransport;
		plainTransport->connect("127.0.0.1", port);
		MS_DEBUG(logger, "[{}] Recorder started → port {} file {}", key, port, outputPath);
	}

	auto transportIt = recorderTransports.find(key);
	if (transportIt == recorderTransports.end() || !transportIt->second) {
		return;
	}

	try {
		json consumeOpts = {
			{"producerId", producer->id()},
			{"rtpCapabilities", room->router()->rtpCapabilities()},
			{"consumableRtpParameters", producer->consumableRtpParameters()},
			{"pipe", true}
		};
		transportIt->second->consume(consumeOpts);
		MS_DEBUG(logger, "[{}] Recording consumer created: producer {} kind={}",
			key, producer->id(), producer->kind());
	} catch (const std::exception& e) {
		MS_ERROR(logger, "[{}] Failed to create recording consumer: {}", key, e.what());
	}
}

} // namespace mediasoup::roomrecording
