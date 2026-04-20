#include "RoomService.h"

#include "RoomRegistry.h"
#include "RoomStatsQosHelpers.h"

namespace mediasoup {

void RoomService::heartbeatRegistry()
{
	if (!registry_) return;
	registry_->heartbeat();
	auto roomIds = roomManager_.getRoomIds();
	if (!roomIds.empty())
		registry_->refreshRooms(roomIds);
	registry_->updateLoad(
		roomManager_.roomCount(),
		roomManager_.workerManager().maxTotalRouters());
}

json RoomService::resolveRoom(const std::string& roomId, const std::string& clientIp)
{
	if (!registry_) return {{"wsUrl", ""}, {"isNew", true}};
	try {
		auto result = registry_->resolveRoom(roomId, clientIp);
		if (result.isNew && result.wsUrl.empty())
			return {{"error", "no available nodes"}, {"wsUrl", ""}, {"isNew", true}};
		return {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
	} catch (const std::exception& e) {
		MS_WARN(logger_, "resolveRoom failed ({}), degrading to local", e.what());
		return {{"wsUrl", ""}, {"isNew", true}};
	}
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
