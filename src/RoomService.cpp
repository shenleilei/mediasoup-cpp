#include "RoomService.h"

namespace mediasoup {

RoomService::RoomService(RoomManager& roomManager)
	: roomManager_(roomManager)
	, logger_(Logger::Get("RoomService")) {}

} // namespace mediasoup
