#pragma once
#include "QosTypes.h"
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace qos {

using json = nlohmann::json;

inline const json& parseClientQosSnapshot(const json& payload) {
	if (!payload.is_object()) throw std::invalid_argument("clientQosSnapshot must be an object");
	if (!payload.contains("schema") || !payload["schema"].is_string() || payload["schema"].get<std::string>() != "mediasoup.qos.client.v1")
		throw std::invalid_argument("clientQosSnapshot.schema must be mediasoup.qos.client.v1");
	if (!payload.contains("seq") || !payload["seq"].is_number_integer() || payload["seq"].get<int64_t>() < 0)
		throw std::invalid_argument("clientQosSnapshot.seq must be a non-negative integer");
	if (!payload.contains("tsMs") || !payload["tsMs"].is_number() || payload["tsMs"].get<double>() < 0)
		throw std::invalid_argument("clientQosSnapshot.tsMs must be a non-negative number");
	if (!payload.contains("peerState") || !payload["peerState"].is_object())
		throw std::invalid_argument("clientQosSnapshot.peerState must be an object");
	if (!payload.contains("tracks") || !payload["tracks"].is_array())
		throw std::invalid_argument("clientQosSnapshot.tracks must be an array");
	if (payload["tracks"].size() > kQosMaxTracksPerSnapshot)
		throw std::invalid_argument("clientQosSnapshot.tracks length exceeds supported maximum");
	return payload;
}

inline bool isClientQosSnapshot(const json& payload) {
	try {
		parseClientQosSnapshot(payload);
		return true;
	} catch (...) {
		return false;
	}
}

inline json serializeClientQosSnapshot(const json& snapshot) {
	return parseClientQosSnapshot(snapshot);
}

inline QosPolicy parseQosPolicy(const json& payload) {
	if (!payload.is_object()) throw std::invalid_argument("qosPolicy must be an object");
	if (!payload.contains("schema") || !payload["schema"].is_string() || payload["schema"].get<std::string>() != "mediasoup.qos.policy.v1")
		throw std::invalid_argument("qosPolicy.schema must be mediasoup.qos.policy.v1");
	if (!payload.contains("sampleIntervalMs") || !payload["sampleIntervalMs"].is_number_integer() || payload["sampleIntervalMs"].get<int>() < 0)
		throw std::invalid_argument("qosPolicy.sampleIntervalMs must be a non-negative integer");
	if (!payload.contains("snapshotIntervalMs") || !payload["snapshotIntervalMs"].is_number_integer() || payload["snapshotIntervalMs"].get<int>() < 0)
		throw std::invalid_argument("qosPolicy.snapshotIntervalMs must be a non-negative integer");
	if (!payload.contains("allowAudioOnly") || !payload["allowAudioOnly"].is_boolean())
		throw std::invalid_argument("qosPolicy.allowAudioOnly must be a boolean");
	if (!payload.contains("allowVideoPause") || !payload["allowVideoPause"].is_boolean())
		throw std::invalid_argument("qosPolicy.allowVideoPause must be a boolean");
	if (!payload.contains("profiles") || !payload["profiles"].is_object())
		throw std::invalid_argument("qosPolicy.profiles must be an object");
	const auto& profiles = payload["profiles"];
	if (!profiles.contains("camera") || !profiles["camera"].is_string() || profiles["camera"].get<std::string>().empty())
		throw std::invalid_argument("qosPolicy.profiles.camera must be a non-empty string");
	if (!profiles.contains("screenShare") || !profiles["screenShare"].is_string() || profiles["screenShare"].get<std::string>().empty())
		throw std::invalid_argument("qosPolicy.profiles.screenShare must be a non-empty string");
	if (!profiles.contains("audio") || !profiles["audio"].is_string() || profiles["audio"].get<std::string>().empty())
		throw std::invalid_argument("qosPolicy.profiles.audio must be a non-empty string");

	QosPolicy policy;
	policy.sampleIntervalMs = payload["sampleIntervalMs"].get<int>();
	policy.snapshotIntervalMs = payload["snapshotIntervalMs"].get<int>();
	policy.allowAudioOnly = payload["allowAudioOnly"].get<bool>();
	policy.allowVideoPause = payload["allowVideoPause"].get<bool>();
	policy.profiles.camera = profiles["camera"].get<std::string>();
	policy.profiles.screenShare = profiles["screenShare"].get<std::string>();
	policy.profiles.audio = profiles["audio"].get<std::string>();
	return policy;
}

inline bool isQosPolicy(const json& payload) {
	try {
		parseQosPolicy(payload);
		return true;
	} catch (...) {
		return false;
	}
}

inline QosOverride parseQosOverride(const json& payload) {
	if (!payload.is_object()) throw std::invalid_argument("qosOverride must be an object");
	if (!payload.contains("schema") || !payload["schema"].is_string() || payload["schema"].get<std::string>() != "mediasoup.qos.override.v1")
		throw std::invalid_argument("qosOverride.schema must be mediasoup.qos.override.v1");
	if (!payload.contains("scope") || !payload["scope"].is_string())
		throw std::invalid_argument("qosOverride.scope must be a string");

	QosOverride overrideData;
	const auto scope = payload["scope"].get<std::string>();
	if (scope == "peer") {
		overrideData.scope = OverrideScope::Peer;
	} else if (scope == "track") {
		overrideData.scope = OverrideScope::Track;
	} else {
		throw std::invalid_argument("qosOverride.scope must be peer or track");
	}
	if (overrideData.scope == OverrideScope::Track
		&& (!payload.contains("trackId") || payload["trackId"].is_null())) {
		throw std::invalid_argument("qosOverride.trackId is required when scope is track");
	}

	if (payload.contains("trackId") && !payload["trackId"].is_null()) {
		if (!payload["trackId"].is_string() || payload["trackId"].get<std::string>().empty())
			throw std::invalid_argument("qosOverride.trackId must be a non-empty string");
		overrideData.trackId = payload["trackId"].get<std::string>();
	}
	if (payload.contains("maxLevelClamp")) {
		if (!payload["maxLevelClamp"].is_number_integer() || payload["maxLevelClamp"].get<int>() < 0)
			throw std::invalid_argument("qosOverride.maxLevelClamp must be a non-negative integer");
		overrideData.maxLevelClamp = payload["maxLevelClamp"].get<int>();
	}
	if (payload.contains("forceAudioOnly")) {
		if (!payload["forceAudioOnly"].is_boolean()) throw std::invalid_argument("qosOverride.forceAudioOnly must be a boolean");
		overrideData.forceAudioOnly = payload["forceAudioOnly"].get<bool>();
	}
	if (payload.contains("disableRecovery")) {
		if (!payload["disableRecovery"].is_boolean()) throw std::invalid_argument("qosOverride.disableRecovery must be a boolean");
		overrideData.disableRecovery = payload["disableRecovery"].get<bool>();
	}
	if (payload.contains("pauseUpstream")) {
		if (!payload["pauseUpstream"].is_boolean()) throw std::invalid_argument("qosOverride.pauseUpstream must be a boolean");
		overrideData.pauseUpstream = payload["pauseUpstream"].get<bool>();
	}
	if (payload.contains("resumeUpstream")) {
		if (!payload["resumeUpstream"].is_boolean()) throw std::invalid_argument("qosOverride.resumeUpstream must be a boolean");
		overrideData.resumeUpstream = payload["resumeUpstream"].get<bool>();
	}
	if (overrideData.pauseUpstream.value_or(false) && overrideData.resumeUpstream.value_or(false))
		throw std::invalid_argument("qosOverride.pauseUpstream and qosOverride.resumeUpstream are mutually exclusive");
	if (!payload.contains("ttlMs") || !payload["ttlMs"].is_number_integer() || payload["ttlMs"].get<int>() < 0)
		throw std::invalid_argument("qosOverride.ttlMs must be a non-negative integer");
	if (!payload.contains("reason") || !payload["reason"].is_string() || payload["reason"].get<std::string>().empty())
		throw std::invalid_argument("qosOverride.reason must be a non-empty string");

	overrideData.ttlMs = payload["ttlMs"].get<int>();
	overrideData.reason = payload["reason"].get<std::string>();
	return overrideData;
}

inline bool isQosOverride(const json& payload) {
	try {
		parseQosOverride(payload);
		return true;
	} catch (...) {
		return false;
	}
}

inline const char* qualityLimitationReasonStr(QualityLimitationReason r) {
	switch (r) {
		case QualityLimitationReason::Bandwidth: return "bandwidth";
		case QualityLimitationReason::Cpu: return "cpu";
		case QualityLimitationReason::Other: return "other";
		case QualityLimitationReason::None: return "none";
		case QualityLimitationReason::Unknown: return "unknown";
	}
	return "unknown";
}

inline const char* resolvePeerMode(const std::vector<PeerTrackState>& tracks, bool peerHasAudioTrack) {
	bool hasVideoTrack = false;
	bool hasAudioTrack = peerHasAudioTrack;
	bool audioOnly = tracks.empty();
	for (const auto& track : tracks) {
		hasVideoTrack = hasVideoTrack || track.kind == TrackKind::Video;
		hasAudioTrack = hasAudioTrack || track.kind == TrackKind::Audio;
		audioOnly = audioOnly || (track.kind == TrackKind::Video && track.inAudioOnlyMode);
	}
	if (audioOnly) return "audio-only";
	if (hasVideoTrack && hasAudioTrack) return "audio-video";
	if (hasVideoTrack) return "video-only";
	return "audio-only";
}

inline json serializeSnapshot(int seq, int64_t tsMs, Quality peerQuality, bool stale,
	const std::vector<PeerTrackState>& tracks, const std::map<std::string, DerivedSignals>& trackSignals,
	const std::map<std::string, PlannedAction>& lastActions,
	const std::map<std::string, RawSenderSnapshot>& rawSnapshots = {},
	const std::map<std::string, bool>& actionApplied = {},
	bool peerHasAudioTrack = true)
{
	json tracksArr = json::array();
	if (tracks.size() > kQosMaxTracksPerSnapshot)
		throw std::invalid_argument("serializeSnapshot supports at most 32 tracks");
	for (auto& t : tracks) {
		json tj = {
			{"localTrackId", t.trackId}, {"producerId", t.producerId},
			{"kind", t.kind == TrackKind::Video ? "video" : "audio"},
			{"source", sourceStr(t.source)}, {"state", stateStr(t.state)},
			{"reason", reasonStr(t.reason)},
			{"quality", qualityStr(t.quality)}, {"ladderLevel", t.level},
		};
		json sig = {};
		auto sit = trackSignals.find(t.trackId);
		if (sit != trackSignals.end()) {
			sig["sendBitrateBps"] = sit->second.sendBitrateBps;
			sig["targetBitrateBps"] = sit->second.targetBitrateBps;
			sig["bitrateUtilization"] = sit->second.bitrateUtilization;
			sig["lossRate"] = sit->second.lossRate;
			sig["lossEwma"] = sit->second.lossEwma;
			sig["rttMs"] = sit->second.rttMs;
			sig["rttEwma"] = sit->second.rttEwma;
			sig["jitterMs"] = sit->second.jitterMs;
			sig["jitterEwma"] = sit->second.jitterEwma;
		}
		auto rit = rawSnapshots.find(t.trackId);
		if (rit != rawSnapshots.end()) {
			sig["frameWidth"] = rit->second.frameWidth;
			sig["frameHeight"] = rit->second.frameHeight;
			sig["framesPerSecond"] = rit->second.framesPerSecond;
			sig["qualityLimitationReason"] = qualityLimitationReasonStr(rit->second.qualityLimitationReason);
		}
		tj["signals"] = sig;
		auto ait = lastActions.find(t.trackId);
		if (ait != lastActions.end()) {
			auto apit = actionApplied.find(t.trackId);
			bool applied = apit != actionApplied.end() ? apit->second : true;
			tj["lastAction"] = {{"type", actionStr(ait->second.type)}, {"applied", applied}};
		}
		tracksArr.push_back(tj);
	}

	const char* peerMode = resolvePeerMode(tracks, peerHasAudioTrack);

	return {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", seq}, {"tsMs", tsMs},
		{"peerState", {
			{"mode", peerMode},
			{"quality", qualityStr(peerQuality)}, {"stale", stale}
		}},
		{"tracks", tracksArr}
	};
}

} // namespace qos
