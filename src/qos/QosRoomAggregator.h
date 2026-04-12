#pragma once
#include "qos/QosTypes.h"
#include <vector>

namespace mediasoup::qos {

class QosRoomAggregator {
public:
	static RoomQosAggregate Aggregate(const std::vector<PeerQosAggregate>& peers);
};

} // namespace mediasoup::qos
