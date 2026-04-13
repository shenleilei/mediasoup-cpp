#pragma once
#include "qos/DownlinkQosTypes.h"
#include "qos/QosTypes.h"

namespace mediasoup::qos {

class QosValidator {
public:
	static ParseResult<ClientQosSnapshot> ParseClientSnapshot(const json& payload);
	static ParseResult<QosPolicy> ParsePolicy(const json& payload);
	static ParseResult<QosOverride> ParseOverride(const json& payload);
	static ParseResult<DownlinkSnapshot> ParseDownlinkSnapshot(const json& payload);
};

} // namespace mediasoup::qos
