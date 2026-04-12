#pragma once
#include "qos/QosTypes.h"

namespace mediasoup::qos {

int64_t NowMs();

json ToJson(const ClientQosSnapshot& snapshot);
json ToJson(const QosPolicy& policy);
json ToJson(const QosOverride& overrideData);
json ToJson(const PeerQosAggregate& aggregate);

} // namespace mediasoup::qos
