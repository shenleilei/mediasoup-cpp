#pragma once

#include "qos/DownlinkQosRegistry.h"
#include "qos/QosRegistry.h"
#include <algorithm>
#include <deque>
#include <string>

namespace mediasoup::roomcleanup {

template<typename Map>
void EraseKeysByPrefix(Map& map, const std::string& prefix)
{
	for (auto it = map.begin(); it != map.end(); ) {
		if (it->first.compare(0, prefix.size(), prefix) == 0) {
			it = map.erase(it);
		} else {
			++it;
		}
	}
}

inline void ErasePendingRoom(std::deque<std::string>& pendingRooms, const std::string& roomId)
{
	pendingRooms.erase(
		std::remove(pendingRooms.begin(), pendingRooms.end(), roomId),
		pendingRooms.end());
}

template<typename SubscriberControllerMap,
	typename AutoOverrideMap,
	typename LastConnectionQualityMap,
	typename LastRoomQosStateMap,
	typename DirtyRoomSet,
	typename DownlinkRoomPlanStateMap,
	typename TrackOverrideMap,
	typename ProducerDemandCacheMap,
	typename ProducerOwnerPeerIdsMap>
void CleanupRoomServiceState(
	const std::string& roomId,
	qos::QosRegistry& qosRegistry,
	qos::DownlinkQosRegistry& downlinkQosRegistry,
	SubscriberControllerMap& subscriberControllers,
	AutoOverrideMap& autoQosOverrideRecords,
	LastConnectionQualityMap& lastConnectionQualitySignatures,
	LastRoomQosStateMap& lastRoomQosStateSignatures,
	DirtyRoomSet& dirtyDownlinkRooms,
	std::deque<std::string>& pendingDownlinkRooms,
	DownlinkRoomPlanStateMap& downlinkRoomPlanStates,
	TrackOverrideMap& trackQosOverrideRecords,
	ProducerDemandCacheMap& producerDemandCache,
	ProducerOwnerPeerIdsMap& producerOwnerPeerIds)
{
	const std::string prefix = roomId + "/";
	qosRegistry.EraseRoom(roomId);
	downlinkQosRegistry.EraseRoom(roomId);
	EraseKeysByPrefix(subscriberControllers, prefix);
	EraseKeysByPrefix(autoQosOverrideRecords, prefix);
	EraseKeysByPrefix(lastConnectionQualitySignatures, prefix);
	lastRoomQosStateSignatures.erase(roomId);
	dirtyDownlinkRooms.erase(roomId);
	ErasePendingRoom(pendingDownlinkRooms, roomId);
	downlinkRoomPlanStates.erase(roomId);

	for (auto it = trackQosOverrideRecords.begin(); it != trackQosOverrideRecords.end(); ) {
		if (it->second.roomId == roomId) {
			it = trackQosOverrideRecords.erase(it);
		} else {
			++it;
		}
	}

	producerDemandCache.erase(roomId);
	producerOwnerPeerIds.erase(roomId);
}

} // namespace mediasoup::roomcleanup
