#include "qos/QosValidator.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace mediasoup::qos {

namespace {
constexpr const char* kCanonicalDownlinkSchema = "mediasoup.qos.downlink.client.v1";
constexpr const char* kLegacyDownlinkSchema = "mediasoup.downlink.v1";
}
namespace {

template<typename T>
ParseResult<T> MakeError(const std::string& error) {
	ParseResult<T> result;
	result.ok = false;
	result.error = error;

	return result;
}

template<typename T>
ParseResult<T> MakeSuccess(T value) {
	ParseResult<T> result;
	result.ok = true;
	result.value = std::move(value);

	return result;
}

bool IsNonEmptyString(const json& value) {
	return value.is_string() && !value.get_ref<const std::string&>().empty();
}

bool IsFiniteNumber(const json& value) {
	if (value.is_number_float()) {
		return std::isfinite(value.get<double>());
	}

	return value.is_number_integer() || value.is_number_unsigned();
}

bool IsNonNegativeNumber(const json& value) {
	return IsFiniteNumber(value) && value.get<double>() >= 0.0;
}

bool IsNonNegativeInteger(const json& value) {
	if (!(value.is_number_integer() || value.is_number_unsigned())) {
		return false;
	}

	return value.get<int64_t>() >= 0;
}

bool OneOf(const std::string& value, const std::initializer_list<const char*> allowed) {
	return std::any_of(allowed.begin(), allowed.end(), [&value](const char* candidate) {
		return value == candidate;
	});
}

template<typename T>
bool RequireObjectField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !object[key].is_object()) {
		errorResult = MakeError<T>(std::string("missing or invalid object field: ") + key);
		return false;
	}

	return true;
}

template<typename T>
bool RequireArrayField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !object[key].is_array()) {
		errorResult = MakeError<T>(std::string("missing or invalid array field: ") + key);
		return false;
	}

	return true;
}

template<typename T>
bool RequireStringField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !IsNonEmptyString(object[key])) {
		errorResult = MakeError<T>(std::string("missing or invalid string field: ") + key);
		return false;
	}

	return true;
}

template<typename T>
bool RequireBoolField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !object[key].is_boolean()) {
		errorResult = MakeError<T>(std::string("missing or invalid bool field: ") + key);
		return false;
	}

	return true;
}

template<typename T>
bool RequireNonNegativeNumberField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !IsNonNegativeNumber(object[key])) {
		errorResult = MakeError<T>(std::string("missing or invalid non-negative number field: ") + key);
		return false;
	}

	return true;
}

template<typename T>
bool RequireNonNegativeIntegerField(const json& object, const char* key, ParseResult<T>& errorResult) {
	if (!object.contains(key) || !IsNonNegativeInteger(object[key])) {
		errorResult = MakeError<T>(std::string("missing or invalid non-negative integer field: ") + key);
		return false;
	}

	return true;
}

