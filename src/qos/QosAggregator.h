#pragma once
#include "qos/QosRegistry.h"

namespace mediasoup::qos {

class QosAggregator {
public:
	static PeerQosAggregate Aggregate(const QosRegistry::Entry* entry,
		int64_t nowMs = NowMs(), int64_t staleAfterMs = kQosStaleAfterMs,
		int64_t lostAfterMs = kQosLostAfterMs);
};

} // namespace mediasoup::qos
