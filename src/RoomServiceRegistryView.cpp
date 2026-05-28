#include "RoomService.h"

#include "RoomStatsQosHelpers.h"

namespace mediasoup {

json RoomService::resolveRoom(const std::string& roomId, const std::string& clientIp)
{
	(void)roomId;
	(void)clientIp;
	return {{"wsUrl", ""}, {"isNew", true}};
}

json RoomService::getNodeLoad() const
{
	return {
		{"rooms", roomManager_.roomCount()},
		{"maxRooms", roomManager_.workerManager().maxTotalRouters()}
	};
}

json RoomService::getDefaultQosPolicy() const
{
	return roomstatsqos::BuildDefaultQosPolicy();
}

} // namespace mediasoup
