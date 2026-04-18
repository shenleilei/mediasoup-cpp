#include "RoomService.h"

namespace mediasoup {

RoomService::RoomService(RoomManager& roomManager, RoomRegistry* registry,
	const std::string& recordDir)
	: roomManager_(roomManager), registry_(registry)
	, recordDir_(recordDir)
	, logger_(Logger::Get("RoomService")) {}

} // namespace mediasoup
