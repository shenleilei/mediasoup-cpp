#include "RoomService.h"

namespace mediasoup {

RoomService::RoomService(RoomManager& roomManager, RoomRegistry* registry)
	: roomManager_(roomManager), registry_(registry)
	, logger_(Logger::Get("RoomService")) {}

} // namespace mediasoup
