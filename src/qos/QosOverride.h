#pragma once
#include "qos/QosTypes.h"
#include <optional>

namespace mediasoup::qos {

struct AutomaticQosOverride {
	QosOverride overrideData;
	std::string signature;
};

class QosOverrideBuilder {
public:
	static std::optional<AutomaticQosOverride> BuildForAggregate(
		const PeerQosAggregate& aggregate);
	static std::optional<AutomaticQosOverride> BuildForRoomAggregate(
		const RoomQosAggregate& aggregate);
};

} // namespace mediasoup::qos
