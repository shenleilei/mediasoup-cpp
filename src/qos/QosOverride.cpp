#include "qos/QosOverride.h"

namespace mediasoup::qos {
namespace {

QosOverride MakeOverride(
	const std::string& reason, uint32_t ttlMs, std::optional<uint32_t> maxLevelClamp,
	std::optional<bool> forceAudioOnly, std::optional<bool> disableRecovery)
{
	QosOverride overrideData;
	overrideData.schema = contract::kOverrideSchema;
	overrideData.scope = "peer";
	overrideData.ttlMs = ttlMs;
	overrideData.reason = reason;
	overrideData.raw = {
		{"schema", overrideData.schema},
		{"scope", overrideData.scope},
		{"trackId", nullptr},
		{"ttlMs", ttlMs},
		{"reason", reason}
	};

	if (maxLevelClamp.has_value()) {
		overrideData.hasMaxLevelClamp = true;
		overrideData.maxLevelClamp = *maxLevelClamp;
		overrideData.raw["maxLevelClamp"] = *maxLevelClamp;
	}
	if (forceAudioOnly.has_value()) {
		overrideData.hasForceAudioOnly = true;
		overrideData.forceAudioOnly = *forceAudioOnly;
		overrideData.raw["forceAudioOnly"] = *forceAudioOnly;
	}
	if (disableRecovery.has_value()) {
		overrideData.hasDisableRecovery = true;
		overrideData.disableRecovery = *disableRecovery;
		overrideData.raw["disableRecovery"] = *disableRecovery;
	}

	return overrideData;
}

std::string BuildSignature(const QosOverride& overrideData) {
	return overrideData.raw.dump();
}

} // namespace

std::optional<AutomaticQosOverride> QosOverrideBuilder::BuildForAggregate(
	const PeerQosAggregate& aggregate)
{
	if (!aggregate.hasSnapshot) {
		return std::nullopt;
	}

	if (aggregate.lost || aggregate.quality == "lost") {
		auto overrideData = MakeOverride(
			"server_auto_lost",
			15000u,
			std::nullopt,
			true,
			true
		);
		return AutomaticQosOverride{ overrideData, BuildSignature(overrideData) };
	}

	if (aggregate.quality == "poor") {
		auto overrideData = MakeOverride(
			"server_auto_poor",
			10000u,
			2u,
			std::nullopt,
			true
		);
		return AutomaticQosOverride{ overrideData, BuildSignature(overrideData) };
	}

	return std::nullopt;
}

std::optional<AutomaticQosOverride> QosOverrideBuilder::BuildForRoomAggregate(
	const RoomQosAggregate& aggregate)
{
	if (!aggregate.hasQos) {
		return std::nullopt;
	}

	if (aggregate.lostPeers > 0u || aggregate.poorPeers >= 2u) {
		auto overrideData = MakeOverride(
			"server_room_pressure",
			8000u,
			1u,
			std::nullopt,
			true
		);
		return AutomaticQosOverride{ overrideData, BuildSignature(overrideData) };
	}

	return std::nullopt;
}

} // namespace mediasoup::qos
