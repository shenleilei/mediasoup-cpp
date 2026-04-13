#include "qos/PublisherSupplyController.h"

namespace mediasoup::qos {
namespace {

QosOverride MakeTrackClamp(const std::string& trackId, uint32_t maxLevelClamp,
	uint32_t ttlMs, const std::string& reason)
{
	QosOverride o;
	o.schema = "mediasoup.qos.override.v1";
	o.scope = "track";
	o.trackId = trackId;
	o.hasTrackId = true;
	o.hasMaxLevelClamp = true;
	o.maxLevelClamp = maxLevelClamp;
	o.ttlMs = ttlMs;
	o.reason = reason;
	o.raw = {
		{"schema", o.schema}, {"scope", "track"}, {"trackId", trackId},
		{"maxLevelClamp", maxLevelClamp}, {"ttlMs", ttlMs}, {"reason", reason}
	};
	return o;
}

QosOverride MakeTrackClear(const std::string& trackId, const std::string& reason) {
	QosOverride o;
	o.schema = "mediasoup.qos.override.v1";
	o.scope = "track";
	o.trackId = trackId;
	o.hasTrackId = true;
	o.hasMaxLevelClamp = false;
	o.ttlMs = 0;
	o.reason = reason;
	o.raw = {
		{"schema", o.schema}, {"scope", "track"}, {"trackId", trackId},
		{"maxLevelClamp", nullptr}, {"ttlMs", 0}, {"reason", reason}
	};
	return o;
}

} // namespace

bool PublisherSupplyController::shouldClamp(const ProducerDemandState& state) const {
	// Clamp when at least one subscriber is visible but not requesting the highest layer
	return state.visibleSubscriberCount > 0 && state.maxSpatialLayer < 2;
}

std::vector<TargetedOverride> PublisherSupplyController::BuildOverrides(
	const std::vector<ProducerDemandState>& states,
	const ResolveTrackFn& resolveTrack,
	int64_t nowMs) const
{
	std::vector<TargetedOverride> overrides;
	for (auto& state : states) {
		auto resolved = resolveTrack(state.producerId);
		if (!resolved) continue;
		auto& [peerId, trackId] = *resolved;

		if (shouldClamp(state)) {
			overrides.push_back({ peerId, MakeTrackClamp(
				trackId, state.maxSpatialLayer, 5000u, "downlink_v2_room_demand") });
		} else if (state.visibleSubscriberCount == 0 && state.holdUntilMs > 0 && nowMs < state.holdUntilMs) {
			overrides.push_back({ peerId, MakeTrackClamp(
				trackId, 0, 5000u, "downlink_v2_zero_demand_hold") });
		} else if (state.visibleSubscriberCount > 0 && state.maxSpatialLayer >= 2) {
			overrides.push_back({ peerId, MakeTrackClear(trackId, "downlink_v2_demand_restored") });
		}
	}
	return overrides;
}

} // namespace mediasoup::qos
