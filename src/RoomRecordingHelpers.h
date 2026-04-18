#pragma once

#include "Logger.h"
#include "PlainTransport.h"
#include "Producer.h"
#include "Recorder.h"
#include "RoomManager.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace mediasoup::roomrecording {

using RecorderMap = std::unordered_map<std::string, std::shared_ptr<PeerRecorder>>;
using RecorderTransportMap = std::unordered_map<std::string, std::shared_ptr<PlainTransport>>;

void RemovePeerRecorder(
	const std::string& key,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger);

void CleanupRoomRecorders(
	const std::string& roomId,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger);

void CleanOldRecordingDirs(
	const std::string& recordDir,
	uint64_t maxBytes,
	const RecorderMap& recorders,
	const std::shared_ptr<spdlog::logger>& logger);

void AutoRecordProducer(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Room>& room,
	const std::shared_ptr<Producer>& producer,
	const std::string& recordDir,
	RecorderMap& recorders,
	RecorderTransportMap& recorderTransports,
	const std::shared_ptr<spdlog::logger>& logger);

} // namespace mediasoup::roomrecording
