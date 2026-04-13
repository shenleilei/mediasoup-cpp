#pragma once
#include "qos/ProducerDemandAggregator.h"
#include "qos/QosTypes.h"
#include <vector>

namespace mediasoup::qos {

struct TargetedOverride {
	std::string targetPeerId;
	QosOverride overrideData;
};

class PublisherSupplyController {
public:
	/// resolveTrack: producerId -> (peerId, trackId) or nullopt
	using ResolveTrackFn = std::function<
		std::optional<std::pair<std::string, std::string>>(const std::string& producerId)>;

	std::vector<TargetedOverride> BuildOverrides(
		const std::vector<ProducerDemandState>& states,
		const ResolveTrackFn& resolveTrack,
		int64_t nowMs) const;

private:
	bool shouldClamp(const ProducerDemandState& state) const;
};

} // namespace mediasoup::qos
