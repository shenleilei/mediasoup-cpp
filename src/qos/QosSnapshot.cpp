#include "qos/QosSnapshot.h"
#include <chrono>

namespace mediasoup::qos {

int64_t NowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

json ToJson(const ClientQosSnapshot& snapshot) {
	return snapshot.raw;
}

json ToJson(const DownlinkSnapshot& snapshot) {
	return snapshot.raw;
}

json ToJson(const QosPolicy& policy) {
	return policy.raw;
}

json ToJson(const QosOverride& overrideData) {
	return overrideData.raw;
}

json ToJson(const PeerQosAggregate& aggregate) {
	return aggregate.data;
}

} // namespace mediasoup::qos