ParseResult<ClientQosTrackSnapshot> ParseTrack(const json& trackJson) {
	ParseResult<ClientQosTrackSnapshot> errorResult;

	if (!trackJson.is_object()) {
		return MakeError<ClientQosTrackSnapshot>("track must be an object");
	}

	if (!RequireStringField(trackJson, "localTrackId", errorResult)) return errorResult;
	if (!RequireStringField(trackJson, "kind", errorResult)) return errorResult;
	if (!RequireStringField(trackJson, "source", errorResult)) return errorResult;
	if (!RequireStringField(trackJson, "state", errorResult)) return errorResult;
	if (!RequireStringField(trackJson, "reason", errorResult)) return errorResult;
	if (!RequireStringField(trackJson, "quality", errorResult)) return errorResult;
	if (!RequireNonNegativeIntegerField(trackJson, "ladderLevel", errorResult)) return errorResult;
	if (!RequireObjectField(trackJson, "signals", errorResult)) return errorResult;

	const auto& kind = trackJson["kind"].get_ref<const std::string&>();
	const auto& source = trackJson["source"].get_ref<const std::string&>();
	const auto& state = trackJson["state"].get_ref<const std::string&>();
	const auto& reason = trackJson["reason"].get_ref<const std::string&>();
	const auto& quality = trackJson["quality"].get_ref<const std::string&>();

	if (!OneOf(kind, { "audio", "video" })) {
		return MakeError<ClientQosTrackSnapshot>("invalid track kind");
	}
	if (!OneOf(source, { "camera", "screenShare", "audio" })) {
		return MakeError<ClientQosTrackSnapshot>("invalid track source");
	}
	if (!OneOf(state, { "stable", "early_warning", "congested", "recovering" })) {
		return MakeError<ClientQosTrackSnapshot>("invalid track state");
	}
	if (!OneOf(reason, { "network", "cpu", "manual", "server_override", "unknown" })) {
		return MakeError<ClientQosTrackSnapshot>("invalid track reason");
	}
	if (!OneOf(quality, { "excellent", "good", "poor", "lost" })) {
		return MakeError<ClientQosTrackSnapshot>("invalid track quality");
	}

	const auto& signals = trackJson["signals"];
	if (!RequireNonNegativeNumberField(signals, "sendBitrateBps", errorResult)) return errorResult;
	if (signals.contains("targetBitrateBps") && !IsNonNegativeNumber(signals["targetBitrateBps"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.targetBitrateBps");
	}
	if (signals.contains("lossRate") && !IsNonNegativeNumber(signals["lossRate"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.lossRate");
	}
	if (signals.contains("rttMs") && !IsNonNegativeNumber(signals["rttMs"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.rttMs");
	}
	if (signals.contains("jitterMs") && !IsNonNegativeNumber(signals["jitterMs"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.jitterMs");
	}
	if (signals.contains("frameWidth") && !IsNonNegativeInteger(signals["frameWidth"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.frameWidth");
	}
	if (signals.contains("frameHeight") && !IsNonNegativeInteger(signals["frameHeight"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.frameHeight");
	}
	if (signals.contains("framesPerSecond") && !IsNonNegativeNumber(signals["framesPerSecond"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid signals.framesPerSecond");
	}
	if (signals.contains("qualityLimitationReason")) {
		if (!IsNonEmptyString(signals["qualityLimitationReason"])) {
			return MakeError<ClientQosTrackSnapshot>("invalid signals.qualityLimitationReason");
		}
		const auto& limitation = signals["qualityLimitationReason"].get_ref<const std::string&>();
		if (!OneOf(limitation, { "bandwidth", "cpu", "other", "none", "unknown" })) {
			return MakeError<ClientQosTrackSnapshot>("invalid signals.qualityLimitationReason");
		}
	}

	if (trackJson.contains("producerId") && !trackJson["producerId"].is_null() &&
		!IsNonEmptyString(trackJson["producerId"])) {
		return MakeError<ClientQosTrackSnapshot>("invalid producerId");
	}

	if (trackJson.contains("lastAction") && !trackJson["lastAction"].is_null()) {
		const auto& lastAction = trackJson["lastAction"];
		if (!lastAction.is_object()) {
			return MakeError<ClientQosTrackSnapshot>("invalid lastAction");
		}
		if (!RequireStringField(lastAction, "type", errorResult)) return errorResult;
		if (!RequireBoolField(lastAction, "applied", errorResult)) return errorResult;
		const auto& actionType = lastAction["type"].get_ref<const std::string&>();
		if (!OneOf(actionType, {
			"setEncodingParameters", "setMaxSpatialLayer", "enterAudioOnly",
			"exitAudioOnly", "pauseUpstream", "resumeUpstream", "noop"
		})) {
			return MakeError<ClientQosTrackSnapshot>("invalid lastAction.type");
		}
	}

	ClientQosTrackSnapshot track;
	track.localTrackId = trackJson["localTrackId"].get<std::string>();
	track.kind = kind;
	track.source = source;
	track.state = state;
	track.reason = reason;
	track.quality = quality;
	track.ladderLevel = static_cast<uint32_t>(trackJson["ladderLevel"].get<uint64_t>());
	track.signals = signals;
	track.raw = trackJson;
	if (trackJson.contains("producerId") && trackJson["producerId"].is_string()) {
		track.producerId = trackJson["producerId"].get<std::string>();
	}
	if (trackJson.contains("lastAction") && trackJson["lastAction"].is_object()) {
		track.lastAction = trackJson["lastAction"];
	}

	return MakeSuccess(std::move(track));
}

} // namespace

ParseResult<ClientQosSnapshot> QosValidator::ParseClientSnapshot(const json& payload) {
	ParseResult<ClientQosSnapshot> errorResult;

	if (!payload.is_object()) {
		return MakeError<ClientQosSnapshot>("payload must be an object");
	}

	if (!RequireStringField(payload, "schema", errorResult)) return errorResult;
	if (payload["schema"].get_ref<const std::string&>() != "mediasoup.qos.client.v1") {
		return MakeError<ClientQosSnapshot>("invalid schema");
	}
	if (!RequireNonNegativeIntegerField(payload, "seq", errorResult)) return errorResult;
	if (payload["seq"].get<uint64_t>() > kQosMaxSeq) {
		return MakeError<ClientQosSnapshot>("seq exceeds supported range");
	}
	if (!RequireNonNegativeNumberField(payload, "tsMs", errorResult)) return errorResult;
	if (!RequireObjectField(payload, "peerState", errorResult)) return errorResult;
	if (!RequireArrayField(payload, "tracks", errorResult)) return errorResult;

	const auto& peerState = payload["peerState"];
	if (!RequireStringField(peerState, "mode", errorResult)) return errorResult;
	if (!RequireStringField(peerState, "quality", errorResult)) return errorResult;
	if (!RequireBoolField(peerState, "stale", errorResult)) return errorResult;

	const auto& mode = peerState["mode"].get_ref<const std::string&>();
	const auto& quality = peerState["quality"].get_ref<const std::string&>();
	if (!OneOf(mode, { "audio-only", "audio-video", "video-only" })) {
		return MakeError<ClientQosSnapshot>("invalid peerState.mode");
	}
	if (!OneOf(quality, { "excellent", "good", "poor", "lost" })) {
		return MakeError<ClientQosSnapshot>("invalid peerState.quality");
	}

	const auto& tracks = payload["tracks"];
	if (tracks.size() > kQosMaxTracksPerSnapshot) {
		return MakeError<ClientQosSnapshot>("too many tracks in snapshot");
	}

	ClientQosSnapshot snapshot;
	snapshot.schema = payload["schema"].get<std::string>();
	snapshot.seq = payload["seq"].get<uint64_t>();
	snapshot.tsMs = payload["tsMs"].get<int64_t>();
	snapshot.peerMode = mode;
	snapshot.peerQuality = quality;
	snapshot.peerStale = peerState["stale"].get<bool>();
	snapshot.raw = payload;

	for (const auto& trackJson : tracks) {
		auto parsedTrack = ParseTrack(trackJson);
		if (!parsedTrack.ok) {
			return MakeError<ClientQosSnapshot>(parsedTrack.error);
		}
		snapshot.tracks.push_back(std::move(parsedTrack.value));
	}

	return MakeSuccess(std::move(snapshot));
}

ParseResult<QosPolicy> QosValidator::ParsePolicy(const json& payload) {
	ParseResult<QosPolicy> errorResult;

	if (!payload.is_object()) {
		return MakeError<QosPolicy>("payload must be an object");
	}
	if (!RequireStringField(payload, "schema", errorResult)) return errorResult;
	if (payload["schema"].get_ref<const std::string&>() != "mediasoup.qos.policy.v1") {
		return MakeError<QosPolicy>("invalid schema");
	}
	if (!RequireNonNegativeIntegerField(payload, "sampleIntervalMs", errorResult)) return errorResult;
	if (!RequireNonNegativeIntegerField(payload, "snapshotIntervalMs", errorResult)) return errorResult;
	if (!RequireBoolField(payload, "allowAudioOnly", errorResult)) return errorResult;
	if (!RequireBoolField(payload, "allowVideoPause", errorResult)) return errorResult;
	if (!RequireObjectField(payload, "profiles", errorResult)) return errorResult;

	const auto& profiles = payload["profiles"];
	if (!RequireStringField(profiles, "camera", errorResult)) return errorResult;
	if (!RequireStringField(profiles, "screenShare", errorResult)) return errorResult;
	if (!RequireStringField(profiles, "audio", errorResult)) return errorResult;

	QosPolicy policy;
	policy.schema = payload["schema"].get<std::string>();
	policy.sampleIntervalMs = static_cast<uint32_t>(payload["sampleIntervalMs"].get<uint64_t>());
	policy.snapshotIntervalMs = static_cast<uint32_t>(payload["snapshotIntervalMs"].get<uint64_t>());
	policy.allowAudioOnly = payload["allowAudioOnly"].get<bool>();
	policy.allowVideoPause = payload["allowVideoPause"].get<bool>();
	policy.cameraProfile = profiles["camera"].get<std::string>();
	policy.screenShareProfile = profiles["screenShare"].get<std::string>();
	policy.audioProfile = profiles["audio"].get<std::string>();
	policy.raw = payload;

	return MakeSuccess(std::move(policy));
}

ParseResult<QosOverride> QosValidator::ParseOverride(const json& payload) {
	ParseResult<QosOverride> errorResult;

	if (!payload.is_object()) {
		return MakeError<QosOverride>("payload must be an object");
	}
	if (!RequireStringField(payload, "schema", errorResult)) return errorResult;
	if (payload["schema"].get_ref<const std::string&>() != "mediasoup.qos.override.v1") {
		return MakeError<QosOverride>("invalid schema");
	}
	if (!RequireStringField(payload, "scope", errorResult)) return errorResult;
	if (!RequireNonNegativeIntegerField(payload, "ttlMs", errorResult)) return errorResult;
	if (!RequireStringField(payload, "reason", errorResult)) return errorResult;

	const auto& scope = payload["scope"].get_ref<const std::string&>();
	if (!OneOf(scope, { "peer", "track" })) {
		return MakeError<QosOverride>("invalid scope");
	}
	if (scope == "track" &&
		(!payload.contains("trackId") || payload["trackId"].is_null())) {
		return MakeError<QosOverride>("track scope requires trackId");
	}

	QosOverride overrideData;
	overrideData.schema = payload["schema"].get<std::string>();
	overrideData.scope = scope;
	overrideData.ttlMs = static_cast<uint32_t>(payload["ttlMs"].get<uint64_t>());
	overrideData.reason = payload["reason"].get<std::string>();
	overrideData.raw = payload;

	if (payload.contains("trackId")) {
		if (payload["trackId"].is_null()) {
			overrideData.hasTrackId = false;
		} else if (IsNonEmptyString(payload["trackId"])) {
			overrideData.hasTrackId = true;
			overrideData.trackId = payload["trackId"].get<std::string>();
		} else {
			return MakeError<QosOverride>("invalid trackId");
		}
	}

	if (payload.contains("maxLevelClamp")) {
		if (!IsNonNegativeInteger(payload["maxLevelClamp"])) {
			return MakeError<QosOverride>("invalid maxLevelClamp");
		}
		overrideData.hasMaxLevelClamp = true;
		overrideData.maxLevelClamp = static_cast<uint32_t>(payload["maxLevelClamp"].get<uint64_t>());
	}
	if (payload.contains("forceAudioOnly")) {
		if (!payload["forceAudioOnly"].is_boolean()) {
			return MakeError<QosOverride>("invalid forceAudioOnly");
		}
		overrideData.hasForceAudioOnly = true;
		overrideData.forceAudioOnly = payload["forceAudioOnly"].get<bool>();
	}
	if (payload.contains("disableRecovery")) {
		if (!payload["disableRecovery"].is_boolean()) {
			return MakeError<QosOverride>("invalid disableRecovery");
		}
		overrideData.hasDisableRecovery = true;
		overrideData.disableRecovery = payload["disableRecovery"].get<bool>();
	}
	if (payload.contains("pauseUpstream")) {
		if (!payload["pauseUpstream"].is_boolean()) {
			return MakeError<QosOverride>("invalid pauseUpstream");
		}
		overrideData.hasPauseUpstream = true;
		overrideData.pauseUpstream = payload["pauseUpstream"].get<bool>();
	}
	if (payload.contains("resumeUpstream")) {
		if (!payload["resumeUpstream"].is_boolean()) {
			return MakeError<QosOverride>("invalid resumeUpstream");
		}
		overrideData.hasResumeUpstream = true;
		overrideData.resumeUpstream = payload["resumeUpstream"].get<bool>();
	}
	if (overrideData.hasPauseUpstream && overrideData.pauseUpstream &&
		overrideData.hasResumeUpstream && overrideData.resumeUpstream) {
		return MakeError<QosOverride>("pauseUpstream and resumeUpstream are mutually exclusive");
	}

	return MakeSuccess(std::move(overrideData));
}

ParseResult<DownlinkSnapshot> QosValidator::ParseDownlinkSnapshot(const json& payload) {
	ParseResult<DownlinkSnapshot> result;
	try {
		if (!payload.is_object()) {
			result.error = "payload must be object";
			return result;
		}

		// Guard against oversized payloads to prevent memory/bandwidth amplification.
		{
			const auto dumpSize = payload.dump(-1, ' ', false, json::error_handler_t::replace).size();
			if (dumpSize > kDownlinkMaxRawPayloadBytes) {
				result.error = "payload too large";
				return result;
			}
		}

		auto& v = result.value;
		v.schema = payload.value("schema", "");
		if (v.schema != kCanonicalDownlinkSchema && v.schema != kLegacyDownlinkSchema) {
			result.error = "unsupported schema: " + v.schema;
			return result;
		}
		if (v.schema == kLegacyDownlinkSchema) {
			v.schema = kCanonicalDownlinkSchema;
		}
		v.seq = payload.value("seq", uint64_t(0));
		if (v.seq > kDownlinkMaxSeq) {
			result.error = "seq out of range";
			return result;
		}
		v.tsMs = payload.value("tsMs", int64_t(0));
		v.subscriberPeerId = payload.value("subscriberPeerId", "");
		if (v.subscriberPeerId.empty()) {
			result.error = "subscriberPeerId is required";
			return result;
		}
		if (v.subscriberPeerId.size() > kDownlinkMaxIdLength) {
			result.error = "subscriberPeerId too long";
			return result;
		}
		if (!payload.contains("subscriptions") || !payload["subscriptions"].is_array()) {
			result.error = "subscriptions array is required";
			return result;
		}
		auto sanitizeFinite = [](double value, double fallback = 0.0) {
			return std::isfinite(value) ? value : fallback;
		};
		if (payload.contains("transport") && payload["transport"].is_object()) {
			auto& t = payload["transport"];
			v.availableIncomingBitrate = sanitizeFinite(
				t.value("availableIncomingBitrate", 0.0));
			v.currentRoundTripTime = sanitizeFinite(
				t.value("currentRoundTripTime", 0.0));
			// Clamp to sane ranges
			if (v.availableIncomingBitrate < 0.0) v.availableIncomingBitrate = 0.0;
			if (v.currentRoundTripTime < 0.0) v.currentRoundTripTime = 0.0;
			if (v.currentRoundTripTime > 60.0) v.currentRoundTripTime = 60.0;
		}
		if (payload["subscriptions"].size() > kDownlinkMaxSubscriptions) {
			result.error = "too many subscriptions";
			return result;
		}
		std::unordered_set<std::string> seenConsumerIds;
		seenConsumerIds.reserve(payload["subscriptions"].size());
		for (auto& s : payload["subscriptions"]) {
			if (!s.is_object()) {
				result.error = "subscription entry must be object";
				return result;
			}
			DownlinkSubscription sub;
			sub.consumerId = s.value("consumerId", "");
			sub.producerId = s.value("producerId", "");
			sub.kind = s.value("kind", "video");
			if (sub.consumerId.empty() || sub.producerId.empty()) {
				result.error = "subscription consumerId and producerId are required";
				return result;
			}
			if (sub.kind != "audio" && sub.kind != "video") {
				result.error = "subscription kind must be audio or video";
				return result;
			}
			if (sub.consumerId.size() > kDownlinkMaxIdLength ||
				sub.producerId.size() > kDownlinkMaxIdLength) {
				result.error = "subscription id too long";
				return result;
			}
			if (!seenConsumerIds.insert(sub.consumerId).second) {
				result.error = "duplicate subscription consumerId";
				return result;
			}
			sub.visible = s.value("visible", true);
			sub.pinned = s.value("pinned", false);
			sub.activeSpeaker = s.value("activeSpeaker", false);
			sub.isScreenShare = s.value("isScreenShare", false);
			sub.targetWidth = s.value("targetWidth", 0u);
			sub.targetHeight = s.value("targetHeight", 0u);
			sub.packetsLost = s.value("packetsLost", 0u);
			sub.jitter = std::max(0.0, sanitizeFinite(s.value("jitter", 0.0)));
			sub.framesPerSecond = std::max(0.0,
				sanitizeFinite(s.value("framesPerSecond", 0.0)));
			sub.frameWidth = s.value("frameWidth", 0u);
			sub.frameHeight = s.value("frameHeight", 0u);
			sub.freezeRate = std::clamp(
				sanitizeFinite(s.value("freezeRate", 0.0)), 0.0, 1.0);
			v.subscriptions.push_back(std::move(sub));
		}
		v.raw = payload;
		v.raw["schema"] = v.schema;
		result.ok = true;
	} catch (const std::exception& e) {
		result.error = e.what();
	}
	return result;
}

} // namespace mediasoup::qos
